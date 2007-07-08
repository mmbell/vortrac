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

/*!
 * AbstractPanel is a model for creating many panels used in the
 * ConfigurationDialog. There are a number of functions in this class that
 * that are only applicable to a few of the subsequent panels created for 
 * ConfigurationDialog.
 *
 */

class AbstractPanel:public QWidget
{
  Q_OBJECT

 public:
  /*!
   * AbstractPanel is a virtual parent class for creating other panels used in
   * configuring the XML configuration information. This class was not designed
   * to be called directly.
   * \param parent The parent QWidget for the new AbstractPanel
   * \return an empty AbstractPanel
   *
   */
   AbstractPanel(QWidget *parent = 0);
  
   ~AbstractPanel();
   
   /*!
    * This function is used to upload values to the panel from the 
    * corresponding XML configuration node. This function is purely virtual.
    * \param panelElement The QDomElement that corresponds with this panel
    * 
    */
   virtual void updatePanel(QDomElement panelElement) = 0;
     // Reads in values from the Configuration and writes
     // values from the corresponding section to the panel.

   /*!
    * This function is used to update the Configuration Object and XML file 
    * when any changes have been made by the user to the panel within the 
    * ConfigurationDialog. This function is purely virtual and is reimplemented
    * in inherited classes to update the Configuration containing the XML node 
    * assigned to this panel.
    * \return False, if there were any problems with the values that the user tried to enter.
    * \return True, otherwise.
    */ 
   virtual bool updateConfig() = 0;
     // Writes current values in the panel to the corresponding section of the 
     // Configuration

   /*! Holds the working directory for the panel. 
    *  This directory is used for reading input or writing 
    *  output depending on the specifics of this panel. 
    *  \sa getCurrentDirectoryPath()
    */
   QLineEdit *dir; 
   /*! This button appears in a panel when dir is visible. It allows the user 
    *  to search for a file or folder depending on the needs of the panel.
    *  \sa connectBrowse() connectFileBrowse()
    */
   QPushButton *browse;
   /*! This directory holds the default directory for the panel. This directory
    *  is part of the default directory hierarchy for a new VORTRAC run.
    */
   QDir *defaultDirectory;
   /*!
    * \return The defaultDirectory member
    * \sa defaultDirectory
    */
   virtual QDir* getDefaultDirectory() { return defaultDirectory; }
   /*!
    * \return The current location QString specified in the dir member 
    *  of the panel.
    * \sa dir
    */
   QString getCurrentDirectoryPath() { return dir->text(); }
   /*!
    * Allows the panel to redirect the default directory to another location.
    * This member is only reimplemented in panels which follow the default
    * directory layout.
    * \return True, if the directory was successfully found or created.
    * \return False, if the directory could not be accessed.
    * \sa
    */
   virtual bool setDefaultDirectory(QDir* newDir);

   /*!
    * Holds the maximum wave number used in GBVTD fitting
    */
   QSpinBox *maxWaveNumBox;
   /*!
    * Contains all members that dynamically appear to control data gap limits 
    * for different wave numbers used in GBVTD fitting.
    */
   QGroupBox *dataGap;
   QGridLayout *dataGapLayout;
   /*
    * Contains all the widgets which adjust data gaps in relent panels 
    * for GBVTD fitting.
    */
   QList<QDoubleSpinBox*> dataGapBoxes;
   QList<QLabel*> dataGapLabels;

   /*
    * The above list of members are used for the automatic adjustment of
    * visible data gap entry boxes which are used for VTD and Center panels
    */
   
   /*!
    * Determines weither any of the panel members have been changed.
    * \return True, if QWidget panel members have been altered by the user.
    * \return False, otherwise.
    */
   bool checkPanelChanged(){return panelChanged;}
   void setPanelChanged(bool hasChanged);
   /*!
    * Allows the user to access the relevent node in the xml file via the 
    * configuration class.
    * \return the QDomElement assigned to the panel.
    *
    */
   QDomElement getPanelElement() {return elem;}
   /*!
    * 
    * \param newElement an XML node containing entries that correspond with
    * the panels member QWidgets.
    * 
    */
   void setElement(QDomElement newElement);
   
   QDoubleSpinBox *radarLatBox, *radarLongBox, *radarAltBox;
   QComboBox *radarName;
   Configuration *radars;
   /*
    * These members are used to allow the pull down menu to update other
    * view fields with information on radar latitude, longitude and altitude
    */
   /*!
    *
    * \param
    * \return
    * \sa
    */
   virtual bool checkValues() = 0;
   /*!
    *
    * \param
    * \return
    * \sa
    */
   bool checkDirectory();

 public slots:
   /*!
    *
    * \param
    * \return
    * \sa
    */
   void createDataGaps();
   // Generates the framework nessecary for automatically adjusting data 
   // gap parameter entry boxes
   /*!
    *
    * \param
    * \return
    * \sa
    */
   void catchLog(const Message& message);
   /*!
    *
    * \param
    * \return
    * \sa
    */
   virtual void turnOffMembers(const bool& isRunning);

 protected:
   /*!
    *
    * \param
    * \return
    * \sa
    */
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
   /*!
    *
    * \param
    * \return
    * \sa
    */
   void valueChanged();
 // These slots receive signals when one of the panel's parameters has
 // been changed.
 /*!
    *
    * \param
    * \return
    * \sa
    */
   void getDirectory();
     // Receives the signal emited by the browse button
     // Call's the getExisting Directory Dialog
   /*!
    *
    * \param
    * \return
    * \sa
    */
   void getFileName();
     // Receives the signal emited by the browseFiles button
     // Call's the getOpenFileName QDialog static member
   
   // Slots specific to the radar panel implementation
   /*!
    *
    * \param
    * \return
    * \sa
    */
   void checkForAnalytic(const QString& format);
     // checks to see if analytic model is the selected format
   /*!
    *
    * \param
    * \return
    * \sa
    */
   void radarChanged(const QString& text);
     // allows for automatic updating of stored radar specs
   /*!
    *
    * \param
    * \return
    * \sa
    */
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
