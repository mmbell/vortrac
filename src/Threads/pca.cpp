/*
 *  pca.cpp
 *  vortrac
 *
 *  Created by Xiaowen Tang on 8/18/13.
 *  Copyright 2013 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "pca.h"
#include <numeric>


pca::pca()
{
	
}


pca::pca(RadarData& vol)
{
	int velLayerFlag;
	for(int i=0; i<vol.getNumSweeps(); ++i){
		if(vol.getSweep(i)->getVel_numgates()>0){
			velLayerFlag = i;
			break;
		}
	}
	
	vr.set_size(vol.getSweep(velLayerFlag)->getNumRays(), vol.getSweep(velLayerFlag)->getVel_numgates());
	az.set_size(vol.getSweep(velLayerFlag)->getNumRays());
	rg.set_size(vol.getSweep(velLayerFlag)->getVel_numgates());
	for(int i=vol.getSweep(velLayerFlag)->getFirstRay(); i<=vol.getSweep(velLayerFlag)->getLastRay(); ++i){
		float* dataPtr = vol.getRay(i)->getVelData();
		az(i-vol.getSweep(velLayerFlag)->getFirstRay()) = vol.getRay(i)->getAzimuth();
		for(int j=0; j<vol.getSweep(velLayerFlag)->getVel_numgates(); ++j)
			vr(i-vol.getSweep(velLayerFlag)->getFirstRay(), j) = dataPtr[j];
	}
	// std::cout<<"azv: "<<az.min()<<", "<<az.max()<<std::endl;
	for(int i=0; i<vol.getSweep(velLayerFlag)->getVel_numgates(); ++i)
		rg(i) = (vol.getSweep(velLayerFlag)->getFirst_vel_gate()+i*vol.getSweep(velLayerFlag)->getVel_gatesp())/1000.;
	// std::cout<<"rgv: "<<rg.min()<<", "<<rg.max()<<std::endl;
	// vr.save("test.txt", arma::raw_ascii);
	
}


bool pca::findCenter(float& azCenter, float& rgCenter)
{
	//azimuthal analysis
	
	//extract mean of each radial for each datum of that radial
	arma::mat vr_a = vr;
	for(int i=0; i<vr.n_rows; ++i){
		std::vector<double> fValid;
		for(int j=0; j<vr.n_cols; ++j)
			if(vr_a(i,j)!=-999.)
				fValid.push_back(vr_a(i,j));
		float vmean = std::accumulate(fValid.begin(), fValid.end(), 0.)/fValid.size();
		for(int j=0; j<vr.n_cols; ++j)
			if(vr_a(i,j)!=-999.)
				vr_a(i,j) -= vmean;
			else
				vr_a(i,j) = 0.;
	}
	arma::mat U, Vt, V;
	arma::vec s;
	
	// azimuth eigenvector
	arma::mat Ya = vr_a.t()/sqrt(vr_a.n_cols-1);
	arma::svd(U, s, Vt, Ya);
	arma::vec e1 = Vt.col(0);
	unsigned int imin,imax;
	e1.min(imin);
	e1.max(imax);
	if(fabs(az(imax)-az(imin))>180.) 
		azCenter = (az(imax)+az(imin)+360.)/2.;
	else
		azCenter = (az(imax)+az(imin))/2.;
	// std::cout<<"az: "<<az(imin)<<", "<<az(imax)<<std::endl;
	
	// radial eigenvector
	arma::mat Yr = vr_a/sqrt(vr_a.n_rows-1);
	arma::svd(U, s, Vt, Yr);
	arma::vec f1 = Vt.col(0);
	arma::vec gf = f1;
	gf.zeros();
	
	for(int i=0; i<gf.n_elem-1; ++i)
		gf(i) = f1(i+1)-f1(i);
	gf(0) = 0.;
	
	gf.min(imin);
	gf.max(imax);
	rgCenter = (rg(imin)+rg(imax))/2.;
	// std::cout<<"rg: "<<rg(imin)<<", "<<rg(imax)<<std::endl;
	// std::cout<<"az center: "<<azCenter<<", rg center: "<<rgCenter<<std::endl;
	return true;
}


void pca::simulateVortex(float azCenter, float rgCenter, float rmw)
{
	float vm = 50.f;
	float x0 = sin(azCenter/180.*arma::datum::pi)*rgCenter;
	float y0 = cos(azCenter/180.*arma::datum::pi)*rgCenter;
	vr.set_size(360, 400);
	az.set_size(360);
	rg.set_size(400);
	for(int j=0; j<vr.n_cols; ++j){
		for(int i=0; i<vr.n_rows; ++i){
			float x = sin(i/180.*arma::datum::pi)*j;
			float y = cos(i/180.*arma::datum::pi)*j;
			float r = sqrt((x-x0)*(x-x0)+(y-y0)*(y-y0))+1e-20;
			float vt;
			if(r<rmw)
				vt = r*vm/rmw;
			else
				vt = vm*sqrt(rmw/r);
			float u = -(y-y0)/r*vt;
			float v =  (x-x0)/r*vt;
			vr(i,j) = (u*x+v*y)/sqrt(x*x+y*y+1e-20);
		}
	}
	for(int i=0; i<az.n_elem; ++i)
		az(i) = i;
	for(int i=0; i<rg.n_elem; ++i)
		rg(i) = i;
	
	// vr.save("test.txt", arma::raw_ascii);
	
}

