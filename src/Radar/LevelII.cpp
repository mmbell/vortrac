/*
 *  LevelII.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "LevelII.h"
#include "RadarQC.h"
#include <unistd.h>

LevelII::LevelII(const QString &radarname, const float &lat, const float &lon, const QString &filename) 
	: RadarData(radarname, lat, lon, filename)
{

  numSweeps = 0;
  numRays = 0;
  volHeader = new nexrad_vol_scan_title;
  //  msg1Header = new digital_radar_data_header;
  msgHeader = NULL;
  //  msgHeader = new nexrad_message_header;
  msg1Header = NULL;
  msg31Header = NULL;
  Sweeps = new Sweep[20];
  Rays = new Ray[15000];
  swap_bytes = false;
  vel_data = NULL;
  sw_data = NULL;
  ref_data = NULL;
}

LevelII::~LevelII()
{
  delete volHeader;
  delete [] Sweeps;
  delete [] Rays;
  msgHeader = NULL;
  msg1Header = NULL;
  msg31Header = NULL;
  vel_data = NULL;
  sw_data = NULL;
  ref_data = NULL;
}

bool LevelII::readVolume()
{

  // Read in a level II file
  // Virtual function
	return false;
}

void LevelII::addSweep(Sweep* newSweep)
{

  // Add a Sweep to the volume
  // Already allocated memory for the sweep in the LevelII constructor
  //Sweep *newSweep = new Sweep;
  numSweeps += 1;
  newSweep->setSweepIndex( numSweeps -1 );
  newSweep->setFirstRay(numRays);
  if (sweepMsgType == 1) {
	newSweep->setElevation( (msg1Header->elevation) * NEX_FIXED_ANGLE );
	newSweep->setUnambig_range( (msg1Header->unamb_range_x10) / 10.0 );
	newSweep->setNyquist_vel( (msg1Header->nyquist_vel_x100) / 100 );
	newSweep->setFirst_ref_gate( msg1Header->ref_gate1 );
	newSweep->setFirst_vel_gate( msg1Header->vel_gate1 );
	newSweep->setRef_gatesp( msg1Header->ref_gate_width );
	newSweep->setVel_gatesp( msg1Header->vel_gate_width );
	newSweep->setRef_numgates( msg1Header->ref_num_gates );
	newSweep->setVel_numgates( msg1Header->vel_num_gates );
	newSweep->setVcp( msg1Header->vol_coverage_pattern );
  } else if (sweepMsgType == 31) {
	newSweep->setElevation( msg31Header->elevation);
	newSweep->setUnambig_range( radial_block->unambig_range / 10.0 );
	newSweep->setNyquist_vel( radial_block->nyquist_vel / 100.0 );
	newSweep->setFirst_ref_gate( ref_gate1 );
	newSweep->setFirst_vel_gate( vel_gate1 );
	newSweep->setRef_gatesp( ref_gate_width );
	newSweep->setVel_gatesp( vel_gate_width );
	newSweep->setRef_numgates( ref_num_gates );
	newSweep->setVel_numgates( vel_num_gates );
	newSweep->setVcp( volume_block->vol_coverage_pattern );
  }
  
  //return *newSweep;
}

void LevelII::addRay(Ray* newRay)
{

  // Add a new ray to the sweep
  //Ray *newRay = new Ray();
  numRays++;
  newRay->setSweepIndex( (numSweeps - 1) );
  
  if (sweepMsgType == 1) {
	newRay->setTime( msg1Header->milliseconds_past_midnight );
	newRay->setDate( msg1Header->julian_date );
	newRay->setAzimuth( (msg1Header->azimuth) * NEX_FIXED_ANGLE );
	newRay->setElevation( (msg1Header->elevation) * NEX_FIXED_ANGLE );
	newRay->setVelResolution( msg1Header->velocity_resolution );
	newRay->setRayIndex( msg1Header->radial_num );
	newRay->setUnambig_range( float(msg1Header->unamb_range_x10) / 10.0 );
	newRay->setNyquist_vel( float(msg1Header->nyquist_vel_x100) / 100 );
	newRay->setFirst_ref_gate( msg1Header->ref_gate1 );
	newRay->setFirst_vel_gate( msg1Header->vel_gate1 );
	newRay->setRef_gatesp( msg1Header->ref_gate_width );
	newRay->setVel_gatesp( msg1Header->vel_gate_width );
	newRay->setRef_numgates( msg1Header->ref_num_gates );
	newRay->setVel_numgates( msg1Header->vel_num_gates );
	newRay->setVcp( msg1Header->vol_coverage_pattern );
  } else if (sweepMsgType == 31) {
	newRay->setTime( msg31Header->milliseconds_past_midnight );
	newRay->setDate( msg31Header->julian_date );
	newRay->setAzimuth( msg31Header->azimuth );
	newRay->setElevation( msg31Header->elevation );
	newRay->setVelResolution( 2 );
	newRay->setRayIndex( msg31Header->azimuth_num );
	newRay->setUnambig_range( radial_block->unambig_range / 10.0);
	newRay->setNyquist_vel( radial_block->nyquist_vel / 100.0);
	newRay->setFirst_ref_gate( ref_gate1 );
	newRay->setFirst_vel_gate( vel_gate1 );
	newRay->setRef_gatesp( ref_gate_width );
	newRay->setVel_gatesp( vel_gate_width );
	newRay->setRef_numgates( ref_num_gates );
	newRay->setVel_numgates( vel_num_gates );
	newRay->setVcp( volume_block->vol_coverage_pattern );
  }
  /* newRay->setRefData( ref_data );
  newRay->setVelData( vel_data );
  newRay->setSwData( sw_data );

  vel_data = NULL;
  sw_data = NULL;
  ref_data = NULL; */

  //return *newRay;
}

void LevelII::decode_ref(Ray* newRay, const char *buffer, short int numGates)
{

  newRay->allocateRefData( numGates );
  float* refArray = newRay->getRefData();
  // Decode each byte
  for (short int i = 0; i <= numGates - 1; i++) {
    unsigned char encoded = (unsigned char)buffer[i];
    if (encoded == 0) {
      refArray[i] = -999;
    } else if (encoded == 1) {
      // Ambiguous, set to bad for now
      refArray[i] = -999;
    } else {
      refArray[i] = (((float)encoded - 2.)/2.) - 32.0;
    }
  }

}

void LevelII::decode_vel(Ray* newRay, const char *buffer, short int numGates,
			   short int velRes)
{

  newRay->allocateVelData( numGates );
  float* velArray = newRay->getVelData();
  // Decode each byte
  for (int i = 0; i <= numGates - 1; i++) {
    unsigned char encoded = (unsigned char)buffer[i];
    if (encoded == 0) {
      velArray[i] = -999;
    } else if (encoded == 1) {
      velArray[i] = -999;
    } else {
      if (velRes == 2) {
	velArray[i] = (((float)encoded - 2.)/2.) - 63.5;
      } else {
	velArray[i] = ((float)encoded - 2.) - 127.0;
      }
    }
  }

}

void LevelII::decode_sw(Ray* newRay, const char *buffer, short int numGates)
{

  newRay->allocateSwData( numGates );
  float* swArray = newRay->getSwData();
  // Decode each byte
  for (int i = 0; i <= numGates - 1; i++) {
    unsigned char encoded = (unsigned char)buffer[i];
    if (encoded == 0) {
      swArray[i] = -999;
    } else if (encoded == 1) {
      swArray[i] = -999;
    } else {
      swArray[i] = (((float)encoded - 2.)/2.) - 63.5;
    }
  }

}

void LevelII::swapVolHeader()
{

  swab((char *)&volHeader->julian_date, (char *)&volHeader->julian_date, 4);
  swab((char *)&volHeader->milliseconds_past_midnight, 
       (char *)&volHeader->milliseconds_past_midnight, 4);

}

void LevelII::swapMsg1Header()
{

  msg1Header->milliseconds_past_midnight = 
    swap4((char *)&msg1Header->milliseconds_past_midnight); /* (15-16) */
  swab((char *)&msg1Header->julian_date,
       (char *)&msg1Header->julian_date, 2);          /* (17) from 1/1/70 */
  swab((char *)&msg1Header->unamb_range_x10,
       (char *)&msg1Header->unamb_range_x10, 2);      /* (18) km. */
  swab((char *)&msg1Header->azimuth,
       (char *)&msg1Header->azimuth,2);     /* (19) binary angle */
  swab((char *)&msg1Header->radial_num,
       (char *)&msg1Header->radial_num, 2);           /* (20) */
  swab((char *)&msg1Header->radial_status,
       (char *)&msg1Header->radial_status, 2);        /* (21) */
  swab((char *)&msg1Header->elevation,
       (char *)&msg1Header->elevation, 2);   /* (22) binary angle */
  swab((char *)&msg1Header->elev_num,
       (char *)&msg1Header->elev_num, 2);             /* (23) */
  swab((char *)&msg1Header->ref_gate1,
       (char *)&msg1Header->ref_gate1, 2);            /* (24) meters */
  swab((char *)&msg1Header->vel_gate1,
       (char *)&msg1Header->vel_gate1, 2);            /* (25) meters */
  swab((char *)&msg1Header->ref_gate_width,
       (char *)&msg1Header->ref_gate_width, 2);       /* (26) meters */
  swab((char *)&msg1Header->vel_gate_width, 
       (char *)&msg1Header->vel_gate_width, 2);       /* (27) meters */
  swab((char *)&msg1Header->ref_num_gates,
       (char *)&msg1Header->ref_num_gates, 2);        /* (28) */
  swab((char *)&msg1Header->vel_num_gates,
       (char *)&msg1Header->vel_num_gates, 2);        /* (29) */
  swab((char *)&msg1Header->sector_num,
       (char *)&msg1Header->sector_num, 2);           /* (30) */
  // float sys_gain_cal_const;   /* (31-32) */
  swab((char *)&msg1Header->ref_ptr,
       (char *)&msg1Header->ref_ptr, 2);              /* (33) byte count from start of drdh */
  swab((char *)&msg1Header->vel_ptr,
       (char *)&msg1Header->vel_ptr, 2);              /* (34) byte count from start of drdh */
  swab((char *)&msg1Header->sw_ptr,
       (char *)&msg1Header->sw_ptr, 2);               /* (35) byte count from start of drdh */
  swab((char *)&msg1Header->velocity_resolution,
       (char *)&msg1Header->velocity_resolution, 2);  /* (36) */
  swab((char *)&msg1Header->vol_coverage_pattern,
       (char *)&msg1Header->vol_coverage_pattern, 2); /* (37) */
  // swab((char *)&msg1Header->VNV1;                 /* V & V simulator reserved */
  // swab((char *)&msg1Header->VNV2;
  // swab((char *)&msg1Header->VNV3;
  // swab((char *)&msg1Header->VNV4;
  swab((char *)&msg1Header->ref_data_playback,
       (char *)&msg1Header->ref_data_playback, 2);    /* (42) */
  swab((char *)&msg1Header->vel_data_playback,
       (char *)&msg1Header->vel_data_playback, 2);    /* (43) */
  swab((char *)&msg1Header->sw_data_playback,
       (char *)&msg1Header->sw_data_playback, 2);     /* (44) */
  swab((char *)&msg1Header->nyquist_vel_x100,
       (char *)&msg1Header->nyquist_vel_x100, 2);     /* (45)m/s */
  swab((char *)&msg1Header->atmos_atten_factor_x1000,
       (char *)&msg1Header->atmos_atten_factor_x1000, 2); /* (46) dB/km */
  swab((char *)&msg1Header->threshold_parameter, 
       (char *)&msg1Header->threshold_parameter, 2);  /* (47) */
  
}

void LevelII::swapMsgHeader()
{

    swab((char *)&msgHeader->message_len,
		 (char *)&msgHeader->message_len, 2);          /* in 16-bit words */
    // OK unsigned char channel_id;
    // OK unsigned char message_type;
	swab((char *)&msgHeader->seq_num,
		 (char *)&msgHeader->seq_num, 2);              /* mod 0x7fff */
	swab((char *)&msgHeader->julian_date,
		 (char *)&msgHeader->julian_date, 2);          /* from 1/1/70 */
    msgHeader->milliseconds_past_midnight = swap4((char *)&msgHeader->milliseconds_past_midnight);
	swab((char *)&msgHeader->num_message_segs,
		 (char *)&msgHeader->num_message_segs, 2);
	swab((char *)&msgHeader->message_seg_num,
		 (char *)&msgHeader->message_seg_num, 2);
		 
}

void LevelII::swapMsg31Header()
{

	msg31Header->milliseconds_past_midnight
		= swap4((char *)&msg31Header->milliseconds_past_midnight);; 
    swab((char *)&msg31Header->julian_date,
		(char *)&msg31Header->julian_date, 2);          /* (17) from 1/1/70 */
    swab((char *)&msg31Header->azimuth_num,
		(char *)&msg31Header->azimuth_num, 2);      /* (18) */
	long tempint = swap4((char *)&msg31Header->azimuth);
	msg31Header->azimuth = *(float *)&tempint;
		
    swab((char *)&msg31Header->block_length,
		(char *)&msg31Header->block_length, 2);           /* (20) */

	tempint	= swap4((char *)&msg31Header->elevation);
	msg31Header->elevation = *(float *)&tempint;
	
	swab((char *)&msg31Header->num_data_blocks,
		(char *)&msg31Header->num_data_blocks, 2);
	msg31Header->vol_ptr
		= swap4((char *)&msg31Header->vol_ptr);
	msg31Header->elev_ptr
		= swap4((char *)&msg31Header->elev_ptr);
	msg31Header->radial_ptr
		= swap4((char *)&msg31Header->radial_ptr);
    msg31Header->ref_ptr
		= swap4((char *)&msg31Header->ref_ptr);              /* (33) byte count from start of drdh */
    msg31Header->vel_ptr
		= swap4((char *)&msg31Header->vel_ptr);              /* (34) byte count from start of drdh */
    msg31Header->sw_ptr
		= swap4((char *)&msg31Header->sw_ptr);               /* (35) byte count from start of drdh */
	msg31Header->zdr_ptr
		= swap4((char *)&msg31Header->zdr_ptr);
	msg31Header->phi_ptr
		= swap4((char *)&msg31Header->phi_ptr);
	msg31Header->rho_ptr
		= swap4((char *)&msg31Header->rho_ptr);
		
}

void LevelII::swapVolumeBlock()
{
	swab((char *)&volume_block->block_size,
		(char *)&volume_block->block_size, 2);
		
	long tempint = swap4((char *)&volume_block->latitude);
	volume_block->latitude = *(float *)&tempint;
	tempint = swap4((char *)&volume_block->longitude);
	volume_block->longitude = *(float *)&tempint;

	swab((char *)&volume_block->altitude,
		(char *)&volume_block->altitude, 2);
	swab((char *)&volume_block->feedhorn_height,
		(char *)&volume_block->feedhorn_height, 2);
//	float calibration_constant;
//	float tx_power_h;
//	float tx_power_v;
//	float zdr_calibration;
//	float initial_phidp;
	swab((char *)&volume_block->vol_coverage_pattern,
		(char *)&volume_block->vol_coverage_pattern, 2);
	// short spare;
}

void LevelII::swapRadialBlock()
{
	swab((char *)&radial_block->block_size,
		(char *)&radial_block->block_size, 2);
	swab((char *)&radial_block->unambig_range,
		(char *)&radial_block->unambig_range, 2);
//	float h_noise;
//	float v_noise;
	swab((char *)&radial_block->nyquist_vel,
		(char *)&radial_block->nyquist_vel, 2);
//	short spare;

}

void LevelII::swapMomentDataBlock(moment_data_block* data_block)
{
	swab((char *)&data_block->num_gates,
		(char *)&data_block->num_gates, 2);
	swab((char *)&data_block->gate1,
		(char *)&data_block->gate1, 2);
	swab((char *)&data_block->gate_width,
		(char *)&data_block->gate_width, 2);
//	short tover;
//	short snr_threshold;
	long tempint = swap4((char *)&data_block->scale);
	data_block->scale = *(float *)&tempint;
	tempint = swap4((char *)&data_block->doffset);
	data_block->doffset = *(float *)&tempint;
}

bool LevelII::machineBigEndian(){

    union {
	unsigned char byte[4];
	int val;
    }word;

    word.val = 0;
    word.byte[3] = 0x01;
    
    return word.val == 1;
}

int short LevelII::swap2(char *ov)		/* swap integer*2 */
{
    union {
	int short newval;
	char nv[2];
    }u;
    u.nv[1] = *ov++; u.nv[0] = *ov++;
    return(u.newval);
}

int long LevelII::swap4(char *ov )		/* swap integer*4 */
{
   union {
     int long newval;
     char nv[4];
   }u;
 
   u.nv[3] = *ov++; u.nv[2] = *ov++; u.nv[1] = *ov++; u.nv[0] = *ov++;

   return(u.newval);
}

float LevelII::swap4f(float swapme)            /* swap real*4 */
{
    double d;
	long l = *(int*)&swapme;
	char* ov = (char *)l;
    union {
        long newval;
        unsigned char nv[4];
    }u;

    u.nv[3] = *ov++; u.nv[2] = *ov++; u.nv[1] = *ov++; u.nv[0] = *ov++;

    return(d = u.newval);
}

