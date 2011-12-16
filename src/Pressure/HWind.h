/*
 *  HWind.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/06.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef HWIND_H
#define HWIND_H

#include "PressureData.h"
#include<QString>

class HWind : public PressureData
{

public:
	HWind();
	HWind(const QString& ob);
	bool readObs(const QString& ob);
	
private:
	QString obsFile;
};

#endif
