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
    this->setObjectName("Storm Signal");
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

    painter->drawPath(*hurrSymbol);

    QTextOption textHint(Qt::AlignCenter);
    textHint.setWrapMode(QTextOption::WordWrap);
    QString stormMessage;
    painter->setBrush(QBrush(QColor(0,100,0)));
    QRectF wordBox(25,25,50,50);
    //QRectF wordBox1(30,30,40,40);
    QRectF wordBox1(25,25,50,50);
    painter->setFont(QFont(QString("Ariel"),14));

    switch(currentStatus)
    {
    case Nothing:
        break;
    case RapidIncrease:
        stormMessage = QString("Rapid Pressure Rise");
        break;
    case RapidDecrease:
        stormMessage = QString("Rapid Pressure Fall");
        break;
    case Ok:
        stormMessage = QString("OK");
        break;
    case OutOfRange:
        stormMessage = QString("Center out of Range");
        wordBox = wordBox1;
        break;
    case SimplexError:
        wordBox = wordBox1;
        stormMessage = QString("Center Error");
        break;
    }
    while(stormMessage!=QString()) {
        QRectF br = painter->boundingRect(wordBox, stormMessage, textHint);
        if(br.width()*br.height() < wordBox.width()*wordBox.height())
            break;
        QFont current = painter->font();
        current.setPointSize(current.pointSize()-1);
        painter->setFont(current);
    }
    painter->drawText(wordBox, stormMessage, textHint);

    if(flashing)
        on = !on;

    if (painter->isActive())
        painter->end();
    delete painter;

    event->accept();
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
