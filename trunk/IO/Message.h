/*
 *  Message.h 
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/14/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

// Process error messages

#ifndef ERROR_H
#define ERROR_H

#include <QMessageBox>
#include <iostream>
#include <QString>

class Message
{

 public:
  Message(const char *errormsg = 0, int newProgress = 0, 
	const char *newLocation = 0, bool newSeverity = false);

  Message(const QString errormsg = QString(), int newProgress = 0,
	const QString newLocation = QString(), bool newSeverity = false);

  ~Message();

  QString getLogMessage() { return logMessage; }
  void setLogMessage(const QString newLogMessage);

  int getProgress() { return progress; }
  void setProgress(int progressPercentage);

  QString getLocation() { return location; }
  void setLocation(const QString newLocation);

  bool isSevere() { return severe; }
  void setSevere(bool newSeverity);

  static void report(const char *errormsg);
  static void report(const QString errormsg);
  static void toScreen(const char *message);
  static void toScreen(const QString message);

 private:
  QString logMessage;
  int progress;
  QString location;
  bool severe;

};

#endif
