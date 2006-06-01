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

Center::Center(const float& newX, const float& newY, const float& vel,
	       const float& newLevel, const float& newRadius)
{
  x = newX;
  y = newY;
  radius = newRadius;
  level = newLevel;
  maxVT = vel;

}

Center::~Center()
{

}
  
void Center::setX(const float &newX)
{
  x = newX;
}

void Center::setY(const float &newY)
{
  y = newY;
}

void Center::setRadius(const float &newRadius)
{
  radius = newRadius;
}

void Center::setLevel(const float &newLevel)
{
  level = newLevel;
}

void Center::setMaxVT(const float &newMaxVT)
{
  maxVT = newMaxVT;
}

bool Center::operator == (const Center &other)
{
  if(x == other.getX())
    if(y == other.getY())
      if(radius == other.getRadius())
	if(level == other.getLevel())
	  if(maxVT == other.getMaxVT())
	    return true;
  return false;
}

bool Center::isNull()
{
  if((x == velNull)||(y == velNull)||(radius == velNull)||(level == velNull)
     ||(maxVT == velNull))
    return true;
  return false;

}
