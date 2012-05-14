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

#include <QList>
#include "DataObjects/VortexData.h"

class QString;

class VortexList : public QList<VortexData>
{

public:
     VortexList(QString filePath = QString());
     virtual ~VortexList();
     
     bool saveXML();
     bool restore();
     void setFilePath(QString filePath);
     void timeSort();

private:
     QString _filePath;
};

#endif
