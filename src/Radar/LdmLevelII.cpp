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
  if(!radarFile->open(QIODevice::ReadOnly)) {
    Message::report("Can't open radar volume");
  }

  QDataStream dataIn(radarFile);
  
  // Get volume header
  dataIn.readRawData((char *)volHeader, sizeof(nexrad_vol_scan_title));
  if (swap_bytes) {
    swapVolHeader();
  }

  // Read in blocks of data
  char* nexBuffer = new char[2432];
  int recNum = 0;
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
			  delete[] uncompressed;
			  uncompressed = new char[uncompSize];
		  } else if (!error) {
			  // Uncompress worked!
			  break;
		  } else {
			  /* Some other error, but log functionality not currently available in this class
			  if (error == BZ_CONFIG_ERROR)
				  emit log(Message(QString("The BZ2 library has been miscompiled"),0,
							   this->objectName(),Red,QString("Error reading Level II File")));
			  if (error == BZ_PARAM_ERROR)
				  emit log(Message(QString("Parameter Error in BZ2 Decompression"),0,
							   this->objectName(),Yellow,QString("Error reading Level II File")));
			  if (error == BZ_MEM_ERROR)
				  emit log(Message(QString("Insufficient memory for BZ2 Decompression"),0,
								   this->objectName(),Red,QString("Error reading Level II File")));
			  if (error == BZ_DATA_ERROR)
				  emit log(Message(QString("Compressed data exceeds destLen in BZ2 unzip"),0,
								   this->objectName(),Yellow,QString("Error reading Level II File")));
			  if (error == BZ_DATA_ERROR_MAGIC)
				  emit log(Message(QString("Compressed data doesn't begin with right magic bytes"),0,
								   this->objectName(),Yellow,QString("Error reading Level II File")));
			  if (error == BZ_UNEXPECTED_EOF)
				  emit log(Message(QString("Compressded data ended unexpectedly"),0,
								   this->objectName(),Yellow,QString("Error reading Level II File")));
			  */
			  break;
		  }
	  }	
	  delete[] compressed;
	  if (error) {
		  // Didn't uncompress the data properly
		  delete[] uncompressed;
		  continue;
	  }
	  
	  recNum++;
	  // Skip the metadata at the beginning
	  if ((recNum == 1) and (uncompSize == 325888)) {
          delete[] uncompressed;
          continue;
      }
	   
	  unsigned int msgIncr = 0;
	  //for (unsigned int i = 0; i < uncompSize; i += 2432) {
	  while (msgIncr < uncompSize) {
		  // Extract a packet, skipping metadata
		  nexBuffer = (uncompressed + msgIncr);

		  // Skip the CTM info
		  //char *readPtr = nexBuffer + sizeof(CTM_info);
		  char *readPtr = nexBuffer + 12;
		  // Read in the message header
		  msgHeader = (nexrad_message_header *)readPtr;
		  if (swap_bytes) {
			  swapMsgHeader();
		  }
		  if (msgHeader->message_type == 1) {
			  // Got some fixed length data
			  sweepMsgType = 1;
			  msg1Header = (message_1_data_header *)(readPtr + sizeof(nexrad_message_header));
			  if (swap_bytes) {
				  swapMsg1Header();
			  }

			  vcp = msg1Header->vol_coverage_pattern;

			  // Is this a new sweep? Check radial status
			  if (msg1Header->radial_status == 3) {

				  // Beginning of volume
				  volumeTime = msg1Header->milliseconds_past_midnight;
				  volumeDate = msg1Header->julian_date;
				  QDate initDate(1970,1,1);
				  radarDateTime.setDate(initDate);
				  radarDateTime.setTimeSpec(Qt::UTC);
				  radarDateTime = radarDateTime.addDays(volumeDate - 1);
				  radarDateTime = radarDateTime.addMSecs((qint64)volumeTime);
	
				  // First sweep and ray
				  addSweep(Sweeps);
				  Sweeps[0].setFirstRay(0);

			  } else if (msg1Header->radial_status == 0) {

				  // New sweep
				  // Use Dennis' stuff here eventually
				  // Count up rays in sweep
				  Sweeps[numSweeps-1].setLastRay(numRays-1);
				  // Increment array
				  addSweep(&Sweeps[numSweeps]);
				  // Sweeps[numSweeps].setFirstRay(numRays);

			  }
			  
			  // Read ray of data
			  if (msg1Header->ref_ptr) {
				  char* const ref_buffer = readPtr + sizeof(nexrad_message_header) + msg1Header->ref_ptr;
				  ref_data = decode_ref(ref_buffer,
										msg1Header->ref_num_gates);
			  }
			  if (msg1Header->vel_ptr) {
				  char* const vel_buffer = readPtr + sizeof(nexrad_message_header) + msg1Header->vel_ptr;
				  vel_data = decode_vel(vel_buffer,
										msg1Header->vel_num_gates,
										msg1Header->velocity_resolution);
			  }
			  if (msg1Header->sw_ptr) {
				  char* const sw_buffer = readPtr + sizeof(nexrad_message_header) + msg1Header->sw_ptr;
				  sw_data = decode_sw(sw_buffer,
									  msg1Header->vel_num_gates);
			  }
			  
			  // Put more rays in the volume, associated with the current Sweep;
			  addRay(&Rays[numRays]);
			  
		  } else if (msgHeader->message_type == 31) {
		  
		      // Got some variable length data
			  sweepMsgType = 31;
			  
			  msg31Header = (message_31_data_header *)(readPtr + sizeof(nexrad_message_header));
			  if (swap_bytes) {
				  swapMsg31Header();
			  }

			  // Read volume and radial data
			  if (msg31Header->vol_ptr) {
				  volume_block = (volume_data_block *)(readPtr + sizeof(nexrad_message_header) + msg31Header->vol_ptr);
				  if (swap_bytes) {
					swapVolumeBlock();
				  }
				  vcp = volume_block->vol_coverage_pattern;
			  }

			  if (msg31Header->radial_ptr) {
				  radial_block = (radial_data_block *)(readPtr + sizeof(nexrad_message_header) + msg31Header->radial_ptr);
				  if (swap_bytes) {
					swapRadialBlock();
				  }
			  }

			  if (msg31Header->ref_ptr) {
				  ref_block = (moment_data_block *)(readPtr + sizeof(nexrad_message_header) + msg31Header->ref_ptr);
				  if (swap_bytes) {
					swapMomentDataBlock(ref_block);
				  }
				  QString blockID(ref_block->block_type);
				  if (blockID != QString("DREF")) { 
					// Skip this ray
					//continue;
				  }
				  char* const ref_buffer = (char *)ref_block + sizeof(moment_data_block);
				  ref_data = decode_ref(ref_buffer,
										ref_block->num_gates);
				  ref_num_gates = ref_block->num_gates;
				  ref_gate1 = ref_block->gate1;
				  ref_gate_width = ref_block->gate_width;
			  } else {
			      ref_data = NULL;
				  ref_num_gates = 0;
				  ref_gate1 = 0;
				  ref_gate_width = 0;
			  }
			  
			  if (msg31Header->vel_ptr) {
				  vel_block = (moment_data_block *)(readPtr + sizeof(nexrad_message_header) + msg31Header->vel_ptr);
				  if (swap_bytes) {
					swapMomentDataBlock(vel_block);
				  }
				  QString blockID(ref_block->block_type);
				  if (blockID != QString("DVEL")) { 
					// Skip this ray
					//continue;
				  }
				  char* const vel_buffer = (char *)vel_block + sizeof(moment_data_block);
				  vel_data = decode_vel(vel_buffer,
										vel_block->num_gates,
										vel_block->scale);
				  vel_num_gates = vel_block->num_gates;
				  vel_gate1 = vel_block->gate1;
				  vel_gate_width = vel_block->gate_width;
			  } else {
				  vel_data = NULL;
				  vel_num_gates = 0;
				  vel_gate1 = 0;
				  vel_gate_width = 0;
			  }

			  if (msg31Header->sw_ptr) {
			      sw_block = (moment_data_block *)(readPtr + sizeof(nexrad_message_header) + msg31Header->sw_ptr);
				  if (swap_bytes) {
					swapMomentDataBlock(sw_block);
				  }
				  char* const sw_buffer = (char *)sw_block + sizeof(moment_data_block);
				  sw_data = decode_sw(sw_buffer,
									  sw_block->num_gates);
			  }

			  

			  // Is this a new sweep? Check radial status
			  if (msg31Header->radial_status == 3) {

				  // Beginning of volume
				  volumeTime = msg31Header->milliseconds_past_midnight;
				  volumeDate = msg31Header->julian_date;
				  QDate initDate(1970,1,1);
				  radarDateTime.setDate(initDate);
				  radarDateTime.setTimeSpec(Qt::UTC);
				  radarDateTime = radarDateTime.addDays(volumeDate - 1);
				  radarDateTime = radarDateTime.addMSecs((qint64)volumeTime);
	
				  // First sweep and ray
				  addSweep(Sweeps);
				  Sweeps[0].setFirstRay(0);

			  } else if (msg31Header->radial_status == 0) {

				  // New sweep
				  // Use Dennis' stuff here eventually
				  // Count up rays in sweep
				  Sweeps[numSweeps-1].setLastRay(numRays-1);
				  // Increment array
				  addSweep(&Sweeps[numSweeps]);
				  // Sweeps[numSweeps].setFirstRay(numRays);

			  } else if (msg31Header->radial_status == 2) {
			      // Bail out?
                  //int status = msg31Header->radial_status;
				  //break;
			  } else if (msg31Header->radial_status == 5) {
                  // Last sweep
                  Sweeps[numSweeps-1].setLastRay(numRays-1);
				  // Increment array
				  addSweep(&Sweeps[numSweeps]);
              } else {
                  // Shouldn't be here
                  /* Check for missing sweep demarcation!
                  if ((msg31Header->elevation - Sweeps[numSweeps-1].getElevation()) > 0.2) {
                      // This is probably a new sweep
                      Sweeps[numSweeps-1].setLastRay(numRays-1);
                      // Increment array
                      addSweep(&Sweeps[numSweeps]);
                  } */
              }
			  			  
			  // Put more rays in the volume, associated with the current Sweep;
			  addRay(&Rays[numRays]);
				
		  } else {
			  // Message Length is too short for binary segment
			  msgHeader->message_len = 1210;
		  }
		  
		  // Skip a variable # of bytes
		  msgIncr += (msgHeader->message_len)*2 + 12;
		  
	  }
	  
	  delete[] uncompressed;
  }  
  // Record the number of rays in the last sweep
  Sweeps[numSweeps-1].setLastRay(numRays-1);

  // Should have all the data stored into memory now
  radarFile->close();

  isDealiased(false);

  return true;

}
