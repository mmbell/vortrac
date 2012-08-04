/*
 * StartDialog.h
 * VORTRAC
 *
 * Created by Michael Bell 2012
 *  Copyright 2012 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef STARTDIALOG_H
#define STARTDIALOG_H

#include <QDialog>
#include "Config/Configuration.h"
#include <QWidget>
#include <QComboBox>
#include "AbstractPanel.h"

class StartPanel : public AbstractPanel
{
public:
    StartPanel(Configuration *initialConfig);
    ~StartPanel();
    void updatePanel(const QDomElement panelElement);
    void updateId();
    void updateRadar();
    bool updateConfig();
    bool checkValues();
    void setConfig(Configuration *config) {configData = config;}
    
private:
    QLineEdit *idName;
    QComboBox *radarName;
    Configuration *configData;
};

class StartDialog : public QDialog
{
    Q_OBJECT

public:
    StartDialog(QWidget* parent=0,
                Configuration *initalConfig = 0);
    ~StartDialog();

private:
    StartPanel *panel;
    
private slots:
    void goButton();
    void manualButton();
    
signals:
    void go();
    void manual();

};

#endif