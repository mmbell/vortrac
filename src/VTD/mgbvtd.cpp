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
	float Rt = sqrt(m_centerx*m_centerx+m_centery*m_centery);
	float curDev = 999.;
	float tmpBest= 0.;
	for(float currentWind=-fabs(guessMax); currentWind<fabs(guessMax)+0.1; currentWind+=0.1){
		std::vector<float> vt;
		std::vector<float> vt_rng;
		//1. compute Vt radial profile
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
			delete[] coeff;
		}
		//2. fit Xt using Vt profile
		arma::fmat A(vt.size(), 2);
		arma::fvec b(vt.size());
		for(int ii=0; ii<vt.size(); ++ii){
			A(ii,0) = log(Rt/vt_rng[ii]);
			A(ii,1) = 1;
			b(ii)   = log(vt[ii]);
		}
		arma::fvec x=arma::solve(A, b);

		//3. do HVVP
		float hvvp_crossBeamWind = hvvp->computeCrossBeamWind(m_centerz, x[0]);
		if(fabs(currentWind-hvvp_crossBeamWind)<curDev){
			curDev  = fabs(currentWind-hvvp_crossBeamWind);
			tmpBest = hvvp_crossBeamWind;
		}
	}
	return tmpBest;
}