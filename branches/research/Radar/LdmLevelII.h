/*
 *  LdmLevelII.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef LDMLEVELII_H
#define LDMLEVELII_H

#include "LevelII.h"
#include <bzlib.h>

class LdmLevelII : public LevelII
{

 public:
  LdmLevelII(const QString &radarname, const float &lat, const float &lon, const QString &filename);
  bool readVolume();
  
};

#endif
