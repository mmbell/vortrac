/*
 * AnalysisPage.h
 * VORTRAC
 *
 * Created by Michael Bell on 7/21/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef ANALYSISPAGE_H
#define ANALYSISPAGE_H

#include <QWidget>
#include <QTextEdit>
#include <QFile>
#include "Config/Configuration.h"
#include "IO/Log.h"
#include "ConfigTree.h"
#include "Threads/PollThread.h"
#include "GraphFace.h"
#include "ConfigurationDialog.h"
#include "DiagnosticPanel.h"

#include "TestGraph.h"

class AnalysisPage : public QWidget
{

  Q_OBJECT

 public:

  AnalysisPage(QWidget *parent = 0);
  ~AnalysisPage();

  void newFile();
  bool loadFile(const QString &fileName);
  bool save();
  bool saveAs();
  bool saveFile(const QString &fileName);
  QString getVortexLabel() { return vortexLabel; }
  QString currentFile() { return configFileName; }
  QDir getWorkingDirectory() { return workingDirectory; }
  void setVortexLabel();
  bool maybeSave();
  void saveDisplay();
  

  GraphFace *getGraph() {return graph;}
  Log* getStatusLog(){ return statusLog;} 
  ConfigurationDialog* getConfigDialog(){ return configDialog;}
  Configuration* getConfiguration() {return configData;}
  bool isRunning();

  public slots:
    void saveLog();
    void catchLog(const Message& message); 

 signals:
    void tabLabelChanged(const QString& new_Label);
    void log(const Message& message);
    void saveGraphImage(const QString& name);
  
  
 private slots:
    void updatePage();
    void runThread();
    void abortThread();
    void autoScroll();
    void prepareToClose();

 protected:
  void closeEvent(QCloseEvent *event);

 private:
  QString vortexLabel;
  QString configFileName;
  Configuration *configData;
  // ConfigTree *configTree;
  DiagnosticPanel *diagPanel;
  Log *statusLog;
  QTextEdit *statusText;
  int lastMax;
  PollThread pollThread;
  bool isUntitled;
  GraphFace* graph;
  ConfigurationDialog *configDialog;
  QDir workingDirectory;
  QString imageFileName;
  QLCDNumber *currPressure;
  QLCDNumber *currRMW;

  bool analyticModel();

};

#endif
