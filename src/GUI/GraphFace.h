/*
 * GraphFace.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 8/9/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef GRAPHFACE_H
#define GRAPHFACE_H

#include<QWidget>
#include<QList>
#include<QEvent>
#include<QImage>
#include<QDir>
#include<QPainter>

#include "DataObjects/VortexList.h"
#include "DataObjects/VortexData.h"
#include "KeyPicture.h"
#include "Message.h"

class GraphFace:public QWidget
{
    Q_OBJECT

public:
    GraphFace(QWidget *parent = 0, const QString& title = QString());
    // The constructor takes a parent and returns a GraphFace object
    // it also preforms several initialization for the object
    ~GraphFace();
    void setImageFileName(QString newName);
    QString getImageFileName() { return autoImageName; }
    QDir getWorkingDirectory() { return workingDirectory;}

    //void setWorkingDirectory(QDir &newDir);



public slots:
    void setWorkingDirectory(QDir &newDir);
    void newInfo(VortexList *VortexPointer);
    void makeKey();
    void newDropSonde(VortexList *dropPointer);
    // This slot recieves a pointer to the dropSonde list when
    // changes to the list occur
    // this modifies nessecary ranges
    void updateTitle(QWidget* labelWidget, const QString& new_Label);
    void saveImage();
    void saveImage(QString fileName);
    void manualAxes(const QString& name, const bool change);
    void manualParameter(const QString& name, const float num);
    void manualParameter(const QString& name, const QString& time);
    void catchLog(const Message& message);

protected:
    void paintEvent (QPaintEvent *event);
    // This function does all the painting on the widget
    // paintEvent is an overwritten function inherited from QWidget
    // paintEvent is called when the widget is first created,
    // and any time update is called
    bool event(QEvent *event);
    void resizeEvent(QResizeEvent * /* event */);

    // all these members are related to the color and size of marks on the graph
    // they are protected so they can easily be passed to the key Widget

    QPen pressurePen;
    QBrush pressureBrush;
    QPen pstd1;
    QPen pstd2;
    QPen rmwPen;
    QBrush rmwBrush;
    QPen rstd1;
    QPen rstd2;
    QPen dropPen;
    QBrush dropBrush;
    QRectF square;
    QRectF drop;

private:
    QImage *image;
    QString autoImageName;
    QDir workingDirectory;
    QFile *imageFile;

    VortexList* VortexDataList;
    VortexList*  dropList;
    QDateTime first;                // Time of first data points
    QDateTime last;
    QDialog* key;
    QString graphTitle;
    int graph_width;
    int graph_height;
    float z1, z2;
    bool imageAltered;
    bool autoAxes;
    bool showPressure;

    // Max and Min radius of maximum wind values
    // These are recorded as the max(min) +(-) the standard deviation in rmw
    float rmwMax;
    float autoRmwMax;
    float rmwMin;
    float autoRmwMin;

    // Max and Min center of vortex pressure values
    // These are recorded as the max(min) +(-) the standard deviation in pressure
    float pressureMax;
    float autoPressureMax;
    float pressureMin;
    float autoPressureMin;

    // Max and Min center of vortex pressure deficit values

    float deficitMax;
    float autoDeficitMax;
    float deficitMin;
    float autoDeficitMin;

    // These are Max and Min graph values for pressure(p) and rmw(r)
    // The difference between these and the above max min is only a small
    // offset to insure that none of the points collide with the edge
    // of the graph
    float pGMax;
    float autoPGMax;
    float pGMin;
    float autoPGMin;
    float rGMax;
    float autoRGMax;
    float rGMin;
    float autoRGMin;
    float dGMax;
    float autoDGMax;
    float dGMin;
    float autoDGMin;

    // These ranges are the span of values for each variable determined by data
    // rmw and pressure go between _GMax and _GMin
    // time range has a 1 minute offset at the left and right
    float rmwRange;
    float pressureRange;
    float deficitRange;
    float timeRange;

    // These functions use information within the list of data points
    // to create a point that is scaled to the current ranges that the graph covers
    // when this point is returned it is ready to graph
    QPointF makePressurePoint(VortexData d);
    QPointF makeDeficitPoint(VortexData d);
    QPointF makeRmwPoint(VortexData d);
    QPointF makeRmwPoint(VortexData d, int bestLevel);
    QPointF makeRmwPoint(VortexData d, float rmw);

    // These functions are used to scale each of the variable to their relative position in
    // the current variable ranges on the graph
    float scaleTime(QDateTime unscaled_time);
    QDateTime unScaleTime(float x);
    float scalePressure(float unscaled_pressure);
    float unScalePressure(float y);
    float scaleRmw(float unscaled_rmw);
    float unScaleRmw(float y);
    float scaleDeficit(float unscaled_deficit);
    float unScaleDeficit(float y);
    float scaleDPressure(float unscaled_dPressure);
    float scaleDRmw(float unscaled_dRmw);
    float scaleDDeficit(float unscaled_dDeficit);
    float getSTDMultiplier(VortexData p, float z);
    int pointAt(const QPointF & position, bool& ONDropSonde);
    void setColors();
    bool autoSave();

    // this function checks to see if the ranges need to be update
    // it will also update ranges when necessary
    void checkRanges();
    void checkPressure(VortexData* point);
    void checkRmw(VortexData* point);
    void checkDeficit(VortexData* point);

    // Constants related to the absolute size of the margins and face of the graph
    // These are in Qt sizes not scaled sizes
    static const int GRAPH_WIDTH = 600;
    static const int GRAPH_HEIGHT = 300;
    static const int LEFT_MARGIN_WIDTH = 40;
    static const int RIGHT_MARGIN_WIDTH = 40;
    static const int BOTTOM_MARGIN_HEIGHT = 40;
    static const int TOP_MARGIN_HEIGHT =25;

    // These constants define the percentiles of the error bars
    static constexpr float Z1 = .67;
    static constexpr float Z2 = .95;

    QPainter* updateImage(QPainter* painter);
    void altUpdateImage();

private slots:

signals:
    void log(const Message& message);

};

#endif


