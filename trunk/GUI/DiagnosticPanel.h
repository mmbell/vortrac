/*
 * DiagnosticPanel.h
 * VORTRAC
 *
 *  Created by Lisa Mauger on 04/09/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef DIAGNOSTICPANEL_H
#define DIAGNOSTICPANEL_H

#include <QTimer>
#include <QLCDNumber>
#include <QPushButton>
#include <QColor>
#include <QLineEdit>

#include "Message.h"
#include "StopLight.h"

class DiagnosticPanel : public QWidget
{

Q_OBJECT

 public:
      DiagnosticPanel(QWidget *parent = 0);
      ~DiagnosticPanel();

 public slots:
      void catchLog(const Message& message);
      void pickColor();
      void testLight();
      void updateVCP(const int newVCP);
 
 private:
      QTimer *timer;
      QLCDNumber *clock;
      QLineEdit *vcp, *warning;
      StopLight *lights;
      int dummy;

 private slots:
      void updateClock();

 protected:

 signals:
      void log(const Message& message); 

};

#endif
