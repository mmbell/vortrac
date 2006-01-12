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
#include "Message.h"
#include <QTextStream>

Message::Message(const char *errormsg, int newProgress, 
	     const char *newLocation, bool newSeverity)
{
  logMessage = QString(errormsg);
  progress = newProgress;
  location = newLocation;
  severe = newSeverity;
}

Message::Message(const QString errormsg, int newProgress,
	     const QString newLocation, bool newSeverity)
{
  logMessage = errormsg;
  progress = newProgress;
  location = newLocation;
  severe = newSeverity;
}
  
Message::~Message()
{
}

void Message::setLogMessage(const QString newLogMessage) 
{
  logMessage = newLogMessage;
}

void Message::setProgress(int progressPercentage)
{
  progress = progressPercentage;
}

void Message::setLocation(const QString newLocation)
{
  location = newLocation;
}

void Message::setSevere(bool newSeverity)
{
  severe = newSeverity;
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
