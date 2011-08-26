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
#include <QDomElement>
#include <QQueue>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "Radar/RadarData.h"
#include "Radar/LevelII.h"
#include "Radar/NcdcLevelII.h"
#include "Radar/LdmLevelII.h"
#include "Radar/AnalyticRadar.h"
#include "IO/Message.h"
#include "GUI/ConfigTree.h"
#include "DataObjects/VortexList.h"

class RadarFactory : public QObject
{

  Q_OBJECT

 public:
  RadarFactory(Configuration* radarConfig, QObject *parent = 0);
  ~RadarFactory();
  RadarData* getUnprocessedData();
  bool hasUnprocessedData();
  int getNumProcessed() const;
  void fetchRemoteData();

 public slots:
   void catchLog(const Message& message);
   void updateDataQueue(const VortexList* list);
   bool getRemoteData(QNetworkReply *catalog_reply);
   bool saveRemoteData(QNetworkReply *datafile_reply);

 signals:
   void log(const Message& message);

 private:
  enum dataFormat {
	ncdclevelII,
	ldmlevelII,
	model,
	dorade,
	netcdf
  };

  QDir dataPath;
  QString radarName;
  float radarLat;
  float radarLon;
  float radarAlt;
  dataFormat radarFormat;
  QQueue<QString> *radarQueue;
  QDateTime startDateTime;
  QDateTime endDateTime;
  QHash<QString, bool> fileAnalyzed;
  QDateTime radarDateTime;
  Configuration* mainConfig;
  QNetworkAccessManager catalog_manager;
  QNetworkAccessManager datafile_manager;

};
  

#endif

