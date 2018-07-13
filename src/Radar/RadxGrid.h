#ifndef RadxGrid_H
#define RadxGrid_H
#include "RadarData.h"

class RadxGrid : public RadarData
{
 public:

  RadxGrid(const QString &radarname, const float &lat, const float &lon,
	 const QString &filename);
  ~RadxGrid();

  bool readVolume();

};

#endif
