/*
 * Center.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/30/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "Center.h"
const float Center::_fillv;

Center::Center(const float& startX, const float& startY, const float& endX, const float& endY,
               const float& maxVT,  const float& level,  const float& radius)
{
    _startX=startX;
    _startY=startY;
    _endX  =endX;
    _endY  =endY;
    _maxVT =maxVT;
    _level =level;
    _radius=radius;
}

Center::Center()
{
    _startX=_fillv;
    _startY=_fillv;
    _endX  =_fillv;
    _endY  =_fillv;
    _maxVT =_fillv;
    _level =_fillv;
    _radius=_fillv;
}

Center::~Center()
{

}

bool Center::operator == (const Center &other)
{
    return (_startX==other.getStartX())&&(_startY==other.getStartY())&&(_endX==other.getX())&&(_endY==other.getY())&&
            (_radius == other.getRadius())&&(_level == other.getLevel());
}

bool Center::isValid()
{
    if((_endX==_fillv)||(_endY==_fillv))
        return false;
    return true;
}
