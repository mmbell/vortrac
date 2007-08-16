/*
 *
 * CappiDisplay.h
 *
 * Created by Michael Bell on 8/26/06
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef CAPPIDISPLAY_H
#define CAPPIDISPLAY_H

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QWidget>
#include <QBrush>
#include <QMutex>
#include "DataObjects/GriddedData.h"

class CappiDisplay : public QWidget
{
    Q_OBJECT

public:
    CappiDisplay(QWidget *parent = 0);

    bool openImage(const QString &fileName);
    bool saveImage(const QString &fileName, const char *fileFormat);

    float getMaxRec() { return maxRec; }
    float getMaxApp() { return maxApp; }
						
public slots:
  void clearImage();
  void constructImage(const GriddedData* cappi);
  void setGBVTDResults(const float& xP, const float& yP, 
		       const float& sMin, const float& sMax, 
		       const float& vMax);
  void exit();
	
signals:
    void hasImage(bool imageAvailable); 

protected:
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    void resizeImage(QImage *image, const QSize &newSize);
    QString cappiLabel;
    QImage image;
    QMutex imageHolder;
    int iDim, jDim;
    QImage legendImage;
    QPoint lastPoint;
    int PaintEngineMode;
    bool exitNow;
    QColor backColor;
    float maxApp;
    float maxRec;
    float maxVel;
    float minVel;
    float velIncr;
    bool hasGBVTDInfo;
    float xPercent, yPercent;
    float simplexMin, simplexMax;
    float vortexMax;
    

};

#endif
