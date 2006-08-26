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
	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
  
  workingDirectory = QDir::current();
  logFileName = QString("autoLog");

  logFile = new QFile(workingDirectory.filePath(logFileName+".log"));
  int i = 1;
  QString newName(logFileName);
  while(logFile->exists())
    {
      i++;
      newName = logFileName+QString().setNum(i);
      logFile = new QFile(workingDirectory.filePath(newName+".log"));
    }

  logFileName = newName + ".log";

  absoluteProgress = 0;
  displayLocation = false;

  //Message::toScreen("log:constructor: "+workingDirectory.path());
}

Log::~Log()
{
  delete logFile;
}

void Log::setWorkingDirectory(QDir& newDir)
{
  if(newDir.exists()) {
    //Message::toScreen("Path does exist");
  }
  else {
    //Message::toScreen("Path does not exist yet");
    newDir.mkpath(newDir.path());
    if(newDir.exists()) {
      //Message::toScreen("Path exists NOW?");
    }
  }
  if(!newDir.isAbsolute())
    newDir.makeAbsolute();

  QString newFileName("autoLog");
  QFile newLogFile(newDir.filePath(newFileName+".log"));
  int i = 1;
  QString newName(newFileName);
  while(newLogFile.exists())
    {
      i++;
      newName = newFileName+QString().setNum(i);
      newLogFile.setFileName(workingDirectory.filePath(newName+".log"));
    }
  newFileName = newName+".log";

  if(!logFile->copy(newDir.filePath(newFileName))) {
    //Message::toScreen("Log::setWorkingDirectory: could not copy "+logFile->fileName()+" to "+newDir.filePath(logFileName));
  }
  
  logFile->remove();
  //Message::toScreen("log:beforeChange: "+workingDirectory.path());
  workingDirectory = newDir;
  //Message::toScreen("log:afterChange: "+workingDirectory.path());
  logFileName = newFileName;
  QFile* oldLogFile = logFile;
  delete oldLogFile;
  logFile = new QFile(workingDirectory.filePath(logFileName));
  //Message::toScreen(" after working dir changed file = "+logFile->fileName());
  
}

void Log::setLogFileName(QString& newName) 
{
  // check and see if it has a file extension
  if(!newName.contains(QString(".log")))
    if(!newName.contains(QString(".")))
      newName.append(QString(".log"));
	// if not, then add .log extension

  QFile *newFile = new QFile(workingDirectory.filePath(newName));

  if(newFile->exists())
    {
      newFile->remove();
    }

  if(!logFile->copy(workingDirectory.filePath(newName)))
    //Message::toScreen("Log::setWorkingDirectory: could not copy "+logFile->fileName()+" to "+workingDirectory.filePath(newName));
  logFileName = newName;
  logFile = new QFile(workingDirectory.filePath(logFileName));
  
}

bool Log::saveLogFile()
{
 QString saveName=QFileDialog::getSaveFileName(this, QString(tr("Save Status Log File as...")), workingDirectory.path(), QString(tr("Text Files *.txt")));
  if(!saveName.isEmpty()) {
    if(logFile->copy(saveName)) {
      return true;
    }
    else {
      Message::toScreen(tr("Failed to save log file"));
      return false;
    }
  }
  return false;
}

bool Log::saveLogFile(const QString& fileName)
{
  QString checkFileName(fileName);
  int lastSlash = checkFileName.lastIndexOf(QString("/"));
  checkFileName.truncate(lastSlash);
  QDir check(checkFileName);
  QFile *newLogFile;
  if(check.isAbsolute())
    newLogFile = new QFile(fileName);
  else 
    newLogFile = new QFile(workingDirectory.filePath(fileName));
  
  if(newLogFile->exists())
    newLogFile->remove();
  delete newLogFile;
  if(check.isAbsolute())
    if(logFile->copy(fileName)) {
      return true;
    }
    else {
      Message::toScreen(tr("Failed to save log file"));
      return false;
    }
  else {
    if(logFile->copy(workingDirectory.filePath(fileName))) {
      return true;
    }
    else {
      Message::toScreen(tr("Failed to save log file"));
      return false;
    }
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
  if(progress==-1) {
    absoluteProgress = 0;
    emit newProgressEntry(absoluteProgress);
  }
  delete log;
}


bool Log::writeToFile(const QString& message)
{
  if(logFile->isOpen())
    Message::toScreen("When logging message: "+message+" logFile was already open");
  if(logFile->open(QIODevice::Append)) 
    {
      logFile->write(message.toAscii());
      logFile->close();
      //Message::toScreen("Sucessfully saved to file named "+logFile->fileName());
      if(logFile->isOpen())
	Message::toScreen("When logging message: "+message+" logFile did not close but did not fail");
      return true;
    }

  // Message::toScreen("Failed to write Log message to file");
  return false;
}
