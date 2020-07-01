/*
 *  Configuration.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/6/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QDomDocument>
#include <QDomNodeList>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QHash>
#include "IO/Message.h"

class Configuration:public QObject
{

    Q_OBJECT

public:
    Configuration(QObject *parent = 0,const QString &filename = QString());
    ~Configuration();

    QDomElement getRoot();
    QDomNodeList* getGroupList();
    bool read(const QString &filename);
    bool write(const QString &filename);
    bool isNull();
    bool loggingChanges() { return logChanges; }
    void setLogChanges(bool logStatus);

    QDomElement getConfig(const QString &configName) const;
    QDomElement getConfig(const QString &configName,
                          const QString &attribName,
                          const QString &attribValue) const;

    const QString getParam(const QDomElement &element,
                           const QString &paramName) const;

    const QString getParam(const QDomElement &element,
                           const QString &paramName,
                           const QString &attribName,
                           const QString &attribValue) const;

    const QString getAttribute(const QDomElement &element,
                               const QString &paramName,
                               const QString &attribName);
    bool checkModified() { return isModified; }

    const QDomElement getElement(const QDomElement &element,const QString &paramName) const;

    const QDomElement getElementWithAttrib(const QDomElement &element,
                                           const QString &paramName,
                                           const QString &attribName,
                                           const QString &attribValue) const;

    Configuration& operator = (const Configuration &other);

    // bool operator == (const Configuration &other);

    QString findConfigNameStartsWith(const QString& name);

public slots:
    void catchLog(const Message& message);

    void setParam(const QDomElement &element,
                  const QString &paramName,
                  const QString &paramValue);

    void setParam(const QDomElement &element,
                  const QString &paramName,
                  const QString &paramValue,
                  const QString &attribName,
                  const QString &attribValue);

    void addDom(const QDomElement &element,
                const QString &paramName,
                const QString &paramValue);

    void addDom(const QDomElement &element,
                const QString &paramName,
                const QString &paramValue,
                const QString &attribName,
                const QString &attribValue);

    void removeDom(const QDomElement &element,
                   const QString &paramName);

    void removeDom(const QDomElement &element,
                   const QString &paramName,
                   const QString &attribName,
                   const QString &attribValue);

    void setAttribute(const QDomElement &element,
                      const QString &paramName,
                      const QString &attribName,
                      const QString &attibValue);

protected:
    // Validation is specific to each kind of configuration
    virtual bool validate();
    QDomDocument domDoc;

private:
    QDomElement root;
    QDomNodeList groupList;
    QHash<QString, int> indexForTagName;
    bool isModified;
    bool logChanges;

signals:
    void log(const Message& message) const;
    void configChanged();
};

#endif
