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
  level = 0;
  radius = 0;
  value = 0;
  parameter = QString("NULL");
}

Coefficient::Coefficient(const Coefficient &other)
{
  level = other.getLevel();
  radius = other.getRadius();
  value = other.getValue();
  parameter = other.getParameter();
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

void Coefficient::operator = (const Coefficient &other)
{
  level = other.getLevel();
  radius = other.getRadius();
  value = other.getValue();
  parameter = other.getParameter();
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
