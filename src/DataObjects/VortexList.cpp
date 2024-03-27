/*
 * VortexList.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/23/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include <QDir>
#include <QXmlStreamWriter>
#include <QFile>
#include <QStringList>
#include <QString>
#include <math.h>
#include <iostream>
#include "VortexList.h"


VortexList::VortexList(QString filePath) : QList<VortexData>()
{
    _filePath = filePath;
}

VortexList::~VortexList()
{

}


bool VortexList::saveXML()
{
  // VortexData record;
    const VortexData *record;
    if(isEmpty())
        return false;
    QFile file(_filePath);
    if(!file.open(QFile::WriteOnly|QFile::Text)){
        std::cout<<"error: Cannot open file"<<_filePath.toStdString()<<std::endl;
        return false;
    }
    QStringList fileParts=QFileInfo(_filePath).fileName().split("_");
    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("vortex");
    xmlWriter.writeTextElement("hurricane",fileParts.at(0));
    xmlWriter.writeTextElement("radar",fileParts.at(1));
    QString tmpStr;
    for(int ii=0;ii<count();ii++){
        xmlWriter.writeStartElement("record");
	record = &(this->at(ii));
	int bestLevel = record->getBestLevel();

	xmlWriter.writeTextElement("time",record->getTime().toString("yyyy/MM/dd hh:mm:ss"));
	tmpStr.asprintf("%6.2f,%6.2f,%6.2f", record->getLat(bestLevel),
                        record->getLon(bestLevel), record->getHeight(bestLevel));
        xmlWriter.writeTextElement("center", tmpStr);
	tmpStr.asprintf("%6.2f,%6.2f,%6.2f,%6.2f", record->getMaxVT(bestLevel), record->getRMW(bestLevel),
                        record->getPressure(), record->getPressureDeficit());
        xmlWriter.writeTextElement("strength",tmpStr);
        xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();
    return true;
}

bool VortexList::restore()
{
    return false;
}

void VortexList::setFilePath(QString newFileName)
{
    _filePath = newFileName;
}


void VortexList::timeSort()
{
  VortexData *vals = data();
  for(int i = 0; i < count(); i++) {
    for(int j = i+1; j < count(); j++) {
      if(vals[i].getTime() > vals[j].getTime()) {
        VortexData tmp(vals[i]);
        vals[i] = vals[j];
        vals[i] = tmp;
      }
    } // j
  } // i
}
