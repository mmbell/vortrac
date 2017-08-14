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
#include <math.h>

CappiDisplay::CappiDisplay(QWidget *parent)
    : QWidget(parent)
{
    this->setObjectName("cappiDisplay");
    setAttribute(Qt::WA_StaticContents);
    PaintEngineMode = -5;
    //connect(this, SIGNAL(hasImage(bool)),this, SLOT(setVisible(bool)));
    displayType = velocity;
    
    backColor = qRgb(255,255,255);
    legendImage = QImage(10,50,QImage::Format_RGB32);
    maxVel = 1;
    minVel = 0;
    maxRec = 0;
    maxApp = 0;
    heightMaxApp = heightMaxRec = 0.;
    distMaxApp = distMaxRec = 0;
    dirMaxApp = dirMaxRec = 0;

    displayLevel = -1;  // default level comes from cappi, unless overwritten here.
    contourIncr = 1;
    
    //Set the palette
    imageHolder.lock();
    
    image = QImage(50,50,QImage::Format_Indexed8);
    image.setNumColors(44);
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
    colorMap[2] = qRgb((int)(.469*255), (int)(.020*255), (int)(.640*255));
    colorMap[3] = qRgb((int)(.403*255), (int)(.227*255), (int)(.559*255));
    colorMap[4] = qRgb((int)(.164*255), (int)(.055*255), (int)(.582*255));
    colorMap[5] = qRgb((int)(.227*255), (int)(.055*255), (int)(.672*255));
    colorMap[7] = qRgb((int)(.352*255), (int)(.141*255), (int)(.898*255));
    colorMap[8] = qRgb((int)(.414*255), (int)(.375*255), (int)(.996*255));
    colorMap[9] = qRgb((int)(.445*255), (int)(.559*255), (int)(.996*255));
    colorMap[10] = qRgb((int)(.281*255), (int)(.590*255), (int)(.602*255));
    colorMap[11] = qRgb((int)(.188*255), (int)(.523*255), (int)(.371*255));
    colorMap[12] = qRgb((int)(.004*255), (int)(.445*255), (int)(.000*255));
    colorMap[13] = qRgb((int)(.000*255), (int)(.492*255), (int)(.000*255));
    colorMap[14] = qRgb((int)(.000*255), (int)(.539*255), (int)(.000*255));
    colorMap[15] = qRgb((int)(.059*255), (int)(.586*255), (int)(.059*255));
    colorMap[16] = qRgb((int)(.176*255), (int)(.633*255), (int)(.176*255));
    colorMap[17] = qRgb((int)(.289*255), (int)(.680*255), (int)(.289*255));
    colorMap[18] = qRgb((int)(.402*255), (int)(.723*255), (int)(.402*255));
    colorMap[19] = qRgb((int)(.520*255), (int)(.770*255), (int)(.520*255));
    colorMap[20] = qRgb((int)(.633*255), (int)(.816*255), (int)(.633*255));
    colorMap[21] = qRgb((int)(.750*255), (int)(.863*255), (int)(.750*255));
    colorMap[22] = qRgb((int)(.863*255), (int)(.910*255), (int)(.863*255));
    colorMap[23] = qRgb((int)(.938*255), (int)(.906*255), (int)(.703*255));
    colorMap[24] = qRgb((int)(.938*255), (int)(.859*255), (int)(.352*255));
    colorMap[25] = qRgb((int)(.938*255), (int)(.812*255), (int)(.000*255));
    colorMap[26] = qRgb((int)(.938*255), (int)(.766*255), (int)(.023*255));
    colorMap[27] = qRgb((int)(.938*255), (int)(.719*255), (int)(.055*255));
    colorMap[28] = qRgb((int)(.926*255), (int)(.672*255), (int)(.086*255));
    colorMap[29] = qRgb((int)(.871*255), (int)(.625*255), (int)(.117*255));
    colorMap[30] = qRgb((int)(.816*255), (int)(.578*255), (int)(.148*255));
    colorMap[31] = qRgb((int)(.758*255), (int)(.531*255), (int)(.180*255));
    colorMap[32] = qRgb((int)(.703*255), (int)(.484*255), (int)(.211*255));
    colorMap[33] = qRgb((int)(.648*255), (int)(.438*255), (int)(.242*255));
    colorMap[34] = qRgb((int)(.590*255), (int)(.391*255), (int)(.250*255));
    colorMap[35] = qRgb((int)(.535*255), (int)(.344*255), (int)(.250*255));
    colorMap[36] = qRgb((int)(.485*255), (int)(.328*255), (int)(.297*255));
    colorMap[37] = qRgb((int)(.629*255), (int)(.312*255), (int)(.375*255));
    colorMap[38] = qRgb((int)(.625*255), (int)(.003*255), (int)(.000*255));
    colorMap[39] = qRgb((int)(.718*255), (int)(.086*255), (int)(.188*255));
    colorMap[40] = qRgb((int)(.813*255), (int)(.148*255), (int)(.273*255));
    colorMap[41] = qRgb((int)(.879*255), (int)(.211*255), (int)(.355*255));
    colorMap[42] = qRgb((int)(.949*255), (int)(.273*255), (int)(.355*255));
    colorMap[43] = qRgb((int)(1.000*255), (int)(.012*255), (int)(.000*255));
    */
    
    imageHolder.unlock();

    hasGBVTDInfo = false;
    hasCappi = false;

    this->clearImage();
//    emit hasImage(true);
//    this->repaint();
    emit hasImage(false);
    this->update();
}

bool CappiDisplay::openImage(const QString &fileName)
{
    QImage loadedImage;
    if (!loadedImage.load(fileName))
        return false;

    QSize newSize = loadedImage.size().expandedTo(size());
    resizeImage(&loadedImage, newSize);
    imageHolder.lock();
    image = loadedImage;
    imageHolder.unlock();
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
    emit hasImage(false);
    imageHolder.lock();
    image.fill(qRgb(0,0,0));
    legendImage.fill(qRgb(0,0,0));
    image = image.scaled((int)500,(int)500);
    legendImage = legendImage.scaled(70,500);
    this->setMinimumSize(QSize(int(image.size().width()*1.2),int(image.size().height())));
    this->resize(this->minimumSize());
    imageHolder.unlock();
    //QString message;
    //message.setNum(PaintEngineMode);
    //QTextStream out(stdout);
    //out << message << endl;
    update();
}

// Print a line showing grid coordinates, and lat and lon at that point.
// This can be used for debugging (to copyt/paste a location for example)

void CappiDisplay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        lastPoint = event->pos();

	// Display origin is at the top left. Cappi origin is at the radar.
	
	int x = currentCappi.getCartesianPointFromIndexI((float)lastPoint.x());
	int y = currentCappi.getCartesianPointFromIndexJ(currentCappi.getJdim() - lastPoint.y());

	float *coords = currentCappi.getAdjustedLatLon(currentCappi.getOriginLat(),
						       currentCappi.getOriginLon(),
						       x, y);
	// coords[0] -> Lon
	// coords[1] -> Lat
	
	std::cout << "Left press at " << "(" << lastPoint.x() << ", " << lastPoint.y()
		  << ") -> (" << x << ", " << y << "), "
		  << "(" << coords[0] << ", " << coords[1] << ")"
		  << std::endl;
	delete[] coords;
    }
}

// Show a tooltip with lat and lon at the mouse position
// Since setMouseTracking() hasn't been called, this will only happen when one button is pressed down.

void CappiDisplay::mouseMoveEvent(QMouseEvent *event)
{
  lastPoint = event->pos();

  // Display origin is at the top left. Cappi origin is at the radar.
	
  int x = currentCappi.getCartesianPointFromIndexI((float)lastPoint.x());
  int y = currentCappi.getCartesianPointFromIndexJ(currentCappi.getJdim() - lastPoint.y());

  float *coords = currentCappi.getAdjustedLatLon(currentCappi.getOriginLat(),
						 currentCappi.getOriginLon(),
						 x, y);

  QToolTip::showText(event->globalPos(),
		     QString("(") + QString::number(coords[0]) + ", " + QString::number(coords[1]) + ")" );
  delete[] coords;
}

void CappiDisplay::paintEvent(QPaintEvent * /* event */)
{
    // Draw the legend image .....
    imageHolder.lock();

    QPainter *imagePainter = new QPainter(&legendImage);

    float offset = 15;
    float boxHeight = (image.height()-2*offset)/(float)42;

    for(int i = 0; i < 42; i++) {
        // we don't use colors 0 & 1 because they are black and white
        imagePainter->setBrush(QBrush(image.color(i+2)));
        imagePainter->drawRect(QRect(0, (int) (offset+((41-i)*boxHeight)),
                                     (int)(2*boxHeight),
                                     (int)boxHeight));
    }
    QFont legendFont("Times",7);
    imagePainter->setFont(legendFont);
    for(int i = 0; i < 42; i+=2) {
        float label;
        if (displayType == velocity){
            label = maxVel-i*contourIncr;
        } else {
            label = 50 - i*contourIncr;
        }
        imagePainter->drawText(QPoint((int)(2.5*boxHeight), (int)(offset+(i*boxHeight)+(.7*boxHeight))), QString().setNum(label, 'f', 0));
    }

    if(imagePainter->isActive())
        imagePainter->end();
    delete imagePainter;
    imageHolder.unlock();

    // Paint the image and the legend onto this wigit

    QPainter painter(this);
    backColor = painter.background().color();
    //Message::toScreen("background Color r = "+QString().setNum(backColor.red())+" g = "+QString().setNum(backColor.green())+" b = "+QString().setNum(backColor.blue()));
    imageHolder.lock();
    painter.drawImage(QPoint(image.size().width()*0,0), image);

    painter.save();
    
    if(hasGBVTDInfo) {
        // Given the relevant GBVTD and config parameters
        // Draw an X (small hurricane symbol?) at (x, y) to mark
        // GBVTD center
        // Draw circles of one color for simplex max & simplex min
        // Draw circles of another color for GBVTD max radius
        // All these values should arrive scaled to percentages of cappi size
      
        QPen xPen(Qt::white);
	xPen.setWidth(5);
	painter.setPen(xPen);

	// Draw a small X at the radar
	float zero = 0.0;
    
	int radX = (int) currentCappi.getIndexFromCartesianPointI(zero);
	int radY = (int) currentCappi.getIndexFromCartesianPointJ(zero);
	// Display origin is top left corner. Cappi origin is bottom left. Adjust radY accordingly
	radY = currentCappi.getJdim() - radY;

	painter.drawLine(QPointF(radX - 2, radY - 2), QPointF(radX + 2, radY + 2));
	painter.drawLine(QPointF(radX - 2, radY + 2), QPointF(radX + 2, radY - 2));

        float w = image.size().width();
        float h = image.size().height();
	
        // Draw a small X in the vortex center
        painter.drawLine(QPointF((xPercent-.01)*w,(1-(yPercent-.01))*h),
                         QPointF((xPercent+.01)*w,(1-(yPercent+.01))*h));
        painter.drawLine(QPointF((xPercent-.01)*w,(1-(yPercent+.01))*h),
                         QPointF((xPercent+.01)*w,(1-(yPercent-.01))*h));

        // Annotate the maximum and minimum velocities
        painter.drawLine(QPointF((maxVelXpercent)*w,(1-(maxVelYpercent-.01))*h),QPointF((maxVelXpercent)*w,(1-(maxVelYpercent+.01))*h));
        painter.drawLine(QPointF((maxVelXpercent-.01)*w,(1-(maxVelYpercent))*h),QPointF((maxVelXpercent+.01)*w,(1-(maxVelYpercent))*h));
        painter.drawLine(QPointF((minVelXpercent-.01)*w,(1-(minVelYpercent))*h),
                         QPointF((minVelXpercent+.01)*w,(1-(minVelYpercent))*h));

        // Draw the circles about this center
        QPen simplexPen(Qt::darkGray);
        simplexPen.setWidth(3);
        painter.setPen(simplexPen);
        QRectF sMinRect((xPercent-simplexMin)*w, (1-(yPercent+simplexMin))*h,2*simplexMin*w, 2*simplexMin*h);
        painter.drawEllipse(sMinRect);
        QRectF sMaxRect((xPercent-simplexMax)*w, (1-(yPercent+simplexMax))*h,2*simplexMax*w, 2*simplexMax*h);
        painter.drawEllipse(sMaxRect);

        //Draw radial max wind radius,darkred
        QPen rmwPen(Qt::darkRed);
        rmwPen.setWidth(2);
        painter.setPen(rmwPen);
        QRectF rmwRect((xPercent-radiusMaximumWind)*w, (1-(yPercent+radiusMaximumWind))*h,2*radiusMaximumWind*w, 2*radiusMaximumWind*h);
        painter.drawEllipse(rmwRect);

        //Draw vtd circle
        QPen vtdPen(Qt::white);
        vtdPen.setWidth(3);
        vtdPen.setStyle(Qt::DotLine);
        painter.setPen(vtdPen);
        QRectF vMaxRect((xPercent-vortexMax)*w, (1-(yPercent+vortexMax))*h,2*vortexMax*w, 2*vortexMax*h);
        painter.drawEllipse(vMaxRect);
    }
    painter.restore();
    painter.fillRect(0,image.size().height(),this->size().width(),this->size().height()-image.size().height(), QBrush(painter.background()));
    painter.fillRect(image.size().width(),0,this->size().width()-image.size().width(),this->size().height(), QBrush(painter.background()));

    painter.drawImage(QPointF(image.size().width()*1.1,0),legendImage);

    imageHolder.unlock();
    painter.end();

}

void CappiDisplay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void CappiDisplay::resizeImage(QImage *image, const QSize &newSize)
{
    Message::report("CappiDisplay: ResizeImage: Someone is using this");
    if (image->size() == newSize)
        return;
    
    QImage newImage(newSize, QImage::Format_Indexed8);
    newImage.fill(qRgb(255, 255, 255));
    QPainter painter(&newImage);
    painter.drawImage(QPoint(0, 0), *image);
    imageHolder.lock();
    *image = newImage;
    imageHolder.unlock();
}

void CappiDisplay::constructImage(const GriddedData& cappi)
{
    // Fill the pixmap with data from the cappi
    currentCappi = cappi;
    hasCappi = true;
    imageHolder.lock();
    //hasGBVTDInfo = false;
    image.fill(qRgb(255, 255, 255));
    iDim = (int)cappi.getIdim();
    jDim = (int)cappi.getJdim();
    QSize cappiSize((int)iDim,(int)jDim);
    image = image.scaled(cappiSize);

    // Get the minimum and maximum Doppler velocities
    maxVel = -9999;
    minVel= 9999;
    
    float k = getDisplayLevel();
      
    QString velfield("ve");
    QString dbzfield("dz");
    QString heightfield("ht");
    float minI, maxI, minJ, maxJ;
    if(hasGBVTDInfo) {
        float xIndex = xPercent*iDim;
        float yIndex = yPercent*jDim;
        minI = xIndex-(simplexMax*iDim*cappi.getIGridsp());
        maxI = xIndex+(simplexMax*iDim*cappi.getIGridsp());
        minJ = yIndex-(simplexMax*iDim*cappi.getJGridsp());
        maxJ = yIndex+(simplexMax*iDim*cappi.getJGridsp());
        if (minI < 0) minI = 0;
        if (maxI > iDim) maxI = iDim;
        if (minJ < 0) minJ = 0;
        if (maxJ > jDim) maxJ = jDim;
    } else {
        minI = 0;
        maxI = iDim;
        minJ = 0;
        maxJ = jDim;
    }
    float maxAppXindex = -999.0;
    float maxAppYindex = -999.0;
    float maxRecXindex = -999.0;
    float maxRecYindex = -999.0;
    for (float i = minI; i < maxI; i++) {
        for (float j = minJ; j < maxJ; j++) {
            float vel = cappi.getIndexValue(velfield,i,j,k);
            if (vel != -999) {
				vel *= 1.9438445;
                if (vel > maxVel) {
                    maxVel = vel;
                    maxRecXindex = i;
                    maxRecYindex = j;
                    maxVelXpercent = (i+1)/iDim;
                    maxVelYpercent = (j+1)/jDim;
                }
                if (vel < minVel) {
                    minVel = vel;
                    maxAppXindex = i;
                    maxAppYindex = j;
                    minVelXpercent = (i+1)/iDim;
                    minVelYpercent = (j+1)/jDim;
                }
            }
        }
    }
    float velRange;
    if (maxVel == -9999) {
        // No winds in domain
        maxApp = 0;
        maxRec = 0;
        velRange = 2;
        maxVel = 1;
    } else {
        maxApp = fabs(minVel);
        maxRec = fabs(maxVel);
        if(maxVel>fabs(minVel)) {
            velRange = 2*maxVel;
            minVel = -1*maxVel;
        }
        else {
            velRange = 2*fabs(minVel);
            maxVel = -1*minVel;
        }
        
        if ((maxAppXindex != -999.0) and (maxAppYindex != -999.0)) {
            heightMaxApp = cappi.getIndexValue(heightfield,maxAppXindex,maxAppYindex,k);
            float cartI = cappi.getCartesianPointFromIndexI(maxAppXindex);
            float cartJ = cappi.getCartesianPointFromIndexJ(maxAppYindex);
            distMaxApp = sqrt(cartI*cartI + cartJ*cartJ);
            dirMaxApp = atan2(cartJ,cartI)*57.2957795130823;
            dirMaxApp = 450.0 - dirMaxApp;
            if (fabs(dirMaxApp) < 0.000001) { dirMaxApp=0.; }
            if (dirMaxApp > 360.0) dirMaxApp -= 360.0;
            if (dirMaxApp < 0.) dirMaxApp += 360.0;
        } else {
            heightMaxApp = distMaxApp = dirMaxApp = -999.0;
        }
        if ((maxRecXindex != -999.0) and (maxRecYindex != -999.0)) {
            heightMaxRec = cappi.getIndexValue(heightfield,maxRecXindex,maxRecYindex,k);
            float cartI = cappi.getCartesianPointFromIndexI(maxRecXindex);
            float cartJ = cappi.getCartesianPointFromIndexJ(maxRecYindex);
            distMaxRec = sqrt(cartI*cartI + cartJ*cartJ);
            dirMaxRec = atan2(cartJ,cartI)*57.2957795130823;
            dirMaxRec = 450.0 - dirMaxRec;
            if (fabs(dirMaxRec) < 0.000001) { dirMaxRec=0.; }
            if (dirMaxRec > 360.0) dirMaxRec -= 360.0;
            if (dirMaxRec < 0.) dirMaxRec += 360.0;
        } else {
            heightMaxRec = distMaxRec = dirMaxRec = -999.0;
        }
    }
    //Message::toScreen("maxVel is "+QString().setNum(maxVel)+" minVel is "+QString().setNum(minVel));
    QString field;
    float minValue;
    if (displayType == velocity) {
        contourIncr = velRange/41;
        field = velfield;
        minValue = minVel;
    } else if (displayType == reflectivity) {
        contourIncr = 1.5;
        field = dbzfield;
        minValue = -11.5;
    }
    // Set each pixel color scaled to the max and min ranges
    for (float i = 0; i < iDim; i++) {
        for (float j = 0; j < jDim; j++) {
            float value = cappi.getIndexValue(field,i,j,k);
            int color = 1;
            if (value == -999) {
                color = 0;
            } else {
				if (displayType == velocity) value *= 1.9438445;				
                color = (int)((value - minValue)/contourIncr) + 2;
                if ((color < 0) or (color > 43)) {
                    // Bad color
                    color = 1;
                }
            }
            int x = (int)i;
            int y = (int)(jDim-j-1);
            if (image.valid(x,y)) {
                image.setPixel(x,y,color);
            }
        }
    }
    //    image = image.scaled((int)500,(int)500);
    image = image.scaled((int)iDim,(int)jDim);
    legendImage = legendImage.scaled(70,500);
    legendImage.fill(qRgb(backColor.red(),backColor.green(),backColor.blue()));

    this->setMinimumSize(QSize(int(image.size().width()*1.2),int(image.size().height())));
    this->resize(this->minimumSize());
    imageHolder.unlock();
    update();
}

// If level was overwritten from the GUI, return that.
// Otherwise, ask the cappi what level to use

int CappiDisplay::getDisplayLevel()
{
  if (displayLevel >= 0)
    return displayLevel;
  return currentCappi.getDisplayKIndex();
}

void CappiDisplay::levelChanged(int level)
{
  displayLevel = level;
  if (hasCappi)
    constructImage(currentCappi);
  update();
}

void CappiDisplay::setGBVTDResults(float x, float y,float rmw, float sMin, float sMax, float vMax,
                                   float userlat, float userlon,float lat, float lon)
{
    imageHolder.lock();
    xPercent = x;
    yPercent = y;
    radiusMaximumWind= rmw;
    simplexMin = sMin;
    simplexMax = sMax;
    vortexMax = vMax;
    hasGBVTDInfo = true;
    imageHolder.unlock();
    emit hasImage(true);
    update();
}

void CappiDisplay::toggleRadarDisplay()
{
    if (displayType == velocity) {
        displayType = reflectivity;
    } else {
        displayType = velocity;
    }
    if (hasCappi) constructImage(currentCappi);
    update();
}
