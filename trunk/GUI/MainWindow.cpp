/*
 * MainWindow.cpp
 * VORTRAC
 *
 * Created by Michael Bell on 7/11/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 ** Parts of this code are taken from Qt 4.0 examples
 ** Copyright (C) 2005-2005 Trolltech AS. All rights reserved.
 */

#include <QtGui>

#include "MainWindow.h"

MainWindow::MainWindow()
{
  this->setObjectName("Main Window");
    tabWidget = new QTabWidget();
    setCentralWidget(tabWidget);
    connect(tabWidget, SIGNAL(currentChanged(int)),
            this, SLOT(updateMenus()));
    tabMapper = new QSignalMapper(this);
    connect(tabMapper, SIGNAL(mapped(int)),
            tabWidget, SLOT(setCurrentIndex(int)));

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    updateMenus();

    readSettings();

    setWindowTitle(tr("VORTRAC"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{

    if (activeAnalysisPage()) 
      {
	tabWidget->setCurrentIndex(tabWidget->count()-1);
	int numTabs = tabWidget->count();
	int i = 0;
	for(; i < numTabs; i++)
	  {
	    if (!closeActiveAnalysis())
	      {
		event->ignore();
		break;
	      }
	  }
	if(i == numTabs)
	  {
	    writeSettings();
	    event->accept();
	  }
	else 
	  event->ignore();
      } 
    else 
      {
        writeSettings();
        event->accept();
      }
}

void MainWindow::newFile() 
{
    AnalysisPage *child = createAnalysisPage();
    child->newFile();
    tabWidget->addTab(child,child->getVortexLabel());
    tabWidget->setCurrentIndex(tabWidget->count() - 1);
    
    connect(this, SIGNAL(log(const Message&)), 
	    child, SLOT(catchLog(const Message&)));

    updateMenus();
    updateVortexMenu();
    child->setVortexLabel();
    
}

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, QString(tr("Select Configuration File")), QString(), QString("Vortrac Configuration Files (*.xml)"));
  
    if (!fileName.isEmpty()) {
        AnalysisPage *existing = findAnalysisPage(fileName);
        if (existing) {
            tabWidget->setCurrentWidget(existing);
            return;
        }

        AnalysisPage *child = createAnalysisPage();
        if (child->loadFile(fileName)) {
            statusBar()->showMessage(tr("File loaded"), 2000);
        } else {
            child->close();
        }
	tabWidget->addTab(child,child->getVortexLabel());
	tabWidget->setCurrentIndex(tabWidget->count() - 1);

	connect(this, SIGNAL(log(const Message&)), 
		child, SLOT(catchLog(const Message&)));

	updateMenus();
	updateVortexMenu();
	child->setVortexLabel();
    }
}

void MainWindow::save()
{
    if (activeAnalysisPage()->save())
        statusBar()->showMessage(tr("File saved"), 2000);
}

void MainWindow::saveAs()
{
    if (activeAnalysisPage()->saveAs())
        statusBar()->showMessage(tr("File saved"), 2000);
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("About VORTRAC"),
		      tr("Vortex Objective Radar Tracking and Circulation Version 1.0\nDeveloped by Wen-Chau Lee, Paul Harasti, Michael Bell, and Lisa Mauger\nUniversity Corporation for Atmospheric Research"));
}

void MainWindow::updateMenus()
{

    bool hasAnalysisPage = (activeAnalysisPage() != 0);
    saveAct->setEnabled(hasAnalysisPage);
    saveAsAct->setEnabled(hasAnalysisPage);
    closeAct->setEnabled(hasAnalysisPage);
    configAct->setEnabled(hasAnalysisPage);
    nextAct->setEnabled(hasAnalysisPage);
    previousAct->setEnabled(hasAnalysisPage);
    separatorAct->setVisible(hasAnalysisPage);
    saveGraphAct->setEnabled(hasAnalysisPage);
    saveLogAct->setEnabled(hasAnalysisPage);
    runAct->setEnabled(hasAnalysisPage && 
		       !(activeAnalysisPage()->isRunning()));
    abortAct->setEnabled(hasAnalysisPage &&
			 activeAnalysisPage()->isRunning());
    if(hasAnalysisPage) {
    connect(runAct, SIGNAL(triggered()), 
	    this->activeAnalysisPage(), SLOT(runThread()));
    connect(abortAct, SIGNAL(triggered()), 
	    this->activeAnalysisPage(), SLOT(abortThread()));
    }

}

void MainWindow::updateVortexMenu()
{
    vortexMenu->clear();
    vortexMenu->addAction(runAct);
    vortexMenu->addAction(abortAct);
    vortexMenu->addAction(configAct);
    vortexMenu->addAction(saveGraphAct);
    vortexMenu->addAction(saveLogAct);
    vortexMenu->addAction(closeAct);
    vortexMenu->addSeparator();
    vortexMenu->addAction(nextAct);
    vortexMenu->addAction(previousAct);
    vortexMenu->addAction(separatorAct);

    for (int i = 0; i < tabWidget->count(); ++i) 
      {
	AnalysisPage *child = (AnalysisPage *)tabWidget->widget(i);
	// TabPage *child = qobject_cast<TabPage *>tabWidget->widget(i);
	QString text;
	if (i < 9) 
	  {
	  text = tr("&%1. %2").arg(i + 1)
	    .arg(child->getVortexLabel());
	  }
	else 
	  {
	    text = tr("%1. %2").arg(i + 1)
	      .arg(child->getVortexLabel());
	  }
	QAction *action  = vortexMenu->addAction(text);
	action->setCheckable(true);
	action ->setChecked(child == activeAnalysisPage());
	connect(action, SIGNAL(triggered()), tabMapper, SLOT(map()));
	tabMapper->setMapping(action, i);
      }
}

AnalysisPage *MainWindow::createAnalysisPage()
{
  AnalysisPage *child = new AnalysisPage;
  
  // Connect the configuration to the tab widget
  connect(child, SIGNAL(tabLabelChanged(QWidget*, const QString&)), 
	  this, SLOT(updateTabLabel(QWidget*, const QString&)));
  
  return child;
}

void MainWindow::createActions()
{
    newAct = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
    newAct->setShortcut(tr("Ctrl+N"));
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcut(tr("Ctrl+O"));
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveAct->setShortcut(tr("Ctrl+S"));
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    closeAct = new QAction(QIcon(":/images/delete.png"), tr("Cl&ose"), this);
    closeAct->setShortcut(tr("Ctrl+F4"));
    closeAct->setStatusTip(tr("Close the current vortex"));
    connect(closeAct, SIGNAL(triggered()),
            this, SLOT(closeActiveAnalysis()));

    nextAct = new QAction(tr("Ne&xt"), this);
    nextAct->setShortcut(tr("Ctrl+F6"));
    nextAct->setStatusTip(tr("Move to the next vortex"));
    connect(nextAct, SIGNAL(triggered()),
            this, SLOT(activateNextAnalysis()));

    previousAct = new QAction(tr("Pre&vious"), this);
    previousAct->setShortcut(tr("Ctrl+Shift+F6"));
    previousAct->setStatusTip(tr("Move to the previous vortex"));
    connect(previousAct, SIGNAL(triggered()),
            this, SLOT(activatePreviousAnalysis()));

    separatorAct = new QAction(this);
    separatorAct->setSeparator(true);

    aboutAct = new QAction(tr("About VORTRAC"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    configAct = new QAction(tr("Con&figuration"), this);
    configAct->setShortcut(tr("Ctrl+F"));
    configAct->setStatusTip(tr("View and Modify the Vortex Configuration"));
    connect(configAct, SIGNAL(triggered()), this, SLOT(openConfigDialog()));

    runAct = new QAction(tr("Run VORTRAC"), this);
    runAct->setStatusTip(tr("Run VORTRAC Analysis"));

    abortAct = new QAction(tr("Abort VORTRAC"), this);
    abortAct->setStatusTip(tr("Abort VORTRAC Analysis"));

    saveGraphAct = new QAction(tr("Save Graph"), this);
    saveGraphAct->setStatusTip(tr("Save Graph to File"));
    connect(saveGraphAct, SIGNAL(triggered()), this, SLOT(saveGraph()));

    saveLogAct = new QAction(tr("Save Log File"),this);
    saveLogAct->setStatusTip(tr("Save Log Information to a File"));
    connect(saveLogAct, SIGNAL(triggered()), this, SLOT(saveLog()));
}

bool MainWindow::closeActiveAnalysis()
{

  if(tabWidget->count() > 0) 
    {
      if (!activeAnalysisPage()->maybeSave())
	return false;
      int newActiveIndex, indexToBeClosed = tabWidget->currentIndex();
      if (tabWidget->count() != 1)
	{
	  if (indexToBeClosed == (tabWidget->count()-1))
	    newActiveIndex = (indexToBeClosed-1);
	  else
	    newActiveIndex = (indexToBeClosed);
	  delete tabWidget->currentWidget();
	  tabWidget->setCurrentIndex(newActiveIndex);
	}
      else {
	delete tabWidget->currentWidget();
      }
    }
  updateMenus();
  return true;
  
}

void MainWindow::activateNextAnalysis()
{
  int nextIndex = tabWidget->currentIndex() + 1;
  if( nextIndex <= (tabWidget->count() - 1)) {
    tabWidget->setCurrentIndex(nextIndex);
  }

}

void MainWindow::activatePreviousAnalysis()
{
  int prevIndex = tabWidget->currentIndex() - 1;
  if( prevIndex >= 0 ) {
    tabWidget->setCurrentIndex(prevIndex);
  }

}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    vortexMenu = menuBar()->addMenu(tr("&Vortex"));
    connect(vortexMenu, SIGNAL(aboutToShow()), this, SLOT(updateVortexMenu()));

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&About"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);
    fileToolBar->addAction(closeAct);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
    QSettings settings("Trolltech", "MDI Example");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    move(pos);
    resize(size);
}

void MainWindow::writeSettings()
{
    QSettings settings("Trolltech", "MDI Example");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}

AnalysisPage *MainWindow::activeAnalysisPage()
{
    return qobject_cast<AnalysisPage *>(tabWidget->currentWidget());
}

AnalysisPage *MainWindow::findAnalysisPage(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();
    for (int i = 0; i < tabWidget->count(); ++i) {
      AnalysisPage *child = (AnalysisPage *)tabWidget->widget(i);
      if (child->currentFile() == canonicalFilePath)
	return child;
    }
    return 0;
}

void MainWindow::updateTabLabel(QWidget* labelWidget, 
				const QString& new_Label)
{
  tabWidget->setTabText(tabWidget->indexOf(labelWidget), new_Label);
}

void MainWindow::openConfigDialog()
{
  activeAnalysisPage()->getConfigDialog()->checkConfig();
  activeAnalysisPage()->getConfigDialog()->show();
}

void MainWindow::saveGraph()
{
  //This will not work for me under any conditions.
}

void MainWindow::saveLog()
{
  activeAnalysisPage()->saveLog();
}

void MainWindow::catchLog(const Message& message)
{
  emit log(message);
}


