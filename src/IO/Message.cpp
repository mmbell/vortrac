/*
 *  Message.cpp 
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/14/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

// Process error messages
#include "IO/Message.h"
#include <QTextStream>
using namespace Qt;

/*Message::Message(const char *errormsg, int newProgress, 
		 const char *newLocation, 
		 StopLightColor newColor, const char *newStopLightMessage, 
		 StormSignalStatus newStatus, 
		 const char *newStormSignalMessage)
{
  logMessage = QString(errormsg);
  progress = newProgress;
  location = newLocation;
  color = newColor;
  stopLightMessage = QString(newStopLightMessage);
  status = newStatus;
  stormSignalMessage = QString(newStormSignalMessage);
} */

Message::Message(const QString errormsg, int newProgress,
		 const QString newLocation, 
		 StopLightColor newColor, 
		 const QString newStopLightMessage, 
		 StormSignalStatus newStatus, 
		 const QString newStormSignalMessage)
{
  logMessage = errormsg;
  progress = newProgress;
  location = newLocation;
  color = newColor;
  stopLightMessage = newStopLightMessage;
  status = newStatus;
  stormSignalMessage = newStormSignalMessage;
}

Message::Message(const Message& other)
{
	this->logMessage = other.logMessage;
	this->progress = other.progress;
	this->location = other.location;
	this->color = other.color;
	this->stopLightMessage = other.stopLightMessage;
	this->status = other.status;
	this->stormSignalMessage = other.stormSignalMessage;
}

Message::~Message()
{
}

void Message::setLogMessage(const QString newLogMessage) 
{
  logMessage = newLogMessage;
}

void Message::setLogMessage(const char *newLogMessage)
{
  logMessage = QString(newLogMessage);
}

void Message::setProgress(int progressPercentage)
{
  progress = progressPercentage;
}

void Message::setLocation(const QString newLocation)
{
  location = newLocation;
}

void Message::setLocation(const char *newLocation)
{
  location = QString(newLocation);
}

void Message::setColor(StopLightColor newColor)
{
  color = newColor;
}

void Message::setStopLightMessage(const QString newStopLightMessage)
{
  stopLightMessage = newStopLightMessage;
}

void Message::setStopLightMessage(const char *newStopLightMessage)
{
  stopLightMessage = QString(newStopLightMessage);
}

void Message::setStatus(StormSignalStatus newStatus)
{
  status = newStatus;
}

void Message::setStormSignalMessage(const QString newMessage)
{
  stormSignalMessage = newMessage;
}

void Message::setStormSignalMessage(const char* newMessage)
{
  stormSignalMessage = QString(newMessage);
}

void Message::report(const char *errormsg)
{
  // Pop up a message box with this info
  QWidget parent;
  const QString caption("VORTRAC");
  const QString error(errormsg);
  QMessageBox::information(&parent,caption,error);
}

void Message::report(const QString errormsg)
{
  QWidget parent;
  const QString caption("VORTRAC");
  QMessageBox::information(&parent, caption, errormsg);

}

void Message::toScreen(const char *message)
{
  QString str(message);
  QTextStream out(stdout);
  out << str << endl;
}

void Message::toScreen(const QString message)
{
  QTextStream out(stdout);
  out << message << endl;
}
