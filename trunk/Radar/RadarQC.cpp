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
  specWidthLimit = 12;
  velNull = -999.;
  maxFold = 4;
  numVGatesAveraged = 30;
  vadthr = 30; // maybe these should be in the config
  gvadthr = 180; 
  useVADWinds = false;
  useUserWinds = false;
  useAWIPSWinds = false;
  numCoEff = 3;
  vadLevels = 20;
  deg2rad = acos(-1)/180;
  int vcp = radarData->getVCP();
  float zero = 0.0;
  radarHeight = radarData->absoluteRadarBeamHeight(zero,zero);

  //Message::toScreen("Radar Height = "+QString().setNum(radarHeight)+" check units and values");

  // Allocate memory for the bincount
  int numSweeps = radarData->getNumSweeps();
  validBinCount = new float*[numSweeps];
  for (int i = 0; i < numSweeps; i++) {
	  Sweep *currentSweep = radarData->getSweep(i);
	  //int index = currentSweep->getFirstRay();
	  //Message::toScreen("Index of First Ray = "+QString().setNum(index));
	  int numBins = currentSweep->getVel_numgates();
	  //Message::toScreen("Number of vel bins in that ray ="+QString().setNum(numBins));
	  validBinCount[i] = new float[numBins];
  }
  
  // Used to determine how the reflectivity data should be interpolated based
  // on the vcp of the radar volume

  if(vcp < 33) {
    q = 3;
    if(vcp == 12) {
      q = 5;
    }
    qinc = 2;
  }
  if(vcp == 121) {
    q = 5;
    qinc = 4;
  }

  //Message::toScreen("Constructor");
  //checkRay();

  // Interpolating reflectivity to split levels

  // Determine rotational direction of radar
  bool clockwise = false;
  float sumazdiff = 0.;
  int k = 0;
  Sweep *currentSweep = radarData->getSweep(k);
  int start = currentSweep->getFirstRay();
  float stop = start+int(currentSweep->getNumRays())/4;
  float az1, az2;
  for(int i = start; i <= stop; i++) {
    az1 = radarData->getRay(i)->getAzimuth();
    if(az1 < 0.0)
      az1 +=360.0;
    az2 = radarData->getRay(i+1)->getAzimuth();
    if(az2 < 0.0)
      az2+=360.0;
    if((az1 < 10.0)&&(az2 > 350.0)) {
      az1 +=360.0;
    }
    if((az2 < 10.0)&&(az1 >350.0)) {
      az2+=360;
    }
    sumazdiff += (az2-az1);
    if(sumazdiff > 0.0)
      clockwise = true;
    else
      clockwise = false;
  }
  
  /*
  //Check to see which sweeps have reflectivity data...
  for(int s = 0; s < radarData->getNumSweeps(); s++) {
    int num = radarData->getRay(radarData->getSweep(s)->getFirstRay())->getRef_numgates();
    //Message::toScreen(("Num gates in first ray(# = "+QString().setNum(radarData->getSweep(s)->getFirstRay())+") of sweep "+QString().setNum(s)+" is "+QString().setNum(num)));
    //emit log(Message(("Num gates in first ray(# = "+QString().setNum(radarData->getSweep(s)->getFirstRay())+") of sweep "+QString().setNum(s)+" is "+QString().setNum(num))));
  }

  //Message::toScreen("q = "+QString().setNum(q)+" qinc "+QString().setNum(qinc));
	
  */	    

  // This parts interpolates reflectivity data onto levels which only contain
  // velocity data.

  int ifoundit = 0;
  float a, b, c, den;
  for(int i = 0; ((i < q)&&(i<radarData->getNumSweeps()-1)); i+=qinc) {
    int upperFirst = radarData->getSweep(i+1)->getFirstRay();
    //int upperLast = radarData->getSweep(i+1)->getLastRay();
    int first = radarData->getSweep(i)->getFirstRay();
    //int last = radarData->getSweep(i)->getLastRay();
    int upperNumRays = radarData->getSweep(i+1)->getNumRays();
    int numRays = radarData->getSweep(i)->getNumRays();
    //Message::toScreen("upperNumRays = "+QString().setNum(upperNumRays));
    float fac1[upperNumRays], fac2[upperNumRays];
    int ipt1[upperNumRays], ipt2[upperNumRays];
    for(int m = 0; m < upperNumRays; m++) {
      ipt1[m] = 0;
      ipt2[m] = 0;
      fac1[m] = 0;
      fac2[m] = 0;
      ifoundit = 0;
      for(int j = 0; j < numRays; j++) {
	int r = j+1;
	if(r > numRays)
	  r -=numRays;
	a = radarData->getRay(j+first)->getAzimuth();
	b = radarData->getRay(r+first)->getAzimuth();
	c = radarData->getRay(m+upperFirst)->getAzimuth();
	if(clockwise) {
	  if((a > 350.0)&&(b < 10.0)&&(c < 10.0))
	    a -=360;
	  if((a > 350.0)&&(b < 10.0)&&(c > 350))
	    b +=360;
	  if((a <= c)&&(b >= c)){
	    ifoundit ++;
	    if(ifoundit==1) {
	      den = b-a;
	      fac1[m] = (b-c)/den;
	      fac2[m] = (c-a)/den;
	      ipt1[m] = j+first;
	      ipt2[m] = r+first;
	      // Set up num ref gates
	      int numGates = radarData->getRay(j+first)->getRef_numgates();
	      if(radarData->getRay(r+first)->getRef_numgates() < numGates)
		numGates = radarData->getRay(r+first)->getRef_numgates();
	      int firstGate = radarData->getRay(j+first)->getFirst_ref_gate();
	      if(radarData->getRay(r+first)->getFirst_ref_gate() > firstGate)
		firstGate = radarData->getRay(r+first)->getFirst_ref_gate();
	      radarData->getRay(m+upperFirst)->setRef_numgates(numGates);
	      radarData->getRay(m+upperFirst)->setFirst_ref_gate(firstGate);
	      float sp1 = radarData->getRay(j+first)->getRef_gatesp();
	      float sp2 = radarData->getRay(r+first)->getRef_gatesp();
	      if(sp1!=sp2)
		Message::toScreen("RadarQC: Error interpolating split level data: Cannot resolve reflectivity gate spacing");
	      radarData->getRay(m+upperFirst)->setRef_gatesp(sp1);
	    }
	  }
	}
	else {
	  if((a < 10.0)&&(b > 350.0)&&(c < 10.0))
	    a-= 360.0;
	  if((a < 10.0)&&(b > 350.0)&&(c > 350.0))
	    b+=360;
	  if((a >= c)&&(b <= c)) {
	    ifoundit++;
	    if(ifoundit==1) {
	      den = a-b;
	      fac1[m] = (c-b)/den;
	      fac2[m] = (a-c)/den;
	      ipt1[m] = j;
	      ipt2[m] = r;
	      //Message::toScreen("For interpolation ray: "+QString().setNum(m+upperFirst)+" from ray = "+QString().setNum(j+first)+" a = "+QString().setNum(a)+" b = "+QString().setNum(b)+" c = "+QString().setNum(c));
	    }
	  }
	}
      }
      //if(m == 0) {
      //Message::toScreen("For ray "+QString().setNum(m+upperFirst)+" ipt1[m]  = "+QString().setNum(ipt1[m])+" ipt2[m] = "+QString().setNum(ipt2[m]));
	//}
    }
    for(int m = 0; m < upperNumRays; m++) {
      float *refValues1, *refValues2;
      refValues1 = radarData->getRay(ipt1[m])->getRefData();
      refValues2 = radarData->getRay(ipt2[m])->getRefData();
      Ray *current = radarData->getRay(m+upperFirst);
      //Message::toScreen("Checking ray # "+QString().setNum(m)+" which has "+QString().setNum(current->getRef_numgates())+" ipt1[m] = "+QString().setNum(ipt1[m])+" ipt2[m] = "+QString().setNum(ipt2[m]));
      float *newRef = new float[current->getRef_numgates()];
      for(int k=current->getFirst_ref_gate(); 
	  k<current->getRef_numgates();k++) {
	  a = refValues1[k];
	  b = refValues2[k];
	  if((a!=velNull)&&(b!=velNull)) {
	    newRef[k] = a*fac1[m]+b*fac2[m];
	  }
	  if((a==velNull)&&(b!=velNull)){
	    newRef[k] = b;
	  }
	  if((a!=velNull)&&(b==velNull)) {
	    newRef[k] = a;
	  }
	  if((a==velNull)&&(b==velNull)) {
	    newRef[k] = velNull;
	  }
      }
      current->setRefData(newRef);
    }
  }
  // Get maximum number of velocity gates in each sweep to get
  // aveVADHeight, which is an average height in each sweep for each gate index
  
  aveVADHeight = new float*[radarData->getNumSweeps()];
  for(int n = 0; n < radarData->getNumSweeps(); n++) {
    int sweepNumVelGates = radarData->getSweep(n)->getVel_numgates();
    int first = radarData->getSweep(n)->getFirstRay();
    int last = radarData->getSweep(n)->getLastRay();
    aveVADHeight[n] = new float[sweepNumVelGates];
    for(int v = 0; v < sweepNumVelGates; v++) {
      aveVADHeight[n][v] = 0;
      int count = 0;
      for(int r = first; r <= last; r++) {
	Ray *currentRay = radarData->getRay(r);
	if(v < currentRay->getVel_numgates()) {
	  count++;
	  aveVADHeight[n][v] += findHeight(currentRay,v); 
	}
      }
      aveVADHeight[n][v] /= float(count);
    }
  }


  //Message::toScreen("After interpolation");
 //checkRay();


}

RadarQC::~RadarQC()
{
  for(int i = 0; i < radarData->getNumSweeps(); i++) {
    delete [] aveVADHeight[i];
	delete [] validBinCount[i];
  }
  delete [] aveVADHeight;
  delete [] validBinCount;
  delete [] envWind;
  delete [] envDir;
}

void RadarQC::getConfig(QDomElement qcConfig)
{
  /*
   *   Retreves user parameters from the XML configuration file
   */

  // Get Thresholding and BB Parameters
  //Message::toScreen("In QC get Config");
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
	  specWidthLimit = 12;
	  numVGatesAveraged = 30;
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
    specWidthLimit = 12;
    numVGatesAveraged = 30;
    maxFold = 4;
    useVADWinds = true;
  }
  //Message::toScreen("leaving get config");
}



bool RadarQC::dealias()
{  

  /*
   *  Preforms basic quality control algorithms and velocity dealiasing
   *  on a single radar volume
   *
   */

  QString checkPrint("First");
  //Message::toScreen(checkPrint);
  //emit log(Message(checkPrint));
  //checkRay();
  //crazyCheck();

  if(!terminalVelocity())
    return false;
  
  checkPrint = QString("Terminal Vel");
  //Message::toScreen(checkPrint);
  //emit log(Message(checkPrint));
 //checkRay();
  //crazyCheck();
  
  thresholdData();
  
  checkPrint = QString("Threshold Data");
  //Message::toScreen(checkPrint);
  //emit log(Message(checkPrint));
 //checkRay();
  //crazyCheck();
  
  if(!findEnvironmentalWind()) {
    Message::toScreen("Failed finding environmental wind");
  }
  //Message::toScreen("findEnvironmentalWind");
  crazyCheck();
  
  if(!BB()) {
    Message::toScreen("Failed in Bargen-Brown dealising");
    return false;
  }
  
  
  checkPrint = QString("BB");
  //Message::toScreen(checkPrint);
  //emit log(Message(checkPrint));
  //crazyCheck();
  
  //checkRay();
  
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
  for (int i = 0; i < numSweeps; i++) {
    Sweep *currentSweep = radarData->getSweep(i);
    //int index = currentSweep->getFirstRay();
    //Message::toScreen("Index of First Ray = "+QString().setNum(index));
    int numBins = currentSweep->getVel_numgates();
    //Message::toScreen("Number of vel bins in that ray ="+QString().setNum(numBins));
    for (int j=0; j < numBins; j++)
		validBinCount[i][j]=0; 
  }

  int numRays = radarData->getNumRays();
  //Message::toScreen("total ray count = "+QString().setNum(numRays));
  // Do we need a new call here? I think just defining a pointer is okay
  //Ray* currentRay = new Ray;
  Ray* currentRay;
  int numVGates = 0;
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
	     (fabs(vGates[j]) > velMax))
	    // ||(refGates[j] < refMin) ||(refGates[j] > refMax))
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
		  /*
		  if((j == 640)&&(i == 1099)) {
		    Message::toScreen("vel = "+QString().setNum(vGates[30]));
		    Message::toScreen("velGateSpacing = "+QString().setNum(currentRay->getVel_gatesp()/1000.0));
		  }
		  */
		  //float range = j/4.0 - 0.375; previously used
		  float range = j*currentRay->getVel_gatesp()/1000.0;
		  float rgatesp = currentRay->getRef_gatesp()/1000.0;
		  float elevAngle = currentRay->getElevation();
		  float height = aveVADHeight[currentRay->getSweepIndex()][j];
		  // height is in km from sea level here
		  
		  float rho = 1.1904*exp(-1*height/9.58);
		  float theta = elevAngle*deg2rad+asin(range*cos(deg2rad*elevAngle)/(ae+height-radarHeight));
		  
		  /* I think this next step makes assumptions about
		   * the reflectivity of the gate spacing
		   */
		  /*
		  if((j == 640)&&(i == 1099))
		    Message::toScreen("height = "+QString().setNum(height)); 
		  */

		  int zgate = int(floor((range/rgatesp) +.5)+1);
		  if(zgate<0)
		    zgate = 0;   // CHECK CHECK
		  if(zgate >= currentRay->getRef_numgates())
		    zgate = currentRay->getRef_numgates()-1;
		  float zData = rGates[zgate];
		  float terminalV = 0;
		  zData = pow(10.0,(zData/10.0));
		  // New logic from Marks and Houze (1987)
		  if(height  < 5.1){
		    terminalV = -2.6*sin(theta)*(pow(zData,0.107))*(pow((1.1904/rho),0.45));
		  }
		  else {
		    if(height > 7.5) {
		      terminalV = (-0.817*sin(theta)*pow(zData,0.063)*pow((1.1904/rho),0.45));
		    }
		    else {
		      float a,b,c,den, v1, v2;
		      c = height;
		      den = 7.5-5.1;
		      a = (7.5-c)/den;
		      b = (c - 5.1)/den;
		      rho = 1.1904*exp(-1*5.1/9.58);
		      theta = deg2rad*elevAngle+asin(range*cos(deg2rad*elevAngle)/(ae+5.1-radarHeight));
		      v1 = (-2.6*sin(theta)*pow(zData, 0.107)*pow((1.1904/rho),0.45));
		      rho = 1.1904*exp(-1*7.5/9.58);
		      theta = deg2rad*elevAngle+asin(range*cos(deg2rad*elevAngle)/(ae+7.5-radarHeight));
		      v2 = (-0.817*sin(theta)*pow(zData, 0.063)*pow((1.1904/rho),0.45));
		      terminalV = a*v1+b*v2;
		    }
		  }
		  if(!isnan(terminalV))
		    vGates[j] -= terminalV;
		  else {
		    QString trap = "Ray = "+QString().setNum(i)+" j = "+QString().setNum(j)+" terminal vel "+QString().setNum(terminalV)+" ISNAN";
		    //Message::toScreen(trap);
		    //emit log(Message(trap));
		  }
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
  if(currentRay->getVel_gatesp()==0){
    Message::toScreen("Find height of ray w/o gate data");
    return -999;
  }
  float range = gateIndex*currentRay->getVel_gatesp()/1000.0;
  float elevAngle = currentRay->getElevation();
  // This height is in km from sea level
  float height = radarData->absoluteRadarBeamHeight(range, elevAngle);
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
    bool useGVAD = true;
    //useGVAD = false;
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
  
  QString boolString("True");
  if(!useGVAD){
    boolString = "False";
    //Message::toScreen("In findVADStart with useGVAD = "+boolString);
  }
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

  //Message::toScreen("thr = "+QString().setNum(thr));

  vadPrep();

  //Message::toScreen("Safely Past VadPrep");


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
				  /*
				   VAD(lowLevelVel, currentSweep, speedl, dirl, rmsl);
				   Message::toScreen("VAD Low m="+QString().setNum(m)+", n="
									 +QString().setNum(n)+" speed = "
									 +QString().setNum(speedl)+", dir = "
									 +QString().setNum(dirl)+" rms = "
									 +QString().setNum(rmsl));
				   
				   VAD(highLevelVel, currentSweep, speedu, diru, rmsu);
				   Message::toScreen("VAD High m="+QString().setNum(m)+", n="
									 +QString().setNum(n)+" speed = "
									 +QString().setNum(speedu)+", dir = "
									 +QString().setNum(diru)+" rms = "
									 +QString().setNum(rmsu));
				   */
				  
				  GVAD(lowLevelVel, currentSweep, speedl, dirl, rmsl);
				  /*
				   Message::toScreen("GVAD Low m="+QString().setNum(m)+", n="
									 +QString().setNum(n)+" speed = "
									 +QString().setNum(speedl)+", dir = "
									 +QString().setNum(dirl)+" rms = "
									 +QString().setNum(rmsl));
				   */
				  GVAD(highLevelVel, currentSweep, speedu, diru, rmsu);
				  /*
				   Message::toScreen("GVAD High m="+QString().setNum(m)+", n="
									 +QString().setNum(n)+" speed = "
									 +QString().setNum(speedu)+", dir = "
									 +QString().setNum(diru)+" rms = "
									 +QString().setNum(rmsu));
				   */
			  }
			  else {
				  VAD(lowLevelVel, currentSweep, speedl, dirl, rmsl);
				  /*
				   Message::toScreen("!!Speed = "+QString().setNum(speedl)+" dir = "
									 +QString().setNum(dirl)+" low, m = "
									 +QString().setNum(m)+", n = "+QString().setNum(n));
				   */
				  VAD(highLevelVel, currentSweep, speedu, diru, rmsu);
				  /*
				   Message::toScreen("!!Speed = "+QString().setNum(speedu)+" dir = "
									 +QString().setNum(diru)+" high, m = "
									 +QString().setNum(m)+", n = "+QString().setNum(n));
				   */
			  }
			  delete [] lowLevelVel;
			  delete [] highLevelVel;
		  }
		  lowSpeed[m][n] = speedl;
		  lowDir[m][n] = dirl;
		  lowRMS[m][n] = rmsl;
		  highSpeed[m][n] = speedu;
		  highDir[m][n] = diru;
		  highRMS[m][n] = rmsu;
		  //Message::toScreen("for m,n = "+QString().setNum(m)+", "+QString().setNum(n)+" lowDir = "+QString().setNum(lowDir[m][n])+" highDir = "+QString().setNum(highDir[m][n])+" lowRMS = "+QString().setNum(lowRMS[m][n])+" highRMS = "+QString().setNum(highRMS[m][n]));
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
		  if(radarData->getSweep(n)->getVel_numgates() > 0) {
			  float dHeightL=m-(aveVADHeight[n][lowVelGate[m][n]]-radarHeight)*3.281; 
			  float dHeightU=(aveVADHeight[n][highVelGate[m][n]]-radarHeight)*3.281 - m;
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
  }


  for(int m = 0; m < vadLevels; m++) {
    for(int n = 0; n < numSweeps; n++) {
      if(vadFound[m]&&(meanSpeed[m][n]!=velNull)) {
	float wgt = pow((1/meanRMS[m][n]), 2);
	envWind[m] += wgt*meanSpeed[m][n];
	envDir[m] += wgt*meanDir[m][n];
      }
    }
   
  }
  
  //if(vad_ntimes < 5)  goto 230 ?!?!?!

  bool foundVelocity = false;

  for(int m = 0; m < vadLevels; m++) {
    if(vadFound[m] && (envWind[m]!=velNull)) {
      envWind[m] *= (1/sumwt[m]);
      envDir[m] *= (1/sumwt[m]);
      vadRMS[m] = sqrt(1/sumwt[m]);
      foundVelocity = true;
      if(envWind[m]>velMax) {
	envWind[m] = velNull;
	envDir[m] = velNull;
	vadRMS[m] = velNull;
      }

      if(!useGVAD)
	emit log(Message("Level:"+ QString().setNum(m)+" VAD SPEED: "
		       +QString().setNum(envWind[m])
		       +" VAD DIR: "+QString().setNum(envDir[m])));
      else
	emit log(Message("Level:"+ QString().setNum(m)+" GVAD SPEED: "
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

    delete [] last_count_up[m];
    delete [] last_count_low[m];
    delete [] highVelGate[m];
    delete [] lowVelGate[m];
    delete [] hasVelData[m];
	delete [] lowSpeed[m];
	delete [] highSpeed[m];
	delete [] lowDir[m];
	delete [] highDir[m];
	delete [] lowRMS[m];
	delete [] highRMS[m];
	delete [] meanSpeed[m];
    delete [] meanDir[m];
    delete [] meanRMS[m];
	
  }

  delete [] vadFound;
  delete [] sumwt;
  delete [] last_count_up;
  delete [] last_count_low;
  delete [] highVelGate;
  delete [] lowVelGate;
  delete [] hasVelData;

  delete [] lowSpeed;
  delete [] highSpeed;
  delete [] lowDir;
  delete [] highDir;
  delete [] lowRMS;
  delete [] highRMS;

  delete [] meanSpeed;
  delete [] meanDir;
  delete [] meanRMS;
  

  // If none of the VAD levels have values we should be able to return false  

  if(!foundVelocity)
    return false;

  return true;
 
}

void RadarQC::vadPrep()
{
  int goodRings = 0;
  float numSweeps = radarData->getNumSweeps();
  // Checking for values needed in VAD
  for (int m = 0; m < vadLevels; m++) {
    for(int n = 0; n < numSweeps; n++) {
      Sweep* currentSweep = radarData->getSweep(n);
      if(currentSweep->getVel_numgates() > 0) {
	int numVBins = currentSweep->getVel_numgates();
	float max_up = 0;
	float max_low = 0;
	bool hasHighVel = false;
	bool hasLowVel = false;
	for(int v = 0; v < numVBins; v++) {
	  float height = (aveVADHeight[n][v] - radarHeight)*3.281;
	  // Here height is in thousands of feet relative to radar (AGL?)
	  if((height >= m)&&(height < m+1)) {
	    if((validBinCount[n][v] > max_up)
	       &&(validBinCount[n][v] < last_count_up[m][n])) {
	      max_up = validBinCount[n][v];
	      highVelGate[m][n] = v;
	    }
	    if(max_up > thr) {
	      hasHighVel = true;
	    }
	      
	  }
	  if((height >= m-1)&&(height < m)) {
	    if((validBinCount[n][v] > max_low)
	       &&(validBinCount[n][v] < last_count_low[m][n])) {
	      max_low = validBinCount[n][v];
	      lowVelGate[m][n] = v;
	    }
	    if(max_low > thr) {
	      hasLowVel = true;
	    }  
	  }
	}
	if(hasHighVel && hasLowVel) {
	  hasVelData[m][n] = true;
	  goodRings++;
	  last_count_up[m][n] = max_up;
	  last_count_low[m][n] = max_low;
	  //Message::toScreen("m = "+QString().setNum(m)+" n = "+QString().setNum(n)+" last count up = "+QString().setNum(max_up)+" last count low = "+QString().setNum(max_low));
	}
      }
    }
  }
  Message::toScreen("How many rings have vel data after vad prep: "+QString().setNum(goodRings));
}



bool RadarQC::VAD(float* &vel, Sweep* &currentSweep, 
		  float &speed, float &direction, float &rms)
{
  int numData = 0;
  int numRays = currentSweep->getNumRays();
  int start = currentSweep->getFirstRay();
  int stop = currentSweep->getLastRay();
  float nyqVel = currentSweep->getNyquist_vel();
  int vadNumCoEff = 3;
  // could be implemented for more than 3 coefficeints  int N = numCoEff/2;
 
  if((nyqVel == 0)||(fabs(nyqVel)>90)) {
    emit log(Message("Nyquist Velocity Not Defined - Dealiasing Not Possible"));
    return false;
  }

  float elevation = 0;
  for(int r = start; r <=stop; r++) {
    elevation+=radarData->getRay(r)->getElevation();
  }
  elevation/=float(numRays);
  
  for(int r = 0; r < numRays; r++) {
    if(fabs(vel[r]) < 90.0) {
      numData++;
    }
  }
  float **X  = new float*[vadNumCoEff];
  for(int i = 0; i < numCoEff; i++)
    X[i] = new float[numData];
  float *Y = new float[numData];
  int dataIndex = 0;
  for(int r = 0; r < numRays; r++) {
    if(fabs(vel[r]) < 90.0)
      {
	float azimuth = radarData->getRay(start+r)->getAzimuth();
	azimuth *=deg2rad;
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
  coEff = new float[vadNumCoEff];
  stError = new float[vadNumCoEff];

  if(! Matrix::lls(vadNumCoEff, numData, X, Y, stDeviation, coEff, stError)) {
    emit log(Message("VAD failed in lls"));
    return false;
  }

  /*
    This does not make use of the stError values,
    Nor does it recognise coefficients higher than 2
    Per Paul's Code.
  */

  float elevAngle = currentSweep->getElevation();
  elevAngle *= deg2rad;
  speed = sqrt(coEff[1]*coEff[1]+coEff[2]*coEff[2])/cos(elevAngle);
  direction = atan2(-1*coEff[1], -1*coEff[2])/deg2rad;
  if(direction < 0)
    direction+=360;
  rms = stDeviation;
  return true;
}

bool RadarQC::GVAD(float* &vel, Sweep* &currentSweep, 
		   float &speed, float &direction, float &rms)
{
 
  speed = velNull;
  direction = velNull;
  rms = velNull;
  numCoEff = 2;
  
  int numData = 0;
  int numRays = currentSweep->getNumRays();
  int start = currentSweep->getFirstRay();
  int stop = currentSweep->getLastRay();
  float nyqVel = currentSweep->getNyquist_vel();
  //Message::toScreen("Nyquist vel is "+QString().setNum(nyqVel));

  //could be added for more than 3 numCoefficients  int N = numCoEff/2;
 
  float *gvr, *gve;
  gvr = new float[numRays];
  gve = new float[numRays];
  for( int r = 0; r < numRays; r++) {
    gvr[r] = 0;
    gve[r] = velNull;
  }
  float elevation = 0;
  for(int r = start; r <=stop; r++) {
    elevation+=radarData->getRay(r)->getElevation();
  }
  elevation/=float(numRays);

  float pi = acos(-1);

  int n_filter = 10;   // Paul's parameter

  int width = n_filter/2; 

  if((nyqVel == 0)||(fabs(nyqVel)>90)) {
    emit log(Message("Nyquist Velocity Not Defined - Dealiasing Not Possible"));
    return false;
  }
  // Used to be fill_vad routine
  for(int r = 0; r < numRays; r++) {
    if(vel[r] < 1.0) {
      vel[r] = velNull;
    }
    // Paul's code went to r-1 ?????
    int last_r = r-1;
    if(last_r < 0)
      last_r = numRays-1;
    int next_r = r+1;
    if(next_r > (numRays-1))  // Paul's code didn't have this
      next_r = 0;
    if(vel[r] == velNull) {
      if((vel[last_r] != velNull)&&(vel[next_r]!=velNull)) {
	vel[r] = (vel[last_r]+vel[next_r])/2.0;
      }
    }
  }
  //-----end of what used to be fill_vad

  for(int r = 0; r < numRays; r++) {
    int rr = r+1;
    if(rr > (numRays-1)) 
      rr -= (numRays-1);
    if((fabs(vel[r])<=90.0)&&(fabs(vel[rr])<=90.0)
       &&(fabs(vel[r])>1.5)&&(fabs(vel[rr])>1.5)) {
      float A = radarData->getRay(r+start)->getAzimuth();
      float AA = radarData->getRay(rr+start)->getAzimuth();
      A *= deg2rad;
      AA *= deg2rad;
      float deltaA = 0;
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
	if(ww < 0)
	  ww += (numRays-1);
	if(ww > (numRays-1))
	  ww -= (numRays-1);
	if(gve[ww] != velNull)
	  count++;
      }
      float sumwgt = 0;
      float wgt;
      if(count >= width) {
	for(int w = -width; w <= width; w++) {
	  int ww = w+r;
	  if(ww < 0) 
	    ww += (numRays-1);
	  if(ww > (numRays-1))
	    ww -= (numRays-1);
	  if(gve[ww] != velNull) {
	    wgt = 1.0-abs(w)*1.0/float(n_filter+1);
	    gvr[r]+=wgt*gve[ww];
	    sumwgt += wgt;
	  }
	}
	gvr[r] *= (1.0/sumwgt);
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
  //Message::toScreen("numRays = "+QString().setNum(numRays));
  //Message::toScreen("numData = "+QString().setNum(numData));
  //Matrix::printMatrix(vel, numRays);

  float **X, *Y;
  X = new float*[numCoEff];
  Y = new float [numData];
  for(int i = 0; i < numCoEff; i++) {
    X[i] = new float[numData];
    for(int j = 0; j < numData; j++) {
      X[i][j] = -777.;
      Y[j] = -777.;
    }
  }

  int dataIndex = 0;
  for(int r = 0; r < numRays; r++) {
    if(fabs(vel[r]) <= 90.0) {
      float rAzimuth = radarData->getRay(r+start)->getAzimuth() *deg2rad;
      //      X[0][dataIndex] = 1;  droped constant?
      X[0][dataIndex] = sin(rAzimuth);
      X[1][dataIndex] = cos(rAzimuth);
      Y[dataIndex] = gvr[r];
      dataIndex++;
    }
  }
  //Message::toScreen("**************************************");
  //Matrix::printMatrix(X, numCoEff, numData);
  //Matrix::printMatrix(Y, numData);

  // printMatrix(Y, numData);
  
  float stDeviation, *coEff, *stError;
  coEff = new float[numCoEff];
  stError = new float[numCoEff];
  
  if(! Matrix::lls(numCoEff, numData, X, Y, stDeviation, coEff, stError))
	  return false;

  float elevAngle = currentSweep->getElevation();
  elevAngle *= deg2rad;
  speed = sqrt(coEff[0]*coEff[0]+coEff[1]*coEff[1])/cos(elevAngle);
  direction = atan2(-1*coEff[1], coEff[0])/deg2rad;
  // Paul uses different coefficents in VAD?!?!?!?
  if(direction < 0)
    direction+=360;
  rms = stDeviation;
 
  
  for(int i = 0; i < numCoEff; i++) {
    delete [] X[i];
  }
  delete [] X;
  delete [] Y;
  delete [] coEff;
  delete [] stError;
  delete [] gvr;
  delete [] gve;
  
  return true;
}

float RadarQC::getStart(Ray *currentRay)
{

  /*
   *   Returns the environmental wind to be used for a specific ray 
   *   in the dealiasing algorithm
   */

  // There as some adjustment factors in here that seem bad!!!!
    
  float startVelocity = 0;
  
  if(useUserWinds) {
    float phi = currentRay->getAzimuth();
    float theta = currentRay->getElevation();
    startVelocity = envWind[0]*cos(deg2rad*theta)*cos(deg2rad*(180+envDir[0]-phi)); 
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
	dataHeight = int(floor((aveVADHeight[currentRay->getSweepIndex()][v]-radarHeight)*3.281+.5));
	if(dataHeight < 0) {
	  dataHeight = 0;
	}
      }
    }
    if(hasDopplerData) {
      //QString d("In getStart rayIndex,sweepIndex = "+QString().setNum(currentRay->getRayIndex())+", "+QString().setNum(currentRay->getSweepIndex())+" azimuth = "+QString().setNum(azimuth)+" envWind @ dataHeight = "+QString().setNum(envWind[dataHeight])+" envDir @ dataHeight = "+QString().setNum(envDir[dataHeight]));
      //emit log(Message(d));
    
      if(envWind[dataHeight] != velNull) {
	envWindFound = true;
      }
      else {
	int up = 0;
	int down = 0;
	for(int h = 1; (!envWindFound||((up!=vadLevels-1)&&(down != 0))); h++) {
	  up = dataHeight + h;
	  if(up >= vadLevels) {
	    up = vadLevels-1;
	  }
	  down = dataHeight - h;
	  if(down < 0) {
	    down = 0;
	  }
	  //QString dd("Up ="+QString().setNum(up)+" envWind @ up = "+QString().setNum(envWind[up])+" down = "+QString().setNum(down)+" envWind @ down = "+QString().setNum(envWind[down]));
	  //emit log(Message(dd));
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

	//startVelocity = envWind[dataHeight]*cos(elevAngle)*cos((180+envDir[dataHeight]-azimuth)*deg2rad);
	startVelocity = -1*envWind[dataHeight]*cos(elevAngle)*cos((envDir[dataHeight]-azimuth)*deg2rad);
	//if((currentRay->getRayIndex()%45) == 0) {
	  //QString d("In getStart rayIndex,sweepIndex = "+QString().setNum(currentRay->getRayIndex())+", "+QString().setNum(currentRay->getSweepIndex())+" azimuth = "+QString().setNum(azimuth)+" envWind @ dataHeight = "+QString().setNum(envWind[dataHeight])+" envDir @ dataHeight = "+QString().setNum(envDir[dataHeight])+" startVelocity = "+QString().setNum(startVelocity));
	  //emit log(Message(d));
	//}
      }
      else {
	startVelocity = velNull;
	//Message::toScreen("Start Velocity = VelNull");
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
      //if((currentRay->getRayIndex()%45)==0)
	//Message::toScreen("DRay #"+QString().setNum(currentRay->getRayIndex())+" startVelocity "+QString().setNum(startVelocity)+" nyquist = "+QString().setNum(nyquistVelocity));
      if((numVelocityGates!=0)&&(startVelocity!=velNull))
	{
	  float sum = float(numVGatesAveraged)*startVelocity;
	  float nyquistSum = float(numVGatesAveraged)*nyquistVelocity;
	  int n = 0;
	  int overMaxFold = 0;
	  bool dealiased;
	  float segVelocity[numVGatesAveraged];
	  for(int k = 0; k < numVGatesAveraged; k++)
	    {
	      segVelocity[k] = startVelocity;
	    }
	  for(int j = 0; j < numVelocityGates; j++)
	    {
	      //Message::toScreen("Gate "+QString().setNum(j));
	      if(vGates[j]!=velNull)
		{
		  //Message::toScreen("has data");
		  n = 0;
		  dealiased = false;
		  while(dealiased!=true)
		    {
		      float tryVelocity = numVGatesAveraged*(vGates[j]+(2.0*n*nyquistVelocity));
		      if((sum+nyquistSum >= tryVelocity)&&
			 (tryVelocity >= sum-nyquistSum))
			{
			  dealiased=true;
			} 
		      else 
			{
			  if(tryVelocity > sum+nyquistSum){
			    n--;
			    //Message::toScreen("n--");
			  }
			  if(tryVelocity < sum-nyquistSum){
			    n++;
			    //Message::toScreen("n++");
			  }
			  if(abs(n) >= maxFold) {
			    //emit log(Message(QString("Ray #")+QString().setNum(i)+QString(" Gate# ")+QString().setNum(j)+QString(" exceeded maxfolds")));
			    overMaxFold++;
			    dealiased=true;
			    vGates[j]=velNull;
			  }
			}
		      //Message::toScreen("Ray = "+QString().setNum(i)+" j = "+QString().setNum(j)+" with "+QString().setNum(n)+" folds");
		    }
		  if(vGates[j]!=velNull) 
		    {
		      vGates[j]+= 2.0*n*(nyquistVelocity);
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
  //Message::toScreen("Getting out of dealias");
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

void RadarQC::checkRay()
{
  Ray* check = radarData->getRay(1099);
  float* checkVel = check->getVelData();
  QString checkPrint;
  for(int r = 0; ((r < check->getVel_numgates())&&(r < 200)); r++)
    {
      checkPrint+=QString(" ")+QString().setNum(checkVel[r]);
    }
  checkVel = check->getRefData();
  QString checkPrint2;
  for(int r = 0; ((r < check->getRef_numgates())&&(r < 200)); r++)
    {
      checkPrint2+=QString(" ")+QString().setNum(checkVel[r]);
    }
  Message::toScreen("num vel gates = "+QString().setNum(check->getVel_numgates()));
  //Message::toScreen("Velocity");
  Message::toScreen(checkPrint); 
  Message::toScreen("num ref gates = "+QString().setNum(check->getRef_numgates()));
  //Message::toScreen("Reflectivity");
  Message::toScreen(checkPrint2);
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
