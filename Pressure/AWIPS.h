/*
 *  AWIPS.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/06.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef AWIPS_H
#define AWIPS_H

#include "PressureData.h"
#include<QString>

class AWIPS : public PressureData
{

public:
	AWIPS();
	AWIPS(const QString& ob);
	void readObs(const QString& ob);
	
private:
	QString obsFile;
};

#endif
