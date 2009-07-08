/*
 *  CappiGrid.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/29/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "CappiGrid.h"
#include "Message.h"
#include <math.h>
#include <QTextStream>
#include <QFile>
#include <QDir>

CappiGrid::CappiGrid() 
  : GriddedData()
{

  //  coordSystem = cartesian; outdated -LM
  iDim = jDim = kDim = 0;
  iGridsp = jGridsp = kGridsp = 0.0;
  
  // To make the cappi bigger but still compute it in a reasonable amount of time,
  // skip the reflectivity grid, otherwise set this to true
  gridReflectivity = false;
  exitNow = NULL;
}

CappiGrid::~CappiGrid()
{
}


void CappiGrid::gridRadarData(RadarData *radarData, QDomElement cappiConfig,
					float *vortexLat, float *vortexLon)
{
  // Message::toScreen("IN CAPPI GRID DATA");

  // Create local exit file
  bool abort = returnExitNow();

  // Set the output file
  QString cappiPath = cappiConfig.firstChildElement("dir").text();
  QString cappiFile = radarData->getDateTimeString();
  cappiFile.replace(QString(":"),QString("_"));
  outFileName = cappiPath + "/" + cappiFile;

  //testing Message::toScreen("OutputFile = "+outFileName);
  
  // Get the dimensions from the configuration 
  // all dimensions are in units of cappi grid points
  iDim = cappiConfig.firstChildElement("xdim").text().toFloat();
  jDim = cappiConfig.firstChildElement("ydim").text().toFloat();
  kDim = cappiConfig.firstChildElement("zdim").text().toFloat();
  // all grid spacings are in units of km
  iGridsp = cappiConfig.firstChildElement("xgridsp").text().toFloat();
  jGridsp = cappiConfig.firstChildElement("ygridsp").text().toFloat();
  kGridsp = cappiConfig.firstChildElement("zgridsp").text().toFloat();


  // Get the relative center and expand the grid around it
  // relDist = new float[2];
  
  //Message::toScreen("In cappi vortexLat = "+QString().setNum(*vortexLat)+" vortexLon = "+QString().setNum(*vortexLon));

  //Message::toScreen("radarLat = "+QString().setNum(*radarData->getRadarLat())+" radarLon = "+QString().setNum(*radarData->getRadarLon()));
  
  // Should this be get cartesian point? Don't we use the grid spacing
  // in that calculation? -LM 6/11/07
  relDist = getCartesianPoint(radarData->getRadarLat(), 
			      radarData->getRadarLon(),
			      vortexLat, vortexLon);
  //Message::toScreen("Difference in x = "+QString().setNum(relDist[0])+" Difference in y = "+QString().setNum(relDist[1]));
  
  /*
    The old way of doing things....
    
    float vXDistance =  iDim/2*iGridsp;
    float vYDistance =  jDim/2*jGridsp;
    setZeroLocation(vortexLat, vortexLon,&vXDistance,&vYDistance);
    latReference = *vortexLat;
    lonReference = *vortexLon;
  */
  
  float rXDistance =  0;
  float rYDistance =  0;

  // connects the (0,0) point on the cappi grid, to the latitude and
  // longitude of the radar.
  
  setLatLonOrigin(radarData->getRadarLat(), radarData->getRadarLon(),
		  &rXDistance,&rYDistance);
  
  // Defines iteration indexes for cappi grid
  
  xmin = nearbyintf(relDist[0] - (iDim/2)*iGridsp);
  xmax = nearbyintf(relDist[0] + (iDim/2)*iGridsp);
  ymin = nearbyintf(relDist[1] - (jDim/2)*jGridsp);
  ymax = nearbyintf(relDist[1] + (jDim/2)*jGridsp);
  
  //  Message::toScreen("Xmin = "+QString().setNum(xmin)+" Xmax = "+QString().setNum(xmax)+" Ymin = "+QString().setNum(ymin)+" Ymax = "+QString().setNum(ymax));
  
  /* Adjust the cappi so that it doesn't waste space on areas without velocity data
  if (xmin < -180) {
    float adjust = -xmin - 180;
    xmin += adjust;
    xmax += adjust;
  } else if (xmax > 180) {
    float adjust = -xmax + 180;
    xmin += adjust;
    xmax += adjust;
  }
  if (ymin < -180) {
    float adjust = -ymin - 180;
    ymin += adjust;
    ymax += adjust;
  } else if (ymax > 180) {
    float adjust = -ymax + 180;
    ymin += adjust;
    ymax += adjust;
  }*/
  
  latReference = *radarData->getRadarLat();
  lonReference = *radarData->getRadarLon();
  
  // Message::toScreen("Xmin = "+QString().setNum(xmin)+" Xmax = "+QString().setNum(xmax)+" Ymin = "+QString().setNum(ymin)+" Ymax = "+QString().setNum(ymax));
  
  /* Changed to be hardcoded absolute minimum zmin at 1 km for array purposes M. Bell
     float distance = sqrt(relDist[0] * relDist[0] + relDist[1] * relDist[1]);
     float beamHeight = radarData->radarBeamHeight(distance, radarData->getSweep(0)->getElevation());
     zmin = (float(int(beamHeight/kGridsp)))*kGridsp;
     zmax = zmin + kDim*kGridsp;
  */
  zmin = 1.0;
  zmax = zmin + kDim*kGridsp;
  
  delete[] relDist;
  
  if(abort){
	  //Message::toScreen("ExitNow in Cressmand Interpolation in CappiGrid");
	  return;
  }
  
  // Interpolate the data depending on method chosen
  QString interpolation = cappiConfig.firstChildElement("interpolation").text();
  //Message::toScreen("Using "+interpolation+" interpolation ");
  if (interpolation == "cressman") {
    CressmanInterpolation(radarData);
  }
   /* else if (interpolation == "barnes") {
    BarnesInterpolation();
  }  else if (interpolation == "closestpoint") {
	  ClosestPointInterpolation();
  } else if (interpolation == "bilinear") {
	  BilinearInterpolation(radarData);
  } */
  
  // Set the initial field names
  fieldNames << "DZ" << "VE" << "SW";
  
  //Message::toScreen("Completed Cappi Process");
  
}

/*
void sift_down(goodVel &sortarray, const int l, const int r)
{
	int j, jold;
	float a;
	
	a = sortarray[l];
	jold = l;
	j=2*l+1;
	while (j <= r) {
		if (j < r && sortarray[j] < sortarray[j+1]) j++;
		if (a >= sortarray[j]) break;
		sortarray[jold] = sortarray[j];
		jold = j;
		j = 2*j+1;
	}
	sortarray[jold] = a;
}
*/

void CappiGrid::CressmanInterpolation(RadarData *radarData)
{

  // Cressman Interpolation

  // Create local abort variable
  bool abort = returnExitNow();
  
  // Calculate radius of influence
  float hROI = 1.0;
  //float vROI = 2.0;
  float xRadius = (iGridsp * iGridsp) * (hROI*hROI);
  float yRadius = (jGridsp * jGridsp) * (hROI*hROI);
  //float zRadius = (kGridsp * kGridsp) * (vROI*vROI);
	float RSquare = xRadius + yRadius; // + zRadius;
  int maxIplus = (int)(RSquare/iGridsp);
  int maxJplus = (int)(RSquare/jGridsp);
  //int maxKplus = (int)(RSquare/kGridsp);
  
  // Initialize weights
  for (int k = 0; k < int(kDim); k++) { 
	  for (int j = 0; j < int(jDim); j++) {
		  abort = returnExitNow();
		  if(abort){
			  //Message::toScreen("ExitNow in Cressmand Interpolation in CappiGrid");
			  return;
		  }
		  for (int i = 0; i < int(iDim); i++) {
			  refValues[i][j][k].sumRef = 0;
			  refValues[i][j][k].weight = 0;
			  velValues[i][j][k].sumVel = 0;
			  velValues[i][j][k].sumSw = 0;
			  velValues[i][j][k].weight = 0;
		  }
	  }
  }
  
  // Find the maximum unambiguous range for the volume
  float maxUnambig_range = 0;
  float maxNyquist = 0;
  for (int n = 0; n < radarData->getNumSweeps(); n++) {
	  Sweep* currentSweep = radarData->getSweep(n);
	  float range = currentSweep->getUnambig_range() * 2;
	  if ((currentSweep->getVel_numgates() > 0) 
		and (range > maxUnambig_range)
		and (currentSweep->getElevation() < 1.0)) {
		  maxUnambig_range = range;
	  }
	  float nyquist= currentSweep->getNyquist_vel();
	  if ((currentSweep->getVel_numgates() > 0)
		  and (nyquist > maxNyquist)) maxNyquist = nyquist;
  }
  
  // Find good values
  for (int n = 0; n < radarData->getNumRays(); n++) {
	  Ray* currentRay = radarData->getRay(n);
	  float theta = deg2rad * fmodf((450. - currentRay->getAzimuth()),360.);
	  float phi = deg2rad * (90. - (currentRay->getElevation()));
	  
	  /* if ((currentRay->getRef_numgates() > 0) and 
		  (gridReflectivity)) {
		  
		  float* refData = currentRay->getRefData();
		  for (int g = 0; g <= (currentRay->getRef_numgates()-1); g++) {
			  if (refData[g] == -999.) { continue; }
			  float range = float(currentRay->getFirst_ref_gate() +
								  (g * currentRay->getRef_gatesp()))/1000.;
			  
			  float x = range*sin(phi)*cos(theta);
			  if ((x < (xmin - iGridsp)) or x > (xmax + iGridsp)) { continue; }
			  float y = range*sin(phi)*sin(theta);
			  if ((y < (ymin - jGridsp)) or y > (ymax + jGridsp)) { continue; }
			  float z = radarData->radarBeamHeight(range,
												   currentRay->getElevation() );
			  if ((z < (zmin - kGridsp)) or z > (zmax + kGridsp)) { continue; }

			  // Looks like a good point, find its closest Cartesian index 
			  float i = (x - xmin)/iGridsp;
			  float j = (y - ymin)/jGridsp;
			  float k = (z - zmin)/kGridsp;
			  float RSquareLinear = RSquare*range*range / 30276.0;
			  for (int kplus = -maxKplus; kplus <= maxKplus; kplus++) { 
				  for (int jplus = -maxJplus; jplus <= maxJplus; jplus++) {
					  for (int iplus = -maxIplus; iplus <= maxIplus; iplus++) {
						  int iIndex = (int)(i+iplus);
						  int jIndex = (int)(j+jplus);
						  int kIndex = (int)(k+kplus);
						  if ((iIndex < 0) or (iIndex > (int)iDim)) { continue; }
						  if ((jIndex < 0) or (jIndex > (int)jDim)) { continue; }
						  if ((kIndex < 0) or (kIndex > (int)kDim)) { continue; }
							  
						  float dx = (i - (int)(i+iplus))*iGridsp;
						  float dy = (j - (int)(j+jplus))*jGridsp;
						  float dz = (k - (int)(k+kplus))*kGridsp;
						  float rSquare = (dx*dx) + (dy*dy) + (dz*dz);
						  if (rSquare > RSquareLinear) { continue; }
						  float weight = (RSquareLinear - rSquare) / (RSquareLinear + rSquare);
						  refValues[iIndex][jIndex][kIndex].weight += weight;
						  refValues[iIndex][jIndex][kIndex].sumRef += weight*refData[g];
					  }
				  }
			  }
		  } 
		  
	  } */
	  if ((currentRay->getVel_numgates() > 0)
		  // Just grab the lowest elevation sweeps
		  //and (currentRay->getElevation() < 0.75)
		  and (fabs(currentRay->getNyquist_vel() - maxNyquist) < 0.1)) {
		  float* velData = currentRay->getVelData();
		  float* swData = currentRay->getSwData();
		  float nyquist = currentRay->getNyquist_vel();
		  for (int g = 0; g <= (currentRay->getVel_numgates()-1); g++) {
			  if (velData[g] == -999.) { continue; }
			  
			  float range = float(currentRay->getFirst_vel_gate() +
								  (g * currentRay->getVel_gatesp()))/1000.;
			  float x = range*sin(phi)*cos(theta);
			  if ((x < (xmin - iGridsp)) or x > (xmax + iGridsp)) { continue; }
			  float y = range*sin(phi)*sin(theta);
			  if ((y < (ymin - jGridsp)) or y > (ymax + jGridsp)) { continue; }
			  float z = radarData->radarBeamHeight(range,
												   currentRay->getElevation() );
			  if ((z < (zmin - kGridsp)) or z > (zmax + kGridsp)) { continue; }
			  
			  // Looks like a good point, find its closest Cartesian index 
			  float i = (x - xmin)/iGridsp;
			  float j = (y - ymin)/jGridsp;
			  float RSquareLinear = RSquare; //* range*range / 30276.0;
			  for (int jplus = -maxJplus; jplus <= maxJplus; jplus++) {
				  for (int iplus = -maxIplus; iplus <= maxIplus; iplus++) {
					  int iIndex = (int)(i+iplus);
					  int jIndex = (int)(j+jplus);
					  //int kIndex = (int)(k+kplus);
					  // Hard-code single level only
					  int kIndex = 0;
					  if ((iIndex < 0) or (iIndex >= (int)iDim)) { continue; }
					  if ((jIndex < 0) or (jIndex >= (int)jDim)) { continue; }
					  if ((kIndex < 0) or (kIndex >= (int)kDim)) { continue; }
					  
					  float dx = (i - (int)(i+iplus))*iGridsp;
					  float dy = (j - (int)(j+jplus))*jGridsp;
					  //float dz = (k - (int)(k+kplus))*kGridsp;
					  float rSquare = (dx*dx) + (dy*dy); // + (dz*dz);
					  if (rSquare > RSquareLinear) { continue; }
					  float weight = (100*nyquist) *(RSquareLinear - rSquare) / (RSquareLinear + rSquare);
					  velValues[iIndex][jIndex][kIndex].weight += weight;
					  velValues[iIndex][jIndex][kIndex].sumVel += weight*velData[g];
					  velValues[iIndex][jIndex][kIndex].sumSw += weight*swData[g];
				  }
			  }
		  }			  
		  
		  velData = NULL;
		  swData = NULL;
		  //delete velData;
		  //delete swData;
	  }
	  currentRay = NULL;
	  //delete currentRay;

	  abort = returnExitNow();
	  if(abort){
		  //Message::toScreen("exitNow in GridRadarData in CappiGrid");
		  return;
	  }
  }
  
  //Message::toScreen("# of Reflectivity gates used in CAPPI = "+QString().setNum(r));
  //Message::toScreen("# of Velocity gates used in CAPPI = "+QString().setNum(v));
  	
	for (int k = 0; k < int(kDim); k++) { 
		for (int j = 0; j < int(jDim); j++) {
			abort = returnExitNow();
			if(abort){
				//Message::toScreen("ExitNow in Cressmand Interpolation in CappiGrid");
				return;
			}
			for (int i = 0; i < int(iDim); i++) {
				
				dataGrid[0][i][j][k] = -999;
				dataGrid[1][i][j][k] = -999;
				dataGrid[2][i][j][k] = -999;
				
				if (refValues[i][j][k].weight > 0) {
					dataGrid[0][i][j][k] = refValues[i][j][k].sumRef/refValues[i][j][k].weight;
				}
				if (velValues[i][j][k].weight > 0) {
					dataGrid[1][i][j][k] = velValues[i][j][k].sumVel/velValues[i][j][k].weight;
					dataGrid[2][i][j][k] = velValues[i][j][k].sumSw/velValues[i][j][k].weight;
				}
				velValues[i][j][k].sumVel = 0;
				velValues[i][j][k].sumSw = 0;
				velValues[i][j][k].weight = 0;				
			}
		}
	}
	
	int maxfoldpasses = 1;
	int localArea = 10;
	for (int foldpass = 0; foldpass < maxfoldpasses; foldpass++) {
		// Find good values
		for (int n = 0; n < radarData->getNumRays(); n++) {
			Ray* currentRay = radarData->getRay(n);
			float theta = deg2rad * fmodf((450. - currentRay->getAzimuth()),360.);
			float phi = deg2rad * (90. - (currentRay->getElevation()));
			
			if ((currentRay->getVel_numgates() > 0)
				// Just grab the lowest elevation sweeps & try to adjust bad folds
				and (currentRay->getElevation() < 0.75)) {
				float* velData = currentRay->getVelData();
				float* swData = currentRay->getSwData();
				float nyquist = currentRay->getNyquist_vel();
				for (int g = 0; g <= (currentRay->getVel_numgates()-1); g++) {
					if (velData[g] == -999.) { continue; }
					
					float range = float(currentRay->getFirst_vel_gate() +
										(g * currentRay->getVel_gatesp()))/1000.;
					float x = range*sin(phi)*cos(theta);
					if ((x < (xmin - iGridsp)) or x > (xmax + iGridsp)) { continue; }
					float y = range*sin(phi)*sin(theta);
					if ((y < (ymin - jGridsp)) or y > (ymax + jGridsp)) { continue; }
					float z = radarData->radarBeamHeight(range,
														 currentRay->getElevation() );
					if ((z < (zmin - kGridsp)) or z > (zmax + kGridsp)) { continue; }
					
					// Looks like a good point, find its closest Cartesian index 
					float i = (x - xmin)/iGridsp;
					float j = (y - ymin)/jGridsp;
					float RSquareLinear = RSquare; //* range*range / 30276.0;
					for (int jplus = -maxJplus; jplus <= maxJplus; jplus++) {
						for (int iplus = -maxIplus; iplus <= maxIplus; iplus++) {
							int iIndex = (int)(i+iplus);
							int jIndex = (int)(j+jplus);
							//int kIndex = (int)(k+kplus);
							// Hard-code single level only
							int kIndex = 0;
							if ((iIndex < 0) or (iIndex >= (int)iDim)) { continue; }
							if ((jIndex < 0) or (jIndex >= (int)jDim)) { continue; }
							if ((kIndex < 0) or (kIndex >= (int)kDim)) { continue; }
							
							float dx = (i - (int)(i+iplus))*iGridsp;
							float dy = (j - (int)(j+jplus))*jGridsp;
							float rSquare = (dx*dx) + (dy*dy); // + (dz*dz);
							if (rSquare > RSquareLinear) { continue; }
							float avgCappi = 0;
							float quadcount = 0;
							for (int quadi = iIndex-localArea; quadi <= iIndex+localArea; quadi++) {
								for (int quadj = jIndex-localArea; quadj <= jIndex+localArea; quadj++) {
									if ((quadi < 0) or (quadi >= (int)iDim)) { continue; }
									if ((quadj < 0) or (quadj >= (int)jDim)) { continue; }
									if (dataGrid[1][quadi][quadj][kIndex] != -999) {
										avgCappi += dataGrid[1][quadi][quadj][kIndex];
										quadcount++;
									}
								}
							}
							int minfold = 0;
							if (quadcount != 0) { // Need at least one seed from the higher nyquist, otherwise use original
								avgCappi /= quadcount;
								float velDiff = velData[g] - avgCappi;
								if (fabs(velDiff) > nyquist) {
									// Potential folding problem
									float mindiff = 999999;							
									for (int fold=-2; fold <=2; fold++) {
										velDiff = velData[g]+2*fold*nyquist - avgCappi;
										if (fabs(velDiff) < mindiff) {
											mindiff = fabs(velDiff);
											minfold = fold;
										}
									}
								}
							}
							velData[g] += 2*minfold*nyquist;
							float newVel = velData[g];
							float weight = (100*nyquist) * (RSquareLinear - rSquare) / (RSquareLinear + rSquare);
							velValues[iIndex][jIndex][kIndex].weight += weight;
							velValues[iIndex][jIndex][kIndex].sumVel += weight*newVel;
						}
					}
				}			  
				
				velData = NULL;
				swData = NULL;
				//delete velData;
				//delete swData;
			}
			currentRay = NULL;
			//delete currentRay;
			
			abort = returnExitNow();
			if(abort){
				//Message::toScreen("exitNow in GridRadarData in CappiGrid");
				return;
			}
		}
		
		/* 2nd round of BB?
		Ray* currentRay = NULL;
		float velNull = -999.;
		int numVGatesAveraged = 15;
		int maxFold = 4;
		float numRays = radarData->getNumRays();
		for(int i = 0; i < numRays; i++) 
		{
			currentRay = radarData->getRay(i);
			float *vGates = currentRay->getVelData();
			float startVelocity = velNull;
			for(int j = 0; j < currentRay->getVel_numgates(); j++) {
				if (vGates[j] != velNull) {
					startVelocity = vGates[j];
					break;
				}
			}
			float nyquistVelocity = currentRay->getNyquist_vel();
			int numVelocityGates = currentRay->getVel_numgates();
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
			vGates = NULL;
			currentRay = NULL;
		} */
		
		for (int k = 0; k < int(kDim); k++) { 
			for (int j = 0; j < int(jDim); j++) {
				abort = returnExitNow();
				if(abort){
					//Message::toScreen("ExitNow in Cressmand Interpolation in CappiGrid");
					return;
				}
				for (int i = 0; i < int(iDim); i++) {
					dataGrid[1][i][j][k] = -999;
					if (velValues[i][j][k].weight > 0) {
						dataGrid[1][i][j][k] = velValues[i][j][k].sumVel/velValues[i][j][k].weight;
					}
					velValues[i][j][k].sumVel = 0;
					velValues[i][j][k].weight = 0;								
				}
			}
		}
	}
	
	// Smooth local outliers
	for (int k = 0; k < int(kDim); k++) { 
		// float sumtexture = 0;
		// float maxtexture = 0;
		for (int j = 1; j < int(jDim)-1; j++) {
			abort = returnExitNow();
			if(abort){
				//Message::toScreen("ExitNow in Cressmand Interpolation in CappiGrid");
				return;
			}
			for (int i = 1; i < int(iDim)-1; i++) {
				float avgCappi = 0;
				float quadcount = 0;
				for (int quadi = i-localArea; quadi <= i+localArea; quadi++) {
					for (int quadj = j-localArea; quadj <= j+localArea; quadj++) {
						if ((quadi < 0) or (quadi >= (int)iDim)) { continue; }
						if ((quadj < 0) or (quadj >= (int)jDim)) { continue; }
						if (dataGrid[1][quadi][quadj][k] != -999) {
							avgCappi += dataGrid[1][quadi][quadj][k];
							quadcount++;
						}
					}
				}
				if (quadcount != 0) {
					avgCappi /= quadcount;
					float stdVel = 0;
					for (int quadi = i-localArea; quadi <= i+localArea; quadi++) {
						for (int quadj = j-localArea; quadj <= j+localArea; quadj++) {
							if ((quadi < 0) or (quadi >= (int)iDim)) { continue; }
							if ((quadj < 0) or (quadj >= (int)jDim)) { continue; }
							if (dataGrid[1][quadi][quadj][k] != -999) {
								stdVel += (dataGrid[1][quadi][quadj][k]-avgCappi)*
									(dataGrid[1][quadi][quadj][k]-avgCappi);
							}
						}
					}
					stdVel = sqrt(stdVel/quadcount);
					float diffCappi = fabs(dataGrid[1][i][j][k] - avgCappi);
					if ((diffCappi > stdVel*2) and (dataGrid[1][i][j][k] != -999)) {
						dataGrid[1][i][j][k] =avgCappi;
					}
				}
			}
		}
	}

	/* Remove global outliers
	for (int k = 0; k < int(kDim); k++) { 
		// float sumtexture = 0;
		// float maxtexture = 0;
		float posCappi = 0;
		float poscount = 0;
		float negCappi = 0;
		float negcount = 0;
		for (int j = 1; j < int(jDim)-1; j++) {
			abort = returnExitNow();
			if(abort){
				//Message::toScreen("ExitNow in Cressmand Interpolation in CappiGrid");
				return;
			}
			for (int i = 1; i < int(iDim)-1; i++) {
				if (dataGrid[1][i][j][k] != -999) {
					if (dataGrid[1][i][j][k] > 0) {
						posCappi += dataGrid[1][i][j][k];
						QString pos;
						poscount++;
					} else {
						negCappi += dataGrid[1][i][j][k];
						negcount++;
					}
				}
			}
		}
		if (poscount != 0) {
			posCappi /= poscount;
			float stdVel = 0;
			for (int j = 1; j < int(jDim)-1; j++) {
				abort = returnExitNow();
				if(abort){
					//Message::toScreen("ExitNow in Cressmand Interpolation in CappiGrid");
					return;
				}
				for (int i = 1; i < int(iDim)-1; i++) {				
					if ((dataGrid[1][i][j][k] != -999) and (dataGrid[1][i][j][k] > 0)) {
						stdVel += (dataGrid[1][i][j][k]-posCappi)*
						(dataGrid[1][i][j][k]-posCappi);
					}
				}
			}
			stdVel = sqrt(stdVel/poscount);
			for (int j = 1; j < int(jDim)-1; j++) {
				abort = returnExitNow();
				if(abort){
					//Message::toScreen("ExitNow in Cressmand Interpolation in CappiGrid");
					return;
				}
				for (int i = 1; i < int(iDim)-1; i++) {
					float diffCappi = fabs(dataGrid[1][i][j][k] - posCappi);
					if ((diffCappi > stdVel*2) and (dataGrid[1][i][j][k] != -999)
						and (dataGrid[1][i][j][k] > 0)) {
						dataGrid[1][i][j][k] = -999;
					}
				}
			}
		}
		if (negcount != 0) {
			negCappi /= negcount;
			float stdVel = 0;
			for (int j = 1; j < int(jDim)-1; j++) {
				abort = returnExitNow();
				if(abort){
					//Message::toScreen("ExitNow in Cressmand Interpolation in CappiGrid");
					return;
				}
				for (int i = 1; i < int(iDim)-1; i++) {				
					if ((dataGrid[1][i][j][k] != -999) and (dataGrid[1][i][j][k] < 0)) {
						stdVel += (dataGrid[1][i][j][k]-negCappi)*
						(dataGrid[1][i][j][k]-negCappi);
					}
				}
			}
			stdVel = sqrt(stdVel/negcount);
			for (int j = 1; j < int(jDim)-1; j++) {
				abort = returnExitNow();
				if(abort){
					//Message::toScreen("ExitNow in Cressmand Interpolation in CappiGrid");
					return;
				}
				for (int i = 1; i < int(iDim)-1; i++) {		
					float diffCappi = fabs(dataGrid[1][i][j][k] - negCappi);
					if ((diffCappi > stdVel*2) and (dataGrid[1][i][j][k] != -999)
						and (dataGrid[1][i][j][k] < 0)) {
						dataGrid[1][i][j][k] = -999;
					}
				}
			}
		} 
	} */

	
}
/*
void CappiGrid::ClosestPointInterpolation()
{
	
	// Closest Point Interpolation
	
	// Calculate radius of influence 
	float xRadius = (iGridsp * iGridsp);
	float yRadius = (jGridsp * jGridsp);
	float zRadius = (kGridsp * kGridsp);
	float gridsp = xRadius + yRadius + zRadius;	
	
	// Create local abort variable
	bool abort = returnExitNow();
	
	for (int k = 0; k < int(kDim); k++) { 
		for (int j = 0; j < int(jDim); j++) {
			abort = returnExitNow();
			if(abort){
				//Message::toScreen("ExitNow in Cressmand Interpolation in CappiGrid");
				return;
			}
			for (int i = 0; i < int(iDim); i++) {
				
				dataGrid[0][i][j][k] = -999.;
				dataGrid[1][i][j][k] = -999.;
				dataGrid[2][i][j][k] = -999.;
				
				float minR = sqrt(iDim*iGridsp*iDim*iGridsp + jDim*jGridsp*jDim*jGridsp);

				for (long n = 0; n <= maxRefIndex; n++) {
					float dx = refValues[n].x - (xmin + i*iGridsp);
					float dy = refValues[n].y - (ymin + j*jGridsp);
					float dz = refValues[n].z - (zmin + k*kGridsp);
					float r = sqrt((dx*dx) + (dy*dy) + (dz*dz));
					if (r > gridsp) { continue; }
					if (r < minR) {
						minR = r;
						dataGrid[0][i][j][k] = refValues[n].refValue;
					}
					if (minR < gridsp/10) {
						// Close enough
						break;
					}
				}
				
				minR = sqrt(iDim*iGridsp*iDim*iGridsp + jDim*jGridsp*jDim*jGridsp);
				for (long n = 0; n <= maxVelIndex; n++) {
					float dx = velValues[n].x - (xmin + i*iGridsp);
					float dy = velValues[n].y - (ymin + j*jGridsp);
					float dz = velValues[n].z - (zmin + k*kGridsp);
					float r = sqrt((dx*dx) + (dy*dy) + (dz*dz));
					if (r > gridsp) { continue; }
					if (r < minR) {
						minR = r;
						dataGrid[1][i][j][k] = velValues[n].velValue;
						dataGrid[2][i][j][k] = velValues[n].swValue;
					}
					if (minR < gridsp/3) {
						// Close enough
						break;
					}
					
				}
			}
		}
	}
}

void CappiGrid::BilinearInterpolation(RadarData *radarData)
{
	
	// Bilinear Interpolation (see Mohr and Vaughn, 1979 for details)

	// Create local abort variable
	bool abort = returnExitNow();

	for (int k = 0; k < int(kDim); k++) { 
		for (int j = 0; j < int(jDim); j++) {
			abort = returnExitNow();
			if(abort){
				//Message::toScreen("ExitNow in Cressmand Interpolation in CappiGrid");
				return;
			}
			for (int i = 0; i < int(iDim); i++) {
				
				dataGrid[0][i][j][k] = -999.;
				dataGrid[1][i][j][k] = -999.;
				dataGrid[2][i][j][k] = -999.;
				
				float x = xmin + i*iGridsp;
				float y = ymin + j*jGridsp;
				float z = zmin + k*kGridsp;
				float gRange = sqrt(x*x + y*y +z*z);
				float gTheta = fixAngle(atan2(y,x))*rad2deg;
				float gPhi = fixAngle(atan2(sqrt(x*x + y*y),z))*rad2deg;
				float rPhi1, rPhi2;
				for (int n = 0; n < radarData->getNumSweeps()-1; n++) {
					Sweep* currentSweep = radarData->getSweep(n);
					Sweep* nextSweep = radarData->getSweep(n+1);
					rPhi1 = deg2rad * (90. - (currentSweep->getElevation()));
					rPhi2 = deg2rad * (90. - (nextSweep->getElevation()));
					if ((rPhi1 < gPhi) and (rPhi2 >= gPhi)) { break; }
				}
				
				for (int n = 0; n < radarData->getNumRays(); n++) {
					Ray* currentRay = radarData->getRay(n);
					float rTheta = deg2rad * fmodf((450. - currentRay->getAzimuth()),360.);
					float rPhi = deg2rad * (90. - (currentRay->getElevation()));
					
					
					if ((currentRay->getRef_numgates() > 0) and 
						(gridReflectivity)) {
						
						float* refData = currentRay->getRefData();
						for (int g = 0; g <= (currentRay->getRef_numgates()-1); g++) {
							if (refData[g] == -999.) { continue; }
							float range = float(currentRay->getFirst_ref_gate() +
												(g * currentRay->getRef_gatesp()))/1000.;
							
			}
		}
	}
}

void CappiGrid::BarnesInterpolation()
{

  // Barnes Interpolation (see Koch et al, 1983 for details)

  // Barnes Interpolation does not seem to be working right now -LM

  float falloff_x = 5.052*pow((4* iGridsp / Pi),2);
  float falloff_y = 5.052*pow((4* jGridsp / Pi),2);
  float falloff_z = 5.052*pow((4* kGridsp / Pi),2);
  float smoother = 0.3;


  for (int k = 0; k < int(kDim); k++) { 
    for (int j = 0; j < int(jDim); j++) {
      for (int i = 0; i < int(iDim); i++) {

	dataGrid[0][i][j][k] = -999.;
	dataGrid[1][i][j][k] = -999.;
	dataGrid[2][i][j][k] = -999.;

	float sumRef = 0;
	float sumVel = 0;
	float sumSw = 0;
	float refWeight = 0;
	float velWeight = 0;

	if(gridReflectivity) {
	  for (int n = 0; n <= maxRefIndex; n++) {
	    float dx = refValues[n].x - (xmin + i*iGridsp);
	    if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }
	    
	    float dy = refValues[n].y - (ymin + j*jGridsp);
	    if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }
	    
	    float dz = refValues[n].z - (zmin + k*kGridsp);
	    if (fabs(dz) > sqrt(20 * falloff_z)) { continue; }
	    
	    float weight = exp(-(dx*dx)/falloff_x 
			       -(dy*dy)/falloff_y
			       -(dz*dz)/falloff_z);
	    
	    refWeight += weight;
	    sumRef += weight*refValues[n].refValue;
	  }
	}

	for (int n = 0; n <= maxVelIndex; n++) {
	  float dx = velValues[n].x - (xmin + i*iGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = velValues[n].y - (ymin + j*jGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = velValues[n].z - (zmin + k*kGridsp);
	  if (fabs(dz) > sqrt(20 * falloff_z)) { continue; }
	  
	  float weight = exp(-(dx*dx)/falloff_x 
			     -(dy*dy)/falloff_y
			     -(dz*dz)/falloff_z);

	  velWeight += weight;
	  sumVel += weight*velValues[n].velValue;
	  sumSw += weight*velValues[n].swValue;

	}

	if (refWeight > 0) {
	  dataGrid[0][i][j][k] = sumRef/refWeight;
	}
	if (velWeight > 0) {
	  dataGrid[1][i][j][k] = sumVel/velWeight;
	  dataGrid[2][i][j][k] = sumSw/velWeight;
	}	  
      }
    }
  }

  for (int k = 0; k < int(kDim); k++) {
    for (int j = 0; j < int(jDim); j++) {
      for (int i = 0; i < int(iDim); i++) {

	float sumRef = 0;
	float sumVel = 0;
	float sumSw = 0;
	float refWeight = 0;
	float velWeight = 0;

	if(gridReflectivity) {
	  for (int n = 0; n <= maxRefIndex; n++) {
	    
	    float dx = refValues[n].x - (xmin + i*iGridsp);
	    if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }
	    
	    float dy = refValues[n].y - (ymin + j*jGridsp);
	    if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }
	    
	    float dz = refValues[n].z - (zmin + k*kGridsp);
	    if (fabs(dz) > sqrt(20 * falloff_z)) { continue; }
	    
	    float weight = exp(-(dx*dx)/(falloff_x*smoother)
			       -(dy*dy)/(falloff_y*smoother)
			       -(dz*dz)/(falloff_z*smoother));
	    
	    
	    refWeight += weight;
	    float interpRef = trilinear(dx,dy,dz,0);
	    sumRef += weight*(refValues[n].refValue - interpRef);
	  }
	}
	for (int n = 0; n <= maxVelIndex; n++) {
	  float dx = velValues[n].x - (xmin + i*iGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = velValues[n].y - (ymin + j*jGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = velValues[n].z - (zmin + k*kGridsp);
	  if (fabs(dz) > sqrt(20 * falloff_z)) { continue; }
	  
	  float weight = exp(-(dx*dx)/falloff_x 
			     -(dy*dy)/falloff_y
			     -(dz*dz)/falloff_z);

	  
	  velWeight += weight;
	  float interpVel = trilinear(dx,dy,dz,1);
	  float interpSw = trilinear(dx,dy,dz,2);
	  sumVel += weight*(velValues[n].velValue - interpVel);
	  sumSw += weight*(velValues[n].swValue - interpSw);

	}
    
	if (refWeight > 0) {
	  dataGrid[0][i][j][k] += sumRef/refWeight;
	}
	if (velWeight > 0) {
	  dataGrid[1][i][j][k] += sumVel/velWeight;
	  dataGrid[2][i][j][k] += sumSw/velWeight;
	}	  	
      }
    }
  }
  Message::toScreen("Leaving Barnes");
}



float CappiGrid::trilinear(const float &x, const float &y,
			      const float &z, const int &param)
{

  // Do a bilinear interpolation from the nearest gridpoints
  int x0 = int((float(int(x/iGridsp))*iGridsp - xmin)/iGridsp);
  int y0 = int((float(int(y/jGridsp))*jGridsp - ymin)/jGridsp);
  int z0 = int((float(int(z/kGridsp))*kGridsp - zmin)/kGridsp);
  float dx = (x/iGridsp) - int(x/iGridsp);
  float dy = (y/jGridsp) - int(y/jGridsp);
  float dz = (z/kGridsp) - int(z/kGridsp);
  float omdx = 1 - dx;
  float omdy = 1 - dy;
  float omdz = 1 - dz;
  int x1 = 0;
  int y1 = 0;
  int z1 = 0;

  if (x0 < 0) {
    x0 = x1 = 0;
  }

  if (y0 < 0) {
    y0 = y1 = 0;
  }

  if (z0 < 0) {
    z0 = z1 = 0;
  }


  if (x0 >= iDim-1) {
    x1 = x0;
  } else {
    x1 = x0 + 1;
  }

  if (y0 >= jDim-1) {
    y1 = y0;
  } else {
    y1 = y0 + 1;
  }

  if (z0 >= kDim-1) {
    z1 = z0;
  } else {
    z1 = z0 + 1;
  }

  float interpValue = 0;
  if (dataGrid[param][x0][y0][z0] != -999) {
    interpValue += omdx*omdy*omdz*dataGrid[param][x0][y0][z0];
  }
  if (dataGrid[param][x0][y1][z0] != -999) {
    interpValue += omdx*dy*omdz*dataGrid[param][x0][y1][z0];
  }
  if (dataGrid[param][x1][y0][z0] != -999) {
    interpValue += dx*omdy*omdz*dataGrid[param][x1][y0][z0];
  }
  if (dataGrid[param][x1][y1][z0] != -999) {
    interpValue += dx*dy*omdz*dataGrid[param][x1][y1][z0];
  }
  if (dataGrid[param][x0][y0][z1] != -999) {
    interpValue += omdx*omdy*dz*dataGrid[param][x0][y0][z1];
  }
  if (dataGrid[param][x0][y1][z1] != -999) {
    interpValue += omdx*dy*dz*dataGrid[param][x0][y1][z1];
  }
  if (dataGrid[param][x1][y0][z1] != -999) {
    interpValue += dx*omdy*dz*dataGrid[param][x1][y0][z1];
  }
  if (dataGrid[param][x1][y1][z1] != -999) {
    interpValue += dx*dy*dz*dataGrid[param][x1][y1][z1];
  }

  return interpValue;
}
*/

void CappiGrid::writeAsi()
{

	// Write out the CAPPI to an asi file
	
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
	id[160] = (int)(xmin * 100);
	id[161] = (int)(xmax * 100);
	id[162] = (int)iDim;
	id[163] = (int)iGridsp * 1000;
	id[164] = 1;
  
	// Y Header
	id[165] = (int)(ymin *  100);
	id[166] = (int)(ymax * 100);
	id[167] = (int)jDim;
	id[168] = (int)jGridsp * 1000;
	id[169] = 2;
  
	// Z Header
	id[170] = (int)(zmin * 1000);
	id[171] = (int)(zmax * 1000);
	id[172] = (int)kDim;
	id[173] = (int)kGridsp * 1000;
	id[174] = 3;

	// Number of radars
	id[303] = 1;
  
	// Index of center
	id[309] = (int)((1 - xmin) * 100);
	id[310] = (int)((1 - ymin) * 100);
	id[311] = 0;
	
	// Write ascii file for grid2ps
	//Message::toScreen("Trying to write cappi to "+outFileName);
	outFileName += ".asi";
	QFile asiFile(outFileName);
	if(!asiFile.open(QIODevice::WriteOnly)) {
		Message::toScreen("Can't open CAPPI file for writing");
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
	for(int k = 0; k < int(kDim); k++) {
		out << reset << "level" << qSetFieldWidth(2) << k+1 << endl;
		for(int j = 0; j < int(jDim); j++) {
			out << reset << "azimuth" << qSetFieldWidth(3) << j+1 << endl;

			for(int n = 0; n < fieldNames.size(); n++) {
				out << reset << left << fieldNames.at(n) << endl;
				int line = 0;
				for (int i = 0; i < int(iDim);  i++){
				    out << reset << qSetRealNumberPrecision(3) << scientific << qSetFieldWidth(10) << dataGrid[n][i][j][k];
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

}     

bool CappiGrid::writeAsi(const QString& fileName)
{
  QString originalOutputFileName = outFileName;
  if(QDir::isAbsolutePath(fileName)) {
    outFileName = fileName;
  }
  else {
    outFileName = QDir::current().filePath(fileName);
  }
  writeAsi();
 
  if(QFile::exists(outFileName)) {
    outFileName = originalOutputFileName;
    return true;
  }
  else {
    outFileName = originalOutputFileName;
    return false;
  }
}
