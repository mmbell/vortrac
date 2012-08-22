/*
 *  PollThread.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/25/05.
 *  Copyright 2005 University Corporation for Atmospheric Research. All rights reserved.
 *
 */

#include <QtGui>
#include "workThread.h"
#include "Message.h"
#include <math.h>
#include "Radar/RadarQC.h"

#include "DataObjects/SimplexList.h"

workThread::workThread(QObject *parent)
    : QThread(parent)
{
    this->setObjectName("Master");
    abort = false;
    runOnce = false;

    dataSource= NULL;
    pressureSource= NULL;
    configData= NULL;

}

workThread::~workThread()
{
    stop();
    this->quit();
}

void workThread::stop()
{
    abort = true;
}

void workThread::run()
{

    //Initialize configuration
    QString mode = configData->getParam(configData->getConfig("vortex"), "mode");
    QDir workingDir(configData->getParam(configData->getConfig("vortex"),"dir"));
    QString vortexName = configData->getParam(configData->getConfig("vortex"), "name");
    if (vortexName == "Unknown") {
        // Problem with ATCF data
        Message newMsg(QString("Vortex name is unknown, please check ATCF data feed"),-1,this->objectName(),
                       Red,"ATCF Error");
        emit log(newMsg);
    }
    QString radarName = configData->getParam(configData->getConfig("radar"), "name");
    QString year=QString().setNum(QDate::fromString(configData->getParam(configData->getConfig("radar"),"startdate"), "yyyy-MM-dd").year());
    QString namePrefix=vortexName+"_"+radarName+"_"+year+"_";

    //initialize the saving path of data-list
    _simplexList.setFilePath(workingDir.filePath(namePrefix+"simplexlist.xml"));
    _vortexList.setFilePath(workingDir.filePath(namePrefix+"vortexlist.xml"));
    _pressureList.setFilePath(workingDir.filePath(namePrefix+"pressurelist.xml"));
    if(continuePreviousRun){
        _simplexList.restore();
        _vortexList.restore();
        _pressureList.restore();
    }

    //create data monitor object
    dataSource = new RadarFactory(configData);
    connect(dataSource, SIGNAL(log(const Message&)),this, SLOT(catchLog(const Message&)));
    PressureFactory *pressureSource = new PressureFactory(configData);
    connect(pressureSource, SIGNAL(log(const Message&)),this, SLOT(catchLog(const Message&)));
    
    // Begin working loop
    while(!abort) {
        //STEP 1: Check for new data
        if (dataSource->hasUnprocessedData()) {
        if(abort) break;
            // Update the data queue with any knowledge of any volumes that might have already been processed
            dataSource->updateDataQueue(&_vortexList);

            //STEP 2: Select a volume off the queue,try to read it
            RadarData *newVolume = dataSource->getUnprocessedData();
            if(newVolume == NULL) {
                delete newVolume;
                continue;
            }
            emit log(Message("Found file:" + newVolume->getFileName(), -1, this->objectName()));
            // Check to makes sure that the file still exists and is readable
            if(!newVolume->fileIsReadable()) {
                emit log(Message(QString("The radar data file "+newVolume->getFileName()+" is not readable"), -1,this->objectName()));
                delete newVolume;
                continue;
            }
            newVolume->readVolume();
            emit newVCP(newVolume->getVCP());
            //radar data quality control
            RadarQC* dealiaser=new RadarQC(newVolume);
            connect(dealiaser,SIGNAL(log(const Message&)),this,SLOT(catchLog(const Message&)));
            dealiaser->getConfig(configData->getConfig("qc"));
            dealiaser->dealias();
            emit log(Message("Finished QC and Dealiasing",10, this->objectName()));
            delete dealiaser;
	if(abort) break;
            //STEP 3: get the first guess of center Lat,Lon for simplex
            _latlonFirstGuess(newVolume);
            QString currentCenter("Processing radar volume at "+ newVolume->getDateTime().toString("hh:mm") + " with (" +
                                  QString().setNum(_firstGuessLat)+", "+QString().setNum(_firstGuessLon)+") center estimate");
            emit log(Message(currentCenter,1,this->objectName()));
        if(abort) break;
            //STEP 4: from Radardata ---> Griddata, make cappi
            GriddedFactory* gridFactory=new GriddedFactory();
            GriddedData* gridData=gridFactory->makeCappi(newVolume,configData,&_firstGuessLat,&_firstGuessLon);
            gridData->writeAsi();
            emit log(Message("Done with Cappi",15,this->objectName()));
            emit newCappi(*gridData);
        if(abort) break;
            //STEP 5: simplex to find a new center
            emit log(Message("Finding center",1,this->objectName()));
            VortexData vortexData;
            vortexData.setTime(newVolume->getDateTime());
            SimplexThread* pSimplex=new SimplexThread();
            pSimplex->initParam(configData,gridData,_firstGuessLat,_firstGuessLon);
            pSimplex->findCenter(&_simplexList);
            delete pSimplex;
            _simplexList.last().setTime(vortexData.getTime());

            //Postprocess simplex result
            int convergedRings = 0;
            for (int ridx=0;ridx<_simplexList.last().getNumRadii();ridx++)
                if (_simplexList.last().getNumConvergingCenters(0,ridx) > 0)
                    convergedRings++;
            // Go with at least a third for now
            if (convergedRings >= _simplexList.last().getNumRadii()/3) {
                _simplexList.timeSort();
                ChooseCenter *centerFinder = new ChooseCenter(configData,&_simplexList,&vortexData);
                centerFinder->findCenter();
                delete centerFinder;
                if(GriddedData::getCartesianDistance(_firstGuessLat,_firstGuessLon,vortexData.getLat(0),vortexData.getLon(0))>50.0f){
                    Message newMsg(QString(),5,this->objectName(),
                                   Yellow,"Center Not Found");
                    emit log(newMsg);
                }
                else{
                    Message newMsg(QString(),5,this->objectName(),
                                   Green,"Center Found");
                    emit log(newMsg);
                }
            } else {
                for(int ll=0;ll<vortexData.getMaxLevels();ll++){
                    vortexData.setLat(ll,_firstGuessLat);
                    vortexData.setLon(ll,_firstGuessLon);
                    vortexData.setHeight(ll, ll+1);
                    Message newMsg(QString(),5,this->objectName(),
                                   Yellow,"Center Not Found");
                    emit log(newMsg);
                }
            }
        if(abort) break;
            float simplexLat = vortexData.getLat(0);
            float simplexLon = vortexData.getLon(0);
            QDomElement radar = configData->getConfig("radar");
            QDomElement simplex = configData->getConfig("center");
            QDomElement vtd   = configData->getConfig("vtd");
            float radarLat = configData->getParam(radar,"lat").toFloat();
            float radarLon = configData->getParam(radar,"lon").toFloat();
            float* xyValues = gridData->getCartesianPoint(&radarLat, &radarLon, &simplexLat, &simplexLon);
            float xPercent = float(gridData->getIndexFromCartesianPointI(xyValues[0])+1)/gridData->getIdim();
            float yPercent = float(gridData->getIndexFromCartesianPointJ(xyValues[1])+1)/gridData->getJdim();
            float rmwEstimate = vortexData.getRMW(0)/(gridData->getIGridsp()*gridData->getIdim());
            float sMin = configData->getParam(simplex, "innerradius").toFloat()/(gridData->getIGridsp()*gridData->getIdim());
            float sMax = configData->getParam(simplex, "outerradius").toFloat()/(gridData->getIGridsp()*gridData->getIdim());
            float vMax = configData->getParam(vtd, "outerradius").toFloat()/(gridData->getIGridsp()*gridData->getIdim());
            emit newCappiInfo(xPercent, yPercent, rmwEstimate, sMin, sMax, vMax, _firstGuessLat, _firstGuessLon, simplexLat, simplexLon);
            delete [] xyValues;
        if(abort) break;
            //STEP 6: Check for new pressure data to process for the current volume
            if(pressureSource->hasUnprocessedData()) {
                // Create a list of new pressure observations that have not yet been processed
                QList<PressureData>* newObs = pressureSource->getUnprocessedData();
                // Add any new observations to the list of observations which are used to calculate the current pressure
                for (int i = newObs->size()-1;i>=0; i--) {
                    bool match = false;
                    for(int j = 0; (!match)&&(j < _pressureList.size()); j++) {
                        if(_pressureList.value(j)==newObs->value(i)) {
                            match = true;
                        }
                    }
                    if(!match) {
                        _pressureList.append(newObs->at(i));
                    }
                }
                delete newObs;
            }
        if(abort) break;
            //STEP 7: GBVTD to calculate the wind
            //if simplex algorithm successfully find the center, then perform the GBVTD
            emit log(Message("Estimating pressure",1,this->objectName()));
            VortexThread* pVtd=new VortexThread();
            if (mode == "operational") {
                pVtd->setEnvPressure(atcf->getEnvPressure());
                pVtd->setOuterRadius(atcf->getOuterRadius());
            }
            pVtd->getWinds(configData,gridData,newVolume,&vortexData,&_pressureList);
            delete pVtd;
            if (vortexData.getMaxValidRadius() != -999) {
                _vortexList.append(vortexData);
            } else  {
                QString status = "No Central Pressure Estimate at " + vortexData.getTime().toString("hh:mm");
                Message newMsg(status,0,this->objectName(),Yellow,"Pressure Not Found");
                emit log(newMsg);
            }
            checkIntensification();
        if(abort) break;
            //STEP 8: finish a round of analysis, clear up
            emit vortexListUpdate(&_vortexList);
            emit log(Message(QString("Completed Analysis On Volume "+newVolume->getFileName()),100,this->objectName()));
            delete newVolume;
            delete gridFactory;
            delete gridData;
        if(abort) break;
            //STEP 9: after finish process each volume,save data to XML
            // Print out summary information to log
			QString summary = "VORTRAC ATCF,";
			QString values;
			summary += vortexData.getTime().toString(Qt::ISODate) + ",";
			summary += values.setNum(vortexData.getLat()) + ",";
			summary += values.setNum(vortexData.getLon()) + ",";
			summary += values.setNum(vortexData.getPressure()) + ",";
			summary += values.setNum(vortexData.getPressureUncertainty()) + ",";
			summary += values.setNum(vortexData.getRMW()) + ",";
			summary += values.setNum(vortexData.getRMWUncertainty());
			emit log(Message(summary,0,this->objectName()));
            _vortexList.saveXML();
            _simplexList.saveXML();
            _pressureList.saveXML();
        }
        else{
            //if there's no data, have a little rest
            sleep(2);
        }
    }
    delete dataSource;
    delete pressureSource;
}

// This slot is used for log message relaying
// Any objects created by this object must be connected
// to this slot

void workThread::catchLog(const Message& message)
{
    emit log(message);
}


void workThread::catchVCP(const int vcp)
{
    emit newVCP(vcp);
}

void workThread::catchCappi(const GriddedData& cappi)
{
    emit newCappi(cappi);
}

void workThread::setOnlyRunOnce(const bool newRunOnce) {
    runOnce = newRunOnce;
}

void workThread::setContinuePreviousRun(const bool &decision)
{
    continuePreviousRun = decision;
}

void workThread::checkIntensification()
{
    // Checks for any rapid changes in pressure
    QDomElement pressure = configData->getConfig("pressure");
    // Units of mb/hr
    float rapidRate = configData->getParam(pressure, QString("rapidlimit")).toFloat();
    if(isnan(rapidRate)) {
        emit log(Message(QString("Could Not Find Rapid Intensification Rate, Using 3 mb/hr"),0,this->objectName()));
        rapidRate = 3.0;
    }

    // So we don't report falsely there must be a rapid increase trend which
    // spans several measurements Number of volumes which are averaged.
    int volSpan = configData->getParam(pressure, QString("av_interval")).toInt();
    if(isnan(volSpan)) {
        emit log(Message(QString("Could Not Find Pressure Averaging Interval for Rapid Intensification, Using 8 volumes"),0,this->objectName()));
        volSpan = 8;
    }

    int lastVol = _vortexList.count()-1;
    if(lastVol > 2*volSpan) {
        if(_vortexList.at(int(volSpan/2.)).getTime().secsTo(_vortexList.at(lastVol-int(volSpan/2.)).getTime()) > 3600) {
            float recentAv = 0;
            float pastAv = 0;

            //get average pressure of last volSpan record
            int recentCount = 0;
            for(int k = lastVol; k > lastVol-volSpan; k--) {
                if(_vortexList.at(k).getPressure()==-999)
                    continue;
                recentAv+=_vortexList.at(k).getPressure();
                recentCount++;
            }
            recentAv /= (recentCount*1.);

            //get the past pressure average
            int pastCenter = 0;
            int pastCount = 0;
            float timeSpan = _vortexList.at(lastVol-volSpan).getTime().secsTo(_vortexList.at(lastVol).getTime());
            QDateTime pastTime = _vortexList.at(lastVol).getTime().addSecs(-1*int(timeSpan/2+3600));
            for(int k = 0; k < lastVol; k++) {
                if(_vortexList.at(k).getPressure()==-999)
                    continue;
                if(_vortexList.at(k).getTime() <= pastTime)
                    pastCenter = k;
            }
            if(pastCenter-int(volSpan/2) >=0 ){
                for(int j = pastCenter-int(volSpan/2.);(j < pastCenter+int(volSpan/2.))&&(j<lastVol); j++) {
                    if(_vortexList.at(j).getPressure()==-999)
                        continue;
                    pastAv+= _vortexList.at(j).getPressure();
                    pastCount++;
                }
                pastAv /= (pastCount*1.);

                if(recentAv - pastAv > rapidRate) {
                    emit(log(Message(QString("Rapid Increase in Storm Central Pressure Reported @ Rate of "+QString().setNum(recentAv-pastAv)+" mb/hour"), 0,this->objectName(), Green, QString(), RapidIncrease, QString("Storm Pressure Rising"))));
                } else {
                    if(recentAv - pastAv < -1.0*rapidRate) {
                        emit(log(Message(QString("Rapid Decline in Storm Central Pressure Reporting @ Rate of "+QString().setNum(recentAv-pastAv)+" mb/hour"), 0, this->objectName(), Green, QString(), RapidDecrease, QString("Storm Pressure Falling"))));
                    }
                    else {
                        emit(log(Message(QString("Storm Central Pressure Stablized"), 0, this->objectName(),Green,QString(), Ok, QString())));
                    }
                }
            }
        }
    }
}


void workThread::checkListConsistency()
{
    if(_vortexList.count()!=_simplexList.count()) {
        emit log(Message(QString("Storage Lists Reloaded With Mismatching Volume Entries"),0,this->objectName()));
    }

    for(int vv = _vortexList.count()-1; vv >= 0; vv--) {
        bool foundMatch = false;
        for(int ss = 0; ss < _simplexList.count(); ss++) {
            if(_vortexList.at(vv).getTime()==_simplexList.at(ss).getTime())
                foundMatch = true;
        }
        if(!foundMatch) {
            emit log(Message(QString("Removing Vortex Entry @ "+_vortexList.at(vv).getTime().toString(Qt::ISODate)+" because no matching simplex was found"),0,this->objectName()));
            _vortexList.removeAt(vv);
        }
    }

    for(int ss = _simplexList.count()-1; ss >= 0; ss--) {
        bool foundMatch = false;
        for(int vv = 0; vv < _vortexList.count(); vv++) {
            if(_simplexList.at(ss).getTime() == _vortexList.at(vv).getTime())
                foundMatch = true;
        }
        if(!foundMatch) {
            emit log(Message(QString("Removing Simplex Entry @ "+_simplexList.at(ss).getTime().toString(Qt::ISODate)+" Because No Matching Vortex Was Found"),0,this->objectName()));
            _simplexList.removeAt(ss);
        }
    }
    // Removing the last ones for safety, any partially formed file could do serious damage
    // to data integrity
    _simplexList.removeAt(_simplexList.count()-1);
    _simplexList.saveXML();
    _vortexList.removeAt(_vortexList.count()-1);
    _vortexList.saveXML();
}

void workThread::catchCappiInfo(float x,float y,float rmwEstimate,float sMin,float sMax,float vMax,
                                float userLat,float userLon,float lat,float lon)
{
    emit newCappiInfo(x,y,rmwEstimate,sMin,sMax,vMax,userLat,userLon,lat,lon);
}

void workThread::_latlonFirstGuess(RadarData* radarVolume)
{
    QString mode = configData->getParam(configData->getConfig("vortex"),"mode");
    QDateTime volDateTime = radarVolume->getDateTime();
    
    if (mode == "manual") {        
        float stormSpd = configData->getParam(configData->getConfig("vortex"),"speed").toFloat();
        float stormDir = configData->getParam(configData->getConfig("vortex"),"direction").toFloat();
        stormDir = 450.0f-stormDir;
        if(stormDir > 360.0f)
            stormDir -=360.0f;
        stormDir*=acos(-1.0f)/180.f;
        
        //calucate the expolation from user define center
        // Get initial lat and lon
        float initLat = configData->getParam(configData->getConfig("vortex"),"lat").toFloat();
        float initLon = configData->getParam(configData->getConfig("vortex"),"lon").toFloat();
        QDate obsDate = QDate::fromString(configData->getParam(configData->getConfig("vortex"),"obsdate"),"yyyy-MM-dd");
        QTime obsTime = QTime::fromString(configData->getParam(configData->getConfig("vortex"),"obstime"),"hh:mm:ss");
        QDateTime usrDateTime = QDateTime(obsDate, obsTime, Qt::UTC);
        int elapsedSeconds =usrDateTime.secsTo(volDateTime);
        
        float distanceMoved = elapsedSeconds*stormSpd/1000.0;
        float changeInX = distanceMoved*cos(stormDir);
        float changeInY = distanceMoved*sin(stormDir);
        float *extrapLatLon = GriddedData::getAdjustedLatLon(initLat,initLon, changeInX, changeInY);
        _firstGuessLat=extrapLatLon[0];
        _firstGuessLon=extrapLatLon[1];
        
        //if there is a vortex result,try to extrapolation from this record
        if (!_vortexList.isEmpty()) {
            _vortexList.timeSort();
            float vortexLat = _vortexList.last().getLat();
            float vortexLon = _vortexList.last().getLon();
            QDateTime obsDateTime = _vortexList.last().getTime();
            
            int elapsedSeconds =obsDateTime.secsTo(volDateTime);
            float distanceMoved = elapsedSeconds*stormSpd/1000.0;
            float changeInX = distanceMoved*cos(stormDir);
            float changeInY = distanceMoved*sin(stormDir);
            
            float *newLatLon = GriddedData::getAdjustedLatLon(vortexLat,vortexLon, changeInX, changeInY);
            float relDist = GriddedData::getCartesianDistance(extrapLatLon[0],extrapLatLon[1],newLatLon[0],newLatLon[1]);
            
            if (relDist < 10 || usrDateTime.secsTo(volDateTime)>60*60) {
                _firstGuessLat = newLatLon[0];
                _firstGuessLon = newLatLon[1];
                //std::cout<<"Using estimation of center ("<<_firstGuessLat<<","<<_firstGuessLon<<") from last vortex"<<std::endl;
            }
            delete [] newLatLon;

        }
        delete [] extrapLatLon;

    } else if (mode == "operational") {
        _firstGuessLat = atcf->getLatitude(volDateTime);
        _firstGuessLon = atcf->getLongitude(volDateTime);
    }
}
