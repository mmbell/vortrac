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
  this->setObjectName("log");
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

  Message::toScreen("log:constructor: "+workingDirectory.path());
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
    Message::toScreen("Log::setWorkingDirectory: could not copy "+logFile->fileName()+" to "+newDir.filePath(logFileName));
  }
  
  logFile->remove();
  //Message::toScreen("log:beforeChange: "+workingDirectory.path());
  workingDirectory = newDir;
  //Message::toScreen("log:afterChange: "+workingDirectory.path());
  logFileName = newFileName;
  
  QFile* oldLogFile = logFile;
  delete oldLogFile;
  logFile = new QFile(workingDirectory.filePath(logFileName));
  Message::toScreen(" after working dir changed file = "+logFile->fileName());
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
  Message::toScreen("Log: SaveLogFile : at location "+fileName);
  QString checkFileName(fileName);
  int lastSlash = checkFileName.lastIndexOf(QString("/"));
  if(lastSlash > 0) {
    checkFileName.truncate(lastSlash);
  }
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
      if(logFile->isOpen())
	Message::toScreen("When logging message: "+message+" logFile did not close but did not fail");
      return true;
    }

  Message::toScreen("Failed to write Log message to file");
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
    // Message::toScreen("The Good "+location+" : "+message);
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
    if(StopLightQueue.count()==0) {
      StopLightQueue.append(mostRecent);
      //      Message::toScreen("Inside "+mostRecent->location+": "+mostRecent->message+" @ i = zero");
    }
    else {
      int initialCount = StopLightQueue.count();
      emit newLogEntry(location+": "+message+"\n");
      
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
	Message::toScreen("Inside "+corrected->location+"  "+corrected->message+" @ i = "+QString().setNum(i));
	delete corrected;
      }
    }
  }
  else {
    //    Message::toScreen("The Bad "+location+" : "+message);
    // Add bad signals to the stack based on status....
    SSChange *mostRecent = new SSChange;
    mostRecent->status = newStatus;
    mostRecent->message = message;
    mostRecent->location = location;
    if(StormStatusQueue.count()==0) {
      StormStatusQueue.append(mostRecent);
      //      Message::toScreen("Inside "+mostRecent->location+": "+mostRecent->message+" @ i = zero");
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
  if((originalCount!=StormStatusQueue.count())||(originalCount==0)) {
    emit newStormSignalStatus(StormStatusQueue.at(0)->status,
			      StormStatusQueue.at(0)->message);
    return true;
  }
  else {
    emit newLogEntry(QString("StormStatusChange Was Not Handled Properly @"+location+"  "+message+"\n"));
    return false;
  }

  emit newStormSignalStatus(newStatus, message);
}
