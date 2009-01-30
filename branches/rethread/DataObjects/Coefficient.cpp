/*
 *  Coefficient.cpp
 *  VORTRAC
 *
 *  Created by Lisa Mauger on 5/24/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "Coefficient.h"

Coefficient::Coefficient()
{
  level = -999;
  radius = -999;
  value = -999;
  parameter = QString("NULL");
}

Coefficient::Coefficient(float newLevel, float newRadius, float newValue,
			 QString name)
{
  level = newLevel;
  radius = newRadius;
  value = newValue;
  parameter = name;

}
  
void Coefficient::setLevel(const float &newLevel)
{
  level = newLevel;
}

void Coefficient::setRadius(const float &newRadius)
{
  radius = newRadius;
}

void Coefficient::setValue(const float &newValue)
{
  value = newValue;
}

void Coefficient::setParameter(const QString &newParameter)
{
  parameter = newParameter;
}

bool Coefficient::operator == (const Coefficient &other)
{
  if(level == other.getLevel())
    if(radius == other.getRadius())
      if(parameter == other.getParameter())
	if(value == other.getValue())
	  return true;
  return false;
}

bool Coefficient::isNull()
{
  if((level == -999)||(radius == -999)||(value == -999))
    return true;
  return false;

}
