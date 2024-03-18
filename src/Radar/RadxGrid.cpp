#include <iostream>

#include <Ncxx/Nc3xFile.hh>
#include <Radx/RadxTime.hh>
#include "RadxGrid.h"

RadxGrid::RadxGrid(const QString &radarname, const float &lat, const float &lon,
                   const QString &filename) : RadarData(radarname, lat, lon, filename)
{
  numSweeps = -1;
  numRays = -1;
  Sweeps = NULL;
  Rays = NULL;
}

RadxGrid::~RadxGrid() {
}

// We are dealing with pre-gridded data. So no such thing as a volume, sweeps, rays...
// TODO: Move RadxGrid stuff from CappiGrid.cpp to here.

bool RadxGrid::readVolume() {
  
  // Need to set the volume date

  Nc3Error ncError(Nc3Error::verbose_nonfatal); // Prevent error from exiting the program

  Nc3File file(radarFileName.toLatin1().data(), Nc3File::ReadOnly);

  if (! file.is_valid() ) {
    std::cerr << "ERROR - reading file: " << radarFileName.toLatin1().data() << std::endl;
    return false;
  }

  Nc3Var *var = file.get_var("start_time");
  if (var == NULL) {
    std::cerr << "ERROR - Can't find 'start_time' in " << radarFileName.toLatin1().data() << std::endl;
    return false;
  }
  double secs;

  if ( ! var->get(&secs, 1, 0, 0, 0, 0) ) {
    std::cerr << "ERROR - Can't get value of 'start_time' in " << radarFileName.toLatin1().data() << std::endl;
    return false;
  }

  radarDateTime.setTimeSpec(Qt::UTC);
  RadxTime rtime((time_t) secs);
  radarDateTime.setTime(QTime(rtime.getHour(), rtime.getMin(), rtime.getSec()));

  return true;
}
