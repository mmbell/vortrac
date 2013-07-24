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

	std::cout<<vt.size()<<", "<<*std::min_element(vt.begin(), vt.end())<<", "<<*std::max_element(vt.begin(), vt.end())<<std::endl;

	//Iterate through all possible values of cross-beam wind
	float curDev = 999.;
	float tmpBest= 0.;
	for(float currentWind=-fabs(guessMax); currentWind<fabs(guessMax)+0.5; currentWind+=0.5){

		// fit Xt using Vt profile
		arma::fmat A(vt.size(), 2);
		arma::fvec b(vt.size());
		for(int ii=0; ii<vt.size(); ++ii){
			A(ii,0) = log(Rt/vt_rng[ii]);
			A(ii,1) = 1;
			b(ii)   = log(vt[ii]-currentWind*vt_rng[ii]/Rt);
		}
		arma::fvec x=arma::solve(A, b);

		// do HVVP
		float hvvp_crossBeamWind = hvvp->computeCrossBeamWind(m_centerz, x[0]);
		//std::cout<<currentWind<<", "<<x[0]<<", "<<hvvp_crossBeamWind<<std::endl;
		if(fabs(currentWind-hvvp_crossBeamWind)<curDev){
			curDev  = fabs(currentWind-hvvp_crossBeamWind);
			tmpBest = currentWind;
		}
	}
	std::cout<<curDev<<std::endl;
	return tmpBest;
}