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
#include <QLineEdit>
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
  timer->setInterval(2000);
  timer->start();

  QGroupBox *clockBox = new QGroupBox("Current Time UTC");
  clockBox->setAlignment(Qt::AlignHCenter);
 
  clock = new QLCDNumber(clockBox);
  clock->setSegmentStyle(QLCDNumber::Flat);
  clock->resize(210,150);

  QGridLayout *clockLayout = new QGridLayout;
  clockLayout->addWidget(clock,0,1,1,1);
  clockLayout->setRowMinimumHeight(1,100);
  clockLayout->setRowStretch(0,9);
  clockBox->setLayout(clockLayout);
  clockBox->resize(230,150);
  

  connect(timer, SIGNAL(timeout()), 
	  this, SLOT(updateClock()), Qt::DirectConnection);
  
  updateClock();

  //QPushButton *color = new QPushButton("color", this);
  //connect(color, SIGNAL(pressed()), this, SLOT(pickColor()));

  QBrush redBrush(Qt::red);
  QBrush yellowBrush(Qt::yellow);
  QBrush greenBrush(Qt::green);
  QPen blackPen(Qt::black);
  QSize theSize(75,75);

  //red = new KeyPicture(4,redBrush, blackPen,theSize,this);
  //yellow = new KeyPicture(4,yellowBrush, blackPen, theSize, this);
  // green = new KeyPicture(4, greenBrush, blackPen, theSize, this);
  //QHBoxLayout *lights = new QHBoxLayout;
  //lights->addWidget(red);
  //lights->addWidget(yellow);
  //lights->addWidget(green);
  
  QLabel *vcpLabel = new QLabel(tr("Current Radar VCP"));
  QLineEdit *vcp = new QLineEdit;
  

  QVBoxLayout *main = new QVBoxLayout();
  // main->addStretch();
  main->addWidget(clockBox);
  // main->addWidget(color);
  // main->addLayout(lights);
  main->addWidget(vcpLabel);
  main->addWidget(vcp);
  main->addStretch();
  setLayout(main);

}

DiagnosticPanel::~DiagnosticPanel()
{

}

void DiagnosticPanel::updateClock()
{
  QString displayTime;
  displayTime = QDateTime::currentDateTime().toUTC().toString("hh:mm");
  clock->display(displayTime);
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
