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
    VortexData record;
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
        record=this->at(ii);
        xmlWriter.writeTextElement("time",record.getTime().toString("yyyy/MM/dd hh:mm:ss"));
        //center lat,lon,alt
        tmpStr.sprintf("%6.2f,%6.2f,%6.2f",record.getLat(),record.getLon(),record.getHeight());
        xmlWriter.writeTextElement("center", tmpStr);

        //maxVT, RMW, Pressure, Pressure deficit (check for uninitialized)
        tmpStr.sprintf("%6.2f,%6.2f,%6.2f,%6.2f",record.getMaxVT(),record.getRMW(),record.getPressure(),record.getPressureDeficit());
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
    for(int i = 0; i < this->count(); i++)
        for(int j = i+1; j < this->count(); j++)
            if(this->at(i).getTime()>this->at(j).getTime())
                this->swap(j,i);
}
