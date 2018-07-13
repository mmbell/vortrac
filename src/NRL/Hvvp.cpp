/*
*  Hvvp.cpp
*  VORTRAC
*
*  Created by Lisa Mauger on 5/18/06.
*  Copyright 2006 University Corporation for Atmospheric Research.
*  All rights reserved.
*
*/

#include "Hvvp.h"
#include "Ray.h"
#include "Sweep.h"
#include <math.h>
#include "Math/Matrix.h"
#include <QFile>
#include <QDir>
#include <QTextStream>

/*
* The HVVP subroutine used here was created and written by Paul Harasti for 
*   determining the cross wind component of the environmental winds. (2006)
*
* Commented lines with ** indicate that these changes were made to 
*   accomadate the differences in the updated version of Paul's code.
*
*/

Hvvp::Hvvp()
{
	setObjectName("HVVP");
	// Generic constructor, initializes some variables..
	velNull = -999.0;
	deg2rad = acos(-1)/180.0;
	rad2deg = 1.0/deg2rad;
	levels = 14;       
	// Current HVVP number of predictor variables
	xlsDimension = 16;
	// New HVVP number of predictor variables
	//  xlsDimension = 10;
	maxpoints = 500000;               // **
  
	z = new float[levels];
	u = new float[levels];
	v = new float[levels];
	var = new float[levels];
	vm_sin = new float[levels];
	xt = new float[levels];
	vt = new float[levels];
	vr = new float[levels];
	xr = new float[levels];
  
	for(int i = 0; i < levels; i++) {
		z[i] = velNull;
		u[i] = velNull;
		v[i] = velNull;
		var[i] = velNull;
		vm_sin[i] = velNull;
		xt[i] = velNull;
		vt[i] = velNull;
		vr[i] = velNull;
		xr[i] = velNull;
	}

	xls = new float*[xlsDimension];
	wgt = new float[maxpoints];
	yls = new float[maxpoints];
	for(int k = 0; k < xlsDimension; k++) {
		xls[k] = new float[maxpoints];
		for(long kk = 0; kk < maxpoints; kk++)
			xls[k][kk] = velNull;
	}

	printOutput = true;
	hgtStart = .600;                // km   // Most Recently Used
	//hgtStart = 1.0;
	hInc = .1;                       // km  // Most Recently Used
	//hInc = .5;
	xt_threshold = 2.0;
  
}

Hvvp::~Hvvp()
{
	delete [] z;
	delete [] u;
	delete [] v;
	delete [] var;
	delete [] vm_sin;
	delete [] xt;
	delete [] vt;
	delete [] xr;
	delete [] vr;
  
	for(int i = 0; i < xlsDimension; i++) {
		delete [] xls[i];
	}
	delete [] yls;
	delete [] xls;
	delete [] wgt;
}

void Hvvp::setRadarData(RadarData *newVolume, float range, float angle, float vortexRmw)
{
	volume = newVolume;
	cca = angle;            // in meterological coord (degrees)
	rt = range;             // in km
	rmw = vortexRmw;        // in km
}

void Hvvp::setConfig(Configuration* newConfig)
{
	configData = newConfig;
  
	// Load all configuration parameters
	// If this function is not called the parameters default
	//  to values in the constructor

	QDomElement hvvp = configData->getConfig("hvvp");
	levels = configData->getParam(hvvp, QString("levels")).toInt();
	hgtStart = configData->getParam(hvvp, QString("hgt_start")).toFloat();
	hInc = configData->getParam(hvvp, QString("hinc")).toFloat();
	xt_threshold = configData->getParam(hvvp, QString("xt")).toFloat();
	QDir workingDirectoryPath(configData->getParam(configData->getConfig("vortex"),"dir"));
	HVVPLogFile.setFileName(workingDirectoryPath.filePath("HVVP_output.txt"));
  
}

float Hvvp::rotateAzimuth(const float &angle)
{
	// Angle received is in Meteorological Coordinates (degrees from north)

	float newAngle;
	newAngle = angle - cca;
	if (newAngle < 0)
		newAngle +=360;
	return newAngle;
}

long Hvvp::hvvpPrep(int m) {

	float cumin = 5.0/rt;                 // What are the units here?
	float cuspec = 0.6;                   // Unitless
	float curmw = (rt - rmw)/rt;          // Unitless
	float cuthr;                          // Unitless
	float ae = 4.0*6371.0/3.0;                // km
	Sweep* currentSweep = NULL;
	Ray* currentRay = NULL;
	float* vel = NULL; 

	if(cuspec < curmw)
		cuthr = cuspec; 
	else 
		cuthr = curmw;

	/*
	// For Printing the results to file for Fortran Comparisons
 
	Message::toScreen("hgtStart = "+QString().setNum(hgtStart)+" hInc = "+QString().setNum(hInc));
  
	QString fileName("PointsSelected_BigLevel"+QString().setNum(m)+".csv");
	QFile* outputFile = new QFile(fileName);
	outputFile->open(QIODevice::WriteOnly);
	QTextStream out(outputFile);
	out << "Printing Data Points in Level " << m << endl;
	Message::toScreen("Writing to "+outputFile->fileName()+" .... ");
  
	if(m == 0) {
	out << "CUTHR, "<< cuthr << endl;
	out << "CUMIN, "<< cumin << endl;
	out << " alt(km), cu, Vd(m/s), elvn(adjusted for range in rad), ray index, az(after rotation in rad)" << endl;
	}
	*/ // End Comments
  
	rot = cca*deg2rad;               // ** 
	// float rot = (cca-4.22)*deg2rad; **
	// ** Special case scenerio for KBRO Data of Bret (1999)
  
	for(int k = 0; k < xlsDimension; k++) {
		for(long l = 0; l < maxpoints; l++) {
			xls[k][l] = 0;
			yls[l] = 0;
		}
	}

	long count = 0;
	float h0 = hgtStart+hInc*float(m);
	z[m] = h0;
	float hLow = h0-hInc;
	float hHigh = h0+hInc;

	//  out << " num sweeps = " << volume->getNumSweeps();

	for(int s = 0; s < volume->getNumSweeps(); s++) {
		currentSweep = volume->getSweep(s);
		//float elevation = currentSweep->getElevation();     // deg
		// if(elevation <= 5.0) {
		int startRay = currentSweep->getFirstRay();
		int stopRay = currentSweep->getLastRay();
		for(int r = startRay; r <= stopRay; r++) {
			currentRay = volume->getRay(r);
			float elevation = currentRay->getElevation();
			// Current HVVP set elevation max to 5.0
			// New HVVP set elevation max to 25.0
			if(elevation <= 5.0) {                           // deg
				vel = currentRay->getVelData();          // still in km/s
				//	float vGateSpace = currentRay->getVel_gatesp(); // in m
				//	vGateSpace /= 1000.0;   // now in km
				float numGates = currentRay->getVel_numgates();
				int first = 0;
				// int first = currentRay->getFirst_vel_gate();   
				// What does getFirst_vel_gate actually return?
				// Index or distance?
				// Are the limits on this ray right or should i go
				// to first+numGates is ? -LM
				float aa = currentRay->getAzimuth();
				aa = rotateAzimuth(aa)*deg2rad;
				float sinaa = sin(aa);
				float cosaa = cos(aa);
				for(int v = first; v < numGates; v++) {
					if(vel[v]!=velNull) {
						// PH 10/2007.  need accurate range - previously missing first gate distance 
						// which  has usually been -0.375 m (due to radar T/R time delay) but is now
						// 0.125 m for VCP 211.
						float srange =  float(currentRay->getFirst_vel_gate()+(v*currentRay->getVel_gatesp()))/1000.;

						//	    float srange = (rangeStart+float(v)*vGateSpace);
						float cu = srange/rt * cos(elevation*deg2rad);    // unitless
						//float alt = volume->absoluteRadarBeamHeight(srange, elevation);  // km
						float alt = volume->radarBeamHeight(srange, elevation);  // km
						if((cu > cumin)&&(cu < cuthr)&&(alt >= hLow)&&(alt < hHigh)) {
							float ee = elevation*deg2rad;
							ee+=asin(srange*cos(elevation*deg2rad)/(ae+alt));
							float cosee = cos(ee);
							float xx = srange*cosee*sinaa;
							float yy = srange*cosee*cosaa;
							float rr = srange*srange*cosee*cosee*cosee;
							float zz = alt-h0;

							//Used for checking the first ten entries
							/*
							//if(((count<=30)||(s == 6))&&(m==0)){
							out << count << "= v: " << vel[v] << " gate " << v << " ray " <<r-startRay << " orig az = " << currentRay->getAzimuth() << endl;
							out << " elev# " << s << " = " << elevation << endl;
							out << count << "= srange: " << srange << " cu " << cu << " alt " << alt << " zz " << zz << endl;
							}
							// Comments End For Paul's Test      
							if(m == 0) {
							// What Paul would like to receive for comparisons
							out << alt << ", " << cu << ", " << vel[v] << ", " << ee << ", "<< r << ", " << aa << endl;
							}
							// Comments Begin For Paul's Test
	      
							//What I was using for test comparisons
							out << vel[v] << ", " << v << ", " << r-startRay <<", " << currentRay->getAzimuth() << ", " << s << ",  " << elevation << ", " << srange << ", " << cu << ", " << alt << ", " << zz << endl;
		

							}
							*/
	      
							yls[count] = vel[v];
							wgt[count] = 1; 
							// why are we weighting it if they are all the same? -LM
							xls[0][count] = sinaa*cosee;
							xls[1][count] = cosee*sinaa*xx;
							xls[2][count] = cosee*sinaa*zz;
							xls[3][count] = cosaa*cosee;
							xls[4][count] = cosee*cosaa*yy;
							xls[5][count] = cosee*cosaa*zz;
							xls[6][count] = cosee*sinaa*yy;
							// For new HVVP comment out to xls[15][count]
							xls[7][count] = rr*sinaa*sinaa*sinaa;
							xls[8][count] = rr*sinaa*cosaa*cosaa;
							xls[9][count] = rr*cosaa*cosaa*cosaa;
							xls[10][count] = rr*cosaa*sinaa*sinaa;
							xls[11][count] = cosee*sinaa*xx*zz;
							xls[12][count] = cosee*cosaa*yy*zz;
							xls[13][count] = cosee*sinaa*zz*zz;
							xls[14][count] = cosee*cosaa*zz*zz;
							xls[15][count] = cosee*sinaa*yy*zz;
							// For new HVVP, uncomment to xls[9][count] 
							//              xls[7][count] = rr*sinaa;
							//              xls[8][count] = rr*cosaa;
							//              xls[9][count] = (1.0 + sinaa*cosaa)*zz*srange*cosee*cosee;
							if (count < maxpoints) {
								count++;
							} else {
								// emit log(Message(QString("Truncating HVVP at 500k rows"), 0,this->objectName()));
								break;
							}
						}
					}
				}
				vel = NULL;
			}
			currentRay = NULL;
		}
		currentSweep = NULL;
	}
	delete currentSweep;
	delete currentRay;
	delete vel;

	//out << "Level " << m << " count = " << count << endl;
	//Message::toScreen("Level "+QString().setNum(m)+" count = "+QString().setNum(count));

	//outputFile->close();

	return count;
}

bool Hvvp::findHVVPWinds(bool both)
{
	/*
	* Calculates HVVP dependent and independent variables within 14, 200 m
	*   thick layers, every 100 m, starting from 600 m. Restrick elevation
	*   used to those less than or equal to 5 degrees.
	*
	*/

  if(volume->getNumSweeps() < 0) {    // TODO: no sweep with pre-gridded
		// For Analytic No Volume Case
		emit log(Message(QString("Found No Volume To Process"),0,
		this->objectName(),Red,
		QString("Could Not Locate Volume")));
		return false;
	}

	if(both)
		emit log(Message(QString("Using Outlier Thresholding in Second Fit"),
	0,this->objectName(),Green));
	else
		emit log(Message(QString("Running With First Fit Only"),
	0,this->objectName(),Green));

	long count = 0; 
	int last = 0;

	// For updating the percentage bar we have 7% to give away in this routine
	float increment = float(levels)/7.0;

	for(int m = 0; m < levels; m++) {

		if(int((m+1)/increment) > last) {
			last++;
			emit log(Message(QString(),1,this->objectName()));
		}

		xt[m] = velNull; 

		count = hvvpPrep(m);

		/* 
		* Empirically determined limit to the minimum number of points
		*   needed for a low variance HVVP result.
		*
		*/


		if(count >= 6500.0) {

			float sse;
			float *stand_err = new float[xlsDimension];
			float *cc = new float[xlsDimension];
			bool flag, outlier;
      
			// Only for comparisons in Mathematica
			/* // Comment start here
			QString fileName("/scr/science40/mauger/Working/trunk/hvvp1824CharBigRepeat"+QString().setNum(m)+".dat");
			writeToFile(fileName,xlsDimension,count,count,1,xls,yls);
			QString midFileName = fileName+QString(".printA");
			flag = Matrix::oldlls(xlsDimension, count, xls, yls, sse, cc, stand_err, midFileName);
			Message::toScreen(QString(" count = ")+QString().setNum(count));
			QFile* outputFile = new QFile(fileName+QString(".cpplls"));
			outputFile->open(QIODevice::WriteOnly);
			QTextStream out(outputFile);
			out << "Volume Used" << volume->getDateTimeString() << endl;
			out << "Range: " << rt << endl;
			out << "CCA: " << cca << endl;
			out << "RMW: " << rmw << endl;
			out << endl;
			out << "OLD C++ ROUTINE" << endl << endl;
			out << "SSE: "<<sse<<endl;
			out << "stand_dev@" << m << ": " << sse << endl;
			for(int zz = 0; zz < xlsDimension; zz++) {
			out << cc[zz] << " " << stand_err[zz] << endl;
			}
      
			flag = Matrix::lls(xlsDimension, count, xls, yls, sse, cc, stand_err);
      
			out << "NEW C++ ROUTINE" << endl << endl;
			out << "SSE: "<< sse << endl;
			out << "stand_dev@" << m << ": " << sse << endl;
			for(int zz = 0; zz < xlsDimension; zz++) {
			out << cc[zz] << " " << stand_err[zz] << endl;
			}
      
			outputFile->close();
			*/ // comments end here
      
			flag = Matrix::lls(xlsDimension, count, xls, yls, sse, cc, stand_err);

			//Message::toScreen("SSE = "+QString().setNum(sse));

			//Message::toScreen("Finished Printing in HVVP");

			/*
			* Check for outliers that deviate more than two standard 
			*   deviations from the least squares fit.
			*
			*/

			if(flag) {
				outlier = false;
				long cgood = 0;
				for (long n = 0; n < count; n++) {
					float vr_est = 0;
					for(int p = 0; p < xlsDimension; p++) {
						vr_est = vr_est+cc[p]*xls[p][n];
					}
					if(fabs(vr_est-yls[n])>2.0*sse) {
						yls[n] = velNull;
						outlier = true;
					}
					else {
						cgood++;
					}
				}

				// Re-calculate the least squares solution if outliers are found.



				if(both) {
					if(outlier && (cgood >=long(6500))) {
						long qc_count = 0;
						float** qcxls = new float*[xlsDimension];
						float* qcyls = new float[count];
						for(int d = 0; d < xlsDimension; d++) {
							qcxls[d] = new float[count];
						} 
						for (long n = 0; n < count; n++) {
							qcyls[n] = 0;
							for(int p = 0; p < xlsDimension; p++) {
								qcxls[p][n] = 0;
							}

							if(yls[n] != velNull) {
								qcyls[qc_count] = yls[n];
								for(int p = 0; p < xlsDimension; p++) {
									qcxls[p][qc_count] = xls[p][n];
								}
								qc_count++;
							}
						}
						// Message::toScreen("qc_count = "+QString().setNum(qc_count));
						flag=Matrix::lls(xlsDimension,qc_count,qcxls,qcyls,sse,cc,stand_err);
						for(int ii = 0; ii < xlsDimension; ii++) {
							delete [] qcxls[ii];
						}
						delete [] qcxls;
						delete [] qcyls;
					}
				}

				// Calculate the HVVP wind parameters:

				// Radial wind above the radar.
				vr[m] = rt*cc[1];

				// Along beam component of the environmental wind above the radar.
				float vm_c = cc[3]+vr[m];

				// Rankine exponent of the radial wind.
				xr[m] = -1.0*cc[4]/cc[1];

				/* 
				* Variance of xr.  This is used in the
				*  weigthed average of the across beam component of the environmental wind,
				*  c and is calculated along the way as follows:
				*/

				float temp = ((stand_err[4]/cc[4])*(stand_err[4]/cc[4]));
				temp += ((stand_err[1]/cc[1])*(stand_err[1]/cc[1]));
				var[m] = fabs(xr[m])*sqrt(temp);

				/*
				* Relations between the Rankine exponent of the tangential wind, xt,
				*   and xr, determined by theoretical (boundary layer) arguments of 
				*   Willoughby (1995) for the case of inflow, and by extension
				*   (constinuity equation considerations) by Harasti for the case
				*   of outflow.
				*/

				if(vr[m] > 0) {
					if(xr[m] > 0)
						xt[m] = 1.0-xr[m];
					else
						xt[m] = -1.0*xr[m]/2.0;
				}
				else {
					if(xr[m] >= 0)
						xt[m] = xr[m]/2.0;
					else
						xt[m] = 1.+xr[m];
				}

				if(fabs(xt[m]) == xr[m]/2.0) 
					var[m] = .5*var[m];

				// Tangential wind above the radar
				// Assume error in rt is 2 km

				vt[m] = rt*cc[6]/(xt[m]+1.0);


				if(xt[m] == 0) {
					emit log(Message(QString("Xt is Zero, Program Logic Problem"),0,this->objectName(),Red,QString("Xt = 0")));
					return false;
				}

				temp = (2./rt)*(2./rt)+(stand_err[6]/cc[6])*(stand_err[6]/cc[6]);
				temp += (var[m]/xt[m])*(var[m]/xt[m]);
				var[m] = vt[m]*sqrt(temp);

				// Across-beam component of the environmental wind
				float vm_s = cc[0]-vt[m];
				//Message::toScreen(" vm_s = "+QString().setNum(vm_s));

				var[m] = sqrt(stand_err[0]*stand_err[0] + var[m]*var[m]);

				// rotate vm_c and vm_s to standard cartesian U and V components,
				// ue and ve, using cca.
				// cca  = cca *deg2rad;
				// float ue = vm_s*cos(cca)+vm_c*sin(cca);
				// float ve = vm_c*cos(cca)-vm_s*sin(cca);
				//Message::toScreen("rot = "+QString().setNum(rot));

				float ue = vm_s*cos(rot)+vm_c*sin(rot);
				float ve = vm_c*cos(rot)-vm_s*sin(rot);
				//Message::toScreen(" ve = "+QString().setNum(ve));
				//Message::toScreen(" ue = "+QString().setNum(ue));
				//Message::toScreen(" z[m] = "+QString().setNum(z[m]));

				// Set realistic limit on magnitude of results.
				if((xt[m] < 0)||(xt[m] > 1.5)||(fabs(ue)>30.0)||(fabs(ve)>30)||(vt[m]<1)) 
				{
					//z[m] = h0;               
					u[m] = velNull;
					v[m] = velNull;
					vm_sin[m] = velNull;
				} else {
					//z[m] = hgtStart+hInc*float(m);
					u[m] = ue;
					v[m] = ve;
					vm_sin[m] = vm_s;
				}
			} else {
				//z[m] = h0;
				u[m] = velNull;
				v[m] = velNull;
				vm_sin[m] = velNull;
			}
			delete [] stand_err;
			delete [] cc;
		} else {
			//z[m] = h0;
			u[m] = velNull;
			v[m] = velNull;
			vm_sin[m] = velNull;
		}
		//Message::toScreen("HVVP Output From Level "+QString().setNum(m)+" z = "+QString().setNum(z[m])+" vm_sin = "+QString().setNum(vm_sin[m]));

	}

	/*
	*  Reject results whose Xt is greater than one SD from average Xt
	*/

	//  Comment this section out until further testing is complete
	emit log(Message(QString(),1,this->objectName()));

	float xtav=0;
	float xtsd=0;
	float xtsdthr=0;
	count=0;
	for(int i = 0; i < levels; i++) {
		if(u[i]!=velNull) {
			xtav += xt[i];
			count++;
		}
	}
	if(count > 0) {
		xtav /=float(count);
	}
	if(count >3) {
		for(int i = 0; i < levels; i++) {
			if(u[i]!=velNull) {
				xtsd+=((xt[i]-xtav)*(xt[i]-xtav));
			}
		}

		xtsd = sqrt(xtsd/(count-1));  
		xtsdthr = xt_threshold*xtsd;
		for(int i = 0; i < levels; i++) {
			if((fabs(xt[i]-xtav)>xtsdthr)||(xt[i]==velNull)){
				u[i] = velNull;
			}
		}
	}

	/*
	*   Calculate the layer, variance-weighted average of vm_sin. 
	*/

	av_VmSin = 0;
	float var_av_VmSin = 0;
	float sumwgt = 0;
	int ifoundit = 0;
	for(int i = 0; i < levels; i++) {
		//Message::toScreen("var["+QString().setNum(i)+"] = "+QString().setNum(var[i])+" vm_sin["+QString().setNum(i)+"] = "+QString().setNum(vm_sin[i]));
		if((u[i] != velNull)&&(var[i]!=0)) {
			var[i] = var[i]*var[i];
			sumwgt += 1.0/var[i];
			av_VmSin += vm_sin[i]/var[i];
			var_av_VmSin +=1.0/var[i];
		}
	}

	// if(u(i)!=velNull) {   ---- This is how it was in Pauls code
	// the for loop extended to the outside of the else bracket below.. 
	// didn't seem right (or I may have interpretted it wrong but I think
	// this is what was intended

	if((sumwgt!=0) && (var_av_VmSin!=0)) {
		av_VmSin /= sumwgt;
		var_av_VmSin = 1.0/var_av_VmSin;
		stdErr_VmSin = sqrt(var_av_VmSin);
	}
	else {
		av_VmSin= velNull;
		var_av_VmSin=velNull;
		stdErr_VmSin = velNull;
	}

	float diff = 100;
	for(int i = 0; i < levels; i++) {
		if((u[i]!=velNull)&&(var[i]!=velNull)) {
			if(fabs(2.0-z[i]) < diff) {
				diff = fabs(2.0-z[i]);
				ifoundit = i;
			}
		}
	}
	//Message::toScreen("ifoundit = "+QString().setNum(ifoundit));
	QString message;
	QString shortMessage;

	// Here Pauls code does a lot of printing which I will make optional 
	// depending on whether or not printOutput is set to true, the default
	// is false

	message += "RAW HVVP RESULTS\n";
	message += "\n";
	message += "Layer, variance-weighted, average Vm_Sin = ";
	message += QString().setNum(av_VmSin)+" +-";
	message += QString().setNum(stdErr_VmSin)+" (m/s).";
	message += "\n";
	message += "Vm_Sin value closest to 2 km altitude is ";
	message += QString().setNum(vm_sin[ifoundit])+" +- ";
	message += QString().setNum(sqrt(var[ifoundit]))+" m/s at ";
	message += QString().setNum(z[ifoundit])+" km altitude.";
	message += "\n";
	message += "Z (km)  Ue (m/s)  Ve (m/s)  Vm_Sin (m/s)";
	message += "   Stderr_Vm_Sin (m/s)\n\n";
	for(int i = 0; i < levels; i++) {
		if((u[i]!=velNull) && (v[i]!=velNull)) {
			message +=QString().setNum(z[i])+"\t   "+QString().setNum(u[i]);
			message +="\t   "+QString().setNum(v[i])+"\t   "+QString().setNum(vm_sin[i]);
			message +="\t   "+QString().setNum(sqrt(var[i]))+"\n";
		}
	}
	message += "Smoothing width is 3 levels, so at least smooth somewhat";
	message += " beyond 3 levels\n";
	if(count > 4) {
		smoothHvvp(u);
		smoothHvvp(v);
		smoothHvvpVmSin(vm_sin, var);

		message += "Vm_Sin value closest to 2 km altitude is ";
		message += QString().setNum(vm_sin[ifoundit])+" +-";
		message += QString().setNum(sqrt(var[ifoundit]))+" m/s at ";
		message += QString().setNum(z[ifoundit])+" km altitude.\n\n";
		message += "Z (km)  Ue (m/s)  Ve (m/s)  Vm_Sin (m/s)";
		message += "   Stderr_Vm_Sin (m/s)\n\n";
		for(int i = 0; i < levels; i++) {
			if((u[i]!=velNull) && (v[i]!=velNull)) {
				message +=QString().setNum(z[i])+"\t   "+QString().setNum(u[i]);
				message +="\t   "+QString().setNum(v[i])+"\t   "+QString().setNum(vm_sin[i]);
				message +="\t   "+QString().setNum(sqrt(var[i]))+"\n";
			}
		}
	}

	// Shorten message for HVVP Results in Log File
	shortMessage += "Smoothed HVVP RESULTS at "+volume->getDateTimeString()+"\n";
	shortMessage += "\n";
	shortMessage += "Layer, variance-weighted, average Vm_Sin = ";
	shortMessage += QString().setNum(av_VmSin)+" +-";
	shortMessage += QString().setNum(stdErr_VmSin)+" (m/s).";
	shortMessage += "\n";
	//shortMessage += "Vm_Sin value closest to 2 km altitude is ";
	//shortMessage += QString().setNum(vm_sin[ifoundit])+" +- ";
	//shortMessage += QString().setNum(sqrt(var[ifoundit]))+" m/s at ";
	//shortMessage += QString().setNum(z[ifoundit])+" km altitude.";
	//shortMessage += "\n";
	shortMessage += "Z (km)  Ue (m/s)  Ve (m/s)  Vm_Sin (m/s)";
	shortMessage += "   Stderr_Vm_Sin (m/s)  Xt\n";
	shortMessage += "   Vt\t   Vr\t    Xr\n\n";
	for(int i = 0; i < levels; i++) {
		if((u[i]!=velNull) && (v[i]!=velNull)) {
			shortMessage +=QString().setNum(z[i])+"\t   "+QString().setNum(u[i]);
			shortMessage +="\t   "+QString().setNum(v[i])+"\t   "+QString().setNum(vm_sin[i]);
			shortMessage +="\t   "+QString().setNum(sqrt(var[i]))+"\t   "+QString().setNum(xt[i])+"\n";
			shortMessage += "\t   "+QString().setNum(vt[i])+"\t   "+QString().setNum(vr[i]);
			shortMessage += "\t   "+QString().setNum(xr[i])+"\n";
		}
	}

	if(ifoundit>0) {
		if(printOutput) {
			emit log(Message(shortMessage,0,this->objectName()));
			if(HVVPLogFile.isOpen()) {
				HVVPLogFile.close();
			}
			if(HVVPLogFile.open(QIODevice::Append)) {
				HVVPLogFile.write(shortMessage.toLatin1());
				HVVPLogFile.close();
			}
		}
		emit log(Message(QString(),0,this->objectName(),Green)); 
		return true;
	}
	else {
		if(printOutput) {
			message = "No Hvvp Results Found\n";
			emit log(Message(message,0,this->objectName()));
			if(HVVPLogFile.isOpen()) {
				HVVPLogFile.close();
			}
			if(HVVPLogFile.open(QIODevice::Append)) {
				HVVPLogFile.write(message.toLatin1());
				HVVPLogFile.close();
			}

			//Message::toScreen(message);
		}
		emit log(Message(QString("Insufficient Data for HVVP Correction"),0,this->objectName())); 
		return false;
	}
	return false;
}


bool Hvvp::computeCrossBeamWind(float height, float& cc0, float& cc6, float& sse)
{
	/*
	* Calculates HVVP dependent and independent variables within 14, 200 m
	*   thick layers, every 100 m, starting from 600 m. Restrick elevation
	*   used to those less than or equal to 5 degrees.
	*
	*/
	sse = 999.f;
	if(volume->getNumSweeps() < 0) {
		// For Analytic No Volume Case
		std::cout<<"Found No Volume To Process"<<std::endl;
		return false;
	}

	hgtStart = height;
	int count = hvvpPrep(0);

	if(count >= 6500) {

		float *stand_err = new float[xlsDimension];
		float *cc = new float[xlsDimension];
		bool flag, outlier;

		flag = Matrix::lls(xlsDimension, count, xls, yls, sse, cc, stand_err);

		if(flag) {
			outlier = false;
			long cgood = 0;
			for (long n = 0; n < count; n++) {
				float vr_est = 0;
				for(int p = 0; p < xlsDimension; p++) {
					vr_est = vr_est+cc[p]*xls[p][n];
				}
				if(fabs(vr_est-yls[n])>2.0*sse) {
					yls[n] = velNull;
					outlier = true;
				}
				else {
					cgood++;
				}
			}

			// Re-calculate the least squares solution if outliers are found.
			if(1) {
				if(outlier && (cgood >=long(6500))) {
					long qc_count = 0;
					float** qcxls = new float*[xlsDimension];
					float* qcyls = new float[count];
					for(int d = 0; d < xlsDimension; d++) {
						qcxls[d] = new float[count];
					} 
					for (long n = 0; n < count; n++) {
						qcyls[n] = 0;
						for(int p = 0; p < xlsDimension; p++) {
							qcxls[p][n] = 0;
						}

						if(yls[n] != velNull) {
							qcyls[qc_count] = yls[n];
							for(int p = 0; p < xlsDimension; p++) {
								qcxls[p][qc_count] = xls[p][n];
							}
							qc_count++;
						}
					}
					// Message::toScreen("qc_count = "+QString().setNum(qc_count));
					flag=Matrix::lls(xlsDimension,qc_count,qcxls,qcyls,sse,cc,stand_err);
					for(int ii = 0; ii < xlsDimension; ii++) {
						delete [] qcxls[ii];
					}
					delete [] qcxls;
					delete [] qcyls;
				}
			}

			// Across-beam component of the environmental wind
			cc0 = cc[0];
			cc6 = cc[6];
			delete[] stand_err;
			delete[] cc;
			return true;
		} 
		else{
			delete[] stand_err;
			delete[] cc;
		}
	}
	return false;
}

void Hvvp::catchLog(const Message& message)
{
	emit log(message);
}


void Hvvp::setPrintOutput(const bool printToLog) {
	printOutput = printToLog;
}

void Hvvp::smoothHvvp(float* data) {

	int wdth = 3;
	int igrid = wdth/2;
	float* temp = new float[levels];

	for(int i = 0; i < levels; i++) {
		if(data[i] > -90) {
			int i1 = i-igrid;
			int i2 = i+igrid;
			// Make sure we are only smoothing levels that are valid
			if(i1 < 0) {
				i1 = 0;
			}
			if(i2 >= levels) {
				i2 = levels-1;
			}
			int numPoints = 0;
			for(int j = i1; j <= i2; j++) {
				if(data[j]!=velNull) {
					temp[numPoints] = data[j];
					numPoints++;
				}
			}
			if(numPoints > 1) {
				// Sort smallest to largest
				for(int k = numPoints-1; k > 0; k--)
				for(int w = 0; w < k; w++) {
					if(temp[w] > temp[w+1]) {
						float hold = temp[w+1];
						temp[w+1] = temp[w];
						temp[w] = hold;
					}
				}
				int mid = numPoints/2;
				if(numPoints%2!=0) {
					data[i] = temp[mid];
				}
				else {
					data[i] = (temp[mid]+temp[mid-1])/2.0;
				}
			}
			else {
				if(numPoints == 1)
					data[i] = temp[0];
				else
					data[i] = -velNull;
			}
		}
	}
	delete [] temp;
}

void Hvvp::smoothHvvpVmSin(float* data1, float* data2) {

	int wdth = 3;
	int igrid = wdth/2;
	float* temp1 = new float[levels];
	float* temp2 = new float[levels];

	for(int i = 0; i < levels; i++) {
		if(data1[i] > -90) {
			int i1 = i-igrid;
			int i2 = i+igrid;
			// Make sure we are only smoothing levels that are valid
			if(i1 < 0) {
				i1 = 0;
			}
			if(i2 >= levels) {
				i2 = levels-1;
			}
			int numPoints = 0;
			for(int j = i1; j <= i2; j++) {
				if(data1[j]!=velNull) {
					temp1[numPoints] = data1[j];
					temp2[numPoints] = data2[j];
					numPoints++;
				}
			}
			if(numPoints > 1) {
				// Sort smallest to largest
				for(int k = numPoints-1; k > 0; k--)
				for(int w = 0; w < k; w++) {
					if(temp1[w] > temp1[w+1]) {
						// Why aren't both arrays adjusted according to the sorted
						// order of one or both, it seems to me that this
						// is picking the var at random.........
						float hold1 = temp1[w+1];
						float hold2 = temp2[w+1];
						temp1[w+1] = temp1[w];
						temp2[w+1] = temp2[w];
						temp1[w] = hold1;
						temp2[w] = hold2;
					}
				}
				int mid = numPoints/2;
				if(numPoints%2!=0) {
					data1[i] = temp1[mid];
					data2[i] = temp2[mid];
				}
				else {
					data1[i] = (temp1[mid]+temp1[mid-1])/2.0;
					data2[i] = (temp2[mid]+temp2[mid-1])/2.0;
				}
			}
			else {
				if(numPoints==1) {
					data1[i] = temp1[0];
					data2[i] = temp2[0];
				}
				else{
					data1[i] = velNull;
					data2[i] = velNull;
				}
			}
		}
	}
	delete [] temp1;
	delete [] temp2;
}

void Hvvp::writeToFile(QString& fileName, long aRows, long aCols,
long bRows, long bCols, float** a, float* b)
{
	QFile* outputFile = new QFile(fileName);
	outputFile->open(QIODevice::WriteOnly);
	QTextStream out(outputFile);
	out << "Writing Matrices To File" << endl;
	out << aRows << endl;
	out << aCols << endl;
	out << bRows << endl;
	out << bCols << endl;

	int line = 0;
	for(long i = 0; i < aRows; i++) {
		for(long j = 0; j < aCols; j++) {
			out << reset << qSetRealNumberPrecision(5) << scientific << qSetFieldWidth(13) << a[i][j];
			line++;
			if(line == 8) {
				out << endl;
				line = 0;
			}
		}
		line = 0;
		out << endl;
	}
	out << endl;

	for(long i = 0; i < bRows; i++) {
		out << reset << qSetRealNumberPrecision(5) << scientific << qSetFieldWidth(13) << b[i] << endl;

	}
	out << endl;


	outputFile->close();

}

void Hvvp::writeToFileWithAltitude(QString& fileName, long aRows, long aCols,
long bRows, long bCols, float** a, float* b)
{
	QFile* outputFile = new QFile(fileName);
	outputFile->open(QIODevice::WriteOnly);
	QTextStream out(outputFile);
	out << "Writing Matrices To File" << endl;
	out << aRows << endl;
	out << aCols << endl;
	out << bRows << endl;
	out << bCols << endl;

	int line = 0;
	for(long i = 0; i < aRows; i++) {
		for(long j = 0; j < aCols; j++) {
			out << reset << qSetRealNumberPrecision(5) << scientific << qSetFieldWidth(13) << a[i][j];
			line++;
			if(line == 8) {
				out << endl;
				line = 0;
			}
		}
		line = 0;
		out << endl;
	}
	out << endl;

	for(long i = 0; i < bRows; i++) {
		out << reset << qSetRealNumberPrecision(5) << scientific << qSetFieldWidth(13) << b[i] << endl;

	}
	out << endl;


	outputFile->close();

}
