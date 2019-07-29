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
#include <cmath>
#include <QInputDialog>
#include <QString>

#include "RadarQC.h"
#include "RadarData.h"
#include "Message.h"
#include "Math/Matrix.h"

RadarQC::RadarQC(RadarData *radarPtr, QObject *parent)
    :QObject(parent)
{
    this->setObjectName("Radar QC");
    radarData = radarPtr;
    specWidthLimit = 10;
    velNull = -999.;
    maxFold = 4;
    numVGatesAveraged = 30;
    useVADWinds = false;
    useGVADWinds = false;
    useUserWinds = false;
    //  useAWIPSWinds = false;
    numCoEff = 3;
    vadLevels = 20;
    deg2rad = acos(-1.)/180.;
    int vcp = radarData->getVCP();
    float zero = 0.0;
    radarHeight = radarData->absoluteRadarBeamHeight(zero,zero);

    //Message::toScreen("Radar Height = "+QString().setNum(radarHeight)+" check units and values");

    // Allocate memory for the bincount
    int numSweeps = radarData->getNumSweeps();
    validBinCount = new float*[numSweeps];

    for (int i = 0; i < numSweeps; i++) {
        Sweep *currentSweep = radarData->getSweep(i);
        int numBins = (currentSweep->getVel_numgates() > 0) ? currentSweep->getVel_numgates() : 1;
        validBinCount[i] = new float[numBins];
    }

    // (Paul Harasti 4/2009)
    // Reflectivity does not need to be interpolated if
    // the super resolution vcp is not recombined to legacy,
    // (i.e., reflectivity already exists on all radial velocity tilts)
    // so skip to line number 256 if that is the case.
    // Tests for this by comparing gate spacings of tilts 1 and 2.

    Sweep *currentSweep = radarData->getSweep(0);
    float gatesp_sweep1 = currentSweep->getRef_gatesp();
    currentSweep = radarData->getSweep(1);
    float gatesp_sweep2 = currentSweep->getVel_gatesp();

    if( (gatesp_sweep1 != gatesp_sweep2) ) {

        // Used to determine how the reflectivity data should be interpolated based
        // on the vcp of the radar volume

        if( (vcp < 33)  || (vcp==211) || (vcp==212) ) {
            q = 3;
            if( (vcp == 12) || (vcp==212) ) {
                q = 5;
            }
            qinc = 2;
        }
        if( (vcp == 121) || (vcp==221) ) {
            q = 5;
            qinc = 4;
        }


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
  // Used for testing the interpolation of ref data onto other sweeps

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
                    // array out of bounds in j  - PH 10/2007
                    //      if(r > numRays)
                    if(r > numRays-1)
                        r -=numRays;
                    a = radarData->getRay(j+first)->getAzimuth();
                    b = radarData->getRay(r+first)->getAzimuth();
                    c = radarData->getRay(m+upperFirst)->getAzimuth();
                    if(clockwise) {
                        if((a > 350.0)&&(b < 10.0)&&(c < 10.0))
                            a -=360.;
                        if((a > 350.0)&&(b < 10.0)&&(c > 350.0))
                            b +=360.;
                        if((a <= c)&&(b >= c)){
                            ifoundit ++;
                            if(ifoundit==1) {
                                den = b-a;
                                if (den!=0.) {
                                    fac1[m] = (b-c)/den;
                                    fac2[m] = (c-a)/den;
                                    ipt1[m] = j+first;
                                    ipt2[m] = r+first;
                                }
                                else {
                                    // PH 10/2007. Need for special case. See comment below.
                                    fac1[m]=0.;
                                    fac2[m]=0.;
                                }
                                // Set up num ref gates
                                int numGates = radarData->getRay(j+first)->getRef_numgates();
                                if(radarData->getRay(r+first)->getRef_numgates() < numGates)
                                    numGates = radarData->getRay(r+first)->getRef_numgates();
                                int firstGate = radarData->getRay(j+first)->getFirst_ref_gate();
                                if(radarData->getRay(r+first)->getFirst_ref_gate() > firstGate)
                                    firstGate = radarData->getRay(r+first)->getFirst_ref_gate();
                                radarData->getRay(m+upperFirst)->emptyRefgates(numGates);
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
                            b+=360.0;
                        if((a >= c)&&(b <= c)) {
                            ifoundit++;
                            if(ifoundit==1) {
                                den = a-b;
                                if (den!=0.) {
                                    fac1[m] = (c-b)/den;
                                    fac2[m] = (a-c)/den;
                                    ipt1[m] = j+first;
                                    ipt2[m] = r+first;
                                }
                                else {
                                    fac1[m]=0.;
                                    fac2[m]=0.;
                                }
                            }
                        }
                    }
                }
            }
            for(int m = 0; m < upperNumRays; m++) {
                float *refValues1, *refValues2;
                refValues1 = radarData->getRay(ipt1[m])->getRefData();
                refValues2 = radarData->getRay(ipt2[m])->getRefData();
                Ray *current = radarData->getRay(m+upperFirst);
                float *newRef = current->getRefData();
                for(int k=current->getFirst_ref_gate();
                    k<current->getRef_numgates();k++) {
                    a = refValues1[k];
                    b = refValues2[k];
                    if((a!=velNull)&&(b!=velNull)) {
                        newRef[k] = a*fac1[m]+b*fac2[m];
                        // Handles case of sweep first ray and last ray  azimuths differences
                        // are  greater than 360 degrees(when j=numRay-1 and r=0 in a and b above),
                        // where fac1 and fac2 are automatically left at their initialized
                        // values of zero, and case where den is zero - PH 10/2007
                        if (fac1[m]!=0.&&fac2[m]!=0.) newRef[k] = velNull;
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
                refValues1 = NULL;
                refValues2 = NULL;
                //delete refValues1;
                //delete refValues2;
            }
        }

        // (Paul Harasti 4/2009)
        // End skip reflectivity interpolation for super resolution case.
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
                currentRay = NULL;
                delete currentRay;
            }
            aveVADHeight[n][v] /= float(count);
        }
    }
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

    if(!qcConfig.isNull()) {

        velMin = qcConfig.firstChildElement("vel_min").text().toFloat();
        velMax = qcConfig.firstChildElement("vel_max").text().toFloat();
        refMin = qcConfig.firstChildElement("ref_min").text().toFloat();
        refMax = qcConfig.firstChildElement("ref_max").text().toFloat();
        specWidthLimit = qcConfig.firstChildElement("sw_threshold").text().toFloat();
        numVGatesAveraged = qcConfig.firstChildElement("bbcount").text().toInt();
        maxFold = qcConfig.firstChildElement("maxfold").text().toInt();

        // Get Information on Environmental Wind Finding Methods

        wind_method = qcConfig.firstChildElement("wind_method").text();

        if(wind_method == QString("user")) {

            // Accept user entered parameters if the wind fields are known

            useUserWinds = true;
            envWind = new float[1];
            envDir = new float[1];
            envWind[0] = qcConfig.firstChildElement("windspeed").text().toFloat();
            envDir[0] = qcConfig.firstChildElement("winddirection").text().toFloat();
        }
        else {
            if (wind_method == QString("vad")) {

                // Get parameters for doing GVAD and VAD routines

                useVADWinds = true;
                // Possible parameters vadthr, gvadthr
                vadthr = qcConfig.firstChildElement("vadthr").text().toInt();
                vadLevels = qcConfig.firstChildElement("vadlevels").text().toInt();
                numCoEff = qcConfig.firstChildElement("numcoeff").text().toInt();
                gvadthr = 180;
            } else if (wind_method == QString("gvad")) {
                gvadthr = qcConfig.firstChildElement("gvadthr").text().toInt();
                vadLevels = 20;
                numCoEff = 2;
                vadthr = 30;
                useGVADWinds = true;
            } else {
                /*
      // Temporarily not concerned about this option
    if(wind_method == QString("known")) {
      useAWIPSWinds = true;
      AWIPSDir = qcConfig.firstChildElement("awips_dir").text();
    }

    else {
    */

                /*
       * Enable default settings in case environmental wind
       * method is not specified
       */

                emit log(Message(QString("Environmental Wind Method Not Specified for VAD"),0,this->objectName()));
                velMin = 1.5;
                velMax = 100;
                refMin = -15;
                refMax = 100;
                specWidthLimit = 10;
                numVGatesAveraged = 30;
                maxFold = 4;
                useGVADWinds = true;
                vadLevels = 20;
                numCoEff = 3;
                vadthr = 30;
                gvadthr = 180;
                //} Pull out because else was removed
            }
        }
    }
    // Set default parameters if configuration data is not available
    else {
        emit log(Message(QString("RadarQC: Quality Control Element is Not Valid, Using Default Parameters"),0,this->objectName()));
        velMin = 1.5;
        velMax = 100;
        refMin = -15;
        refMax = 100;
        specWidthLimit = 10;
        numVGatesAveraged = 30;
        maxFold = 4;
        useGVADWinds = true;
        vadthr = 30;
        gvadthr = 180;
    }
}



bool RadarQC::dealias()
{  

    /*
   *  Preforms basic quality control algorithms and velocity dealiasing
   *  on a single radar volume
   *
   */

    // PH 10/2007.  Changed order of QC.  Better to threshold
    // first to prevent velocity of clutter from being artifically
    // incremented beyond the clutter threshold  limit - clutter typically has large
    // reflectivity which could cause a large enough terminal velocity correction
    // to push clutter velocity beyond the assume +-1.5 m/s clutter threshold.

    thresholdData();
    emit log(Message(QString(),1,this->objectName()));

    if(!terminalVelocity())
        return false;
    emit log(Message(QString(),1,this->objectName()));
    
    if(!findEnvironmentalWind()) {
        Message::toScreen("Failed finding environmental wind");
    }

    emit log(Message(QString(),1,this->objectName()));
	

    if(!BB()) {
        Message::toScreen("Failed in Bargen-Brown dealising");
        return false;
    }

	/* if(!multiprfDealias())
		return false; */

	if(!derivativeDealias())
		return false;

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
        int numBins = currentSweep->getVel_numgates();
        for (int j=0; j < numBins; j++)
            validBinCount[i][j]=0.;
    }

    int numRays = radarData->getNumRays();
    Ray* currentRay;
    int numVGates = 0;

    for(int i = 0; i < numRays; i++)
    {
        if(i == (int) numRays/2.0)
            emit log(Message(QString(),1,this->objectName()));
        currentRay = radarData->getRay(i);
	if (currentRay == NULL) {
	  std::cout << "No ray at index " << i << std::endl;
	  continue;
	}
        int sweepIndex = currentRay->getSweepIndex();
	if (sweepIndex < 0)
	  continue;
	
        Sweep *currentSweep = radarData->getSweep(sweepIndex);
	if(currentSweep == NULL)
	  continue;

        int numSweepGates = currentSweep->getVel_numgates();
	int numRayGates = currentRay->getVel_numgates();
	if (numRayGates < numSweepGates) {
	  numVGates = numRayGates;
	} else {
	  numVGates = numSweepGates;
	}
        //      float velGateSp = currentRay->getVel_gatesp();
        //      float refGateSp = currentRay->getRef_gatesp();
        float *vGates = currentRay->getVelData();
        float *swGates = currentRay->getSwData();
	// float *refGates = currentRay->getRefData();
        if (swGates != NULL) {
            for (int j = 0; j < numVGates; j++)
            {
                //         if(j<20) {
                //         vGates[j]=velNull;
                //       }
                //	  int jref = int((float)j * velGateSp / refGateSp);
                //	  if(jref >= currentRay->getRef_numgates())
                //	    jref = int(velNull);
                if((swGates[j] > specWidthLimit)||
                   (fabs(vGates[j]) < velMin) ||
                   (fabs(vGates[j]) > velMax))
                    //||(jref==velNull)||(refGates[jref] < refMin)
                    //  ||(refGates[jref] > refMax))
                {
                    // This was extended to threshold against min and max
                    // reflectivity as well but we are not currently using
                    // these thresholds - LM
                    vGates[j] = velNull;
                }
                
                if(vGates[j]!=velNull) {
                    validBinCount[sweepIndex][j]++;
                }
            }
        } else {
            // No spectrum width data, so just go with it for now
	  if(vGates != NULL)
            for (int j = 0; j < numVGates; j++) {
                if(vGates[j]!=velNull) {
                    validBinCount[sweepIndex][j]++;
                }
            }
        }
        vGates = NULL;
        swGates = NULL;
        // refGates = NULL;
    }
    currentRay = NULL;
    delete currentRay;
}


bool RadarQC::terminalVelocity()
{

    /*
   *  This algorithm is used to remove the terminal velocity component
   *  from each valid doppler velocity reading in the radar volume.
   */

    float ae = 6371.*4./3.; // Adjustment factor for 4/3 Earth Radius (in km)
    int numRays = radarData->getNumRays();
    int numVGates;
    Ray* currentRay;

    for(int i = 0; i < numRays; i++)
    {
        currentRay = radarData->getRay(i);
	
	if (currentRay->getSweepIndex() == -999)
	  continue;

	
        numVGates = currentRay->getVel_numgates();
        float *vGates = currentRay->getVelData();
        float *rGates = currentRay->getRefData();

	// Some rays might not have VEL data
	if ( vGates == NULL )
	  continue;

        if((currentRay->getRef_gatesp()!=0)&&(currentRay->getVel_gatesp()!=0)&&(currentRay->getRef_numgates()!=0))
        {
            for(int j = 0; j < numVGates; j++)
            {
                if(vGates[j]!=velNull)
                {
                    // PH 10/2007.  need accurate range - previously missing first gate distance
                    // which  has usually been -0.375 m (due to radar T/R time delay) but is now
                    // 0.125 m for VCP 211.
                    //		  float range = j*currentRay->getVel_gatesp()/1000.0;
                    float range = float(currentRay->getFirst_vel_gate()+(j*currentRay->getVel_gatesp()))/1000.;
                    if (range<0.) range=0.;
                    float elevAngle = currentRay->getElevation();

		    int sweepIndex = currentRay->getSweepIndex();
		    if (sweepIndex < 0)
		      continue;

                    float height = aveVADHeight[sweepIndex][j];
                    // height is in km from sea level here

                    float rho = 1.1904*exp(-1*height/9.58);
                    float theta = elevAngle*deg2rad+asin(range*cos(deg2rad*elevAngle)/(ae+height-radarHeight));
                    int zgate = 0;
                    /* I think this next step makes assumptions about
           * the reflectivity of the gate spacing
           */

                    // PH 10/2007.  Previous logic not accurate enough.
                    // The objective is to use the Z datum that is within +- 0.5 km of the
                    // current radial velocity datum's range - previous accuracy was only 1 km.
                    //                 int zgate = int(floor(range/rgatesp)+1);

                    // Paul Harasti 3/2009: Add logic for Super Resolution Z gate spacing
                    //					float rgatesp = currentRay->getRef_gatesp()/1000.0;
                    //                  int zgate = int(floor(0.5+range/rgatesp));
                    // Ref_gatesp = 1 km at all times when not super resolution
                    // and division by rgatesp does not work for super resolution case
                    // since range is offset by first gate distance rendering range
                    // unevenly divisible by rgatesp.

                    if(currentRay->getRef_gatesp()!=currentRay->getVel_gatesp()) {
						zgate = (int)(floor(0.5+(range*1000.0 - currentRay->getFirst_ref_gate())/currentRay->getRef_gatesp()));
                        /*zgate = int(floor(0.5+(currentRay->getFirst_vel_gate()-currentRay->getFirst_ref_gate())+
                                          +j*currentRay->getRef_gatesp()/currentRay->getVel_gatesp())); */
                        //if(zgate<1) zgate = 1;
                        // Why don't we use the first gate? -LM
                        // PH 10/2007.  Because it is at range = 0 km for current 88D VCPs.
                    }
                    else {
                        // Ref_gatesp = Vel_gatesp at all times when  super resolution
                        zgate = j;
                    }
                    if(zgate >= currentRay->getRef_numgates())
                        zgate = currentRay->getRef_numgates()-1;
					if (zgate < 1) zgate = 1;
                    float zData = rGates[zgate];
                    //                  int zwhere=currentRay->getSweepIndex();
                    //                  if ((zwhere < 22)&&(zData > 0.)&&(zData < 100.))
                    //  Message::toScreen("SweepNo = "+QString().setNum(zwhere));
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
                    if(!std::isnan(terminalV)) {
                        vGates[j] -= terminalV;
                    }
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
        vGates = NULL;
        rGates = NULL;
    }
    currentRay = NULL;
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

    // PH 10/2007.  need accurate range - previously missing first gate distance
    // which  has usually been -0.375 m (due to radar T/R time delay) but is now
    // 0.125 m for VCP 211.

    float range = float(currentRay->getFirst_vel_gate()+(gateIndex*currentRay->getVel_gatesp()))/1000.;
    if (range<0.) range=0.;
    //  float range = gateIndex*currentRay->getVel_gatesp()/1000.0;
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

    if(useUserWinds) {
        // Do nothing move straight to BB
        return true;
    }

    if(useGVADWinds) {

        // Attempt to use GVAD algorithm to determine
        //  environmental winds

        bool useGVAD = true;

        if(findVADStart(useGVAD))
            return true;

        // If GVAD fails attempt to use the VAD algorithm to
        //   determine environmental winds.

        else {
	  return findVADStart(!useGVAD);
        }
    }
    if(useVADWinds) {

        // Attempt to use GVAD algorithm to determine
        //  environmental winds
        bool useGVAD = false;

        return findVADStart(useGVAD);
    }

    return false;
}

bool RadarQC::findVADStart(bool useGVAD)
{

    QString boolString("True");
    if(!useGVAD){
        boolString = "False";
    }

    // This method is used to run either the VAD or GVAD algorithms including
    // any intiailization or preparation algoriths

    int numSweeps = radarData->getNumSweeps();
    envWind = new float[vadLevels];
    envDir = new float[vadLevels];
    vadFound = new bool[vadLevels];
    sumwt = new float[vadLevels];
    //vadRMS = new float[vadLevels];  // does this ever get used??
    highVelGate = new int*[vadLevels];
    lowVelGate = new int*[vadLevels];
    hasVelData = new bool*[vadLevels];

    for(int m = 0; m < vadLevels; m++) {

        // create dynamic arrays for VAD wind finder

        highVelGate[m] = new int[numSweeps];
        lowVelGate[m] = new int[numSweeps];
        hasVelData[m] = new bool[numSweeps];

        // initialize many arrays
        vadFound[m] = false;
        sumwt[m] = 0;
        envWind[m] = 0;
        envDir[m] = 0;
        //vadRMS[m] = 0;
        for(int n = 0; n < numSweeps; n++) {
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

    // I remain very confused about all these variables and the interations
    // of the algorithms after failures. Do we need to try GVAD multiple times
    // if it fails the first - or do we just move on to VAD. This part was
    // very confusing to me -LM
    // Here we just try GVAD once and if that fails move into VAD

    if(useGVAD)
        thr = gvadthr;
    else
        thr = vadthr;

    // PH 10/2007.  Added new logic for previously omitted
    // 5 iterations for more robust GVAD and VAD estimates
    // - includes adjustment of positions of previous delete
    // statements and ellimination of old and unnecessary
    // last_count_up and last_count_low variables

    // Begin 5 iterations
    for (int i=0; i < 5; i++) {

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
                        // Need to check this due to potential mismatch between Sweep and Ray
                        if (vel != NULL) {
                            lowLevelVel[index] = vel[lowestGate];
                            highLevelVel[index] = vel[highestGate];
                            index++;
                        }
                        vel = NULL;
                        delete vel;
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
            }
        }

        // ADD some sort of checking and repeat logic
        // What should we check against? What adjustments need to be made
        // to get better results from a second analysis?

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
                // PH 10/2007.  Adjusted so that m=0 is useful and in agreement
                // with parent FORTRAN code. Previously, m=0 included negative heights.
                if(radarData->getSweep(n)->getVel_numgates() > 0) {
                    float dHeightL=1.0+m-(aveVADHeight[n][lowVelGate[m][n]]-radarHeight)*3.281;
                    //			  float dHeightL=m-(aveVADHeight[n][lowVelGate[m][n]]-radarHeight)*3.281;
                    float dHeightU=(aveVADHeight[n][highVelGate[m][n]]-radarHeight)*3.281 - m -1.0;
                    //			  float dHeightU=(aveVADHeight[n][highVelGate[m][n]]-radarHeight)*3.281 - m;
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


        for(int m = 0; m < vadLevels; m++) {

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

        delete [] lowSpeed;
        delete [] highSpeed;
        delete [] lowDir;
        delete [] highDir;
        delete [] lowRMS;
        delete [] highRMS;

        delete [] meanSpeed;
        delete [] meanDir;
        delete [] meanRMS;

        // end 5 iterations - 10 GVAD/VAD attempted estimates per sweep per layer
    }

    bool foundVelocity = false;

    for(int m = 0; m < vadLevels; m++) {
        if(vadFound[m] && (envWind[m]!=velNull)) {
            envWind[m] *= (1/sumwt[m]);
            envDir[m] *= (1/sumwt[m]);
            //vadRMS[m] = sqrt(1/sumwt[m]);
            foundVelocity = true;
            if(envWind[m]>velMax) {
                envWind[m] = velNull;
                envDir[m] = velNull;
                //vadRMS[m] = velNull;
            }

//            if(!useGVAD) {
//                emit log(Message("Level:"+ QString().setNum(m)+" VAD SPEED: "
//                                 +QString().setNum(envWind[m])
//                                 +" VAD DIR: "+QString().setNum(envDir[m])));
//                //Message::toScreen("m = "+QString().setNum(m)+" VAD Wind Speed = "+QString().setNum(envWind[m])+" VAD Wind Direction = "+QString().setNum(envDir[m]));
//            }
//            else {
//                emit log(Message("Level:"+ QString().setNum(m)+" GVAD SPEED: "
//                                 +QString().setNum(envWind[m])
//                                 +" GVAD DIR: "+QString().setNum(envDir[m])));
//                //Message::toScreen("m = "+QString().setNum(m)+" GVAD Wind Speed = "+QString().setNum(envWind[m])+" GVAD Wind Direction = "+QString().setNum(envDir[m]));
//            }
        }
        else {
            envWind[m] = velNull;
            envDir[m] = velNull;
            //emit log(Message("Level:"+QString().setNum(m)+" No VAD Speed Found"));
        }

    }


    for(int m = 0; m < vadLevels; m++) {

        delete [] highVelGate[m];
        delete [] lowVelGate[m];
        delete [] hasVelData[m];

    }

    delete [] vadFound;
    delete [] sumwt;
    delete [] highVelGate;
    delete [] lowVelGate;
    delete [] hasVelData;


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
            hasVelData[m][n] = false;
            if(currentSweep->getVel_numgates() > 0) {
                int numVBins = currentSweep->getVel_numgates();
                float max_up = 0.;
                float max_low = 0.;
                bool hasHighVel = false;
                bool hasLowVel = false;
                for(int v = 0; v < numVBins; v++) {
                    float height = (aveVADHeight[n][v] - radarHeight)*3.281;
                    // Here height is in thousands of feet relative to radar (AGL?)
                    // PH 10/2007.  Adjusted so that m=0 is useful and in agreement
                    // with parent FORTRAN code. Previously, m=0 included negative heights
                    if((height >= m+1)&&(height < m+2)) {
                        //	  if((height >= m)&&(height < m+1)) {
                        if(validBinCount[n][v] > max_up) {
                            max_up = validBinCount[n][v];
                            highVelGate[m][n] = v;
                        }
                        if(max_up >= thr) {
                            hasHighVel = true;
                        }

                    }
                    if((height >= m)&&(height < m+1)) {
                        //	  if((height >= m-1)&&(height < m)) {
                        if(validBinCount[n][v] > max_low) {
                            max_low = validBinCount[n][v];
                            lowVelGate[m][n] = v;
                        }
                        if(max_low >= thr) {
                            hasLowVel = true;
                        }
                    }
                }
                if(hasHighVel && hasLowVel) {
                    hasVelData[m][n] = true;
                    validBinCount[n][highVelGate[m][n]] = 0.;
                    validBinCount[n][lowVelGate[m][n]] = 0.;
                    goodRings++;
                    //	  Message::toScreen("m = "+QString().setNum(m)+" n = "+QString().setNum(n)+" last count up = "+QString().setNum(max_up)+" last count low = "+QString().setNum(max_low));
                }
            }
        }
    }
    //  Message::toScreen("How many rings have vel data after vad prep: "+QString().setNum(goodRings));
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
    // This was the number used in lls_gvad.f & lls_vad.f on the last version
    // I saw, so it is set for number comparisons - LM

    if((nyqVel == 0)||(fabs(nyqVel)>90)) {
        emit log(Message(QString("Nyquist Velocity Not Defined - Dealiasing Not Possible"),0,this->objectName()));
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
    for(int i = 0; i < vadNumCoEff; i++)
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
        emit log(Message(QString("VAD failed in lls"),0,this->objectName()));
        return false;
    }

    /*
    This does not make use of the stError values,
    Nor does it recognise coefficients higher than 2
    Per Paul's Code.
  */

    // PH 10/2007. Elevation calculated previously, but not used.
    // Replaced redundant elevAngle that was not in original FORTRAN code.
    //  float elevAngle = currentSweep->getElevation();
    // elevAngle *= deg2rad;
    elevation *= deg2rad;
    speed = sqrt(coEff[1]*coEff[1]+coEff[2]*coEff[2])/cos(elevation);
    direction = atan2(-1*coEff[1], -1*coEff[2])/deg2rad;
    if(direction < 0)
        direction+=360;
    rms = stDeviation;
    return true;
}

bool RadarQC::GVAD(float* &vel, Sweep* &currentSweep,float &speed, float &direction, float &rms)
{

    speed = velNull;
    direction = velNull;
    rms = velNull;
    //  numCoEff = 2;
    int gvadnumCoEff=2;

    int numData = 0;
    int numRays = currentSweep->getNumRays();
    int start = currentSweep->getFirstRay();
    int stop = currentSweep->getLastRay();
    float nyqVel = currentSweep->getNyquist_vel();
    //Message::toScreen("Nyquist vel is "+QString().setNum(nyqVel));

    //could be added for more than 3 numCoefficients  int N = numCoEff/2;
    // the numCoeff is set for comparison with the May 06 lls_gvad.f file
    // I was looking at which is my most recent copy -LM

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
    //  Message::toScreen("Ave Elev = "+QString().setNum(elevation));


    float pi = acos(-1.);

    //  int n_filter = 10;
    // hardwire -PH 10/2007
    float n_filter=11.;

    //  int width = n_filter/2;
    // hardwire -PH 10/2007
    int width = 5;

    if((nyqVel == 0)||(fabs(nyqVel)>90)) {
        emit log(Message(QString("Nyquist Velocity Not Defined - Dealiasing Not Possible"),0,this->objectName()));
	    delete [] gvr;
	    delete [] gve;
        return false;
    }

    // Used to be fill_vad routine
    // PH 10/2007.  r beginning and end indices need adjusting. See comments below.
    //  for(int r = 0; r < numRays; r++) {

    for(int r = 1; r < numRays-1; r++) {

        // Major error - should be fabs(vel[r])) - was causing most of
        // negative radial velocities to be set missing in GVAD analysis.
        // This is redundant QC (already done by default)
        // that should not be done twice anyway.
        // PH 10/2007.
        //    if(vel[r] < 1.0) {
        //      vel[r] = velNull;
        //    }
        // Paul's code went to r-1 ?????  why does it iterate through that pt
        // instead of r ? -LM
        // Answer: most older VCP WSR-88D PPIs overshoot 360 degrees of azimuth range
        // from r=0 to r=numRays-1 (except then new VCP 211,
        // and perhaps 212 and 221 as well - need to check)
        // by 2 to 32 degrees (usually 7 degrees); thus, r=0 should
        // not be assumed to be the ray immediately before r=numRays-1. Therefore,
        // your interpolating between the last and first ray caused bad estimates.
        // PH 10/2007 - for loop above and code below now correct.
        int last_r = r-1;
        //    if(last_r == -1)
        //     last_r = numRays-1;
        int next_r = r+1;
        // Didn't have this for good reason - see above. PH 10/2007.
        //    if(next_r > (numRays-1))  // Paul's code didn't have this
        //      next_r = 0;
        if(vel[r] == velNull) {
            if((vel[last_r] != velNull)&&(vel[next_r]!=velNull)) {
                vel[r] = (vel[last_r]+vel[next_r])/2.0;
            }
        }
    }

    //-----end of what used to be fill_vad

    // PH 10/2007. Last ray should not be included,
    // for same reason given immediately above.
    //  for(int r = 0; r < numRays; r++) {

    for(int r = 0; r < numRays-1; r++) {
        int rr = r+1;
        //    if(rr > (numRays-1))
        //      rr = 0;
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
            if((fabs(vel[rr]-vel[r])> nyqVel)||(fabs(deltaA)<0.001)) {
                gve[r] = velNull;
            }
            else {
                gve[r] = (vel[rr]-vel[r])/deltaA;
            }
        }
    }


    int count;

    // PH 10/2007. Section re-worked because -width
    // before first ray and +width after last ray would
    // cause a similar but worse problem as above.
    // Problem solved by excluding data
    // less than  r=0 and greater than r=numRays-1.

    //  for(int r = 0; r < numRays; r++) {

    for(int r = 0; r < numRays; r++) {
        if ((r >= width) && (r < numRays-width)) {
            count = 0;
            int last_r = r-1;
            int next_r = r+1;
            //    if(next_r > (numRays-1))
            //      next_r = 0;
            //    if(last_r < 0)
            //      last_r = numRays-1;
            if((gve[r] == velNull)&&(gve[next_r] == velNull)
                    &&(gve[last_r]==velNull)) {
                gvr[r] = velNull;
            }
            else {
                for(int w = -1*width; w <= width; w++) {
                    int ww = w+r;
                    //	if(ww < 0)
                    //	  ww += numRays;
                    //	if(ww > (numRays-1))
                    //	  ww -= numRays;
                    if(gve[ww] != velNull)
                        count++;
                }
                float sumwgt = 0;
                float wgt;
                if(count >= width) {
                    for(int w = -1*width; w <= width; w++) {
                        int ww = w+r;
                        //	  if(ww < 0)
                        //	    ww += numRays;
                        //	  if(ww > (numRays-1))
                        //	    ww -= numRays;
                        if(gve[ww] != velNull) {
                            // PH 10/2007. Now hardwired with new n_filter equal to old plus one
                            //    wgt = 1.0-abs(w)*1.0/float(n_filter+1);
                            wgt = 1.0-float(abs(w))*1.0/n_filter;
                            gvr[r]+=wgt*gve[ww];
                            sumwgt += wgt;
                        }
                    }
                    if (sumwgt < 0.01) Message::toScreen("Error-DivisionByZero: sumwgt = "+QString().setNum(sumwgt));
                    gvr[r] *= (1.0/sumwgt);
                }
                else {
                    gvr[r] = velNull;
                }
            }
        }
        else {
            gvr[r] = velNull;
        }
    }
    numData = 0;
    for(int r = 0; r < numRays; r++) {
        // PH 10/2007. Does not work - causes false and huge GVAD wind speeds
        // by allowing gvr[r] missing data into the least squares analysis
        //    if(fabs(vel[r]) <= 90.0) {
        if(gvr[r]!=velNull) {
            numData++;
        }
        //  float el_where=radarData->getRay(r)->getSweep();
        if (r==0) {
            //    Message::toScreen("Elevation No. = "+QString().setNum(el_where));
            //    Message::toScreen("Numrays = "+QString().setNum(numRays));
        }
    }
    //  Message::toScreen("numData = "+QString().setNum(numData));
    //Matrix::printMatrix(vel, numRays);

    float **X, *Y;
    X = new float*[gvadnumCoEff];
    Y = new float [numData];
    for(int i = 0; i < gvadnumCoEff; i++) {
        X[i] = new float[numData];
        for(int j = 0; j < numData; j++) {
            X[i][j] = -777.;
            Y[j] = -777.;
        }
    }

    int dataIndex = 0;
    for(int r = 0; r < numRays; r++) {
        //    if(fabs(vel[r]) <= 90.0) {
        if(gvr[r]!=velNull) {
            float rAzimuth = radarData->getRay(r+start)->getAzimuth() *deg2rad;
            //  Message::toScreen("deg2rad = "+QString().setNum(deg2rad));
            //  Message::toScreen("rAzimuth = "+QString().setNum(rAzimuth/deg2rad));
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
    coEff = new float[gvadnumCoEff];
    stError = new float[gvadnumCoEff];

    if(! Matrix::lls(gvadnumCoEff, numData, X, Y, stDeviation, coEff, stError)) {
	    for(int i = 0; i < numCoEff; i++) {
	        delete [] X[i];
	    }
	    delete [] X;
	    delete [] Y;
	    delete [] coEff;
	    delete [] stError;
	    delete [] gvr;
	    delete [] gve;
        return false;	
	}

    // PH 10/2007. Elevation calculated previously, but not used.
    // Replaced redundant elevAngle that was not in original FORTRAN code.
    //float elevAngle = currentSweep->getElevation();
    //  Message::toScreen("Elevation Angle  = "+QString().setNum(elevation));
    elevation *= deg2rad;

    //  elevAngle *= deg2rad;
    speed = sqrt(coEff[0]*coEff[0]+coEff[1]*coEff[1])/cos(elevation);
    direction = atan2(-1*coEff[1], coEff[0])/deg2rad;
    // Paul uses different coefficents in VAD?!?!?!? - I don't understand - LM
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
    if((useVADWinds) or (useGVADWinds)) {

        int dataHeight = -1;
        int numVBins = currentRay->getVel_numgates();
        float *velGates = currentRay->getVelData();
        float elevAngle = currentRay->getElevation()*deg2rad;
        float azimuth = currentRay->getAzimuth();
        bool hasDopplerData = false;
        bool envWindFound = false;

	// Some rays might not have VEL data
	if (velGates == NULL)
	  return startVelocity;
	
        for(int v = 0; (v < numVBins) && (!hasDopplerData); v++) {
            if(velGates[v] != velNull) {
                hasDopplerData = true;

		int sweepIndex = currentRay->getSweepIndex();
		if (sweepIndex < 0)
		  continue;

	dataHeight = int(floor((aveVADHeight[sweepIndex][v]-radarHeight)*3.281 + 0.5));
	//if(std::isnan(dataHeight)||isinf(dataHeight))
	//  Message::toScreen("RadarQC getting funky... sweep = "+QString().setNum(currentRay->getSweepIndex())+" ray# = "+QString().setNum(currentRay->getRayIndex())+" v# = "+QString().setNum(v));
	if(dataHeight < 0) {
	  dataHeight = 0;
	}
	if(dataHeight > vadLevels-1) dataHeight = vadLevels - 1;
            }
        }
        velGates = NULL;
        if(hasDopplerData) {
            if(envWind[dataHeight] != velNull) {
                envWindFound = true;
            }
            else {
                int up = 0;
                int down = 0;
                // PH 10/2007.  Previous logic was incorrect - was choosing furthest point,
                // not closest point as required.
                //      for(int h = 1; (!envWindFound||((up!=vadLevels-1)&&(down != 0))); h++) {
                for(int h = 1; (!envWindFound&&((up!=vadLevels-1)||(down != 0))); h++) {
                    up = dataHeight + h;
                    if(up >= vadLevels) {
                        up = vadLevels-1;
                    }
                    down = dataHeight - h;
                    if(down < 0) {
                        down = 0;
                    }
                    if((envWind[up]!= velNull)||(envWind[down]!=velNull)) {
                        if(envWind[up]!= velNull) {
                            dataHeight = up;
                        }
                        else {
                            dataHeight = down;
                        }
                        envWindFound = true;
                    }
                }
            }
            if(envWindFound) {

                startVelocity = envWind[dataHeight]*cos(elevAngle)*cos((180+envDir[dataHeight]-azimuth)*deg2rad);
                //startVelocity = -1*envWind[dataHeight]*cos(elevAngle)*cos((envDir[dataHeight]-azimuth)*deg2rad); //- not same as paul's my bad -LM
            }
            else {
                startVelocity = velNull;
                //Message::toScreen("Start Velocity = VelNull");
            }
        }
        else
            startVelocity = velNull;
    }
    /*
    if(useAWIPSWinds) {
    // Not yet implemented
    return 0;
    }
  */
    return startVelocity;
}

bool RadarQC::BB()
{
    //emit log(Message("In BB"));
    Ray* currentRay = NULL;
    float numRays = radarData->getNumRays();
    for(int i = 0; i < numRays; i++)
    {
        currentRay = radarData->getRay(i);
	if (currentRay->getSweepIndex() == -999)
	  continue;
	
        float *vGates = currentRay->getVelData();

	// Some rays might not have VEL data
	
	if(vGates == NULL)
	  continue;
	
        float startVelocity = getStart(currentRay);
        float nyquistVelocity = currentRay->getNyquist_vel();
        int numVelocityGates = currentRay->getVel_numgates();
        if((numVelocityGates!=0)&&(startVelocity!=velNull))
        {
            float sum = float(numVGatesAveraged)*startVelocity;
	    float median, mean;
	    mean = median = startVelocity;
	    // float nyquistSum = float(numVGatesAveraged)*nyquistVelocity;
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
                        float tryVelocity = vGates[j]+(2.0*n*nyquistVelocity);
                        if((median+nyquistVelocity >= tryVelocity)&&
                                (tryVelocity >= median-nyquistVelocity))
                        {
                            dealiased=true;
                        }
                        else
                        {
                            if(tryVelocity > median+nyquistVelocity){
                                n--;
                                //Message::toScreen("n--");
                            }
                            if(tryVelocity < median-nyquistVelocity){
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
						mean = sum / numVGatesAveraged;
						float threshold_mean = 0.0;
						int meancount = 0;
						QList<float> sortVelocity;
                        for(int m = 0; m < numVGatesAveraged-1; m++)
                        {
							sortVelocity << segVelocity[m];
                            segVelocity[m] = segVelocity[m+1];
							if (fabs(segVelocity[m] - mean) < nyquistVelocity)
							{
								threshold_mean += segVelocity[m];
								meancount++;
							}							
                        }
						sortVelocity << vGates[j];
						segVelocity[numVGatesAveraged-1] = vGates[j];
						threshold_mean += vGates[j];
						meancount++;
						//if (meancount < numVGatesAveraged) Message::toScreen(QString("Threshold reduced to ") + QString().setNum(meancount));
						qSort(sortVelocity);
						int mIndex = float(numVGatesAveraged)/2;
						median = (sortVelocity.at(mIndex));// + (threshold_mean / (float)meancount)) / 2;
                    }
                }
            }
        }
        vGates = NULL;
        currentRay = NULL;
    }
	
    //Message::toScreen("Getting out of dealias");
    return true;
}

bool RadarQC::derivativeDealias()
{
	
	// Minimize 2nd derivative in azimuth after BB routine
	for (int n = 0; n < radarData->getNumSweeps(); n++) {
        Sweep* currentSweep = radarData->getSweep(n);
		int rays = currentSweep->getNumRays();
		int gates = currentSweep->getVel_numgates();
        if (gates == 0) continue;

		float nyquistVelocity = currentSweep->getNyquist_vel();
		// Allocate memory for the gradient fields
		float** a1 = new float*[rays];
		float** veldata = new float*[rays];
		for (int i=0; i < rays; i++) {
			a1[i] = new float[gates];
			veldata[i] = new float[gates];
            for (int j=0; j < gates; j++) {
                a1[i][j] = veldata[i][j] = velNull;
            }
		}
		
		// Find the gradient
		float sum;
		int ray_index;
		for (int i=0; i < rays; i++)  {
			for (int j=0; j < gates; j++) {
				sum = 0.0;
				double weights[5] = { 1./12., -2./3., 0, 2./3., -1./12. }; 
				//double weights[2] = {-1.0, 1.0};
				sum = 0;
				for (int m = i-2; m < i+3; m++) {
					//for (int m = i; m < i+2; m++) {
					ray_index = m + currentSweep->getFirstRay();
					if (ray_index < currentSweep->getFirstRay()) ray_index += rays;
					if (ray_index > currentSweep->getLastRay()) ray_index -= rays;
					Ray* currentRay = radarData->getRay(ray_index);
					float* raydata = currentRay->getVelData();
					int ri = (m >= rays) ? (m-rays) : m;
					ri = (ri < 0) ? (ri+rays) : ri;
					if ((raydata != NULL) and (j < currentRay->getVel_numgates())) {
					   veldata[ri][j] = raydata[j];
				    }
					if (veldata[ri][j] != velNull) {
						sum += weights[m-i+2]*veldata[ri][j];
						//sum += weights[m-i]*veldata[i][j];
					} else {
						sum = velNull;
						break;
					}
				}
				if (sum != velNull) 
					a1[i][j] = fabs(sum);
			}
		}
		for (int j=0; j < gates; j++) {
			float mingrad = 1e34;
			int startindex = 0;
			for (int i=0; i < rays; i++)  {
				if ((a1[i][j] != velNull) and (a1[i][j] < mingrad)) {
					mingrad = a1[i][j];
					startindex = i;
				}
			}
			// Use a much smaller azimuthal average because of radial shear across eyewall
			int azavg = 1; //numVGatesAveraged / 6;
			float startVelocity = veldata[startindex][j];
			if(startVelocity!=velNull)
			{
				float sum = float(azavg)*startVelocity;
				float median, mean;
				mean = median = startVelocity;
				float segVelocity[azavg];
				for(int k = 0; k < azavg; k++)
				{
					segVelocity[k] = startVelocity;
				}
				for (int ri=startindex; ri < rays+startindex; ri++)
				{
					
					int i = (ri >= rays) ? (ri-rays) : ri;
					
					//Message::toScreen("Gate "+QString().setNum(j));
					if(veldata[i][j]!=velNull)
					{
						int minfold = 0;
						mingrad = 1e34;
						for (int fold = -1; fold < 2; fold++)
						{
							//double weights[5] = { -1./12., 4./3., -5./2., 4./3., -1./12. }; 
							double weights[3] = {1.0, -2.0, 1.0};
							sum = 0;
							//for (int m = i-2; m < i+3; m++) {
							for (int m = i-1; m < i+2; m++) {
								int ri = (m >= rays) ? (m-rays) : m;
								ri = (ri < 0) ? (ri+rays) : ri;
								if (veldata[ri][j] != velNull) {
									float tryVelocity = veldata[ri][j];
									if (ri == i) tryVelocity += (2.0*fold*nyquistVelocity);
									sum += weights[m-i+1]*tryVelocity;
									//sum += weights[m-i]*veldata[i][j];
								} else {
									sum = velNull;
									break;
								}
							}
							if ((sum != velNull) and (fabs(sum) < mingrad)) {
								mingrad = sum;
								minfold = fold;
							}
						}
						if(veldata[i][j]!=velNull)
						{
							veldata[i][j]+= 2.0*minfold*(nyquistVelocity);
							ray_index = i + currentSweep->getFirstRay();
							Ray* currentRay = radarData->getRay(ray_index);
							float* raydata = currentRay->getVelData();
							if ((raydata != NULL) and (j < currentRay->getVel_numgates())) {
							   raydata[j] = veldata[i][j];
						    }
						}
					}
				}
			}			
		}
		for (int i=0; i < rays; i++)  {
			delete[] veldata[i];
			delete[] a1[i];
		}
		delete[] veldata;
		delete[] a1;
		
	}
    //Message::toScreen("Getting out of dealias");
    return true;
}	

bool RadarQC::multiprfDealias()
{
	int maxgates = 0;
	for (int n = 0; n < radarData->getNumSweeps(); n++) {
        Sweep* currentSweep = radarData->getSweep(n);
		if (currentSweep->getVel_numgates() > maxgates) maxgates = currentSweep->getVel_numgates();
	}
	
	for (int theta = 0; theta < 360; theta++) {
		float raydata[10][maxgates];
		float nyquists[10];
		int rayindex[10];
		for (int i = 0; i < 10; i++) {
			nyquists[i] = 0.0;
			rayindex[i] = -999;
			for (int j = 0; j < maxgates; j++) {
				raydata[i][j] = velNull;
			}
		}		
		int raycount = 0;
		for (int n = 0; n < radarData->getNumRays(); n++) {
			Ray* currentRay = radarData->getRay(n);
			if (currentRay->getElevation() > 0.75) continue;
			//float phi = deg2rad * (90. - (currentRay->getElevation()));
			
			float azdiff = fabs((float)theta - currentRay->getAzimuth());
			if ((360.0 - azdiff) < azdiff) azdiff = 360.0 - azdiff;
			if (azdiff > 0.5) continue;
			if (currentRay->getVel_numgates() == 0) continue;
			float *vGates = currentRay->getVelData();
			nyquists[raycount] = currentRay->getNyquist_vel();
			for (int j = 0; j < currentRay->getVel_numgates(); j++) {
				raydata[raycount][j] = vGates[j];
			}
			rayindex[raycount] = n;
			raycount++;
			if (raycount == 10) break;
		}
		
		for (int j = 0; j < maxgates; j++) {
			float folddata[10][5];
			int foldindex[10];
			raycount = 0;
			float mean = 0.;
			for (int i = 0; i < 10; i++) {
				if ((raydata[i][j] != velNull) and (rayindex[i] != -999)) {
					mean += raydata[i][j];					
					for (int fold = -2; fold < 3; fold++) {
						folddata[raycount][fold+2]= raydata[i][j] + 2*fold*nyquists[i];
						foldindex[raycount] = rayindex[i];
					}
					raycount++;
				}
			}

			if (raycount > 2) {
				mean /= raycount;
				float var = 0;
				for (int i = 0; i < raycount; i++) {
					var += (folddata[i][2]-mean)*(folddata[i][2]-mean);
				}
				var /= (raycount-1);
				if (var < 16) continue;
				
				int fold[raycount];
				float minfold[raycount];
				float minvar = var;
				for (int i = 0; i < raycount; i++) fold[i] = -2;
				int n = 0;
				int basen = 0;
				int basefold = -2;
				while (basefold < 3) {
					
					mean = 0.;
					for (int i = 0; i < raycount; i++) {
						mean += folddata[i][fold[i]];
					}
					mean /= raycount;
					var = 0.;
					for (int i = 0; i < raycount; i++) {
						var += (folddata[i][fold[i]]-mean)*(folddata[i][fold[i]]-mean);
					}
					var /= (raycount-1);
					if (var < minvar) {
						minvar = var;
						for (int i = 0; i < raycount; i++) minfold[i] = folddata[i][fold[i]];
					}
					fold[n]++;
					if (fold[n] > 2) {
						fold[n] = -2;
						n++;
						if (n > raycount-1) {
							n = basen;
							basen++;
							if (basen > raycount-1) basen = 0;
							if (n == 0) basefold++;
							fold[n] = basefold;
							n++;
						}
						fold[n]++;
					}
				}
				if (minvar != 1e34){
					for (int i = 0; i < raycount; i++) {
						Ray* currentRay = radarData->getRay(foldindex[i]);
						float *vGates = currentRay->getVelData();
						vGates[j] = minfold[i];
					}
				}
			} else {
				continue;
			}
		}
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
            if(std::isnan(vbins[v])) {
                emit log(Message(QString("!!ISSUE!! ")
                                 +QString("Dealias Ray #")
                                 +QString().setNum(i)
                                 +QString(" Gate# ")
                                 +QString().setNum(v)
                                 +QString(" Vel = ")
                                 +QString().setNum(vbins[v])));
            }
        }
        vbins = NULL;
        currentRay = NULL;
        delete vbins;
        delete currentRay;
    }
    emit log(Message("Crazy Check Done"));
}

#if 0

// Not used

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
    check = NULL;
    checkVel = NULL;
    delete check;
    delete checkVel;
    
}

#endif

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

#if 0

// Debug functions.

void RadarQC::dumpRay(RadarData *radarPtr, int rayNum)
{
  Ray *ray = radarPtr->getRay(rayNum);
  if (ray == NULL)
    return;
  ray->dump();
  ray->dumpRef();
}

void RadarQC::debugDump(RadarData *radarPtr, int sweepNum)
{
  Sweep *sweep = radarPtr->getSweep(sweepNum);

  if(sweep == NULL) {
    std::cout << "No such sweep " << sweepNum << std::endl;
    return;
  }
  
  int first_ray = sweep->getFirstRay();
  int last_ray = sweep->getLastRay();

  std::cout << "Dumping sweep " << sweepNum
	    << " first ray: " << first_ray
	    << " last ray: " << last_ray
	    << ::std::endl;

  Ray *ray = radarPtr->getRay(first_ray);
  ray->dump();
  ray->dumpRef();

  ray = radarPtr->getRay(last_ray);
  ray->dump();
}

#endif

