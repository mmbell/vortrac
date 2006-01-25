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

  connect(this, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));
  
  workingDirectory = QString("workingDirs/");
  logFileName = QString("autoLog");

  logFile = new QFile(workingDirectory+logFileName+".log");
  int i = 1;
  QString newName(logFileName);
  while(logFile->exists())
    {
      i++;
      newName = logFileName+QString().setNum(i);
      logFile = new QFile(workingDirectory+newName+".log");
    }

  logFileName = newName + ".log";

  absoluteProgress = 0;
  displayLocation = false;
}

Log::~Log()
{

}

void Log::setWorkingDirectory(const QString& newDir)
{
  QString newFileName("autoLog.log");
  
  /*
  QFile* newLogFile = new QFile(newDir+newFileName);
  
    Best plan of action for if log files already exist there??
  if(newLogFile->exists()) {
    QFile* newLogFile = new QFile(newDir+newFileName+".log");
  }
  */
  logFile->copy(newDir+logFileName);
  logFile->remove();
  workingDirectory = newDir;
  logFileName = newFileName;
  logFile = new QFile(workingDirectory+logFileName);
  
  
}

void Log::setLogFileName(const QString& newName) 
{
  QFile *newFile = new QFile(workingDirectory+newName);

  if(newFile->exists())
    {
      newFile->remove();
    }

  logFile->copy(workingDirectory+newName);
  logFileName = newName;
  logFile = new QFile(workingDirectory+logFileName);
  
}

bool Log::saveLogFile()
{
 QString saveName=QFileDialog::getSaveFileName(this, QString(tr("Save Status Log File as...")), workingDirectory, QString(tr("Text Files *.txt")));
  if(!saveName.isEmpty()) {
    if(logFile->copy(saveName)) {
      return true;
    }
    else {
      emit log(Message(tr("Failed to save log file")));
      return false;
    }
  }
  return false;
}

bool Log::saveLogFile(const QString& fileName)
{
  QFile newLogFile(fileName);
  if(newLogFile.exists())
    newLogFile.remove();
  
  if(logFile->copy(fileName)) {
    return true;
  }
  else {
    log(Message(tr("Failed to save log file")));
    return false;
  }
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
    message+="\n";
    writeToFile(message);
    emit(newLogEntry(message));
  }
  
  if(progress!=0) {
    absoluteProgress+=progress;
    emit newProgressEntry(absoluteProgress);
  }
  
}


bool Log::writeToFile(const QString& message)
{

  if(logFile->open(QIODevice::Append)) 
    {
      logFile->write(message.toAscii());
      logFile->close();
      return true;
    }

  Message::toScreen("Failed to write Log message to file");
  return false;
}
