/*
 *  mgbvtd.h
 *  vortrac
 *
 *  Created by Xiaowen Tang on 5/28/13.
 *  Copyright 2013 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef MGBVTD_H
#define MGBVTD_H

#include "GriddedData.h"
#include "Hvvp.h"
#include "gbvtd.h"

#include <QString>

class MGBVTD
{
public:
	MGBVTD(float x0, float y0, float hgt, float rmw, GriddedData& cappi);
	float computeCrossBeamWind(float guessMax, QString& velField, GBVTD* gbvtd, Hvvp* hvvp);

private:
	GriddedData& m_cappi;
	float m_centerx;
	float m_centery;
	float m_centerz;
	float m_rmw;

};
#endif
