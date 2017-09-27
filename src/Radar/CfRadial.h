#ifndef CfRadial_H
#define CfRadial_H
#include "RadarData.h"

class CfRadial : public RadarData
{
 public:

  CfRadial(const QString &radarname, const float &lat, const float &lon,
	 const QString &filename);
  ~CfRadial();

  bool readVolume();

};

#endif
