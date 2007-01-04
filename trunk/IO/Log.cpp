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
      logFile->setFileName(workingDirectory.filePath(newName+".log"));
    }

  logFileName = newName + ".log";
  logFile->setFileName(workingDirectory.filePath(logFileName));
  
  absoluteProgress = 0;
  //displayLocation = false;
  displayLocation = true;

  //Message::toScreen("log:constructor: "+workingDirectory.path());
}

Log::~Log()
{
  delete logFile;
}

void Log::setWorkingDirectory(QDir& newDir)
{
	if (newDir == workingDirectory) {
		// Don't need to do anything
		return;
	}
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
  
  if(!logFile->copy(newDir.filePath(logFileName))) {
    //Message::toScreen("Log::setWorkingDirectory: could not copy "+logFile->fileName()+" to "+newDir.filePath(logFileName));
  }
  
  logFile->remove();
  //Message::toScreen("log:beforeChange: "+workingDirectory.path());
  workingDirectory = newDir;
  //Message::toScreen("log:afterChange: "+workingDirectory.path());
  logFileName = newFileName;
  /*
  QFile* oldLogFile = logFile;
  delete oldLogFile;
  logFile = new QFile(workingDirectory.filePath(logFileName));
  //Message::toScreen(" after working dir changed file = "+logFile->fileName());
  */
  logFile->setFileName(workingDirectory.filePath(logFileName));
}

void Log::setLogFileName(QString& newName) 
{
  // check and see if it has a file extension
  if(!newName.contains(QString(".log")))
    if(!newName.contains(QString(".")))
      newName.append(QString(".log"));
	// if not, then add .log extension

  QFile newFile(workingDirectory.filePath(newName));

  if(newFile.exists())
    {
      newFile.remove();
    }

  if(!logFile->copy(workingDirectory.filePath(newName)))
    //Message::toScreen("Log::setWorkingDirectory: could not copy "+logFile->fileName()+" to "+workingDirectory.filePath(newName));
  logFileName = newName;
  logFile->setFileName(workingDirectory.filePath(logFileName));
  //logFile = new QFile(workingDirectory.filePath(logFileName));
  
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
  StopLightColor stopLightColor = log->getColor();
  QString stopLightMessage = log->getStopLightMessage();
  StormSignalStatus stormSignalStatus = log->getStatus();
  QString stormSignalMessage = log->getStormSignalMessage();
  
  if(message!=QString()) {
    if(displayLocation && (location!=QString())) {
      message = location+": "+message;
    }
    message+="\n";
    //writeToFile(message);
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

  if((stopLightColor!=AllOff) || (stopLightMessage!=QString())) {
    if(!handleStopLightUpdate(stopLightColor,stopLightMessage,location))
      Message::toScreen("Log:Trouble Logging StopLight Change! "+location+" : "+stopLightMessage);
  }
  if((stormSignalStatus!=Nothing) || (stormSignalMessage!=QString())) {
    if(!handleStormSignalUpdate(stormSignalStatus,stormSignalMessage,location))
      Message::toScreen("Log:Trouble Logging StormStatus Change! "+location+" : "+stormSignalMessage);
  }

  delete log;
}


bool Log::writeToFile(const QString& message)
{
  
  if(logFile->isOpen()) {
    Message::toScreen("When logging message: "+message+" logFile was already open");
    logFile->close();
  }
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

bool Log::handleStopLightUpdate(StopLightColor newColor, QString message, 
				QString location)
{
  // Check to see if update is an error or a corrected error
  int originalCount = StopLightQueue.count();

  if((newColor == Green)||(newColor == BlinkGreen)) {
    // Message::toScreen("The Good");
    if(StopLightQueue.count()==0)
      return true;
    for(int i = StopLightQueue.count()-1; i >= 0; i--) {
      if((StopLightQueue[i]->location == location) ||
	 (StopLightQueue[i]->location == QString()))
	StopLightQueue.removeAt(i);
    }
  }
  else {
    // Add bad signals to the stack based on color....
    SLChange *mostRecent = new SLChange;
    mostRecent->color = newColor;
    mostRecent->message = message;
    mostRecent->location = location;
    if(StopLightQueue.count()==0)
      StopLightQueue.append(mostRecent);
    else {
      for(int i = 0; i < StopLightQueue.count(); i++) {
	if(mostRecent->color < StopLightQueue[i]->color)
	  StopLightQueue.insert(i+1, mostRecent);
      }
    }
    //Message::toScreen("The Bad");
  }
  
  if((originalCount!=StopLightQueue.count())||(originalCount==0)) {
    emit newStopLightColor(StopLightQueue[0]->color,
			   StopLightQueue[0]->message);
    return true;
  }
  else {
    // A color came in and was not handled .... issue
    return true;
  }

}

bool Log::handleStormSignalUpdate(StormSignalStatus newStatus, QString message,
				  QString location)
{
  emit newStormSignalStatus(newStatus, message);
}
