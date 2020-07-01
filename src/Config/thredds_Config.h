/*
 *  thredds_Config.h
 *  VORTRAC
 *
 *  Created by Michael Bell.
 *  Copyright 2011 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef THREDDSCONFIG_H
#define THREDDSCONFIG_H

#include "Config/Configuration.h"

#include <QDomDocument>

class thredds_Config : public Configuration
{

	public:
	thredds_Config(QObject *parent, const QString &filename);
	~thredds_Config();
	bool validate();
};

#endif


