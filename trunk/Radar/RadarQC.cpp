/*
 *  RadarQC.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/28/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include <stdlib.h>
#include <math.h>
#include <QInputDialog>
#include <QString>

#include "RadarQC.h"
#include "RadarData.h"
#include "Message.h"
#include "Math/Matrix.h"

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

    velMin = qcConfig.firstChildElement("vel_min").text().toFloat();
    velMax = qcConfig.firstChildElement("vel_max").text().toFloat();
    refMin = qcConfig.firstChildElement("ref_min").text().toFloat();
    refMax = qcConfig.firstChildElement("ref_max").text().toFloat();
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
	  velMin = 1;
	  velMax = 100;
	  refMin = -15;
	  refMax = 100;
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
    emit log(Message("RadarQC: Quality Control Element is Not Valid, Using Default Parameters"));
    velMin = 1;
    velMax = 100;
    refMin = -15;
    refMax = 100;
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


  /*  
  Ray* check = radarData->getRay(35);
  float* checkVel = check->getVelData();
  QString checkPrint("First");
  for(int r = 0; r < check->getVel_numgates(); r++)
    {
      checkPrint+=QString(" ")+QString().setNum(checkVel[r]);
    }
  Message::toScreen(checkPrint);
  */
  if(!terminalVelocity())
    return false;
  /*
  check = radarData->getRay(35);
  checkVel = check->getVelData();
  checkPrint = QString("Terminal Vel");
  for(int r = 0; r < check->getVel_numgates(); r++)
    {
      checkPrint+=QString(" ")+QString().setNum(checkVel[r]);
    }
  Message::toScreen(checkPrint);
  */
  thresholdData();
  /*
  check = radarData->getRay(35);
  checkVel = check->getVelData();
  checkPrint = QString("Threshold Data");
  for(int r = 0; r < check->getVel_numgates(); r++)
    {
      checkPrint+=QString(" ")+QString().setNum(checkVel[r]);
    }
  Message::toScreen(checkPrint);
  */
  if(!findEnvironmentalWind()) {
    Message::toScreen("Failed in findEnvWind");
  }
  
  if(!BB()) {
    Message::toScreen("Failed in BB");
    return false;
  }
  /*
  check = radarData->getRay(35);
  checkVel = check->getVelData();
  checkPrint = QString("BB");
  for(int r = 0; r < check->getVel_numgates(); r++)
    {
      checkPrint+=QString(" ")+QString().setNum(checkVel[r]);
    }
  Message::toScreen(checkPrint);
  */
  return true;
}



void RadarQC::thresholdData()
{
  /*  
   *  This method iterates through each Ray of the RadarData volume,
   *  and removes velocity data that exceeds spectral width, reflectivity
   *  and velocity thresholds. This improves data quality in order to 
   *  improve the preformance of the following quality control routines.
   *  This method also calculates the number of valid gates, which is used
   *  in the VAD and GVAD methods.
   */

  int numSweeps = radarData->getNumSweeps();
  validBinCount = new float*[numSweeps];
  for (int i = 0; i < numSweeps; i++) {
    Sweep *currentSweep = radarData->getSweep(i);
    int index = currentSweep->getFirstRay();
    //Message::toScreen("Index of First Ray = "+QString().setNum(index));
    int numBins = radarData->getRay(index)->getVel_numgates();
    //Message::toScreen("Number of vel bins in that ray ="+QString().setNum(numBins));
    validBinCount[i] = new float[numBins]; 
  }

  int numRays = radarData->getNumRays();
  //Message::toScreen("total ray count = "+QString().setNum(numRays));
  Ray* currentRay = new Ray;
  int numVGates;
  for(int i = 0; i < numRays; i++)
    {
      currentRay = radarData->getRay(i);
      int sweepIndex = currentRay->getSweepIndex();
      numVGates = currentRay->getVel_numgates();
      float *vGates = currentRay->getVelData();
      float *swGates = currentRay->getSwData();
      float *refGates = currentRay->getRefData();
      for (int j = 0; j < numVGates; j++)
	{
	  if((swGates[j] > specWidthLimit)||
	     (fabs(vGates[j]) < velMin) ||
	     (fabs(vGates[j]) > velMax) ||
	     (refGates[j] < refMin) ||
	     (refGates[j] > refMax))
	    {
	      vGates[j] = velNull;
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
   *  relative to the absolute height of the radar in km.
   */

  float range = gateIndex/4.0 - 0.375;
  float elevAngle = currentRay->getElevation();
  // This height is relative to the height of the radar.
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
    return false
  }
  */
  if(useUserWinds) {
    // Do nothing move straight to BB
    return true;
  }

  if(useVADWinds) {

    // Attempt to use GVAD algorithm to determine 
    //  environmental winds
    bool useGVAD = false;
    if(findVADStart(useGVAD)) 
      return true;
    // If GVAD fails attempt to use the VAD algorithm to 
    //   determine environmental winds.
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

  bool foundVelocity = false;

  for(int m = 0; m < vadLevels; m++) {
    if(vadFound[m]) {
      envWind[m] *= (1/sumwt[m]);
      envDir[m] *= (1/sumwt[m]);
      vadRMS[m] = sqrt(1/sumwt[m]);
      foundVelocity = true;

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
      emit log(Message("M:"+QString().setNum(m)+" No VAD Speed Found"));
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

  // If none of the VAD levels have values we should be able to return false  

  if(!foundVelocity)
    return false;

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
      int stop = currentSweep->getLastRay();
      for(int r = start; r <= stop; r++) {
	//int r = start;
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

  if(!llsSolver->lls(numCoEff, numData, X, Y, stDeviation, coEff, stError)) {
    emit log(Message("Failed in lls"));
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
  
  if(!llsSolver->lls(numCoEff, numData, X, Y, stDeviation, coEff, stError))
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
      else {
	startVelocity = velNull;
	Message::toScreen("Start Velocity = VelNull");
      }
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
	  bool dealiased;
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
		  dealiased = false;
		  while(dealiased!=true)
		    {
		      float tryVelocity = numVGatesAveraged
			*(vGates[j]+(2*n*nyquistVelocity));
		      if((sum+nyquistSum > tryVelocity)&&
			 (tryVelocity > sum-nyquistSum))
			{
			  dealiased=true;
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
			    dealiased=true;
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

void RadarQC::catchLog(const Message& message)
{
  emit log(message);
}
