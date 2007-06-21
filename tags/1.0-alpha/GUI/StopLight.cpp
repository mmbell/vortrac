/*
 * StopLight.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 4/20/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include <QColor>
#include <QRadialGradient>

#include "StopLight.h"

StopLight::StopLight(QSize hint, QWidget *parent)
  :QWidget(parent), hint(hint)
{  
  this->setObjectName("stopLight");
  resize(hint);
  setBackgroundRole(QPalette::Base);
  // setAttribute(Qt::WA_StaticContents);
  connect(parent, SIGNAL(destroyed()), this, SLOT(close()));
  
  brightRed = new QRadialGradient(5.5, 5.5, 4.5);
  brightRed->setColorAt(0, QColor(255,200,200));
  brightRed->setColorAt(1, Qt::red);
  
  brightYellow = new QRadialGradient(16.5,5.5,4.5,10.5,0);
  brightYellow->setColorAt(0, QColor(255,255,150));
  brightYellow->setColorAt(1, Qt::yellow);

  brightGreen = new QRadialGradient(26.5, 5.5, 4.5, 20.5,0);
  brightGreen->setColorAt(0, QColor(150,255,0));
  brightGreen->setColorAt(1, Qt::green);
  
  timer = new QTimer;

  red = false;
  yellow = false;
  green = false;
  on = true;

  currentColor = 0;

}

StopLight::~StopLight()
{
  delete brightRed;
  delete brightGreen;
  delete brightYellow;
  delete timer;
}

QSize StopLight::sizeHint() const
{
  return(hint);
}


void StopLight::paintEvent(QPaintEvent *event)
{

  QPainter *painter = new QPainter(this);
  painter->setRenderHint(QPainter::Antialiasing);
  painter->scale(width()/31, height()/11);
  painter->setPen(pen);
  painter->setBrush(QColor(255,217,0));
  painter->drawRect(QRectF(QPointF(0,0),QSize(31,11)));
  
  if(red && on)
    painter->setBrush(QBrush(*brightRed));
  else
    painter->setBrush(QBrush(QColor(120,0,0)));

  painter->drawEllipse(QRectF(1, 1, 9, 9));

  if(yellow && on)
    painter->setBrush(QBrush(*brightYellow));
  else 
    painter->setBrush(QBrush(QColor(150, 150, 0)));

  painter->drawEllipse(QRectF(11, 1, 9, 9));

  if(green && on)
    painter->setBrush(QBrush(*brightGreen));
  else
    painter->setBrush(QBrush(QColor(0,100,0)));

  painter->drawEllipse(QRectF(21, 1, 9, 9));

  if(flashing)
    on = !on;

  if (painter->isActive())
    painter->end();
  delete painter;
}

void StopLight::catchLog(const Message& message)
{
  emit log(message);
}

void StopLight::changeColor(StopLightColor light)
{
  red = false;
  yellow = false;
  green = false;
  flashing = false;

  currentColor = light;
  
  switch(light)
    {
    case AllOff:             // No lights
      break;
    case BlinkRed:           // Flashing Red
      flashing = true;
    case Red:                // Red 
      red = true;
      break;
    case BlinkYellow:        // Flashing Yellow
      flashing = true;
    case Yellow:             // Yellow
      yellow = true;
      break;
    case BlinkGreen:         // Flashing Green
      flashing = true;
    case Green:              // Green
      green = true;
      break;
    case AllOn:
      red = true;
      yellow = true;
      green = true;
    }

  if(flashing)
    {
      connect(timer, SIGNAL(timeout()), this, SLOT(repaint()));
      timer->start(30000);
      on = false;
    }
  else 
    {
      disconnect(timer, SIGNAL(timeout()), this, SLOT(repaint()));
      timer->stop(); 
      on = true;
    }
  
  update();
}

