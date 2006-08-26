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

class CappiDisplay : public QWidget
{
    Q_OBJECT

public:
    CappiDisplay(QWidget *parent = 0);

    bool openImage(const QString &fileName);
    bool saveImage(const QString &fileName, const char *fileFormat);

public slots:
    void clearImage();

protected:
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    void resizeImage(QImage *image, const QSize &newSize);

    QImage image;
    QPoint lastPoint;
    int PaintEngineMode;
};

#endif
