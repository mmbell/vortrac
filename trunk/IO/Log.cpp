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
  this->setObjectName("Log");
  connect(this, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
  
  workingDirectory = QDir::current();
  logFileName = QString("autoLog");

  int i = 0;
  QString newName(logFileName);
  while(QFile::exists(workingDirectory.filePath(newName+".log")))
    {
      i++;
      newName = logFileName+QString().setNum(i);
    }

  logFileName = newName + ".log";
  logFile = new QFile(workingDirectory.filePath(logFileName));
  
  absoluteProgress = 0;
  //displayLocation = false;
  displayLocation = true;
  //  usingFile = false;
  // Message::toScreen("log:constructor: "+workingDirectory.path());
}

Log::~Log()
{
  delete logFile;
  for(int i = StopLightQueue.count()-1; i >= 0; i--) {
    delete StopLightQueue[i];
  }
  for(int i = StormStatusQueue.count()-1; i >= 0; i--) {
    delete StormStatusQueue[i];
  }
}

void Log::setWorkingDirectory(QDir& newDir)
{
 
  
  if (newDir == workingDirectory) {
    // Don't need to do anything
    return;
  }
  if(!newDir.exists()) {
    newDir.mkpath(newDir.path());
    if(!newDir.exists()) {
      emit log(Message(QString("Could not create directory "+newDir.path()+" check permissions"), 0, this->objectName()));
    }
  }
  if(!newDir.isAbsolute())
    newDir.makeAbsolute();

  QString newFileName("autoLog");
  QFile newLogFile(newDir.filePath(newFileName+".log"));
  int i = 0;
  QString newName(newFileName);
  while(QFile::exists(newDir.filePath(newName+".log")))
    {
      i++;
      newName = newFileName+QString().setNum(i);
      //      Message::toScreen("Trying..."+newDir.filePath(newName+".log"));
    }
  newFileName = newName+".log";
  newLogFile.setFileName(workingDirectory.filePath(newName+".log"));
  
  usingFile.lock();
  if(logFile->isOpen())
    logFile->close();
  
  if(!logFile->copy(newDir.filePath(newFileName))) {
    emit log(Message(QString("SetWorkingDirectory: Could not copy "+logFile->fileName()+" to "+newDir.filePath(newFileName)+".  May not be logging errors"),0,this->objectName(),Yellow,QString("Could not move log file!")));
    return;
  }
  
  logFile->remove();
  //Message::toScreen("log:beforeChange: "+workingDirectory.path());
  workingDirectory = newDir;
  //Message::toScreen("log:afterChange: "+workingDirectory.path());
  logFileName = newFileName;
  
  QFile* oldLogFile = logFile;
  delete oldLogFile;
  logFile = new QFile(workingDirectory.filePath(logFileName));
  logFile->setFileName(workingDirectory.filePath(logFileName));

  emit log(Message(QString("Log location after working dir changed, log file = "+logFile->fileName()),0,this->objectName(),Green));

  usingFile.unlock();

}

void Log::setLogFileName(QString& newName) 
{
  
  // check and see if it has a file extension
  if(!newName.contains(QString(".log")))
    if(!newName.contains(QString(".")))
      newName.append(QString(".log"));
	// if not, then add .log extension

  if(QFile::exists(workingDirectory.filePath(newName)))
    {
      QFile::remove(workingDirectory.filePath(newName));
    }
  
  usingFile.lock();

  if(!logFile->copy(workingDirectory.filePath(newName)))
    //Message::toScreen("Log::setWorkingDirectory: could not copy "+logFile->fileName()+" to "+workingDirectory.filePath(newName));
    logFileName = newName;
  logFile->setFileName(workingDirectory.filePath(logFileName));
  //logFile = new QFile(workingDirectory.filePath(logFileName));
  usingFile.unlock();
}

bool Log::saveLogFile()
{
  QString saveName=QFileDialog::getSaveFileName(this, QString(tr("Save Status Log File as...")), workingDirectory.path(), QString(tr("Text Files *.txt")));
  
  if(!saveName.isEmpty()) {
    usingFile.lock();
    if(logFile->copy(saveName)) {
      usingFile.unlock();
      return true;
    }
    else {
      Message::toScreen(tr("Failed to save log file"));
      usingFile.unlock();
      return false;
    }
  }
  return false;
}

bool Log::saveLogFile(const QString& fileName)
{
  Message::toScreen("Log: SaveLogFile : at location "+fileName);
  QString checkFileName(fileName);
  int lastSlash = checkFileName.lastIndexOf(QString("/"));
  if(lastSlash > 0) {
    checkFileName.truncate(lastSlash);
  }
  QDir check(checkFileName);
  QString newFileName;

  if(check.isAbsolute())
    newFileName = fileName;
  else 
    newFileName = workingDirectory.filePath(fileName);
  
  if(QFile::exists(newFileName))
    QFile::remove(newFileName);

  usingFile.lock();
  if(check.isAbsolute()) {
    if(logFile->copy(fileName)) {
      usingFile.unlock();
      return true;
    }
    else {
      Message::toScreen(tr("Failed to save log file"));
      usingFile.unlock();
      return false;
    }
  }
  else {
    if(logFile->copy(workingDirectory.filePath(fileName))) {
      usingFile.unlock();
      return true;
    }
    else {
      Message::toScreen(tr("Failed to save log file"));
      usingFile.unlock();
      return false;
    }
  }
}

void Log::catchLog(const Message& logEntry)
{
  Message* logg = new Message(logEntry);
  QString message = logg->getLogMessage();
  int progress = logg->getProgress();
  QString location = logg->getLocation();
  StopLightColor stopLightColor = logg->getColor();
  QString stopLightMessage = logg->getStopLightMessage();
  StormSignalStatus stormSignalStatus = logg->getStatus();
  QString stormSignalMessage = logg->getStormSignalMessage();
  
  if(message!=QString()) {
    if(displayLocation && (location!=QString())) {
      message = location+": "+message;
    }
    message+="\n";
    messagesWaiting.append(message);
    emit(newLogEntry(message));
    writeToFile();
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
    if(!handleStopLightUpdate(stopLightColor,stopLightMessage,location)) {
      Message::toScreen("Log:Trouble Logging StopLight Change! "+location+" : "+stopLightMessage);
      emit log(Message(QString("Trouble Logging StopLight Change! "+location+" : "+stopLightMessage),0,this->objectName()));
    }
  }
  if((stormSignalStatus!=Nothing) || (stormSignalMessage!=QString())) {
    if(!handleStormSignalUpdate(stormSignalStatus,stormSignalMessage,location))
      {
	Message::toScreen("Log:Trouble Logging StormStatus Change! "+location+" : "+stormSignalMessage);
	emit log(Message(QString("Trouble Logging StormStatus Change! "+location+" : "+stormSignalMessage),0,this->objectName()));
    }
  }

  delete logg;
}

bool Log::writeToFile()
{
  if(usingFile.tryLock()) {
    if(logFile->isOpen()) {
      logFile->close();
      //Message::toScreen("LOG: WRITETOFILE: LogFile "+logFile->fileName()+" was open and then I closed it");
	}
    //Message::toScreen("LOG: WRITETOFILE: Trying To Open "+logFile->fileName());
    if(logFile->open(QIODevice::Append)) {
      while((messagesWaiting.count() > 0)) {
	logFile->write(messagesWaiting.first().toAscii());
	QString message = messagesWaiting.takeFirst();
      }
      logFile->close();
      if(logFile->isOpen()) {
	Message::toScreen("When logging messags logFile did not close but did not fail");
	usingFile.unlock();
	return false;
      }
    }
    usingFile.unlock();
    return true;
  }
  return false;
}

bool Log::handleStopLightUpdate(StopLightColor newColor, QString message, 
				QString location)
{
  // This is the dummy signal do nothing
  if(newColor == AllOff)
    return true;
    
  // Check to see if update is an error or a corrected error
  int originalCount = StopLightQueue.count();
  
  if((newColor == Green)||(newColor == BlinkGreen)) {
    if(StopLightQueue.count()==0)
      return true;
    for(int i = StopLightQueue.count()-1; i >= 0; i--) {
      if((StopLightQueue.at(i)->location == location) ||
	 (StopLightQueue.at(i)->location == QString())) {
	SLChange *corrected = StopLightQueue.value(i);
	StopLightQueue.removeAt(i);
	//Message::toScreen("Inside "+corrected->location+"  "+corrected->message+" @ i = "+QString().setNum(i));
	delete corrected;
      }
    }
    if(originalCount == StopLightQueue.count()) {
      // We got a good signal but couldn't find a bad one to remove
      // I don't see this as a problem
      return true;
    }
  }
  else {
    //    Message::toScreen("The Bad "+location+" : "+message);
    // Add bad signals to the stack based on color....
    SLChange *mostRecent = new SLChange;
    mostRecent->color = newColor;
    mostRecent->message = message;
    mostRecent->location = location;
    emit newLogEntry(location+": "+message+"\n");
    if(StopLightQueue.count()==0) {
      StopLightQueue.append(mostRecent);
      //      Message::toScreen("Inside "+mostRecent->location+": "+mostRecent->message+" @ i = zero");
    }
    else {
      int initialCount = StopLightQueue.count();
      //emit newLogEntry(location+": "+message+"\n");
      
      // Make sure we don't already have this signal
      for(int i = 0; i < initialCount; i++) {
	if(mostRecent == StopLightQueue.at(i)) {
	  // If we have an identical one it will do us no good 
	  // to add another because the corrections 
	  // process will not clear up multiple
	  delete mostRecent;
	  return true;
	}
      }
	
       //  If not we should figure out where to put it

      for(int i = 0; (i < initialCount) 
	    && (initialCount == StopLightQueue.count()); i++) {
	
	// Figure out where the signal goes in the lineup
	if(mostRecent->color < StopLightQueue.at(i)->color) {
	  
	  // Append according to color order 
	  StopLightQueue.insert(i+1, mostRecent);
	  // Message::toScreen("Inside "+mostRecent->location+"  "+mostRecent->message+" @ i = "+QString().setNum(i));
	}
      }
      if(initialCount==StopLightQueue.count()) {
	StopLightQueue.append(mostRecent);
	//Message::toScreen("Inside "+mostRecent->location+"  "+mostRecent->message+" @ i = deadLast"+QString().setNum(StopLightQueue.count()));
      }
    }
  }

  if(StopLightQueue.count()==0) {
    emit newStopLightColor(Green, QString());
    return true;
  }
  if((originalCount!=StopLightQueue.count())||(originalCount==0)) {
    emit newStopLightColor(StopLightQueue.at(0)->color,
			   StopLightQueue.at(0)->message);
    return true;
  }
  else {
    emit newLogEntry(QString("StopLightChange Was Not Handled Properly @"+location+"  "+message+"\n"));
    return false;
  }
}

bool Log::handleStormSignalUpdate(StormSignalStatus newStatus, QString message,
				  QString location)
{

  // Priority set up this way

  // if we have an OutOfRange or SimplexError those are turned off by Ok
  // if we have RapidIncrease or RapidDecrease those are turned off by Ok 
  //       from the same object, but not Ok in general
  // Simplex Error has the highest visualization priority
  // OutOfRange has the second highest visualization priority
  // RapidIncrease has the third highest visualization priority
  // RapidDecrease has the forth
  // Ok has the fifth
  // Nothing does nothing

  // This is the dummy signal do nothing
  if(newStatus == Nothing)
    return true;
    
  // Check to see if update is an error or a corrected error
  int originalCount = StormStatusQueue.count();
  
  if(newStatus == Ok) {
    // Message::toScreen("The Good "+location+" : "+message);
    if(StormStatusQueue.count()==0)
      return true;
    for(int i = StormStatusQueue.count()-1; i >= 0; i--) {
      if((StormStatusQueue.at(i)->location == location) ||
	 (StormStatusQueue.at(i)->location == QString())||
	 (StormStatusQueue.at(i)->status==OutOfRange)||
	 (StormStatusQueue.at(i)->status==SimplexError)) {
	SSChange *corrected = StormStatusQueue.value(i);
	StormStatusQueue.removeAt(i);
	//Message::toScreen("Inside "+corrected->location+"  "+corrected->message+" @ i = "+QString().setNum(i));
	delete corrected;
      }
    }
    if(originalCount==StormStatusQueue.count()) {
      // Couldn't match this good signal with a bad one
      // not a problem for me!
      return true;
    }
  }
  else {
    // Add bad signals to the stack based on status....
    SSChange *mostRecent = new SSChange;
    mostRecent->status = newStatus;
    mostRecent->message = message;
    mostRecent->location = location;
    if(StormStatusQueue.count()==0) {
      emit newLogEntry(location+": "+message+"\n");
      StormStatusQueue.append(mostRecent);
    }
    else {
      int initialCount = StormStatusQueue.count();
      emit newLogEntry(location+": "+message+"\n");
      
      // Make sure we don't already have this signal
      for(int i = 0; i < initialCount; i++) {
	if(mostRecent == StormStatusQueue.at(i)) {
	  // If we have an identical one it will do us no good 
	  // to add another because the corrections 
	  // process will not clear up multiple
	  delete mostRecent;
	  return true;
	}
      }
	
       //  If not we should figure out where to put it

      for(int i = 0; (i < initialCount) 
	    && (initialCount == StormStatusQueue.count()); i++) {
	
	// Figure out where the signal goes in the lineup
	if(mostRecent->status >= StormStatusQueue.at(i)->status) {
	  
	  // Append according to status order 
	  StormStatusQueue.insert(i, mostRecent);
	  // Message::toScreen("Inside "+mostRecent->location+"  "+mostRecent->message+" @ i = "+QString().setNum(i));
	}
      }
      if(initialCount==StormStatusQueue.count()) {
	StormStatusQueue.append(mostRecent);
	//Message::toScreen("Inside "+mostRecent->location+"  "+mostRecent->message+" @ i = deadLast"+QString().setNum(StormStatusQueue.count()));
      }
    }
  }

  if(StormStatusQueue.count()==0) {
    emit newStormSignalStatus(Ok, QString());
    return true;
  }
  if((originalCount!=StormStatusQueue.count())||(originalCount==0)||
     (StormStatusQueue.at(0)->status==SimplexError)||
     (StormStatusQueue.at(0)->status==OutOfRange)) {
    emit newStormSignalStatus(StormStatusQueue.at(0)->status,
			      StormStatusQueue.at(0)->message);
    return true;
  }
  else {
    emit newLogEntry(QString("StormStatusChange Was Not Handled Properly @"+location+"  "+message+"\n"));
    return false;
  }
  // Do we need this it is not getting called
  emit newStormSignalStatus(newStatus, message);
}
