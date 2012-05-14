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
    Center();
    Center(const float& startX, const float& startY, const float& endX, const float& endY,
           const float& maxVT,  const float& level,  const float& radius);
    ~Center();

    float getStartX() const            { return _startX; }
    void  setStartX(const float& newX) { _startX=newX;}
    float getStartY() const            { return _startY; }
    void  setStartY(const float& newY) { _startY=newY;}
    float getX() const                 { return _endX;}
    void  setX(const float& newX)      { _endX=newX;  }
    float getY() const                 { return _endY;}
    void  setY(const float& newY)      { _endY=newY;  }

    float getRadius() const            { return _radius; }
    void  setRadius(float Radius)      { _radius=Radius; }
    float getLevel() const             { return _level; }
    void  setLevel(float Level)        { _level=Level; }
    float getMaxVT() const             { return _maxVT; }
    void  setMaxVT(float maxVT)        { _maxVT=maxVT; }

    bool operator == (const Center &other);
    bool isValid();

    static const float _fillv=-999.0f;

private:
    float _startX;
    float _startY;
    float _endX;
    float _endY;
    float _radius;
    float _level;
    float _maxVT;

};
#endif
