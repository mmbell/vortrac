/*
 * DiagnosticPanel.cpp
 * VORTRAC
 *
 *  Created by Lisa Mauger on 04/09/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */


#include "DiagnosticPanel.h"
#include <QDateTime>
#include <QGroupBox>
#include <QTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QBrush>
#include <QGradient>
#include <QPen>
#include <QSize>
#include <QColorDialog>

DiagnosticPanel::DiagnosticPanel(QWidget *parent)
  :QWidget(parent)
{

  timer = new QTimer(this);
  timer->setSingleShot(false);
  timer->setInterval(1000);
  timer->start();

  QGroupBox *clockBox = new QGroupBox("Current Time UTC");
  clockBox->setAlignment(Qt::AlignHCenter);
 
  clock = new QLCDNumber(clockBox);
  clock->setSegmentStyle(QLCDNumber::Flat);
  clock->resize(210,150);

  QGridLayout *clockLayout = new QGridLayout;
  clockLayout->addWidget(clock,0,1,1,1);
  //clockLayout->setRowMinimumHeight(1,100);
  clockLayout->setRowStretch(0,8);
  clockBox->setLayout(clockLayout);
  //clockBox->resize(230,150);
  

  connect(timer, SIGNAL(timeout()), 
	  this, SLOT(updateClock()), Qt::DirectConnection);
  
  updateClock();

  //QPushButton *color = new QPushButton("color", this);
  //connect(color, SIGNAL(pressed()), this, SLOT(pickColor()));
  

  lights = new StopLight(QSize(225,80), this);
  connect(lights, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));

  QHBoxLayout *lightsLayout = new QHBoxLayout;
  lightsLayout->addStretch();
  lightsLayout->addWidget(lights);
  lightsLayout->addStretch();

  QLabel *vcpLabel = new QLabel(tr("Current Radar VCP"));
  vcp = new QLineEdit(QString("No VCP available"));
  vcp->setReadOnly(true);

  warning = new QLineEdit;
  warning->setReadOnly(true);

  QVBoxLayout *main = new QVBoxLayout();
  // main->addStretch();
  main->addWidget(clockBox);

  main->addWidget(vcpLabel);
  main->addWidget(vcp);
  
  main->addLayout(lightsLayout);
  main->addWidget(warning);

  main->addStretch();

  //main->addWidget(color);
  setLayout(main);

  //  dummy = 0;
  lights->changeColor(6);
}

DiagnosticPanel::~DiagnosticPanel()
{

}

void DiagnosticPanel::updateClock()
{
  QString displayTime;
  displayTime = QDateTime::currentDateTime().toUTC().toString("hh:mm");
  clock->display(displayTime);
  /*
  if(QDateTime::currentDateTime().time().second()%10 == 0){
    lights->changeColor(dummy);
    if(dummy < 6)
      dummy++;
    else 
      dummy = 0;
  }
  */

  update();

}

void DiagnosticPanel::catchLog(const Message& message)
{
  emit log(message);
}

void DiagnosticPanel::pickColor()

{
  QColorDialog::getColor(); 
}

void DiagnosticPanel::updateVCP(const int newVCP)
{
  vcp->clear();
  vcp->insert(QString().setNum(newVCP));
}
