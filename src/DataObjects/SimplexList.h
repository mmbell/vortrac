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

class SimplexList : public QList<SimplexData>
{

public:
    SimplexList(QString filePath = QString());
    virtual ~SimplexList();
    void setFilePath(QString filePath) {_filePath=filePath;}
    void timeSort();
    bool restore();
    bool saveXML();

    void dump() const;
    
private:
    QString _filePath;
};

#endif
