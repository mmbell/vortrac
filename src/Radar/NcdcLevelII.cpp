/*
 *  NcdcLevelII.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "NcdcLevelII.h"
#include "RadarQC.h"

NcdcLevelII::NcdcLevelII(const QString &radarname, const float &lat, const float &lon, const QString &filename) : LevelII(radarname, lat, lon, filename)
{
    // All of these happen in LevelII constructor
    //numSweeps = 0;
    //numRays = 0;
    //volHeader = new nexrad_vol_scan_title;
    //msg1Header = new digital_radar_data_header;
    //msgHeader = new nexrad_message_header;
    //Sweeps = new Sweep[20];
    //Rays = new Ray[7500];
    //swap_bytes = false;

}

bool NcdcLevelII::readVolume()
{

    // Read in a level II file
    // Thanks to Dick Oye for some code used here

    // Check the byte order
    if (!machineBigEndian()) {
        swap_bytes = true;
    }

    // Open the QFile object from the header
    if(!radarFile->open(QIODevice::ReadOnly)) {
        Message::report("Can't open radar volume");
        return false;
    }

    QDataStream dataIn(radarFile);

    // Get volume header
    dataIn.readRawData((char *)volHeader, sizeof(nexrad_vol_scan_title));
    if (swap_bytes) {
        swapVolHeader();
    }

    // Read in blocks of data
    char* const headBuffer = new char[28];
    char* const nexBuffer = new char[131054];
    int recNum = 0;
    //int recSize, headSize;
    while (!dataIn.atEnd()) {

        recNum++;
        int recSize = 0;
        int headSize = sizeof(nexrad_message_header) + 12;
        dataIn.readRawData(headBuffer,headSize);

        // Skip the CTM info
        char *headPtr = headBuffer + 12;

        // Read in the message header
        msgHeader = (nexrad_message_header *)headPtr;
        if (swap_bytes) {
            swapMsgHeader();
        }
        // Read a variable # of bytes
        if (msgHeader->message_type == 31) {
            recSize = (msgHeader->message_len)*2 + 12 - headSize;
        } else {
            recSize = 2432 - headSize;
        }
        dataIn.readRawData(nexBuffer,recSize);
        char *readPtr = nexBuffer;

        if (msgHeader->message_type == 1) {
            // Got some fixed length data
            sweepMsgType = 1;
            msg1Header = (message_1_data_header *)(readPtr);
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
                char* const ref_buffer = readPtr + msg1Header->ref_ptr;
                decode_ref(&Rays[numRays], ref_buffer, msg1Header->ref_num_gates);
            }
            if (msg1Header->vel_ptr) {
                char* const vel_buffer = readPtr + msg1Header->vel_ptr;
                decode_vel(&Rays[numRays], vel_buffer, msg1Header->vel_num_gates, msg1Header->velocity_resolution);
            }
            if (msg1Header->sw_ptr) {
                char* const sw_buffer = readPtr + msg1Header->sw_ptr;
                decode_sw(&Rays[numRays], sw_buffer, msg1Header->vel_num_gates);
            }

            // Put more rays in the volume, associated with the current Sweep;
            addRay(&Rays[numRays]);

        } else if (msgHeader->message_type == 31) {

            // Got some variable length data
            sweepMsgType = 31;

            msg31Header = (message_31_data_header *)(readPtr);
            if (swap_bytes) {
                swapMsg31Header();
            }

            // Read volume and radial data
            if (msg31Header->vol_ptr) {
                volume_block = (volume_data_block *)(readPtr + msg31Header->vol_ptr);
                if (swap_bytes) {
                    swapVolumeBlock();
                }
                vcp = volume_block->vol_coverage_pattern;
            }

            if (msg31Header->radial_ptr) {
                radial_block = (radial_data_block *)(readPtr + msg31Header->radial_ptr);
                if (swap_bytes) {
                    swapRadialBlock();
                }
            }

            if (msg31Header->ref_ptr) {
                ref_block = (moment_data_block *)(readPtr + msg31Header->ref_ptr);
                if (swap_bytes) {
                    swapMomentDataBlock(ref_block);
                }
                QString blockID(ref_block->block_type);
                if (blockID != QString("DREF")) {
                    // Report this ray
                    Message::report("Error in reflectivity block");
                }
                char* const ref_buffer = (char *)ref_block + sizeof(moment_data_block);
                decode_ref(&Rays[numRays], ref_buffer, ref_block->num_gates);
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
                vel_block = (moment_data_block *)(readPtr + msg31Header->vel_ptr);
                if (swap_bytes) {
                    swapMomentDataBlock(vel_block);
                }
                QString blockID(vel_block->block_type);
                if (blockID != QString("DVEL")) {
                    // Report this ray
                    Message::report("Error in velocity block");
                }
                char* const vel_buffer = (char *)vel_block + sizeof(moment_data_block);
                decode_vel(&Rays[numRays], vel_buffer, vel_block->num_gates, vel_block->scale);
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
                sw_block = (moment_data_block *)(readPtr + msg31Header->sw_ptr);
                if (swap_bytes) {
                    swapMomentDataBlock(sw_block);
                }
                char* const sw_buffer = (char *)sw_block + sizeof(moment_data_block);
                decode_sw(&Rays[numRays], sw_buffer, sw_block->num_gates);
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

            } else if (msg31Header->radial_status == 4) {
                // Bail out?
                //break;
            }

            // Put more rays in the volume, associated with the current Sweep;
            addRay(&Rays[numRays]);

        } else {
            // Message Length is too short for binary segment
            msgHeader->message_len = 1210;
        }

    }
    // Record the number of rays in the last sweep
    Sweeps[numSweeps-1].setLastRay(numRays-1);

    // Should have all the data stored into memory now
    radarFile->close();

    isDealiased(false);

    delete[] headBuffer;
    delete[] nexBuffer;

    return true;

}

NcdcLevelII::~NcdcLevelII()
{
}
