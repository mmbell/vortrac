/*
 * AnalysisPage.cpp
 * VORTRAC
 *
 * Created by Michael Bell on 7/21/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include <QtGui>
#include "AnalysisPage.h"
#include "Message.h"

AnalysisPage::AnalysisPage(QWidget *parent)
  : QWidget(parent)
{

  QFont f("Helvetica", 12, QFont::Bold);
  setFont(f);
  lastMax = -999;
  statusLog = new Log();
  
  connect(this, SIGNAL(log(const Message&)),
	  statusLog, SLOT(catchLog(const Message&)), Qt::DirectConnection);

  // Create a new configuration instance
  configData = new Configuration;
  connect(configData, SIGNAL(log(const Message&)), 
	  this, SLOT(catchLog(const Message&)));

  // Define all the widgets

  configDialog = new ConfigurationDialog(configData);
  connect(configDialog, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));
  
  diagPanel = new DiagnosticPanel;
  diagPanel->setFixedWidth(250);
  connect(diagPanel, SIGNAL(log(const Message&)),
  	  this, SLOT(catchLog(const Message&)));
  connect(&pollThread, SIGNAL(newVCP(const int)),
	  diagPanel, SLOT(updateVCP(const int)), Qt::DirectConnection);

  GraphFace *graph = new GraphFace(vortexLabel);
  connect(graph, SIGNAL(log(const Message&)), 
	  this, SLOT(catchLog(const Message&)));

  imageFileName = graph->getImageFileName();


  // Connect signals and slots that update the configuration
  
  connect(configData, SIGNAL(configChanged()), this, SLOT(updatePage()));

  connect(this, SIGNAL(tabLabelChanged(const QString&)), 
  	  graph, SLOT(updateTitle(const QString&)));


  // Construct Analysis Page Layout
 
  QLabel *pressureLabel = new QLabel(tr("Current Pressure (mb):"));
  currPressure = new QLCDNumber;
  //currPressure->display(917);
  currPressure->setSegmentStyle(QLCDNumber::Flat);
  currPressure->resize(100,100);
  QLabel *rmwLabel = new QLabel(tr("Current RMW (km):"));
 currRMW = new QLCDNumber;
  //currRMW->display(16);
  currRMW->setSegmentStyle(QLCDNumber::Flat);
  currRMW->resize(100,100);
  
  QGroupBox *pressureGraph = new QGroupBox;
  
  QPushButton *legend = new QPushButton("See Legend");
  QPushButton *saveGraph = new QPushButton("Save Graph");
  
  //--For Demo
  QHBoxLayout *subLayout = new QHBoxLayout;

  QPushButton *example = new QPushButton("Plot Example Points");

  TestGraph *tester = new TestGraph;

  connect(example, SIGNAL(clicked()), 
	  tester, SLOT(listPlot()));
  connect(tester, SIGNAL(listChanged(QList<VortexData>*)), 
	  graph, SLOT(newInfo(QList<VortexData>*)));
  connect(tester, SIGNAL(dropListChanged(QList<VortexData>*)), 
	  graph, SLOT(newDropSonde(QList<VortexData>*)));
  connect(legend, SIGNAL(clicked()), graph, SLOT(makeKey()));

  subLayout->addWidget(legend);
  subLayout->addWidget(example);
  subLayout->addWidget(saveGraph);
  //--For Demo

  connect(saveGraph, SIGNAL(clicked()),
	  graph, SLOT(saveImage()));
  connect(configDialog, SIGNAL(stateChange(const QString&,const bool)),
	  graph, SLOT(manualAxes(const QString&, const bool)));
  connect(configDialog, 
	  SIGNAL(changeGraphicsParameter(const QString&, const float)),
	  graph,SLOT(manualParameter(const QString&,const float)));

  QVBoxLayout *layout = new QVBoxLayout(pressureGraph);
  layout->addWidget(graph, 100);
  layout->addLayout(subLayout);
  pressureGraph->setLayout(layout);
  
  statusText = new QTextEdit;
  statusText->setReadOnly(true);
  statusText->setFixedHeight(100);

  QLabel *hLine = new QLabel;
  hLine->setFrameStyle(QFrame::HLine);
  hLine->setLineWidth(2);

  QPushButton *runButton = new QPushButton("&Run");
  QPushButton *abortButton = new QPushButton("&Abort");
  QProgressBar *progressBar = new QProgressBar;
  progressBar->setMinimum(0);
  progressBar->setMaximum(100);
  progressBar->reset();

  // Lay them out
  QHBoxLayout *lcdLayout = new QHBoxLayout;
  lcdLayout->addStretch();
  lcdLayout->addWidget(pressureLabel);
  lcdLayout->addWidget(currPressure);
  lcdLayout->addStretch();
  lcdLayout->addWidget(rmwLabel);
  lcdLayout->addWidget(currRMW);
  lcdLayout->addStretch();

  QHBoxLayout *runLayout = new QHBoxLayout;
  runLayout->addWidget(progressBar);
  runLayout->addWidget(runButton);
  runLayout->addWidget(abortButton);

  QGridLayout *displayPanel = new QGridLayout;
  displayPanel->addLayout(lcdLayout,0,0);
  displayPanel->addWidget(pressureGraph,1,0);
  displayPanel->addWidget(hLine,2,0);
  displayPanel->addWidget(statusText,3,0);
  displayPanel->addLayout(runLayout,4,0);
  displayPanel->setRowMinimumHeight(0,50);

  QHBoxLayout *display = new QHBoxLayout;
  display->addWidget(diagPanel);
  display->addLayout(displayPanel);

  setLayout(display);

  // Connect the run and abort buttons
  connect(runButton, SIGNAL(clicked()), this, SLOT(runThread()));
  connect(abortButton, SIGNAL(clicked()), this, SLOT(abortThread()));

  // Connect a log to the text window and progress bar
  connect(statusLog, SIGNAL(newLogEntry(QString)), 
	  statusText, SLOT(insertPlainText(QString)));

  connect(statusLog, SIGNAL(newProgressEntry(int)),
	  progressBar, SLOT(setValue(int)));

  connect(statusText, SIGNAL(textChanged()),
	  this, SLOT(autoScroll()));
  
  statusLog->catchLog(Message("VORTRAC Status Log for "+
			    QDateTime::currentDateTime().toUTC().toString()
			    + " UTC"));
  
  connect(&pollThread, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);

  //connect(this, SIGNAL(saveGraphImage(const QString&)),
  //   graph, SLOT(saveImage(const QString&)));
  
}

AnalysisPage::~AnalysisPage()
{
  prepareToClose();
}

void AnalysisPage::newFile()
{

  // Load the default configuration
  if (!loadFile(QDir::current().filePath("vortrac_default.xml")))
    emit log(Message("Couldn't load default configuration"));
  
  // Set the current filename to untitled
  isUntitled = true;
  configFileName = QDir::current().filePath("vortrac_newconfig.xml");

}

bool AnalysisPage::loadFile(const QString &fileName)
{
  // Load up a configuration file in the ConfigurationDialog
  if (!configData->read(fileName)) {
    emit log(Message("Couldn't load configuration file"));
    return false;
  }
 
  configDialog->read();

  // Set the filename
  configFileName = fileName;
  QString directoryString(configData->getParam(configData->getConfig("vortex"),
					       "dir"));
  workingDirectory = QDir(directoryString);
  //Message::toScreen(workingDirectory.path());
  if(!workingDirectory.isAbsolute()) {
    workingDirectory.makeAbsolute();
  }
  // Check to make sure the workingDirectory exists
  if(!workingDirectory.exists())
    if(!workingDirectory.mkpath(directoryString)) {
      emit log(Message("failed to find or create working directory path: "+directoryString));
      return false;
    }
  //Message::toScreen("2"+workingDirectory.path());
  statusLog->setWorkingDirectory(workingDirectory);
  //graph->setWorkingDirectory(workingDirectory);
  return true;
}

bool AnalysisPage::save()
{

  // Save the current configuration
  if (isUntitled) {
    return saveAs();
  } else {
    return saveFile(configFileName);
  }

}

bool AnalysisPage::saveAs()
{
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                        configFileName);
  if (fileName.isEmpty())
    return false;


  return saveFile(fileName);
  
}

bool AnalysisPage::saveFile(const QString &fileName)
{
  
  if (!configData->write(fileName)) {
    emit log(Message("Couldn't save configuration file")); 
    return false;
  } 
  
  QString noExtensionFileName = fileName;
  
  if (noExtensionFileName.contains('.')) {
    int dotIndex = noExtensionFileName.indexOf('.');
    int drop = noExtensionFileName.count()-dotIndex+1;
    noExtensionFileName.remove(dotIndex,drop);
  }

  statusLog->saveLogFile(noExtensionFileName+".log");
  emit saveGraphImage(noExtensionFileName+".png");



  return true;

}

void AnalysisPage::setVortexLabel()
{

  QString vortexName = configData->getParam(configData->getConfig("vortex"),
	     "name");
  QString radarName = configData->getParam(configData->getConfig("radar"),
					    "name");
  QString underscore = "_";
  vortexLabel = vortexName + underscore + radarName;
  emit tabLabelChanged(vortexLabel);

}

void AnalysisPage::closeEvent(QCloseEvent *event)
{
  //Message::toScreen("closeEvent");
  if (maybeSave()) 
    {
      //Message::toScreen("Do We Get To Close Event?");
      QString autoLogName = statusLog->getLogFileName();
      //Message::toScreen("CloseEvent "+workingDirectory.path());
      QFile autoLog(workingDirectory.filePath(autoLogName));
      autoLog.remove();
      event->accept();
    } 
  else 
    {
      event->ignore();
    }
}

bool AnalysisPage::maybeSave()
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

void AnalysisPage::updatePage()
{
  // Can add more here as need be
  setVortexLabel();
  QString temp = getVortexLabel();
  emit(tabLabelChanged(temp));
  //Message::toScreen("updatepage before "+workingDirectory.path());
  QString newPathString(configData->getParam(configData->getConfig("vortex"), 
					     "dir"));
  QDir newPath(newPathString);
  if(newPath.isRelative())
    newPath.makeAbsolute();
  if(newPath!=workingDirectory) {
    workingDirectory = newPath;
    statusLog->setWorkingDirectory(workingDirectory);
    //graph->setWorkingDirectory(workingDirectory);
      
    }
  //Message::toScreen("updatePage after "+workingDirectory.path());
}

void AnalysisPage::runThread()
{
  // Start a processing thread using the current configuration
  
  if(configData->getParam(configData->getConfig("radar"), "format")
     == QString("MODEL")){
    if(analyticModel()) {
      pollThread.setConfig(configData);
      pollThread.start();
    }
  }

  
  else {
    pollThread.setConfig(configData);
    pollThread.start();
  }

}

void AnalysisPage::abortThread()
{

  // Try to kill the thread
  if(pollThread.isRunning()) {
	pollThread.quit();
	emit log(Message("Analysis Aborted!"));
  }

  

}


bool AnalysisPage::isRunning()
{
  if (pollThread.isRunning())
    return true;
  return false;
}

void AnalysisPage::saveDisplay()
{
  graph->saveImage();
}

void AnalysisPage::saveLog()
{
  statusLog->saveLogFile();
}

void AnalysisPage::catchLog(const Message& message)
{
  emit log(message);
}

void AnalysisPage::autoScroll()
{
  int pos = statusText->verticalScrollBar()->value();
  if(pos == lastMax) {
    lastMax = statusText->verticalScrollBar()->maximum();
    statusText->verticalScrollBar()->setValue(lastMax);
  }
  else
    lastMax = statusText->verticalScrollBar()->maximum();
}

void AnalysisPage::prepareToClose()
{
  //Message::toScreen("prepare to Close "+workingDirectory.path());
  QString autoLogName = statusLog->getLogFileName();
  QFile autoLog(workingDirectory.filePath(autoLogName));
  autoLog.remove();

  // autoImage is deleted in the GraphFace destructor

}

bool AnalysisPage::analyticModel()
{

  // Creates a rudimentry dialog for editing or confirming the parameters of
  // the model vortex to be created and sampled by the theoretical radar.

  QString modelFile = configData->getParam(configData->getConfig("radar"),
					   "dir");
  Configuration *modelConfig = new Configuration(this, modelFile);
  connect(modelConfig, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));
  QDialog *modelDialog = new QDialog;
  modelDialog->setModal(true);
  QPushButton *run = new QPushButton("Create Analytic Model");
  run->setDefault(true);
  QPushButton *cancel = new QPushButton("Cancel");
  ConfigTree *configTreeModel = new ConfigTree(modelDialog, modelConfig);
  connect(configTreeModel, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));
  
  connect(configTreeModel, SIGNAL(newParam(const QDomElement&, 
					   const QString&, const QString&)), 
	  modelConfig, SLOT(setParam(const QDomElement&, 
				     const QString&, const QString&)));
  connect(configTreeModel, SIGNAL(addDom(const QDomElement&, const QString&, 
					 const QString&)), 
	  modelConfig, SLOT(addDom(const QDomElement&,
				   const QString&, const QString&)));
  connect(configTreeModel, SIGNAL(removeDom(const QDomElement&,
					    const QString&)),
	  modelConfig, SLOT(removeDom(const QDomElement&,const QString&)));
  
  if(!configTreeModel->read())
    emit log(Message("Error Reading Analytic Storm Configuration"));
  
  connect(run, SIGNAL(pressed()), modelDialog, SLOT(accept()));
  connect(cancel, SIGNAL(pressed()), modelDialog, SLOT(reject()));
  
  QHBoxLayout *buttons = new QHBoxLayout;
  buttons->addStretch(1);
  buttons->addWidget(run);
  buttons->addWidget(cancel);
  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(configTreeModel);
  layout->addLayout(buttons);
  modelDialog->setLayout(layout);
  modelDialog->exec();

  if(modelDialog->result()==QDialog::Accepted)
    return true;
  else 
    return false;

}
