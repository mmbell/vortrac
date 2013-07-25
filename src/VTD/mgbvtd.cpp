/*
 *  mgbvtd.cpp
 *  vortrac
 *
 *  Created by Xiaowen Tang on 5/28/13.
 *  Copyright 2013 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */
#include <vector>
#include <algorithm>
#include <cmath>
#include <armadillo>
#include "mgbvtd.h"

MGBVTD::MGBVTD(float x0, float y0, float hgt, float rmw, GriddedData& cappi):
m_cappi(cappi)
{
	m_centerx = x0;
	m_centery = y0;
	m_centerz = hgt;
	m_rmw     = rmw;
}

float MGBVTD::computeCrossBeamWind(float guessMax, QString& velField, GBVTD* gbvtd, Hvvp* hvvp)
{
	const float Rt = sqrt(m_centerx*m_centerx+m_centery*m_centery);

	//1. calculate Vt profile first
	std::vector<float> vt;
	std::vector<float> vt_rng;
	//1. compute the radial profile of symmetric tangential wind  
	for(float rng=m_rmw; rng<=1.7*m_rmw; rng+=1.){
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

	//Compute hvvp first
	float cc0, cc6, vt_std, hvvp_std;
	if(!hvvp->computeCrossBeamWind(m_centerz, cc0, cc6, hvvp_std))
		return 0.f;
	
	//Iterate through all possible values of cross-beam wind
	arma::fmat A(vt.size(), 2);
	for(int ii=0; ii<vt.size(); ++ii){
		A(ii,0) = log(Rt/vt_rng[ii]);
		A(ii,1) = 1;
	}
	
	std::vector<float> guessWinds;
	for(float currentWind=-fabs(guessMax); currentWind<fabs(guessMax)+0.5; currentWind+=0.5)
		guessWinds.push_back(currentWind);
	
	arma::fmat B(vt.size(), guessWinds.size());
	for(std::vector<float>::iterator it=guessWinds.begin(); it!=guessWinds.end(); ++it){
		for(int ii=0; ii<vt.size(); ++ii){
			B(ii, std::distance(guessWinds.begin(), it)) = log(vt[ii]-*it*vt_rng[ii]/Rt);
		}
	}
	arma::fmat X=arma::solve(A, B);
	arma::fmat E=(A*X-B);
	vt_std = sqrt(arma::accu(arma::square(E))/E.size());
	
	//Compare results and find the best one
	float curDev=999., tmpBest=0.;
	for(std::vector<float>::iterator it=guessWinds.begin(); it!=guessWinds.end(); ++it){
		int idx = std::distance(guessWinds.begin(), it);
		float hvvp_vm = cc0-Rt*cc6/(X(0,idx)+1.);
		if(fabs(*it-hvvp_vm)<curDev){
			curDev  = fabs(*it-hvvp_vm);
			tmpBest = *it;
		}
	}
	
	printf("num_vt_fit=%3d, vt_std=%5.2f, hvvp_std=%5.2f, vm=%5.2f, dev=%5.2f\n", vt.size(), vt_std, hvvp_std, tmpBest, curDev);
		
	return tmpBest;
}