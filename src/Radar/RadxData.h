#ifndef RADXDATA_H
#define RADXDATA_H

#include "Radx/RadxRay.hh"
#include "Radar/RadarData.h"

class RadxData : public RadarData
{
 public:
  
  RadxData(const QString &radarname, const float &lat, const float &lon,
	 const QString &filename);
  ~RadxData();

  bool readVolume();
  float *getRayData(RadxRay *fileRay, const char *fieldName);
  
};

#endif
