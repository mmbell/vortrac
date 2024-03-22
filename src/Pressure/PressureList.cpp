/*
 * PressureList.cpp
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

#include <iostream>

#include "PressureList.h"

PressureList::PressureList(QString prsFilePath) : QList<PressureData>()
{
    _filePath = prsFilePath;
}
PressureList::~PressureList()
{
}

void PressureList::setFilePath(QString prsFilePath)
{
    _filePath=prsFilePath;
}

bool PressureList::saveXML()
{
  if (isEmpty())
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
  xmlWriter.writeStartElement("pressures");
  xmlWriter.writeTextElement("hurricane", fileParts.at(0));
  xmlWriter.writeTextElement("radar", fileParts.at(1));
  
  QString tmpStr;
  PressureData record;
  
  for(int ii = 0; ii < count(); ++ii) {
    xmlWriter.writeStartElement("record");
    record = this->at(ii);
    xmlWriter.writeTextElement("time", record.getTime().toString("yyyy/MM/dd hh:mm:ss"));

    xmlWriter.writeTextElement("stationName", record.getStationName());
    
    tmpStr.asprintf("%6.2f,%6.2f,%6.2f", record.getLat(), record.getLon(), record.getAltitude());
    xmlWriter.writeTextElement("location", tmpStr);

    tmpStr.asprintf("%6.2f",record.getPressure());
    xmlWriter.writeTextElement("pressure", tmpStr);
    
    tmpStr.asprintf("%6.2f",record.getWindSpeed());
    xmlWriter.writeTextElement("windSpeed", tmpStr);
	
    tmpStr.asprintf("%6.2f",record.getWindDirection());
    xmlWriter.writeTextElement("windDirection", tmpStr);
	
    xmlWriter.writeEndElement();
  }
  return true;
}

bool PressureList::restore()
{
    return false;
}
