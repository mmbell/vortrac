/*
 *  Metar.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/06.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef METAR_H
#define METAR_H

#include "PressureData.h"
#include<QString>

class Metar : public PressureData
{

public:
	Metar();
	Metar(const QString& filename);
	bool readObs();
	
private:
	QString obsFile;
};

#endif
