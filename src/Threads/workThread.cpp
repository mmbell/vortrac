/*
*  workThread.cpp
*  VORTRAC
*
*  Created by Michael Bell on 7/25/05.
*  Copyright 2005 University Corporation for Atmospheric Research. All rights reserved.
*
*/

#include <fstream>
#include <QtGui>
#include "workThread.h"
#include "Message.h"
#include <math.h>
#include "Radar/RadarQC.h"
#include <unistd.h>
#include "DataObjects/SimplexList.h"

workThread::workThread(QObject *parent)
	: QObject(parent)
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
	//    stop();
	//    this->quit();
}

void workThread::stop()
{
	abort = true;
}

void workThread::run()
{
	std::cout << "Running workThread ...\n";
	
	//Initialize configuration
	bool preGridded = "true" == configData->getParam(configData->getConfig("radar"),
							 "pre_gridded");

	float bottomLevel = configData->getParam(configData->getConfig("center"), "bottomlevel").toFloat();

	// Load vortex centers if the config file specifies a path
	loadCenterLocations(configData->getParam(configData->getConfig("vortex"), "centers"));

	QString mode = configData->getParam(configData->getConfig("vortex"), "mode");
	QDir workingDir(configData->getParam(configData->getConfig("vortex"),"dir"));
	QString vortexName = configData->getParam(configData->getConfig("vortex"), "name");
	
	if (vortexName == "Unknown") {
		// Problem with ATCF data
		Message newMsg(QString("Vortex name is unknown, please check ATCF data feed"),-1,this->objectName(),
		Red,"ATCF Error");
		emit log(newMsg);
	}
	QDomElement radar = configData->getConfig("radar");
	float radarLat = configData->getParam(radar,"lat").toFloat();
	float radarLon = configData->getParam(radar,"lon").toFloat();
	QString radarName = configData->getParam(radar, "name");
	QString year = QString().setNum(QDate::fromString(configData->getParam(configData->getConfig("radar"),
									     "startdate"), "yyyy-MM-dd").year());
	QString namePrefix = vortexName + "_" + radarName + "_" + year + "_";

	//initialize the saving path of data-list
	_simplexList.setFilePath(workingDir.filePath(namePrefix+"simplexlist.xml"));
	_vortexList.setFilePath(workingDir.filePath(namePrefix+"vortexlist.xml"));
	_pressureList.setFilePath(workingDir.filePath(namePrefix+"pressurelist.xml"));

	if(continuePreviousRun){
		_simplexList.restore();
		_vortexList.restore();
		_pressureList.restore();
	}

	// where to save coefficients.
	QString coeffFilePath = workingDir.filePath(namePrefix + "coefficientlist.csv");
	std::ofstream outfile(coeffFilePath.toLatin1().data());
	// Put a comment with column headers
	outfile << "# level, radius, param, value" << std::endl;
	outfile.close();

	//create data monitor object
	dataSource = new RadarFactory(configData);
	connect(dataSource, SIGNAL(log(const Message&)),this, SLOT(catchLog(const Message&)));
	PressureFactory *pressureSource = new PressureFactory(configData);
	connect(pressureSource, SIGNAL(log(const Message&)),this, SLOT(catchLog(const Message&)));

	// Flag to just construct the cappi.
	// Useful if all you want to do is look at the radar data on the display

	bool just_display = "true" == configData->getParam(configData->getConfig("cappi"),
							 "just_display");
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
				continue;
			}			
			
			emit log(Message("Found file:" + newVolume->getFileName(), -1, this->objectName()));
			
			// Check to makes sure that the file still exists and is readable
			if((!newVolume->fileIsReadable()) or (!newVolume->readVolume())) {
			  emit log(Message(QString("The radar data file " + newVolume->getFileName() +
						   " is not readable"), -1, this->objectName()));
			  delete newVolume;
			  continue;
			}
			std::cout << newVolume->getDateTimeString().toStdString() << ": ";

			// TODO what do we do with that? not needed, will it break anything "volume coverage pattern"
			emit newVCP(newVolume->getVCP());

			GriddedFactory *gridFactory = new GriddedFactory();
			GriddedData *gridData;

			if (preGridded) {
			  
			  gridData = gridFactory->fillPreGriddedData(newVolume, configData);
			  newVolume->setPreGridded();

			  // See if the config wants to overwrite the default max unambiguated range
			  QDomElement n = configData->getConfig("radar").firstChildElement("max_unambig_range");
			  if (! n.isNull() ) {
			    float maxRange = n.text().toFloat();
			    newVolume->setMaxRange(maxRange);
			  }
			  
			  //STEP 3: get the first guess of center Lat,Lon for simplex
			  _latlonFirstGuess(newVolume);
			  QString currentCenter("Processing radar volume at "
						+ newVolume->getDateTime().toString("hh:mm")
						+ " with (" + QString().setNum(_firstGuessLat)
						+ ", "+QString().setNum(_firstGuessLon)+") center estimate");
			  emit log(Message(currentCenter,1,this->objectName()));
			  if(abort) break;
			} else {
			  
			  //radar data quality control
			  RadarQC* dealiaser=new RadarQC(newVolume);
			  connect(dealiaser,SIGNAL(log(const Message&)),
				  this,SLOT(catchLog(const Message&)));
			  dealiaser->getConfig(configData->getConfig("qc"));
			  dealiaser->dealias();
			  emit log(Message("Finished QC and Dealiasing",10, this->objectName()));
			  delete dealiaser;
			  if(abort) {
			    delete newVolume;
			    break;
			  }
			
			  //STEP 3: get the first guess of center Lat,Lon for simplex

			  _latlonFirstGuess(newVolume);
			  QString currentCenter("Processing radar volume at "
						+ newVolume->getDateTime().toString("hh:mm") + " with ("
						+ QString().setNum(_firstGuessLat)+ ", "
						+ QString().setNum(_firstGuessLon)+") center estimate");
			  emit log(Message(currentCenter,1,this->objectName()));
			  if(abort) break;

			  //STEP 4: from Radardata ---> Griddata, make cappi
			  gridData=gridFactory->makeCappi(newVolume,configData,&_firstGuessLat,&_firstGuessLon);
			}

			gridData->writeAsi();
			emit log(Message("Done with Cappi", 15, this->objectName()));
			emit newCappi(*gridData);

			if(abort) {
			  delete newVolume;
			  delete gridFactory;
			  delete gridData;
			  break;
			}

			if(just_display) {
			  // sleep 3 seconds to give the user a chance to click around
			  sleep(3);	
			  continue;
			}
			
			//STEP 5: simplex to find a new center
			emit log(Message("Finding center",1,this->objectName()));

			VortexData *vortexData = new VortexData();
			vortexData->setTime(newVolume->getDateTime());
			
			std::cout << "Vortex time: " << newVolume->getDateTime().toString("hh:mm").toLatin1().data() << std::endl;
			
			SimplexThread* pSimplex = new SimplexThread();
			pSimplex->initParam(configData, gridData, _firstGuessLat, _firstGuessLon);
			
			// TODO this does the work.
			// We get "Center Not Found" if we pick a center bottomLevel too low in the config file.
			
			pSimplex->findCenter(&_simplexList);  // TODO check the return value!
			delete pSimplex;
			_simplexList.last().setTime(vortexData->getTime());

			//Postprocess simplex result

			// TODO
			// This only checks if there are converged centers at level 0.
			// Should we check which level has the more convergence and use that?
			// Or iterate on each level until we find an appropriate center?
			// Or ??
#if 0			
			for (int ridx = 0; ridx < _simplexList.last().getNumRadii(); ridx++)
			  if (_simplexList.last().getNumConvergingCenters(0, ridx) > 0)  // TODO: Why level 0?
					convergedRings++;
			int convergedRings = 0;
#endif
			int maxConverged = 0;
			int maxConvergedLevel = -1;
			int bestLevel = -1;
			
			// TODO involve the simplex std deviation into this chosing of a level

			int *numConvRings = new int[_simplexList.last().getNumLevels()]();
			
			for (int level = 0; level < _simplexList.last().getNumLevels(); level++) {
			  int convergedThisLevel = 0;
			  for (int ridx = 0; ridx < _simplexList.last().getNumRadii(); ridx++)
			    if (_simplexList.last().getNumConvergingCenters(level, ridx) > 0)
			      convergedThisLevel++;
			  numConvRings[level] = convergedThisLevel;
			  if (convergedThisLevel > maxConverged) {
			    maxConverged = convergedThisLevel;
			    maxConvergedLevel = level;
			  }
			}

			if (maxConvergedLevel > -1) {
				_simplexList.timeSort();

				ChooseCenter *centerFinder = new ChooseCenter(configData, &_simplexList, vortexData);
				centerFinder->findCenter(maxConvergedLevel);
				delete centerFinder;
				
				// Find the best std dev among all the levels that have enough converged rings.

				float bestStdDev = 9999;
				float threashold = _simplexList.last().getNumRadii() / 3; // Go with at least a third for now
			
				for (int level = 0; level < _simplexList.last().getNumLevels(); level++) {
				  if (numConvRings[level] < threashold)
				    continue;
				  float dev = vortexData->getCenterStdDev(level);
#if 0				  
				  if(dev > 0) // debug
				    std::cout << "level: " << level << ", stdDev: " << dev << std::endl;
#endif				  
				  if( (dev > 0) && (dev < bestStdDev) ) {
				    bestLevel = level;
				    bestStdDev = dev;
				  }
				}
#if 0
				std::cout << "*** Best std dev: " << bestStdDev << ", at level " << bestLevel << std::endl;
#endif				
				if(bestLevel == -1)
				  bestLevel = maxConvergedLevel;

				float userDistance = GriddedData::getCartesianDistance(_firstGuessLat, _firstGuessLon,
										       vortexData->getLat(bestLevel),
										       vortexData->getLon(bestLevel));


				//  vortexData.getLat(0), vortexData.getLon(0));
				float range = GriddedData::getCartesianDistance(radarLat, radarLon,
										vortexData->getLat(bestLevel),
										vortexData->getLon(bestLevel));
				if( (userDistance > 25.0f) 
				   or (range > newVolume->getMaxUnambig_range() - 
				       configData->getParam(configData->getConfig("center"), "innerradius").toFloat())) {
					Message newMsg(QString(), 5, this->objectName(),
						       Yellow, "Center Not Found");
					emit log(newMsg);
					std::cout << "*** Center not found (center too far away)" << std::endl;
				}
				else {
					Message newMsg(QString(), 5, this->objectName(),
						       Green, "Center Found");
					emit log(newMsg);
					std::cout << "** New center Lat: " << vortexData->getLat(bestLevel)
						  << ", Lon: " << vortexData->getLon(bestLevel) << std::endl;
					QString values;
					QString result = "Position estimate " + values.setNum(vortexData->getLat(bestLevel));
					result += " N " + values.setNum(vortexData->getLon(bestLevel)) + " E";
					emit log(Message(result, 0, this->objectName()));
					if (vortexData->getRMW() != -999.) {
					  result = "RMW estimate " + values.setNum(vortexData->getRMW());
					  result += " +/- " + values.setNum(vortexData->getRMWUncertainty()) + " km";
					  emit log(Message(result,0,this->objectName()));
					} else {
						emit log(Message(QString("RMW not found"),0,this->objectName()));
					}
				}
			} else {  // Not enough converged rings
			  std::cout << "**** Center not found (not enough converged rings)" << std::endl;
			  
			  for(int ll = 0; ll < vortexData->getMaxLevels(); ll++){
			    vortexData->setLat(ll, _firstGuessLat);
			    vortexData->setLon(ll, _firstGuessLon);
			    vortexData->setHeight(ll, bottomLevel + ll * gridData->getKGridsp());
			    Message newMsg(QString(), 5, this->objectName(),
					   Yellow,"Center Not Found");
			    emit log(newMsg);
			  }
			}

			delete[] numConvRings;
			
			if(abort) {
				delete newVolume;
				delete gridFactory;
				delete gridData;
				break;
			}

			if (bestLevel < 0) {
			  std::cout << "Abandoning this volume since no converged rings found." << std::endl;
			  delete newVolume;
			  delete gridFactory;
			  delete gridData;
			  continue;
			}
			vortexData->setBestLevel(bestLevel);
			
			float simplexLat = vortexData->getLat(bestLevel);
			float simplexLon = vortexData->getLon(bestLevel);
			
			QDomElement simplex = configData->getConfig("center");
			QDomElement vtd   = configData->getConfig("vtd");
			
			float* xyValues = gridData->getCartesianPoint(&radarLat, &radarLon, &simplexLat, &simplexLon);
			float xPercent = float(gridData->getIndexFromCartesianPointI(xyValues[0])+1)/gridData->getIdim();
			float yPercent = float(gridData->getIndexFromCartesianPointJ(xyValues[1])+1)/gridData->getJdim();
			float rmwEstimate = vortexData->getRMW(bestLevel)/(gridData->getIGridsp()*gridData->getIdim());
			float sMin = configData->getParam(simplex, "innerradius").toFloat()/(gridData->getIGridsp()*gridData->getIdim());
			float sMax = configData->getParam(simplex, "outerradius").toFloat()/(gridData->getIGridsp()*gridData->getIdim());
			float vMax = configData->getParam(vtd, "outerradius").toFloat()/(gridData->getIGridsp()*gridData->getIdim());
			emit newCappiInfo(xPercent, yPercent, rmwEstimate, sMin, sMax, vMax, _firstGuessLat, _firstGuessLon, simplexLat, simplexLon);
			delete [] xyValues;

			if(abort) {
				delete newVolume;
				delete gridFactory;
				delete gridData;
				break;
			}
			
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
			
			if(abort) {
				delete newVolume;
				delete gridFactory;
				delete gridData;
				break;
			}
			
			//STEP 7: GBVTD to calculate the wind
			//        if simplex algorithm successfully find the center, then perform the GBVTD
			
			float range = GriddedData::getCartesianDistance(radarLat, radarLon,
									vortexData->getLat(bestLevel),
									vortexData->getLon(bestLevel));
			if (range < (newVolume->getMaxUnambig_range()
				     - configData->getParam(simplex, "innerradius").toFloat())) {
			  emit log(Message("Estimating pressure", 1, this->objectName()));

	            VortexThread* pVtd = new VortexThread();
		    
	            if (mode == "operational") {
	                pVtd->setEnvPressure(atcf->getEnvPressure());
	                pVtd->setOuterRadius(atcf->getOuterRadius());
	            }

		    pVtd->getWinds(configData, gridData, newVolume, vortexData, &_pressureList); // Runs the VortexThread
	            delete pVtd;
		    
		    if (vortexData->getMaxValidRadius() != -999) {
		      _vortexList.append(*vortexData);
		      QString values;
		      QString result = "Central Pressure estimate " + values.setNum(vortexData->getPressure());
		      result += " +/- " + values.setNum(vortexData->getPressureUncertainty()) + " hPa";
		      result += " at " + vortexData->getTime().toString("hh:mm");
	                emit log(Message(result,0,this->objectName()));
	                // Print out summary information to log
	                QString summary = "VORTRAC ATCF,";
			
			summary += vortexData->getTime().toString(Qt::ISODate) + ",";
	                summary += values.setNum(vortexData->getLat(bestLevel)) + ",";
	                summary += values.setNum(vortexData->getLon(bestLevel)) + ",";
	                summary += values.setNum(vortexData->getPressure()) + ",";
	                summary += values.setNum(vortexData->getPressureUncertainty()) + ",";
	                summary += values.setNum(vortexData->getRMW(bestLevel)) + ",";
	                summary += values.setNum(vortexData->getRMWUncertainty(bestLevel)) + ",";
			summary += values.setNum(vortexData->getMaxSfcWind());

	                emit log(Message(summary,0,this->objectName()));
	            } else  {
		      QString status = "No Central Pressure Estimate at " + vortexData->getTime().toString("hh:mm");
	                Message newMsg(status,0,this->objectName(),Yellow,"Pressure Not Found");
	                emit log(newMsg);
	            }
		    } else {
			  QString status = "No Central Pressure Estimate at " + vortexData->getTime().toString("hh:mm");
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
            _vortexList.saveXML();
            _simplexList.saveXML();
            _pressureList.saveXML();
	    vortexData->saveCoefficients(coeffFilePath);
        } else {
            //if there's no data, have a little rest
            sleep(2);  
            //if in batch mode, abort
            if (this->parent()){
				std::cout<<"Finished processing all files in batch mode\n";
	            abort = true;
	            emit finished();
            }
        }
		
	} // while ! abort
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

void workThread::catchCappiInfo(float x, float y, float rmwEstimate, float sMin, float sMax, float vMax,
				float userLat, float userLon, float lat, float lon)
{
  emit newCappiInfo(x, y, rmwEstimate, sMin, sMax, vMax, userLat ,userLon, lat, lon);
}

// load content of center location file into a hash table

void workThread::loadCenterLocations(QString fName)
{
  if(fName == "")
    return;
  QFile file(fName);
  if (! file.open(QIODevice::ReadOnly)) {
    std::cout << "Error opening center file." << fName.toLatin1().data() << std::endl;
    return;
  }

  QDateTime time;

  while (! file.atEnd()) {
    QString line = file.readLine();
    QStringList entries;
    for (QString item : line.split(',')) {
      entries.append(item);
    }
    int i = entries.size();
    if (i < 3) {
      std::cout << "Unsupported line format in " << fName.toLatin1().data()
		<< ": '" << line.toLatin1().data() << "'" << std::endl;
      continue;
    }
    float *loc = new float[2];
    time = QDateTime::fromString(entries[0], "yyyy-MM-dd:hh:mm");
    loc[0] = entries[1].toFloat();
    loc[1] = entries[2].toFloat();
    qint64 key = time.toMSecsSinceEpoch();
    centerLocations.insert(key, loc);
#if 0
    std::cout << time.toString("yyyy-MM-dd:hh:mm:ss").toLatin1().data()
	      << " key: " << key << ", lat: " << loc[0] << ", lon: " << loc[1] << std::endl;
#endif
  }
}

void workThread::_latlonFirstGuess(RadarData* radarVolume)
{
  QString mode = configData->getParam(configData->getConfig("vortex"),"mode");
  QDateTime volDateTime = radarVolume->getDateTime();

  if (mode == "operational") {
    _firstGuessLat = atcf->getLatitude(volDateTime);
    _firstGuessLon = atcf->getLongitude(volDateTime);
    return;
  }

  if (mode != "manual") {
    std::cout << "Unknown/unsuported vortex mode in config file: '" << mode.toLatin1().data()
	      << "'. Defaulting to 'manual'" << std::endl;
  }

  // Check if the center at this time was loaded from the <centers> file
  // Chop off msecs
  
  QDateTime volDT = QDateTime::fromString(radarVolume->getDateTime().toString("yyyy-MM-dd:hh:mm"), ("yyyy-MM-dd:hh:mm"));
  qint64 key = volDT.toMSecsSinceEpoch();
#if 0
  std::cout << "** " << volDT.toString("yyyy-MM-dd:hh:mm:ss").toLatin1().data()
    	    << " key: " << key << std::endl;
#endif
  
  if(centerLocations.contains(key)) {
    float *loc = centerLocations.value(key);
    _firstGuessLat = loc[0];
    _firstGuessLon = loc[1];
    std::cout << "Using saved center as first guess. key: " << key << ", " << loc[0] << ", " << loc[1] << std::endl;
    return;
  }

  // This assumes that the storm speed and direction are somewhat correct in the config file.
  // If set to 0, this will end up being the Lat and Lon specified in the config.
  
  float stormSpd = configData->getParam(configData->getConfig("vortex"),"speed").toFloat();
  float stormDir = configData->getParam(configData->getConfig("vortex"),"direction").toFloat();
  stormDir = 450.0f - stormDir;
  if(stormDir > 360.0f)
    stormDir -= 360.0f;
  stormDir *= acos(-1.0f) / 180.f;
        
  //calculate the expolation from user define center
  
  // Get initial lat and lon
  float initLat = configData->getParam(configData->getConfig("vortex"),"lat").toFloat();
  float initLon = configData->getParam(configData->getConfig("vortex"),"lon").toFloat();
  QDate obsDate = QDate::fromString(configData->getParam(configData->getConfig("vortex"),"obsdate"),"yyyy-MM-dd");
  QTime obsTime = QTime::fromString(configData->getParam(configData->getConfig("vortex"),"obstime"),"hh:mm:ss");
  QDateTime usrDateTime = QDateTime(obsDate, obsTime, Qt::UTC);
  int elapsedSeconds = usrDateTime.secsTo(volDateTime);
        
  float distanceMoved = elapsedSeconds*stormSpd / 1000.0;
  float changeInX = distanceMoved * cos(stormDir);
  float changeInY = distanceMoved * sin(stormDir);
  float *extrapLatLon = GriddedData::getAdjustedLatLon(initLat, initLon, changeInX, changeInY);
  _firstGuessLat = extrapLatLon[0];
  _firstGuessLon = extrapLatLon[1];
        
  // if there is a vortex result,try to extrapolation from this record

  // TODO This is broken. It gives us a Lat and Lon outside of the grid.
  // The problem is that the vortexc Lat and Lon might not have been at level 0.
  // We need to add a "bestLevel" to the VortexData object.
  //
  // But the qestion remains: How can we get a center outside of the grid???
  // It seems like only level 0 has that problem.
  // That's because _simplexResults->last().getMeanY(k,bestRadii) could be -999
  //  ChooseCenter.cpp:978
  
  if (!_vortexList.isEmpty()) {
    _vortexList.timeSort();
    float vortexLat = _vortexList.last().getLat(_vortexList.last().getBestLevel());
    float vortexLon = _vortexList.last().getLon(_vortexList.last().getBestLevel());
    QDateTime obsDateTime = _vortexList.last().getTime();
            
    int elapsedSeconds = obsDateTime.secsTo(volDateTime);
    float distanceMoved = elapsedSeconds*stormSpd / 1000.0;
    float changeInX = distanceMoved*cos(stormDir);
    float changeInY = distanceMoved*sin(stormDir);
            
    float *newLatLon = GriddedData::getAdjustedLatLon(vortexLat, vortexLon, changeInX, changeInY);
    float relDist = GriddedData::getCartesianDistance(extrapLatLon[0], extrapLatLon[1], newLatLon[0], newLatLon[1]);
            
    if (relDist < 10 || usrDateTime.secsTo(volDateTime) > 60*60) {
      _firstGuessLat = newLatLon[0];
      _firstGuessLon = newLatLon[1];
      //std::cout<<"Using estimation of center ("<<_firstGuessLat<<","<<_firstGuessLon<<") from last vortex"<<std::endl;
    }
    delete [] newLatLon;

  }
  delete [] extrapLatLon;
}
