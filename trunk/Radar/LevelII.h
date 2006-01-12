/*
 *  LevelII.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef LEVELII_H
#define LEVELII_H

#include "nexh.h"
#include "radarh.h"
#include "RadarData.h"
#include "Sweep.h"
#include "Ray.h"
#include "IO/Message.h"

class LevelII : public RadarData
{

 public:
  LevelII(VolumeInfo *volInfo);
  bool readVolume();
  Sweep addSweep();
  Ray addRay();

 private:
  nexrad_vol_scan_title *volHeader;
  digital_radar_data_header *radarHeader; 
  nexrad_message_header *msgHeader;
  bool swap_bytes;
  float* ref_data;
  float* vel_data;
  float* sw_data;
  float* decode_ref(const char *buffer, short int numGates);
  float* decode_vel(const char *buffer, short int numGates, short int velRes);
  float* decode_sw(const char *buffer,  short int numGates);
  long int volumeTime;
  short int volumeDate;
  void swapVolHeader();
  void swapRadarHeader();
  bool machineBigEndian();
  int short swap2(char *ov);
  int long swap4(char *ov);

};

#endif
