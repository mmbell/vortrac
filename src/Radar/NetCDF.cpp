#include <iostream>

#include <netcdfcpp.h>
#include "NetCDF.h"

NetCDF::NetCDF(const QString &radarname, const float &lat, const float &lon,
	       const QString &filename) : RadarData(radarname, lat, lon, filename)
{
  numSweeps = -1;
  numRays = -1;
  Sweeps = NULL;
  Rays = NULL;
}

NetCDF::~NetCDF() {
}

// We are dealing with pre-gridded data. So no such thing as a volume, sweeps, rays...
// TODO: Move NetCDF stuff from CappiGrid.cpp to here.

bool NetCDF::readVolume() {

  // Need to set the volume date

  NcError ncError(NcError::verbose_nonfatal); // Prevent error from exiting the program

  NcFile file(radarFileName.toLatin1().data(), NcFile::ReadOnly);
  
  if (! file.is_valid() ) {
    std::cerr << "ERROR - reading file: " << radarFileName.toLatin1().data() << std::endl;
    return false;
  }

  NcVar *var = file.get_var("start_time");
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
  radarDateTime.setTime_t(int(secs));
  
  return true;
}
