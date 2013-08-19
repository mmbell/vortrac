/*
 *  pca.h
 *  vortrac
 *
 *  Created by Xiaowen Tang on 8/18/13.
 *  Copyright 2013 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef pca_h
#define pca_h

#include "Radar/RadarData.h"
#include <armadillo>

class pca
{
public:
	pca(RadarData& vol);
	pca();
	bool findCenter(float& azCenter, float& rgCenter);
	void simulateVortex(float azCenter, float rgCenter, float rmw);

private:
	arma::mat vr;
	arma::vec az,rg;

};
#endif