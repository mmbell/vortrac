/*
 * StopLight.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 4/20/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef STOPLIGHT_H
#define STOPLIGHT_H

#include <QString>
#include <QPainter>
#include <QWidget>
#include <QPen>
#include <QBrush>
#include <QSize>
#include <QRadialGradient>
#include <QTimer>
#include <QEvent>
#include <QPaintEvent>
#include "IO/Message.h"

class StopLight:public QWidget
{

  Q_OBJECT
    
 public:
        StopLight(QSize hint = QSize(225,80), QWidget *parent = 0);
        ~StopLight();

	QSize sizeHint() const;
	int getCurrentColor() {return currentColor; }

 protected:
	void paintEvent(QPaintEvent *event);
 
 private:
        QRadialGradient *brightRed, *brightYellow, *brightGreen;
	bool red, yellow, green, flashing, on;
	QTimer *timer;
	QPen pen;
	QSize hint;
	int currentColor;
 
 public slots:
        void catchLog(const Message& message);
        void changeColor(StopLightColor light);
 
 signals:
        void log(const Message& message);

};
 
#endif
