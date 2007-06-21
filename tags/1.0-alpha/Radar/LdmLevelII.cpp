/*
 *  LdmLevelII.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "LdmLevelII.h"
#include "RadarQC.h"

LdmLevelII::LdmLevelII(const QString &radarname, const float &lat, const float &lon, const QString &filename) 
	: LevelII(radarname, lat, lon, filename)
{
  
}

bool LdmLevelII::readVolume()
{

  // Read in a compressed LDM level II file
  // Thanks to Dick Oye for some code used here

  // Check the byte order
  if (!machineBigEndian()) {
    swap_bytes = true;
  }

  // Open the QFile object from the header
  if(!radarFile.open(QIODevice::ReadOnly)) {
    Message::report("Can't open radar volume");
  }

  QDataStream dataIn(&radarFile);
  
  // Get volume header
  dataIn.readRawData((char *)volHeader, sizeof(nexrad_vol_scan_title));
  if (swap_bytes) {
    swapVolHeader();
  }

  // Read in blocks of data
  char* nexBuffer = new char[2432];
  while (!dataIn.atEnd()) {
	  
	  // Try to read 4 bytes for size
	  int recSize;
	  dataIn.readRawData((char *)&recSize, 4);
	  if (swap_bytes) {
		  recSize = swap4((char *)&recSize); 
	  }
  
	  // Read in the compressed record
	  if (recSize < 0) {
		  recSize = -recSize;
	  }
	  char* compressed = new char[recSize];
	  dataIn.readRawData((char *)compressed,recSize);
					 
	  unsigned int uncompSize = 262144;
	  char* uncompressed = new char[uncompSize];
	  int error;
	  while (1) {
		  error = BZ2_bzBuffToBuffDecompress(uncompressed, &uncompSize,
											 compressed, recSize, 0, 0);
		  if (error == BZ_OUTBUFF_FULL) {
			  // Double the array
			  uncompSize += 262144;
			  delete uncompressed;
			  uncompressed = new char[uncompSize];
		  } else if (!error) {
			  // Uncompress worked!
			  break;
		  } else {
			  Message::report("Error uncompressing LDM Level II block");
		  }
	  }	

	  delete compressed;
	  
	  for (unsigned int i = 0; i < uncompSize; i += 2432) {
		  // Extract a packet
		  nexBuffer = (uncompressed + i);

		  // Skip the CTM info
		  //char *readPtr = nexBuffer + sizeof(CTM_info);
		  char *readPtr = nexBuffer + 12;
		  // Read in the message header
		  msgHeader = (nexrad_message_header *)readPtr;

		  if (msgHeader->message_type == 1) {
			  // Got some data
			  radarHeader = (digital_radar_data_header *)(readPtr + sizeof(nexrad_message_header));
			  if (swap_bytes) {
				  swapRadarHeader();
			  }

			  vcp = radarHeader->vol_coverage_pattern;

			  // Is this a new sweep? Check radial status
			  if (radarHeader->radial_status == 3) {

				  // Beginning of volume
				  volumeTime = radarHeader->milliseconds_past_midnight;
				  volumeDate = radarHeader->julian_date;
				  QDate initDate(1970,1,1);
				  radarDateTime.setDate(initDate);
				  radarDateTime.setTimeSpec(Qt::UTC);
				  radarDateTime = radarDateTime.addDays(volumeDate - 1);
				  radarDateTime = radarDateTime.addMSecs((qint64)volumeTime);
	
				  // First sweep and ray
				  addSweep(Sweeps);
				  Sweeps[0].setFirstRay(0);

			  } else if (radarHeader->radial_status == 0) {

				  // New sweep
				  // Use Dennis' stuff here eventually
				  // Count up rays in sweep
				  Sweeps[numSweeps-1].setLastRay(numRays-1);
				  // Increment array
				  addSweep(&Sweeps[numSweeps]);
				  // Sweeps[numSweeps].setFirstRay(numRays);

			  }
			  
			  // Read ray of data
			  if (radarHeader->ref_ptr) {
				  char* const ref_buffer = readPtr + sizeof(nexrad_message_header) + radarHeader->ref_ptr;
				  ref_data = decode_ref(ref_buffer,
										radarHeader->ref_num_gates);
			  }
			  if (radarHeader->vel_ptr) {
				  char* const vel_buffer = readPtr + sizeof(nexrad_message_header) + radarHeader->vel_ptr;
				  vel_data = decode_vel(vel_buffer,
										radarHeader->vel_num_gates,
										radarHeader->velocity_resolution);
			  }
			  if (radarHeader->sw_ptr) {
				  char* const sw_buffer = readPtr + sizeof(nexrad_message_header) + radarHeader->sw_ptr;
				  sw_data = decode_sw(sw_buffer,
									  radarHeader->vel_num_gates);
			  }
			  
			  // Put more rays in the volume, associated with the current Sweep;
			  addRay(&Rays[numRays]);
			  
		  } else {
			  // Some other junk, no need to skip
			  // nexBuffer += (msgHeader->message_len) * 2;
		  }
		  
	  }
	  
	  delete uncompressed;
  }  
  // Record the number of rays in the last sweep
  Sweeps[numSweeps-1].setLastRay(numRays-1);

  // Should have all the data stored into memory now
  radarFile.close();

  isDealiased(false);

  return true;

}
