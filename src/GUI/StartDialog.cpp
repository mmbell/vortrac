/*
 * StartDialog.cpp
 * VORTRAC
 *
 * Created by Michael Bell 2012
 *  Copyright 2012 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "StartDialog.h"
#include <QtGui>
#include <QDomElement>

StartDialog::StartDialog(QWidget* parent,
                        Configuration *initialConfig)
:QDialog(parent)
{
    this->setObjectName("Start Dialog");
    setWindowTitle(tr("Start new analysis"));
    
    panel = new StartPanel(initialConfig);
    
    QPushButton *go = new QPushButton(tr("Go"));
    go->setDefault(true);
    QPushButton *manual = new QPushButton(tr("Configure"));
    QHBoxLayout *buttons = new QHBoxLayout;
    connect(go, SIGNAL(clicked()), this, SLOT(goButton()));
    connect(manual, SIGNAL(clicked()), this, SLOT(manualButton()));
    buttons->addStretch(1);
    buttons->addWidget(go);
    buttons->addWidget(manual);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(panel);
    layout->addLayout(buttons);
    setLayout(layout);
}

StartDialog::~StartDialog()
{
    delete panel;
}

void StartDialog::goButton()
{
    panel->updateConfig();
    emit go();
    close();
}

void StartDialog::manualButton()
{
    panel->updateConfig();
    emit manual();
    close();
}

StartPanel::StartPanel(Configuration *initialConfig):AbstractPanel()
{
    configData = initialConfig;
    QLabel *idNameLabel = new QLabel(tr("Storm ID:"));
    idName = new QLineEdit();
    QHBoxLayout *nameLayout = new QHBoxLayout;
    nameLayout->addWidget(idNameLabel);
    nameLayout->addWidget(idName);
    connect(idName, SIGNAL(textChanged(const QString&)), this, SLOT(valueChanged()));

    QString resources = QCoreApplication::applicationDirPath() + "/../Resources";
    QDir resourceDir = QDir(resources);
    QLabel *radarNameLabel = new QLabel(tr("Radar Name:"));
    radarName = new QComboBox();
    radars = new Configuration(this,resourceDir.filePath(QString("vortrac_radarList.xml")));
    connect(radars, SIGNAL(log(const Message&)),
            this, SLOT(catchLog(const Message&)));
    QDomNodeList radarList =
    radars->getRoot().childNodes();
    for (int i = 0; i <= radarList.count()-1; i++)
    {
        QDomNode curNode = radarList.item(i);
        radarName->addItem(curNode.firstChildElement(QString("text")).text());
    }
    radarName->setEditable(false);
    connect(radarName, SIGNAL(activated(const QString&)),
            this, SLOT(valueChanged()));
//    connect(radarName, SIGNAL(activated(const QString&)),
//            this, SLOT(radarChanged(const QString&)));
    
    QHBoxLayout *radarLayout = new QHBoxLayout;
    radarLayout->addWidget(radarNameLabel);
    radarLayout->addWidget(radarName);
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(nameLayout);
    layout->addLayout(radarLayout);
    setLayout(layout);
    
    connect(this, SIGNAL(log(const Message&)),
            this, SLOT(catchLog(const Message&)));
    
    connect(this, SIGNAL(changeDom(const QDomElement&, 
                                    const QString&, const QString&)), 
            configData, SLOT(setParam(const QDomElement&, 
                                      const QString&, const QString&)));
    connect(this, SIGNAL(changeDom(const QDomElement&, const QString&, 
                                    const QString&, const QString&, 
                                    const QString&)), 
            configData, SLOT(setParam(const QDomElement&, const QString&, 
                                      const QString&, const QString&,
                                      const QString&)));

}

StartPanel::~StartPanel()
{
    delete idName;
    delete radarName;
    delete radars;
}

void StartPanel::updatePanel(const QDomElement panelElement)
{
    // Not needed
}

void StartPanel::updateId()
{
    // Sets the location of the panel's information in the Configuration,
    // iterates through all elements within this section of the Configuration,
    // and writes the values of these parameters to the corresponding member
    // widget.
    QDomNodeList nodeList = configData->getRoot().childNodes();
    for (int i = 0; i <= nodeList.count()-1; i++) 
    {
        QDomNode currNode = nodeList.item(i);
        QDomElement child = currNode.toElement();
        if (child.tagName() == "vortex") {
            while (!child.firstChildElement().isNull())
            {
                QString name = child.tagName();
                QString parameter = child.text();
                if (name == "id") {
                    idName->clear();
                    idName->insert(parameter); }
            }
        }            
    }
    setPanelChanged(false);
}

void StartPanel::updateRadar()
{
    // Sets the location of the panel's information in the Configuration,
    // iterates through all elements within this section of the Configuration,
    // and writes the values of these parameters to the corresponding member
    // widget.
    QDomNodeList nodeList = configData->getRoot().childNodes();
    for (int i = 0; i <= nodeList.count()-1; i++) 
    {
        QDomNode currNode = nodeList.item(i);
        QDomElement child = currNode.toElement();
        if (child.tagName() == "radar") {
            while (!child.firstChildElement().isNull())
            {
                QString name = child.tagName();
                QString parameter = child.text();
                if(name == "name") {
                    int index = radarName->findText(parameter,
                                                    Qt::MatchStartsWith);
                    if (index != -1)
                        radarName->setCurrentIndex(index);
                    else
                        radarName->setCurrentIndex(0);}
            }
        }
    }
    setPanelChanged(false);
}

bool StartPanel::updateConfig()
{
    // If any of the Panel's members have been changed these values will be
    // writen to the corresponding location within the Configuration
    if (checkPanelChanged())
    {
        QDomNodeList nodeList = configData->getRoot().childNodes();
        for (int i = 0; i <= nodeList.count()-1; i++) 
        {
            QDomNode currNode = nodeList.item(i);
            QDomElement child = currNode.toElement();
            if (child.tagName() == "vortex") {
                if(child.firstChildElement("id").text()!=idName->text()) {
                    emit changeDom(child, "id", idName->text());
                }
            }                
            if (child.tagName() == "radar") {
                QString radarID = radarName->currentText().left(4);
                if(child.firstChildElement("name").text()!= radarID) {
                    if(radarID!=QString("Plea")) {
                        emit changeDom(child, "name", radarID);
                        QDomElement radar = radars->getRoot().firstChildElement(radarID);
                        emit changeDom(child, "lat",
                                       radar.firstChildElement("latitude").text());
                        emit changeDom(child, "lon",
                                       radar.firstChildElement("longitude").text());
                        emit changeDom(child, "alt",
                                       radar.firstChildElement("latitude").text());
                    }
                }

            }
        }

    }
    setPanelChanged(false);
    return true;
}

bool StartPanel::checkValues()
{
    // Returning False means that one of the values has not been set correctly
    // Returning True means that all the values check out...
    emit log(Message(QString(), 0, this->objectName(), Green));
    return true;
}


