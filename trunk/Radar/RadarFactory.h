/*
 *  RadarFactory.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/24/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */


#ifndef RADARFACTORY_H
#define RADARFACTORY_H

#include <QString>
#include <QDir>
#include <QFile>
#include <QDomElement>
#include <QQueue>
#include <QHash>
#include "Radar/RadarData.h"
#include "Radar/LevelII.h"
#include "Radar/radarh.h"
#include "IO/Message.h"

class RadarFactory : public QObject
{

  Q_OBJECT

 public:
  RadarFactory(QDomElement radarConfig, QObject *parent = 0);
  ~RadarFactory();
  RadarData* getUnprocessedData();
  bool hasUnprocessedData();
  
 public slots:
   void catchLog(const Message& message);

 signals:
   void log(const Message& message);

 private:
  QDir dataPath;
  VolumeInfo volumeInfo;
  QQueue<QString> *radarQueue;
  QDateTime startDateTime;
  QDateTime endDateTime;
  QHash<QString, bool> fileAnalyzed;

};
  

#endif

