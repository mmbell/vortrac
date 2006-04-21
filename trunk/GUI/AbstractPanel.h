/*
 * AbstractPanel.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 8/30/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef ABSTRACTPANEL_H
#define ABSTRACTPANEL_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QDomElement>
#include <QSpinBox>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QList>
#include <QGridLayout>
#include <QLabel>
#include <QDateTime>
#include <QComboBox>
#include "Message.h"
#include "Configuration.h"

class AbstractPanel:public QWidget
{
  Q_OBJECT
    // AbstractPanel is a model for creating many panels used in the
    // ConfigurationDialog
 public:
   AbstractPanel(QWidget *parent = 0);
   virtual void updatePanel(QDomElement panelElement);
     // Reads in values from the Configuration and writes
     // values from the corresponding section to the panel.
   virtual bool  updateConfig();
     // Writes current values in the panel to the corresponding section of the 
     // Configuration

   QLineEdit *dir;
   QPushButton *browse;
   QSpinBox *maxWaveNumBox;
   QGroupBox *dataGap;
   QGridLayout *dataGapLayout;
   QList<QDoubleSpinBox*> dataGapBoxes;
   QList<QLabel*> dataGapLabels;
     // The dir and browse memebers create a generic setup that can be added to
     // any panel for finding and returning an existing folder location.
   bool checkPanelChanged(){return panelChanged;}
   void setPanelChanged(bool hasChanged);
   QDomElement getPanelElement() {return elem;}
   void setElement(QDomElement newElement);
   void createDataGaps();
   
   /*
    *  Used to provide automatic radar latitude and longitude display
    */
   QDoubleSpinBox *radarLatBox, *radarLongBox, *radarAltBox;
   QComboBox *radarName;
   Configuration *radars;

 public slots:
   void createDataGaps(const QString& value);
   void catchLog(const Message& message);

 protected:
   void connectBrowse();
   void connectFileBrowse();

 private:
   bool panelChanged;
     // The panelChanged variable holds information on whether the state of
     // of the has been altered since last comparison the the Configuration.
   QDomElement elem;
     // Holds a reference to the QDomElement of the Configuration contains 
     // the parameter values that fit the pages

 private slots:
   void valueChanged();
   void valueChanged(const bool signal);
   void valueChanged(const QString& text);
   void valueChanged(const QDateTime& dateTime);
     // These slots receive signals when one of the panel's parameters has
     // been changed.
   void getDirectory();
     // Receives the signal emited by the browse button
     // Call's the getExisting Directory Dialog
   void getFileName();
     // Receives the signal emited by the browseFiles button
     // Call's the getOpenFileName QDialog static member
   void checkForAnalytic(const QString& format);
   void radarChanged(const QString& text);
 

 signals:
   void log(const Message& message);
   void changeDom(const QDomElement &element, const QString &name, 
		  const QString &value);
     // Sends information needed to update the Configuration
   void addDom(const QDomElement &element, const QString &name, 
	       const QString &value);
   // Sends information needed to create a new dom element
   void removeDom(const QDomElement &element, const QString &name);
   // Sends information needed to remove a dom element that is no longer needed
   void stateChange(const QString &name, const bool change);

};

#endif
