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
    image.fill(qRgb(255, 255, 255));
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
    if (width() > image.width() || height() > image.height()) {
        int newWidth = qMax(width() + 128, image.width());
        int newHeight = qMax(height() + 128, image.height());
        resizeImage(&image, QSize(newWidth, newHeight));
        update();
    }
    QWidget::resizeEvent(event);
}

void CappiDisplay::resizeImage(QImage *image, const QSize &newSize)
{
    if (image->size() == newSize)
        return;

    QImage newImage(newSize, QImage::Format_RGB32);
    newImage.fill(qRgb(255, 255, 255));
    QPainter painter(&newImage);
    painter.drawImage(QPoint(0, 0), *image);
    *image = newImage;
}
