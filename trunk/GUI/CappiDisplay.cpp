/*
 *
 * CappiDisplay.cpp
 *
 * Created by Michael Bell on 8/26/06
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include <QtGui>

#include "CappiDisplay.h"

CappiDisplay::CappiDisplay(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StaticContents);
    PaintEngineMode = -5;
    connect(this, SIGNAL(hasImage(bool)),
	    this, SLOT(setVisible(bool)));
    emit hasImage(false);
    exitNow = false;
	
	//Set the palette
	image = QImage(50,50,QImage::Format_Indexed8);
	image.setNumColors(256);
	image.setColor(0, qRgb(0,0,0));
	image.setColor(1, qRgb(255, 255, 255));
	image.setColor(2, qRgb((int)(.469*255), (int)(.020*255), (int)(.640*255)));
	image.setColor(3, qRgb((int)(.403*255), (int)(.227*255), (int)(.559*255)));
	image.setColor(4, qRgb((int)(.164*255), (int)(.055*255), (int)(.582*255)));
	image.setColor(5, qRgb((int)(.227*255), (int)(.055*255), (int)(.672*255)));
	image.setColor(6, qRgb((int)(.289*255), (int)(.055*255), (int)(.766*255)));
	image.setColor(7, qRgb((int)(.352*255), (int)(.141*255), (int)(.898*255)));
	image.setColor(8, qRgb((int)(.414*255), (int)(.375*255), (int)(.996*255)));
	image.setColor(9, qRgb((int)(.445*255), (int)(.559*255), (int)(.996*255)));
	image.setColor(10, qRgb((int)(.281*255), (int)(.590*255), (int)(.602*255)));
	image.setColor(11, qRgb((int)(.188*255), (int)(.523*255), (int)(.371*255)));
	image.setColor(12, qRgb((int)(.004*255), (int)(.445*255), (int)(.000*255)));
	image.setColor(13, qRgb((int)(.000*255), (int)(.492*255), (int)(.000*255)));
	image.setColor(14, qRgb((int)(.000*255), (int)(.539*255), (int)(.000*255)));
	image.setColor(15, qRgb((int)(.059*255), (int)(.586*255), (int)(.059*255)));
	image.setColor(16, qRgb((int)(.176*255), (int)(.633*255), (int)(.176*255)));
	image.setColor(17, qRgb((int)(.289*255), (int)(.680*255), (int)(.289*255)));
	image.setColor(18, qRgb((int)(.402*255), (int)(.723*255), (int)(.402*255)));
	image.setColor(19, qRgb((int)(.520*255), (int)(.770*255), (int)(.520*255)));
	image.setColor(20, qRgb((int)(.633*255), (int)(.816*255), (int)(.633*255)));
	image.setColor(21, qRgb((int)(.750*255), (int)(.863*255), (int)(.750*255)));
	image.setColor(22, qRgb((int)(.863*255), (int)(.910*255), (int)(.863*255)));
	image.setColor(23, qRgb((int)(.938*255), (int)(.906*255), (int)(.703*255)));
	image.setColor(24, qRgb((int)(.938*255), (int)(.859*255), (int)(.352*255)));
	image.setColor(25, qRgb((int)(.938*255), (int)(.812*255), (int)(.000*255)));
	image.setColor(26, qRgb((int)(.938*255), (int)(.766*255), (int)(.023*255)));
	image.setColor(27, qRgb((int)(.938*255), (int)(.719*255), (int)(.055*255)));
	image.setColor(28, qRgb((int)(.926*255), (int)(.672*255), (int)(.086*255)));
	image.setColor(29, qRgb((int)(.871*255), (int)(.625*255), (int)(.117*255)));
	image.setColor(30, qRgb((int)(.816*255), (int)(.578*255), (int)(.148*255)));
	image.setColor(31, qRgb((int)(.758*255), (int)(.531*255), (int)(.180*255)));
	image.setColor(32, qRgb((int)(.703*255), (int)(.484*255), (int)(.211*255)));
	image.setColor(33, qRgb((int)(.648*255), (int)(.438*255), (int)(.242*255)));
	image.setColor(34, qRgb((int)(.590*255), (int)(.391*255), (int)(.250*255)));
	image.setColor(35, qRgb((int)(.535*255), (int)(.344*255), (int)(.250*255)));
	image.setColor(36, qRgb((int)(.485*255), (int)(.328*255), (int)(.297*255)));
	image.setColor(37, qRgb((int)(.629*255), (int)(.312*255), (int)(.375*255)));
	image.setColor(38, qRgb((int)(.625*255), (int)(.003*255), (int)(.000*255)));
	image.setColor(39, qRgb((int)(.718*255), (int)(.086*255), (int)(.188*255)));
	image.setColor(40, qRgb((int)(.813*255), (int)(.148*255), (int)(.273*255)));
	image.setColor(41, qRgb((int)(.879*255), (int)(.211*255), (int)(.355*255)));
	image.setColor(42, qRgb((int)(.949*255), (int)(.273*255), (int)(.355*255)));
	image.setColor(43, qRgb((int)(1.000*255), (int)(.012*255), (int)(.000*255)));
	/*
	image = QImage(50,50,QImage::Format_RGB32);
	//image.setNumColors(256);
	colorMap[0] = qRgb(0,0,0);
	colorMap[1] = qRgb(255, 255, 255);
	colorMap[2] = qRgb((int)(.469*255, (int)(.020*255, (int)(.640*255);
	colorMap[3] = qRgb((int)(.403*255, (int)(.227*255, (int)(.559*255);
	colorMap[4] = qRgb((int)(.164*255, (int)(.055*255, (int)(.582*255);
	colorMap[5] = qRgb((int)(.227*255, (int)(.055*255, (int)(.672*255);
	colorMap[7] = qRgb((int)(.352*255, (int)(.141*255, (int)(.898*255);
	colorMap[8] = qRgb((int)(.414*255, (int)(.375*255, (int)(.996*255);
	colorMap[9] = qRgb((int)(.445*255, (int)(.559*255, (int)(.996*255);
	colorMap[10] = qRgb((int)(.281*255, (int)(.590*255, (int)(.602*255);
	colorMap[11] = qRgb((int)(.188*255, (int)(.523*255, (int)(.371*255);
	colorMap[12] = qRgb((int)(.004*255, (int)(.445*255, (int)(.000*255);
	colorMap[13] = qRgb((int)(.000*255, (int)(.492*255, (int)(.000*255);
	colorMap[14] = qRgb((int)(.000*255, (int)(.539*255, (int)(.000*255);
	colorMap[15] = qRgb((int)(.059*255, (int)(.586*255, (int)(.059*255);
	colorMap[16] = qRgb((int)(.176*255, (int)(.633*255, (int)(.176*255);
	colorMap[17] = qRgb((int)(.289*255, (int)(.680*255, (int)(.289*255);
	colorMap[18] = qRgb((int)(.402*255, (int)(.723*255, (int)(.402*255);
	colorMap[19] = qRgb((int)(.520*255, (int)(.770*255, (int)(.520*255);
	colorMap[20] = qRgb((int)(.633*255, (int)(.816*255, (int)(.633*255);
	colorMap[21] = qRgb((int)(.750*255, (int)(.863*255, (int)(.750*255);
	colorMap[22] = qRgb((int)(.863*255, (int)(.910*255, (int)(.863*255);
	colorMap[23] = qRgb((int)(.938*255, (int)(.906*255, (int)(.703*255);
	colorMap[24] = qRgb((int)(.938*255, (int)(.859*255, (int)(.352*255);
	colorMap[25] = qRgb((int)(.938*255, (int)(.812*255, (int)(.000*255);
	colorMap[26] = qRgb((int)(.938*255, (int)(.766*255, (int)(.023*255);
	colorMap[27] = qRgb((int)(.938*255, (int)(.719*255, (int)(.055*255);
	colorMap[28] = qRgb((int)(.926*255, (int)(.672*255, (int)(.086*255);
	colorMap[29] = qRgb((int)(.871*255, (int)(.625*255, (int)(.117*255);
	colorMap[30] = qRgb((int)(.816*255, (int)(.578*255, (int)(.148*255);
	colorMap[31] = qRgb((int)(.758*255, (int)(.531*255, (int)(.180*255);
	colorMap[32] = qRgb((int)(.703*255, (int)(.484*255, (int)(.211*255);
	colorMap[33] = qRgb((int)(.648*255, (int)(.438*255, (int)(.242*255);
	colorMap[34] = qRgb((int)(.590*255, (int)(.391*255, (int)(.250*255);
	colorMap[35] = qRgb((int)(.535*255, (int)(.344*255, (int)(.250*255);
	colorMap[36] = qRgb((int)(.485*255, (int)(.328*255, (int)(.297*255);
	colorMap[37] = qRgb((int)(.629*255, (int)(.312*255, (int)(.375*255);
	colorMap[38] = qRgb((int)(.625*255, (int)(.003*255, (int)(.000*255);
	colorMap[39] = qRgb((int)(.718*255, (int)(.086*255, (int)(.188*255);
	colorMap[40] = qRgb((int)(.813*255, (int)(.148*255, (int)(.273*255);
	colorMap[41] = qRgb((int)(.879*255, (int)(.211*255, (int)(.355*255);
	colorMap[42] = qRgb((int)(.949*255, (int)(.273*255, (int)(.355*255);
	colorMap[43] = qRgb((int)(1.000*255, (int)(.012*255, (int)(.000*255); */
}

bool CappiDisplay::openImage(const QString &fileName)
{
    QImage loadedImage;
    if (!loadedImage.load(fileName))
        return false;

    QSize newSize = loadedImage.size().expandedTo(size());
    resizeImage(&loadedImage, newSize);
    image = loadedImage;
    update();
    return true;
}

bool CappiDisplay::saveImage(const QString &fileName, const char *fileFormat)
{
    QImage visibleImage = image;
    resizeImage(&visibleImage, size());

    if (visibleImage.save(fileName, fileFormat)) {
        return true;
    } else {
        return false;
    }
}

void CappiDisplay::clearImage()
{
    //image.fill(qRgb(255, 255, 255));
    QString message;
    message.setNum(PaintEngineMode);
    QTextStream out(stdout);
    out << message << endl;
    update();
}

void CappiDisplay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        lastPoint = event->pos();
    }
}

void CappiDisplay::paintEvent(QPaintEvent * /* event */)
{
  QPainter painter(this);
    painter.drawImage(QPoint(0, 0), image);
	painter.drawText(QPoint(0, this->height()), cappiLabel);
    if(painter.paintEngine()->type() == QPaintEngine::X11)
      PaintEngineMode = -1;
    if(painter.paintEngine()->type() == QPaintEngine::Windows)
      PaintEngineMode = 1;
    if(painter.paintEngine()->type() == QPaintEngine::MacPrinter)
      PaintEngineMode = 4;
    if(painter.paintEngine()->type() == QPaintEngine::CoreGraphics)
      PaintEngineMode = 3;
    if(painter.paintEngine()->type() == QPaintEngine::QuickDraw)
      PaintEngineMode = 2;
    if(painter.paintEngine()->type() == QPaintEngine::QWindowSystem)
      PaintEngineMode = 5;
    if(painter.paintEngine()->type() == QPaintEngine::PostScript)
      PaintEngineMode = 6;
    if(painter.paintEngine()->type() == QPaintEngine::OpenGL)
      PaintEngineMode = 7;
    if(painter.paintEngine()->type() == QPaintEngine::Picture)
      PaintEngineMode = 8;
    if(painter.paintEngine()->type() == QPaintEngine::SVG)
      PaintEngineMode = 9;
    if(painter.paintEngine()->type() == QPaintEngine::Raster)
      PaintEngineMode = 10;
}

void CappiDisplay::resizeEvent(QResizeEvent *event)
{
    /* if (width() > image.width() || height() > image.height()) {
        int newWidth = qMax(width() + 128, image.width());
        int newHeight = qMax(height() + 128, image.height());
        resizeImage(&image, QSize(newWidth, newHeight));
        update();
    } */
    QWidget::resizeEvent(event);
}

void CappiDisplay::resizeImage(QImage *image, const QSize &newSize)
{
    if (image->size() == newSize)
        return;

    QImage newImage(newSize, QImage::Format_Indexed8);
    newImage.fill(qRgb(255, 255, 255));
    QPainter painter(&newImage);
    painter.drawImage(QPoint(0, 0), *image);
    *image = newImage;
}

void CappiDisplay::constructImage(const GriddedData* cappi)
{
	
  // Fill the pixmap with data from the cappi
  image.fill(qRgb(255, 255, 255));	
  float iDim = cappi->getIdim();
  float jDim = cappi->getJdim();
  QSize cappiSize((int)iDim,(int)jDim);
  image = image.scaled(cappiSize);
  
  //this->resizeImage(&image, cappiSize);
  
  // Get the minimum and maximum Doppler velocities
  float maxVel = -9999;
  float minVel = 9999;
  float k = 0;
  QString field("ve");
  for (float i = 0; i < iDim; i++) {
    if(exitNow)
      return;
    for (float j = 0; j < jDim; j++) {
      float vel = cappi->getIndexValue(field,i,j,k);
      if (vel != -999) {
	if (vel > maxVel) 
	  maxVel = vel;
	if (vel < minVel)
	  minVel = vel;
      }
    }
  }
  float velRange = maxVel - minVel;
  float velIncr = velRange/41;
  // Set each pixel color scaled to the max and min ranges
  for (float i = 0; i < iDim; i++) {
    if(exitNow)
      return;
    for (float j = 0; j < jDim; j++) {
      float vel = cappi->getIndexValue(field,i,j,k);
      int color = 1;
      if (vel == -999) {
	color = 0;
      } else {
	color = (int)((vel - minVel)/velIncr) + 2;
      }
      int x = (int)i;
      int y = (int)(jDim-j-1);
      image.setPixel(x,y,color);
    }
  }
  image = image.scaled((int)500,(int)500);
  cappiLabel = "Velocities = " +QString().setNum(maxVel) + " to " 
    + QString().setNum(minVel) + " in " + QString().setNum(velIncr) + " m/s incr.";
  QSize textSize(0, (int)(image.height() * 0.05));
  this->setMinimumSize(image.size() + textSize);
  this->resize(image.size() + textSize);
  emit hasImage(true);
  
}

void CappiDisplay::exit()
{
  exitNow = true;
}
