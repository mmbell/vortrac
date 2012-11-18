/*
 * DriverBatch.h
 * VORTRAC
 *
 * Created by Annette Foerster in 2012
 * Copyright 2005 University Corporation for Atmospheric Research.
 * All rights reserved.
 *
 *
 */


#ifndef DRIVERBATCH_H
#define DRIVERBATCH_H

#include <QWidget>
#include "Threads/workThread.h"
#include <QDomDocument>


class DriverBatch  : public QWidget
{
    Q_OBJECT

public:
    DriverBatch(QWidget *parent, QDomDocument& xmlfile);
    ~DriverBatch();
    bool initialize();
    bool run();
    bool finalize();

protected:
    workThread *pollThread;
    QDomDocument *domDoc;
    bool parseXMLconfig();
    bool validateXMLconfig();

};

#endif // DRIVERBATCH_H
