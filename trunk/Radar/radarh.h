
// Useful structures for radar data

#ifndef RADARH_H
#define RADARH_H

#include <QString>
#include <QFile>
#include <QDateTime>


enum dataFormat {
  levelII,
  dorade,
  netcdf
};

struct VolumeInfo {
  QString radarName;
  float radarLat;
  float radarLon;
  QDateTime radarDateTime;
  QFile radarFile;
  dataFormat radarFormat;
};

#endif
