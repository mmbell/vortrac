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
#include "RadarData.h"
#include "Sweep.h"
#include "Ray.h"
#include "IO/Message.h"

class LevelII : public RadarData
{

 public:
  LevelII(const QString &radarname, const float &lat, const float &lon, const QString &filename);
  virtual ~LevelII();
  virtual bool readVolume() = 0;
  void addSweep(Sweep* newSweep);
  void addRay(Ray* newRay);

 protected:
  nexrad_vol_scan_title *volHeader;
  message_1_data_header *msg1Header; 
  nexrad_message_header *msgHeader;
  message_31_data_header *msg31Header;
  volume_data_block *volume_block;
  radial_data_block *radial_block;
  moment_data_block *ref_block;
  moment_data_block *vel_block;
  moment_data_block *sw_block;
  
  bool swap_bytes;
  int sweepMsgType;
  float* ref_data;
  float* vel_data;
  float* sw_data;
  int ref_gate1;
  int vel_gate1;
  int ref_gate_width;
  int vel_gate_width;
  int ref_num_gates;
  int vel_num_gates;
  
  void decode_ref(Ray* newRay, const char *buffer, short int numGates);
  void decode_vel(Ray* newRay, const char *buffer, short int numGates, short int velRes);
  void decode_sw(Ray* newRay, const char *buffer,  short int numGates);
  long int volumeTime;
  short int volumeDate;
  void swapVolHeader();
  void swapMsg1Header();
  void swapMsgHeader();
  void swapMsg31Header();
  void swapVolumeBlock();
  void swapRadialBlock();
  void swapMomentDataBlock(moment_data_block* data_block);
  bool machineBigEndian();
  int short swap2(char *ov);
  int long swap4(char *ov);
  float swap4f(float swapme);
};

#endif
