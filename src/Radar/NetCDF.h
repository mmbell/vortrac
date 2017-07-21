#ifndef NetCDF_H
#define NetCDF_H
#include "RadarData.h"

class NetCDF : public RadarData
{
 public:
  
  NetCDF(const QString &radarname, const float &lat, const float &lon,
	 const QString &filename);
  ~NetCDF();

  bool readVolume();

};

#endif
