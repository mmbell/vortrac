/*
 * SimplexList.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/23/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "SimplexList.h"
#include <QFileInfo>
#include <QFile>
#include <QXmlStreamWriter>

SimplexList::SimplexList(QString filePath) : QList<SimplexData>()
{
    _filePath = filePath;
}

SimplexList::~SimplexList()
{
}

bool SimplexList::saveXML()
{
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
    SimplexData *record=new SimplexData();
    QString tmpStr;
    for(int vid=0;vid<count();vid++){
        xmlWriter.writeStartElement("record");
        *record=this->at(vid);
        xmlWriter.writeTextElement("time",record->getTime().toString("yyyy/MM/dd hh:mm:ss"));
        for(int hidx=0;hidx<record->getNumLevels();hidx++){
            xmlWriter.writeStartElement("level");
            xmlWriter.writeAttribute("height",QString().setNum(record->getHeight(hidx)));
            for(int ridx=0;ridx<record->getNumRadii();ridx++){
                xmlWriter.writeStartElement("ring");
                xmlWriter.writeAttribute("range",QString().setNum(record->getRadius(ridx)));
                tmpStr.asprintf("%6.2f,%6.2f,%6.2f,%6.2f", record->getMeanX(hidx, ridx),
                                record->getMeanY(hidx, ridx), record->getCenterStdDev(hidx, ridx),
                                record->getMaxVT(hidx, ridx) // , record->getVTUncertainty(hidx, ridx)
                                );
                xmlWriter.writeTextElement("mean value",tmpStr);
                for(int pidx=0;pidx<record->getNumPointsUsed();pidx++){
                    Center center=record->getCenter(hidx,ridx,pidx);
                    tmpStr.asprintf("%6.2f,%6.2f,%6.2f,%6.2f,%6.2f",center.getStartX(),center.getStartY(),center.getX(),center.getY(),center.getMaxVT());
                    xmlWriter.writeTextElement("point value",tmpStr);
                }
                xmlWriter.writeEndElement();
            }
            xmlWriter.writeEndElement();
        }
        xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();
    delete record;
    return true;
}


bool SimplexList::restore()
{
    return false;
}

void SimplexList::timeSort()
{
  SimplexData *vals = data();
  for(int i = 0; i < count(); i++) {
    for(int j = i+1; j < count(); j++) {
      if(vals[i].getTime() > vals[j].getTime()) {
        SimplexData tmp(vals[i]);
        vals[i] = vals[j];
        vals[i] = tmp;
      }
    } // j
  } // i
}

void SimplexList::dump() const
{
  for(int i = 0; i < count(); i++) {
    std::cout << "Simplex element at index " << i << std::endl;
    SimplexData data = at(i);
    data.printString();
  }
}
