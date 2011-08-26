/*
 *  thredds_Config.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell.
 *  Copyright 2011 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "thredds_Config.h"

thredds_Config::thredds_Config(QObject *parent, const QString &filename)
: Configuration(parent, filename)
{
}

thredds_Config::~thredds_Config()
{
}

bool thredds_Config::validate()
{
	//Basic check to see if this is really a configuration file
	QDomElement root = domDoc.documentElement();
	if (root.tagName() != "thredds") {
		emit log(Message(QString("The file is not an THREDDS configuration file."),0,this->objectName(),Red,QString("Not a THREDDS file")));
		return false;
		
	}
	return true;
}
