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

#include <QThread>
#include "ConfigTree.h"
#include "GraphFace.h"
#include "DiagnosticPanel.h"
#include "StartDialog.h"
//#include "TestGraph.h"

#include "DriverAnalysis.h"

class AnalysisPage : public DriverAnalysis
{

    Q_OBJECT

public:

    AnalysisPage(QWidget *parent = 0);
    ~AnalysisPage();

    void newFile();
    bool save();
    bool saveAs();
    bool saveFile(const QString &fileName);
    bool loadFile(const QString &fileName);
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

signals:
    void tabLabelChanged(QWidget* labelWidget, const QString& new_Label);
    void saveGraphImage(const QString& name);

private slots:
    void updateTcvitals();
    void updatePage();
    void runThread();
    void abortThread();
    void autoScroll();
    void prepareToClose();
    void popUpStatus();
    void updateStatusLog(const QString& entry);
    void openConfigDialog();
    
protected:
    void closeEvent(QCloseEvent *event);

private:
    //QThread* thread;
    // ConfigTree *configTree;
    QTextEdit *textPopUp;
    QDialog *popUp;
    int lastMax;
    GraphFace* graph;
    QDir workingDirectory;
    QString imageFileName;

    QTabWidget* visuals;

    QPushButton *runButton;
    QPushButton *abortButton;
    //  TestGraph *tester;

    bool analyticModel();

    StartDialog *quickstart;
};

#endif
