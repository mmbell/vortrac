/*
 *  NcdcLevelII.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef NCDCLEVELII_H
#define NCDCLEVELII_H

#include "LevelII.h"

class NcdcLevelII : public LevelII
{

public:
    NcdcLevelII(const QString &radarname, const float &lat, const float &lon, const QString &filename);
    ~NcdcLevelII();
    bool readVolume();

};

#endif
