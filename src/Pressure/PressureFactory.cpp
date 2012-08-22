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
#include <math.h>

PressureFactory::PressureFactory(Configuration *mainCfg, QObject *parent) : QObject(parent)
{
    this->setObjectName("pressureFactory");

    QDomElement pressureConfig = mainCfg->getConfig("pressure");
    QDomElement radarConfig = mainCfg->getConfig("radar");

    // Will poll for data and return pressure objects in a queue
    pressureQueue = new QQueue<QString>;

    // Get relevant configuration info
    QDate startDate = QDate::fromString(mainCfg->getParam(radarConfig,QString("startdate")),Qt::ISODate);
    QDate endDate = QDate::fromString(mainCfg->getParam(radarConfig,QString("enddate")),Qt::ISODate);
    QTime startTime = QTime::fromString(mainCfg->getParam(radarConfig,QString("starttime")),"hh:mm:ss");
    // Set the start time 1 hour earlier so that we can get all relevant pressure measurements
    startTime = startTime.addSecs(-3600);
    QTime endTime = QTime::fromString(mainCfg->getParam(radarConfig,QString("endtime")),"hh:mm:ss");

    startDateTime = QDateTime(startDate, startTime, Qt::UTC);
    endDateTime = QDateTime(endDate, endTime, Qt::UTC);
    dataPath = QDir(mainCfg->getParam(pressureConfig,QString("dir")));
    radarlat = mainCfg->getParam(radarConfig,"lat").toFloat();
    radarlon = mainCfg->getParam(radarConfig,"lon").toFloat();

    QString format = mainCfg->getParam(pressureConfig,QString("format"));
    if (format == "HWind") {
        pressureFormat = hwind;
    } else if (format == "AWIPS") {
        pressureFormat = awips;
    } else if (format == "MADIS") {
        pressureFormat = madis;
    } else {
        emit log(Message("Data format not supported"));
    }
}

PressureFactory::~PressureFactory()
{
    delete pressureQueue;
}

QList<PressureData>* PressureFactory::getUnprocessedData()
{
    // Get the latest files off the queue and make pressure objects
    // Have to return a list in case there are multiple obs in the same file (which is likely)

    if (pressureQueue->isEmpty()) {
        // Problem, shouldn't be here
        emit log(Message("Trying to get nonexistent pressure data off queue"));
    }

    // Get the files off the queue
    QString fileName = dataPath.filePath(pressureQueue->dequeue());

    // Test file to make sure it is not growing
    QFile pressureFile(fileName);
    qint64 newFilesize = pressureFile.size();
    qint64 prevFilesize = 0;
    while (prevFilesize != newFilesize) {
        prevFilesize = newFilesize;
        sleep(1);
        newFilesize = pressureFile.size();
    }
    sleep(1);


    // Mark it as processed
    fileParsed[fileName] = true;
    
    // Now make a new pressureList from that file and send it back
    QList<PressureData>* pressureList = new QList<PressureData>;
    switch(pressureFormat) {
    case hwind :
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return 0;

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString ob = in.readLine();
            HWind *pressureData = new HWind(ob);
            // Check to make sure it is not a duplicate -- this messes up the XML structure
            bool duplicateOb = false;
            for (int i = 0; i < pressureList->size(); i++) {
                if ((pressureList->at(i).getStationName() == pressureData->getStationName())
                        and (pressureList->at(i).getTime() == pressureData->getTime())) {
                    //emit log(Message("Omitting duplicate surface ob"));
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
    case awips:
    {
        QFile file(fileName);
        QString timepart = fileName.split("/").last();
        // Parse the timestamp
        QStringList timestamp = timepart.split("_");
        QDate obDate = QDate::fromString(timestamp.at(2), "yyyyMMdd");
        QTime obTime = QTime::fromString(timestamp.at(3), "hhmm");

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return 0;

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString ob = in.readLine();
            AWIPS *pressureData = new AWIPS(ob);
            //pressureData->setTime(obDateTime);

            // Check to make sure it is not a duplicate -- this messes up the XML structure
            bool duplicateOb = false;
            for (int i = 0; i < pressureList->size(); i++) {
                if ((pressureList->at(i).getStationName() == pressureData->getStationName())
                        and (pressureList->at(i).getTime() == pressureData->getTime())) {
                    //emit log(Message("Omitting duplicate surface ob"));
                    duplicateOb = true;
                }
            }
            // Check to make sure it is not a duplicate and not too far away
            float obLat = pressureData->getLat();
            float obLon = pressureData->getLon();
            float LatRadians = radarlat * acos(-1.0)/180.0;
            float fac_lat = 111.13209 - 0.56605 * cos(2.0 * LatRadians)
                    + 0.00012 * cos(4.0 * LatRadians) - 0.000002 * cos(6.0 * LatRadians);
            float fac_lon = 111.41513 * cos(LatRadians)
                    - 0.09455 * cos(3.0 * LatRadians) + 0.00012 * cos(5.0 * LatRadians);

            float relX = (obLon - radarlon) * fac_lon;
            float relY = (obLat - radarlat) * fac_lat;
            float obRange = sqrt(relX*relX + relY*relY);
            if 	((!duplicateOb) and (obRange < 500)) {
                pressureList->append(*pressureData);
            }
            delete pressureData;
        }
        file.close();
        return pressureList;

        break;
    }
        case madis:
        {
            QFile file(fileName);
            QString timepart = fileName.split("/").last();
            // Parse the timestamp
            QStringList timestamp = timepart.split("_");
            QDate obDate = QDate::fromString(timestamp.at(0), "yyyyMMdd");
            QTime obTime = QTime::fromString(timestamp.at(1), "hhmm");
            
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return 0;
            
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString ob = in.readLine();
                if (ob.contains("No matching data")) continue;
                MADIS *pressureData = new MADIS(ob);
                //pressureData->setTime(obDateTime);
                
                // Check to make sure it is not a duplicate -- this messes up the XML structure
                bool duplicateOb = false;
                for (int i = 0; i < pressureList->size(); i++) {
                    if ((pressureList->at(i).getStationName() == pressureData->getStationName())
                        and (pressureList->at(i).getTime() == pressureData->getTime())) {
                        //emit log(Message("Omitting duplicate surface ob"));
                        duplicateOb = true;
                    }
                }
                // Check to make sure it is not a duplicate and not too far away
                float obLat = pressureData->getLat();
                float obLon = pressureData->getLon();
                float obAlt = pressureData->getAltitude();
                float obPressure = pressureData->getPressure();
                float LatRadians = radarlat * acos(-1.0)/180.0;
                float fac_lat = 111.13209 - 0.56605 * cos(2.0 * LatRadians)
                + 0.00012 * cos(4.0 * LatRadians) - 0.000002 * cos(6.0 * LatRadians);
                float fac_lon = 111.41513 * cos(LatRadians)
                - 0.09455 * cos(3.0 * LatRadians) + 0.00012 * cos(5.0 * LatRadians);
                
                float relX = (obLon - radarlon) * fac_lon;
                float relY = (obLat - radarlat) * fac_lat;
                float obRange = sqrt(relX*relX + relY*relY);
                if 	((!duplicateOb) and (obRange < 500) and (obAlt < 15.0)
                     and (obPressure < 1050.) and (obPressure > 850.)) {
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
    case hwind:
    {
        // Assuming that the filename structure is a timestamp
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

    case awips:
    {
        // Assuming that the filename structure has a trailing timestamp
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
            if(timestamp.size()<4)
                continue;
            QDate fileDate = QDate::fromString(timestamp.at(2), "yyyyMMdd");
            QTime fileTime = QTime::fromString(timestamp.at(3), "hhmm");
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
        case madis:
        {
            // Assuming that the filename structure has a trailing timestamp
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
                if(timestamp.size()<3)
                    continue;
                QDate fileDate = QDate::fromString(timestamp.at(0), "yyyyMMdd");
                QTime fileTime = QTime::fromString(timestamp.at(1), "hhmm");
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
