/*
 * RadarListDialog.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 4/18/06
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef RADARLISTDIALOG_H
#define RADARLISTDIALOG_H

#include <QDomDocument>
#include <QString>
#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QRadioButton>
#include <QLineEdit>

#include "Config/Configuration.h"
#include "IO/Message.h"

class RadarListDialog:public QDialog
{

Q_OBJECT

 public:
 RadarListDialog(QWidget *parent = 0, Configuration *initialConfig = 0);
    ~RadarListDialog();
    void readConfig();

 public slots:
    void catchLog(const Message& message);
    
 private:
    QString xmlFileName;
    Configuration *radars;
    QRadioButton *editRadarButton, *newRadarButton;
    QComboBox *editRadar;
    QLineEdit *editRadarName, *newRadarName, *newRadarLocation;
    QDoubleSpinBox *editRadarLat, *editRadarLon, *editRadarAlt;
    QDoubleSpinBox *newRadarLat, *newRadarLon, *newRadarAlt;

 private slots:
    void savePressed();
    void prepareToClose();
    void radarValues(const QString& choice);

 signals:
    void log(const Message& message);
    void changeDom(const QDomElement &element, const QString &name, 
		   const QString &value);
    void addDom(const QDomElement &element, const QString &name, 
		const QString &value);
    void newEntry(const QString &newString);

    
};

#endif
