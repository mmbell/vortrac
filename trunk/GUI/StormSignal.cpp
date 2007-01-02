/*
 * StormSignal.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 12/30/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include <QColor>

#include "StormSignal.h"

StormSignal::StormSignal(QSize hint, QWidget *parent)
  :QWidget(parent), hint(hint)
{  
  resize(hint);
  setBackgroundRole(QPalette::Base);
  // setAttribute(Qt::WA_StaticContents);
  connect(parent, SIGNAL(destroyed()), this, SLOT(close()));
  
  timer = new QTimer;

  red = false;
  yellow = false;
  green = false;
  on = true;

}

StormSignal::~StormSignal()
{
  delete timer;
}

QSize StormSignal::sizeHint() const
{
  return(hint);
}


void StormSignal::paintEvent(QPaintEvent *event)
{

  QPainter *painter = new QPainter(this);
  painter->setRenderHint(QPainter::Antialiasing);
  painter->scale(width()/31, height()/11);
  painter->setPen(pen);
  painter->setBrush(QColor(255,255,255));
  
  if(red && on)
    painter->setBrush(QBrush(QColor(120,0,0)));

  if(yellow && on)
    painter->setBrush(QBrush(QColor(150, 150, 0)));

  if(green && on)
    painter->setBrush(QBrush(QColor(0,100,0)));

  painter->drawRect(QRectF(QPointF(0,0),QSize(31,11)));

  if(flashing)
    on = !on;

  if (painter->isActive())
    painter->end();
  delete painter;
}

void StormSignal::catchLog(const Message& message)
{
  emit log(message);
}

void StormSignal::changeStatus(int light)
{
  red = false;
  yellow = false;
  green = false;
  flashing = false;
  
  switch(light)
    {
    case 0:             // No lights
      break;
    case 1:             // Flashing Red
      flashing = true;
    case 2:             // Red 
      red = true;
      break;
    case 3:             // Flashing Yellow
      flashing = true;
    case 4:             // Yellow
      yellow = true;
      break;
    case 5:             // Flashing Green
      flashing = true;
    case 6:             // Green
      green = true;
      break;
    default:
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
