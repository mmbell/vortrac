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
  xDim = yDim = zDim = 0;
  xGridsp = yGridsp = zGridsp = 0.0;

}

CappiGrid::~CappiGrid()
{
}


void CappiGrid::gridRadarData(RadarData *radarData, QDomElement cappiConfig,
					float *vortexLat, float *vortexLon)
{

  // Set the output file
  QString cappiPath = cappiConfig.firstChildElement("dir").text();
  QString cappiFile = radarData->getDateTimeString();
  cappiFile.replace(QString(":"),QString("_"));
  outFileName = cappiPath + "/" + cappiFile;
  
  // Get the dimensions from the configuration
  xDim = cappiConfig.firstChildElement("xdim").text().toFloat();
  yDim = cappiConfig.firstChildElement("ydim").text().toFloat();
  zDim = cappiConfig.firstChildElement("zdim").text().toFloat();
  xGridsp = cappiConfig.firstChildElement("xgridsp").text().toFloat();
  yGridsp = cappiConfig.firstChildElement("ygridsp").text().toFloat();
  zGridsp = cappiConfig.firstChildElement("zgridsp").text().toFloat();

  // Get the relative center and expand the grid around it
  relDist = new float[2];
  relDist = relEarthLocation(radarData->getRadarLat(), radarData->getRadarLon(),
			vortexLat, vortexLon);
  xmin = nearbyintf(relDist[0] - (xDim/2)*xGridsp);
  xmax = nearbyintf(relDist[0] + (xDim/2)*xGridsp);
  ymin = nearbyintf(relDist[1] - (yDim/2)*yGridsp);
  ymax = nearbyintf(relDist[1] + (yDim/2)*yGridsp);
  latReference = *vortexLat;
  lonReference = *vortexLon;
  float distance = sqrt(relDist[0] * relDist[0] + relDist[1] * relDist[1]);
  float beamHeight = radarData->radarBeamHeight(distance, radarData->getSweep(0)->getElevation());
  zmin = (float(int(beamHeight/zGridsp)))*zGridsp;
  zmax = zmin + zDim*zGridsp;
  
  // Find good values
  int r = 0;
  int v = 0;
  
  for (int n = 0; n <= radarData->getNumRays(); n++) {
    Ray* currentRay = radarData->getRay(n);
    float theta = deg2rad * fmodf((450. - currentRay->getAzimuth()),360.);
    float phi = deg2rad * (90. - (currentRay->getElevation()));
    
    if (currentRay->getRef_numgates() > 0) {

      float* refData = currentRay->getRefData();
      for (int g = 0; g <= (currentRay->getRef_numgates()-1); g++) {
		if (refData[g] == -999.) { continue; }
		float range = float(currentRay->getFirst_ref_gate() +
					  (g * currentRay->getRef_gatesp()))/1000.;
		float x = range*sin(phi)*cos(theta);
		if ((x < (xmin - xGridsp)) or x > (xmax + xGridsp)) { continue; }
		float y = range*sin(phi)*sin(theta);
		if ((y < (ymin - yGridsp)) or y > (ymax + yGridsp)) { continue; }
		float z = radarData->radarBeamHeight(range,
					     currentRay->getElevation() );
		if ((z < (zmin - zGridsp)) or z > (zmax + zGridsp)) { continue; }

		// Looks like a good point
		refValues[r].refValue = refData[g];
		refValues[r].x = x;
		refValues[r].y = y;
		refValues[r].z = z;
		refValues[r].rg = range;
		refValues[r].az = currentRay->getAzimuth();
		refValues[r].el = currentRay->getElevation();
		r++;
      }

    }
    if (currentRay->getVel_numgates() > 0) {

      float* velData = currentRay->getVelData();
      float* swData = currentRay->getSwData();
      for (int g = 0; g <= (currentRay->getVel_numgates()-1); g++) {
		if (velData[g] == -999.) { continue; }
		float range = float(currentRay->getFirst_vel_gate() +
					  (g * currentRay->getVel_gatesp()))/1000.;
		float x = range*sin(phi)*cos(theta);
		if ((x < (xmin - xGridsp)) or x > (xmax + xGridsp)) { continue; }
		float y = range*sin(phi)*sin(theta);
		if ((y < (ymin - yGridsp)) or y > (ymax + yGridsp)) { continue; }
		float z = radarData->radarBeamHeight(range,
					     currentRay->getElevation() );
		if ((z < (zmin - zGridsp)) or z > (zmax + zGridsp)) { continue; }

		// Looks like a good point
		velValues[v].velValue = velData[g];
		velValues[v].swValue = swData[g];
		velValues[v].x = x;
		velValues[v].y = y;
		velValues[v].z = z;
		velValues[r].rg = range;
		velValues[r].az = currentRay->getAzimuth();
		velValues[r].el = currentRay->getElevation();
		v++;
      }
	  
    }

  }

  // Subtract off one from the count for iterative purposes
  maxRefIndex = r - 1;
  maxVelIndex = v - 1;

  // Interpolate the data depending on method chosen
  QString interpolation = cappiConfig.firstChildElement("interpolation").text();
  if (interpolation == "barnes") {
    BarnesInterpolation();
  } else if (interpolation == "cressman") {
    CressmanInterpolation();
  }

  // Set the initial field names
  fieldNames << "DZ" << "VE" << "SW";

}
void CappiGrid::CressmanInterpolation()
{

  // Cressman Interpolation
  
  // Calculate radius of influence 
  float xRadius = (xGridsp * xGridsp) * 2.25;
  float yRadius = (yGridsp * yGridsp) * 2.25;
  float zRadius = (zGridsp * zGridsp) * 2.25;
  float RSquare = xRadius + yRadius + zRadius;

  for (int k = 0; k < int(zDim); k++) { 
    for (int j = 0; j < int(yDim); j++) {
      for (int i = 0; i < int(xDim); i++) {

	cartGrid[0][i][j][k] = -999.;
	cartGrid[1][i][j][k] = -999.;
	cartGrid[2][i][j][k] = -999.;

	float sumRef = 0;
	float sumVel = 0;
	float sumSw = 0;
	float refWeight = 0;
	float velWeight = 0;

	for (int n = 0; n <= maxRefIndex; n++) {
	  float dx = refValues[n].x - (xmin + i*xGridsp);
	  float dy = refValues[n].y - (ymin + j*yGridsp);
	  float dz = refValues[n].z - (zmin + k*zGridsp);
	  float rSquare = (dx*dx) + (dy*dy) + (dz*dz);
	  if (rSquare > RSquare) { continue; }
	  
	  float weight = (RSquare - rSquare) / (RSquare + rSquare);

	  refWeight += weight;
	  sumRef += weight*refValues[n].refValue;

	}

	for (int n = 0; n <= maxVelIndex; n++) {
	  float dx = velValues[n].x - (xmin + i*xGridsp);
	  float dy = velValues[n].y - (ymin + j*yGridsp);
	  float dz = velValues[n].z - (zmin + k*xGridsp);
	  float rSquare = (dx*dx) + (dy*dy) + (dz*dz);
	  if (rSquare > RSquare) { continue; }
	  
	  float weight = (RSquare - rSquare) / (RSquare + rSquare);

	  velWeight += weight;
	  sumVel += weight*velValues[n].velValue;
	  sumSw += weight*velValues[n].swValue;

	}

	if (refWeight > 0) {
	  cartGrid[0][i][j][k] = sumRef/refWeight;
	}
	if (velWeight > 0) {
	  cartGrid[1][i][j][k] = sumVel/velWeight;
	  cartGrid[2][i][j][k] = sumSw/velWeight;
	}	  
      }
    }
  }
}

void CappiGrid::BarnesInterpolation()
{

  // Barnes Interpolation (see Koch et al, 1983 for details)

  float falloff_x = 5.052*pow((4* xGridsp / Pi),2);
  float falloff_y = 5.052*pow((4* yGridsp / Pi),2);
  float falloff_z = 5.052*pow((4* zGridsp / Pi),2);
  float smoother = 0.3;


  for (int k = 0; k < int(zDim); k++) { 
    for (int j = 0; j < int(yDim); j++) {
      for (int i = 0; i < int(xDim); i++) {

	cartGrid[0][i][j][k] = -999.;
	cartGrid[1][i][j][k] = -999.;
	cartGrid[2][i][j][k] = -999.;

	float sumRef = 0;
	float sumVel = 0;
	float sumSw = 0;
	float refWeight = 0;
	float velWeight = 0;

	for (int n = 0; n <= maxRefIndex; n++) {
	  float dx = refValues[n].x - (xmin + i*xGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = refValues[n].y - (ymin + j*xGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = refValues[n].z - (zmin + k*xGridsp);
	  if (fabs(dz) > sqrt(20 * falloff_z)) { continue; }
	  
	  float weight = exp(-(dx*dx)/falloff_x 
			     -(dy*dy)/falloff_y
			     -(dz*dz)/falloff_z);

	  refWeight += weight;
	  sumRef += weight*refValues[n].refValue;

	}

	for (int n = 0; n <= maxVelIndex; n++) {
	  float dx = velValues[n].x - (xmin + i*xGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = velValues[n].y - (ymin + j*xGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = velValues[n].z - (zmin + k*xGridsp);
	  if (fabs(dz) > sqrt(20 * falloff_z)) { continue; }
	  
	  float weight = exp(-(dx*dx)/falloff_x 
			     -(dy*dy)/falloff_y
			     -(dz*dz)/falloff_z);

	  velWeight += weight;
	  sumVel += weight*velValues[n].velValue;
	  sumSw += weight*velValues[n].swValue;

	}

	if (refWeight > 0) {
	  cartGrid[0][i][j][k] = sumRef/refWeight;
	}
	if (velWeight > 0) {
	  cartGrid[1][i][j][k] = sumVel/velWeight;
	  cartGrid[2][i][j][k] = sumSw/velWeight;
	}	  
      }
    }
  }

  for (int k = 0; k < int(zDim); k++) {
    for (int j = 0; j < int(yDim); j++) {
      for (int i = 0; i < int(xDim); i++) {

	float sumRef = 0;
	float sumVel = 0;
	float sumSw = 0;
	float refWeight = 0;
	float velWeight = 0;

	for (int n = 0; n <= maxRefIndex; n++) {

	  float dx = refValues[n].x - (xmin + i*xGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = refValues[n].y - (ymin + j*xGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = refValues[n].z - (zmin + k*xGridsp);
	  if (fabs(dz) > sqrt(20 * falloff_z)) { continue; }
	  
	  float weight = exp(-(dx*dx)/(falloff_x*smoother)
			     -(dy*dy)/(falloff_y*smoother)
			     -(dz*dz)/(falloff_z*smoother));

	  
	  refWeight += weight;
	  float interpRef = trilinear(dx,dy,dz,0);
	  sumRef += weight*(refValues[n].refValue - interpRef);
	  
	}
	for (int n = 0; n <= maxVelIndex; n++) {
	  float dx = velValues[n].x - (xmin + i*xGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = velValues[n].y - (ymin + j*xGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = velValues[n].z - (zmin + k*xGridsp);
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
	  cartGrid[0][i][j][k] += sumRef/refWeight;
	}
	if (velWeight > 0) {
	  cartGrid[1][i][j][k] += sumVel/velWeight;
	  cartGrid[2][i][j][k] += sumSw/velWeight;
	}	  	
      }
    }
  }
  
}

float CappiGrid::trilinear(const float &x, const float &y,
			      const float &z, const int &param)
{

  // Do a bilinear interpolation from the nearest gridpoints
  int x0 = int((float(int(x/xGridsp))*xGridsp - xmin)/xGridsp);
  int y0 = int((float(int(y/yGridsp))*yGridsp - ymin)/yGridsp);
  int z0 = int((float(int(z/zGridsp))*zGridsp - zmin)/zGridsp);
  float dx = (x/xGridsp) - int(x/xGridsp);
  float dy = (y/yGridsp) - int(y/yGridsp);
  float dz = (z/zGridsp) - int(z/zGridsp);
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


  if (x0 >= xDim-1) {
    x1 = x0;
  } else {
    x1 = x0 + 1;
  }

  if (y0 >= yDim-1) {
    y1 = y0;
  } else {
    y1 = y0 + 1;
  }

  if (z0 >= zDim-1) {
    z1 = z0;
  } else {
    z1 = z0 + 1;
  }

  float interpValue = 0;
  if (cartGrid[param][x0][y0][z0] != -999) {
    interpValue += omdx*omdy*omdz*cartGrid[param][x0][y0][z0];
  }
  if (cartGrid[param][x0][y1][z0] != -999) {
    interpValue += omdx*dy*omdz*cartGrid[param][x0][y1][z0];
  }
  if (cartGrid[param][x1][y0][z0] != -999) {
    interpValue += dx*omdy*omdz*cartGrid[param][x1][y0][z0];
  }
  if (cartGrid[param][x1][y1][z0] != -999) {
    interpValue += dx*dy*omdz*cartGrid[param][x1][y1][z0];
  }
  if (cartGrid[param][x0][y0][z1] != -999) {
    interpValue += omdx*omdy*dz*cartGrid[param][x0][y0][z1];
  }
  if (cartGrid[param][x0][y1][z1] != -999) {
    interpValue += omdx*dy*dz*cartGrid[param][x0][y1][z1];
  }
  if (cartGrid[param][x1][y0][z1] != -999) {
    interpValue += dx*omdy*dz*cartGrid[param][x1][y0][z1];
  }
  if (cartGrid[param][x1][y1][z1] != -999) {
    interpValue += dx*dy*dz*cartGrid[param][x1][y1][z1];
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
	id[160] = (int)(xmin * xGridsp * 100);
	id[161] = (int)(xmax * xGridsp * 100);
	id[162] = (int)xDim;
	id[163] = (int)xGridsp * 1000;
	id[164] = 1;
  
	// Y Header
	id[165] = (int)(ymin * yGridsp * 100);
	id[166] = (int)(ymax * yGridsp * 100);
	id[167] = (int)yDim;
	id[168] = (int)yGridsp * 1000;
	id[169] = 2;
  
	// Z Header
	id[170] = (int)(zmin * zGridsp * 1000);
	id[171] = (int)(zmax * zGridsp * 1000);
	id[172] = (int)zDim;
	id[173] = (int)zGridsp * 1000;
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
		Message::report("Can't open CAPPI file for writing");
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
	for(int k = 0; k < int(zDim); k++) {
		out << reset << "level" << qSetFieldWidth(2) << k+1 << endl;
		for(int j = 0; j < int(yDim); j++) {
			out << reset << "azimuth" << qSetFieldWidth(3) << j+1 << endl;

			for(int n = 0; n < fieldNames.size(); n++) {
				out << reset << left << fieldNames.at(n) << endl;
				int line = 0;
				for (int i = 0; i < int(xDim);  i++){
				    out << reset << qSetRealNumberPrecision(3) << scientific << qSetFieldWidth(10) << cartGrid[n][i][j][k];
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