/*
 * TabPage.cpp
 * VORTRAC
 *
 * Created by Michael Bell on 7/21/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include <QtGui>
#include "TabPage.h"

TabPage::TabPage(QWidget *parent)
  : QWidget(parent)
{

  // Create a new configuration instance
  configData = new Configuration;

  // Define all the widgets
  configTree = new ConfigTree(window(), configData);
  // Connect the change signal to update the vortex label
  connect(configTree, SIGNAL(configChanged()), SLOT(updatePage()));

  QLabel *pressureLabel = new QLabel(tr("Current Pressure (mb):"));
  QLCDNumber *currPressure = new QLCDNumber;
  QLabel *rmwLabel = new QLabel(tr("Current RMW (km):"));
  QLCDNumber *currRMW = new QLCDNumber;
  QLabel *pressureGraph = new QLabel;
  pressureGraph->setFrameStyle(QFrame::Panel);
  pressureGraph->setLineWidth(2);

  QTextEdit *statusText = new QTextEdit;
  statusText->setReadOnly(true);
  QScrollArea *scrollLog = new QScrollArea;
  scrollLog->setWidget(statusText);

  QLabel *hLine = new QLabel;
  hLine->setFrameStyle(QFrame::HLine);
  hLine->setLineWidth(2);

  QPushButton *runButton = new QPushButton("&Run");
  QPushButton *abortButton = new QPushButton("&Abort");
  QProgressBar *progressBar = new QProgressBar;

  // Lay them out
  QHBoxLayout *lcdLayout = new QHBoxLayout;
  lcdLayout->addWidget(pressureLabel);
  lcdLayout->addWidget(currPressure);
  lcdLayout->addWidget(rmwLabel);
  lcdLayout->addWidget(currRMW);

  QHBoxLayout *runLayout = new QHBoxLayout;
  runLayout->addWidget(progressBar);
  runLayout->addWidget(runButton);
  runLayout->addWidget(abortButton);

  QVBoxLayout *displayPanel = new QVBoxLayout;
  displayPanel->addLayout(lcdLayout);
  displayPanel->addWidget(pressureGraph);
  displayPanel->addWidget(hLine);
  displayPanel->addWidget(scrollLog);
  displayPanel->addLayout(runLayout);

  QHBoxLayout *display = new QHBoxLayout;
  display->addWidget(configTree);
  display->addLayout(displayPanel);

  setLayout(display);

  // Connect a log to the text window
  statusLog = new Log();
  connect(statusLog, SIGNAL(newLogEntry(QString *)), statusText, SLOT(insertPlainText(QString *)));

  // Connect the run and abort buttons
  connect(runButton, SIGNAL(clicked()), this, SLOT(runThread()));
  connect(abortButton, SIGNAL(clicked()), this, SLOT(abortThread());

}

void TabPage::newFile()
{

  // Load the default configuration
  if (!loadFile("vortrac_default.xml"))
    Error::report("Couldn't load default configuration");
  
  // Set the current filename to untitled
  isUntitled = true;
  configFileName = "vortrac_newconfig.xml";

}

bool TabPage::loadFile(const QString &fileName)
{

  // Load up a configuration file via the configTree widget
  if (!configTree->read(fileName)) {
    Error::report("Couldn't load configuration file");
    return false;
  }

  // Set the filename & vortex Label
  configFileName = fileName;
  setVortexLabel();

  return true;
}

bool TabPage::save()
{

  // Save the current configuration
  if (isUntitled) {
    return saveAs();
  } else {
    return saveFile(configFileName);
  }

}

bool TabPage::saveAs()
{
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                        configFileName);
  if (fileName.isEmpty())
    return false;

  return saveFile(fileName);

}

bool TabPage::saveFile(const QString &fileName)
{

  if (!configTree->write(fileName)) {
    Error::report("Couldn't save configuration file"); 
    return false;
  } 

  return true;

}

void TabPage::setVortexLabel()
{

  QString vortexName = configData->getParam(configData->getConfig("vortex"),
	     "name");
  QString radarName = configData->getParam(configData->getConfig("radar"),
					    "name");
  QString underscore = "_";
  vortexLabel = vortexName + underscore + radarName;

}

void TabPage::closeEvent(QCloseEvent *event)
{
  if (maybeSave()) {
    event->accept();
  } else {
    event->ignore();
  }
}

bool TabPage::maybeSave()
{
  if (configData->checkModified()) {
    int ret = QMessageBox::warning(this, tr("VORTRAC"),
				   tr("'%1' has been modified.\n"
				      "Do you want to save your changes?")
				   .arg((configFileName)),
				   QMessageBox::Yes | QMessageBox::Default,
				   QMessageBox::No,
				   QMessageBox::Cancel | QMessageBox::Escape);
    if (ret == QMessageBox::Yes)
      return save();
    else if (ret == QMessageBox::Cancel)
      return false;
  }
  return true;
}

void TabPage::updatePage()
{

  // Can add more here as need be

  setVortexLabel();
  emit(labelUpdate(vortexLabel));

}

void TabPage::runThread()
{

  // Start a processing thread using the current configuration

  analysisThread = new AnalysisThread(configData);
  analysisThread->start();

}

void TabPage::abortThread()
{

  // Try to kill the thread
  
  analysisThread->quit();

}
