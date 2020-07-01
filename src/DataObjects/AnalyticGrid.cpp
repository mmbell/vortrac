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
#include "IO/Message.h"
#include <math.h>
#include <QTextStream>
#include <QFile>
#include <QDir>

AnalyticGrid::AnalyticGrid() 
  : GriddedData()
{


  iDim = jDim = kDim = 0;
  // Dimensions of the gridded data set (number of points) 
  // in the i, j and k directions.

  iGridsp = jGridsp = kGridsp = 0.0;
  // Spacing between the grid points in the i, j and k directions (km)

  sphericalRangeSpacing = 3;
  // The spacing to be used between range calculations at a specific
  // elevation and azimuthal angle (km)

  sphericalAzimuthSpacing = 3;
  // Spacing between azimuth angle measurements in degrees

  sphericalElevationSpacing = 3;
  // Spacing between elevation angle measurements in degrees

  //testRange();

}

AnalyticGrid::~AnalyticGrid()
{
  delete [] relDist;
  vLat = NULL;
  delete vLat;
  vLon = NULL;
  delete vLon;
  rLat = NULL;
  delete rLat;
  rLon = NULL;
  delete rLon;
}

bool AnalyticGrid::getConfigInfo(Configuration* mainConfig,
				 Configuration* analyticConfig)
{
  // Set the output file
  QDomElement cappi = mainConfig->getConfig("cappi");
  QString filePath = mainConfig->getParam(cappi,"dir");
  QString fileName("analyticModel");
  outFileName = filePath + "/" + fileName;

  // TODO For pre-gridded, these shouldn't come from the config file
  
  // Get the dimensions from the configuration
  iDim = mainConfig->getParam(cappi, "xdim").toFloat();
  // iDim: Number of data points running on the i-axis

  jDim = mainConfig->getParam(cappi, "ydim").toFloat();
  // jDim: Number of data points running on the j-axis

  kDim = mainConfig->getParam(cappi, "zdim").toFloat();
  // kDim: Number of data points running on the k-axis

  iGridsp = mainConfig->getParam(cappi, "xgridsp").toFloat();
  jGridsp = mainConfig->getParam(cappi, "ygridsp").toFloat();
  kGridsp = mainConfig->getParam(cappi, "zgridsp").toFloat();

  // Reset Size of Data Grid

  // Determine what type of analytic storm is desired
  QString sourceString = analyticConfig->getRoot().firstChildElement("source").text();
  
  QDomElement radar = analyticConfig->getConfig("analytic_radar");
  
  float* radLocation = getCartesianPoint(rLat,rLon,vLat,vLon);

  //Message::toScreen("vortexLat = "+QString().setNum(*vLat)+" vortexLon = "+QString().setNum(*vLon));
  
  //Message::toScreen("radarLat = "+QString().setNum(*rLat)+" radarLon = "+QString().setNum(*rLon));
  
  //Message::toScreen("Difference in x = "+QString().setNum(radLocation[0])+" Difference in y = "+QString().setNum(radLocation[1]));

  float rXDistance =  0;
  float rYDistance =  0;

  // connects the (0,0) point on the cappi grid, to the latitude and
  // longitude of the radar.

  setLatLonOrigin(rLat, rLon,&rXDistance,&rYDistance);

  // Defines iteration indexes for cappi grid

  xmin = nearbyintf(radLocation[0] - (iDim/2)*iGridsp);
  xmax = nearbyintf(radLocation[0] + (iDim/2)*iGridsp);
  ymin = nearbyintf(radLocation[1] - (jDim/2)*jGridsp);
  ymax = nearbyintf(radLocation[1] + (jDim/2)*jGridsp);
  zmin = 1;
  zmax = zmin+kDim*kGridsp;

  Message::toScreen("Xmin = "+QString().setNum(xmin)+" Xmax = "+QString().setNum(xmax)+" Ymin = "+QString().setNum(ymin)+" Ymax = "+QString().setNum(ymax));
 
  setCartesianReferencePoint(0,0,1);
    
  centX = iDim/2*iGridsp+xmin;  // x coordinate of storm center on grid
  centY = jDim/2*jGridsp+ymin;  // y coordinate of storm center on grid
  
  delete[] radLocation;
  
  //testing Message::toScreen("RadX,Y ="+QString().setNum(*rXDistance)+", "+QString().setNum(*rYDistance)+"  VortX,Y ="+QString().setNum(centX)+", "+QString().setNum(centY));
  
  if (sourceString == QString("wind_field")) {
    // This indicates that the analytic storm will be constructed from 
    // a series of overlayed wind fields from the specifications in 
    // the default analytic xml file
    
    source = windFields;
  }
  if (sourceString == QString("lamb")) {
    source = lamb;
  }
  if(sourceString == QString("deformation")) {
    source = deformation;
  }
  if (sourceString == QString("mm5")) {
    source = mm5;
    // Not implemented
  }
   
  // Get the needed configuration parameters
  if((source == windFields)||(source == lamb)||(source == deformation)) {
    QDomElement winds =analyticConfig->getConfig("wind_field");
    
    // radius of maximum wind for analytic storm
    rmw = analyticConfig->getParam(winds, "rmw").toFloat();
    
    // speed and direction of mean environmental wind 
    envSpeed = analyticConfig->getParam(winds, "env_speed").toFloat();
    envDir = analyticConfig->getParam(winds, "env_dir").toFloat();

    QString tan("vt"+QString().setNum(0));
    QString rad("vr"+QString().setNum(0));
    QString tanAngle = tan+QString("angle");
    QString radAngle = rad+QString("angle");   
    vT.append(analyticConfig->getParam(winds, tan).toFloat());
    vR.append(analyticConfig->getParam(winds, rad).toFloat()); 
    vTAngle.append(0);
    vRAngle.append(0); 
    
    if(source == windFields) {
      for(int v = 1; v < 3; v++) {
	QString tan("vt"+QString().setNum(v));
	QString rad("vr"+QString().setNum(v));
	QString tanAngle = tan+QString("angle");
	QString radAngle = rad+QString("angle");
	
	vT.append(analyticConfig->getParam(winds, tan).toFloat()*vT[0]);
	vR.append(analyticConfig->getParam(winds, rad).toFloat()*vR[0]); 
	vTAngle.append(analyticConfig->getParam(winds, 
						tanAngle).toFloat()*deg2rad);
	vRAngle.append(analyticConfig->getParam(winds, 
						radAngle).toFloat()*deg2rad);
      }
    }
    else {
      if(source == lamb) {
	
      // Analytic Storm based on Lamb Model
      
      QDomElement lamb = analyticConfig->getConfig("lamb");
      vorDisturbance = analyticConfig->getParam(lamb, "disturbance").toFloat();
      lambAmplitude = analyticConfig->getParam(lamb, "amplitude").toFloat();
      lambAngle = analyticConfig->getParam(lamb, "angle").toFloat()*deg2rad;

      }
      else {
	if (source == deformation) {

	  // Analytic storm based on deformation field model

	  QDomElement defElement = analyticConfig->getConfig("deformation");
	  defMagnitude = analyticConfig->getParam(defElement, 
						  "magnitude").toFloat();
	  Message::toScreen("deformation magnitude ="+QString().setNum(defMagnitude));
	  dialationAxis = analyticConfig->getParam(defElement, 
						  "dialation_axis").toFloat();
	  Message::toScreen("deformation axis ="+QString().setNum(dialationAxis));
	  dialationAxis *=deg2rad;
	}
      }
    }
    
    /*
     * All the angle measurements that come in from the analytic configuration
     *   xml file are in meterological coordinates (degrees clockwise from north)
     *
     */
  }
  else {
    if(source == mm5) {
      Message::toScreen("MM5 models not yet implemented");
      return false;
    }
  }


  // Set the initial field names
  fieldNames << "DZ" << "VE" << "SW";

  return true;

}

void AnalyticGrid::gridAnalyticData(Configuration* mainConfig,
				    Configuration* analyticConfig, 
				    float *vortexLat, float *vortexLon,
				    float *radarLat, float *radarLon)
  
  /* 
   * Retreives information from the analytic vortex configuration xml file
   * and the cappi portion of the master configuration xml file. 
   * Determines the desired model vortex with necessary parameters 
   * from the configuration information and creates and populates a cartesian
   * grid of data points.
   *
   */

{
  vLat = vortexLat; 
  vLon = vortexLon;
  rLat = radarLat;
  rLon = radarLon;

  getConfigInfo(mainConfig, analyticConfig);
  
  if (source == windFields) { 
    Message::toScreen("Hit Enum to Wind Field");
    gridWindFieldData();
  }
  
  if (source == lamb) {
    gridLambData();
    Message::toScreen("Hit Enum to Lamb");
  }
  if (source == deformation) {
    gridDefFieldData();
    Message::toScreen("Hit Enum to Def");
  }
  if (source == mm5) {
        
  }

}

void AnalyticGrid::gridWindFieldData()

  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // Working on math this is the only right option right now
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  /*
   * Creates data from user specified winds based on the rankine vortex model
   * allows for asymmetries in tangential and radial flows within the idealized
   * vortex.
   */
{
  /*
   * Creates data points based on given analytic vortex parameters using
   * the rankine vortex model with asymetric wind options
   */

  //QTextStream out(stdout);
  for(int k = 0; k < int(kDim); k++) {
    //Message::toScreen("K = "+QString().setNum(k));
    for(int j = int(jDim)-1; j >= 0; j--) {
      //Message::toScreen("J = "+QString().setNum(j));
      for(int i = int(iDim)-1; i >= 0; i--) {
	//Message::toScreen("I = "+QString().setNum(i));
	for(int a = 0; a < 3; a++) {
	  // zero out all the points
	  dataGrid[a][i][j][k] = 0;
	}

	float vx = 0;
	// Wind component in the positive x direction in a cartesian 
	// system where positive y is north
	float vy = 0;
	// Wind component in the positive y direction in a cartesian 
	// system where positive y is north
	float ref = 0;
	float delX, delY, r, theta, delRX, delRY, radR;
	delX = centX-(iGridsp*i+xmin);
	delY = centY-(jGridsp*j+ymin);
	r = sqrt(delX*delX+delY*delY);
	theta = atan2(delX,delY);
	delRX = radX-(iGridsp*i+xmin);
	delRY = radY-(jGridsp*j+ymin);
	radR = sqrt(delRX*delRX+delRY*delRY);
	float c1 = .1;
	float c2 = 3;

	// Add in tangential wind structure

	/* The storm velocity is calculated at each point according to the
	 * relative position of the storm. A vector is drawn from the point
	 * of interest to the 
	 */

	if(r>rmw) {
	  for(int a = 0; a < vT.count(); a++) {
	    float tangentialV =  vT[a]*cos(a*(Pi-theta+vTAngle[a]))*(rmw/r);
	    vx +=tangentialV*(delY/r);
	    //vy +=tangentialV*(delX/r);
	    vy += tangentialV*(-1*delX/r);
	    ref += tangentialV;
	  } 
	}
	else {
	  if(r!=0) {
	    for(int a = 0; a < vT.count(); a++) {
	      float tangentialV =vT[a]*cos(a*(Pi-theta+vTAngle[a]))*(r/rmw);
	      vx+=tangentialV*(delY/r);
	      //vy+=tangentialV*(delX/r);
	      vy+= tangentialV*(-1*delX/r);
	      ref+=tangentialV;
	    } 
	  }
	}

	// Haven't fixed radial yet
	
	if(r>rmw) {
	  for(int a = 0; a < vR.count(); a++) {
	    float radialV = vR[a]*-c2*sqrt(r-rmw)*(rmw/r);
	    radialV *= cos(a*(Pi-theta+vRAngle[a]));
	    vx+=radialV *(-delX/r);
	    vy+=radialV *(delY/r);
	    //ref+=radialV;
	  }
	}
	else {
	  if(r!=0) {
	    for(int a = 0; a < vR.count(); a++) {
	      float radialV = vR[a]*c1*sqrt((rmw-r)*r);
	      radialV *=cos(a*(Pi-theta+vRAngle[a]));
	      vx+=radialV*(-delX/r);
	      vy+=radialV*(delY/r);
	    //ref+=radialV;
	    }
	  }
	}
	
	// Add in environmental wind structure
	
   /* The angle used here, envDir, is the angle that the environmental wind 
    * is coming from according to the azimuthal, or meterological convention
    * (0 north, clockwise)
    */

	float vex = envSpeed*-1*sin(envDir*deg2rad);
	float vey = envSpeed*-1*cos(envDir*deg2rad);
	
	vx += vex;
	vy += vey;
	//ref+= sqrt(vex*vex+vey*vey);
	ref += 5;
	
	// Sample in direction of radar
	if(radR != 0) {
      
	  dataGrid[1][i][j][k] = -(delRX*vx+delRY*vy)/radR;
	  //dataGrid[1][i][j][k] = envSpeed*radR/200;
     
	}      	
	dataGrid[0][i][j][k] = ref;
	dataGrid[2][i][j][k] = -999;

	// out << "("<<QString().setNum(i)<<","<<QString().setNum(j)<<")";
	//out << int (dataGrid[0][i][j]) << " ";
	//out << abs((int)theta) << " ";
      }
      // out << endl;
    } 
  }
  Message::toScreen("Finished Grid Wind Field");
}

void AnalyticGrid::gridLambData()
{

  Message::toScreen("Got into GridLambField");

  // Initializes all data fields

  for(int k = 0; k < kDim; k++) {
    for(int j = int(jDim) - 1; j >= 0; j--) {
      for(int i = int(iDim) - 1; i >= 0; i--) {
	for(int a = 0; a < 3; a++) {
	  // zero out all the points
	  dataGrid[a][i][j][k] = 0;
	}

	float vx = 0;
	float vy = 0;
	float ref = 0;
	float delX, delY, r, theta, delRX, delRY, radR;
	delX = centX-(iGridsp*i);
	delY = centY-(jGridsp*j);
	r = sqrt(delX*delX+delY*delY);
	theta = atan2(delX,delY);
	delRX = radX-(iGridsp*i);
	delRY = radY-(jGridsp*j);
	radR = sqrt(delRX*delRX+delRY*delRY);
	float c1 = .1;
	float c2 = 3;

	// Add in tangential wind structure

	/* The storm velocity is calculated at each point according to the
	 * relative position of the storm. A vector is drawn from the point
	 * of interest to the 
	 */

	if(r>rmw) {
	  float tangentialV =  vT[0]*(rmw/r);
	  vx +=tangentialV*(delY/r);
	  vy +=tangentialV*(delX/r);
	  ref += tangentialV;
	}
	else {
	  if(r!=0) {
	    float tangentialV =vT[0]*(r/rmw);
	    vx+=tangentialV*(delY/r);
	    vy+=tangentialV*(delX/r);
	    ref+=tangentialV;
	  }
	}

	// Add in radial wind structure

	if(r>rmw) {
	  float radialV = vR[0]*-c2*sqrt(r-rmw)*(rmw/r);
	  vx+=radialV *(-delX/r);
	  vy+=radialV *(delY/r);
	  ref+=radialV;
	}
	else {
	  if(r!=0) {
	    float radialV = vR[0]*c1*sqrt((rmw-r)*r);
	    vx+=radialV*(-delX/r);
	    vy+=radialV*(delY/r);
	    ref+=radialV;
	  }
	}

	// Add in environmental wind structure
	
   /* The angle used here, envDir, is the angle that the environmental wind 
    * is coming from according to the azimuthal, or meterological convention
    * (0 north, clockwise)
    */

	float vex = envSpeed*sin(Pi+(envDir)*deg2rad);
	float vey = envSpeed*cos(Pi+(envDir)*deg2rad);
	vx += vex;
	vy += vey;
	

	// Add in winds created from vorticity disturbance

     /*
      * The tangential and radial wind contributions from a vorticity disturbance
      *   are taken from Doppler Velocity Signatures of Idealized Elliptical 
      *   Vortices. Lee, Harasti, Bell, Jou, Change (Not yet published)?
      */
	
	// radial winds

	float mathAz = .25*Pi-theta;
	if(mathAz < 0)
	  mathAz += 2*Pi;
	
	float vrl;
	float rEll = rmw+vorDisturbance*cos(2*(mathAz+lambAngle));
	if(r>rEll) {
	  vrl = .5*(rmw*rmw*rmw/(r*r*r))*vorDisturbance*sin(2*(mathAz+lambAngle));
	  ref += vrl;
	  vx+=vrl*(-delX/r);
	  vy+=vrl*(delY/r);
	}
	else {
	  vrl = .5*r*(vorDisturbance/rmw)*sin(2*(mathAz+lambAngle)); 
	  ref += vrl;
	  vx+=vrl*(-delX/r);
	  vy+=vrl*(delY/r);
	}

	// tangential winds

	float vtl;
	if(r>rEll) {
	  vtl = 1-vorDisturbance*(rmw/(r*r))*cos(2*(mathAz+lambAngle));
	  vtl *= .5*(rmw*rmw/r);

	  ref += vrl;
	  vx+=vrl*(-delX/r);
	  vy+=vrl*(delY/r);
	}
	else {
	  if(r!=0) {
	    vtl = .5*r*(1+(vorDisturbance/rmw)*cos(2*(mathAz+lambAngle)));
	    ref += vrl;
	    vx+=vrl*(-delX/r);
	    vy+=vrl*(delY/r);
	  }
	}

	// Sample in direction of radar
	if(radR != 0) {
	  
	  dataGrid[1][i][j][k] = -(delRX*vx-delRY*vy)/radR;
	}      	
	dataGrid[0][i][j][k] = ref;
	dataGrid[2][i][j][k] = -999;

      }
    } 
  }
  Message::toScreen("Finished grid Lamb data");
}


void AnalyticGrid::gridDefFieldData()
{

  Message::toScreen("Got into GridDefField");

  // Initializes all data fields

  for(int k = 0; k < kDim; k++) {
    for(int j = int(jDim) - 1; j >= 0; j--) {
      for(int i = int(iDim) - 1; i >= 0; i--) {
	for(int a = 0; a < 3; a++) {
	  // zero out all the points
	  dataGrid[a][i][j][k] = 0;
	}

	float vx = 0;
	float vy = 0;
	float ref = 0;
	float delX, delY, r, delRX, delRY, radR;
        // float theta;
	delX = centX-(iGridsp*i);
	delY = centY-(jGridsp*j);
	r = sqrt(delX*delX+delY*delY);
	// theta = atan2(delX,delY);
	delRX = radX-(iGridsp*i);
	delRY = radY-(jGridsp*j);
	radR = sqrt(delRX*delRX+delRY*delRY);
	float c1 = .1;
	float c2 = 3;

	// Add in tangential wind structure

	/* The storm velocity is calculated at each point according to the
	 * relative position of the storm. A vector is drawn from the point
	 * of interest to the 
	 */

	if(r>rmw) {
	  float tangentialV =  vT[0]*(rmw/r);
	  vx +=tangentialV*(delY/r);
	  vy +=tangentialV*(delX/r);
	  ref += tangentialV;
	}
	else {
	  if(r!=0) {
	    float tangentialV =vT[0]*(r/rmw);
	    vx+=tangentialV*(delY/r);
	    vy+=tangentialV*(delX/r);
	    ref+=tangentialV;
	  }
	}

	// Add in radial wind structure

	if(r>rmw) {
	  float radialV = vR[0]*-c2*sqrt(r-rmw)*(rmw/r);
	  vx+=radialV *(-delX/r);
	  vy+=radialV *(delY/r);
	  ref+=radialV;
	}
	else {
	  if(r!=0) {
	    float radialV = vR[0]*c1*sqrt((rmw-r)*r);
	    vx+=radialV*(-delX/r);
	    vy+=radialV*(delY/r);
	    ref+=radialV;
	  }
	}

	// Add in environmental wind structure
	
   /* The angle used here, envDir, is the angle that the environmental wind 
    * is coming from according to the azimuthal, or meterological convention
    * (0 north, clockwise)
    */

	float vex = envSpeed*sin(Pi+(envDir)*deg2rad);
	float vey = envSpeed*cos(Pi+(envDir)*deg2rad);
	vx += vex;
	vy += vey;
	

	// Add in winds from deformation field

     /*
      * The U and V wind contributions from a deformation field
      *   are taken from Doppler Velocity Signatures of Idealized Elliptical 
      *   Vortices. Lee, Harasti, Bell, Jou, Change (Not yet published)?
      */
       
	
	float uDef = 0;
	uDef += -0.5*defMagnitude*(delX*cos(2*dialationAxis));
	uDef += -0.5*defMagnitude*(delY*sin(2*dialationAxis));
	float vDef = 0;
	vDef += 0.5*defMagnitude*(-1*delX*sin(2*dialationAxis));
	vDef += 0.5*defMagnitude*(delY*cos(2*dialationAxis));
	vx += uDef;
	vy += vDef;
       	ref += sqrt(uDef*uDef+vDef*vDef);
	


	// Sample in direction of radar
	if(radR != 0) {
	  
	  dataGrid[1][i][j][k] = -(delRX*vx-delRY*vy)/radR;
	}      	
	dataGrid[0][i][j][k] = ref;
	dataGrid[2][i][j][k] = -999;

      }
    } 
  }
  Message::toScreen("Finished grid deformation data");
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
		int int_1 = *name_1.toLatin1().data();
		int int_2 = *name_2.toLatin1().data();
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
	id[160] = (int)(xmin * iGridsp * 100);
	id[161] = (int)(xmax * iGridsp * 100);
	id[162] = (int)iDim;
	id[163] = (int)iGridsp * 1000;
	id[164] = 1;
  
	// Y Headerhttps://webapps.fanda.ucar.edu/hrisConnect/Admin/Logoff.aspx
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

bool AnalyticGrid::writeAsi(const QString& fileName)
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



void AnalyticGrid::testRange() 
{
  iDim = 100;
  jDim = 100;
  kDim = 5;
  iGridsp = 1;
  jGridsp = 1;
  kGridsp = 1;
  for(int i = 0; i < iDim; i++) {
    for(int j = 0; j < jDim; j++) {
      for(int k = 0; k < kDim; k++) {
	for(int field = 0; field < 3; field++) {
	  float range = sqrt((i-50)*(i-50)+(j-50)*(j-50)+k*k);
	  dataGrid[field][i][j][k] = range;
	}
      }
    }
  }
  setCartesianReferencePoint(50, 50, 0);
  int numPoints = getSphericalRangeLength(90, 0);
  float *refData = new float[numPoints];
  QString field("DZ");
  refData = getSphericalRangeData(field, 90, 0);
  QString print;
  for(int n = 0; n < numPoints; n++) {
    print+=QString().setNum(refData[n])+" ";
  }
  delete[] refData;
  Message::toScreen("testRange");
  Message::toScreen(print);
}
