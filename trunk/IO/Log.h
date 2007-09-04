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
#include <QDomElement>
#include <QDir>
#include <QMutex>
#include "Message.h"

class Log : public QWidget
{
  Q_OBJECT

 public:
  Log(QWidget *parent = 0);
  ~Log();
  bool saveLogFile();
  bool saveLogFile(const QString& fileName);
  QString getLogFileName() { return logFileName; }

 public slots:
  void setWorkingDirectory(QDir& newDir);
  void setLogFileName(QString& newName);
  void catchLog(const Message& logEntry);

    
 signals:
  void newLogEntry(const QString & logEntry);
  void newProgressEntry(int progress);
  void newStopLightColor(StopLightColor newColor, const QString newMessage);
  void newStormSignalStatus(StormSignalStatus newStatus, 
			    const QString newMessage);
  void log(const Message& message);
  void redLightAbort();

 private:
  QString logFileName;
  QFile *logFile;
  QDir workingDirectory;
  int absoluteProgress;
  bool displayLocation;
  QStringList messagesWaiting;
  // bool usingFile;
  QMutex usingFile;

  bool writeToFile();

  struct SLChange {
    StopLightColor color;
    QString message;
    QString location;
  };

  struct SSChange {
    StormSignalStatus status;
    QString message;
    QString location;
  };
  
  bool handleStopLightUpdate(StopLightColor newColor, QString message, 
			      QString location);
  QList<SLChange*> StopLightQueue;

  bool handleStormSignalUpdate(StormSignalStatus newStatus, QString message,
			       QString location);
  QList<SSChange*> StormStatusQueue;

};

#endif
