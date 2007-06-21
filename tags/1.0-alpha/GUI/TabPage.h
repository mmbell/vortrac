/*
 * TabPage.h
 * VORTRAC
 *
 * Created by Michael Bell on 7/21/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef TABPAGE_H
#define TABPAGE_H

#include <QWidget>
#include "Config/Configuration.h"
#include "IO/Log.h"
#include "ConfigTree.h"
#include "AnalysisThread.h"

class TabPage : public QWidget
{

  Q_OBJECT

 public:
  TabPage(QWidget *parent = 0);
  void newFile();
  bool loadFile(const QString &fileName);
  bool save();
  bool saveAs();
  bool saveFile(const QString &fileName);
  QString getVortexLabel() { return vortexLabel; }
  QString currentFile() { return configFileName; }
  void setVortexLabel();
  bool maybeSave();

 signals:
  void labelUpdate(QString);
  
 private slots:
   void updatePage();

 protected:
  void closeEvent(QCloseEvent *event);

 private:
  QString vortexLabel;
  QString configFileName;
  Configuration *configData;
  ConfigTree *configTree;
  Log *statusLog;
  AnalysisThread *analysisThread;
  bool isUntitled;

};

#endif
