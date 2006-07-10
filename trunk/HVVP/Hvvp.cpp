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
//#include "Matrix.h"

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
  // Generic constructor, initializes some variables..
  velNull = -999.0;
  deg2rad = acos(1)/180;
  rad2deg = 1.0/deg2rad;
  levels = 14;       

  z = new float[levels];
  u = new float[levels];
  v = new float[levels];
  var = new float[levels];
  vm_sin = new float[levels];

  printOutput = false;

}

Hvvp::~Hvvp()
{
  delete z;
  delete u;
  delete v;
  delete var;
  delete vm_sin;
  delete volume;
  for(int i = 0; i < 16; i++) {
    delete xls[i];
  }
  delete yls;
  delete xls;
  delete wgt;
}

void Hvvp::setRadarData(RadarData *newVolume, float range, float angle,
			float vortexRmw)
{
  volume = newVolume;
  cca = angle;            // in meterological coord (degrees)
  rt = range;             // in km
  rmw = vortexRmw;        // in km
}

float Hvvp::rotateAzimuth(const float &angle)
{
  // Angle received is in Meteorological Coordinates (degrees from north)

  float newAngle = angle - cca;
  if (newAngle < 0)
    newAngle +=360;

  return newAngle;
}

bool Hvvp::lls(int numCoeff, int numData, int effective_nData, float** x, 
	       float* y, float* weight, float stDeviation, float* stError, 
	       float* coEff)
{

  /*
   * x: a matrix with numCoeff rows, and numData columns
   * y: a matrix with numData rows
   * coEff: the product containing the coefficient values (numCoeff rows)
   * stError: a product containing the estimated error for each
   *  coefficent in coEff, (numCoeff rows)
   * stDeviation: is the estimated standard deviation of the regression
   * 
   * effective_nData and weight are not currently implemented since 
   *  they are not used.
   *
   * This algorithm is identical to the one used in RadarQC
   *
   * Some parts of the algorithm were borrowed from numerical recipes in C++
   *
   */

  if(numData < numCoeff) {
    //emit log(Message("Least Squares: Not Enough Data"));
    return false;
  }
  // We need at least one more data point than coefficient in order to
  // estimate the standard deviation of the fit.
  
  float** A = new float*[numCoeff];
  float** AA = new float*[numCoeff];
  float* B = new float[numCoeff];
  float** BB = new float*[numCoeff];
  coEff = new float[numCoeff];
  for(int row = 0; row < numCoeff; row++) {
    A[row] = new float[numCoeff];
    AA[row] = new float[numCoeff];
    BB[row] = new float[1];
    for(int col = 0; col < numCoeff; col++) {
      A[row][col] = 0;
      AA[row][col] = 0;
      BB[row][0] = 0;
    }
    B[row] = 0;
    coEff[row] = 0;
  }

  // accumulate the covariances of all the data into the regression
  // matrices

  for(int i = 0; i < numData; i++) {
    for(int row = 0; row < numCoeff; row++) {
      for(int col = 0; col < numCoeff; col++) {
	A[row][col]+=(x[row][i]*x[col][i]);
	AA[row][col]+=(x[row][i]*x[col][i]);
      }
      B[row] +=(x[row][i]*y[i]);
      BB[row][0] +=(x[row][i]*y[i]);
    }
  }

  
  float** Ainv = new float*[numCoeff];
  for(int p = 0; p < numCoeff; p++) 
    Ainv[p] = new float[p];


  if(!gaussJordan(AA,BB, numCoeff, 1)) {
    emit log(Message("Least Squares Fit Failed"));
    return false;
  }
  

  for(int i = 0; i < numCoeff; i++) {
    coEff[i] = BB[i][0];
    for(int j = 0; j < numCoeff; j++) {
      Ainv[i][j] = AA[i][j];
    }
  }
  
  // calculate the stDeviation and stError
  float sum = 0;
  for(int i = 0; i < numData; i++) {
    float regValue = 0;
    for(int j = 0; j < numCoeff; j++) {
      regValue += coEff[j]*x[j][i]; 
    }
    sum +=((y[i]-regValue)*(y[i]-regValue));
  }
  
  stDeviation = sqrt(sum/float(numData-numCoeff));
  
  // calculate the standard error for the coefficients

  for(int i = 0; i < numCoeff; i++) {
    stError[i] = stDeviation*sqrt(Ainv[i][i]);
  }

  for(int i = 0; i < numCoeff; i++) {
    delete A[i];
    delete AA[i];
    delete BB[i];
    delete Ainv[i];
  }
  delete A;
  delete B;
  delete AA;
  delete BB;
  delete Ainv;
   
  return true;
}

bool Hvvp::gaussJordan(float **a, float **b, int n, int m)
{

  // get this to do what lls does using cpp recipes
  // Gauss-Jordan Elemination
  // a = nxn coefficient matrix
  // b = nxm matrix

  // identical to leastSquaresRegression in RadarQC

  // returns the inverse of a in a and the solutions to the linear system in b
  
  int i, icol, irow, j, k, l, ll;
  float big, temp, pivinv;
  
  int indxc[n], indxr[n], ipiv[n];

  for(j = 0; j < n; j++) {
    ipiv[j] = 0;
  }
  
  for (i = 0; i < n; i++) {
    big = 0.0;
    for (j = 0;j < n; j++) {
      if(ipiv[j]!=1) {
	for (k = 0; k < n; k++) {
	  if (ipiv[k] ==0) {
	    if(fabs(a[j][k]) >= big) {
	      big = fabs(a[j][k]);
	      irow = j;
	      icol = k;
	    }
	  }
	}
      }
    }
    ++(ipiv[icol]);
    if(irow != icol) {
      for(l = 0; l < n; l++) {
	float dummy = a[irow][l];
	a[irow][l] = a[icol][l];
	a[icol][l] = dummy;
      }
      for(l = 0; l < m; l++) {
	float dummy = b[irow][l];
	b[irow][l] = b[icol][l];
	b[icol][l] = dummy;
      }
    }
    indxr[i] = irow;
    indxc[i] = icol;
    if (a[icol][icol]==0.0) {
      return false; 
    }
    pivinv = 1/a[icol][icol];
    a[icol][icol] = 1.0;
    for(l = 0; l < n; l++) {
      a[icol][l] *=pivinv;
    }
    for(l = 0; l < m; l++) {
      b[icol][l] *= pivinv; 
    }
    for(ll = 0; ll < n; ll++) {
      if(ll != icol) {
	temp = a[ll][icol];
	a[ll][icol] = 0.0;
	for(l = 0; l < n; l++) {
	  a[ll][l] -= a[icol][l]*temp;
	}
	for(l = 0; l < m; l++) {
	  b[ll][l] -= b[icol][l]*temp;
	}
      }
    } 
  }
  for( l = n-1; l >= 0; l--) {
    if(indxr[l] != indxc[l]) {
      for(k = 0; k < n; k++) {
	float dummy = a[k][indxr[l]];
	a[k][indxr[l]] = a[k][indxc[l]];
	a[k][indxc[l]] = dummy;
      }
    }
  }
  

  return true;
}


int Hvvp::hvvpPrep(int m) {

  //**float hgtStart = .500;                // km
  float hgtStart = .600;                // km 
  //**float hInc = .075;                    // km
  float hInc = .1;                       // km
  float rangeStart = -.375;             // ???? km
  float cumin = 5.0/rt;                 // What are the units here?
  float cuspec = 0.6;                   // Unitless
  float curmw = (rt - rmw)/rt;          // Unitless
  float cuthr;                          // Unitless
  float ae = 4*6371/3.0;                // km
  int maxpoints = 200000;               // **

  if(cuspec < curmw)
    cuthr = cuspec; 
  else 
    cuthr = curmw;

  rot = cca*deg2rad;               // ** 
  // float rot = (cca-4.22)*deg2rad; **
  // ** Special case scenerio for KBRO Data of Bret (1999)

  xls = new float*[16];
  yls = new float[maxpoints];
  for(int k = 0; k < 16; k++) {
    xls[k] = new float[maxpoints];
    for(int l = 0; l < maxpoints; l++) {
      xls[k][l] = 0;
      yls[l] = 0;
    }
  }
  
  int count = 0;
  float h0 = hgtStart+hInc*m;
  float hLow = h0-hInc;
  float hHigh = h0+hInc;
  for(int s = 0; s < volume->getNumSweeps(); s++) {
    Sweep *currentSweep = volume->getSweep(s);
    float elevation = currentSweep->getElevation();     // deg
    if(elevation <= 5) {
      int startRay = currentSweep->getFirstRay();
      int stopRay = currentSweep->getLastRay();
      for(int r = startRay; r <= stopRay; r++) {
	Ray *currentRay = volume->getRay(r);
	float *vel = currentRay->getVelData();          // still in km/s
	float vGateSpace = currentRay->getVel_gatesp(); // in km
	float numGates = currentRay->getVel_numgates();
	int first = currentRay->getFirst_vel_gate();   
	// What does getFirst_vel_gate actually return?
	// Index or distance?
	// Are the limits on this ray right or should i go
	// to first+numGates is 
	for(int v = first; v < numGates; v++) {
	  if(vel[v]!=velNull) {
	    float srange = (rangeStart+float(v)*vGateSpace);
	    float cu = srange * (cos(elevation)/rt);    // unitless
	    float alt = volume->radarBeamHeight(cu, elevation);  // km
	    if((cu >= cumin)&&(cu < cuthr)&&(alt >= hLow)&&(alt < hHigh)) {
	      float ee = elevation*deg2rad;
	      ee+=asin(srange*cos(elevation)/(ae+alt));
	      float cosee = cos(ee);
	      float aa = rotateAzimuth(currentRay->getAzimuth())*deg2rad;
	      float sinaa = sin(aa);
	      float cosaa = cos(aa);
	      float xx = srange*cosee*sinaa;
	      float yy = srange*cosee*cosaa;
	      float rr = srange*srange*cosee*cosee*cosee;
	      float zz = alt-h0;
	      yls[count] = vel[v];
	      wgt[count] = 1; // why are we weighting it?
	      xls[0][count] = sinaa*cosee;
	      xls[1][count] = cosee*sinaa*xx;
	      xls[2][count] = cosee*sinaa*zz;
	      xls[3][count] = cosaa*cosee;
	      xls[4][count] = cosee*cosaa*yy;
	      xls[5][count] = cosee*cosaa*zz;
	      xls[6][count] = cosee*sinaa*yy;
	      xls[7][count] = rr*sinaa*sinaa*sinaa;
	      xls[8][count] = rr*sinaa*cosaa*cosaa;
	      xls[9][count] = rr*cosaa*cosaa*cosaa;
	      xls[10][count] = rr*cosaa*sinaa*sinaa;
	      xls[11][count] = cosee*sinaa*xx*zz;
	      xls[12][count] = cosee*cosaa*yy*zz;
	      xls[13][count] = cosee*sinaa*zz*zz;
	      xls[14][count] = cosee*cosaa*zz*zz;
	      xls[15][count] = cosee*sinaa*yy*zz;
	      count++;
	    }
	  }
	}
      }
    }
  }
  return count;
}

bool Hvvp::findHVVPWinds()
{
  /*
   * Calculates HVVP dependent and independent variables within 14, 200 m
   *   thick layers, every 100 m, starting from 600 m. Restrick elevation
   *   used to those less than or equal to 5 degrees.
   *
   */

  int count; 
  float mod_Rankine_xt[levels];

  for(int m = 0; m < levels; m++) {

    float hgtStart = .600;    
    float h0 = hgtStart+.1*m;
    
    count = hvvpPrep(m);
    
    /* 
     * Empirically determined limit to the minimum number of points
     *   needed for a low varieance HVVP result.
     *
     */
    
    if(count >= 6500) {

      float sse;
      float stand_err[16];
      float cc[16];
      bool flag, outlier;
      
      flag = lls(16, count, count, xls, yls, wgt, sse, stand_err, cc);
      
      /*
       * Check for outliers that deviate more than two standard 
       *   deviations from the least squares fit.
       *
       */
      
      if(flag)
	outlier = false;
      int cgood = 0;
      for (int n = 0; n < count; n++) {
	float vr_est = 0;
	for(int p = 0; p < 16; p++) {
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
      

      int qc_count;
 
      float** qcxls = new float*[16];
      float* qcyls = new float[count];
      float* qcwgt = new float[count];
      for(int d = 0; d < 16; d++) {
	qcxls[d] = new float[count];
      }

      if(outlier && (cgood >=6500)) {
	
	qc_count = 0;
	
	for (int n = 0; n < count; n++) {
	  
	  qcwgt[n] = 0;
	  qcyls[n] = 0;
	  for(int p = 0; p < 16; p++) {
	    qcxls[p][n] = 0;
	  }
	  
	  if(yls[n] != velNull) {
	    qcyls[qc_count] = yls[n];
	    qcwgt[qc_count] = 1;
	    for(int p = 0; p < 16; p++) {
	      qcxls[p][qc_count] = xls[p][n];
	    }
	    qc_count++;
	  }
	}
	flag = lls(16, qc_count, qc_count, qcxls,qcyls,qcwgt,sse,stand_err,cc);
	
	// Calculate the HVVP wind parameters:
	
	// Radial wind above the radar.
	float vr = rt*cc[1];
	
	// Along beam component of the environmental wind above the radar.
	float vm_c = cc[3]+vr;
	
	// Rankine exponent of the radial wind.
	float xr = -1*cc[4]/cc[1];
	
	/* 
	 * Variance of xr.  This is used in the
	 *  weigthed average of the across beam component of the environmental wind,
	 *  c and is calculated along the way as follows:
	 */
	
	float temp = ((stand_err[4]/cc[4])*(stand_err[4]/cc[4]));
	temp += ((stand_err[1]/cc[1])*(stand_err[1]/cc[1]));
	var[m] = fabs(xr)*sqrt(temp);
	
	/*
	 * Relations between the Rankine exponent of the tangential wind, xt,
	 *   and xr, determined by theoretical (boundary layer) arguments of 
	 *   Willoughby (1995) for the case of inflow, and by extension
	 *   (constinuity equation considerations) by Harasti for the case
	 *   of outflow.
	 */
	
	float xt;
	
	if(vr > 0) {
	  if(xr > 0)
	    xt = 1-xr;
	  else
	    xt = -1.0*xt/2.0;
	}
	else {
	  if(xr >= 0)
	    xt = xr/2.0;
	  else 
	    xt = 1+xr;
	}
	
	mod_Rankine_xt[m] = xt;
	
	if(fabs(xt) == xr/2.0) 
	  var[m] = .5*var[m];
	
	// Tangential wind above the radar
	// Assume error in rt is 2 km
	
	float vt = rt*cc[6]/(xt+1.0);
	
	temp = (2./rt)*(2./rt)+(stand_err[6]/cc[6])*(stand_err[6]/cc[6]);
	temp += (var[m]/xt)*(var[m]/xt);
	var[m] = vt*sqrt(temp);
	
	// Across-beam component of the environmental wind
	float vm_s = cc[0]-vt;
	
	var[m] = sqrt(stand_err[0]*stand_err[0]+var[m]*var[m]);
	
	// rotate vm_c and vm_s to standard cartesian U and V components,
	// ue and ve, csing cca.
	// cca  = cca *deg2rad;
	// float ue = vm_s*cos(cca)+vm_c*sin(cca);
	// float ve = vm_c*cos(cca)-vm_s*sin(cca);
	
	float ue = vm_s*cos(rot)+vm_c*sin(rot);
	float ve = vm_c*cos(rot)-vm_s*sin(rot);
	
	// Set realistic limit on magnitude of results.
	if((xt < 0)||(fabs(ue)>30.0)||(fabs(ve)>30)) {
	  z[m] = h0;               
	  u[m] = velNull;
	  v[m] = velNull;
	  vm_sin[m] = velNull;
	}
	else {
	  z[m] = h0;
	  u[m] = ue;
	  v[m] = ve;
	  vm_sin[m] = vm_s;
	}
      }
      else {
	z[m] = h0;
	u[m] = velNull;
	v[m] = velNull;
	vm_sin[m] = velNull;
      }
    }
    else {
      z[m] = h0;
      u[m] = velNull;
      v[m] = velNull;
      vm_sin[m] = velNull;
    }
  }

  /*
   *  Reject results whose Xt is greater than one SD from average Xt
   */
  
  float xtav=0;
  float xtsd=0;
  count=0;
  for(int i = 0; i < levels; i++) {
    if(u[i]!=velNull) {
      xtav += mod_Rankine_xt[i];
      count++;
    }
  }
  if(count > 0) {
    xtav /=float(count);
  }
  if(count >3) {
    for(int i = 0; i < levels; i++) {
      if(u[i]!=velNull) {
	xtsd+=((mod_Rankine_xt[i]-xtav)*(mod_Rankine_xt[i]-xtav));
      }
    }
    xtsd = sqrt(xtsd);
    for(int i = 0; i < levels; i++) {
      if(mod_Rankine_xt[i] > xtsd) {
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
    if(u[i] != velNull) {
      var[i] = var[i]*var[i];
      sumwgt += 1.0/var[i];
      av_VmSin += vm_sin[i]/var[i];
      var_av_VmSin +=1.0/var[i];
    }
 
    if(u[i]!=velNull) {
      av_VmSin /= sumwgt;
      var_av_VmSin = 1.0/var_av_VmSin;
    }
    else {
      av_VmSin= velNull;
      var_av_VmSin=velNull;
    }
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
  QString message;
  if(ifoundit >= 1) {
    // Here Pauls code does a lot of printing which I will make optional 
    // depending on whether or not printOutput is set to true, the default
    // is false
    message += "RAW HVVP RESULTS\n";
    message += "\n\n";
    message += "Layer, variance-weighted, average Vm_Sim = ";
    message += QString().setNum(av_VmSin)+" +-";
    message += QString().setNum(sqrt(var_av_VmSin))+" (m/s).";
    message += "\n\n";
    message += "Vm_Sin value closest to 2 km altitude is ";
    message += QString().setNum(vm_sin[ifoundit])+" +- ";
    message += QString().setNum(sqrt(var[ifoundit]))+" m/s at ";
    message += QString().setNum(z[ifoundit])+" km altitude.";
    message += "\n\n";
    message += "Z (km)  Ue (m/s)  Ve (m/s)  Vm_Sin (m/s)";
    message += "   Stderr_Vm_Sin (m/s)\n\n";
    for(int i = 0; i < levels; i++) {
      if((u[i]!=velNull) && (v[i]!=velNull)) {
	message +=QString().setNum(z[i])+" "+QString().setNum(u[i]);
	message +=" "+QString().setNum(v[i])+" "+QString().setNum(vm_sin[i]);
	message +=" "+QString().setNum(sqrt(var[i]))+"\n";
      }
    }
    message += "Smoothing width is 3 levels, so at least smooth somewhat";
    message += " beyond 3 levels\n\n";
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
	  message +=QString().setNum(z[i])+" "+QString().setNum(u[i]);
	  message +=" "+QString().setNum(v[i])+" "+QString().setNum(vm_sin[i]);
	  message +=" "+QString().setNum(sqrt(var[i]))+"\n";
	}
      }
    }
    if(printOutput) {
      emit log(Message(message));
    }
    return true;
  }
  else {
    message = "No Hvvp Results Found";
    if(printOutput) {
      emit log(Message(message));
    }
    return false;
  }
  
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
	  numPoints++;
	  temp[numPoints] = data[j];
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
	  data[i] = (temp[mid]+temp[mid+1])/2.0;
	}
      }
      else {
	data[i] = temp[i];
      }
    }
  }
  delete temp;
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
	  numPoints++;
	  temp1[numPoints] = data1[j];
	  temp2[numPoints] = data2[j];
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
	  data1[i] = (temp1[mid]+temp1[mid+1])/2.0;
	  data2[i] = (temp2[mid]+temp2[mid+1])/2.0;
	}
      }
      else {
	data1[i] = temp1[i];
	data2[i] = temp2[i];
      }
    }
  }
  delete temp1;
  delete temp2;
}

