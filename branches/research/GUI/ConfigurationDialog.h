/*
 * ConfigurationDialog.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 8/18/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef CONFIGURATIONDIALOG_H
#define CONFIGURATIONDIALOG_H

#include "panels.h"
#include "Configuration.h"

#include <QtGui>
#include <QtCore>
#include <QDialog>

class QListWidget;
class QListWidgetItem;
class QStackedWidget;

class ConfigurationDialog:public QDialog
{
       Q_OBJECT
public:
       ConfigurationDialog(QWidget* parent = 0, 
			   Configuration *initialConfig = 0);
       ~ConfigurationDialog();
       bool read();
       void checkConfig();

public slots:
       void switchPanel(QListWidgetItem *current, QListWidgetItem *previous); 
       void stateChanged(const QString& name, const bool);
       void graphicsParameter(const QDomElement& element, const QString& name,
			      const QString& parameter);
       void catchLog(const Message& message);
       bool checkPanels();
       void turnOffMembers(const bool& off);
         
private:
       QListWidget *selection;
       QStackedWidget *panels;
       VortexPanel* vortex;
       RadarPanel* radar;
       CappiPanel* cappi;
       CenterPanel* center;
       ChooseCenterPanel* chooseCenter;
       VTDPanel* vtd;
       HVVPPanel* hvvp;
       PressurePanel* pressure;
       GraphicsPanel* graphics;
       QCPanel* qc;
       ResearchPanel* research;
       QHash <QString, AbstractPanel*> panelForString;
       QDir* workingDirectory;

       void populateSelection();
       void populatePanels();
       void connectPanel(AbstractPanel* currPanel);
       void makePanelForString();
       bool readConfig();
       void makeSelectionItem(QListWidgetItem * item);
       Configuration *configData;
       QHash<QDomElement, QWidget*> domElementforWidget;

private slots:
       void applyChanges();
       void setPanelDirectories(); 
 
signals:
       void log(const Message& message); 
       void updatePanels();
       void configChanged();
       void stateChange(const QString& name, const bool);
       void changeGraphicsParameter(const QString& name, const float);
       void changeGraphicsParameter(const QString& name, const QString& time);

};

#endif
