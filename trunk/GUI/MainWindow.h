/*
 * MainWindow.h
 * VORTRAC
 *
 * Created by Michael Bell on 7/11/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "AnalysisPage.h"
#include "IO/Message.h"

class QAction;
class QMenu;
class QTabWidget;
class QSignalMapper;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    
public slots:
    void catchLog(const Message& message);

signals:
    void log(const Message& message); 

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void newFile();
    void open();
    void save();
    void saveAs();
    void about();
    void updateMenus();
    void updateVortexMenu();
    bool closeActiveAnalysis();
    void activateNextAnalysis();
    void activatePreviousAnalysis();
    AnalysisPage *createAnalysisPage();
    void openConfigDialog();
    void updateTabLabel(QWidget* labelWidget, const QString& new_Label);
    void saveGraph();
    void saveLog();

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    AnalysisPage *activeAnalysisPage();
    AnalysisPage *findAnalysisPage(const QString &fileName);

    QTabWidget *tabWidget;
    QSignalMapper *tabMapper;

    QMenu *fileMenu;
    QMenu *vortexMenu;
    QMenu *helpMenu;
    QToolBar *fileToolBar;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QAction *closeAct;
    QAction *nextAct;
    QAction *previousAct;
    QAction *separatorAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *configAct;
    QAction *runAct;
    QAction *abortAct;
    QAction *saveGraphAct;
    QAction *saveLogAct;

};

#endif
