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

CappiGrid::CappiGrid() : GriddedData()
{

  coordSystem = cartesian;
  iDim = jDim = kDim = 0;
  iGridsp = jGridsp = kGridsp = 0.0;
  
  // To make the cappi bigger but still compute it in a reasonable amount of time,
  // skip the reflectivity grid, otherwise set this to true
  gridReflectivity = false;
  exitNow = false;
}

CappiGrid::~CappiGrid()
{
}


void CappiGrid::gridRadarData(RadarData *radarData, QDomElement cappiConfig,
					float *vortexLat, float *vortexLon)
{
  // Message::toScreen("IN CAPPI GRID DATA");

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
  
  //Message::toScreen("vortexLat = "+QString().setNum(*vortexLat)+" vortexLon = "+QString().setNum(*vortexLon));

  //Message::toScreen("radarLat = "+QString().setNum(*radarData->getRadarLat())+" radarLon = "+QString().setNum(*radarData->getRadarLon()));

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
  
  //Message::toScreen("Xmin = "+QString().setNum(xmin)+" Xmax = "+QString().setNum(xmax)+" Ymin = "+QString().setNum(ymin)+" Ymax = "+QString().setNum(ymax));

  // Adjust the cappi so that it doesn't waste space on areas without velocity data
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
  }
  
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
  
  // Find good values
  long r = 0;
  long v = 0;
  
  for (int n = 0; n < radarData->getNumRays(); n++) {
    Ray* currentRay = radarData->getRay(n);
    float theta = deg2rad * fmodf((450. - currentRay->getAzimuth()),360.);
    float phi = deg2rad * (90. - (currentRay->getElevation()));
    
    if ((currentRay->getRef_numgates() > 0) and 
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

		// Looks like a good point
		refValues[r].refValue = refData[g];
		refValues[r].x = x;
		refValues[r].y = y;
		refValues[r].z = z;
		refValues[r].rg = range;
		refValues[r].az = currentRay->getAzimuth();
		refValues[r].el = currentRay->getElevation();
		/*
		if(r == 888){
		  QString print("r = "+QString().setNum(r));
		  print+=" Range= "+QString().setNum(range);
		  print+=" x= "+QString().setNum(x);
		  print+=" y= "+QString().setNum(y);
		  print+=" z= "+QString().setNum(z);
		  print+=" ref= "+QString().setNum(refData[g]);
		  Message::toScreen(print);
		}
		*/
		r++;
		if (r > 199999)
			Message::toScreen("Gone out of array bounds in CappiGrid.ccp refValues");
      }
      
    }
    if ((currentRay->getVel_numgates() > 0) and
	(currentRay->getUnambig_range() > 170)) {
      
      float* velData = currentRay->getVelData();
      float* swData = currentRay->getSwData();
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
		
		
		// Looks like a good point
		velValues[v].velValue = velData[g];
		velValues[v].swValue = swData[g];
		velValues[v].x = x;
		velValues[v].y = y;
		velValues[v].z = z;
		velValues[v].rg = range;
		velValues[v].az = currentRay->getAzimuth();
		velValues[v].el = currentRay->getElevation();
		/*
		if(v == 888){
		  QString print("v = "+QString().setNum(v));
		  print+=" Range= "+QString().setNum(range);
		  print+=" x= "+QString().setNum(x);
		  print+=" y= "+QString().setNum(y);
		  print+=" z= "+QString().setNum(z);
		  print+=" vel= "+QString().setNum(velData[g]);
		  Message::toScreen(print);
		}
		*/
		v++;
		if(v >999999)
		  Message::toScreen("Gone out of array bounds in CappiGrid.ccp velValues");
      }
      velData = NULL;
      swData = NULL;
      delete velData;
      delete swData;
    }
    currentRay = NULL;
    delete currentRay;
    if(exitNow)
      return;
  }
  Message::toScreen("# of Reflectivity gates used in CAPPI = "+QString().setNum(r));
  Message::toScreen("# of Velocity gates used in CAPPI = "+QString().setNum(v));
  
  // Subtract off one from the count for iterative purposes
  maxRefIndex = r - 1;
  maxVelIndex = v - 1;

  if((maxRefIndex > 199999)||(maxVelIndex >999999))
    Message::toScreen("Gone out of array bounds in CappiGrid.ccp velValues or refValues");
  
  // Interpolate the data depending on method chosen
  QString interpolation = cappiConfig.firstChildElement("interpolation").text();
  Message::toScreen("Using "+interpolation+" interpolation ");
  if (interpolation == "barnes") {
    BarnesInterpolation();
  } else if (interpolation == "cressman") {
    CressmanInterpolation();
  }
  
  // Set the initial field names
  fieldNames << "DZ" << "VE" << "SW";
  
  Message::toScreen("Completed Cappi Process");
  
}

void CappiGrid::CressmanInterpolation()
{

  // Cressman Interpolation
  
  // Calculate radius of influence 
  float xRadius = (iGridsp * iGridsp) * 2.25;
  float yRadius = (jGridsp * jGridsp) * 2.25;
  float zRadius = (kGridsp * kGridsp) * 2.25;
  float RSquare = xRadius + yRadius + zRadius;

  for (int k = 0; k < int(kDim); k++) { 
    for (int j = 0; j < int(jDim); j++) {
      if(exitNow)
	return;
      for (int i = 0; i < int(iDim); i++) {

	dataGrid[0][i][j][k] = -999.;
	dataGrid[1][i][j][k] = -999.;
	dataGrid[2][i][j][k] = -999.;

	float sumRef = 0;
	float sumVel = 0;
	float sumSw = 0;
	float refWeight = 0;
	float velWeight = 0;

	for (long n = 0; n <= maxRefIndex; n++) {
	  float dx = refValues[n].x - (xmin + i*iGridsp);
	  float dy = refValues[n].y - (ymin + j*jGridsp);
	  float dz = refValues[n].z - (zmin + k*kGridsp);
	  float rSquare = (dx*dx) + (dy*dy) + (dz*dz);
	  if (rSquare > RSquare) { continue; }
	  
	  float weight = (RSquare - rSquare) / (RSquare + rSquare);
	  
	  refWeight += weight;
	  sumRef += weight*refValues[n].refValue;
	  
	}
	
	for (long n = 0; n <= maxVelIndex; n++) {
	  float dx = velValues[n].x - (xmin + i*iGridsp);
	  float dy = velValues[n].y - (ymin + j*jGridsp);
	  float dz = velValues[n].z - (zmin + k*kGridsp);
	  float rSquare = (dx*dx) + (dy*dy) + (dz*dz);
	  if (rSquare > RSquare) { continue; }
	  
	  float weight = (RSquare - rSquare) / (RSquare + rSquare);

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

	for (int n = 0; n <= maxRefIndex; n++) {
	  float dx = refValues[n].x - (xmin + i*iGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = refValues[n].y - (ymin + j*iGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = refValues[n].z - (zmin + k*iGridsp);
	  if (fabs(dz) > sqrt(20 * falloff_z)) { continue; }
	  
	  float weight = exp(-(dx*dx)/falloff_x 
			     -(dy*dy)/falloff_y
			     -(dz*dz)/falloff_z);

	  refWeight += weight;
	  sumRef += weight*refValues[n].refValue;

	}

	for (int n = 0; n <= maxVelIndex; n++) {
	  float dx = velValues[n].x - (xmin + i*iGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = velValues[n].y - (ymin + j*iGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = velValues[n].z - (zmin + k*iGridsp);
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

	for (int n = 0; n <= maxRefIndex; n++) {

	  float dx = refValues[n].x - (xmin + i*iGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = refValues[n].y - (ymin + j*iGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = refValues[n].z - (zmin + k*iGridsp);
	  if (fabs(dz) > sqrt(20 * falloff_z)) { continue; }
	  
	  float weight = exp(-(dx*dx)/(falloff_x*smoother)
			     -(dy*dy)/(falloff_y*smoother)
			     -(dz*dz)/(falloff_z*smoother));

	  
	  refWeight += weight;
	  float interpRef = trilinear(dx,dy,dz,0);
	  sumRef += weight*(refValues[n].refValue - interpRef);
	  
	}
	for (int n = 0; n <= maxVelIndex; n++) {
	  float dx = velValues[n].x - (xmin + i*iGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = velValues[n].y - (ymin + j*iGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = velValues[n].z - (zmin + k*iGridsp);
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
	id[160] = (int)(xmin * iGridsp * 100);
	id[161] = (int)(xmax * iGridsp * 100);
	id[162] = (int)iDim;
	id[163] = (int)iGridsp * 1000;
	id[164] = 1;
  
	// Y Header
	id[165] = (int)(ymin * jGridsp * 100);
	id[166] = (int)(ymax * jGridsp * 100);
	id[167] = (int)jDim;
	id[168] = (int)jGridsp * 1000;
	id[169] = 2;
  
	// Z Header
	id[170] = (int)(zmin * kGridsp * 1000);
	id[171] = (int)(zmax * kGridsp * 1000);
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
