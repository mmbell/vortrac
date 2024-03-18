/*
 *  RadxData.cpp
 *  VORTRAC
 *
 *  Created by Bruno Melli 9/13/17
 *
 */
#include "Radx/Radx.hh"
#include "Radx/NcfRadxFile.hh"
#include "Radx/RadxVol.hh"
#include "Radx/RadxSweep.hh"
#include "Radx/RadxRay.hh"

#include "RadxData.h"

RadxData::RadxData(const QString &radarname, const float &lat, const float &lon, const QString &filename)
  : RadarData(radarname, lat, lon, filename)
{
  numSweeps = 0;
  numRays = 0;
  radarLat = lat;
  radarLon = lon;

}

RadxData::~RadxData()
{
  delete [] Sweeps;
  delete [] Rays;
  Sweeps = NULL;
  Rays = NULL;
}

// TODO
// This essentially takes an array of float from the Radx library,
// and shoves it into an array of float in the Radar object.
// See if we can change the RadarData interface to access the
// Radx data directly
// Maybe put a handle to the field instead of the arrays

float *RadxData::getRayData(RadxRay *fileRay, const char *fieldName)
{

  RadxField *field = fileRay->getField(fieldName);
  if (field == NULL)
    return NULL;

  float *retVal = new float[field->getNPoints()];
  if (retVal == NULL) {
    std::cerr << "RadxData::fillRayData: Failed to allocate data array" << std::endl;
    return NULL;
  }

  // Convert field to float32
  field->convertToFl32();
  
  // When homebrew picks up the fixed version,
  // uncomment the next line, and remove the check
  // for missingFl32.
  // field->setMissingFl32(-999.0);

  const float missing32 = field->getMissingFl32();

  const Radx::fl32 *fieldPtr = field->getDataFl32();
  for(size_t index = 0; index < field->getNPoints(); index++) {
    float val = fieldPtr[index];
    if (val == missing32)
      val = -999.0;
    retVal[index] = val;
  }
  return retVal;
}

bool RadxData::readVolume()
{
  // Read in a file using the Radx interface.
  // The file can be in any format supported by the Radx library.

  RadxFile file;
  RadxVol vol;

  QString fileName = getFileName();

  if ( ! file.isSupported(fileName.toLatin1().data()) ) {
    std::cerr << "ERROR - File '" << fileName.toLatin1().data()
	      << "' is not in a Radx supported format." << std::endl;
    return false;
  }

  file.setReadPreserveSweeps(true); // prevent Radx from tossing away long sweeps

  if (file.readFromPath(fileName.toLatin1().data(), vol) ) {
    std::cerr << "ERROR - reading file: " << fileName.toLatin1().data() << std::endl;
    std::cerr << file.getErrStr() << std::endl;
    return false;
  }

  numSweeps = vol.getNSweeps();
  numRays = vol.getNRays();

  int scanID = vol.getScanId();  // VCP

  // Allocate storage for sweeps and rays

  Sweeps = new Sweep[numSweeps];
  Rays = new Ray[numRays];

  // Iterate on the rays (since they have info we need for the sweep)

  const vector<RadxRay *> rays = vol.getRays();
  vector<RadxRay *>::const_iterator ray_it;
  int rayCount = 0;

  for(ray_it = rays.begin(); ray_it < rays.end(); ray_it++, rayCount++) {
    RadxRay *fileRay = *ray_it;
    Ray *myRay = &Rays[rayCount];

    myRay->setSweepIndex(fileRay->getSweepNumber());

    //    std::cout << "ray " << rayCount << " sweep " << fileRay->getSweepNumber() << std::endl;

    // Done in the sweep look
    // myRay->setRayIndex(rayCount);

    // Ray time
    // getTimeDouble is timeSecs + nanoSecs / 1.0e9
    // TODO: Not sure what to do with it. Is it used anywhere?

    double secs = fileRay->getTimeDouble();
    myRay->setDate((int) secs);
    double nanoSecs = fileRay->getNanoSecs();
    myRay->setTime((int) nanoSecs);

    myRay->setAzimuth(fileRay->getAzimuthDeg());
    myRay->setElevation(fileRay->getElevationDeg());
    myRay->setUnambig_range(fileRay->getUnambigRangeKm());
    myRay->setNyquist_vel(fileRay->getNyquistMps());

    // TODO: Not sure these are used
    // myRay->setVelResolution( msg1Header->velocity_resolution );
    // see velScaleBiasFactor in Radx's SweepData.hh

    myRay->setVelResolution(fileRay->getAngleResDeg());
    myRay->setVcp(scanID);

    // TODO: Would ref and vel have different number of gates and gate spacing?

    int nGates = fileRay->getNGates();
    myRay->setRef_numgates(nGates);
    myRay->setVel_numgates(nGates);
    myRay->setRef_gatesp(fileRay->getGateSpacingKm() * 1000 ); // Previous reader assumed meters
    myRay->setVel_gatesp(fileRay->getGateSpacingKm() * 1000 );
    myRay->setFirst_ref_gate(fileRay->getStartRangeKm() * 1000);
    myRay->setFirst_vel_gate(fileRay->getStartRangeKm() * 1000);

    // Fill in the ray data (ref, vel, ...)
    // With file.setReadPreserveSweeps(true) above (to match what the old reader was doing),
    //    we might have long rays that don't have VEL and SW

    myRay->setRefData(getRayData(fileRay, "REF"));
    myRay->setVelData(getRayData(fileRay, "VEL"));
    myRay->setSwData( getRayData(fileRay, "SW"));

    // Lots of algorithms (QC Cappi, can't deal with missing Vel)
    // So fill in the Velocity data with -999)

    if(myRay->getVelData() == NULL) {
      float *buffer = new float[nGates];
      for(int i = 0; i < nGates; i++)
	buffer[i] = -999;
      myRay->setVelData(buffer);
    }
  }

  // Iterate on the sweeps

  const vector<RadxSweep *>  sweeps = vol.getSweeps();
  vector<RadxSweep *>::const_iterator sweep_it;
  int sweepCount = 0;

  for (sweep_it = sweeps.begin(); sweep_it < sweeps.end(); sweep_it++, sweepCount++) {
    RadxSweep *file_sweep = *sweep_it;

    // std::cout << "Sweep " << sweepCount << " has index " << file_sweep->getSweepNumber()
    // 	      << " first: " << file_sweep->getStartRayIndex()
    // 	      << " last: " << file_sweep->getEndRayIndex()
    // 	      << std::endl;

    Sweep *my_sweep = &Sweeps[sweepCount];

    int firstRayIndex = file_sweep->getStartRayIndex();
    Ray *firstRay = &Rays[firstRayIndex];

    my_sweep->setSweepIndex(sweepCount);
    my_sweep->setFirstRay(firstRayIndex);
    my_sweep->setLastRay(file_sweep->getEndRayIndex());

    // Set the sweep index for all the rays in this sweep.
    // Also sump up the gates from all the rays in this sweep. TODO Is this correct???

    // int gateCount = 0;

    // Set the rayIndex to match what the native NcdcLevelII did
    // either radial_num, or azimuth_num depending onthe header type.
    // I don't have access to these values, but it looks like 1..n for each sweep

    int rayIndex = 1;
    for(size_t rayCount = firstRayIndex; rayCount <= file_sweep->getEndRayIndex();
	rayCount++, rayIndex++) {
      Rays[rayCount].setSweepIndex(sweepCount);
      Rays[rayCount].setRayIndex(rayIndex);
      // gateCount += Rays[rayCount].getRef_numgates();
    }

    int gateCount = firstRay->getRef_numgates();   // TODO: Is that correct?

    my_sweep->setRef_numgates(gateCount);
    my_sweep->setVel_numgates(gateCount);

    my_sweep->setElevation(firstRay->getElevation());
    my_sweep->setUnambig_range(firstRay->getUnambig_range());
    my_sweep->setFirst_ref_gate(firstRay->getFirst_ref_gate());
    my_sweep->setFirst_vel_gate(firstRay->getFirst_vel_gate());
    my_sweep->setRef_gatesp(firstRay->getRef_gatesp());
    my_sweep->setVel_gatesp(firstRay->getVel_gatesp());
    my_sweep->setNyquist_vel(firstRay->getNyquist_vel());
    my_sweep->setVcp(firstRay->getVcp());
  }

  // Apparently, Radx gives me a certain number of sweeps, but some of them are missing.
  // These are the long range sweeps that do not contain dopler velocity.
  // So adjust how many sweeps we really have

  numSweeps = sweepCount;

  // Set the volume date to the date of the first ray

  radarDateTime.setTimeSpec(Qt::UTC);

  RadxTime rtime((time_t) secs);
  radarDateTime.setTime(QTime(rtime.getHour(), rtime.getMin(), rtime.getSec()));

  radarDateTime = QDateTime::fromTime_t(Rays[0].getDate());


  return true;
}
