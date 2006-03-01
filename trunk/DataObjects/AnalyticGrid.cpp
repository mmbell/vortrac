/*
 *  AnalyticGrid.cpp
 *  VORTRAC
 *
 *  Created by Lisa Mauger on 2/11/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "AnalyticGrid.h"
#include "Message.h"
#include <math.h>
#include <QTextStream>
#include <QFile>

AnalyticGrid::AnalyticGrid() 
  : GriddedData()
{
  coordSystem = cartesian;
  xDim = yDim = zDim = 0;
  xGridsp = yGridsp = zGridsp = 0.0;
  sphericalRangeSpacing = 1;
  sphericalAzimuthSpacing = 1;
  sphericalElevationSpacing = 1;

}

AnalyticGrid::~AnalyticGrid()
{
}

void AnalyticGrid::gridAnalyticData(QDomElement cappiConfig,
				    Configuration* analyticConfig, 
				    float *vortexLat, float *vortexLon)
{

  // Set the output file
  QString filePath = cappiConfig.firstChildElement("dir").text();
  QString fileName("analyticModel");
  outFileName = filePath + "/" + fileName;

  // Get the dimensions from the configuration
  xDim = cappiConfig.firstChildElement("xdim").text().toFloat();
  yDim = cappiConfig.firstChildElement("ydim").text().toFloat();
  zDim = cappiConfig.firstChildElement("zdim").text().toFloat();
  xmin = ymin = zmin = 0;
  xmax = xmin+xDim;
  ymax = ymin+yDim;
  zmax = 1;
  xGridsp = cappiConfig.firstChildElement("xgridsp").text().toFloat();
  yGridsp = cappiConfig.firstChildElement("ygridsp").text().toFloat();
  zGridsp = cappiConfig.firstChildElement("zgridsp").text().toFloat();

  QString sourceString = analyticConfig->getRoot().firstChildElement("source").text();
  
  QDomElement radar = analyticConfig->getRoot().firstChildElement("analytic_radar");
  if (sourceString == QString("wind_field")) {
    //source = windFields;
    QDomElement winds = analyticConfig->getRoot().firstChildElement("wind_field");
    centX = analyticConfig->getParam(winds, "vortexX").toFloat();
    centY = analyticConfig->getParam(winds, "vortexY").toFloat();
    radX = analyticConfig->getParam(radar, "radarX").toFloat();
    radY = analyticConfig->getParam(radar, "radarY").toFloat();
    rmw = analyticConfig->getParam(winds, "rmw").toFloat();
    envSpeed = analyticConfig->getParam(winds, "env_speed").toFloat();
    envDir = analyticConfig->getParam(winds, "env_dir").toFloat();

    for(int v = 0; v < 3; v++) {
      QString tan("vt");
      tan+=QString().setNum(v);
      QString rad("vr");
      rad+=QString().setNum(v);
      QString tanAngle = tan+QString("angle");
      QString radAngle = rad+QString("angle");
      
      if(v == 0) {
	vT.append(analyticConfig->getParam(winds, tan).toFloat());
	vR.append(analyticConfig->getParam(winds, rad).toFloat()); 
      }
      else {
	vT.append(analyticConfig->getParam(winds, tan).toFloat()*vT[0]);
	vR.append(analyticConfig->getParam(winds, rad).toFloat()*vR[0]); 
      }
      vTAngle.append(analyticConfig->getParam(winds, 
					      tanAngle).toFloat()*deg2rad);
      vRAngle.append(analyticConfig->getParam(winds, 
					      radAngle).toFloat()*deg2rad);
      
    }
    
    // Message::toScreen("EnvirWinds: "+QString().setNum(envSpeed));
    //Message::toScreen("Beginning Velocity Assignment");
    //Message::toScreen("Vortex Center = ("+QString().setNum(centX)+","+QString().setNum(centY)+")");
    //Message::toScreen("Radar Center = ("+QString().setNum(radX)+","+QString().setNum(radY)+")");

    QTextStream out(stdout);

    for(int j = yDim - 1; j >= 0; j--) {
      for(int i = xDim - 1; i >= 0; i--) {
	for(int a = 0; a < 3; a++) {
	  // zero out all the points
	  dataGrid[a][i][j][0] = 0;
	}

	//Message::toScreen("loco ("+QString().setNum(i)+","
	//+QString().setNum(j)+")");

	float vx = 0;
	float vy = 0;
	float ref = 0;
	float delX, delY, r, theta, delRX, delRY, radR;
	delX = centX-(xGridsp*i);
	delY = centY-(yGridsp*j);
	r = sqrt(delX*delX+delY*delY);
	theta = atan2(delX,delY);
	delRX = radX-(xGridsp*i);
	delRY = radY-(yGridsp*j);
	radR = sqrt(delRX*delRX+delRY*delRY);

	// Add in tangential wind structure

	/* The storm velocity is calculated at each point according to the
	 * relative position of the storm. A vector is drawn from the point
	 * of interest to the 
	 */

	if(r>rmw) {
	  for(int a = 0; a < vT.count(); a++) {
	    vx += vT[a]*cos(a*(Pi-theta+vTAngle[a]))*(rmw/r)*(delY/r);
	    vy += vT[a]*cos(a*(Pi-theta+vTAngle[a]))*(rmw/r)*(delX/r);
	    ref += vT[a]*cos(a*(Pi-theta+vTAngle[a]))*(rmw/r);
	  } 
	}
	else {
	  if(r!=0) {
	    for(int a = 0; a < vT.count(); a++) {
	      vx+=vT[a]*cos(a*(Pi-theta+vTAngle[a]))*(r/rmw)*(delY/r);
	      vy+=vT[a]*cos(a*(Pi-theta+vTAngle[a]))*(r/rmw)*(delX/r);
	      ref+=vT[a]*cos(a*(Pi-theta+vTAngle[a]))*(r/rmw);
	    } 
	  }
	}
  
	//
	
	// Add in environmental wind structure
	
   /* The angle used here, envDir, is the angle that the environmental wind 
    * is coming from according to the azimuthal, or meterological convention
    * (0 north, clockwise)
    */

	float vex = envSpeed*sin(Pi+(envDir)*deg2rad);
	float vey = envSpeed*cos(Pi+(envDir)*deg2rad);
	vx += vex;
	vy += vey;
	ref+= sqrt(vex*vex+vey*vey);
	
	// Sample in direction of radar
	if(radR !=0) {
	  dataGrid[1][i][j][0] = (delRX*vx-delRY*vy)/radR;
	}      	
	dataGrid[0][i][j][0] = ref;
	dataGrid[2][i][j][0] = -999;

	// out << "("<<QString().setNum(i)<<","<<QString().setNum(j)<<")";
	//out << int (dataGrid[0][i][j]) << " ";
	//out << abs((int)theta) << " ";
      }
      // out << endl;
    } 
  }
  
  if (sourceString == "mm5") {
    source = mm5;
    
  }

  // Set the initial field names
  fieldNames << "DZ" << "VE" << "SW";

}

void AnalyticGrid::writeAsi()
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
	/*
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

	*/
	for(int b = 33; b <=38; b++)
	  id[b] = 0;

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
	for(int k = 0; k < 1; k++) {
		out << reset << "level" << qSetFieldWidth(2) << k+1 << endl;
		for(int j = 0; j < int(yDim); j++) {
			out << reset << "azimuth" << qSetFieldWidth(3) << j+1 << endl;

			for(int n = 0; n < fieldNames.size(); n++) {
			  out << reset << left << fieldNames.at(n) << endl;
				int line = 0;
				for (int i = 0; i < int(xDim);  i++){
				    out << reset << qSetRealNumberPrecision(3) << scientific << qSetFieldWidth(10) << dataGrid[n][i][j];
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




