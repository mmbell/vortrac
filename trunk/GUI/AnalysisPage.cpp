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

  statusLog = new Log();
  
  connect(this, SIGNAL(log(const Message&)),
	  statusLog, SLOT(catchLog(const Message&)));

  // Create a new configuration instance
  configData = new Configuration;
  connect(configData, SIGNAL(log(const Message&)), 
	  statusLog, SLOT(catchLog(const Message&))); 
  
  // Define all the widgets

  configDialog = new ConfigurationDialog(configData);
  connect(configDialog, SIGNAL(log(const Message&)),
	  statusLog, SLOT(catchLog(const Message&)));
  connect(configDialog, SIGNAL(configChanged()), this, SLOT(updatePage()));
  
  configTree = new ConfigTree(window(), configData);
  configTree->setFixedWidth(300);
  connect(configTree, SIGNAL(log(const Message&)),
  	  statusLog, SLOT(catchLog(const Message&)));

  GraphFace *graph = new GraphFace(vortexLabel);
  connect(graph, SIGNAL(log(const Message&)), 
	  statusLog, SLOT(catchLog(const Message&)));


  // Connect signals and slots that update the configuration
  
  connect(configData, SIGNAL(configChanged()), this, SLOT(updatePage()));
  connect(configDialog, SIGNAL(configChanged()), configTree, SLOT(reread()));
  connect(configTree, SIGNAL(newParam(const QDomElement&, 
				      const QString&, const QString&)), 
	  configData, SLOT(setParam(const QDomElement&, 
				    const QString&, const QString&)));
  connect(configTree, SIGNAL(addDom(const QDomElement&, const QString&, 
				    const QString&)), 
	  configData, SLOT(addDom(const QDomElement&,
				  const QString&, const QString&)));
  connect(configTree, SIGNAL(removeDom(const QDomElement&,const QString&)),
	  configData, SLOT(removeDom(const QDomElement&,const QString&)));

  connect(this, SIGNAL(tabLabelChanged(const QString&)), 
  	  graph, SLOT(updateTitle(const QString&)));


  // Construct Analysis Page Layout
 
  QLabel *pressureLabel = new QLabel(tr("Current Pressure (mb):"));
  QLCDNumber *currPressure = new QLCDNumber;
  QLabel *rmwLabel = new QLabel(tr("Current RMW (km):"));
  QLCDNumber *currRMW = new QLCDNumber;
  
  QGroupBox *pressureGraph = new QGroupBox;
  
  QPushButton *legend = new QPushButton("See Legend");
  QPushButton *saveGraph = new QPushButton("Save Graph");
  
  //--For Demo
  QHBoxLayout *subLayout = new QHBoxLayout;

  QPushButton *example = new QPushButton("Plot Example Points");

  TestGraph *tester = new TestGraph;

  connect(example, SIGNAL(clicked()), 
	  tester, SLOT(examplePlot()));
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
  connect(configDialog, SIGNAL(log(const Message&)),
	  statusLog, SLOT(catchLog(const Message&)));
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
  displayPanel->addWidget(statusText);
  displayPanel->addLayout(runLayout);

  QHBoxLayout *display = new QHBoxLayout;
  display->addWidget(configTree);
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
	  statusLog, SLOT(catchLog(const Message&)), Qt::DirectConnection);
  
}

void AnalysisPage::newFile()
{

  // Load the default configuration
  if (!loadFile("vortrac_default.xml"))
    emit log(Message("Couldn't load default configuration"));
  
  // Set the current filename to untitled
  isUntitled = true;
  configFileName = "vortrac_newconfig.xml";

}

bool AnalysisPage::loadFile(const QString &fileName)
{
  // Load up a configuration file in the configTree and ConfigurationDialog
  if (!configData->read(fileName)) {
    emit log(Message("Couldn't load configuration file"));
    return false;
  }
  configTree->read();
  configDialog->read();

  // Set the filename
  configFileName = fileName;
  
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
  if (maybeSave()) 
    {
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

}

void AnalysisPage::runThread()
{
  // Start a processing thread using the current configuration

  pollThread.setConfig(configData);
  pollThread.start();

}

void AnalysisPage::abortThread()
{

  // Try to kill the thread
  if(pollThread.isRunning()) {
	pollThread.quit();
  }

  emit log(Message("Analysis Aborted!"));

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
  if(pos = lastMax) {
    lastMax = statusText->verticalScrollBar()->maximum();
    statusText->verticalScrollBar()->setValue(lastMax);
  }
  else
    lastMax = statusText->verticalScrollBar()->maximum();
}
