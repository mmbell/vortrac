/*
 * SimplexList.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/31/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef SIMPLEXLIST_H
#define SIMPLEXLIST_H

#include "SimplexData.h"
#include <QList>
#include "Configuration.h"
#include <QString>
#include <QObject>

class SimplexList : public QList<SimplexData>
{

public:
     SimplexList(const QString &newFileName = QString());
     SimplexList(Configuration* newConfig);
     virtual ~SimplexList();
     
     bool save();
     bool open();
     bool openNodeFile(const QDomNode &newNode);
     void setFileName(const QString &newFileName);
	 void setRadarName(const QString &newRadarName);
	 void setVortexName(const QString &newVortexName);
     void setNewWorkingDirectory(const QString &newDirectory);
     QString getFileName() { return fileName; }
     QString getWorkingDirectory() { return workingDir; }
     void timeSort();
     
     void append(const SimplexData &value);

private:
     Configuration *config;
     QString fileName;
     QString workingDir;

     QString vortexName;
     QString radarName;
     QString productType;

     QList<Configuration*>* simplexDataConfigs;
     QList<QString>* configFileNames;
     
     void createDomSimplexDataEntry(const SimplexData &newData);
     


};

#endif
