/*
 * VortexList.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/23/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef VORTEXLIST_H
#define VORTEXLIST_H

#include "VortexData.h"
#include <QList>
#include "Configuration.h"
#include <QString>
#include <QObject>

class VortexList : public QList<VortexData>
{

public:
     VortexList(const QString &newFileName = QString());
     VortexList(Configuration* newConfig);
     virtual ~VortexList();
     
     bool save();
     bool open();
     bool openNodeFile(const QDomNode &newNode);
     bool saveNodeFile(int index, const QString& newName);
     void setFileName(const QString &newFileName);
     void setRadarName(const QString &newRadarName);
     void setVortexName(const QString &newVortexName);
     void setProductType(const QString &newProductType);
     void setIndividualProductType(int dataIndex, const QString& newType);
     QString getVortexName() { return vortexName; }
     void setNewWorkingDirectory(const QString &newDirectory);
     QString getFileName() { return fileName; }
     QString getWorkingDirectory() { return workingDir; }
     void timeSort();
     
     void append(const VortexData &value);
     void removeAt(int i);

private:
     Configuration *config;
     QString fileName;
     QString workingDir;

     QString vortexName;
     QString radarName;
     QString productType;
     
     QList<Configuration*>* vortexDataConfigs;
     QList<QString>* configFileNames;
     
     void createDomVortexDataEntry(const VortexData &newData);
     


};

#endif
