/*
 * Log.cpp
 * VORTRAC
 *
 * Created by Michael Bell on 7/22/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "Log.h"
#include <QFileDialog>

Log::Log(QWidget *parent) 
  : QWidget(parent)
{
  workingDir = QString("/scr/science40/mauger/vortrac/workingDirs/");
  logFileName = QString("autoLog.txt");

  logFile = new QFile(workingDir+logFileName);
  if(logFile->exists())
    {
      logFile->remove();
    }

  absoluteProgress = 0;
  displayLocation = false;
}

void Log::setWorkingDir(const QString& newDir)
{

}

void Log::setLogFileName(const QString& newName) 
{
  QFile *newFile = new QFile(workingDir+newName);
  if(newFile->exists())
    {
      newFile->remove();
    }
  logFile->copy(workingDir+newName);
  logFileName = newName;
  logFile = new QFile(workingDir+logFileName);
  
}

bool Log::saveLogFile()
{
 QString saveName=QFileDialog::getSaveFileName(this, QString(tr("Save Status Log File as...")), workingDir, QString(tr("Text Files *.txt")));
  if(!saveName.isEmpty()) {
    if(logFile->copy(saveName)) {
      return true;
    }
    else {
      catchLog(Message(tr("Failed to save log file")));
      return false;
    }
  }
  return false;
}

void Log::catchLog(const Message& logEntry)
{
  Message* log = new Message(logEntry);
  QString message = log->getLogMessage();
  int progress = log->getProgress();
  QString location = log->getLocation();
  bool severe = log->isSevere();
  
  if(message!=QString()) {
    if(displayLocation) {
      message = location+": "+message;
    }
    logFile->open(QIODevice::Append);
    message += "\n";
    logFile->write(message.toAscii());
    logFile->close();
    emit(newLogEntry(message));
  }
  
  if(progress!=0) {
    absoluteProgress+=progress;
    emit newProgressEntry(absoluteProgress);
  }
  
}

