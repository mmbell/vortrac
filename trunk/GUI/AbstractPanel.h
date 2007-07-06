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
#include <QDir>
#include "Message.h"
#include "Configuration.h"

class AbstractPanel:public QWidget
{
  Q_OBJECT
    /*
     * AbstractPanel is a model for creating many panels used in the
     * ConfigurationDialog.
     */
 public:
   AbstractPanel(QWidget *parent = 0);
   ~AbstractPanel();
 
   virtual void updatePanel(QDomElement panelElement) = 0;
     // Reads in values from the Configuration and writes
     // values from the corresponding section to the panel.
 
   virtual bool  updateConfig() = 0;
     // Writes current values in the panel to the corresponding section of the 
     // Configuration

   QLineEdit *dir;
   QPushButton *browse;
   QDir *defaultDirectory;
   virtual QDir* getDefaultDirectory() { return defaultDirectory; }
   QString getCurrentDirectoryPath() { return dir->text(); }
   virtual bool setDefaultDirectory(QDir* newDir);
   // The dir and browse memebers create a generic setup that can be added to
   // any panel for finding and returning an existing folder location.

   QSpinBox *maxWaveNumBox;
   QGroupBox *dataGap;
   QGridLayout *dataGapLayout;
   QList<QDoubleSpinBox*> dataGapBoxes;
   QList<QLabel*> dataGapLabels;
   /*
    * The above list of members are used for the automatic adjustment of
    * visible data gap entry boxes which are used for VTD and Center panels
    */
 
   bool checkPanelChanged(){return panelChanged;}
   void setPanelChanged(bool hasChanged);

   QDomElement getPanelElement() {return elem;}
   void setElement(QDomElement newElement);
   
   QDoubleSpinBox *radarLatBox, *radarLongBox, *radarAltBox;
   QComboBox *radarName;
   Configuration *radars;
   /*
    * These members are used to allow the pull down menu to update other
    * view fields with information on radar latitude, longitude and altitude
    */

   virtual bool checkValues() = 0;
   bool checkDirectory();

 public slots:
   void createDataGaps();
   // Generates the framework nessecary for automatically adjusting data 
   // gap parameter entry boxes
   //void createDataGaps(const QString& value);
   void catchLog(const Message& message);
   virtual void turnOffMembers(const bool& isRunning);

 protected:
   void connectBrowse();
     // sets the browse button to allow the user to select a directory
   void connectFileBrowse();
     // sets the browse button to allow the user to select a file
   QString getFromElement(const QString& childElemName);
     // Interacts with the panel element in a way that produces errors
     // if the correct child is not found. Use this rather than
     // firstChildElement for safety.
   QList<QWidget*> turnOffWhenRunning;

 private:
   bool panelChanged;
     // The panelChanged variable holds information on whether the state of
     // of the has been altered since last comparison the the Configuration.
   QDomElement elem;
     // Holds a reference to the QDomElement of the Configuration contains 
     // the parameter values that fit the pages

 private slots:
   void valueChanged();
 //void valueChanged(const bool signal);
 //void valueChanged(const QString& text);
 //void valueChanged(const QDateTime& dateTime);
     // These slots receive signals when one of the panel's parameters has
     // been changed.
   
   void getDirectory();
     // Receives the signal emited by the browse button
     // Call's the getExisting Directory Dialog
   void getFileName();
     // Receives the signal emited by the browseFiles button
     // Call's the getOpenFileName QDialog static member
   
   // Slots specific to the radar panel implementation

   void checkForAnalytic(const QString& format);
     // checks to see if analytic model is the selected format
   void radarChanged(const QString& text);
     // allows for automatic updating of stored radar specs
   void createDirectory();
     // creates a directory in the filesystem based on the current directory
     // held in member dir

   
 

 signals:
   void log(const Message& message);
   void changeDom(const QDomElement &element, const QString &name, 
		  const QString &value);

   void changeDom(const QDomElement &element, const QString &name,
		  const QString &value, const QString &aname, 
		  const QString &avalue);
     // Sends information needed to update the Configuration

   void addDom(const QDomElement &element, const QString &name, 
	       const QString &value);
   void addDom(const QDomElement &element, const QString &name,
	       const QString &value, const QString& aname,
	       const QString &avalue);
     // Sends information needed to create a new dom element

   void removeDom(const QDomElement &element, const QString &name);
   void removeDom(const QDomElement &element, const QString &name, 
		  const QString &aname, const QString &avalue);
     // Sends information needed to remove a dom element that is 
     //  no longer needed
 
   // Signals specific to the graphics panel implementation
   void stateChange(const QString &name, const bool change);
     // Updates the graph if changes have been made to the
     //   viewable limits of the graph

   void workingDirectoryChanged();
     // emitted whenever the working directory is changed 
};

#endif
