/*
 * KeyPicture.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 8/9/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include <QImage>
#include <QPaintEvent>

#include "KeyPicture.h"

KeyPicture::KeyPicture(const int& i, QBrush brush, QPen pen, 
		       QSize hint, QWidget *parent)
  :QWidget(parent), paintFlag(i), brush(brush), pen(pen), hint(hint)
{  
  this->setObjectName("keyPicture");
  resize(hint);
  setBackgroundRole(QPalette::Base);
  setAttribute(Qt::WA_StaticContents);
  createImages();
  connect(parent, SIGNAL(destroyed()), this, SLOT(close()));
}

KeyPicture::KeyPicture(const int& i, QBrush brush, QPen pen, 
		       QPen stdPen1, QPen stdPen2, QSize hint, QWidget *parent)
  :QWidget(parent), paintFlag(i), brush(brush), pen(pen), 
   stdPen1(stdPen1), stdPen2(stdPen2), hint(hint)
{
  resize(hint);
  setBackgroundRole(QPalette::Base);
  setAttribute(Qt::WA_StaticContents);
  createImages();
  connect(parent, SIGNAL(destroyed()), this, SLOT(close()));
}


void KeyPicture::createImages()
{
  switch (paintFlag)
    {
    case 1:
      {
	QImage DropSondeImage(hint,QImage::Format_ARGB32_Premultiplied);
	QPainter *painter = new QPainter(&DropSondeImage);
	painter->setRenderHint(QPainter::Antialiasing);
	painter->setBrush(Qt::white);
	painter->drawRect(QRectF(QPointF(0,0),hint));
	painter->scale(width()/10, height()/10);
	painter->setPen(pen);
	painter->setBrush(brush);
	painter->drawEllipse(QRectF(2.5, 2.5, 5, 5));
	if(!DropSondeImage.save("images/DropSondeImage", "PNG"))
	  Q_ASSERT("Drop Image save Failed");
	if (painter->isActive())
	  painter->end();
	break;
      }
    case 2:
      {
	QImage pressureImage(hint, QImage::Format_ARGB32_Premultiplied);
	QPainter *painter = new QPainter(&pressureImage);
	painter->setRenderHint(QPainter::Antialiasing);
	painter->setBrush(Qt::white);
	painter->drawRect(QRectF(QPointF(0,0),hint));
	painter->scale(width()/10, height()/50);
	painter->setPen(stdPen2);
	
	painter->save();
	painter->translate(3,1);
	painter->drawLine(0,0,4,0);
	painter->restore();
	
	painter->save();
	painter->translate(QPointF(5,1));
	painter->drawLine(QPointF(0,0),QPointF(0,21.5));
	painter->restore();
	
	painter->save();  
	painter->translate(QPointF(5,27.5));
	painter->drawLine(QPointF(0,0),QPointF(0,21.5)); 
	painter->restore();
	
	painter->save();
	painter->translate(3,49);
	painter->drawLine(0, 0, 4, 0);
	painter->restore();
	
	painter->setPen(stdPen1);
	painter->save();
	painter->translate(3,12);
	painter->drawLine(0,0,4,0);
	painter->restore();
	
	painter->save();
	painter->translate(5,12);
	painter->drawLine(QPointF(0,0),QPointF(0,10.5));
	painter->restore();
	
	painter->save();
	painter->translate(QPointF(5,27.5));
	painter->drawLine(QPointF(0,0), QPointF(0, 10.5));
	painter->restore();
	
	painter->save();
	painter->translate(3,38);
	painter->drawLine(0,0,4,0);
	painter->restore();
	
	painter->setPen(pen);
	painter->setBrush(brush);
	painter->save();
	painter->translate(1,25);
	painter->drawLine(0,0,8,0);
	painter->restore();
	
	painter->save();
	painter->drawEllipse(QRectF(2.5,22.5,5,5));
	painter->restore();
	pressureImage.save("images/PressureImage", "PNG");
	if (painter->isActive())
	  painter->end();
	break;
      }

    case 3:
      {
	QImage rmwImage(hint,QImage::Format_ARGB32_Premultiplied);
	QPainter *painter = new QPainter(&rmwImage);
	painter->setRenderHint(QPainter::Antialiasing);
	painter->setBrush(Qt::white);
	painter->drawRect(QRectF(QPointF(0,0),hint));
	painter->scale(width()/10, height()/50);
	painter->setPen(stdPen2);
	
	painter->save();
	painter->translate(3,1);
	painter->drawLine(0,0,4,0);
	painter->restore();
	
	painter->save();
	painter->translate(5,1);
	painter->drawLine(QPointF(0,0),QPointF(0,21.5));
	painter->restore();
	
	painter->save();
	painter->translate(QPointF(5, 27.5));
	painter->drawLine(QPointF(0,0),QPointF(0,21.5)); 
	painter->restore();
	
	painter->save();
	painter->translate(3,49);
	painter->drawLine(0,0, 4, 0);
	painter->restore();
	
	painter->setPen(stdPen1);
	painter->save();
	painter->translate(3,12);
	painter->drawLine(0,0,4,0);
	painter->restore();
	
	painter->save();
	painter->translate(QPointF(5, 12));
	painter->drawLine(QPointF(0,0),QPointF(0,10.5));
	painter->restore();
	
	painter->save();
	painter->translate(QPointF(5,27.5));
	painter->drawLine(QPointF(0,0), QPointF(0, 10.5));
	painter->restore();
	
	painter->save();
	painter->translate(3,38);
	painter->drawLine(0,0,4,0);
	painter->restore();
	
	painter->setPen(pen);
	painter->setBrush(brush);
	
	painter->save();
	painter->translate(1,25);
	painter->drawLine(0,0,8,0);
	painter->restore();
	
	painter->save();
	painter->drawEllipse(QRectF(2.5,22.5,5,5));
	painter->restore();
	rmwImage.save("images/RMWImage", "PNG");
	if (painter->isActive())
	  painter->end();
	break;
      }
    }
}

QSize KeyPicture::sizeHint() const
{
  return(hint);
}


void KeyPicture::paintEvent(QPaintEvent *event)
{

  QPainter painter(this);
  QImage *displayImage = new QImage;
  QString *filename = new QString("images/");
  switch (paintFlag)
    {
    case 1:
      {
	filename->append("DropSondeImage");
	break;
      }
    case 2: 
      {
	filename->append("PressureImage");
	break;
      }
    case 3:
      {
	filename->append("RMWImage");
	break;
      }
    }
  displayImage->load(*filename, "PNG");
  if (!  displayImage->load(*filename, "PNG"))
    Q_ASSERT("Image would not load");
  painter.drawImage(QPoint(0,0), *displayImage);
  if (painter.isActive())
    painter.end();

  event->accept();
}

void KeyPicture::catchLog(const Message& message)
{
  emit log(message);
}
