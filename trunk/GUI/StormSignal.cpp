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
#include <QFont>
#include <QRectF>
#include <QTextOption>

#include "StormSignal.h"

StormSignal::StormSignal(QSize hint, QWidget *parent)
  :QWidget(parent), hint(hint)
{  
  this->setObjectName("stormSignal");
  resize(hint);
  setBackgroundRole(QPalette::Base);
  // setAttribute(Qt::WA_StaticContents);
  connect(parent, SIGNAL(destroyed()), this, SLOT(close()));
  
  timer = new QTimer;

  on = true;

  pen.setWidth(1);

  hurrSymbol = new QPainterPath;

  hurrSymbol->moveTo(75,50);
  hurrSymbol->arcTo(25,25,50,50,0,112);
  hurrSymbol->arcTo(33.75,0,100,100,153,-47);
  hurrSymbol->arcTo(25,0,100,100,95,85);
  hurrSymbol->arcTo(25,25,50,50,180,110);
  hurrSymbol->arcTo(-33.75,0,100,100,333,-47);
  hurrSymbol->arcTo(-25,0,100,100,-85,85);

  flashing = false;
  
  currentStatus = Nothing;

}

StormSignal::~StormSignal()
{
  delete timer;
  delete hurrSymbol;
}

QSize StormSignal::sizeHint() const
{
  return(hint);
}


void StormSignal::paintEvent(QPaintEvent *event)
{

  QPainter *painter = new QPainter(this);
  painter->setRenderHint(QPainter::Antialiasing);
  painter->scale(width()/100, height()/100);
  painter->setPen(pen);
  painter->setBrush(QColor(255,255,255));
  /*
  if(red && on)
    painter->setBrush(QBrush(QColor(120,0,0)));

  if(yellow && on)
    painter->setBrush(QBrush(QColor(150, 150, 0)));

  if(green && on)
    painter->setBrush(QBrush(QColor(0,100,0)));
  */
  painter->drawPath(*hurrSymbol);

  QTextOption textHint(Qt::AlignCenter);
  textHint.setWrapMode(QTextOption::WordWrap);
  QString stormMessage;
  QFont font("Helvetica", 8, QFont::Bold);
  QFontMetrics fontMetrics(font);
  float fontHeight = fontMetrics.height();
  painter->setFont(font);
  painter->setBrush(QBrush(QColor(0,100,0)));
  QRectF wordBox(25,25,50,50);
  QRectF wordBox1(25,25-fontHeight*.5,50,50);
  QRectF wordBox2(25,25,50,50);
  QRectF wordBox3(25,25+fontHeight*.5,50,50);
  QRectF wordBox21(25,25-fontHeight*.3,50,50);
  QRectF wordBox22(25,25+fontHeight*.3,50,50);
  //  painter->drawRect(wordBox);

  switch(currentStatus)
    {
    case Nothing:
      break;
    case RapidIncrease:
      painter->drawText(wordBox1, QString("Rapid"), textHint);
      painter->drawText(wordBox2, QString("Pressure"), textHint);
      painter->drawText(wordBox3, QString("Rise"), textHint);
      break;
    case RapidDecrease:
      painter->drawText(wordBox1, QString("Rapid"), textHint);
      painter->drawText(wordBox2, QString("Pressure"), textHint);
      painter->drawText(wordBox3, QString("Fall"),textHint);
      break;
    case Ok:
      font = QFont("Helvetica", 14, QFont::Bold);
      painter->setFont(font);
      painter->drawText(wordBox, QString("OK"), textHint);
      break;
    case OutOfRange:
      painter->drawText(wordBox1, QString("Center"), textHint);
      painter->drawText(wordBox2, QString("out of"), textHint);
      painter->drawText(wordBox3, QString("Range"), textHint);
      break;
    case SimplexError:
      painter->drawText(wordBox21, QString("Simplex"), textHint);
      painter->drawText(wordBox22, QString("Error"), textHint);
      break;
    }

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

void StormSignal::changeStatus(StormSignalStatus status)
{
  
  currentStatus = status;

  switch(status)
    {
    case Nothing:             // Nothing at all
      break;
    case RapidIncrease:       // turn on flashing now
      //flashing = true;
      break;
    case RapidDecrease:       // turn on flashing now
      //flashing = true;
      break;
    case Ok:                  // turn off flashing now
      //flashing = false;
    case OutOfRange:          // turn off flashing now
      //flashing = false;
      break;
    case SimplexError:        
      break;
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
