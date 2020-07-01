/*
 * KeyPicture.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 8/9/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef KEYPICTURE_H
#define KEYPICTURE_H

#include <QString>
#include <QPainter>
#include <QWidget>
#include <QPen>
#include <QBrush>
#include <QSize>
#include "IO/Message.h"

class KeyPicture:public QWidget
{

  Q_OBJECT
    
 public:
        KeyPicture(const int& i,QBrush brush, QPen pen, QSize hint, 
		   QWidget *parent = 0);
        // This constructor is designed for making the dropsonde picture 
        // on the key that pops up to explaine the pressure rmw graph


        KeyPicture(const int& i, QBrush brush, QPen pen, QPen stdPen1, 
		   QPen stdPen2, QSize hint, QWidget *parent = 0);

	// This constructor is designed for making the point with std
	// deviation bars that is used in the key for rmw and pressure points

	QSize sizeHint() const;

	void createImages();

 protected:
	void paintEvent(QPaintEvent *event);
 
 private:
	int paintFlag;
	QBrush brush;
	QPen pen, stdPen1, stdPen2;
	QSize hint;
	QString fileName;
 
 public slots:
        void catchLog(const Message& message);
 
 signals:
        void log(const Message& message);

};
 
#endif
