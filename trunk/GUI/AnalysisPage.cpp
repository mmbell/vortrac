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
  this->setObjectName("analysisPage");
  QFont f("Helvetica", 12, QFont::Bold);
  setFont(f);
  lastMax = -999;
  statusLog = new Log();
  
  connect(this, SIGNAL(log(const Message&)),
	  statusLog, SLOT(catchLog(const Message&)), Qt::DirectConnection);

  // Create the widgets needed to interact with the log
  
  diagPanel = new DiagnosticPanel;
  diagPanel->setFixedWidth(250);
  connect(diagPanel, SIGNAL(log(const Message&)),
  	  this, SLOT(catchLog(const Message&)));
  connect(statusLog, SIGNAL(newStopLightColor(StopLightColor, const QString)),
	  diagPanel, SLOT(changeStopLight(StopLightColor, const QString)), 
	  Qt::DirectConnection);
  connect(statusLog, SIGNAL(newStormSignalStatus(StormSignalStatus, 
						 const QString)),
  	  diagPanel, SLOT(changeStormSignal(StormSignalStatus, 
  					    const QString)),
  	  Qt::DirectConnection);
  
  /*
    Didn't solve the crash related to timer in TextEdit
  connect(statusLog, SIGNAL(newStopLightColor(StopLightColor, const QString)),
	  this, SLOT(changeStopLight(StopLightColor, const QString)),
	  Qt::DirectConnection);
  connect(statusLog, SIGNAL(newStormSignalStatus(StormSignalStatus, 
						 const QString)),
	  this, SLOT(changeStormSignal(StormSignalStatus, const QString)),
	  Qt::DirectConnection);
  connect(this, SIGNAL(newStopLightColor(StopLightColor, const QString)),
	  diagPanel, SLOT(changeStopLight(StopLightColor, const QString)),
	  Qt::DirectConnection);
  connect(this, SIGNAL(newStormSignalStatus(StormSignalStatus, const QString)),
	  diagPanel, SLOT(changeStormSignal(StormSignalStatus, const QString)),
	  Qt::DirectConnection);
  */
  
  // Create a new configuration instance
  configData = new Configuration;
  connect(configData, SIGNAL(log(const Message&)), 
	  this, SLOT(catchLog(const Message&)));

  // Define all the widgets

  configDialog = new ConfigurationDialog(this, configData);
  connect(configDialog, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));

  QTabWidget *visuals = new QTabWidget;
  visuals->setTabPosition(QTabWidget::West);

  GraphFace *graph = new GraphFace();
  graph->updateTitle(vortexLabel);
  connect(graph, SIGNAL(log(const Message&)), 
	  this, SLOT(catchLog(const Message&)));

  cappiDisplay = new CappiDisplay();
  cappiDisplay->clearImage();

  imageFileName = graph->getImageFileName();


  // Connect signals and slots that update the configuration
  
  connect(configData, SIGNAL(configChanged()), this, SLOT(updatePage()));

  connect(this, SIGNAL(tabLabelChanged(const QString&)), 
  	  graph, SLOT(updateTitle(const QString&)));


  // Construct Analysis Page Layout
  deficitLabel = new QLabel(tr("Pressure Deficit From 0 km (mb):"));
  currDeficit = new QLCDNumber;
  currDeficit->setSegmentStyle(QLCDNumber::Flat);
  currDeficit->resize(100,100);
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

  tester = new TestGraph;

  connect(example, SIGNAL(clicked()), 
	  tester, SLOT(listPlot()));
  connect(tester, SIGNAL(listChanged(VortexList*)), 
	  graph, SLOT(newInfo(VortexList*)));
  connect(tester, SIGNAL(dropListChanged(VortexList*)), 
	  graph, SLOT(newDropSonde(VortexList*)));
  connect(legend, SIGNAL(clicked()), graph, SLOT(makeKey()));

  connect(this, SIGNAL(vortexListChanged(VortexList*)), 
  	  graph, SLOT(newInfo(VortexList*)));

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

  appMaxWind = new QLCDNumber();
  appMaxWind->setSegmentStyle(QLCDNumber::Flat);
  appMaxWind->resize(100,100);
  appMaxWind->display(cappiDisplay->getMaxApp());
  QLabel* appMaxLabel = new QLabel(tr("Maximum"));
  QLabel* appMaxLabel2 = new QLabel(tr("Approaching Wind (m/s)"));
  recMaxWind = new QLCDNumber();
  recMaxWind->setSegmentStyle(QLCDNumber::Flat);
  recMaxWind->resize(100,100);
  recMaxWind->display(cappiDisplay->getMaxRec());
  QLabel* recMaxLabel = new QLabel(tr("Maximum"));
  QLabel* recMaxLabel2 = new QLabel(tr("Receeding Wind (m/s)"));
  QLabel* emptyLabel = new QLabel();
  QLabel* emptyLabel2 = new QLabel();
  QLabel* emptyLabel3 = new QLabel();  
  QVBoxLayout *appRecLayout = new QVBoxLayout();
  appRecLayout->addStretch();
  appRecLayout->addWidget(emptyLabel, 100, Qt::AlignHCenter);
  appRecLayout->addWidget(appMaxLabel, 0, Qt::AlignHCenter);
  appRecLayout->addWidget(appMaxLabel2, 0, Qt::AlignHCenter);
  appRecLayout->addWidget(appMaxWind, 0, Qt::AlignHCenter);
  appRecLayout->addWidget(emptyLabel2, 100, Qt::AlignHCenter);
  appRecLayout->addWidget(recMaxLabel, 0, Qt::AlignHCenter);
  appRecLayout->addWidget(recMaxLabel2, 0, Qt::AlignHCenter);
  appRecLayout->addWidget(recMaxWind, 0, Qt::AlignHCenter);
  appRecLayout->addWidget(emptyLabel3, 100, Qt::AlignHCenter);
  appRecLayout->addStretch();

  connect(cappiDisplay, SIGNAL(hasImage(bool)), 
	  this, SLOT(updateCappiDisplay(bool)));
  
  QScrollArea* cappiBox = new QScrollArea;
  QHBoxLayout *cappiLayout = new QHBoxLayout(cappiBox);
  cappiLayout->addLayout(appRecLayout);
  cappiLayout->addStretch();
  cappiLayout->addWidget(cappiDisplay);
  cappiLayout->addStretch();
  cappiBox->setLayout(cappiLayout);

  visuals->addTab(pressureGraph, "Pressure & RMW");
  visuals->addTab(cappiBox, "Current Cappi");
  //visuals->setTabEnabled(visuals->indexOf(cappiBox), false);
  
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
  lcdLayout->addWidget(deficitLabel);
  lcdLayout->addWidget(currDeficit);
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
  displayPanel->addWidget(visuals,1,0);
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

  // Do I need this?
  //pollThread = new PollThread;
  pollThread = NULL;

  connect(this, SIGNAL(saveGraphImage(const QString&)),
	  graph, SLOT(saveImage(const QString&)));

}

AnalysisPage::~AnalysisPage()
{
  prepareToClose();
  //  pollThread->exit();
  delete pollThread;
  delete cappiDisplay;
  //delete graph; this gets deleted anyway, not sure how
  delete configData;
  delete diagPanel;
  delete statusLog;
  delete statusText;
  delete configDialog;
  delete currPressure;
  delete currRMW;
  delete currDeficit;
  delete deficitLabel;
  delete tester;
}

void AnalysisPage::newFile()
{
  // Load the default configuration
  if(QDir::current().exists("vortrac_default.xml")) {
    if (!loadFile(QDir::current().filePath("vortrac_default.xml")))
      emit log(Message(QString("Couldn't load default configuration - Please check the default file vortrac_default.xml and insure that it is accesible"),0,this->objectName(),Red));
  }
  else {
    emit log(Message(QString("Couldn't locate default configuration - Please locate vortrac_default.xml and use the open button"),0,this->objectName(),Red));
  }
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
  if (noExtensionFileName.contains('/')){
    int slashIndex = noExtensionFileName.lastIndexOf('/');
    noExtensionFileName.remove(0,slashIndex+1);
  }

  Message::toScreen("No extension file name is ... "+noExtensionFileName);

  if(workingDirectory.exists()) {
    Message::toScreen("AnalysisPage: Saving copies of .log and .png in the working directory: "+workingDirectory.filePath(noExtensionFileName));
    if(!statusLog->saveLogFile(workingDirectory.filePath(noExtensionFileName+".log")))
      Message::toScreen("AnalysisPage: Failed to save .log file in working directory");
      
    emit saveGraphImage(workingDirectory.filePath(noExtensionFileName+".png"));
  }
  else {
    Message::toScreen("AnalysisPage: workingDirectory doesn not exist?");
    statusLog->saveLogFile(noExtensionFileName+".log");
    emit saveGraphImage(noExtensionFileName+".png");
  }

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

  if(!configDialog->checkPanels()) {
    Message::toScreen("Didn't clear all diagnostic hoops");
    emit log(Message(QString("Please check errors: One or more entries in configuration cannot be used"), -1, this->objectName()));
    return;
  }
  
  emit log(Message(QString(),0,this->objectName(),AllOff,QString(),
		   Ok, QString()));

  pollThread = new PollThread();
 
  connect(pollThread, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)), Qt::DirectConnection);
  connect(pollThread, SIGNAL(newVCP(const int)),
	  this, SLOT(catchVCP(const int)), Qt::DirectConnection);
  connect(this, SIGNAL(newVCP(const int)),
	  diagPanel, SLOT(updateVCP(const int)), Qt::DirectConnection);  
  connect(pollThread, SIGNAL(newCappi(const GriddedData*)),
	  this, SLOT(updateCappi(const GriddedData*)), Qt::DirectConnection);
  connect(this, SIGNAL(newCappi(const GriddedData*)), 
	  cappiDisplay, SLOT(constructImage(const GriddedData*)));

  // Check to see if there are old list files that we want to start from in
  // the working directory. We have to do this here because we cannot create 
  // widgets in the threads.

  QString workingDirectoryPath = configData->getParam(configData->getConfig("vortex"),"dir");
  QDir workingDirectory(workingDirectoryPath);
  QString vortexName = configData->getParam(configData->getConfig("vortex"), "name");
  QString radarName = configData->getParam(configData->getConfig("radar"), "name");
  QString nameFilter = vortexName+"_"+radarName+"_";
  QStringList allPossibleFiles = workingDirectory.entryList(QDir::Files);
  allPossibleFiles = allPossibleFiles.filter(nameFilter, Qt::CaseInsensitive);
  bool continuePreviousRun = false;
  QString openOldMessage = QString(tr("Vortrac has found information about a previously started run. Would you like to continue this run?\nPress 'Yes' to continue a previous run. Press 'No' to start a new run."));
  if(allPossibleFiles.count()> 0){
    if(allPossibleFiles.filter("vortexList").count()>0)
      if(allPossibleFiles.filter("simplexList").count()>0) {
	//if(allPossibleFiles.filter("pressureList").count()>0)
	//if(allPossibleFiles.filter("dropSondeList").count()>0)
	int answer = QMessageBox::information(this,tr("VORTRAC"),
					      openOldMessage,3,4,0);
	if(answer== 3)
	  continuePreviousRun = true;
	if(answer==0)
	  return;
      }
    
  }
  
  pollThread->setContinuePreviousRun(continuePreviousRun);

  if(configData->getParam(configData->getConfig("radar"), "format")
     == QString("MODEL")){
    if(analyticModel()) {
      pollThread->setOnlyRunOnce();
      pollThread->setConfig(configData);
      pollThread->start();
    }
  }
  else {
    pollThread->setConfig(configData);
    pollThread->start();
    //connect(pollThread, SIGNAL(vortexListChanged(VortexList*)), 
    //	  graph, SLOT(newInfo(VortexList*)));
    connect(pollThread, SIGNAL(vortexListUpdate(VortexList*)), 
	    this, SLOT(pollVortexUpdate(VortexList*)));
    //connect(pollThread, SIGNAL(dropListChanged(VortexList*)), 
    //		  graph, SLOT(newDropSonde(VortexList*)));
  
  }
}

void AnalysisPage::abortThread()
{
  Message::toScreen("In AnalysisPage Abort");
 
  // Try to kill the threads
  if((pollThread!=NULL)&&(pollThread->isRunning())) {
    disconnect(pollThread, SIGNAL(vortexListUpdate(VortexList*)), 
	       this, SLOT(pollVortexUpdate(VortexList*)));
    pollThread->abortThread();//Deletes everything but doesn't realocate memory
    pollThread = NULL;
    //pollThread = new PollThread;
    pollVortexUpdate(NULL);
    cappiDisplay->clearImage();
    emit log(Message("Analysis Aborted!", -1));
  }
  Message::toScreen("Leaving AnalysisPage Abort");
}


bool AnalysisPage::isRunning()
{
  if ((pollThread!=NULL) && (pollThread->isRunning()))
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

void AnalysisPage::catchVCP(const int vcp)
{
	emit newVCP(vcp);
}

void AnalysisPage::updateCappi(const GriddedData* cappi)
{
  Message::toScreen("Creating Cappi Image");
  // Got a cappi now, create a new image

  emit newCappi(cappi);
}

void AnalysisPage::updateCappiDisplay(bool hasImage)
{
  if(hasImage) {
    appMaxWind->display(cappiDisplay->getMaxApp());
    recMaxWind->display(cappiDisplay->getMaxRec());
  }
  else {
    appMaxWind->display(0);
    recMaxWind->display(0);
  }
  
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
  //abortThread();
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

void AnalysisPage::pollVortexUpdate(VortexList* list)
{
  if((list!=NULL)&&(list->count() > 0)) {
    
    // Find the outermost vtd mean wind coefficient that is not equal to -999
    float maxRadius = 0;
    for(int level = 0; level < list->last().getNumLevels(); level++) {
      for(int rad = 0; rad < list->last().getNumRadii(); rad++) {
	Coefficient current = list->last().getCoefficient(level,rad,QString("VTC0"));
	if((current.getValue()!=-999)&&(current.getValue()!=0)) {
	  if(current.getRadius()>maxRadius)
	    maxRadius = current.getRadius();
	}
      }
    }
    currPressure->display((int)list->last().getPressure());

    // Get the average RMW from those levels with low uncertainty in rmw
    float rmwSum = 0;
    int count = 0;
    int numLev = list->last().getNumLevels();
    for(int ii = 0; ii < numLev; ii++) {
      float newRMW = list->last().getRMW(ii);
      if((newRMW!=0)&&(newRMW!=-999)) {
	if(list->last().getRMWUncertainty(ii) > 10)
	  continue;
	rmwSum += newRMW;
	count++;
      }
    }
    if(count == 0)
      currRMW->display("0");
    else
      currRMW->display(rmwSum/(float)count);	
  
    currDeficit->display(list->last().getPressureDeficit());
    deficitLabel->setText(tr("Pressure Deficit From ")+QString().setNum(maxRadius)+tr(" km (mb):"));
    emit vortexListChanged(list);
  }
  else {
    currPressure->display(0);
    currRMW->display(0);
    currDeficit->display(0);
    deficitLabel->setText(tr("Pressure Deficit From 0 km (mb):"));
    emit vortexListChanged(NULL);
  }
}

/*
void AnalysisPage::changeStopLight(StopLightColor color, const QString message)
{
  emit newStopLightColor(color, message);
}

void AnalysisPage::changeStormSignal(StormSignalStatus status, 
				     const QString message)
{
  emit newStormSignalStatus(status, message);
}
*/
