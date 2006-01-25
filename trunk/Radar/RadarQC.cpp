/*
 *  RadarQC.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/28/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "RadarQC.h"
#include "RadarData.h"
#include <stdlib.h>
#include <math.h>
#include <QInputDialog>
#include <QString>
#include "Message.h"


RadarQC::RadarQC(RadarData *radarPtr, QObject *parent)
  :QObject(parent)
{
  radarData = radarPtr;
  specWidthLimit = 10;
  velNull = -999.;
  maxFold = 4;
  numVGatesAveraged = 50;
  vadthr = 30; // maybe these should be in the config
  gvadthr = 180; 
  useVADWinds = false;
  useUserWinds = false;
  useAWIPSWinds = false;
  numCoEff = 3;
  vadLevels = 20;
  deg2rad = acos(-1)/180;
}

RadarQC::~RadarQC()
{

}

void RadarQC::getConfig(QDomElement qcConfig)
{
  /*
   *   Retreves user parameters from the XML configuration file
   */

  // Get Thresholding and BB Parameters

  if(!qcConfig.isNull()) {

    velThresholdLimit = 
      qcConfig.firstChildElement("vel_threshold").text().toFloat();
    specWidthLimit = 
      qcConfig.firstChildElement("sw_threshold").text().toFloat();
    numVGatesAveraged = qcConfig.firstChildElement("bbcount").text().toInt();
    maxFold = qcConfig.firstChildElement("maxfold").text().toInt();

    // Get Information on Environmental Wind Finding Methods

    wind_method = qcConfig.firstChildElement("wind_method").text();

    if(wind_method == QString("user")) {
      useUserWinds = true;
      envWind = new float[1];
      envDir = new float[1];
      envWind[0] = qcConfig.firstChildElement("windspeed").text().toFloat();
      envDir[0] = qcConfig.firstChildElement("winddirection").text().toFloat();
    }
    else {
      if (wind_method == QString("vad")) {

	useVADWinds = true;
	// Possible parameters vadthr, gvadthr
	vadLevels = qcConfig.firstChildElement("vadlevels").text().toInt();
	numCoEff = qcConfig.firstChildElement("numcoeff").text().toInt();
      }
      else {
	
	if(wind_method == QString("known")) {
	  useAWIPSWinds = true;
	  AWIPSDir = qcConfig.firstChildElement("awips_dir").text();
	}
	else {
	  
	  /*
	   * Enable default settings in case environmental wind 
	   * method is not specified
	   */
	  
	  emit log(Message("Environmental Wind Method Not Specified for VAD"));
	  velThresholdLimit = 1;
	  specWidthLimit = 10;
	  numVGatesAveraged = 50;
	  maxFold = 4;
	  useVADWinds = true;
	  vadLevels = 20;
	  numCoEff = 3;
	}
      }
      
    }
  }
  // Set default parameters if configuration data is not available
  else {
    velThresholdLimit = 1;
    specWidthLimit = 10;
    numVGatesAveraged = 50;
    maxFold = 4;
    useVADWinds = true;
  }
}



bool RadarQC::dealias()
{  

  /*
   *  Preforms basic quality control algorithms and velocity dealiasing
   *  on a single radar volume
   *
   */

  if(!terminalVelocity())
    return false;
  
  thresholdData();
  
  findEnvironmentalWind();
  
  if(!BB())
     return false;
  return true;
}



void RadarQC::thresholdData()
{
  /*  
   *  This method iterates through each Ray of the RadarData volume,
   *  and removes velocity data that does not exceed certain spectral
   *  and velocity thresholds. This improves data quality in order to 
   *  improve the preformance of the following quality control routines
   *  This method also calculates the number of valid gate, which is used
   *  in the VAD and GVAD methods.
   */

  int numSweeps = radarData->getNumSweeps();
  validBinCount = new float*[numSweeps];
  for (int i = 0; i < numSweeps; i++) {
    Sweep *currentSweep = radarData->getSweep(i);
    int index = currentSweep->getFirstRay();
    int numBins = radarData->getRay(index)->getVel_numgates();
    validBinCount[i] = new float[numBins]; 
  }

  int numRays = radarData->getNumRays();
  Ray* currentRay = new Ray;
  int numVGates;
  for(int i = 0; i < numRays; i++)
    {
      currentRay = radarData->getRay(i);
      int sweepIndex = currentRay->getSweepIndex();
      numVGates = currentRay->getVel_numgates();
      float *vGates = currentRay->getVelData();
      float *swGates = currentRay->getSwData();
      for (int j = 0; j < numVGates; j++)
	{
	  if((swGates[j] > specWidthLimit)||
	     (fabs(vGates[j]) < velThresholdLimit)) {
	    vGates[j]= velNull;
	  }
	  if(vGates[j]!=velNull) {
	    validBinCount[sweepIndex][j]++;
	  }
	}
    }
}

bool RadarQC::terminalVelocity()
{

  /*
   *  This algorithm is used to remove the terminal velocity component
   *  from each valid doppler velocity reading in the radar volume.
   */


  float ae = 6371*4./3.; // Adjustment factor for 4/3 Earth Radius (in km)
  int numRays = radarData->getNumRays();
  int numVGates;
  Ray* currentRay = new Ray;
  for(int i = 0; i < numRays; i++)
    {
      currentRay = radarData->getRay(i);
      numVGates = currentRay->getVel_numgates();
      float *vGates = currentRay->getVelData();
      float *rGates = currentRay->getRefData();
      if((currentRay->getRef_gatesp()!=0)&&(currentRay->getVel_gatesp()!=0))
	{
	  for(int j = 0; j < numVGates; j++)
	    {
	      if(vGates[j]!=velNull)
		{
		  float range = j/4.0 - 0.375;
		  float elevAngle = currentRay->getElevation();
		  // This height is relative to the height of the radar
		  float height = radarData->radarBeamHeight(range, elevAngle);

		  float rho = 1.1904*exp(-1*height/9.58);
		  float theta = elevAngle+asin(range*cos(deg2rad*elevAngle)
					       /(ae+height))/deg2rad;
		  int zgate = int(range)+1;
		  if(zgate==1)
		    zgate = 2;
		  float zData = rGates[zgate];
		  zData = pow(10,(zData/10));
		  float terminalV = -2.6*sin(deg2rad*theta)
		    *(pow(zData,0.107))*(pow((1.1904/rho),0.45));

		  vGates[j] = vGates[j]-terminalV;
		}
	    }
	}
      else {
      	for(int j = 0; j < numVGates; j++) {
      	  vGates[j] = velNull;
      	}
      }
    }
  return true;
}

float RadarQC::findHeight(Ray *currentRay, int gateIndex)
{

  /*
   *  The method calculates the height of gate in a ray, 
   *  relative to the absolute height of the radar.
   */

  float range = gateIndex/4.0 - 0.375;
  float elevAngle = currentRay->getElevation();
  // This height is relative to the height of the radar
  float height = radarData->radarBeamHeight(range, elevAngle);
  return height;
}

bool RadarQC::findEnvironmentalWind()
{

  /*
   *   Provides environmental wind according to user specified methods
   */

  /*
  if(useAWIPSWinds) {
    //not implemented
  }
  */
  if(useUserWinds) {
    // Do nothing move straight to BB
  }

  if(useVADWinds) {
    bool useGVAD = false;
    if(findVADStart(useGVAD)) 
      return true;
    else {
     if(findVADStart(!useGVAD))
       return true;
     else
       return false;
    }
  }

  return false;
}

bool RadarQC::findVADStart(bool useGVAD)
{

  // This method is used to run either the VAD or GVAD algorithms including
  // any intiailization or preparation algoriths

  int numSweeps = radarData->getNumSweeps();
  envWind = new float[vadLevels];
  envDir = new float[vadLevels];
  vadFound = new bool[vadLevels];
  sumwt = new float[vadLevels];
  vadRMS = new float[vadLevels];  // does this ever get used??
  last_count_up = new float*[vadLevels];
  last_count_low = new float*[vadLevels];
  highVelGate = new int*[vadLevels];
  lowVelGate = new int*[vadLevels];
  hasVelData = new bool*[vadLevels];

  for(int m = 0; m < vadLevels; m++) {
    // create dynamic arrays for VAD wind finder

    last_count_up[m] = new float[numSweeps];
    last_count_low[m] = new float[numSweeps];
    highVelGate[m] = new int[numSweeps];
    lowVelGate[m] = new int[numSweeps];
    hasVelData[m] = new bool[numSweeps];
    
    // initialize many arrays
    vadFound[m] = false;
    sumwt[m] = 0;
    envWind[m] = 0;
    envDir[m] = 0;
    vadRMS[m] = 0;
    for(int n = 0; n < numSweeps; n++) {
      last_count_up[m][n]= 5000;
      last_count_low[m][n] = 5000;
      highVelGate[m][n] = 0;
      lowVelGate[m][n] = 0;
      hasVelData[m][n] = false;
    }
    
  }

  //grad_vad = true; // who gets this??
  //vad_found = false; // who gets this??
  //gvad_found = false; // who gets this??
  //pass = 0; // who gets this??
  
  //for loop begins??  //this was all in code i don't know if it is valuble 
  //pass += 1;         // need to figure that out.
  //if(grad_vad)
  //  thr = gvadthr;
  //else
  //  thr = vadthr;
  //novad = false;
  if(useGVAD)
    thr = gvadthr;
  else
    thr = vadthr;

  vadPrep();

  float **lowSpeed, **highSpeed, **lowDir, **highDir;
  float **lowRMS, **highRMS;
  lowSpeed = new float*[vadLevels];
  highSpeed = new float*[vadLevels];
  lowDir = new float*[vadLevels];
  highDir = new float*[vadLevels];
  lowRMS = new float*[vadLevels];
  highRMS = new float*[vadLevels];

  for(int m = 0; m < vadLevels; m++) {
    lowSpeed[m] = new float[numSweeps];
    highSpeed[m] = new float[numSweeps];
    lowDir[m] = new float[numSweeps];
    highDir[m] = new float[numSweeps];
    lowRMS[m] = new float[numSweeps];
    highRMS[m] = new float[numSweeps];
  }

  for(int m = 0; m < vadLevels; m++) {
    for(int n = 0; n < numSweeps; n++) {
	
      float speedl = velNull;
      float speedu = velNull;
      float dirl = velNull;
      float diru = velNull;
      float rmsl = velNull;
      float rmsu = velNull;

      if(hasVelData[m][n]) {
	Sweep* currentSweep = radarData->getSweep(n);
	int numRays = currentSweep->getNumRays();
	int start = currentSweep->getFirstRay();
	int stop = currentSweep->getLastRay();
	int lowestGate = lowVelGate[m][n];
	int highestGate = highVelGate[m][n];
	float *lowLevelVel = new float[numRays];
	float *highLevelVel = new float[numRays];
	int index = 0;
	for(int r = start; r <= stop; r++) {
	  float* vel = radarData->getRay(r)->getVelData();
	  lowLevelVel[index] = vel[lowestGate];
	  highLevelVel[index] = vel[highestGate];
	  index++;
	}

	if(useGVAD) {
	  GVAD(lowLevelVel, currentSweep, speedl, dirl, rmsl);
	  GVAD(highLevelVel, currentSweep, speedu, diru, rmsu);
	}
	else {
	  VAD(lowLevelVel, currentSweep, speedl, dirl, rmsl);
	  /*
	  emit log(Message("!!Speed = "+QString().setNum(speedl)+" dir = "
			 +QString().setNum(dirl)+" low, m = "
			 +QString().setNum(m)+", n = "+QString().setNum(n));)
	  */
	  VAD(highLevelVel, currentSweep, speedu, diru, rmsu);
	  /*
	  emit log(Message("!!Speed = "+QString().setNum(speedu)+" dir = "
			 +QString().setNum(diru)+" high, m = "
			 +QString().setNum(m)+", n = "+QString().setNum(n)));
	  */
	}
      }
      lowSpeed[m][n] = speedl;
      lowDir[m][n] = dirl;
      lowRMS[m][n] = rmsl;
      highSpeed[m][n] = speedu;
      highDir[m][n] = diru;
      highRMS[m][n] = rmsu;
    }
  }

  //ADD some sort of checking and repeat logic, once all Pauls
  //Boolean indicators are in place.

  // Since VAD and GVAD fits were done twice for each vad Level
  // They will now be averaged together to compute a resulting estimate

  float **meanSpeed = new float*[vadLevels];
  float **meanDir = new float*[vadLevels];
  float **meanRMS = new float*[vadLevels];
  for(int m = 0; m < vadLevels; m++) {
    meanSpeed[m] = new float[numSweeps];
    meanDir[m] = new float[numSweeps];
    meanRMS[m] = new float[numSweeps];
    for(int n = 0; n < numSweeps; n++) {
      meanSpeed[m][n] = velNull;
      meanDir[m][n] = velNull;
      meanRMS[m][n] = velNull;
    }
  }

  for(int m = 0; m < vadLevels; m++) {
    for(int n = 0; n < numSweeps; n++) {
      
      int rayIndex = radarData->getSweep(n)->getFirstRay();
      Ray *currentRay = radarData->getRay(rayIndex);
      float dHeightL = m-findHeight(currentRay,lowVelGate[m][n]); 
      float dHeightU = findHeight(currentRay, highVelGate[m][n]) - m;
      meanSpeed[m][n] = bilinear(highSpeed[m][n], lowSpeed[m][n],
				     dHeightU, dHeightL);

      float dirl = lowDir[m][n];
      float diru = highDir[m][n];
      if(fabs(dirl-diru) > 270) {
	if(dirl>diru)
	  diru+=360;
	if(diru>dirl)
	  dirl+=360;
      }
      meanDir[m][n] = bilinear(diru, dirl, dHeightU, dHeightL);
      
      if(meanDir[m][n]>360) {
	meanDir[m][n] -=360;
      }
      if(meanDir[m][n]< 0) {
	meanDir[m][n] +=360;
      }
      
      if(meanSpeed[m][n] != velNull) {
	/*
	Message::toScreen("Bilinear: m: "+QString().setNum(m)+" n:"
		       +QString().setNum(n)+" velocity: "
		       +QString().setNum(meanSpeed[m][n])+" dir: "
		       +QString().setNum(meanDir[m][n]));
	*/
	dHeightL = pow(dHeightL, 2);
	dHeightU = pow(dHeightU, 2);
	float rmsl = pow(lowRMS[m][n],2);
	float rmsu = pow(highRMS[m][n],2);
	float rms = bilinear(rmsu, rmsl,
			     dHeightU, dHeightL);
	meanRMS[m][n]= sqrt(rms);
      }
      else {
	meanRMS[m][n] = velNull;
      }
      if(meanSpeed[m][n] != velNull) {
	sumwt[m]+= pow((1/meanRMS[m][n]),2);
	vadFound[m] = true;
      }
    }
  }


  for(int m = 0; m < vadLevels; m++) {
    for(int n = 0; n < numSweeps; n++) {
      if(vadFound[m]) {
	float wgt = pow((1/meanRMS[m][n]), 2);
	envWind[m] += wgt*meanSpeed[m][n];
	envDir[m] += wgt*meanDir[m][n];
      }
    }
   
  }
  
  //if(vad_ntimes < 5)  goto 230 ?!?!?!

  for(int m = 0; m < vadLevels; m++) {
    if(vadFound[m]) {
      envWind[m] *= (1/sumwt[m]);
      envDir[m] *= (1/sumwt[m]);
      vadRMS[m] = sqrt(1/sumwt[m]);

      if(!useGVAD)
	emit log(Message("M:"+ QString().setNum(m)+" VAD SPEED: "
		       +QString().setNum(envWind[m])
		       +" VAD DIR: "+QString().setNum(envDir[m])));
      else
	emit log(Message("M:"+ QString().setNum(m)+" GVAD SPEED: "
		       +QString().setNum(envWind[m])
		       +" VAD DIR: "+QString().setNum(envDir[m])));

    }
    else {
      envWind[m] = velNull;
      envDir[m] = velNull;
    }

  }


  
  for(int m = 0; m < vadLevels; m++) {

    delete last_count_up[m];
    delete last_count_low[m];
    delete highVelGate[m];
    delete lowVelGate[m];
    delete hasVelData[m];
    
  }

  delete vadFound;
  delete sumwt;
  delete last_count_up;
  delete last_count_low;
  delete highVelGate;
  delete lowVelGate;
  delete hasVelData;
  
  return true;
 
}

void RadarQC::vadPrep()
{

  float numSweeps = radarData->getNumSweeps();
  // Checking for values needed in VAD
  for (int m = 0; m < vadLevels; m++) {
    for(int n = 0; n < numSweeps; n++) {
      Sweep* currentSweep = radarData->getSweep(n);
      int start = currentSweep->getFirstRay();
      //int stop = currentSweep->getLastRay();
      //for(int r = start; r <= stop; r++) {
      int r = start;
      Ray *currentRay = radarData->getRay(r);
      int numVBins = currentRay->getVel_numgates();
      float max_up = 0;
      float max_low = 0;
      bool hasHighVel = false;
      bool hasLowVel = false;
      for(int v = 0; v < numVBins; v++) {
	float height = findHeight(currentRay, v);
	if((height >= m)&&(height < m+1)) {
	  if((validBinCount[n][v] > max_up)
	     &&(validBinCount[n][v] < last_count_up[m][n])) {
	    max_up = validBinCount[n][v];
	    highVelGate[m][n] = v;
	    if(max_up > thr) {
	      hasHighVel = true;
	    }
	  } 
	}
	if((height >= m-1)&&(height < m)) {
	  if((validBinCount[n][v] > max_low)
	     &&(validBinCount[n][v] < last_count_low[m][n])) {
	    max_low = validBinCount[n][v];
	    lowVelGate[m][n] = v;
	    if(max_low > thr)
	      hasLowVel = true;  
	  }
	}
	if(hasHighVel && hasLowVel) {
	  hasVelData[m][n] = true;
	  last_count_up[m][n] = max_up;
	  last_count_low[m][n] = max_low;
	}
      }
      
      /*
      //if((m == 5)&&(n == 1)) {
      emit log(Message("M = "+QString().setNum(m)+" N + "+QString().setNum(n)+
      " Max_Up = "+QString().setNum(last_count_up[m][n])
      +" Max_Down = "
      +QString().setNum(last_count_low[m][n])));
      //}
      */
      
    }
  }
}

bool RadarQC::VAD(float* &vel, Sweep* &currentSweep, 
		  float &speed, float &direction, float &rms)
{
  int numData = 0;
  int numRays = currentSweep->getNumRays();
  int start = currentSweep->getFirstRay();
  float nyqVel = currentSweep->getNyquist_vel();
  // could be implemented for more than 3 coefficeints  int N = numCoEff/2;
 
  if((nyqVel == 0)||(fabs(nyqVel)>90)) {
    emit log(Message("Nyquist Velocity Not Defined - Dealiasing Not Possible"));
    return false;
  }

 for(int r = 0; r < numRays; r++) {
    float velocity = fabs(vel[r]);
    if(velocity > 1.0 && velocity < 90.0) {
      numData++;
    }
  }
  float **X  = new float*[numCoEff];
  for(int i = 0; i < numCoEff; i++)
    X[i] = new float[numData];
  float *Y = new float[numData];
  int dataIndex = 0;
  for(int r = 0; r < numRays; r++) {
    float velocity = fabs(vel[r]);
    if(velocity > 1.0 && velocity < 90.0)
      {
	float azimuth = radarData->getRay(start+r)->getAzimuth();
	azimuth *=(acos(-1.0)/180);
	X[0][dataIndex] = 1;
	
	/*
	for(int i = 1; i <= N; i++) {
	  X[2*i-1][dataIndex] = sin(float(i)*azimuth); 
     	  X[2*i][dataIndex] = cos(float(i)*azimuth);
	}
	*/
	X[1][dataIndex] = sin(azimuth);
	X[2][dataIndex] = cos(azimuth);
	
	Y[dataIndex] = vel[r];
	dataIndex++;
      }
  }
  float stDeviation, *coEff, *stError;
  coEff = new float[numCoEff];
  stError = new float[numCoEff];

  if(!lls(numCoEff, numData, X, Y, stDeviation, coEff, stError)) {
    // emit log(Message("Failed in lls"));
    return false;
  }

  /*
    This does not make use of the stError values,
    Nor does it recognise coefficients higher than 3
    Per Paul's Code.
  */

  float elevAngle = currentSweep->getElevation();
  elevAngle *= deg2rad;
  speed = sqrt(coEff[1]*coEff[1]+coEff[2]*coEff[2])/cos(elevAngle);
  direction = atan2(-1*coEff[1], -1*coEff[2])/deg2rad;
  rms = stDeviation;
  return true;
}

bool RadarQC::GVAD(float* &vel, Sweep* &currentSweep, 
		   float &speed, float &direction, float &rms)
{
 
  speed = velNull;
  direction = velNull;
  rms = velNull;
  
  int numData = 0;
  int numRays = currentSweep->getNumRays();
  int start = currentSweep->getFirstRay();
  float nyqVel = currentSweep->getNyquist_vel();

  //could be added for more than 3 numCoefficients  int N = numCoEff/2;
 
  float *gvr, *gve;
  gvr = new float[numRays];
  gve = new float[numRays];
  for( int r = 0; r < numRays; r++) {
    gvr[r] = 0;
    gve[r] = velNull;
  }
  float pi = acos(-1);

  int n_filter = 6;  //?? I made this value up;

  int width = n_filter/2; //??????

  if((nyqVel == 0)||(fabs(nyqVel)>90)) {
    emit log(Message("Nyquist Velocity Not Defined - Dealiasing Not Possible"));
    return false;
  }

  for(int r = 0; r < numRays; r++) {
    if(vel[r] < 1.0) {
      // vel[r] = velNull;
    }
    // Paul's code went to r-1 ?????
    int k = 0;
    float velSum = 0;
    int last_r = r-1;
    if(last_r < 0)
      last_r = numRays-1;
    int next_r = r+1;
    if(next_r > (numRays-1))  // Paul's code didn't have this
      next_r = 0;
    if(vel[r] == velNull) {
      if(vel[last_r] != velNull) {
	velSum += vel[last_r];
	k++;
 
	if(vel[next_r]!=velNull) {
	  velSum += vel[next_r];
	  k++;
	  
	  if(k>1) {
	    vel[r] = velSum/float(k);
	  }
	}
      }
    }
  }




  for(int r = 0; r < numRays; r++) {
    int rr = r+1;
    if(rr > (numRays-1)) 
      rr -= (numRays-1);
    if((fabs(vel[r])<=90.0)&&(fabs(vel[rr])<=90.0)
       &&(fabs(vel[r])>1.9)&&(fabs(vel[rr])>1.9)) {
      float A = radarData->getRay(r+start)->getAzimuth();
      float AA = radarData->getRay(rr+start)->getAzimuth();
      A *= deg2rad;
      AA *= deg2rad;
      float deltaA;
      if(fabs(AA-A) >  pi) {
	if(AA>A) 
	  deltaA = AA-A-2*pi;
	if(AA<A) 
	  deltaA = AA-A+2*pi;
      }
      else {
	deltaA = AA-A; 
      }
      if((fabs(vel[rr]-vel[r])> nyqVel)||(fabs(AA-A)<0.001)) {
	gve[r] = velNull;
      }
      else {
	gve[r] = (vel[rr]-vel[r])/deltaA;
      }
    }
  }

  
  int count;

  for(int r = 0; r < numRays; r++) {
    count = 0;
    int last_r = r-1;
    int next_r = r+1;
    if(next_r > (numRays-1))
      next_r = 0;
    if(last_r < 0)
      last_r = numRays-1;
    if((gve[r] == velNull)&&(gve[next_r] == velNull)
       &&(gve[last_r]==velNull)) {
      gvr[r] = velNull;
    }
    else {
      for(int w = -width; w <= width; w++) {
	int ww = w+r;
	if(ww < 1)
	  ww += (numRays-1);
	if(gve[ww] != velNull)
	  count++;
      }
      float sumwgt = 0;
      float wgt;
      if(count >= width) {
	for(int w = -width; w <= width; w++) {
	  int ww = w+r;
	  if(ww < 1) {
	    ww += (numRays-1);
	  }
	  if(gve[ww] != velNull) {
	    wgt = 1.0-abs(w)/(n_filter+1);
	    gvr[r]+=wgt*gve[ww];
	    sumwgt += wgt;
	  }
	}
	gvr[r] *= (1/sumwgt);
      }
      else {
	gvr[r] = velNull;
      } 
    }
  }
  numData = 0;
  for(int r = 0; r < numRays; r++) {
    if(fabs(vel[r]) <= 90.0) {
      numData++;
    }
  }


  float **X, *Y;
  X = new float*[numCoEff];
  Y = new float [numData];
  for(int i = 0; i < numCoEff; i++) {
    X[i] = new float[numData];
    for(int j = 0; j < numData; j++) {
      X[i][j] = 0;
      Y[j] = 0;
    }
  }

  int dataIndex = 0;
  for(int r = 0; r < numRays; r++) {
    if(fabs(vel[r]) <= 90.0) {
      float rAzimuth = radarData->getRay(r+start)->getAzimuth() *deg2rad;
      X[0][dataIndex] = 1;
      X[1][dataIndex] = sin(rAzimuth);
      X[2][dataIndex] = cos(rAzimuth);
      Y[dataIndex] = gvr[r];
      dataIndex++;
    }
  }

  // printMatrix(Y, numData);
  
  float stDeviation, *coEff, *stError;
  coEff = new float[numCoEff];
  stError = new float[numCoEff];
  
  if(!lls(numCoEff, numData, X, Y, stDeviation, coEff, stError))
	  return false;

  float elevAngle = currentSweep->getElevation();
  elevAngle *= deg2rad;
  speed = sqrt(coEff[1]*coEff[1]+coEff[2]*coEff[2])/cos(elevAngle);
  direction = atan2(-1*coEff[2], coEff[1])/deg2rad;
  // Paul uses different coefficents in VAD?!?!?!?
  if(direction < 0)
    direction+=360;
  rms = stDeviation;
  return true;
}

float RadarQC::getStart(Ray *currentRay)
{

  /*
   *   Returns the environmental wind to be used for a specific ray 
   *   in the dealiasing algorithm
   */

  // There as some adjustment factors in here that seem bad!!!!
    
  float startVelocity;
  
  if(useUserWinds) {
    float phi = currentRay->getAzimuth();
    float theta = currentRay->getElevation();
    startVelocity = envWind[0]*cos(deg2rad*theta/100)
      *cos(deg2rad*(180+envDir[0]-phi/10.0)); 
  }
  if(useVADWinds) {
    
    int dataHeight = -1;
    int numVBins = currentRay->getVel_numgates();
    float *velGates = currentRay->getVelData();
    float elevAngle = currentRay->getElevation()*deg2rad;
    float azimuth = currentRay->getAzimuth();
    bool hasDopplerData = false;
    bool envWindFound = false;
    for(int v = 0; (v < numVBins) && (!hasDopplerData); v++) {
      if(velGates[v] != velNull) {
	hasDopplerData = true;
	dataHeight = int(floor(findHeight(currentRay, v)+.5));
	if(dataHeight == 0) {
	  dataHeight++;
	}
      }
    }
    if(hasDopplerData) {
    
      if(envWind[dataHeight] != velNull) {
	envWindFound = true;
      }
      else {

	int up = 0;
	int down = 0;
	for(int h = 1; (up!=vadLevels)&&(down != 1); h++) {
	  up = dataHeight + h;
	  if(up > vadLevels) {
	    up = vadLevels;
	  }
	  down = dataHeight + h;
	  if(down < 1) {
	    down = 1;
	  }
	  if((envWind[up]!= velNull)||(envWind[down]!=velNull)) {
	    if(envWind[up]!= velNull) 
	      dataHeight = up;
	    else 
	      dataHeight = down;
	    envWindFound = true;
	  }
	}
      }
      if(envWindFound) {

	startVelocity = envWind[dataHeight]*cos(elevAngle/100.0)
	  *cos((180.0+envWind[dataHeight]+azimuth)*deg2rad);
      }
      else 
	startVelocity = velNull;
    }
    else
      startVelocity = velNull;
  }
  if(useAWIPSWinds) {
    
  }
  
  return startVelocity;
}

bool RadarQC::BB()
  /*
   * ??? Failures to dealias a single row are ignored and move on
   */

{
  //emit log(Message("In BB"));
  Ray* currentRay;
  float numRays = radarData->getNumRays();
  for(int i = 0; i < numRays; i++) 
    {
      currentRay = radarData->getRay(i);
      float *vGates = currentRay->getVelData();
      float startVelocity = getStart(currentRay);
      float nyquistVelocity = currentRay->getNyquist_vel();
      int numVelocityGates = currentRay->getVel_numgates();
      if((numVelocityGates!=0)&&(startVelocity!=velNull))
	{
	  float sum = float(numVGatesAveraged)*startVelocity;
	  float nyquistSum = float(numVGatesAveraged)*nyquistVelocity;
	  float n = 0.0;
	  int overMaxFold = 0;
	  bool unaliased;
	  float segVelocity[numVGatesAveraged];
	  for(int k = 0; k < numVGatesAveraged; k++)
	    {
	      segVelocity[k] = startVelocity;
	    }
	  for(int j = 0; j < numVelocityGates; j++)
	    {
	      if(vGates[j]!=velNull)
		{
		  n = 0.0;
		  unaliased = false;
		  while(unaliased!=true)
		    {
		      float tryVelocity = numVGatesAveraged
			*(vGates[j]+(2*n*nyquistVelocity));
		      if((sum+nyquistSum > tryVelocity)&&
			 (tryVelocity > sum-nyquistSum))
			{
			  unaliased=true;
			} 
		      else 
			{
			  if(tryVelocity >= sum+nyquistSum)
			    n--;
			  if(tryVelocity <= sum-nyquistSum)
			    n++;
			  if(fabs(n) > maxFold) {
			    emit log(Message(QString("Ray #")
					   +QString().setNum(i)
					   +QString(" Gate# ")
					   +QString().setNum(j)
					   +QString(" exceeded maxfolds")));
			    overMaxFold++;
			    unaliased=true;
			    vGates[j]=velNull;
			  }
			}
		    }
		  if(vGates[j]!=velNull) 
		    {
		      vGates[j]+= 2*n*(nyquistVelocity);
		      sum -= segVelocity[0];
		      sum += vGates[j];
		      for(int m = 0; m < numVGatesAveraged-1; m++)
			{
			  segVelocity[m] = segVelocity[m+1];
			}
		      segVelocity[numVGatesAveraged-1]= vGates[j];
		    }
		}
	    }
	}
      //delete vGates;
    }
  return true;
}


void RadarQC::crazyCheck()
{
  int numR = radarData->getNumRays();
  for(int i = 0; i < numR; i++)
    {
      Ray* currentRay = radarData->getRay(i);
      float *vbins = currentRay->getVelData();
      int numBins = currentRay->getVel_numgates();
      for (int v = 0; v < numBins; v++)
	{
	  if(isnan(vbins[v])) {
	    emit log(Message(QString("!!WTF!! ")
			   +QString("Dealias Ray #")
			   +QString().setNum(i)
			   +QString(" Gate# ")
			   +QString().setNum(v)
			   +QString(" Vel = ")
			   +QString().setNum(vbins[v])));
	  }
	}
    }
  emit log(Message("Crazy Check Done"));
}

float RadarQC::bilinear(float value_high,float value_low,
			float deltaH_high,float deltaH_low)

  /* 
   * This function produces the interpolated speed for a single VAD level.
   * The measurement is created by weighting the values with the distance of
   * the opposite point divided by the total distance. 
   */
{
  //Why would the point further from vad level m get a larger weight
  
  if(value_high==velNull || value_low == velNull) {
    // One or more of the values have been are Null so return error.
    return velNull;
  }
  float deltaH = deltaH_high +deltaH_low;
  if(fabs(deltaH) < 0.00001) {
    // Not possible to do bilinear averaging, return error.
    return velNull;
  }
  return ((value_high*deltaH_low + value_low*deltaH_high)/deltaH);
}

bool RadarQC::lls(const int &numCoEff,const int &numData, 
		  float** &x, float* &y, 
		  float &stDeviation, float* &coEff, float* &stError)
{
  /*
   * x is a matrix with numCoEff rows, and numData columns,
   * y is a matrix with numData rows,
   * coEff is the product containing the coefficient values (numCoEff rows)
   * stError is a product containing the estimated error for each
   *  coefficent in coEff, (numCoEff rows)
   * stDeviation is the estimated standard deviation of the regression
   */

  if(numData < numCoEff) {
    //emit log(Message("Least Squares: Not Enough Data"));
    return false;
  }
  // We need at least one more data point than coefficient in order to
  // estimate the standard deviation of the fit.
  
  float** A = new float*[numCoEff];
  float** AA = new float*[numCoEff];
  float* B = new float[numCoEff];
  float** BB = new float*[numCoEff];
  coEff = new float[numCoEff];
  for(int row = 0; row < numCoEff; row++) {
    A[row] = new float[numCoEff];
    AA[row] = new float[numCoEff];
    BB[row] = new float[1];
    for(int col = 0; col < numCoEff; col++) {
      A[row][col] = 0;
      AA[row][col] = 0;
      BB[row][0] = 0;
    }
    B[row] = 0;
    coEff[row] = 0;
  }

  // accumulate the covariances of all the data into the regression
  // matrices

  for(int i = 0; i < numData; i++) {
    for(int row = 0; row < numCoEff; row++) {
      for(int col = 0; col < numCoEff; col++) {
	A[row][col]+=(x[row][i]*x[col][i]);
	AA[row][col]+=(x[row][i]*x[col][i]);
      }
      B[row] +=(x[row][i]*y[i]);
      BB[row][0] +=(x[row][i]*y[i]);
    }
  }

  
  float** Ainv = new float*[numCoEff];
  for(int p = 0; p < numCoEff; p++) 
    Ainv[p] = new float[p];

  //The C++ Recipes Code Works so All this can be done away with

  /*
  // find the inverse of A
  if(!matrixInverse(A, numCoEff, numCoEff, Ainv)) {
    Message::toScreen("lls: matrix inverse failed");
    return false;
  }
  
  // use the inverse of A to find coEff
  if(!matrixMultiply(Ainv, numCoEff, numCoEff, B, numCoEff, coEff, numCoEff)) {
    Message::toScreen("lls: Matrix Multiply Failed");
    return false;
  }
  */

  if(!leastSquaresRegression(AA,BB, numCoEff, 1))
    emit log(Message("Least Squares Fit Failed"));
  
  /* 
  Message::toScreen("CHECK: coeff[0] = "+QString().setNum(coEff[0])+" coeff[1] = "
		 +QString().setNum(coEff[1])+" coeff[2] = "
		 +QString().setNum(coEff[2]));
  Message::toScreen("     : BB[0][0] = "+QString().setNum(BB[0][0])+" BB[1][0] = "
		 +QString().setNum(BB[1][0])+" BB[2][0] = "
		 +QString().setNum(BB[2][0]));
  */

  for(int i = 0; i < numCoEff; i++) {
    coEff[i] = BB[i][0];
    for(int j = 0; j < numCoEff; j++) {
      Ainv[i][j] = A[i][j];
    }
  }
  
  // calculate the stDeviation and stError
  float sum = 0;
  for(int i = 0; i < numData; i++) {
    float regValue = 0;
    for(int j = 0; j < numCoEff; j++) {
      regValue += coEff[j]*x[j][i]; 
    }
    sum +=((y[i]-regValue)*(y[i]-regValue));
  }
  
  stDeviation = sqrt(sum/float(numData-numCoEff));
  
  // calculate the standard error for the coefficients

  for(int i = 0; i < numCoEff; i++) {
    stError[i] = stDeviation*sqrt(Ainv[i][i]);
  }
   
  return true;
} 

bool RadarQC::matrixInverse(float **A, int M, int N, float** &Ainv)
{
  if(M!=N) {
    emit log(Message("matrixInverse: Bad Demensions M!=N"));
    return false;
  }

  //Message::toScreen("Matrix before inverting");
  //printMatrix(A, M, N);

  for(int p = 0; p < M; p++) {
    for(int q = 0; q < M; q++) 
      Ainv[p][q] = 0.0;
  }

  float **temp;
  temp = new float*[M];
  for(int p = 0; p < M; p++) {
    temp[p] = new float[2*M];
    for(int q = 0; q < 2*M; q++) 
      temp[p][q] = 0.0;
  }

  for(int i = 0; i < M; i++) {
    for(int j = 0; j < M; j++) {
      temp[i][j] = A[i][j];
    }
    for(int j = M; j < 2*M; j++) {
      if(i == (j-M))
	temp[i][j]=1.0;
      else
	temp[i][j]=0.0;
    }
  }

  if(!reduceRow(temp, M, 2*M)) {
    emit log(Message("matrixInverse: reduceRow failed"));
    return false;
  }

  for(int i = 0; i < M; i++) {
    for(int j = 0; j < M; j++) {
      Ainv[i][j] = temp[i][j+M];
    }
  }
  
  for(int i = 0; i < M; i++) {
    for(int j = 0; j < M; j++) {
      if(i==j){
	if(temp[i][j]!=1.0) {
	  emit log(Message("matrixInverse:WARNING:Matrix May Be Singular! :("));
	  return false;
	}
      }
      else{
	if(temp[i][j]!=0.0) {
	  emit log(Message("matrixInverse:WARNING:Matrix May Be Singular! :<>"));
	  return false;
	}
      }
    }
  }
  
  //Message::toScreen("Matrix after inverting");
  //printMatrix(Ainv, M, M);

  return true;
}

bool RadarQC::reduceRow(float **A, int M, int N)
{

  /* Passed basic tests, need more thorough testing with tricky zeros included
   * Single implementation of gaussian jordan elimination
   * Architecture designed by Paul Harasti
   */

  //  printMatrix(A, M, N);
  float X, W;
  bool *allZero = new bool[N];

  for(int col = 0; col < N; col++) {
    allZero[col] = true;
  }

  for(int col = 0; col < N; col++)
    {
      for(int row = 0; row < M; row++)
	{
	  if(A[row][col]!=0)
	    allZero[col]=false;
	}
    }
  int colStart = 0;
  int rowStart = 0;
  while(colStart < N && rowStart < M)
    {
      int pivotRow = 0;
      float Y = 0;
      float Z = 0;
      if(allZero[colStart]) {
	colStart++;
	if (colStart == N) {
	  emit log(Message("rowReduce:WARNING:Matrix May Be Singular! AllZero"));
	  return false;
	}
      }
      else {
	X =0;
	for(int row = rowStart; row < M; row++) {
	  W = 0;
	  for(int col = colStart; col < N; col++){
	    if(fabs(A[row][col])> W) {
	      W = fabs(A[row][col]);
	    }
	  }
	  if(W!=0) {
	    //Message::toScreen("Pivot value W: "+QString().setNum(W));
	    if(fabs(A[row][colStart])/W > X) {
	      X = fabs(A[row][colStart])/W;
	      //if(X < nearZero)
		//Message::toScreen("RadarQC:rowReduce:WARNING:Matrix May Be Singular!");
	      pivotRow = row;
	      //Message::toScreen("Pivot Row = "+QString().setNum(row));
	    }
	  }
	}
      }
      //printMatrix(A, M, N);
      if (pivotRow!=rowStart) {
	for(int col = colStart; col < N; col++) {
	  float temp = A[rowStart][col];
	  A[rowStart][col] = A[pivotRow][col];
	  A[pivotRow][col] = temp;
	}
      }
      //printMatrix(A, M, N);
      Y = A[rowStart][colStart];
      A[rowStart][colStart] = 1.0;
      if (colStart+1<N) {
	for(int col = colStart+1; col < N; col++) {
	  A[rowStart][col] = A[rowStart][col]/Y;
	}
      }
      //printMatrix(A, M, N);
      for(int row = 0; row < M; row++) {
	if(row!=rowStart) { 
	  Z = A[row][colStart];
	  A[row][colStart] = 0;
	  if(colStart+1 < N) {
	    for(int col = colStart+1; col < N; col++) {
	      A[row][col] = A[row][col] - A[rowStart][col]*Z;
	    }
	  }
	}
	//printMatrix(A, M, N);
      }
      rowStart++;
      colStart++;
    }
  //Message::toScreen("Out of Row Reduce");
  return true;
}

bool RadarQC::matrixMultiply(float **A, int MA, int NA, 
			     float **B, int MB, int NB, 
			     float **C, int MC, int NC)
{
  if(NA != MB || MA!=MC || NB!=NC)
    return false;
 
  float sum;
  for(int icol = 0; icol < NB; icol++) {
    for(int irow = 0; irow < MA; irow++) {
      sum = 0;
      for(int p = 0; p < NA; p++) {
	sum +=A[irow][p]*B[p][icol];
	/*
	Message::toScreen("C"+QString().setNum(sum)+" += A ("
		       +QString().setNum(A[irow][p])+")*B("+
		       QString().setNum(B[p][icol])+")");
	*/
      }
      C[irow][icol] = sum;
      /*
      Message::toScreen("C["+QString().setNum(irow)+"]["+QString().setNum(icol)+
		     "] = "+QString().setNum(sum));
      */
    }
  }
  return true;
  
}

bool RadarQC::matrixMultiply(float **A, int MA, int NA, 
			     float *B, int MB, 
			     float *C, int MC)
{
  if(NA != MB || MA!=MC)
    return false;

  float sum;
  for(int irow = 0; irow < MA; irow++) {
    sum = 0;
    for(int p = 0; p < NA; p++) {
      sum +=A[irow][p]*B[p];
    }
    C[irow] = sum;
  }
  
  return true;
  
}

bool RadarQC::leastSquaresRegression(float **a, float **b, int n, int m)
{

  // get this to do what lls does using cpp recipes
  // a = nxn coefficient matrix
  // b = nxm matrix
  
  int i, icol, irow, j, k, l, ll;
  float big, temp, pivinv;
  
  int indxc[n], indxr[n], ipiv[n];

  for(j = 0; j < n; j++) {
    ipiv[j] = 0;
  }
  
  for (i = 0; i < n; i++) {
    big = 0.0;
    for (j = 0;j < n; j++) {
      if(ipiv[j]!=1) {
	for (k = 0; k < n; k++) {
	  if (ipiv[k] ==0) {
	    if(fabs(a[j][k]) >= big) {
	      big = fabs(a[j][k]);
	      irow = j;
	      icol = k;
	    }
	  }
	}
      }
    }
    ++(ipiv[icol]);
    if(irow != icol) {
      for(l = 0; l < n; l++) {
	float dummy = a[irow][l];
	a[irow][l] = a[icol][l];
	a[icol][l] = dummy;
      }
      for(l = 0; l < m; l++) {
	float dummy = b[irow][l];
	b[irow][l] = b[icol][l];
	b[icol][l] = dummy;
      }
    }
    indxr[i] = irow;
    indxc[i] = icol;
    if (a[icol][icol]==0.0) {
      return false; 
    }
    pivinv = 1/a[icol][icol];
    a[icol][icol] = 1.0;
    for(l = 0; l < n; l++) {
      a[icol][l] *=pivinv;
    }
    for(l = 0; l < m; l++) {
      b[icol][l] *= pivinv; 
    }
    for(ll = 0; ll < n; ll++) {
      if(ll != icol) {
	temp = a[ll][icol];
	a[ll][icol] = 0.0;
	for(l = 0; l < n; l++) {
	  a[ll][l] -= a[icol][l]*temp;
	}
	for(l = 0; l < m; l++) {
	  b[ll][l] -= b[icol][l]*temp;
	}
      }
    } 
  }
  for( l = n-1; l >= 0; l--) {
    if(indxr[l] != indxc[l]) {
      for(k = 0; k < n; k++) {
	float dummy = a[k][indxr[l]];
	a[k][indxr[l]] = a[k][indxc[l]];
	a[k][indxc[l]] = dummy;
      }
    }
  }
  

  return true;
}



void RadarQC::printMatrix(float **A, int M, int N)
{
  for(int i = 0;i < M; i++ ) {
    QString print;
    for(int j = 0; j < N; j++) {
      print += (QString().setNum(A[i][j])+QString(", " ));
    }
    Message::toScreen(print);
  }
  Message::toScreen("-----------------------------------");
}
void RadarQC::printMatrix(float *A, int M)
{   
  QString print;
  for(int j = 0; j < M; j++) {
    print += (QString().setNum(A[j])+QString(", " ));
  }
  Message::toScreen(print);
  Message::toScreen("-----------------------------------");
}

void RadarQC::catchLog(const Message& message)
{
  emit log(message);
}
