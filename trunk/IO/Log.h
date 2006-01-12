/*
 * Log.h
 * VORTRAC
 *
 * Created by Michael Bell on 7/22/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef LOG_H
#define LOG_H

#include <QObject>
#include <QString>
#include <QFile>
#include "Message.h"

class Log : public QWidget
{
  Q_OBJECT

 public:
  Log(QWidget *parent = 0);
  bool saveLogFile();

 public slots:
  void setWorkingDir(const QString& newDir);


  void catchLog(const Message& logEntry);
    
 signals:
  void newLogEntry(const QString & logEntry);
  void newProgressEntry(int progress);

 private:
  QString logFileName;
  QFile *logFile;
  QString workingDir;
  int absoluteProgress;
  bool displayLocation;

};

#endif
