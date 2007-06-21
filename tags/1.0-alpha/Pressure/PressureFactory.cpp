/*
 *  PressureFactory.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/18/06.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "PressureFactory.h"
#include "Pressure/PressureData.h"
#include <iostream>
#include <QPushButton>

PressureFactory::PressureFactory(Configuration *wholeConfig, QObject *parent)
  : QObject(parent)
{
  this->setObjectName("pressureFactory");
  //Set the domNode for the pressure specific stuff
  QDomElement pressureConfig = wholeConfig->getConfig("pressure");
  //Set the domNode for the radar part to get the corresponding date & time
  QDomElement radarConfig = wholeConfig->getConfig("radar");
	  
  // Will poll for data and return pressure objects in a queue
  pressureQueue = new QQueue<QString>;

  // Get relevant configuration info
  QDate startDate = QDate::fromString(wholeConfig->getParam(radarConfig,QString("startdate")),
									  Qt::ISODate);
  QDate endDate = QDate::fromString(wholeConfig->getParam(radarConfig,QString("enddate")),
									Qt::ISODate);
  QTime startTime = QTime::fromString(wholeConfig->getParam(radarConfig,QString("starttime")),
									  "hh:mm:ss");
  // Set the start time 1 hour earlier so that we can get all relevant pressure measurements
  startTime.addSecs(-3600);
  
  QTime endTime = QTime::fromString(wholeConfig->getParam(radarConfig,QString("endtime")),
									"hh:mm:ss");
  startDateTime = QDateTime(startDate, startTime, Qt::UTC);
  endDateTime = QDateTime(endDate, endTime, Qt::UTC);
  QString path = wholeConfig->getParam(pressureConfig, 
											QString("dir"));
  dataPath = QDir(path);

  QString format = wholeConfig->getParam(pressureConfig, 
										QString("format"));
  if (format == "METAR") {
    pressureFormat = metar;
  } 
  if (format == "AWIPS") {
    pressureFormat = awips;
  }
  else { 
    // Will implement more later but give error for now
    emit log(Message("Data format not supported"));
  }

}

PressureFactory::~PressureFactory()
{
  delete pressureQueue;
}

PressureList* PressureFactory::getUnprocessedData()
{
  
  // Get the latest files off the queue and make pressure objects
  // Have to return a list in case there are multiple obs in the same file (which is likely)

  if (pressureQueue->isEmpty()) {
    // Problem, shouldn't be here
    emit log(Message("Trying to get nonexistent pressure data off queue"));
  }

  // Get the files off the queue
  QString fileName = dataPath.filePath(pressureQueue->dequeue());
  // Mark it as processed
  fileParsed[fileName] = true;
    
  // Now make a new pressureList from that file and send it back
  PressureList* pressureList = new PressureList();
  switch(pressureFormat) {
  case metar :
    {
		//Not implemented
		break;
    }
  case awips:
    {
      	QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return 0;
		
		QTextStream in(&file);
		while (!in.atEnd()) {
			QString ob = in.readLine();
			AWIPS *pressureData = new AWIPS(ob);
			// Check to make sure it is not a duplicate -- this messes up the XML structure
			bool duplicateOb = false;
			for (int i = 0; i < pressureList->size(); i++) {
				if ((pressureList->at(i).getStationName() == pressureData->getStationName())
					and (pressureList->at(i).getTime() == pressureData->getTime())) {
					emit log(Message("Omitting duplicate surface ob"));
					duplicateOb = true;
				}
			}
			// Check to make sure it is a near-surface measurement
			if ((pressureData->getAltitude() >= 0) and
				(pressureData->getAltitude() <= 20) and
				(!duplicateOb)) {
				pressureList->append(*pressureData);
			}
			delete pressureData;
		}
		file.close();
		return pressureList;
		
      break;
    }
  case netcdf:
    {
      // Not yet implemented
      break;
    }
  }

  // If we get here theres a problem, return a null pointer
  emit log(Message("Problem with pressure data Factory"));
  return 0;

}

bool PressureFactory::hasUnprocessedData()
{
  
  // Check the unprocessed list first, if it has files no need to reread directory yet
  if(!pressureQueue->isEmpty()) {
    return true;
  }

  // Otherwise, check the directory for appropriate files

  switch(pressureFormat) {
  case metar:
    {
		// Not implemented
		break;
    }
   
  case awips:
    {
		// Assuming that the filename structure is a timestamp
		//dataPath.setNameFilters(QStringList("*"));
		dataPath.setFilter(QDir::Files);
		//dataPath.setSorting(QDir::Time | QDir::Reversed);
		dataPath.setSorting(QDir::Name);
		QStringList filenames = dataPath.entryList();
		
		// Check to see which are in the time limits
		for (int i = 0; i < filenames.size(); ++i) {
			QString file = filenames.at(i);
			QString timepart = file;
			// Parse the timestamps
			QStringList timestamp = timepart.split("_");
			if(timestamp.size()<2)
			  continue;
			QDate fileDate = QDate::fromString(timestamp.at(0), "yyyyMMdd");
			QTime fileTime = QTime::fromString(timestamp.at(1), "hhmmss");
			QDateTime fileDateTime = QDateTime(fileDate, fileTime, Qt::UTC);
			
			if (fileDateTime >= startDateTime && fileDateTime <= endDateTime) {	
				// Valid time and pressure name, check to see if it has been processed
				if (!fileParsed[dataPath.filePath(file)]) {
					// File has not been parsed, add it to the queue
					pressureQueue->enqueue(file);
				}
			}
		}
		
		break;
    }
    
  case netcdf:
    {
      // Not yet implemented
      break; 
    } 
    
  }

  // See if we added any new files to the queue
  if(!pressureQueue->isEmpty()) {
    return true;
  }
  
  // We made it here so there must be nothing new
  return false;
  
}

void PressureFactory::catchLog(const Message& message)
{
  emit log (message);
}
