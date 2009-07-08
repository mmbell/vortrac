/*
 * PressureList.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/23/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef PRESSURELIST_H
#define PRESSURELIST_H

#include "Pressure/PressureData.h"
#include <QList>
#include "Configuration.h"
#include <QString>
#include <QObject>

class PressureList : public QList<PressureData>
{

public:
     PressureList(const QString &newFileName = QString());
     PressureList(Configuration* newConfig);
     virtual ~PressureList();
     
     bool save();
     bool open();
     void setFileName(const QString &newFileName);
	 void setRadarName(const QString &newRadarName);
	 void setVortexName(const QString &newVortexName);
     void setNewWorkingDirectory(const QString &newDirectory);
     QString getFileName() { return fileName; }
     QString getWorkingDirectory() { return workingDir; }
     void timeSort();
     
     void append(const PressureData &value);

private:
     Configuration *config;
     QString fileName;
     QString workingDir;

     QString vortexName;
     QString radarName;
     QString productType;
         
     void createDomPressureDataEntry(const PressureData &newData);
     


};

#endif
