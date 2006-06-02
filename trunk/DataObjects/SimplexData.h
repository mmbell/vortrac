/*
 *  SimplexData.h
 *  VORTRAC
 *
 *  Created by Lisa Mauger on 5/30/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef SIMPLEXDATA_H
#define SIMPLEXDATA_H

#include "Center.h"
#include<QDateTime>

class SimplexData
{

 public:
  SimplexData();
  SimplexData(int availLevels, int availRadii, int availWaveNum);
  ~SimplexData();
  
  float getX(const int& lev, const int& rad) const;
  void setX(const int& lev, const int& rad, const float& newX);
  void setX(const float** a, const int& numLev, const int& numRad);

  float getY();
  float getY(const int& lev, const int& radius) const;
  void setY(const int& lev, const int& radius, const float& newY);
  void setY(const float** a, const int& numLev, const int& numRad);

  float getHeight(const int& i) const;
  void setHeight(const int& index, const float& height);
  void setHeight(const float* a, const int& howMany);

  float getRadius(const int& i) const;
  void setRadius(const int& index, const float& newRadius);
  void setRadius(const float* a, const int& numRad);

  float getCenterStdDev(const int& lev, const int& rad) const;
  void setCenterStdDev(const int& lev, const int& rad, const float& number);
  void setCenterStdDev(const float** a, const int& numLev, const int& numRad);

  QDateTime getTime() const;
  void setTime(const QDateTime& radartime);

  float getMaxVT(const int& lev, const int& rad) const;
  void setMaxVT(const int& lev, const int& rad, const float& vel);
  void setMaxVT(const float** a, const int& numLev, const int& numRad);

  float getVTUncertainty(const int& lev, const int& rad) const;
  void setVTUncertainty(const int& lev, const int& rad, const float& vel);
  void setVTUncertainty(const float** a, const int& numLev, const int& numRad);

  int getNumConvergingCenters(const int& lev, const int& rad) const;
  void setNumConvergingCenters(const int& lev, const int& rad, const int& num);
  void setNumConvergingCenters(const int** a, const int& numLev, 
			       const int& numRad);
 
  Center getCenter(const int& lev, const int& rad, 
		   const int& centers) const;
  void setCenter(const int& lev, const int& rad, 
		 const int& center, const Center &newCenter);

  int getNumPointsUsed() const;
  void setNumPointsUsed(const int& i);

  bool operator ==(const SimplexData &other);
  bool operator < (const SimplexData &other);
  bool operator > (const SimplexData &other);

  int getNumLevels() const {return numLevels;}
  int getNumRadii() const {return numRadii;}
  int getNumCenters() const {return numCenters;}

  bool isNull();
  bool emptyLevelRadius(const int& l, const int& r) const;

  void printString();

 private:
  static const int maxLevels = 15;
  static const int maxRadii = 30;
  static const int maxCenters = 25;
  static const float velNull = -999.;

  int numLevels;
  int numRadii;
  int numCenters;

  int numPointsUsed;

  float meanX[maxLevels][maxRadii];
  float meanY[maxLevels][maxRadii];

  float height[maxLevels];
  float radius[maxRadii];
  // These two members hold absolute values of the distances the represent
  // since the area of interest might have non-interger units, or be offset
  // physically while still occupying the lowest index

  float centerStdDeviation[maxLevels][maxRadii];

  int numConvergingCenters[maxLevels][maxRadii];
  
  QDateTime time;

  float meanVT[maxLevels][maxRadii];
  float meanVTUncertainty[maxLevels][maxRadii];

  Center centers[maxLevels][maxRadii][maxCenters];
  
};


#endif
