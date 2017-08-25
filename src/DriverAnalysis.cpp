/*
 * DriverAnalysis.cpp
 * VORTRAC
 *
 * Created by Annette Foerster in 2012
 * Copyright 2005 University Corporation for Atmospheric Research.
 * All rights reserved.
 *
 *
 */

#include "DriverAnalysis.h"

DriverAnalysis::DriverAnalysis(QWidget *parent)
    :QWidget(parent)
{
}

void DriverAnalysis::saveLog()
{
    statusLog->saveLogFile();
}

void DriverAnalysis::catchLog(const Message& message)
{
    emit log(message);
}

void DriverAnalysis::pollVortexUpdate(VortexList* list)
{
    if (list==NULL) {
        currPressure->setText(QString().setNum(0));
        currRMW->setText(QString().setNum(0));
        currDeficit->setText(QString().setNum(0));
	currMaxWind->setText(QString().setNum(0));
        deficitLabel->setText(tr("Pressure Deficit Unavailable"));
        emit vortexListChanged(NULL);
	return;
    }

    if(list->count() > 0) {
        int currRow = historyTable->rowCount();
	QString currTime = list->last().getTime().toString("dd/hh:mm");
	if (currRow > 0) {
	  QString lastTime = historyTable->item(currRow-1,0)->text();
	  if (currTime == lastTime) {
	    // No new analysis
	    return;
	  }
	}
	historyTable->insertRow(currRow);
	QTableWidgetItem *time= new QTableWidgetItem(currTime);
	time->setFlags(Qt::ItemIsSelectable);
	historyTable->setItem(currRow, 0, time);
		
        // Find the outermost vtd mean wind coefficient that is not equal to -999
        float maxRadius = list->last().getMaxValidRadiusnm();
        currDeficit->setText(QString().setNum(list->last().getPressureDeficit(), 'f', 0));
        deficitLabel->setText(tr("Pressure Deficit From ")+QString().setNum(maxRadius, 'f', 0)+tr(" nm (mb):"));

	int bestLevel = list->last().getBestLevel();
	QTableWidgetItem *lat = new QTableWidgetItem(QString().setNum(list->last().getLat(bestLevel),'f', 2));
	lat->setFlags(Qt::ItemIsSelectable);
	historyTable->setItem(currRow, 1, lat);
		
	QTableWidgetItem *lon = new QTableWidgetItem(QString().setNum(list->last().getLon(bestLevel),'f', 2));
	lon->setFlags(Qt::ItemIsSelectable);
	historyTable->setItem(currRow, 2, lon);
				
        currPressure->setText(QString().setNum((int)list->last().getPressure()));
	QTableWidgetItem *pressure = new QTableWidgetItem(QString().setNum((int)list->last().getPressure()));
	historyTable->setItem(currRow, 3, pressure);
		
        if(list->last().getAveRMW()==-999.0) {
            currRMW->setText(QString().setNum(0));
	    QTableWidgetItem *rmw= new QTableWidgetItem(QString().setNum(0));
	    rmw->setFlags(Qt::ItemIsSelectable);
	    historyTable->setItem(currRow, 4, rmw);
	} else {
	  currRMW->setText(QString().setNum(list->last().getAveRMWnm(), 'f', 0));
	  QTableWidgetItem *rmw= new QTableWidgetItem(QString().setNum(list->last().getAveRMWnm(), 'f', 0));
	  rmw->setFlags(Qt::ItemIsSelectable);
	  historyTable->setItem(currRow, 4, rmw);
	}
		
	QTableWidgetItem *appwind = new QTableWidgetItem(QString().setNum(cappiDisplay->getMaxApp(),'f', 1));
	appwind->setFlags(Qt::ItemIsSelectable);
	historyTable->setItem(currRow, 5, appwind);
		
	QTableWidgetItem *recwind = new QTableWidgetItem(QString().setNum(cappiDisplay->getMaxRec(),'f', 1));
	recwind->setFlags(Qt::ItemIsSelectable);
	historyTable->setItem(currRow, 6, recwind);
		
	currMaxWind->setText(QString().setNum(list->last().getMaxSfcWind(), 'f', 0));
	QTableWidgetItem *maxwind = new QTableWidgetItem(QString().setNum(list->last().getMaxSfcWind(), 'f', 0));
	historyTable->setItem(currRow, 7, maxwind);
		
        emit vortexListChanged(list);
    } else {
        currPressure->setText(QString().setNum(0));
        currRMW->setText(QString().setNum(0));
        currDeficit->setText(QString().setNum(0));
        deficitLabel->setText(tr("Pressure Deficit Unavailable"));
	currMaxWind->setText(QString().setNum(0));
        emit vortexListChanged(NULL);
    }
}

void DriverAnalysis::updateCappiDisplay(bool hasImage)
{
    if(hasImage) {
        QChar deg(0x00B0);
        QString vel = QString().setNum(cappiDisplay->getMaxApp(),'f', 1);
        QString loc = "at " + QString().setNum(cappiDisplay->getMaxAppHeight()*3280.8399,'f', 1) + " feet alt, " +
        QString().setNum(cappiDisplay->getMaxAppDir(),'f', 1) + deg + ", " +
        QString().setNum(cappiDisplay->getMaxAppDist()*0.5399568,'f', 1) + " nm range";
        appMaxWind->setText(vel);
        appMaxLabel2->setText(loc);
		
        QString msg = "Maximum inbound velocity of " + vel + " kts " + loc + " (";
        vel = QString().setNum(cappiDisplay->getMaxApp()/1.9438445,'f', 1);
        loc = "at " + QString().setNum(cappiDisplay->getMaxAppHeight(),'f', 1) + " km alt, " +
        QString().setNum(cappiDisplay->getMaxAppDir(),'f', 1) + deg + ", " +
        QString().setNum(cappiDisplay->getMaxAppDist(),'f', 1) + " km range";
        msg += vel + " m/s " + loc + " )";
        emit log(Message(msg,0,this->objectName()));

        vel = QString().setNum(cappiDisplay->getMaxRec(),'f', 1);
        loc = "at " + QString().setNum(cappiDisplay->getMaxRecHeight()*3280.8399,'f', 1) + " feet alt, " +
        QString().setNum(cappiDisplay->getMaxRecDir(),'f', 1) + deg + ", " +
        QString().setNum(cappiDisplay->getMaxRecDist()*0.5399568,'f', 1) + " nm range";
        recMaxWind->setText(vel);
        recMaxLabel2->setText(loc);
        msg = "Maximum outbound velocity of " + vel + " kts " + loc + " (";
        vel = QString().setNum(cappiDisplay->getMaxRec()/1.9438445,'f', 1);
        loc = "at " + QString().setNum(cappiDisplay->getMaxRecHeight(),'f', 1) + " km alt, " +
            QString().setNum(cappiDisplay->getMaxRecDir(),'f', 1) + deg + ", " +
            QString().setNum(cappiDisplay->getMaxRecDist(),'f', 1) + " km range";
        msg += vel + " kts " + loc + " )";
        emit log(Message(msg,0,this->objectName()));
		
    }
    else {
        appMaxWind->setText(QString().setNum(0));
        recMaxWind->setText(QString().setNum(0));
        appMaxLabel2->setText(QString());
        recMaxLabel2->setText(QString());
    }

}

void DriverAnalysis::updateCappiInfo(float x, float y, float rmwEstimate, float sMin, float sMax, float vMax,
                                   float userCenterLat, float userCenterLon, float centerLat, float centerLon)
{
    lcdCenterLat->setText(QString().setNum(centerLat,'f', 2));
    lcdCenterLon->setText(QString().setNum(centerLon,'f', 2));
    lcdUserCenterLat->setText(QString().setNum(userCenterLat,'f', 2));
    lcdUserCenterLon->setText(QString().setNum(userCenterLon,'f', 2));
    emit updateMadis(userCenterLat, userCenterLon);
}
