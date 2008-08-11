/*
 *  VortexThread.cpp
 *  vortrac
 *
 *  Created by Michael Bell on 2/6/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include <QtGui>
#include <cmath>
#include "VortexThread.h"
#include "DataObjects/Coefficient.h"
#include "DataObjects/Center.h"
#include "VTD/GBVTD.h"
#include "Math/Matrix.h"
#include "HVVP/Hvvp.h"


VortexThread::VortexThread(QObject *parent)
  : QThread(parent)
{
    this->setObjectName("Vortex");

	  // Initialize RhoBar for pressure calculations (units are Pascal/m)
	  rhoBar[0] = 10.672;
	  rhoBar[1] = 9.703; 
	  rhoBar[2] = 8.792;
	  rhoBar[3] = 7.955;
	  rhoBar[4] = 7.183;
	  rhoBar[5] = 6.467;  
	  rhoBar[6] = 5.817; 
	  rhoBar[7] = 5.227; 
	  rhoBar[8] = 4.689;
	  rhoBar[9] = 4.207;
	  rhoBar[10] = 3.8;
	  rhoBar[11] = 3.3;
	  rhoBar[12] = 2.9;
	  rhoBar[13] = 2.6;
	  rhoBar[14] = 2.2;
	  rhoBar[15] = 1.8;
	  
	  abort = false;

	  // Claim and empty all the memory set aside
	  gridData = NULL;
	  vortexData = NULL;
	  pressureList = NULL;
	  configData = NULL;
	  dataGaps = NULL;
}

VortexThread::~VortexThread()
{
  mutex.lock();
  abort = true;
  waitForData.wakeOne();
  mutex.unlock();

  gridData = NULL;
  radarVolume = NULL;
  vortexData = NULL;
  pressureList = NULL;
  configData = NULL;
  
  delete gridData;
  delete vortexData;
  delete pressureList;
  delete configData;

  delete [] dataGaps;
 
  // Wait for the thread to finish running if it is still processing
  wait();

}

void VortexThread::getWinds(Configuration *wholeConfig, GriddedData *dataPtr, RadarData *radarPtr, VortexData* vortexPtr, PressureList *pressurePtr)
{

	// Lock the thread
	QMutexLocker locker(&mutex);

	// Set the pressure List
	pressureList = pressurePtr;
	
	// Set the grid object
	gridData = dataPtr;

	// Set the radar object
	radarVolume = radarPtr;

	// Set the vortex data object
	vortexData = vortexPtr;
	
	// Set the configuration info
	configData = wholeConfig;
      
	// Start or wake the thread
	if(!isRunning()) {
		start();
	} else {
		waitForData.wakeOne();
	}
}

void VortexThread::run()
{
  //emit log(Message("VortexThread Started"));
  
	forever {
		// Check to see if we should quit
		if (abort)
		  return;

		// OK, Let's find a center
		mutex.lock();
		bool foundWinds = true;
		emit log(Message(QString(),0,this->objectName(),Green));
		
		// Initialize variables
		readInConfig();
		
		// Set the output directory??
		//vortexResults.setNewWorkingDirectory(vortexPath);
	     
		 maxCoeffs = maxWave*2 + 3;

		// Find & Set Average RMW
		float rmw = 0;
		int goodrmw = 0;
		for(int l = 0; l < vortexData->getNumLevels(); l++) {
		  if((vortexData->getRMW(l)!=-999)&&(vortexData->getRMW(l)!=0)) {
		    //Message::toScreen("radius @ level "+QString().setNum(l)+" = "+QString().setNum(vortexData->getRMW(l)));
		    if(vortexData->getRMWUncertainty(l) < 10) {
		      rmw += vortexData->getRMW(l);
		      goodrmw++;
		    }
		  }
		}
		rmw = rmw/(1.0*goodrmw);
		vortexData->setAveRMW(rmw);
		// RMW is the average rmw taken over all levels of the vortexData
		
		// Create a GBVTD object to process the rings
		if(closure.contains(QString("hvvp"),Qt::CaseInsensitive)) {
		  if(calcHVVP(true)) {
		    vtd = new GBVTD(geometry, closure, maxWave, dataGaps, hvvpResult);
		    emit log(Message(QString(),6,this->objectName(),Green));
		  }
		  else {
		    emit log(Message(QString(),6,this->objectName(),Yellow,
				     QString("Could Not Retrieve HVVP Wind")));
		    vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
		  }
		}
		else {
		  vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
		  emit log(Message(QString(),6,this->objectName(),Green));
		}
		vtdCoeffs = new Coefficient[20];
	
		// Placeholders for centers
		float xCenter = -999;
		float yCenter = -999;
		
		mutex.unlock();
		// Loop through the levels and rings
		
		// We can't iterate through height like this and keep z spacing working -LM

		int loopPercent = int(7.0/float(gridData->getKdim()));
		int endPercent = 7-int(gridData->getKdim()*loopPercent);
		storageIndex = -1;


		//get some boundary values
		xdim = gridData->getIdim();
		ydim = gridData->getJdim();
		zdim = gridData->getKdim();

		for(int h = 0; h < gridData->getKdim(); h++) {
		  emit log(Message(QString(),loopPercent,this->objectName()));
		  if(abort)
		    return;
		  float height = gridData->getCartesianPointFromIndexK(h);
		  if((height<firstLevel)||(height>lastLevel)) { continue; }
		  storageIndex++;
		  mutex.lock();
		  // Set the reference point
		  float referenceLat = vortexData->getLat(h);
		  float referenceLon = vortexData->getLon(h);
		  gridData->setAbsoluteReferencePoint(referenceLat, referenceLon, height);
		  if ((gridData->getRefPointI() < 0) || 
		      (gridData->getRefPointJ() < 0) ||
		      (gridData->getRefPointK() < 0)) {
		    // Out of bounds problem
		    emit log(Message(QString("Simplex center is outside CAPPI"),0,this->objectName()));
		    mutex.unlock();
		    continue;
		  }			
		  
		  if(!abort) {
		    // should we be incrementing radius using ringwidth? -LM
		    for (float radius = firstRing; radius <= lastRing; radius++) {
		      
		      // Get the cartesian points
		      xCenter = gridData->getCartesianRefPointI();
		      yCenter = gridData->getCartesianRefPointJ();
		      
		      // Get the data
		      int numData = gridData->getCylindricalAzimuthLength(radius, height);
		      float* ringData = new float[numData];
		      float* ringAzimuths = new float[numData];
		      gridData->getCylindricalAzimuthData(velField, numData, radius, height, ringData);
		      gridData->getCylindricalAzimuthPosition(numData, radius, height, ringAzimuths);
		      
		      // Call gbvtd
			if (vtd->analyzeRing(xCenter, yCenter, radius, height, numData, ringData,
					     ringAzimuths, vtdCoeffs, vtdStdDev)) {
			  if (vtdCoeffs[0].getParameter() == "VTC0") {
			    // VT[v] = vtdCoeffs[0].getValue();
			  } else {
			    emit log(Message(QString("Error retrieving VTC0 in vortex!"),0,this->objectName()));
			  } 
			} else {
			  QString err("Insufficient data for VTD winds: radius ");
			  QString loc;
			  err.append(loc.setNum(radius));
			  err.append(", height ");
			  err.append(loc.setNum(height));
			  emit log(Message(err));
			}
		      

		      delete[] ringData;
		      delete[] ringAzimuths;
				
		      // All done with this radius and height, archive it
		      archiveWinds(radius, storageIndex, maxCoeffs);
		    }
		  }
		  mutex.unlock();
		

		  
		}
		storageIndex = -1;

		//printing meanwind header in HVVPoutput.txt log		
		QFile HVVPLogFile;
		QDir workingDirectoryPath(configData->getParam(configData->getConfig("vortex"),"dir"));
		HVVPLogFile.setFileName(workingDirectoryPath.filePath("HVVP_output.txt"));
		QString Header;
		Header += "Average GVTD RESULTS\n";
		Header += "Z (km)     VMx (m/s)     VMy (m/s)     Mag.Vm (m/s)     ThetaM (rad)     VmSin (m/s)    VmCos (m/s)     VmCosCheck\n";
		if(HVVPLogFile.isOpen()) {
		  HVVPLogFile.close();
		}
		if(HVVPLogFile.open(QIODevice::Append)) {
		  HVVPLogFile.write(Header.toAscii());
		  HVVPLogFile.close();
		}

		
		for (int h = 0; h < gridData->getKdim(); h++) {
		  height = gridData->getCartesianPointFromIndexK(h);
		  if((height<firstLevel)||(height>lastLevel)) { continue; }
		  storageIndex++;

		  if (closure.contains(QString("MeanWind"),Qt::CaseInsensitive)) {


		    

		    //now find mean wind using GVTD
		    //by finding gradient of Vd*D
	    



		    ///////////////////////////////////////////////////////////////////////////////////////////////////////////

		    //initial guess for simplex search routine (finds minimum standard deviation of gradient Vd*D)
		    calcHVVP(false);
		    float radiusTemp = 10;
		    float heightTemp = 1;
		    int numData = gridData->getCylindricalAzimuthLength(radiusTemp, heightTemp);
		    float* ringData = new float[numData];
		    float* ringAzimuths = new float[numData];
		    gridData->getCylindricalAzimuthData(velField, numData, radiusTemp, heightTemp, ringData);
		    gridData->getCylindricalAzimuthPosition(numData, radiusTemp, heightTemp, ringAzimuths);
		    //float* vars = new float[2]; 
		    float vmcos = -999;
		    float b1b3 = -999;
		    vtd->analyzeMeanWind(xCenter, yCenter, radiusTemp, heightTemp, numData, ringData,
						 ringAzimuths, vtdCoeffs, vtdStdDev, &vmcos, &b1b3);
		    //float vmcos = vars[1];
		    //hvvp->findHVVPWinds(true);
		    float thetaT = atan2(yCenter,xCenter);
		    thetaT = vtd->fixAngle(thetaT);
		    float HVVPvm = sqrt(hvvpResult*hvvpResult + vmcos*vmcos);
		    ThetaM = thetaT - atan2(hvvpResult,vmcos);
		    float vmxguess = HVVPvm*cos(ThetaM);
		    float vmyguess = HVVPvm*sin(ThetaM);
		    
		    Message::toScreen("HVVP initial guess for GVTD-simplex is "+QString().setNum(vmxguess)+"-"+QString().setNum(vmyguess));

		    MeanWindField = new float*[xdim];
		    for (int x =0; x < xdim; x++) {
		      MeanWindField[x] = new float[ydim];
		    }

		    //plot the simplex phase space
		    /*
		    Message::toScreen("***Generating Phase Space***");
		    float** PhaseSpace = new float*[31];
		    for (int i = 0;i<=30;i++) {
		      PhaseSpace[i] = new float[31];
		    }

		    for (int x = 15;x <=25;x++) {
		      for (int y = 10;y <=20;y++) {
			IT = 998;
			VMx = x-15;
			VMy = y-15;
			PhaseSpace[x][y] = IterateMeanWind(0);
		      }
		    }
		    int q = 0;
		    int w = 11;
		    int e = 9;
		    writeANAsi(PhaseSpace,IT,q, w, e);
		    Message::toScreen("***Phase Space Complete***");
		    */

		    IT = 0;
		    //call simplex to find Vm
		    DownSimplex(vmxguess, vmyguess, h);

		    //deleting arrays
		    for (int i=0; i < xdim; i++) {
		      delete [] MeanWindField[i];
		    }
		    delete [] MeanWindField;

/*
		      QString fieldName = "VE";	
  		      float** CappiWindField = new float*[xdim];
		      bool** goodData = new bool*[xdim];
		      float** MeanWindField = new float*[xdim];
		      float** vmx = new float*[xdim];
		      float** vmy = new float*[xdim];
		      for (int x =0; x < xdim; x++) {
			CappiWindField[x] = new float[ydim];
			MeanWindField[x] = new float[ydim];
			vmx[x] = new float[ydim];
			vmy[x] = new float[ydim];
			goodData[x] = new bool[ydim];
		      }	      

		      // Get the cartesian center
		      xCenter = gridData->getCartesianRefPointI();
		      yCenter = gridData->getCartesianRefPointJ();
		      float centerDistance = sqrt(xCenter*xCenter + yCenter*yCenter);
		      		      
		      // Get thetaT
		      float thetaT = atan2(yCenter,xCenter);
		      thetaT = vtd->fixAngle(thetaT);
		      
		      //retrieve the cappi wind field
		      float xref;
		      for (int i = 1; i < xdim; i++) {
			xref = gridData->getCartesianPointFromIndexI(i);
			CappiWindField[i] = gridData->getCartesianYslice(fieldName,xref,h+1);
		      }
		      float MeanWindDiff = 100;
		      //   bool FirstIteration = true;
		      float LastMeanWind = 0;
		      float NewMeanWind = -999;
		      QString vtc0 = "VTCO";
		      QString vrc0 = "VRCO";
		      float VMCosCheck;
		      float VMx;
		      float VMy;
		      float IT = 0;
		      float stdold = 999999999; //very large!
		      float lastthetaM;

		      if (h == 0) {
			writeANAsi(CappiWindField,IT,zdim, xdim, ydim);
		      }

		      while (MeanWindDiff > 10) {
			IT += 1;
			

			if (IT == 1) {
			  //QDomElement meanwind = configData->getConfig("meanwind");
			  //float VMGuess = configData->getParam(meanwind,"VMGuess").toFloat();
			  //float thetaMGuess = configData->getParam(meanwind,"ThetaMGuess").toFloat();
			  float VMGuess = 4.60;
			  float thetaMGuess = 0;
			  VMx = VMGuess*cos(thetaMGuess);
			  VMy = VMGuess*sin(thetaMGuess);
			  lastthetaM = atan2(VMy,VMx);
			  NewMeanWind = VMGuess;
			  LastMeanWind = VMGuess-.05;
			}
			else{
			  //if (thetaConverged) {
			  // VMx = Xgradient;
			  // VMy = Ygradient;
			    //}
			    //else{
			    //;
			    //}
			}
			

			//Pondering Convergence....
			//NewMeanWind = sqrt( VMx*VMx + VMy*VMy);
			//float thetaM = atan2(VMy,VMx);
			float thetaM = 0;
			//check to make sure we're not diverging
			float ITdifference = LastMeanWind-NewMeanWind;
			/*if (stdnew > stdold) {
			  //diverging, lets reverse the direction of dVm
			  if (ITdifference > 0) {NewMeanWind += .01; LastMeanWind -= .01;}
			  if (ITdifference < 0) {NewMeanWind -= .01; LastMeanWind += .01;}
			  if (ITdifference == 0) {NewMeanWind = LastMeanWind;}
			  thetaM = lastthetaM;
			}
			else {
			  //converging, lets keep incrementing
			  if (ITdifference > 0) {NewMeanWind -= .01; LastMeanWind -= .01;}
			  if (ITdifference < 0) {NewMeanWind += .01; LastMeanWind += .01;}
			  if (ITdifference == 0) {NewMeanWind = LastMeanWind;}
			  thetaM = lastthetaM;
			  }

		    	NewMeanWind += .05;
			LastMeanWind += .05;
			//thetaM = lastthetaM;
			thetaM = 0;

			//MeanWindDiff = abs((stdnew-stdold));
			MeanWindDiff = 1000;
			

		        
			//if (MeanWindDiff <= 500) {break;}


			//find the mean wind field
			float xpos;float ypos;float D;float xp;float yp;float theta;float R;float Vt;float Vr;float VTC0;float VRC0;float VTC1; float VTS1;
			float radius;
		
			for (int i = 1; i < xdim; i++) {
			  for (int j = 1; j < ydim; j++) {
			    goodData[i][j] = false; //for now...
			    xpos = gridData->getCartesianPointFromIndexI(i);
			    ypos = gridData->getCartesianPointFromIndexJ(j);
			    D = sqrt( xpos*xpos + ypos*ypos );
			    xp = xpos - xCenter;
			    yp = ypos - yCenter;
			    theta = atan2(yp,xp);
			    theta = vtd->fixAngle(theta);
			    R = sqrt( xp*xp + yp*yp );

			    if ( R-(int)R < .5 ) {
			    radius = (int)R;
			    } else {
			      radius = (int)R + 1;
			    }
			  

			    if (height == 1) {
			      // Message::toScreen(QString().setNum(IT) + " "+QString().setNum(radius) + " "+QString().setNum(Vt) + " "+ QString().setNum(Vr) );
			    }


			    if ((radius < lastRing) && (radius > 0)) {
			      // these values depend on NumWaveNum, set in AnalysisThread.cpp. 
			      //        i.e. if NumWaveNum = 2, VTC1 and VTS1 return -999
			      VTC0 = vortexData->getCoefficient(height, radius, QString("VTC0")).getValue();
			      VTC1 = vortexData->getCoefficient(height, radius, QString("VTC1")).getValue();
			      VTS1 = vortexData->getCoefficient(height, radius, QString("VTS1")).getValue();
			      VRC0 = vortexData->getCoefficient(height, radius, QString("VRC0")).getValue();
			      if (VTC0 == -999) { VTC0 = 0;}
			      if (VTC1 == -999) { VTC1 = 0;}
			      if (VTS1 == -999) { VTS1 = 0;}
			      if (VRC0 == -999) { VRC0 = 0;}
			      //			      Vt = VTC0 + VTC1*cos(theta - thetaT) + VTS1*sin(theta - thetaT);
			      Vt = VTC0;
			      Vr = VRC0;

			      if ( (Vt != -999) ) {
				goodData[i][j] = true;
			      }
			    }
			    else { Vt = 0; Vr = 0;}

			    //   float meanVTx1 = vortexData->getCoefficient((int)h, (int)radius, QString("VTC0")).getValue();
			    // float meanVTx2 = vortexData->getCoefficient((int)h, (int)radius+1, QString("VTC0")).getValue();
		      
			      //			    }
	// 		    else{
// 			      Vt = NewVt;
// 			      Vr = NewVr;
// 			    }
			    if (radius < 10){
			    float thetaLAME = theta;
			    }
			    MeanWindField[i][j] = CappiWindField[i][j]*D + centerDistance*Vt*sin(theta - thetaT) - centerDistance*Vr*(R/centerDistance + cos(theta - thetaT));
 			    if ( height == 1 ) {
			      // Message::toScreen("VT or VR NOT ZERO>>> Iteration=" + QString().setNum(IT) + ", level=" + QString().setNum(h) + ", Radius=" + QString().setNum(radius) + ", i="+ QString().setNum(i) + ", j="+ QString().setNum(j) + ", Vt="+ QString().setNum(Vt) + ", Vr="+ QString().setNum(Vr) );
			      //  Message::toScreen(QString().setNum(Vt) + " "+ QString().setNum(Vr) ); 
 			    }
			    //MeanWindField[i][j] = (  CappiWindField[i][j] + Vt*sin(theta - thetaT)*centerDistance/D 
			    //			     - Vr/D*centerDistance*(R/centerDistance + cos(theta - thetaT))   ) * D;
			  }
			}

			
			//Perform the gradient:
			float dx = 2;
			float dy = 2;
			int numGradValues = 0;
			float gradYsum = 0;
			for (int x = 1; x < xdim; x++) {
			  for (int y = 2; y < ydim-1; y++) {
			    if (goodData[x][y] && goodData[x][y+1]) {
			      vmy[x][y] = (MeanWindField[x][y+1] - MeanWindField[x][y-1])/dy;
			      gradYsum += vmy[x][y];
			      numGradValues += 1;
			    }
			  }
			}
			float Ygradient = gradYsum/numGradValues;
			numGradValues = 0;
			float gradXsum = 0;
			for (int y = 1; y < ydim; y++) {
			  for (int x = 2; x < xdim-1; x++) {
			    if (goodData[x][y]  && goodData[x+1][y]) {
			      vmx[x][y] = (MeanWindField[x+1][y] - MeanWindField[x-1][y])/dx;
			      gradXsum += vmx[x][y];
			      numGradValues += 1;
			    }
			  }
			}
			float Xgradient = gradXsum/numGradValues;

	


			//find standard dev., then remove outliers
			//float Vmbar = sqrt( Xgradient*Xgradient + Ygradient*Ygradient );
			float Vmbar = NewMeanWind;
			//float Thetambar = atan2(Ygradient,Xgradient);
			float Thetambar = 0;
			float stdnew = 0;
			float stdbad = 0;
			float stdgood = 0;
			float numthrownout = 0;
			float n = 0;
			for (int y = 1; y < ydim; y++) {
			  for (int x = 1; x < xdim-1; x++) {
			    if (goodData[x][y]) {
			      float Vm = sqrt( vmx[x][y]*vmx[x][y] + vmy[x][y]*vmy[x][y] );
			      float Thetam = atan2(vmy[x][y],vmx[x][y]);
			      stdnew +=  (Vm - Vmbar)*(Vm - Vmbar) + (Thetam - Thetambar)*(Thetam - Thetambar);
			      n++;
			    }
			  }
			}
			if (n != 0) {
			  stdbad = sqrt(stdnew/n);
			}

			for (int y = 1; y < ydim; y++) {
			  for (int x = 1; x < xdim-1; x++) {
			    float Vm = sqrt( vmx[x][y]*vmx[x][y] + vmy[x][y]*vmy[x][y] );
			    float Thetam = atan2(vmy[x][y],vmx[x][y]);
			    float variance = sqrt( (Vm - Vmbar)*(Vm - Vmbar) + (Thetam - Thetambar)*(Thetam - Thetambar) );
			    if (variance > stdbad) {
			      //throw out values greater than one standard Dev.
			      goodData[x][y] = 0;
			      numthrownout++;
			    }
			  }
			}
		      
			//find new, improved standard deviation
			stdnew = 0;
			n = 0;
			for (int y = 1; y < ydim; y++) {
			  for (int x = 1; x < xdim-1; x++) {
			    if (goodData[x][y]) {
			      float Vm = sqrt( vmx[x][y]*vmx[x][y] + vmy[x][y]*vmy[x][y] );
			      float Thetam = atan2(vmy[x][y],vmx[x][y]);
			      stdnew +=  (Vm - Vmbar)*(Vm - Vmbar) + (Thetam - Thetambar)*(Thetam - Thetambar);
			      n++;
			    }
			  }
			}
			if (n != 0) {
			  stdgood = sqrt(stdnew/n);
			}

			//Perform the gradient again, this time with outliers removed:
		        numGradValues = 0;
			gradYsum = 0;
			for (int x = 1; x < xdim; x++) {
			  for (int y = 2; y < ydim-1; y++) {
			    if (goodData[x][y]) {
			      gradYsum += vmy[x][y];
			      numGradValues += 1;
			    }
			  }
			}
			Ygradient = gradYsum/numGradValues;
			numGradValues = 0;
			gradXsum = 0;
			for (int y = 1; y < ydim; y++) {
			  for (int x = 2; x < xdim-1; x++) {
			    if (goodData[x][y]  && goodData[x+1][y]) {
			      gradXsum += vmx[x][y];
			      numGradValues += 1;
			    }
			  }
			}
			Xgradient = gradXsum/numGradValues;



			if (( (NewMeanWind < 5.02) && (NewMeanWind > 4.98) ) || ((NewMeanWind < 5.47) && (NewMeanWind > 5.43)) ) {
			  Message::toScreen( "MeanWind: " + QString().setNum(NewMeanWind) + "\n" );
			  for (int y = 1; y < ydim; y++) {
			    for (int x = 1; x < xdim-1; x++) {
			      if (vmx[x][y] >= 100 or vmy[x][y] >= 50) {
				xpos = gridData->getCartesianPointFromIndexI(x);
				ypos = gridData->getCartesianPointFromIndexJ(y);
				//				D = sqrt( xpos*xpos + ypos*ypos );
				xp = xpos - xCenter;
				yp = ypos - yCenter;
// 				theta = atan2(yp,xp);
// 				theta = vtd->fixAngle(theta);
				R = sqrt( xp*xp + yp*yp );
				
				if ( R-(int)R < .5 ) {
				  radius = (int)R;
				} else {
				  radius = (int)R + 1;
				}
				Message::toScreen(QString().setNum(x) + " "+QString().setNum(y)+ " "+QString().setNum(radius) + " "+ QString().setNum(vmx[x][y]) + " "+ QString().setNum(vmy[x][y]) );
			      }
			    }
			  }
			  Message::toScreen( "\n----------------------------------------------------------------\n" );
			  }


			stdold = stdnew;

			//send Vm to GVTD
			VMx = NewMeanWind*cos(thetaM);
			VMy = NewMeanWind*sin(thetaM);
			vtd->setGVTD(VMx,VMy);

			Message::toScreen(QString().setNum(NewMeanWind) + " " + QString().setNum(stdbad)+ " "+ QString().setNum(stdgood)+ " "+ QString().setNum(numthrownout) );

			//now call meanwind GVTD
			for (float radius = firstRing; radius <= lastRing; radius++) {
			  
			  // Get the data
			  int numData = gridData->getCylindricalAzimuthLength(radius, height);
			  float* ringData = new float[numData];
			  float* ringAzimuths = new float[numData];
			  gridData->getCylindricalAzimuthData(velField, numData, radius, height, ringData);
			  gridData->getCylindricalAzimuthPosition(numData, radius, height, ringAzimuths);
			  
			  // Call gvtd		  
			  VMCosCheck = vtd->analyzeMeanWind(xCenter, yCenter, radius, height, numData, ringData,
							     ringAzimuths, vtdCoeffs, vtdStdDev);  
			  //update vortexData with new values
			  archiveWinds(radius, storageIndex, maxCoeffs);
		// 	  NewVt = NewWinds[0];
// 			  NewVr = NewWinds[1];

			  delete[] ringData;
			  delete[] ringAzimuths;

			
			}
			//	FirstIteration = false;
			//repeat while loop
			//break;
		  } //ends 'while' loop

*/
		      
		      //writing ANALYTIC CAPPI ASI for first level only

		      // All done with this radius and height, archive it
		      //archiveWinds(radius, storageIndex, maxCoeffs);


		      

		      //printing meanwind to log
		      float Vm = sqrt( (VMx)*(VMx) + (VMy)*(VMy) );
		      float thetaM = atan2(VMy,VMx);
		      Vmsin = Vm * sin(thetaT - thetaM);
		      Vmcos = Vm * cos(thetaT - thetaM);
		      //QFile HVVPLogFile;
		      //HVVPLogFile.setFileName(workingDirectoryPath.filePath("HVVP_output.txt"));
		      QString shortMessage;
		      shortMessage +=QString().setNum(height)+"\t   "+QString().setNum(VMx);
		      shortMessage +="\t   "+QString().setNum(VMy)+"\t   "+QString().setNum(Vm);
		      shortMessage +="\t   "+QString().setNum(thetaM)+"\t   "+QString().setNum(Vmsin);
		      shortMessage += "\t   "+QString().setNum(Vmcos)+ "\t   "+QString().setNum(VMCosCheck)+ "\n";
		      //shortMessage += "\t   "+QString().setNum(xr[i])+"\n";
		      
		      if(HVVPLogFile.isOpen()) {
			HVVPLogFile.close();
		      }
		      if(HVVPLogFile.open(QIODevice::Append)) {
			HVVPLogFile.write(shortMessage.toAscii());
			HVVPLogFile.close();
		      }
		      
		      
		  }


		}

	       	
		emit log(Message(QString(),endPercent,this->objectName()));

		mutex.lock();
		
		// Clean up
		delete [] vtdCoeffs;
		delete vtd;
		
		// Integrate the winds to get the pressure deficit at the 2nd level (presumably 2km)
		float* pressureDeficit = new float[(int)lastRing+1];
		float kdim = gridData->getKdim();
		getPressureDeficit(vortexData,pressureDeficit, gradientHeight, kdim);
		
		mutex.unlock();
		if(abort)
		  return;
		mutex.lock();
	     		
		// Get the central pressure
		float centralPressure = 0;
		centralPressure = calcCentralPressure(vortexData,pressureDeficit, centralPressure, gradientHeight); // is firstLevel right?
		emit log(Message(QString(),3,this->objectName())); // 78 %

		mutex.unlock();
		if(abort)
		  return;
		mutex.lock();

//                                ****APPROXIMATED Temp Field Solver***
		Message::toScreen("\n ***Beginning Approximated Temp Field Solver*** \n");
		// Integrate the winds to get the density deficit at all levels
		//	float kdim = gridData->getKdim();
       		float** rhoDeficit = new float*[(int)lastRing+1];
		for (int i=0; i <= (int)lastRing; i++) {
      		  rhoDeficit[i] = new float[(int)kdim+1];
		}
		getRhoDeficit(vortexData,rhoDeficit,kdim);
		mutex.unlock();
		if(abort)
		  return;
		mutex.lock();
		//declaring arrays
		float** Tdeficit1 = new float*[(int)lastRing+1];
		float** P1 = new float*[(int)lastRing+1];
		float** Rho1 = new float*[(int)lastRing+1];
		float** T1 = new float*[(int)lastRing+1];
		float** theta1 = new float*[(int)lastRing+1];
		float** thetadiff1 = new float*[(int)lastRing+1];
		for (int i=0; i <= (int)lastRing; i++) {
		  Tdeficit1[i] = new float[(int)kdim+1];
		  P1[i] = new float[(int)kdim+1];
		  Rho1[i] = new float[(int)kdim+1];
		  T1[i] = new float[(int)kdim+1];
		  theta1[i] = new float[(int)kdim+1];
		  thetadiff1[i] = new float[(int)kdim+1];
		}
		float*** propGrid1 = new float**[8];
		for (int n=0 ; n<=7 ; n++) {
		  propGrid1[n]= new float*[(int)lastRing+1];
		  for (int r=0 ; r<=(int)lastRing ; r++) {
		    propGrid1[n][r]= new float[(int)kdim+1];
		  }
		}
		//Calculate the temp. deficit using ideal gas law, pressureDeficit, and rho deficit	
		getTDeficit(vortexData,T1,P1,Rho1,theta1,thetadiff1,Tdeficit1,rhoDeficit,kdim,centralPressure,propGrid1);
//                                ****Done With APPROXIMATED Temp Field Solver***

		Message::toScreen("\n ***Approximated Solver Done, Declaring PropGrid*** \n");
		//Declare the property grid to be written into WriteAsi()
		float*** propGrid = new float**[17];
		for (int n=0 ; n<=16 ; n++) {
		  propGrid[n]= new float*[(int)lastRing+1];
		  for (int r=0 ; r<=(int)lastRing ; r++) {
		    propGrid[n][r]= new float[(int)kdim+1];
		  }
 		}

		//combining approx and unapprox propgrids
		for (int h=0 ; h<=kdim ; h++) {
		  for (int r=0 ; r<=(int)lastRing ; r++) {
		    for (int n=0 ; n<=7 ; n++) {
		      propGrid[n+9][r][h] = propGrid1[n][r][h];
		    }
		  }
		}

//                                ****UNAPPROXIMATED Temp Field Solver***		
		Message::toScreen("\n ***Running UNAPPROXIMATED Temp Field Solver*** \n");
		//Solve Temp, Rho, Press deficit fields using unaproximated thermal wind equation.
		getDeficitFields(vortexData,kdim,propGrid,centralPressure);

		// Write ASCII file for solved fields
		writeAsi(propGrid,kdim);

		//Delete that property grid
		for (int n=0; n <= 16; n++) {
		  for (int i=0; i <= (int)lastRing; i++) {
		  delete [] propGrid[n][i];
		  }
		  delete [] propGrid[n];
		}
		delete [] propGrid;

		//deleting arrays
		for (int i=0; i <= (int)lastRing; i++) {
		  delete [] Tdeficit1[i];
		  delete [] P1[i];
		  delete [] Rho1[i];
		  delete [] T1[i];
		  delete [] theta1[i];
		  delete [] thetadiff1[i];
		}
		delete [] Tdeficit1;
		delete [] P1;
		delete [] Rho1;
		delete [] T1;
		delete [] theta1;
		delete [] thetadiff1;


		// Get the central pressure uncertainty 
		/*
		 * For Gathering Stats
		 * calcPressureUncertainty(0, QString("CenterStdPresErr"));
		 * calcPressureUncertainty(1, QString("FlatPresErr"));
		 * calcPressureUncertainty(1.5, QString("FlatPresErr"));
		 */
		// 
		calcPressureUncertainty(1,QString());  
		//This is the one we are using?
		
		// 95 %
		
		delete [] pressureDeficit;
		float *temp = dataGaps;
		dataGaps = NULL;
		delete [] temp;
		
		if(!foundWinds)
		{
			// Some error occurred, notify the user
			emit log(Message(QString("Vortex Error!"),0,this->objectName(),Yellow));
			return;
		} else {
			// Update the vortex list
			// vortexData ???
		
			// Update the progress bar and log
			emit log(Message(QString("Done with Vortex"),0,this->objectName(),Green));

			// Let the AnalysisThread know we're done
			emit(windsFound());
		}
		mutex.unlock();
		
		// Go to sleep, wait for more data
		mutex.lock();
		if (!abort)
		{
			// Wait until new data is available
			waitForData.wait(&mutex);
			mutex.unlock();		
		}
		if(abort)
		  return;

		emit log(Message(QString("End of Vortex Run"),0,
				 this->objectName()));
	}
}

void VortexThread::archiveWinds(float& radius, int& hIndex, float& maxCoeffs)
{

  // Save the centers to the VortexData object

  int level = hIndex;
  int ring = int(radius - firstRing);
	
  for (int coeff = 0; coeff < (int)(maxCoeffs); coeff++) {
    vortexData->setCoefficient(level, ring, coeff, vtdCoeffs[coeff]);
  }
	
}

void VortexThread::archiveWinds(VortexData& data, float& radius, int& hIndex, float& maxCoeffs)
{

  // Save the centers to the VortexData object

  int level = hIndex;
  int ring = int(radius - firstRing);
	
  for (int coeff = 0; coeff < (int)(maxCoeffs); coeff++) {
    data.setCoefficient(level, ring, coeff, vtdCoeffs[coeff]);
  }
	
}

//overloaded function, this one used by temp field solvers and is slightly different
void VortexThread::getPressureDeficit(VortexData* data, float* pDeficit,const float& height, float& kdim)
{

	float* dpdr = new float[(int)lastRing+1];
	float* FrhoBar = new float[(int)kdim+1];

	//reference density function
	for (float Fheight = 1; Fheight <= kdim+1; Fheight++) {
	  FrhoBar[(int)Fheight-1]=1.244*exp(-Fheight*1000/8864)*10;
	}

	// get the index of the height we want
	int heightIndex = data->getHeightIndex(height);
	//Message::toScreen("GetPressureDeficit for height "+QString().setNum(height)+" km which corresponds to index "+QString().setNum(heightIndex)+" in data; this corresponds to level "+QString().setNum(gridData->getIndexFromCartesianPointK(height)));

	// Assuming radius is in KM here, when we correct for units later change this
	float deltar = 1000;
	//float H=8; //km
	
	// Initialize arrays
	for (float radius = 0; radius <= lastRing; radius++) {
		dpdr[(int)radius] = -999;
		pDeficit[(int)radius] = 0;
	}
	
	// Get coriolis parameter
	float f = 2 * 7.29e-5 * sin(data->getLat(heightIndex)*3.141592653589793238462643/180.);
	
	for (float radius = firstRing; radius <= lastRing; radius++) {
		if (!(data->getCoefficient(height, radius, QString("VTC0")) == Coefficient())) {

		  float meanVT = data->getCoefficient(heightIndex, (int)radius-1, QString("VTC0")).getValue();
	
			 

			if (meanVT != 0) {
				dpdr[(int)radius] = ((f * meanVT) + (meanVT * meanVT)/(radius * deltar)) * FrhoBar[(int)height-1];
			}
		}
	}
	if (dpdr[(int)lastRing] != -999) {
		pDeficit[(int)lastRing] = -dpdr[(int)lastRing] * deltar * 0.001;
	}
	
	for (float radius = lastRing-1; radius >= 0; radius--) {
		if (radius >= firstRing) {
			if ((dpdr[(int)radius] != -999) and (dpdr[(int)radius+1] != -999)) {
				pDeficit[(int)radius] = pDeficit[(int)radius+1] - 
					(dpdr[(int)radius] + dpdr[(int)radius+1]) * deltar * 0.001/2;
			} else if (dpdr[(int)radius] != -999) {
				pDeficit[(int)radius] = pDeficit[(int)radius+1] - 
					dpdr[(int)radius] * deltar * 0.001;
			} else if (dpdr[(int)radius+1] != -999) {
				pDeficit[(int)radius] = pDeficit[(int)radius+1] - 
					dpdr[(int)radius+1] * deltar * 0.001;
			} else {
				// Flat extrapolation for data gaps > 2 radii
				pDeficit[(int)radius] = pDeficit[(int)radius+1];
			}
		} else {
			pDeficit[(int)radius] = pDeficit[(int)firstRing];
		}
	}
	delete [] dpdr;
	
}

//used for original vortrac stuff
void VortexThread::getPressureDeficit(VortexData* data, float* pDeficit,const float& height)
{
 
	float* dpdr = new float[(int)lastRing+1];


	// get the index of the height we want
	int heightIndex = data->getHeightIndex(height);

	// Assuming radius is in KM here, when we correct for units later change this
	float deltar = 1000;
	//	float H=8; //km
	
	// Initialize arrays
	for (float radius = 0; radius <= lastRing; radius++) {
		dpdr[(int)radius] = -999;
		pDeficit[(int)radius] = 0;
	}
	
	// Get coriolis parameter
	float f = 2 * 7.29e-5 * sin(data->getLat(heightIndex)*3.141592653589793238462643/180.);
	
	for (float radius = firstRing; radius <= lastRing; radius++) {
		if (!(data->getCoefficient(height, radius, QString("VTC0")) == Coefficient())) {

		  float meanVT = data->getCoefficient(height, radius, QString("VTC0")).getValue();

			if (meanVT != 0) {
				dpdr[(int)radius] = ((f * meanVT) + (meanVT * meanVT)/(radius * deltar)) * rhoBar[(int)height-1];
			}
		}
	}
	if (dpdr[(int)lastRing] != -999) {
		pDeficit[(int)lastRing] = -dpdr[(int)lastRing] * deltar * 0.001;
	}
	
	for (float radius = lastRing-1; radius >= 0; radius--) {
		if (radius >= firstRing) {
			if ((dpdr[(int)radius] != -999) and (dpdr[(int)radius+1] != -999)) {
				pDeficit[(int)radius] = pDeficit[(int)radius+1] - 
					(dpdr[(int)radius] + dpdr[(int)radius+1]) * deltar * 0.001/2;
			} else if (dpdr[(int)radius] != -999) {
				pDeficit[(int)radius] = pDeficit[(int)radius+1] - 
					dpdr[(int)radius] * deltar * 0.001;
			} else if (dpdr[(int)radius+1] != -999) {
				pDeficit[(int)radius] = pDeficit[(int)radius+1] - 
					dpdr[(int)radius+1] * deltar * 0.001;
			} else {
				// Flat extrapolation for data gaps > 2 radii
				pDeficit[(int)radius] = pDeficit[(int)radius+1];
			}
		} else {
			pDeficit[(int)radius] = pDeficit[(int)firstRing];
		}
	}
	delete [] dpdr;
	
}

void VortexThread::getDeficitFields(VortexData* data, float& kdim, float*** propGrid,float centralPressure)
{

  int deltar = 1000; //meters
  float deltaz = gridData->getKGridsp()*1000; //km to meters
  float g = 9.81;
  
  //Initialize WriteAsi Property Grid 
  for(float height = 0; height <= kdim; height++) {
    for (float radius=lastRing ; radius >= 0 ; radius--) {
      for (float n=0 ; n<=8 ; n++) {
	propGrid[(int)n][(int)radius][(int)height]=-999;
      }      
    }
  }

  //Declare arrays
  float** Td = new float*[(int)lastRing+1];
  float** pd = new float*[(int)lastRing+1];
  float** rhod = new float*[(int)lastRing+1];
  float** p = new float*[(int)lastRing+1];
  float** rho = new float*[(int)lastRing+1];
  float** T = new float*[(int)lastRing+1];
  float** meanVT1 = new float*[(int)lastRing+1];
  float** fref = new float*[(int)lastRing+1];
  for (int i=0; i <= (int)lastRing; i++) {
    Td[i] = new float[(int)kdim+1];
    rhod[i] = new float[(int)kdim+1];
    pd[i] = new float[(int)kdim+1];
    p[i] = new float[(int)kdim+1];
    rho[i] = new float[(int)kdim+1];
    T[i] = new float[(int)kdim+1];
    meanVT1[i] = new float[(int)kdim+1];
    fref[i] = new float[(int)kdim+1];
  }
  float* pbar = new float[(int)kdim+1];
  float* dpbar = new float[(int)kdim+1];
  float* pressD = new float[(int)lastRing+1];
  float* Tbar = new float[(int)kdim+1];
  float* rhobar = new float[(int)kdim+2];
  float* thetabar = new float[(int)kdim+1];


  // Initialize arrays    
  for (float height = 0; height <= kdim; height++) {
    for (float radius = 0; radius <= lastRing; radius++) {
      rhod[(int)radius][(int)height] = -999;
      Td[(int)radius][(int)height] = -999;
      pd[(int)radius][(int)height] = -999;
      T[(int)radius][(int)height] = -999;
      rho[(int)radius][(int)height] = -999;
      p[(int)radius][(int)height] = -999;
      meanVT1[(int)radius][(int)height] = -999;
      fref[(int)radius][(int)height] = -999;
    }
  }
  

  //initialize boundary values

  //find RhoBar
  for (float h = 0; h <= kdim+1 ; h++) {
    rhobar[(int)h]=1.244*exp(-h*1000/8864);
  }

  //find pbar of height(aka altitude in km...not index)
  getPressureDeficit(vortexData,pressD, 1,kdim);
  pbar[0] = (centralPressure-pressD[0])*100; // *100 to convert to hPa
  for(float height = 1; height <= kdim; height++) {
      dpbar[(int)height]=(rhobar[(int)height-1]*g+rhobar[(int)height]*g)*-deltaz/(2); 
      pbar[(int)height]=( pbar[(int)height-1]+dpbar[(int)height] );// in Pascals
  }

  //find Tbar and ThetaBar
  float kap = .28557213930348;//287/1005
  float p0 = 100000;//pa
  for (float h = 0; h <= kdim ; h++) {
    Tbar[(int)h] = pbar[(int)h]/(rhobar[(int)h]*287);
    thetabar[(int)h] = Tbar[(int)h]*pow((p0/pbar[(int)h]),kap);
  }  

  float zsum;
  float zline;
  float dlnrhosum;

  //height in km
  for(float height = 1; height <= kdim; height++) {

    //integrate out radially to get dz and dlnrho for each gridpoint
    // radius in km
	for (float radius = firstRing; radius <= lastRing; radius++) {

	  zsum = 0;
	  zline = height;
	  dlnrhosum = 0;
	  float f = 2 * 7.29e-5 * sin(data->getLat(data->getHeightIndex(zline))*3.141592653589793238462643/180.);
	  float v0 = data->getCoefficient((int)height-1, (int)radius-1, QString("VTC0")).getValue();
	  float dz = (1/g)*(v0*v0/(radius*1000)+f*v0)*deltar/(1000);

		if (!(data->getCoefficient(height, (int)radius, QString("VTC0")) == Coefficient())) {

		  //linerad also in km
		  for (float linerad = radius; linerad < lastRing; linerad++){

		    //find dz and dlnrho for each point along the press surface towards lastring
		    f = 2 * 7.29e-5 * sin(data->getLat(data->getHeightIndex(zline))*3.141592653589793238462643/180.);
		    float v1 = data->getCoefficient((int)zline-1, (int)linerad-1, QString("VTC0")).getValue();
		    float v2 = data->getCoefficient((int)(zline+dz)-1, (int)linerad+1-1, QString("VTC0")).getValue();
		    dz = (  (1/g)*(v1*v1/(linerad*1000)+f*v1) + (1/g)*(v2*v2/((linerad+1)*1000)+f*v2)  )*deltar/(1000*2); //over 1000 to convert to km
		    zsum += dz; //in km!
		    zline += dz;
		    
		    //computing a weighted average to interpolate points above and below for center diffrence dCdz along 
		    //the next linerad out along the pressure surface.
		    //points: point 'a' is 1/2 gridpoint above the height of the pressure surface at the next linerad out, with point
		    //        'b' being 1/2 gridpoint below it.
		    //        'aa' is the nearest gridpoint above 'a', 'ab' is the nearest gripoint below 'a'.
		    //        'ba' is the nearest gridpoint above 'b', 'bb' is the nearest gripoint below 'b'.
		    //        'h' means height, 'v' means tan. vel. , 'W' is the weight for the averaging of velocities, 'f' is the corr. param.
		    //         the '1' and '2' correspond to adjacent linerad's, with '2' being 1 gridpont(km) beyond '1' these are used as
		    //         two adjacent gridpoints for trap-rule integration. 
		    float za = height+zsum+.5;
		    float zb = height+zsum-.5;
		    //values for point above, and interp around it
		    float haa = (int)za+1;
		    float hab = (int)za;
		    float Waa = fabs(hab-za);
		    float Wab = fabs(haa-za);
		    float vaa = data->getCoefficient((int)haa-1, (int)linerad-1, QString("VTC0")).getValue();
		    float vab = data->getCoefficient((int)hab-1, (int)linerad-1, QString("VTC0")).getValue();

		    if (vaa == -999){
		      vaa = vab;
		    }

		    float za2 = height+zsum+dz+.5;
		    float zb2 = height+zsum+dz-.5;
		    //values for point above, and interp around it
		    float haa2 = (int)za2+1;
		    float hab2 = (int)za2;
		    float Waa2 = fabs(hab2-za2);
		    float Wab2 = fabs(haa2-za2);
		    float vaa2 = data->getCoefficient((int)haa2-1, (int)linerad+1-1, QString("VTC0")).getValue();
		    float vab2 = data->getCoefficient((int)hab2-1, (int)linerad+1-1, QString("VTC0")).getValue();

		    if (vaa2 == -999){
		      vaa2 = vab2;
		    }

		    float faa = 2 * 7.29e-5 * sin(data->getLat(data->getHeightIndex(haa))*3.141592653589793238462643/180.);
		    float fab =  2 * 7.29e-5 * sin(data->getLat(data->getHeightIndex(hab))*3.141592653589793238462643/180.);
		    float Caa = vaa*vaa/(linerad*1000)+faa*vaa;
		    float Cab = vab*vab/(linerad*1000)+fab*vab;
		    float Ca = Caa*Waa+Cab*Wab;
		    //values for point below, and interp around it
		    float hba = (int)zb+1;
		    float hbb = (int)zb;
		    float Wba = fabs(hbb-zb);
		    float Wbb = fabs(hba-zb);
		    float vba = data->getCoefficient((int)hba-1, (int)linerad-1, QString("VTC0")).getValue();
		    float vbb = data->getCoefficient((int)hbb-1, (int)linerad-1, QString("VTC0")).getValue();

		    float fba = 2 * 7.29e-5 * sin(data->getLat(data->getHeightIndex(hba))*3.141592653589793238462643/180.);
		    float fbb =  2 * 7.29e-5 * sin(data->getLat(data->getHeightIndex(hbb))*3.141592653589793238462643/180.);
		    float Cba = vba*vba/(linerad*1000)+fba*vba;
		    float Cbb = vbb*vbb/(linerad*1000)+fbb*vbb;
		    float Cb = Cba*Wba+Cbb*Wbb;


		    float faa2 = 2 * 7.29e-5 * sin(data->getLat(data->getHeightIndex(haa2))*3.141592653589793238462643/180.);
		    float fab2 =  2 * 7.29e-5 * sin(data->getLat(data->getHeightIndex(hab2))*3.141592653589793238462643/180.);
		    float Caa2 = vaa2*vaa2/((linerad+1)*1000)+faa2*vaa2;
		    float Cab2 = vab2*vab2/((linerad+1)*1000)+fab2*vab2;
		    float Ca2 = Caa2*Waa2+Cab2*Wab2;
		    //values for point below, and interp around it
		    float hba2 = (int)zb2+1;
		    float hbb2 = (int)zb2;
		    float Wba2 = fabs(hbb2-zb2);
		    float Wbb2 = fabs(hba2-zb2);
		    float vba2 = data->getCoefficient((int)hba2-1, (int)linerad+1-1, QString("VTC0")).getValue();
		    float vbb2 = data->getCoefficient((int)hbb2-1, (int)linerad+1-1, QString("VTC0")).getValue();

		    float fba2 = 2 * 7.29e-5 * sin(data->getLat(data->getHeightIndex(hba2))*3.141592653589793238462643/180.);
		    float fbb2 =  2 * 7.29e-5 * sin(data->getLat(data->getHeightIndex(hbb2))*3.141592653589793238462643/180.);
		    float Cba2 = vba2*vba2/((linerad+1)*1000)+fba2*vba2;
		    float Cbb2 = vbb2*vbb2/((linerad+1)*1000)+fbb2*vbb2;
		    float Cb2 = Cba2*Wba2+Cbb2*Wbb2;


		    //finally, for the center difference...
		    float dcdz1 = (Ca-Cb)/deltaz;
		    float dcdz2 = (Ca2-Cb2)/deltaz;
		    float dlnrho = ( (-1/g)*(dcdz1) + (-1/g)*(dcdz2) )*deltar/2; 
		    dlnrhosum += dlnrho;

		    //Integrating hydrostatic eq to zbar to find P at height and radius
		    if (linerad == lastRing-1) {
		      float zbar = height + zsum;
 		      float pbar1 = pbar[0]; //initial surface pressure, Pa
		      for (float h = 0; h <= zbar ; h+=.01) {
 			float rho1 = 1.244*exp(-h*1000/8864);
 			float rho2 = 1.244*exp(-(h+.01)*1000/8864);
 			float dpbar=(rho1*g+rho2*g)*-deltaz*.01/2; 
 		        pbar1 += dpbar;
 		      }

		      //assigning fields
		      p[(int)radius][(int)height] = pbar1; //in Pa
		      rho[(int)radius][(int)height] = exp( std::log( 1.244*exp(-zbar/8.864) ) - dlnrhosum);
		      T[(int)radius][(int)height] = p[(int)radius][(int)height]/(rho[(int)radius][(int)height]*287);
		      Td[(int)radius][(int)height] = T[(int)radius][(int)height] - Tbar[(int)height];
		      pd[(int)radius][(int)height] = p[(int)radius][(int)height] - pbar[(int)height];
		      rhod[(int)radius][(int)height] = rho[(int)radius][(int)height] - rhobar[(int)height];

		    }		    
		  }
		}
	
		// compare analytic derivatives
	// 	float pcsdef = -5000;
// 		float zstar = 8;
// 		float x = .2;
// 		float z0 = 16;
// 		float rm = 40;
// 		float s = radius / rm;
		//float ANrhoprime = (-1/g)*( pcsdef*( 1-exp(-x/s) )* (   (-1/(zstar*1000))*exp(-height/zstar) - (3.14159/(2*z0*1000))*exp(-height/zstar)*sin(3.14159*height/(2*z0))    ) );
		//float ANpprime  = (pcsdef)*( 1-exp(-x/s) )*exp(-height/zstar)*cos(.5*3.14159*height/z0);
		
		
		
		QString vtc0=QString("VTC0");
		float meanVT1 = data->getCoefficient((int)height-1, (int)radius-1, QString("VTC0")).getValue();
		//float fref = 2 * 7.29e-5 * sin(data->getLat(data->getHeightIndex(height))*3.141592653589793238462643/180.);

		//Find Potential Temp
		float kap = .28557213930348;//287/1005
		float p0 = 100000;//pa
		float theta = T[(int)radius][(int)height]*pow((p0/p[(int)radius][(int)height]),kap);
		float thetadiff = theta - thetabar[(int)height];

		    if (height == 5) {
		      kap = kap;
		    }

		
		//Writing to Property Grid for use in writeAsi()
		propGrid[0][(int)radius][(int)height]=p[(int)radius][(int)height]/100;
		propGrid[1][(int)radius][(int)height]=rho[(int)radius][(int)height];
		propGrid[2][(int)radius][(int)height]=T[(int)radius][(int)height];
		propGrid[3][(int)radius][(int)height]=pd[(int)radius][(int)height]/100;
		propGrid[4][(int)radius][(int)height]=rhod[(int)radius][(int)height];
		propGrid[5][(int)radius][(int)height]=Td[(int)radius][(int)height];
		propGrid[6][(int)radius][(int)height]=meanVT1;
// 		propGrid[7][(int)radius][(int)height]=fref;
// 		propGrid[9][(int)radius][(int)height]=ANrhoprime;
// 		propGrid[10][(int)radius][(int)height]=ANpprime/100;
		propGrid[7][(int)radius][(int)height]=theta;
		propGrid[8][(int)radius][(int)height]=thetadiff;
	}
  }

  //Delete all those 2D arrays
  for (int i=0; i <= (int)lastRing; i++) {
    delete [] pd[i];
    delete [] rhod[i];
    delete [] Td[i];
    delete [] T[i];
    delete [] rho[i];
    delete [] p[i];
  }
  delete [] pd;
  delete [] rhod;
  delete [] Td;
  delete [] T;
  delete [] rho;
  delete [] p;

  //and delete those other arrays
  delete [] pbar;
  delete [] dpbar;
  delete [] pressD;
  delete [] Tbar;
  delete [] rhobar;
  delete [] thetabar;
  
  Message::toScreen("\n ***UNAPPROXIMATED Sovler Complete*** \n"); 
}



//********************************************************************APPROXIMATED VERSION*****************************************
//BELOW, consists of getRhoDeficit() and getTdeficit()

void VortexThread::getRhoDeficit(VortexData* data, float** rhoDeficit, float& kdim)
{
  float** drhodr = new float*[(int)lastRing+1];
  for (int i=0; i <= (int)lastRing; i++) {
    drhodr[i] = new float[(int)kdim+1];
  }
  int deltar = 1000; //meters
  float deltaz = gridData->getKGridsp()*1000; //km to meters

  float g = 9.81;
  //  float H = 8; //km
  float* FrhoBar = new float[(int)kdim+1];

	//reference density function
	for (float height = 1; height <= kdim; height++) {
	  FrhoBar[(int)height-1]=1.244*exp(-height*1000/8864)*10;
	} 
    

   QString vtc0=QString("VTC0");

  for(float height = 1; height <= kdim; height++) {

	// get the index of the height we want
	int heightindex = data->getHeightIndex(height);
	
	// Initialize arrays   
	for (float radius = 0; radius <= lastRing; radius++) {
		drhodr[(int)radius][(int)height] = -999;
		rhoDeficit[(int)radius][(int)height] = 0;
	}
	
	// Get coriolis parameters
	float f1 = 2 * 7.29e-5 * sin(data->getLat(heightindex)*3.141592653589793238462643/180.);
	float f2 = 2 * 7.29e-5 * sin(data->getLat(heightindex+1)*3.141592653589793238462643/180.);
	float f3 = 2 * 7.29e-5 * sin(data->getLat(heightindex-1)*3.141592653589793238462643/180.);

	//Find drho/dr in kg/m^4
	//rhobar divided by 10 to convert from .1 kg/m^3 to kg/m^3
	for (float radius = firstRing; radius <= lastRing; radius++) {
 	  if (!(data->getCoefficient(height, (int)radius, QString("VTC0")) == Coefficient())) {
	    
	    if (height == 1) {
	      float meanVT1 = data->getCoefficient(heightindex, (int)radius-1, QString("VTC0")).getValue();
	      float meanVT2 = data->getCoefficient(heightindex+1, (int)radius-1, QString("VTC0")).getValue();
	      
	      if (meanVT1 != 0 && meanVT2 != 0) {
		drhodr[(int)radius][(int)height] = (-1/g) * ((FrhoBar[heightindex+1]/10*((meanVT2*meanVT2/(radius*1000))+f2*meanVT2))-(FrhoBar[heightindex]/10*((meanVT1*meanVT1/(radius*1000))+f1*meanVT1))) / deltaz;
	      }
	      else {
		drhodr[(int)radius][(int)height]=-999;
	      }
	    }
	    else{
	      if (height == kdim) {
		float meanVT1 = data->getCoefficient(heightindex, (int)radius-1, QString("VTC0")).getValue();
		float meanVT3 = data->getCoefficient(heightindex-1, (int)radius-1, QString("VTC0")).getValue();
		
		if (meanVT1 != 0 && meanVT3 != 0) {
		  drhodr[(int)radius][(int)height] = (-1/g) * ((FrhoBar[heightindex]/10*((meanVT1*meanVT1/(radius*1000))+f1*meanVT1))-(FrhoBar[heightindex-1]/10*((meanVT3*meanVT3/(radius*1000))+f3*meanVT3))) / deltaz;
		}
		else {
		  drhodr[(int)radius][(int)height]=-999;
		}
	      }
	      else {
		QString vtc0=QString("VTC0");
		float meanVT1 = data->getCoefficient(heightindex, (int)radius-1, vtc0).getValue();
		float meanVT2 = data->getCoefficient(heightindex+1, (int)radius-1, vtc0).getValue();
		float meanVT3 = data->getCoefficient(heightindex-1, (int)radius-1, vtc0).getValue();
		
		if (meanVT2 != 0 && meanVT3 != 0) {
		  drhodr[(int)radius][(int)height] = (-1/g) * ((FrhoBar[heightindex+1]/10*((meanVT2*meanVT2/(radius*1000))+f2*meanVT2))-(FrhoBar[heightindex-1]/10*((meanVT3*meanVT3/(radius*1000))+f3*meanVT3))) / (deltaz*2);
		}
		else {
		  if (meanVT1 != 0 && meanVT3 != 0) {
		    drhodr[(int)radius][(int)height] = (-1/g) * ((FrhoBar[heightindex]/10*((meanVT1*meanVT1/(radius*1000))+f1*meanVT1))-(FrhoBar[heightindex-1]/10*((meanVT3*meanVT3/(radius*1000))+f3*meanVT3))) / deltaz;
		  }
		  else {
		    drhodr[(int)radius][(int)height]=-999;
		  }
		}
		if (radius==20) {
		  
		}
	      }
	    }
	  }
	}
	
	if (drhodr[(int)lastRing][(int)height] != -999) {
	  rhoDeficit[(int)lastRing][(int)height] = -drhodr[(int)lastRing][(int)height] * deltar;
	}
	
	for (float radius = lastRing-1; radius >= 0; radius--) {
	  if (radius >= firstRing) {
	    if ((drhodr[(int)radius][(int)height] != -999) and (drhodr[(int)radius+1][(int)height] != -999)) {
	      rhoDeficit[(int)radius][(int)height] = rhoDeficit[(int)radius+1][(int)height] - 
		(drhodr[(int)radius][(int)height] + drhodr[(int)radius+1][(int)height]) * deltar/2;
	    } else if (drhodr[(int)radius][(int)height] != -999) {
	      rhoDeficit[(int)radius][(int)height] = rhoDeficit[(int)radius+1][(int)height] - 
		drhodr[(int)radius][(int)height] * deltar;
	    } else if (drhodr[(int)radius+1][(int)height] != -999) {
	      rhoDeficit[(int)radius][(int)height] = rhoDeficit[(int)radius+1][(int)height] - 
		drhodr[(int)radius+1][(int)height] * deltar;
	    } else {
	      // Flat extrapolation for data gaps > 2 radii
	      rhoDeficit[(int)radius][(int)height] = rhoDeficit[(int)radius+1][(int)height];
	    }
	  } 
	  else {
	    rhoDeficit[(int)radius][(int)height] = rhoDeficit[(int)firstRing][(int)height];
	  }
	}	
  } 
  
  for (int i=0; i <= (int)lastRing; i++) {
    delete [] drhodr[i];
  }
  delete [] drhodr;
}



void VortexThread::getTDeficit(VortexData* data,float** T,float** P,float** Rho,float** theta,float** thetadiff,float** Tdeficit,float** rhoD, float& kdim, float centralPressure, float*** propGrid)
{
  int deltaz=1000; //meters
  int R=287; //metric
  float g=9.81;
  
  float* pressD = new float[(int)lastRing+1];
  float* pbar = new float[(int)kdim+1];
  float* dpbar = new float[(int)kdim+1];
  float* tbar = new float[(int)kdim+1];
  float* thetabar = new float[(int)kdim+1];
  float** Pdeficit = new float*[(int)lastRing+1];

  float* FrhoBar = new float[(int)kdim+1];

  //reference density function
  for (float height = 0; height <= kdim; height++) {
    FrhoBar[(int)height]=1.244*exp(-height*1000/8864)*10;
  } 
  
  for (int i=0; i <= (int)lastRing; i++) {
    Pdeficit[i] = new float[(int)kdim+1];
  }
  
  //Initialize WriteAsi Property Grid 
  for(float height = 0; height <= kdim; height++) {
    for (float radius=lastRing ; radius >= 0 ; radius--) {
      for (float n=0 ; n<=7 ; n++) {
	propGrid[(int)n][(int)radius][(int)height]=-999;
      }      
    }
  }

  //Using getPressureDeficit at altitude=1km for initial pressure value
  getPressureDeficit(vortexData,pressD, 1,kdim);
  pbar[0] = centralPressure-pressD[0];
  tbar[0]=pbar[0]*100/(FrhoBar[0]/10*R);
  float kap = .28557213930348;//287/1005
  float p0 = 1000; //mb
  
  //find pbar of height(aka altitude in km...not index)
  for(float height = 1; height <= kdim; height++) {
    dpbar[(int)height]=(FrhoBar[(int)height-1]*g+FrhoBar[(int)height]*g)*-deltaz/(2*1000); // /1000 to convert to hPa (since rho0 is in .1kg/m^3)
    pbar[(int)height]=pbar[(int)height-1]+dpbar[(int)height];
    tbar[(int)height]=pbar[(int)height]*100/(FrhoBar[(int)height]/10*R);
    thetabar[(int)height] = tbar[(int)height]*pow((p0/pbar[(int)height]),kap);
    
    getPressureDeficit(vortexData,pressD, height, kdim);
    
    //Use pbar and FrhoBar to find full 2D Pressure and Density field
    for (float radius=0 ; radius <= lastRing ; radius++) {
      P[(int)radius][(int)height]=(pbar[(int)height]+pressD[(int)radius])*100; //times 100 to convert hPa to Pa
      Pdeficit[(int)radius][(int)height]=pressD[(int)radius];
      Rho[(int)radius][(int)height]=FrhoBar[(int)height]/10+rhoD[(int)radius][(int)height]; //already in kg/m^3
    }
  }
  
  //use ideal gas law to find T at every height, then Tdeficit by subtraction
  for(float height = 1; height <= kdim; height++) {
    //T[(int)lastRing+1][(int)height]=tbar[(int)height];
    for (float radius=lastRing ; radius >= 1 ; radius--) {
      T[(int)radius][(int)height] = P[(int)radius][(int)height]/(Rho[(int)radius][(int)height]*R);
      Tdeficit[(int)radius][(int)height]=T[(int)radius][(int)height]-tbar[(int)height];
      
      //Find Potential Temp
      float kap = .28557213930348;//287/1005
      float p0 = 1000; //mb
      theta[(int)radius][(int)height] = T[(int)radius][(int)height]*pow((p0*100/P[(int)radius][(int)height]),kap);
      thetadiff[(int)radius][(int)height] = theta[(int)radius][(int)height] - thetabar[(int)height];

      QString vtc0=QString("VTC0");

      //Writing to Property Grid for use in writeAsi()
      propGrid[0][(int)radius][(int)height]=P[(int)radius][(int)height]/100;
      propGrid[1][(int)radius][(int)height]=Rho[(int)radius][(int)height];
      propGrid[2][(int)radius][(int)height]=T[(int)radius][(int)height];
      propGrid[3][(int)radius][(int)height]=Pdeficit[(int)radius][(int)height];
      propGrid[4][(int)radius][(int)height]=rhoD[(int)radius][(int)height];
      propGrid[5][(int)radius][(int)height]=Tdeficit[(int)radius][(int)height];
      propGrid[6][(int)radius][(int)height]=theta[(int)radius][(int)height];
      propGrid[7][(int)radius][(int)height]=thetadiff[(int)radius][(int)height];

    }
  }
    delete [] pressD;
    delete [] pbar;
    delete [] dpbar;
    for (int i=0; i <= (int)lastRing; i++) {
      delete [] Pdeficit[i];
    }
    delete [] Pdeficit;
}

//*********************************************************END APPROXIMATED VERSION*****************************************


void VortexThread::writeAsi(float*** propGrid,float& kdim)
{

  Message::toScreen("\n ***Beginning Write ASI*** \n");

	// Write out the CAPPI to an asi file

	// Set the initial fields
        QStringList fieldNames;
        fieldNames << "P " << "RO" << "T " << "PD" << "RD" << "TD" << "VT" << "H1" << "D1" << "P2" << "R2" << "T2" << "DP" << "DR" << "DT" << "H2" << "D2" ;
	float latReference = 0;
	float lonReference = 0;
	float rmin = 0;
	float rmax = lastRing;
	float rDim = lastRing+1;
	float rGridsp = 1;
	float zmin = 1;
	float zmax = kdim;
      	float kDim = kdim;
	float tDim = 0;
	//float iDim = lastRing;
	float jDim = kdim;
	float kGridsp = 1;
	float xmin = 0;
	//float xmax = 0;
	float ymin = 0;
	//float ymax = 0;
	
	QDomElement vtdConfig = configData->getConfig("vtd");
	QDomElement vortexElem = configData->getConfig("vortex");
	QDomElement radarElem = configData->getConfig("radar");
	QString vortexName = configData->getParam(vortexElem, "name");
	QString radarName = configData->getParam(radarElem,"name");
	QString timeString = vortexData->getTime().toString("yyyy_MM_ddThh_mm_ss");
	geometry = configData->getParam(vtdConfig,QString("geometry"));
	// name_radr_date_description.asi

	const QString& outFileName = "/h/eol/baldwins/vortrac/research/ResOut/" + vortexName + "_" + radarName + "_"  + timeString +  "_"  + geometry + "_PropGrid.asi";
  
	Message::toScreen("Writing file: /h/eol/baldwins/vortrac/research/ResOut/" + vortexName + "_" + radarName + "_"  + timeString + "_"  + geometry + "_PropGrid.asi \n");

	// Initialize header
	int id[511];
	for (int n = 1; n <= 510; n++) {
		id[n]=-999;
	}

	// Calculate headers
	id[175] = fieldNames.size();
    for(int n = 0; n < id[175]; n++) {
		QString name_1 = fieldNames.at(n).left(1);
        QString name_2 = fieldNames.at(n).mid(1,1);
		int int_1 = *name_1.toAscii().data();
		int int_2 = *name_2.toAscii().data();
		id[176 + (5 * n)] = (int_1 * 256) + int_2;
		id[177 + (5 * n)] = 8224;
		id[178 + (5 * n)] = 8224;
		id[179 + (5 * n)] = 8224;
		id[180 + (5 * n)] = 1;
	}

	// Cartesian file
	id[16] = 17217;
	id[17] = 21076;
  
	// Lat and Lon
	id[33] = (int)latReference;
	id[34] = (int)((latReference - (float)id[33]) * 60.);
	id[35] = (int)((((latReference - (float)id[33]) * 60.) - (float)id[34]) * 60.) * 100;
	if (lonReference < 0) {
		lonReference += 360.;
	}
	id[36] = (int)lonReference;
	id[37] = (int)((lonReference - (float)id[36]) * 60.);
	id[38] = (int)((((lonReference - (float)id[36]) * 60.) - (float)id[37]) * 60.) * 100;
	id[40] = 90;

	// Scale factors
	id[68] = 100;
	id[69] = 64;

	// X Header
	id[160] = (int)(rmin+1 * 100);
	id[161] = (int)(rmax * 100);
	id[162] = (int)rDim-1;
	id[163] = (int)rGridsp * 1000;
	id[164] = 1;
  
	// Y Header
	id[165] = (int)(zmin);
	id[166] = (int)(zmax * 100);
	id[167] = (int)kDim;
	id[168] = (int)kGridsp * 1000;
	id[169] = 2;
  
	// Z Header
	id[170] = (int)1;
	id[171] = (int)1;
	id[172] = (int)1;
	id[173] = (int)1;
	id[174] = 3;

	// Number of radars
	id[303] = 1;
  
	// Index of center
	id[309] = (int)((1 - xmin) * 1000);
	id[310] = (int)((1 - ymin) * 1000);
	id[311] = 0;
	
	// Write ascii file for grid2ps
	//Message::toScreen("Trying to write cappi to "+outFileName);
	//outFileName += ".asi";
	QFile asiFile(outFileName);
	if(!asiFile.open(QIODevice::WriteOnly)) {
	  Message::toScreen("Can't open .asi file for writing");
	}

	QTextStream out(&asiFile);
	
	// Write header
    int line = 0;
	for (int n = 1; n <= 510; n++) {
		line++;
		out << qSetFieldWidth(8) << id[n];
		if (line == 10) {
			out << endl;
            line = 0;
		}
	}

	// Write data


	for(int k = 0; k <= int(tDim); k++) {
		out << reset << "level" << qSetFieldWidth(2) << k+1 << endl;
		for(int j = 1; j <= int(jDim); j++) {
			out << reset << "azimuth" << qSetFieldWidth(3) << j << endl;

			for(int n = 0; n < fieldNames.size(); n++) {
				out << reset << left << fieldNames.at(n) << endl;
				int line = 0;
				for (int i = 0; i < int(rDim-1);  i++){
				    out << reset << qSetRealNumberPrecision(3) << scientific << qSetFieldWidth(10) << propGrid[n][i][j];
					line++;
					if (line == 8) {
						out << endl;
						line = 0;
					}
				}
				if (line != 0) {
					out << endl;
				}
			}
		}
	}	

	Message::toScreen("\n ***Write ASI complete*** \n");

}     

void VortexThread::writeANAsi(float** DataField,float& IT,int& kdim, int& xdim, int& ydim)
{

  Message::toScreen("\n ***Beginning Write_AN_ASI*** \n");

	// Write out the CAPPI to an asi file

	// Set the initial fields
        QStringList fieldNames;
        fieldNames << "VE";
	//	QString iteration = QString(IT);
	float latReference = 0;
	float lonReference = 0;
	float rmin = 0;
	float rmax = (float)xdim;
	float rDim = (float)xdim+1;
	float rGridsp = 1;
	//float zmin = 1;
	//float zmax = (float)kdim;
      	//float kDim = (float)kdim;
	float tDim = 0;
	//float iDim = lastRing;
	float jDim = (float)ydim+1;
	//float kGridsp = 1;
	float xmin = 0;
	//float xmax = 0;
	float ymin = 0;
	float ymax = ydim;
	
	QDomElement vtdConfig = configData->getConfig("vtd");
	QDomElement vortexElem = configData->getConfig("vortex");
	QDomElement radarElem = configData->getConfig("radar");
	QString vortexName = configData->getParam(vortexElem, "name");
	QString radarName = configData->getParam(radarElem,"name");
	QString timeString = vortexData->getTime().toString("yyyy_MM_ddThh_mm_ss");
	geometry = configData->getParam(vtdConfig,QString("geometry"));
	// name_radr_date_description.asi

	const QString& outFileName = "/h/eol/baldwins/vortrac/research/ResOut/ANCappi/" + vortexName + "_" + radarName + "_"  + geometry + "_AnalyticCappi" + "_Iteration" + QString().setNum(IT) + ".asi";
  
	Message::toScreen("Writing file: /h/eol/baldwins/vortrac/research/ResOut/ANCappi/" + vortexName + "_" + radarName  + "_"  + geometry + "_AnalyticCappi" + "_Iteration" + QString().setNum(IT) + ".asi \n");

	// Initialize header
	int id[511];
	for (int n = 1; n <= 510; n++) {
		id[n]=-999;
	}

	// Calculate headers
	id[175] = fieldNames.size();
    for(int n = 0; n < id[175]; n++) {
		QString name_1 = fieldNames.at(n).left(1);
        QString name_2 = fieldNames.at(n).mid(1,1);
		int int_1 = *name_1.toAscii().data();
		int int_2 = *name_2.toAscii().data();
		id[176 + (5 * n)] = (int_1 * 256) + int_2;
		id[177 + (5 * n)] = 8224;
		id[178 + (5 * n)] = 8224;
		id[179 + (5 * n)] = 8224;
		id[180 + (5 * n)] = 1;
	}

	// Cartesian file
	id[16] = 17217;
	id[17] = 21076;
  
	// Lat and Lon
	id[33] = (int)latReference;
	id[34] = (int)((latReference - (float)id[33]) * 60.);
	id[35] = (int)((((latReference - (float)id[33]) * 60.) - (float)id[34]) * 60.) * 100;
	if (lonReference < 0) {
		lonReference += 360.;
	}
	id[36] = (int)lonReference;
	id[37] = (int)((lonReference - (float)id[36]) * 60.);
	id[38] = (int)((((lonReference - (float)id[36]) * 60.) - (float)id[37]) * 60.) * 100;
	id[40] = 90;

	// Scale factors
	id[68] = 100;
	id[69] = 64;

	// X Header
	id[160] = (int)(rmin+1 * 100);
	id[161] = (int)(rmax * 100);
	id[162] = (int)rDim-1;
	id[163] = (int)rGridsp * 1000;
	id[164] = 1;
  
	// Y Header
	id[165] = (int)(ymin+1 * 100);
	id[166] = (int)(ymax * 100);
	id[167] = (int)jDim-1;
	id[168] = (int)rGridsp * 1000;
	id[169] = 2;
  
	// Z Header
	id[170] = (int)1;
	id[171] = (int)1;
	id[172] = (int)1;
	id[173] = (int)1;
	id[174] = 3;

	// Number of radars
	id[303] = 1;
  
	// Index of center
	id[309] = (int)((1 - xmin) * 1000);
	id[310] = (int)((1 - ymin) * 1000);
	id[311] = 0;
	
	// Write ascii file for grid2ps
	//Message::toScreen("Trying to write cappi to "+outFileName);
	//outFileName += ".asi";
	QFile asiFile(outFileName);
	if(!asiFile.open(QIODevice::WriteOnly)) {
	  Message::toScreen("Can't open .asi file for writing");
	}

	QTextStream out(&asiFile);
	
	// Write header
    int line = 0;
	for (int n = 1; n <= 510; n++) {
		line++;
		out << qSetFieldWidth(8) << id[n];
		if (line == 10) {
			out << endl;
            line = 0;
		}
	}

	// Write data


	for(int k = 0; k <= int(tDim); k++) {
		out << reset << "level" << qSetFieldWidth(2) << k+1 << endl;
		for(int j = 1; j <= int(jDim); j++) {
			out << reset << "azimuth" << qSetFieldWidth(3) << j << endl;

			for(int n = 0; n < fieldNames.size(); n++) {
				out << reset << left << fieldNames.at(n) << endl;
				int line = 0;
				for (int i = 1; i < int(rDim-1);  i++){
				  if ((IT == 0) or (IT == 999)) {
				    out << reset << qSetRealNumberPrecision(3) << scientific << qSetFieldWidth(10) << DataField[i][j];
				  }
				  else {
				    out << reset << qSetRealNumberPrecision(3) << scientific << qSetFieldWidth(10) << DataField[i][j]/108;
				  }
					line++;
					if (line == 8) {
						out << endl;
						line = 0;
					}
				}
				if (line != 0) {
					out << endl;
				}
			}
		}
	}	

	Message::toScreen("\n ***Write ANALYTIC Cappi ASI complete*** \n");

}     

float VortexThread::calcCentralPressure(VortexData* vortex, float* pD, float centralPressure, float height)
{

  int heightIndex = vortex->getHeightIndex(height);

  // Sum values to hold pressure estimates
  emit log(Message(QString("Calculating Pressure at height = "+QString().setNum(height)+"km "),0,this->objectName()));
  float pressWeight = 0;
  float pressSum = 0;
  numEstimates = 0;
  float pressEstimates[100];
  float weightEstimates[100];
    
  // Iterate through the pressure data
  //Message::toScreen("Size of searching List = "+QString().setNum(pressureList->size())+" within time "+QString().setNum(maxObTimeDiff)+" of vortex time "+vortex->getTime().toString(Qt::ISODate));
  for (int i = 0; i < pressureList->size(); i++) {
    float obPressure = pressureList->at(i).getPressure();
    if (obPressure > 0) {
      // Check the time
      QDateTime time = pressureList->at(i).getTime();
      if(time.date()==vortex->getTime().date()) {
	int obTimeDiff = time.time().secsTo(vortex->getTime().time());
	if ((obTimeDiff > 0) and (obTimeDiff <= maxObTimeDiff)) {
	  // Check the distance
	  float vortexLat = vortex->getLat(heightIndex);
	  float vortexLon = vortex->getLon(heightIndex);
	  float obLat = pressureList->at(i).getLat();
	  float obLon = pressureList->at(i).getLon();
	  float* relDist = gridData->getCartesianPoint(&vortexLat, &vortexLon,
						       &obLat, &obLon);
	  float obRadius = sqrt(relDist[0]*relDist[0] + relDist[1]*relDist[1]);
	  delete [] relDist;
	  
	  if ((obRadius >= vortex->getRMW(heightIndex)) and (obRadius <= maxObRadius)) {
	    // Good ob anchor!
	    presObs[numEstimates]=pressureList->at(i);
	    
	    float pPrimeOuter;
	    if (obRadius >= lastRing) {
	      pPrimeOuter = pD[(int)lastRing];
	    } else {
	      pPrimeOuter = pD[(int)obRadius];
	    }
	    float cpEstimate = obPressure - (pPrimeOuter - pD[0]);
	    float weight = (((maxObTimeDiff - obTimeDiff) / maxObTimeDiff) +
			    ((maxObRadius - obRadius) / maxObRadius)) / 2;
	    
	    // Sum the estimate and save the value for Std Dev calculation
	    pressWeight += weight;
	    pressSum += (weight * cpEstimate);
	    pressEstimates[numEstimates] = cpEstimate;
	    weightEstimates[numEstimates] = weight;
	    numEstimates++;
	    // Log the estimate
	    QString station = pressureList->at(i).getStationName();
	    QString pressLog = "Anchor pressure " + QString::number(obPressure) + " found at " + station
	      + " " + time.toString(Qt::ISODate) + "," + QString::number(obRadius) + " km"
	      + "(CP = " + QString::number(cpEstimate) + ")";
	    emit log(Message(pressLog));
	  } else if (obRadius < vortex->getRMW(heightIndex)) {
	    // Close enough to be called a central pressure
	    // Let PollThread handle that one
	  }
	
	  if(numEstimates > 100) {
	    log(Message(QString("Reached Pressure Estimate Limit"),0,this->objectName()));
	    break;
	  }
	}
      }
    }
  }
  
  // Should have a sum of pressure estimates now, if not use 1013
  float avgPressure = 0;
  float avgWeight = 0;
  emit log(Message(QString("Number of Anchor Pressures Available "+QString().setNum(numEstimates)),0,this->objectName()));
  if (numEstimates > 1) {
    //Message::toScreen("Option 1");
    avgPressure = pressSum/pressWeight;
    avgWeight = pressWeight/(float)numEstimates;
    
		// Calculate the standard deviation of the estimates and set the global variables
    float sumSquares = 0;
    for (int i = 0; i < numEstimates; i++) {
      // Message::toScreen("Pressure Estimate "+QString().setNum(i)+" = "+QString().setNum(pressEstimates[i]));
      float square = weightEstimates[i] * (pressEstimates[i] - avgPressure) * (pressEstimates[i] - avgPressure);
      sumSquares += square;
    }

    centralPressure = avgPressure;
    
  } else if (numEstimates == 1) {
    // Can use a single pressure estimate but no standard deviation

    centralPressure = pressSum/pressWeight;
    	  
    emit log(Message(QString("Single anchor pressure only, using 2.5 hPa for uncertainty"),0,this->objectName()));
    
  } else {
    //Message::toScreen("Option 3");
    // Assume standard environmental pressure
    centralPressure = 1013 - (pD[(int)lastRing] - pD[0]);
    
    emit log(Message(QString("No anchor pressures, using 1013 hPa for environment and 5 hPa for uncertainty"),0,this->objectName()));
  }

  vortex->setPressure(centralPressure);
  //vortex->setPressureUncertainty(centralPressureStdDev);
  vortex->setPressureDeficit(pD[(int)lastRing]-pD[(int)firstRing]);
  return(centralPressure);
}

void VortexThread::catchLog(const Message& message)
{
  emit log(message);
}


void VortexThread::calcPressureUncertainty(float setLimit, QString nameAddition)
{
  // Calculates the uncertanty in pressure by perturbing the center of the vortex

  // Acquire vortexData center uncertainty for the second level we examined
  int goodLevel = 0;
  float height = vortexData->getHeight(goodLevel);
  float maxCoeffs = maxWave*2 + 3;
  float centerStd = vortexData->getCenterStdDev(goodLevel);
  
  //Message::toScreen("VortexThread: CalcPressureUncertainty: Uncertainty of center from vortexData is "+QString().setNum(centerStd));
  
  if(setLimit!=0) {
    centerStd = setLimit;
    //Message::toScreen("VortexThread: CalcPressureUncertainty: Uncertainty of center from vortexData is "+QString().setNum(centerStd));    
  }
  if(nameAddition!=QString())
    nameAddition = nameAddition+QString().setNum(centerStd);

  // Now move this amount of space in each cardinal direction to get 4 additional pressure estimates
  int numErrorPoints = 4;
  float angle = 2*acos(-1)/numErrorPoints;

  // Create extra vortexData to house uncertainty stuff
  QString file("vortrac_defaultVortexListStorage.xml");
  Configuration* vortexConfig = new Configuration(0, file);
  vortexConfig->setLogChanges(false);
  connect(vortexConfig, SIGNAL(log(const Message&)), 
	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
  VortexList* errorVertices = new VortexList(vortexConfig);
  errorVertices->open();

  QDomElement vortexElem = configData->getConfig("vortex");
  QDomElement radarElem = configData->getConfig("radar");
  QString vortexPath = configData->getParam(vortexElem, "dir");
  QString vortexName = configData->getParam(vortexElem, "name");
  errorVertices->setVortexName(vortexName);
  QString radarName = configData->getParam(radarElem,"name");
  errorVertices->setRadarName(radarName);
  errorVertices->setProductType("Uncertainty in Pressure");
  QString timeString = vortexData->getTime().toString("yyyy_MM_ddThh_mm_ss");
  
  QDir workingDirectory(vortexPath);
  if(nameAddition!=QString()) {

    // Output of the uncertainty will save if the file name addition is not 
    // empty - Testing and analysis of methods only

    workingDirectory.mkdir("pError_"+nameAddition+"_"+timeString);
    QString outFileName = workingDirectory.path()+"/";
    outFileName += vortexName+"_"+radarName+"_"+timeString+"_"+nameAddition+".xml";
    errorVertices->setFileName(outFileName);
    errorVertices->setNewWorkingDirectory(workingDirectory.path()+"/pError_"+nameAddition+"_"+timeString+"/");
    errorVertices->save();
  }

  // Add the current vortexData to the errorVertices for comparison purposes
  // Be careful how we handle this point in subsequent averages
  //errorVertices->setNewWorkingDirectory(workingDirectory.path()+"/uncertainty+"+QString().setNum(0)+"/");
  //workingDirectory.mkdir("uncertainty"+QString().setNum(0)+"/");
  errorVertices->append(*vortexData);
  if(nameAddition!=QString())
    errorVertices->save();
  
  emit log(Message(QString(),2,this->objectName()));  // 82 %
  
  // Create a GBVTD object to process the rings
  if(closure.contains(QString("hvvp"),Qt::CaseInsensitive)) {
    if(calcHVVP(false)) {
      vtd = new GBVTD(geometry, closure, maxWave, dataGaps, hvvpResult);
      emit log(Message(QString(),0,this->objectName(),Green));
    }
    else{
      emit log(Message(QString(),0,this->objectName(),Yellow,
		       QString("Could Not Retrieve HVVP Wind")));
      vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
    }
  }
  else {
    vtd = new GBVTD(geometry, closure, maxWave, dataGaps);
  }
  vtdCoeffs = new Coefficient[20];

  int loopPercent = int(15.0/float(numErrorPoints));
  int endPercent = 15-(numErrorPoints*loopPercent);
  
  
  float refLat = vortexData->getLat(goodLevel);
  float refLon = vortexData->getLon(goodLevel);
  float sqDeficitSum = 0;
  for(int p = 0; p < numErrorPoints; p++) {
    VortexData* errorVertex = new VortexData(1,vortexData->getNumRadii(), vortexData->getNumWaveNum());
    errorVertex->setTime(vortexData->getTime().addDays(p).addYears(2));
    
    // Set the reference point
    float* newLatLon = gridData->getAdjustedLatLon(refLat,refLon,
						   centerStd*cos(p*angle),centerStd*sin(p*angle));
    errorVertex->setLat(0,newLatLon[0]); 
    errorVertex->setLon(0,newLatLon[1]);
    errorVertex->setHeight(0,height);
    gridData->setAbsoluteReferencePoint(newLatLon[0], newLatLon[1], height);
    delete  [] newLatLon;

    emit log(Message(QString(),loopPercent,this->objectName()));

    if ((gridData->getRefPointI() < 0) || 
	(gridData->getRefPointJ() < 0) ||
	(gridData->getRefPointK() < 0)) {
      // Out of bounds problem
      emit log(Message(QString("Error Vertex is outside CAPPI"),0,
		       this->objectName()));
      continue;
    }			
    
    for (float radius = firstRing; radius <= lastRing; radius++) {
      
      // Get the cartesian points
      float xCenter = gridData->getCartesianRefPointI();
      float yCenter = gridData->getCartesianRefPointJ();
      
      // Get the data
      int numData = gridData->getCylindricalAzimuthLength(radius, height);
      float* ringData = new float[numData];
      float* ringAzimuths = new float[numData];
      gridData->getCylindricalAzimuthData(velField, numData, radius, height, ringData);
      gridData->getCylindricalAzimuthPosition(numData, radius, height, ringAzimuths);
	
      // Call gbvtd
      if (vtd->analyzeRing(xCenter, yCenter, radius, height, numData, ringData,
			   ringAzimuths, vtdCoeffs, vtdStdDev)) {
	if (vtdCoeffs[0].getParameter() == "VTC0") {
	  // VT[v] = vtdCoeffs[0].getValue();
	} else {
	  emit log(Message(QString("CalcPressureUncertainty:Error retrieving VTC0 in vortex!"),0,this->objectName()));
	} 
      } else {
	QString err("Insufficient data for VTD winds at radius ");
	QString loc;
	err.append(loc.setNum(radius));
	err.append(", height ");
	err.append(loc.setNum(height));
	emit log(Message(err));
      }
      
      // All done with this radius and height, archive it
      int vertexLevel = 0;
      archiveWinds(*errorVertex, radius, vertexLevel, maxCoeffs);

      // Clean up
      delete[] ringData;
      delete[] ringAzimuths;
    }

    // Now calculate central pressure for each of these
    float* errorPressureDeficit = new float[(int)lastRing+1];
    getPressureDeficit(errorVertex,errorPressureDeficit, height);
    errorVertex->setPressureDeficit(fabs(*errorPressureDeficit));

    // Sum for Deficit Uncertainty
    sqDeficitSum += (errorVertex->getPressureDeficit()-vortexData->getPressureDeficit())*(errorVertex->getPressureDeficit()-vortexData->getPressureDeficit());
  
    // Add in uncertainty from multiple pressure measurements
    if(numEstimates < 1){
      // No outside data available use the 1013 bit.
      errorVertex->setPressure(1013 - (errorPressureDeficit[(int)lastRing] - errorPressureDeficit[0]));
      errorVertex->setPressureDeficit(errorPressureDeficit[(int)lastRing] - errorPressureDeficit[0]);
      //Message::toScreen("Adding uncertainty calib pressure "+QString().setNum(errorVertex->getPressure()));
      errorVertices->append(*errorVertex);
      if(nameAddition!=QString())
	errorVertices->save();
    }
    else {
      for(int j = 0; j < numEstimates; j++) {
	QString stationName = presObs[j].getStationName();
	QString obsTime = presObs[j].getTime().toString();
	float obPressure = presObs[j].getPressure();
	float vortexLat = errorVertex->getLat(0);
	float vortexLon = errorVertex->getLon(0);
	float obLat = presObs[j].getLat();
	float obLon = presObs[j].getLon();
	float* relDist = gridData->getCartesianPoint(&vortexLat, &vortexLon,
						     &obLat, &obLon);
	float obRadius = sqrt(relDist[0]*relDist[0] + relDist[1]*relDist[1]);
	delete [] relDist;
	float pPrimeOuter;
	if (obRadius >= lastRing) {
	  pPrimeOuter = errorPressureDeficit[(int)lastRing];
	} else {
	  pPrimeOuter = errorPressureDeficit[(int)obRadius]; 
	}
	//Message::toScreen("Calculating uncertainty calib "+QString().setNum(j)+" = "+QString().setNum(obPressure)+" @ "+QString().setNum(obRadius));
	errorVertex->setPressure(obPressure - (pPrimeOuter - errorPressureDeficit[0]));
	errorVertex->setPressureDeficit(errorPressureDeficit[(int)lastRing] - errorPressureDeficit[0]);

	VortexData* errorVertex2 = new VortexData(*errorVertex);
	errorVertex2->setTime(vortexData->getTime().addYears(2).addDays(p).addSecs((j+1)*60));
	errorVertices->append(*errorVertex2);
	QString obsStamp = stationName+"_"+obsTime;
	obsStamp+="_"+QString().setNum(obPressure);
	errorVertices->setIndividualProductType(errorVertices->count()-1,
						obsStamp);
	//Message::toScreen("Adding uncertainty calib pressure "+QString().setNum(errorVertex->getPressure()));
	
	if(nameAddition!=QString())
	  errorVertices->save();
	delete errorVertex2;
      }
    }
    delete [] errorPressureDeficit;
    delete errorVertex;
  }
  
  emit log(Message(QString(),endPercent, this->objectName()));
  
  delete [] vtdCoeffs;
  delete vtd;
  
  // Standard deviation from the center point
  float sqPressureSum = 0;
  for(int i = 1; i < errorVertices->count();i++) {
    sqPressureSum += pow((errorVertices->at(i).getPressure()-vortexData->getPressure()),2);
  }
  float pressureUncertainty = sqrt(sqPressureSum/(errorVertices->count()-2));
  float deficitUncertainty = sqrt(sqDeficitSum/(numErrorPoints-1));
  if((numEstimates <= 1)&&(pressureUncertainty < 2.5)) {
    pressureUncertainty = 2.5;
  }
  if(numEstimates == 0)
    pressureUncertainty = 5.0;

  vortexData->setPressureUncertainty(pressureUncertainty);
  vortexData->setDeficitUncertainty(deficitUncertainty);
  float aveRMWUncertainty = 0;
  int goodrmw = 0;
  for(int jj = 0; jj < vortexData->getNumLevels();jj++) {
    if(vortexData->getRMWUncertainty(jj) < (gridData->getIGridsp()/2)||
       vortexData->getRMWUncertainty(jj) < (gridData->getJGridsp()/2)) {
      if(gridData->getIGridsp()>=gridData->getJGridsp()) 
	vortexData->setRMWUncertainty(jj,gridData->getIGridsp()/2);
      else
	vortexData->setRMWUncertainty(jj,gridData->getJGridsp()/2);
    }
    if(fabs(vortexData->getRMWUncertainty(jj))<10 
       && vortexData->getRMW()!=-999 && vortexData->getRMW()>0) {
      goodrmw++;
      aveRMWUncertainty += vortexData->getRMWUncertainty();
    }
  }
  vortexData->setAveRMWUncertainty(aveRMWUncertainty/(1.0*goodrmw));

  //Message::toScreen(nameAddition+" uncertainty is "+QString().setNum(pressureUncertainty));
  errorVertices->takeFirst();
  errorVertices->prepend(*vortexData);
  if(nameAddition!=QString())
    errorVertices->save();
  delete errorVertices;

}

void VortexThread::readInConfig()
{
  QDomElement vtdConfig = configData->getConfig("vtd");
  QDomElement pressureConfig = configData->getConfig("pressure");

   vortexPath = configData->getParam(vtdConfig, 
				     QString("dir"));
   geometry = configData->getParam(vtdConfig,
				   QString("geometry"));
   refField =  configData->getParam(vtdConfig,
				    QString("reflectivity"));
   velField = configData->getParam(vtdConfig,
				   QString("velocity"));
   closure = configData->getParam(vtdConfig,
				  QString("closure"));
   
   firstLevel = configData->getParam(vtdConfig,
				     QString("bottomlevel")).toFloat();
   lastLevel = configData->getParam(vtdConfig,
				    QString("toplevel")).toFloat();
   firstRing = configData->getParam(vtdConfig,
				    QString("innerradius")).toFloat();
   lastRing = configData->getParam(vtdConfig, 
				   QString("outerradius")).toFloat();
   
   ringWidth = configData->getParam(vtdConfig, 
				    QString("ringwidth")).toFloat();
   maxWave = configData->getParam(vtdConfig, 
				  QString("maxwavenumber")).toInt();

   // Define the maximum allowable data gaps
   dataGaps = new float[maxWave+1];
   for (int i = 0; i <= maxWave; i++) {
     dataGaps[i] = configData->getParam(vtdConfig, 
					QString("maxdatagap"), QString("wavenum"), 
					QString().setNum(i)).toFloat();
   }

   // Set GriddedData to use ringwidth for spacing
   gridData->setCylindricalAzimuthSpacing(ringWidth);

  maxObRadius = 0;
  maxObTimeDiff = 60 * configData->getParam(pressureConfig, "maxobstime").toFloat();
  if(configData->getParam(pressureConfig, "maxobsmethod") == "center")
    maxObRadius = configData->getParam(pressureConfig, "maxobdist").toFloat();
  if(configData->getParam(pressureConfig, "maxobsmethod") == "ring")
    maxObRadius = lastRing+configData->getParam(pressureConfig, "maxobsdist").toFloat();

  if(maxObRadius == -999){
     maxObRadius = lastRing + 50;
  }
  if(maxObTimeDiff == -999){
     maxObTimeDiff = 59 * 60;
  }
  /*
  gradientHeight = configData->getParam(pressureConfig, "height").toFloat();
  Message::toScreen("Gradient Height is "+QString().setNum(gradientHeight)+" firstLevel is "+QString().setNum(firstLevel));
  if(gradientHeight < firstLevel) {
    QString message = QString(tr("Attempting to calculate pressure gradient below analyzed levels, using lowest analysis level ")+QString().setNum(firstLevel)+tr(" km"));
    emit log(Message(message, 0, this->objectName(),Yellow,
		     QString("Parameter Disagreement")));
    gradientHeight = firstLevel;
  }

 if(gradientHeight > lastLevel) {
    QString message = QString(tr("Attempting to calculate pressure gradient above analyzed levels, using highest analysis level ")+QString().setNum(firstLevel)+tr(" km"));
    emit log(Message(message, 0, this->objectName(),Yellow,
		     QString("Parameter Disagreement")));
    gradientHeight = lastLevel;
  }
  */
  gradientHeight = firstLevel;
}

bool VortexThread::calcHVVP(bool printOutput)
{
  // Get environmental wind
  /*
   * rt: Range from radar to circulation center (km).
   *
   * cca: Meteorological azimuth angle of the direction
   *      to the circulation center (degrees from north).
   *
   * rmw: radius of maximum wind measured from the TC circulation
   *      center outward.
   */

  int gradientIndex = vortexData->getHeightIndex(gradientHeight);
  QDomElement radar = configData->getConfig("radar");
  float radarLat = configData->getParam(radar,"lat").toFloat();
  float radarLon = configData->getParam(radar,"lon").toFloat();
  float vortexLat = vortexData->getLat(gradientIndex);
  float vortexLon = vortexData->getLon(gradientIndex);
  
  float* distance;
  distance = gridData->getCartesianPoint(&radarLat, &radarLon, 
					 &vortexLat, &vortexLon);
  float rt = sqrt(distance[0]*distance[0]+distance[1]*distance[1]);
  float cca = atan2(distance[0], distance[1])*180/acos(-1);
  delete [] distance;
  
  if(printOutput) {
    //Message::toScreen("Vortex (Lat,Lon): ("+QString().setNum(vortexLat)+", "+QString().setNum(vortexLon)+")");
    emit log(Message(QString("Vortex (Lat,Lon): ("+QString().setNum(vortexLat)+", "+QString().setNum(vortexLon)+")")));
    QString hvvpInput("Hvvp Parameters: Distance to Radar "+QString().setNum(rt)+" (km), angle to vortex center in degrees ccw from north "+QString().setNum(cca)+" (deg), rmw "+QString().setNum(vortexData->getAveRMW())+" km");
    emit log(Message(hvvpInput,0,this->objectName()));
    //Message::toScreen(hvvpInput);
  }
  
  Hvvp *envWindFinder = new Hvvp;
  envWindFinder->setConfig(configData);
  envWindFinder->setPrintOutput(printOutput);
  connect(envWindFinder, SIGNAL(log(const Message)), 
	  this, SLOT(catchLog(const Message)), 
	  Qt::DirectConnection);
  envWindFinder->setRadarData(radarVolume,rt, cca, vortexData->getAveRMW());
  emit log(Message(QString(), 1,this->objectName()));
  //envWindFinder->findHVVPWinds(false); for first fit only
  bool hasHVVP = envWindFinder->findHVVPWinds(true);
  hvvpResult = envWindFinder->getAvAcrossBeamWinds();
  hvvpUncertainty = envWindFinder->getAvAcrossBeamWindsStdError();
  if(isnan(hvvpResult)||(hvvpResult == -999)||
     isnan(hvvpUncertainty)||(hvvpUncertainty == -999)){
    hvvpResult = 0;
    hvvpUncertainty = 0;
    hasHVVP = false;
  }

  QString finalHVVP;
  if(printOutput && hasHVVP)
    finalHVVP = QString("Hvvp finds mean wind "+QString().setNum(hvvpResult)+" +/- "+QString().setNum(fabs(hvvpUncertainty)));
  
  emit log(Message(finalHVVP, 0,this->objectName()));
  delete envWindFinder;
  
  return hasHVVP;
}


void VortexThread::DownSimplex(float vmxguess, float vmyguess, int h) {
  // Allocate memory for the simplex vertices	
  vertex = new float*[3];
  vertex[0] = new float[2];
  vertex[1] = new float[2];
  vertex[2] = new float[2];

  STD = new float[2];
  vertexSum = new float[2];
  

  //parameters are VMx and VMy
  //set the initial simplex vertices

	vertex[0][0] = vmxguess;
	vertex[0][1] = vmyguess;

	vertex[1][0] = vmxguess + 4;
	vertex[1][1] = vmyguess;

	vertex[2][0] = vmxguess;
	vertex[2][1] = vmyguess + 4;


	
	vertexSum[0] = 0;
	vertexSum[1] = 0;

	//find standard deviation of gradVd*D for each vertex
	for (int v=0; v <= 2; v++) {	
	  //VT[v] = getS2(vertex[v][0],vertex[v][1]);
	  VMx = vertex[v][0];
	  VMy = vertex[v][1];
	  STD[v] = IterateMeanWind(h);
	}
	/////////////////////////////////////////////////
	// Run the simplex search loop
	getVertexSum();
	int numIterations = 0;
	int low = 0;
	int mid = 0;
	int high = 0;
	
	for(;;) {
		
		low = 0;
		// Sort the initial guesses
		high = STD[0] > STD[1] ? (mid = 1,0) : (mid = 0,1);
		for (int v=0; v<=2; v++) {
			if (STD[v] <= STD[low]) low = v;
			if (STD[v] > STD[high]) {
				mid = high;
				high = v;
			} else if (STD[v] > STD[mid] && v != high) mid = v;
		}
		
		// Check convergence
		float epsilon = 2.0 * fabs(STD[high]-STD[low])/(fabs(STD[high]) + fabs(STD[low]) + 1.0e-10);
		Message::toScreen(QString().setNum(IT)+" "+QString().setNum(h)+" "+QString().setNum(vertex[high][0])+"-"+QString().setNum(vertex[high][1])+":"+QString().setNum(STD[high])+" "+QString().setNum(vertex[mid][0])+"-"+QString().setNum(vertex[mid][1])+":"+QString().setNum(STD[mid])+" "+QString().setNum(vertex[low][0])+"-"+QString().setNum(vertex[low][1])+":"+QString().setNum(STD[low]));
		if (epsilon < 0.05) {
		  VMx = vertex[high][0];
		  VMy = vertex[high][1];

		  //print the best mean wind field for level 1
		  if (h == 0) {
		    writeANAsi(MeanWindField,IT,zdim, xdim, ydim);
		  }

		  break;
		}
		
		// Check iterations
		if (numIterations > 100) {
			std::cout << "Too many iterations (>100)";
			
			break;
		}
		
		numIterations += 2;
		// Reflection
		float STDtest = simplexTest(low, -1.0, h);
		if (STDtest >= STD[high])
			// Better point than highest, so try expansion
			STDtest = simplexTest(low, 2.0, h);
		else if (STDtest <= STD[mid]) { 
			// Worse point than second highest, so try contraction
			float STDsave = STD[low];
			STDtest = simplexTest(low, 0.5, h);
			if (STDtest <= STDsave) {
				for (int v=0; v<=3; v++) {
					if (v != high) {
						for (int i=0; i<=2; i++)
							vertex[v][i] = vertexSum[i] = 0.5*(vertex[v][i] + vertex[high][i]);
						VMx = vertex[v][0];
						VMy = vertex[v][1];
						STD[v] = IterateMeanWind(h);
					}
				}
				numIterations += 2;
				getVertexSum();
			}
		} else --numIterations;
		
	}


	

}




float VortexThread::IterateMeanWind(int h) {


		      QString fieldName = "VE";	
  		      float** CappiWindField = new float*[xdim];
		      bool** goodData = new bool*[xdim];
		      //float** MeanWindField = new float*[xdim];
		      float** vmx = new float*[xdim];
		      float** vmy = new float*[xdim];
		      for (int x =0; x < xdim; x++) {
			CappiWindField[x] = new float[ydim];
			//MeanWindField[x] = new float[ydim];
			vmx[x] = new float[ydim];
			vmy[x] = new float[ydim];
			goodData[x] = new bool[ydim];
		      }	      

		      // Get the cartesian center
		      float xCenter = gridData->getCartesianRefPointI();
		      float yCenter = gridData->getCartesianRefPointJ();
		      float centerDistance = sqrt(xCenter*xCenter + yCenter*yCenter);
		      		      
		      // Get thetaT
		      float thetaT = atan2(yCenter,xCenter);
		      thetaT = vtd->fixAngle(thetaT);
		      
		      //retrieve the cappi wind field
		      float xref;
		      for (int i = 1; i < xdim; i++) {
			xref = gridData->getCartesianPointFromIndexI(i);
			CappiWindField[i] = gridData->getCartesianYslice(fieldName,xref,h+1);
		      }
		      //float MeanWindDiff = 100;
		      //   bool FirstIteration = true;
		      //float LastMeanWind = 0;
		      //float NewMeanWind = -999;
		      QString vtc0 = "VTCO";
		      QString vrc0 = "VRCO";
		      //float VMCosCheck;
		      //float VMx;
		      //float VMy;

		      //float stdold = 999999999; //very large!
		      //float lastthetaM;

		      //write out the cappi field as asi
		      if (h == 0 && IT == 0) {
			writeANAsi(CappiWindField,IT,zdim, xdim, ydim);
		      }

			IT += 1;


			
			//	VMx = Vm*cos(thetaM);
			//VMy = Vm*sin(thetaM);

			vtd->setGVTD(VMx,VMy);

			//Message::toScreen(QString().setNum(NewMeanWind) + " " + QString().setNum(stdbad)+ " "+ QString().setNum(stdgood)+ " "+ QString().setNum(numthrownout) );

			float* B1B3 = new float[(int)(lastRing-firstRing)+1];
			float* slopeiness = new float[(int)(lastRing-firstRing)+1];

			//find new VT and VR wind field
			for (float radius = firstRing; radius <= lastRing; radius++) {
			  
			  // Get the data
			  int numData = gridData->getCylindricalAzimuthLength(radius, height);
			  float* ringData = new float[numData];
			  float* ringAzimuths = new float[numData];
			  gridData->getCylindricalAzimuthData(velField, numData, radius, height, ringData);
			  gridData->getCylindricalAzimuthPosition(numData, radius, height, ringAzimuths);
			  
			  // Call gvtd		  
			  //float* vars = new float[2]; 
			  float vmcos = -999;
			  float b1b3 = -999;
			  vtd->analyzeMeanWind(xCenter, yCenter, radius, height, numData, ringData,
						       ringAzimuths, vtdCoeffs, vtdStdDev, &vmcos, &b1b3); 
			  
			  VMCosCheck = vmcos;
			  B1B3[(int)radius] = b1b3;
			  slopeiness[(int)radius] = -999;

			  //update vortexData with new values
			  archiveWinds(radius, storageIndex, maxCoeffs);

			  delete[] ringData;
			  delete[] ringAzimuths;

			
			}

			//find outer vortex VT slope parameter
			float thetaM = atan2(VMy,VMx);
			float Vmsin = sqrt(VMx*VMx + VMy*VMy)*sin(thetaT - thetaM);
			for (int radius = firstRing+1; radius <= lastRing-1; radius++) {
			  float dB1B3dr = ( B1B3[radius+1] - B1B3[radius-1] )/2;
			  float weight = 1;
			  slopeiness[radius] = weight*(Vmsin + centerDistance*dB1B3dr);
			}

			//printing sample values
			/*
			if (height == 1 && IT == 17) {
			  for (int i = 0; i <= lastRing; i++) {
			    float radius = i;
			    if ((radius < lastRing) && (radius > 0)) {
			      float Vter = vortexData->getCoefficient(height, radius, QString("VTC0")).getValue();
			      float Vrer = vortexData->getCoefficient(height, radius, QString("VRC0")).getValue();
			      Message::toScreen(QString().setNum(IT) + " "+QString().setNum(radius) + " "+QString().setNum(Vter) + " "+ QString().setNum(Vrer)+ " " + QString().setNum(slopeiness[i]) );
			    }
			  }
			}
			*/

			//find the mean wind field from new VTC0 and VRC0
			float xpos;float ypos;float D;float xp;float yp;float theta;float R;float Vt;float Vr;float VTC0;float VRC0;float VTC1; float VTS1;
			float radius;
		
			for (int i = 1; i < xdim; i++) {
			  for (int j = 1; j < ydim; j++) {
			    goodData[i][j] = false; //for now...
			    xpos = gridData->getCartesianPointFromIndexI(i);
			    ypos = gridData->getCartesianPointFromIndexJ(j);
			    D = sqrt( xpos*xpos + ypos*ypos );
			    xp = xpos - xCenter;
			    yp = ypos - yCenter;
			    theta = atan2(yp,xp);
			    theta = vtd->fixAngle(theta);
			    R = sqrt( xp*xp + yp*yp );

			    if ( R-(int)R < .5 ) {
			    radius = (int)R;
			    } else {
			      radius = (int)R + 1;
			    }
			  

			    if ((radius < lastRing) && (radius > 0)) {
			      // these values depend on NumWaveNum, set in AnalysisThread.cpp. 
			      //        i.e. if NumWaveNum = 2, VTC1 and VTS1 return -999
			      VTC0 = vortexData->getCoefficient(height, radius, QString("VTC0")).getValue();
			      VTC1 = vortexData->getCoefficient(height, radius, QString("VTC1")).getValue();
			      VTS1 = vortexData->getCoefficient(height, radius, QString("VTS1")).getValue();
			      VRC0 = vortexData->getCoefficient(height, radius, QString("VRC0")).getValue();
			      if (VTC0 == -999) { VTC0 = 0;}
			      if (VTC1 == -999) { VTC1 = 0;}
			      if (VTS1 == -999) { VTS1 = 0;}
			      if (VRC0 == -999) { VRC0 = 0;}
			      //			      Vt = VTC0 + VTC1*cos(theta - thetaT) + VTS1*sin(theta - thetaT);
			      Vt = VTC0;
			      Vr = VRC0;

			      if ( (Vt != -999) ) {
				goodData[i][j] = true;
			      }
			    }
			    else { Vt = 0; Vr = 0;}

			    

			    //   float meanVTx1 = vortexData->getCoefficient((int)h, (int)radius, QString("VTC0")).getValue();
			    // float meanVTx2 = vortexData->getCoefficient((int)h, (int)radius+1, QString("VTC0")).getValue();
		      
			      //			    }
	// 		    else{
// 			      Vt = NewVt;
// 			      Vr = NewVr;
// 			    }
			    // if (radius < 10){
			      //float thetaLAME = theta;
			    // }
			    MeanWindField[i][j] = CappiWindField[i][j]*D + centerDistance*Vt*sin(theta - thetaT) - centerDistance*Vr*(R/centerDistance + cos(theta - thetaT));
			    // if ( height == 1 ) {
			      // Message::toScreen("VT or VR NOT ZERO>>> Iteration=" + QString().setNum(IT) + ", level=" + QString().setNum(h) + ", Radius=" + QString().setNum(radius) + ", i="+ QString().setNum(i) + ", j="+ QString().setNum(j) + ", Vt="+ QString().setNum(Vt) + ", Vr="+ QString().setNum(Vr) );
			      //  Message::toScreen(QString().setNum(Vt) + " "+ QString().setNum(Vr) ); 
 			    //}
			    //MeanWindField[i][j] = (  CappiWindField[i][j] + Vt*sin(theta - thetaT)*centerDistance/D 
			    //			     - Vr/D*centerDistance*(R/centerDistance + cos(theta - thetaT))   ) * D;
			  }
			}

			
			//Perform the gradient using center difference:
			float dx = 2;
			float dy = 2;
			int numGradValues = 0;
			float gradYsum = 0;
			for (int x = 1; x < xdim; x++) {
			  for (int y = 2; y < ydim-1; y++) {
			    if (goodData[x][y] && goodData[x][y+1]) {
			      vmy[x][y] = (MeanWindField[x][y+1] - MeanWindField[x][y-1])/dy;
			      gradYsum += vmy[x][y];
			      numGradValues += 1;
			    }
			  }
			}
			float Ygradient = gradYsum/numGradValues;
			numGradValues = 0;
			float gradXsum = 0;
			for (int y = 1; y < ydim; y++) {
			  for (int x = 2; x < xdim-1; x++) {
			    if (goodData[x][y]  && goodData[x+1][y]) {
			      vmx[x][y] = (MeanWindField[x+1][y] - MeanWindField[x-1][y])/dx;
			      gradXsum += vmx[x][y];
			      numGradValues += 1;
			    }
			  }
			}
			float Xgradient = gradXsum/numGradValues;

	


			//find standard dev., then remove outliers
			//float Vmbar = sqrt( Xgradient*Xgradient + Ygradient*Ygradient );
			float vmxbar = VMx;
			//float Thetambar = atan2(Ygradient,Xgradient);
			float vmybar = VMy;
			float stdnew = 0;
			float stdbad = 0;
			float stdgood = 0;
			float numthrownout = 0;
			float n = 0;
			int rad;
			for (int y = 2; y < ydim; y++) {
			  for (int x = 2; x < xdim-1; x++) {
			    if (goodData[x][y]) {
			      xpos = gridData->getCartesianPointFromIndexI(x);
			      ypos = gridData->getCartesianPointFromIndexJ(y);
			      xp = xpos - xCenter;
			      yp = ypos - yCenter;
			      R = sqrt( xp*xp + yp*yp );
			      if ( R-(int)R < .5 ) {
				rad = (int)R;
			      } else {
				rad = (int)R + 1;
			      }
			      //float Vm = sqrt( vmx[x][y]*vmx[x][y] + vmy[x][y]*vmy[x][y] );
			      //float Thetam = atan2(vmy[x][y],vmx[x][y]);
			      float VMX = vmx[x][y];
			      float VMY = vmy[x][y];
			      stdnew +=  (VMX - vmxbar)*(VMX - vmxbar) + (VMY - vmybar)*(VMY - vmybar);
			      n++;
			      
			    }
			  }
			}
			if (n != 0) {
			  stdbad = sqrt(stdnew/n);
			}
		       	
			//find new, improved standard deviation
			stdnew = 0;
			n = 0;	
			for (int y = 2; y < ydim; y++) {
			  for (int x = 2; x < xdim-1; x++) {
			    if (goodData[x][y]) {
			      xpos = gridData->getCartesianPointFromIndexI(x);
			      ypos = gridData->getCartesianPointFromIndexJ(y);
			      xp = xpos - xCenter;
			      yp = ypos - yCenter;
			      R = sqrt( xp*xp + yp*yp );
			      if ( R-(int)R < .5 ) {
				rad = (int)R;
			      } else {
				rad = (int)R + 1;
			      }
			      //float Vm = sqrt( vmx[x][y]*vmx[x][y] + vmy[x][y]*vmy[x][y] );
			      //float Thetam = atan2(vmy[x][y],vmx[x][y]);
			      float VMX = vmx[x][y];
			      float VMY = vmy[x][y];
			      
			      //quality control:
			      float variance = sqrt( (VMX - vmxbar)*(VMX - vmxbar) + (VMY - vmybar)*(VMY - vmybar) );
			      if (variance > stdbad) {
				//throw out values greater than one standard Dev.
				goodData[x][y] = 0;
				numthrownout++;
			      }
			      else {
				//if ( (IT == 1) && (h == 0) ) {
				  // Message::toScreen(QString().setNum(VMX)+" "+QString().setNum(VMY));
				//}
				if (rad < 100) {
				  stdnew +=  (VMX - vmxbar)*(VMX - vmxbar) + (VMY - vmybar)*(VMY - vmybar);
				}
				else {
				  stdnew +=  (VMX - vmxbar)*(VMX - vmxbar) + (VMY - vmybar)*(VMY - vmybar) + slopeiness[rad]*slopeiness[rad];
				}
				n++;
			      }
			    }
			  }
			}
			//	Message::toScreen("NumThrownOut: " + QString().setNum(numthrownout));
      			if (n != 0) {
			  stdgood = sqrt(stdnew/n);
			}


			//deleting arrays
			for (int i=0; i < ydim; i++) {
			  // delete [] MeanWindField[i];
			  delete [] vmx[i];
			  delete [] vmy[i];
			  delete [] CappiWindField[i];
			  delete [] goodData[i];
			}
			//delete [] MeanWindField;
			delete [] vmx;
			delete [] vmy;
			delete [] CappiWindField;
			delete [] goodData;

			float returnstd = stdgood*(-1);

			return returnstd;
}

inline void VortexThread::getVertexSum()
{
  
  float sum;
  int v;
  for (int i=0; i<=1; i++) {
    for (sum = 0.0, v=0; v<=2; v++)
      sum += vertex[v][i];
    vertexSum[i] = sum;
  }
}

float VortexThread::simplexTest(int& low, float factor, int h)
{
	
	// Test a simplex vertex
	float STDtest = -999;
	float* vertexTest = new float[2];
	float factor1 = (1.0 - factor)/2;
	float factor2 = factor1 - factor;
	for (int i=0; i<=1; i++)
		vertexTest[i] = vertexSum[i]*factor1 - vertex[low][i]*factor2;
	VMx = vertexTest[0];
	VMy = vertexTest[1];
	STDtest = IterateMeanWind(h);
	
	// If its a better point than the worst, replace it
	if (STDtest > STD[low]) {
		STD[low] = STDtest;
		for (int i=0; i<=1; i++) {
			vertexSum[i] += vertexTest[i]-vertex[low][i];
			vertex[low][i] = vertexTest[i];
		}
	}
	delete[] vertexTest;
	return STDtest;
	
}


