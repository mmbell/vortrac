/*
 *  mgbvtd.cpp
 *  vortrac
 *
 *  Created by Xiaowen Tang on 5/28/13.
 *  Copyright 2013 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

// This is temporarily disabled as installing the armadillo library pulls in an hdf5 library that
// interferes with lrose-core.
// The proposed fix is to remove the dependency on the lrose legacy netcdf interface so that we can
// use the system libraries instead of building the lrose netcdf library
// Bruno Melli 08/02.19

#include <vector>
#include <algorithm>
#include <cmath>
// #include <armadillo>
#include "mgbvtd.h"

MGBVTD::MGBVTD(float x0, float y0, float hgt, float rmw, GriddedData& cappi):
m_cappi(cappi)
{
	m_centerx = x0;
	m_centery = y0;
	m_centerz = hgt;
	m_rmw     = rmw;
	std::cout << "MGBVTD is not supported in this cyclone release. Aborting" << std::endl;
 	exit(1);
}

float MGBVTD::computeCrossBeamWind(float guessMax, QString& velField, GBVTD* gbvtd, Hvvp* hvvp)
{
#if 0
	const float Rt = sqrt(m_centerx*m_centerx+m_centery*m_centery);

	//1. calculate Vt profile first
	std::vector<float> vt;
	std::vector<float> vt_rng;
	//1. compute the radial profile of symmetric tangential wind  
	for(float rng=m_rmw*1.2; rng<=.6*Rt; rng+=1.){
		m_cappi.setCartesianReferencePoint(m_centerx, m_centery, m_centerz);
		int numData = m_cappi.getCylindricalAzimuthLength(rng, m_centerz);
		float* ringData = new float[numData];
		float* ringAzi  = new float[numData];
		m_cappi.getCylindricalAzimuthData(velField, numData, rng, m_centerz, ringData);
        m_cappi.getCylindricalAzimuthPosition(numData, rng, m_centerz, ringAzi);
		Coefficient* coeff = new Coefficient[20];
		float vtdDev;
		if(gbvtd->analyzeRing(m_centerx, m_centery, rng, m_centerz, numData, ringData, ringAzi, coeff, vtdDev)){
			if(coeff[0].getParameter()=="VTC0"){
				vt.push_back(coeff[0].getValue());
				vt_rng.push_back(rng);
			}
		}
		delete[] ringAzi;
		delete[] ringData;
		delete[] coeff;
	}
	if(vt.size()<15) {
		// std::cout<<std::endl;
		return 0.f;
	}
	
	//Compute hvvp first
	float cc0, cc6, vt_std, hvvp_std;
	if(!hvvp->computeCrossBeamWind(m_centerz, cc0, cc6, hvvp_std)) {
		// std::cout<<std::endl;
		return 0.f;
	}
	// printf("cc0=%5.2f, cc6=%5.2f, ", cc0, cc6);
	
	//Iterate through all possible values of cross-beam wind
	arma::fmat A(vt.size(), 2);
	for(int ii=0; ii<vt.size(); ++ii){
		A(ii,0) = log(Rt/vt_rng[ii]);
		A(ii,1) = 1;
	}
	
	std::vector<float> guessWinds;
	for(float currentWind=-fabs(guessMax); currentWind<fabs(guessMax)+1.; currentWind+=1.)
		guessWinds.push_back(currentWind);
	
	std::vector<bool> flag(guessWinds.size(), false);
	arma::fmat B(vt.size(), guessWinds.size());
	B.fill(0.f);
	arma::fvec b(vt.size());
	for(std::vector<float>::iterator it=guessWinds.begin(); it!=guessWinds.end(); ++it){
		int idx = std::distance(guessWinds.begin(), it);
		for(int ii=0; ii<vt.size(); ++ii)
			b(ii) = vt[ii]-*it*vt_rng[ii]/Rt;
		if( b.min()>0.f){
			B.col(idx) = arma::log(b);
			flag[idx]  = true;
		}
	}
	arma::fmat X=arma::solve(A, B);
	arma::fmat E=(A*X-B);
	vt_std = sqrt(arma::accu(arma::square(E))/E.size());
	// printf("min_Xt=%5.2f, max_Xt=%5.2f, ", arma::min(X.row(0)), arma::max(X.row(0)));
	
	//Compare results and find the best one
	float curDev=999., tmpBest=0., tmpXt=999.;
	for(std::vector<float>::iterator it=guessWinds.begin(); it!=guessWinds.end(); ++it){
		int idx = std::distance(guessWinds.begin(), it);
		if(!flag[idx] || X(0,idx)<=0.f ) continue;
		float hvvp_vm = cc0-Rt*cc6/(X(0,idx)+1.);
		if(fabs(*it-hvvp_vm)<curDev){
			curDev  = fabs(*it-hvvp_vm);
			tmpBest = *it;
			tmpXt   = X(0,idx);
		}
	}
	
	// printf("num_vt_fit=%3d, vt_std=%5.2f, hvvp_std=%5.2f, tmpXt=%5.2f, vm=%5.2f, dev=%5.2f\n", vt.size(), vt_std, hvvp_std, tmpXt, tmpBest, curDev);
	if(curDev>4.0f)
		return 0.0f;
	else	
		return tmpBest;
 return 0.0;
#endif
}
