/*
 *  MADIS.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/06.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef MADISOB_H
#define MADISOB_H

#include "PressureData.h"
#include<QString>

class MADIS : public PressureData
{

public:
	MADIS();
	MADIS(const QString& ob);
	void readObs(const QString& ob);
	
private:
	QString obsFile;
};

#endif
