/*
 * Center.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/30/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef CENTER_H
#define CENTER_H

class Center
{
 public:

    Center(const float& newX = -999, const float& newY = -999,  
	   const float& maxVT = -999, const float& newLevel = -999, 
	   const float& newRadius = -999);
    ~Center();

    float getX() const { return x; }
    void setX(const float &newX);

    float getY() const { return y; }
    void setY(const float &newY);

    float getRadius() const {return radius; }
    void setRadius(const float &newRadius);

    float getLevel() const { return level; }
    void setLevel(const float &newLevel);
    
    float getMaxVT() const { return maxVT; }
    void setMaxVT(const float &newMaxVT);

    bool operator == (const Center &other);

    bool isNull();




 private:

    float velNull;

    float x;
    float y;
    float radius;
    float level;
    float maxVT;


};
#endif
