/*
 *  Coefficient.h
 *  VORTRAC
 *
 *  Created by Lisa Mauger on 5/24/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef COEFFICIENT_H
#define COEFFICIENT_H

#include <QString>

class Coefficient 
{

public:
    Coefficient();
    Coefficient(float newLevel, float newRadius, float newValue, QString name);
    Coefficient(const Coefficient &other);

    bool isValid() const;
    
    float getLevel() const { return level; }
    void setLevel(const float &newLevel);

    float getRadius() const {return radius; }
    void setRadius(const float &newRadius);

    float getValue() const { return value; }
    void setValue(const float &newValue);

    QString getParameter() const { return parameter; }
    void setParameter(const QString &newParameter);

    bool operator == (const Coefficient &other);

    bool isNull();


private:
    float level;
    float radius;
    float value;
    QString parameter;

};

#endif
