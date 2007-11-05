/*
 * ChooseCenter.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 5/30/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include "ChooseCenter.h"
#include <math.h>
#include <QDomElement>
#include <QHash>

ChooseCenter::ChooseCenter(Configuration* newConfig, 
			   const SimplexList* newList, VortexData* vortexPtr,
			   QObject *parent)
  :QObject(parent)
{
  velNull = -999.;
  config = newConfig;
  simplexResults = NULL;
  simplexResults = newList;
  vortexData = vortexPtr;
  score = NULL;
  bestRadius = NULL;
  centerDev = NULL;
  radiusDev = NULL;
  bestFitVariance = NULL;
  bestFitDegree = NULL;
  bestFitCoeff = NULL;
  newBestRadius = NULL;
  newBestCenter = NULL;
  numHeights = NULL;
  indexOfHeights = NULL;
  setObjectName("Center Chooser");
}

ChooseCenter::~ChooseCenter()
{
  config = NULL;
  delete config;
  vortexData = NULL;
  delete vortexData;
  
  if(score!=NULL) {
    for(int i = 0; i < simplexResults->count(); i++) {
      for(int j = 0; j < simplexResults->at(i).getNumRadii(); j++) {
	delete [] score[i][j];
      }
      delete [] score[i];
    }
    delete [] score;
  }
  else 
    delete score;

  // We might not be able to delete these depending on weither they 
  // are passed out for further use or not after the ChooseCenter object is
  // distroyed
  
  if(bestRadius!=NULL) {
    for(int i = 0; i < simplexResults->count(); i++) 
      delete[] bestRadius[i];
    delete[] bestRadius;   }
  else
    delete bestRadius;
  
  if(newBestRadius!=NULL) {
    for(int i = 0; i < simplexResults->count(); i++)
      delete[] newBestRadius[i];
    delete[] newBestRadius; }
  else
      delete newBestRadius;

  if(newBestCenter!=NULL) {
    for(int i = 0; i < simplexResults->count(); i++)
      delete[] newBestCenter[i];    
    delete[] newBestCenter;
  }
  else
    delete newBestCenter;
  if(bestFitCoeff!=NULL) {
    for(int m = 0; m < 4; m++) {
      for(int p = 0; p < numHeights->count(); p++) 
	delete[] bestFitCoeff[m][p];
      delete[] bestFitCoeff[m];
    }
    delete[] bestFitCoeff;
  }
  else
    delete bestFitCoeff;
      
  if(bestFitDegree!=NULL) {
    for(int m = 0; m < 4; m++) 
      delete[] bestFitDegree[m];
    delete[] bestFitDegree;
  }
  else 
    delete bestFitCoeff;

  if(bestFitVariance!=NULL) {
    for(int m = 0; m < 4; m++) 
      delete[] bestFitVariance[m];
    delete[] bestFitVariance;
  }
  if(centerDev!=NULL)
    delete[] centerDev;
  else 
    delete centerDev;
  if(radiusDev!=NULL)
    delete[] radiusDev;
  else
    delete radiusDev;
  if(numHeights!=NULL)
    numHeights->clear();
  if(indexOfHeights!=NULL) {
    for(int ii = indexOfHeights->count()-1; ii >= 0; ii++) {
      int* trash = indexOfHeights->take(ii);
      delete [] trash;
    }
    indexOfHeights->clear();
  }
  delete numHeights;
  delete indexOfHeights;
}

void ChooseCenter::setConfig(Configuration* newConfig)
{
  config = NULL;
  config = newConfig; 
}

void ChooseCenter::setSimplexList(const SimplexList* newList)
{
  simplexResults = NULL;
  simplexResults = newList;
}

bool ChooseCenter::findCenter()
{
  
   initialize();
   if(!chooseMeanCenters()) {
     emit errorlog(Message(QString("Failed in ChooseMeanCenters!"),
			   0,this->objectName(), Red,
			   QString("Data Issues Could Not Find Mean Center")));
     return false;
   }

   // Check to see if there are an adequate number of volumes within
   // the specified time range

   int goodVolumes = 0;
   for(int ii = 0; ii < simplexResults->count(); ii++) {
     if((simplexResults->at(ii).getTime() >= startTime) &&
	(simplexResults->at(ii).getTime() <= endTime)) 
       goodVolumes++;
   }
  
   if(goodVolumes > minVolumes) {
     emit errorlog(Message(QString("Using Polynomial Fit to Determine Best Center"),0,this->objectName()));
     if(constructPolynomial()) {
       if(!fixCenters()) {
	 QString fail("Failed to Find Polynomial Fit Center - Reverting To Mean");
	 emit errorlog(Message(fail,0,this->objectName(),Yellow,
			       QString("Center From Fit Not Found")));
	 useLastMean();
	 return true;
       }
       else {
	 emit errorlog(Message(QString(),0,this->objectName(),Green));
	 return true;
       }
     }
     else {
       QString fail("Failed to Find Polynomial Fit Center - Reverting To Mean");
       emit errorlog(Message(fail,0,this->objectName(),Yellow,
			     QString("Center From Fit Not Found")));
       useLastMean();
       return true;
     }
   }
   else {
     emit errorlog(Message(QString("Using Last Mean Center For Guess"),
			   0,this->objectName()));
     useLastMean();
   }
   return true;
}

void ChooseCenter::initialize()
{
  // Pulls all the necessary user parameters from the configuration panel
  //  and initializes the array used for fTesting

  QDomElement ccElement = config->getConfig("choosecenter");
  minVolumes = config->getParam(ccElement, QString("min_volumes")).toInt();
  windWeight = config->getParam(ccElement, QString("wind_weight")).toFloat();
  stdWeight = config->getParam(ccElement, QString("stddev_weight")).toFloat();
  ptsWeight = config->getParam(ccElement, QString("pts_weight")).toFloat();

  positionWeight = config->getParam(ccElement, QString("position_weight")).toFloat();
  rmwWeight = config->getParam(ccElement, QString("rmw_weight")).toFloat();
  
  velWeight = config->getParam(ccElement, QString("vt_weight")).toFloat();

  int fPercent = config->getParam(ccElement, QString("stats")).toInt();
  QDate sDate = QDate::fromString(config->getParam(ccElement, 
						   QString("startdate")),
				  Qt::ISODate);
  QDate eDate = QDate::fromString(config->getParam(ccElement, 
						   QString("enddate")),
				  Qt::ISODate);
  QTime sTime = QTime::fromString(config->getParam(ccElement, 
						   QString("starttime")),
				  Qt::ISODate);
  QTime eTime = QTime::fromString(config->getParam(ccElement,
						   QString("endtime")),
				  Qt::ISODate);
 
  startTime = QDateTime(sDate, sTime, Qt::UTC);
  endTime = QDateTime(eDate, eTime, Qt::UTC);

  if(fPercent == 99) {
    fCriteria[0] = 4052.2;  
    fCriteria[1] = 98.50;  
    fCriteria[2] = 34.12;  
    fCriteria[3] = 21.20;  
    fCriteria[4] = 16.26;  
    fCriteria[5] = 13.75;  
    fCriteria[6] = 12.25;  
    fCriteria[7] = 11.26;  
    fCriteria[8] = 10.56;  
    fCriteria[9] = 10.04;  
    fCriteria[10] = 9.65;  
    fCriteria[11] = 9.33;  
    fCriteria[12] = 9.07;  
    fCriteria[13] = 8.86;  
    fCriteria[14] = 8.68;  
    fCriteria[15] = 8.53;  
    fCriteria[16] = 8.40;  
    fCriteria[17] = 8.29;  
    fCriteria[18] = 8.18;  
    fCriteria[19] = 8.10;  
    fCriteria[20] = 8.02;  
    fCriteria[21] = 7.95;  
    fCriteria[22] = 7.88;  
    fCriteria[23] = 7.82;  
    fCriteria[24] = 7.77;  
    fCriteria[25] = 7.72;  
    fCriteria[26] = 7.68;  
    fCriteria[27] = 7.64;  
    fCriteria[28] = 7.60;  
    fCriteria[29] = 7.56;

  }
  else {
    // fPercent = 95 
    // these are our two options for now
    fCriteria[0] = 161.45;  
    fCriteria[1] = 18.513;  
    fCriteria[2] = 10.128;  
    fCriteria[3] = 7.7086;  
    fCriteria[4] = 6.6079;  
    fCriteria[5] = 5.9874;  
    fCriteria[6] = 5.5914;  
    fCriteria[7] = 5.3177;  
    fCriteria[8] = 5.1174;  
    fCriteria[9] = 4.9646;  
    fCriteria[10] = 4.8443;  
    fCriteria[11] = 4.7472;  
    fCriteria[12] = 4.6672;  
    fCriteria[13] = 4.6001;  
    fCriteria[14] = 4.5431;  
    fCriteria[15] = 4.4940;  
    fCriteria[16] = 4.4513;  
    fCriteria[17] = 4.4139;  
    fCriteria[18] = 4.3808;  
    fCriteria[19] = 4.3513;  
    fCriteria[20] = 4.3248;  
    fCriteria[21] = 4.3009;  
    fCriteria[22] = 4.2793;  
    fCriteria[23] = 4.2597;  
    fCriteria[24] = 4.2417;  
    fCriteria[25] = 4.2252;  
    fCriteria[26] = 4.2100;  
    fCriteria[27] = 4.1960;  
    fCriteria[28] = 4.1830;  
    fCriteria[29] = 4.1709;
  }
  
  bestRadius = new int*[simplexResults->count()];
  centerDev = new float[simplexResults->count()];
  radiusDev = new float[simplexResults->count()];
  score = new float**[simplexResults->count()];
  for(int i = 0; i < simplexResults->count(); i++) {
    //SimplexData currData = simplexResults->at(i);
    score[i] = new float*[simplexResults->at(i).getNumRadii()];
    bestRadius[i] = new int[simplexResults->at(i).getNumLevels()];
    for(int rad = 0; rad < simplexResults->at(i).getNumRadii(); rad++) {
      score[i][rad] = new float[simplexResults->at(i).getNumLevels()];
      for(int level = 0; level < simplexResults->at(i).getNumLevels(); level++){
	score[i][rad][level] = 0.0;
	bestRadius[i][level] = -1;
      }
    }
  }
}

bool ChooseCenter::chooseMeanCenters()
{
  /*
   * Determines the radius of maximum wind for each level of each volume used
   *  based on the scoring system based on averaged parameters from all the
   *  converging centers found in the simplex run
   *
   */

  if(simplexResults->isEmpty())
    return false;
  
  for(int i = 0; i < simplexResults->count(); i++) {
 
    // Now we zero out the variables used for finding the mean of each volume

    float radiusSum = 0;
    float xSum = 0; 
    float ySum = 0;
    for(int k = 0; k < simplexResults->at(i).getNumLevels(); k++) {
      //Message::toScreen("Simplex "+QString().setNum(i)+" @ level "+QString().setNum(k)+" has total Levels "+QString().setNum(simplexResults->at(i).getNumLevels()));
      int lastj = -1;
      float *winds = new float[simplexResults->at(i).getNumRadii()];
      float *stds = new float[simplexResults->at(i).getNumRadii()];
      float *pts = new float[simplexResults->at(i).getNumRadii()];
      float bestWind = 0.0;
      float bestStd = 50.;
      float bestPts = 0.;
      float ptRatio = (float)simplexResults->at(i).getNumPointsUsed()/2.718281828;

      for(int j = 0; j < simplexResults->at(i).getNumRadii(); j++) {
	
	// Examine each radius based on index j, for the one containing the 
	//   highest tangential winds
	
	winds[j] = velNull;
	stds[j] = velNull;
	pts[j] = velNull;
	if(simplexResults->at(i).getMaxVT(k,j)!=velNull) {
	  winds[j] = simplexResults->at(i).getMaxVT(k,j);
	  if(winds[j] > bestWind)
	    bestWind = winds[j];
	}
	if(simplexResults->at(i).getCenterStdDev(k,j)!=velNull) {
	  stds[j] = simplexResults->at(i).getCenterStdDev(k,j);
	  if(stds[j] < bestStd)
	    bestStd = stds[j];
	}
	if(simplexResults->at(i).getNumConvergingCenters(k,j)!=velNull) {
	  pts[j] = simplexResults->at(i).getNumConvergingCenters(k,j);
	  if(pts[j] > bestPts)
	    bestPts = pts[j];
	}
      }
      
      // Formerlly know as fix winds
      //   which was a sub routine in the perl version of this algorithm
      //   zeros all wind entrys that are not a local maxima, or adjacent 
      //   to a local maxima
      
      int count = 0;
      float *peakWinds = new float[simplexResults->at(i).getNumRadii()];
      float *peaks = new float[simplexResults->at(i).getNumRadii()];
      for(int z = 0; z < simplexResults->at(i).getNumRadii(); z++) {
	peakWinds[z] = 0;
	peaks[z] = 0;
      }
      float meanPeak,meanStd;
      
      for(int a = 1; a < simplexResults->at(i).getNumRadii()-1; a++) {
	if((winds[a] >= winds[a-1])&&(winds[a] >=winds[a+1])) {
	  peakWinds[count] = winds[a];
	  count++;
	  peaks[a] = 1;
	}
	else {
	  peaks[a]=0;
    	}
      }
      float windSum = 0;
      for(int a = 0; a < count; a++) {
	windSum += peakWinds[a];
      }
      if(count>0) {
	meanPeak = windSum/((float)count);
	meanStd = 0;
	for(int z = 0; z < count; z++) 
	  meanStd += (peakWinds[z]-meanPeak)*(peakWinds[z]-meanPeak);
	meanStd/=count;
      }
      else {
	meanStd = 0;
	//peakWinds = NULL;
	//peakWinds = winds;
      }
      
      delete [] peakWinds;
      
      // End of function formally known as fix winds
      // returned meanpeak, meanstd, peaks[array]
      
      for(int jj = 0; jj < simplexResults->at(i).getNumRadii(); jj++) {
	if(((jj > 0)&&(jj < simplexResults->at(i).getNumRadii()-1))
	   &&((peaks[jj] == 1)||(peaks[jj+1] == 1)||(peaks[jj-1] == 1))) {
	  winds[jj] = simplexResults->at(i).getMaxVT(k,jj);
	  // Keep an eye out for the maxima 
	  if(winds[jj] > bestWind) {
	    bestWind = winds[jj];
	  }
	}
	else {
	  winds[jj] = velNull;
	}
      }
      
      float tempBest = 0.0;
    
      for(int j = 0; j < simplexResults->at(i).getNumRadii(); j++) {
	score[i][j][k] = velNull;
	float windScore = 0.0;
	float stdScore = 0.0;
	float ptsScore = 0.0;
	//for(int w = 0; w <= #weightSchemes; w++) {
	// Mike says we only need to run this once
	// assign windWeight, stdWeight, ptsWeight, but not
	// distWeight cause we are droping it
	if((bestWind!=0.0)&&(winds[j]!=velNull))
	  windScore = exp(winds[j]-bestWind)*windWeight;
	if((stds[j]!=velNull)&&(stds[j]!=0.0))
	  stdScore = bestStd/stds[j]*stdWeight;
	if((bestPts!=0)&&(pts[j]!=velNull)&&(ptRatio!=0.0)) {
	  ptsScore = log((float)pts[j]/ptRatio)*ptsWeight; 
	  //Message::toScreen("ptsScore = "+QString().setNum(ptsScore)+" ptRatio "+QString().setNum(ptRatio));
	}
	
	// How do we get log without log(const Message)??
	
	
	if(winds[j]!=velNull) {
	  // We don't want any score if the wind didn't hit near peak
	  score[i][j][k] = windScore+stdScore+ptsScore;
	  // I think we might want this to be totalscore too........
	  // ps score was never declared the original version had a w index
	  // totalScore was just suming them up over the w's
	  // but that was commented out???
	  
	  
	  // Use default weight scheme to set radius and mean 
	  if((score[i][j][k] > tempBest)&&(k>=0)
	     &&(k<=simplexResults->at(i).getNumLevels())) {
	    tempBest = score[i][j][k];
	    if(lastj == -1){
	      // sort through to find the highest scoring x,y,radius
	      // for each level, later we will sum those together over 
	      // all levels and take the mean; if this is the first acceptable
	      // radius found in a level we know because lastj is still set to 
	      // -1, otherwise the at that was formerly the best must be
	      // subtracted from the sums.
	      lastj = j;
	      radiusSum += simplexResults->at(i).getRadius(j);
	      // gives the actual distance of the radius not just index
	      xSum+= simplexResults->at(i).getX(k,j);
	      ySum+= simplexResults->at(i).getY(k,j);
	    }
	    else {
	      // if we have found a higher scoring radius on the same level
	      // remove the old one and add the new
	      // consider changing jLast to jHigh or something, jBest mayber
	      radiusSum -= simplexResults->at(i).getRadius(j);
	      // gives the actual distance of the radius not just index	     
	      xSum-=simplexResults->at(i).getX(k,lastj);
	      ySum-=simplexResults->at(i).getY(k,lastj);
	      
	      radiusSum += simplexResults->at(i).getRadius(j);
	      xSum+=simplexResults->at(i).getX(k,j);
	      ySum+=simplexResults->at(i).getY(k,j);
	    }
	  }
	}
      }
      delete [] winds;
      delete [] stds;
      delete [] pts;
      delete [] peaks;
    }
    //Message::toScreen("Made it to means");
    
    // calculate the mean radius and center scores over all levels
    float currentNumLevels = simplexResults->at(i).getNumLevels(); 
    // what is the current level index verses height issue
    float meanRadius = radiusSum/currentNumLevels;
    //float meanXChoose = xSum/currentNumLevels;
    //float meanYChoose = ySum/currentNumLevels;
    
    //if(opt_i){
    //   calc_radscore();   we don't need this it is only related to crazy
    //}                     weight scheme stuff
    
    
    for(int k = 0; k < simplexResults->at(i).getNumLevels(); k++) {
      int bestIndex = 0;
      float bestScore = 0.0;
      for(int j = 0; j < simplexResults->at(i).getNumRadii(); j++) {
	if(isnan(score[i][j][k])||isinf(score[i][j][k])) {
	  emit errorlog(Message(QString("ChooseCenter: ChooseMeanCenters: Score no good here... i = "+QString().setNum(i)+" j = "+QString().setNum(j)+" k = "+QString().setNum(k)+" score = "+QString().setNum(score[i][j][k])),0,this->objectName(),Red,QString("Bad Score in ChooseMeanCenters")));
	  return false;
	}
	if(score[i][j][k] > bestScore) {
	  bestScore = score[i][j][k];
	  bestIndex = j;
	}
      }
      bestRadius[i][k] = bestIndex;
    }
    // Calculate initial standard deviations of radius and center
    centerDev[i] = 0.0;
    radiusDev[i] = 0.0;
    
    for(int k = 0; k < simplexResults->at(i).getNumLevels(); k++) {
      int j = bestRadius[i][k];
      //Message::toScreen("simplexResults["+QString().setNum(i)+"].getNumRadii() = "+QString().setNum(simplexResults->at(i).getNumRadii()));
      //Message::toScreen("Best Radius (j = "+QString().setNum(j)+")");
      //Message::toScreen(" has at ("+QString().setNum(simplexResults->at(i).getRadius(j))+")");
      
      float radLength = simplexResults->at(i).getRadius(j);
      radiusDev[i] +=(meanRadius-radLength)*(meanRadius-radLength);
      centerDev[i] +=sqrt(centerDev[i]/currentNumLevels);
      // Do these get put in the simplexData???
    }
  } 
  //Message::toScreen("Made it out of chooseMeanCenters");
  return true; // ??? assign this other options........
}



bool ChooseCenter::constructPolynomial()
{

  //Message::toScreen("Made it into construct Poly");

  // This initial check relies on the idea that the last volume will be
  // the volume currently being processed, and additionally the most recent
  // volume in the time series.

  if((simplexResults->last().getTime() > endTime) ||
     (simplexResults->last().getTime() < startTime)) {
    emit errorlog(Message(QString("Vortex is not within the specified time period for constructing polynomials."),0,this->objectName()));
    return false;
  }

  this->findHeights();

  int maxPolyArray = simplexResults->count();
  if(simplexResults->count() > 20)
    maxPolyArray = 21;
  
  // Construct a least squares polynomial to fit track
  
  bestFitVariance = new float*[4];
  bestFitDegree = new int*[4];
  bestFitCoeff = new float**[4];
  for(int criteria = 0; criteria < 4; criteria++) {
    bestFitVariance[criteria] = new float[numHeights->count()];
    bestFitDegree[criteria] = new int[numHeights->count()];
    bestFitCoeff[criteria] = new float*[numHeights->count()];
    for(int k = 0;k < numHeights->count(); k++){
      bestFitVariance[criteria][k] = 0;
      bestFitDegree[criteria][k] = 0;
      bestFitCoeff[criteria][k] = new float[maxPolyArray];
      for(int b = 0; b < maxPolyArray; b++)
	bestFitCoeff[criteria][k][b] = 0;
    }
  }
  
  QList<int> heights = numHeights->keys();
  // Sort heights so that k index is attached to a specific height
  // Sort smallest to largest
  // The sorting is nessecary because QHash randomizes ordering
  for(int s = 0; s < heights.count()-1; s++)
    for(int t = s+1; t < heights.count(); t++)
      if(heights[t] < heights[s])
	heights.swap(t,s);

  for(int k = 0; k < numHeights->count(); k++) {
    int currHeight = heights[k];
    int* levelIndices = indexOfHeights->value(currHeight);
    
    // In a lot of places we will replace k with 
    // levelIndices[simplexResultIndex] to keep the height consistant

    //Message::toScreen("Level = "+QString().setNum(k)+" @ height "+QString().setNum(currHeight));
    
    int maxPoly = 0;
    
    // We want to find the eariliest time for this process;
    // we don't want to use (simplexList)sort this late in the game 
    // because it might ruin the member arrays

    int timeRefIndex = 0;
    firstTime = QDateTime();
    while(levelIndices[timeRefIndex]==-1) {
      timeRefIndex++;
    }
    firstTime = simplexResults->at(timeRefIndex).getTime();
    int goodVolumes = numHeights->value(currHeight);
    for(int i = timeRefIndex+1; i < simplexResults->count(); i++) {
      if(levelIndices[i]!=-1) {
	if(simplexResults->at(i).getTime()<firstTime) {
	  timeRefIndex = i;
	  firstTime = simplexResults->at(i).getTime();
	}
      }
    }
    
    //Message::toScreen("How many good volumes: "+QString().setNum(goodVolumes));
    
    // What should the highest order basis polynomial;
    if(goodVolumes > 20)  
      maxPoly = 20;
    else 
      maxPoly = goodVolumes-1;

    if(maxPoly < 3){
      emit errorlog(Message(QString("Cannot Adequately Fit Storm Parameters to Polynomials of Less Than 3rd Order"),0,this->objectName(),Yellow,QString("Construct Polynomial Error")));
      continue;
    }
    
    //Message::toScreen("maxPoly = "+QString().setNum(maxPoly));
    
    // Things are stored here so far but never used
    // Store the variance of each polynomial fit by the maximum order
    // polynomial that the fit uses
    float* squareVariance = new float[maxPoly+1];

    // Initialize fitting matrices
    float* BB = new float[goodVolumes]; 
    float** MM = new float*[maxPoly+1];
    for(int rr = 0; rr <=maxPoly; rr++) {
      MM[rr] = new float[goodVolumes];
      for(int ii = 0; ii < goodVolumes; ii++) {
	MM[rr][ii] = 0;
	BB[ii] = 0;
      }
    }

    float *lastFitCoeff;
    lastFitCoeff = new float[maxPoly+1];
    float *currentCoeff; 
    currentCoeff = new float[maxPoly+1];
    
    for(int ii = 0; ii <= maxPoly; ii++) {
      lastFitCoeff[ii] = 0;
      currentCoeff[ii] = 0;
    }
    
    for(int criteria = 0; criteria < 4; criteria++) {

      //Message::toScreen("Criteria is "+QString().setNum(criteria));
      // Iterates through the procedure for the four different
      // curve fitting criteria

      for(int ii = 0; ii <= maxPoly; ii++)
	squareVariance[ii] = 0;
      
      for(int n = 1; n <=maxPoly; n++) {
	//Message::toScreen("Fitting to "+QString().setNum(n)+" order polynomial");
	for(int m = 0; m < n; m++) {
	  // Copy the last set into the lastFit array
	  lastFitCoeff[m]= currentCoeff[m];
	  currentCoeff[m] = 0;
	}
	currentCoeff[n] = 0;
       	for(int ii = 0; ii < goodVolumes; ii++) {
	  BB[ii] = 0;
	  for(int rr = 0; rr <= maxPoly; rr++)
	    MM[rr][ii]= 0;
	}
	
	int gv = 0;
	for(int i = 0; i < simplexResults->count(); i++) {
	  
	  // Only take those values which are in specified time range 
	  // and additionally have a level at this height
	  // Both of these characteristics are specified if the 
	  // value of the index in indexOfHeights value array is not set
	  // to -1.  This is done in findHeights()
	  
	  if(levelIndices[i]!=-1) {
	    
	    for(int r = 0; r <= n; r++) {
	      float min = ((float)firstTime.secsTo(simplexResults->at(i).getTime())/60.0);
	      MM[r][gv] = pow(min,(double)(r));
	    }
	    int jBest = bestRadius[i][levelIndices[i]];
	    float y = 0;
	    switch(criteria) {
	    case 0:
	      y = simplexResults->at(i).getX(levelIndices[i], jBest); break;
	    case 1:
	      y = simplexResults->at(i).getY(levelIndices[i], jBest); break;
	    case 2:
	      y = simplexResults->at(i).getRadius(jBest); break;
	    case 3:
	      y = simplexResults->at(i).getMaxVT(levelIndices[i], jBest); 
	      break; 
	    }
	    BB[gv] = y;
	    gv++;
	  }
	}
	float stDev;
	float *stError = new float[n+1];
	for(int ii = 0; ii <= n; ii++)
	  stError[ii] = 0;
	//Message::toScreen("MM");
	//Matrix::printMatrix(MM,n+1,goodVolumes);
	
	//Message::toScreen("BB");
	//Matrix::printMatrix(BB,goodVolumes);
	
	if(!Matrix::lls(n+1,goodVolumes,MM,BB,stDev,currentCoeff,stError)) {
	  emit errorlog(Message(QString("Least Squares Fit Failed in Construct Polynomial"),0,this->objectName(),Yellow,QString("Least Squares Fit Failed")));
	  return false;
	}
       
	float errorSum = 0;
	delete [] stError;
	for(int i = 0; i < simplexResults->count(); i++) {
	  if(levelIndices[i]!=-1) {
	    float min = ((float)firstTime.secsTo(simplexResults->at(i).getTime())/60.0);
	    float func_y  = 0;
	    for(int m = 0; m <=n; m++) {
	      func_y += currentCoeff[m]*pow(min,m);
	    }
	    //if(criteria == 2)
	    //Message::toScreen(" Fitted Radius = "+QString().setNum(func_y));
	    int jBest = bestRadius[i][levelIndices[i]];
	    switch(criteria) {
	    case 0:
	      errorSum +=pow(func_y-simplexResults->at(i).getX(levelIndices[i], jBest), 2); break;
	    case 1:
	      errorSum +=pow(func_y-simplexResults->at(i).getY(levelIndices[i], jBest), 2); break;
	    case 2:
	      errorSum +=pow(func_y-simplexResults->at(i).getRadius(jBest),2); break;
	    case 3:
	      errorSum +=pow(func_y-simplexResults->at(i).getMaxVT(levelIndices[i], jBest), 2); 
	      break;
	    }
	  }
	} 
	int degFreedom = goodVolumes-n;
	if(degFreedom == 0)
	  Message::toScreen("ChooseCenter construct Polynomial, division by zero");
	squareVariance[n] = errorSum/(float)degFreedom;
	//Message::toScreen("Got to here");
	if(n > 1) {
	  if(squareVariance[n] > squareVariance[n-1]) {
	    // we found the best fit!!!!
	    bestFitVariance[criteria][k] = squareVariance[n-1];
	    bestFitDegree[criteria][k] = n-1;
	    // Now we have to revert back to the old set
	    // This is why everything is kept in storage
	    for(int l = 0; l <= n; l++) {
	      bestFitCoeff[criteria][k][l] = lastFitCoeff[l];
	    }
	    break;
	  }
	  else {
	    if(n == maxPoly) {
	      // Maxed out
	      bestFitVariance[criteria][k] = squareVariance[n];
	      bestFitDegree[criteria][k] = n;
	      for(int l = 0; l <= n; l++) {
		bestFitCoeff[criteria][k][l] = currentCoeff[l];
	      }
	    }
	    else {
	      //Message::toScreen("Using F-Test");
	      // preform the f test to see if a higher polynomial is
	      // warranted
	      float fTest;
	      float m_delta = squareVariance[n-1]*(float)(degFreedom+1) ;
	      m_delta-= squareVariance[n]*(float)degFreedom;
	      if(squareVariance[n] > 0) 
		fTest = m_delta/squareVariance[n];
	      else 
		fTest = 0;
	      if(degFreedom >30) 
		degFreedom = 30;
	      float fCrit = fCriteria[degFreedom-1];
	      if(fTest < fCrit) {
		// Found the best center
		bestFitVariance[criteria][k] = squareVariance[n-1];
		bestFitDegree[criteria][k] = n-1;
		// degree = n-1; phasing out
		for(int l = 0; l <= (n-1); l++) {
		  bestFitCoeff[criteria][k][l] = lastFitCoeff[l];
		}		  
	      }
	    }
	  }
	}
	else {
	  // Doing a linear fit with only 3 points
	  bestFitVariance[criteria][k] = squareVariance[n];
	  bestFitDegree[criteria][k] = n;
	  for(int l = 0; l <= n; l++) {
	    bestFitCoeff[criteria][k][l] = currentCoeff[l];
	  }
	}
      }
    }
    //Message::toScreen("Got to deleting arrays");
    for(int ii = 0; ii <= maxPoly; ii++) {
      delete [] MM[ii];
    }
    delete [] MM;
    delete [] BB;
    delete [] squareVariance;
    delete [] lastFitCoeff;
    delete [] currentCoeff;
    //delete [] stError;
  }
  
  //Message::toScreen("Finished Construct Polynomial");
  return true;
}



bool ChooseCenter::fixCenters()
{

  // Set up parameters for populating the vortexData

  float radarLat = config->getParam(config->getConfig("radar"), QString("lat")).toFloat();
  float radarLon = config->getParam(config->getConfig("radar"), QString("lon")).toFloat();
  float radarLatRadians = radarLat * acos(-1.0)/180.0;
  float fac_lat = 111.13209 - 0.56605 * cos(2.0 * radarLatRadians) + 0.00012 * cos(4.0 * radarLatRadians) - 0.000002 * cos(6.0 * radarLatRadians);
  float fac_lon = 111.41513 * cos(radarLatRadians) - 0.09455 * cos(3.0 * radarLatRadians) + 0.00012 * cos(5.0 * radarLatRadians);
  
  //Message::toScreen("Lat Lon Conversions and Time Resolved");
  
  float **newVariance = new float*[4];
  for(int m = 0; m < 4; m++) {
    newVariance[m] = new float[numHeights->count()];
    for(int k = 0; k < numHeights->count(); k++)
      newVariance[m][k] = 0;
  }
  newBestRadius = new int*[simplexResults->count()];
  newBestCenter = new int*[simplexResults->count()];
  for(int i = 0; i < simplexResults->count(); i++) {
    newBestRadius[i] = new int[numHeights->count()];
    newBestCenter[i] = new int[numHeights->count()];
    for(int k = 0; k < numHeights->count(); k++) {
      newBestRadius[i][k] = 0;
      newBestCenter[i][k] = 0;
    }
  }

  // Get the volume with the latest time so we know which one we are
  // currently working on.

  // set firstTime to the earliest volume in the simplexResults set
  
  firstTime = simplexResults->at(0).getTime();
  for(int ii = 1; ii < simplexResults->count(); ii++) {
    if(simplexResults->at(ii).getTime() < firstTime)
      firstTime = simplexResults->at(ii).getTime();
  }

  // Find the last index for picking a center;
  int lastTimeIndex = 0;
  float longestTime = 0;
  for(int i = 0; i < simplexResults->count(); i++) {
    if(getMinutesTo(simplexResults->at(i).getTime()) > longestTime) {
      lastTimeIndex = i;
      longestTime = getMinutesTo(simplexResults->at(i).getTime());
    }
  }
  
  QList<int> heights = numHeights->keys();
  // Sort heights so that k index is attached to a specific height
  // Sort smallest to largest
  // The sorting is nessecary because QHash randomizes ordering
  for(int s = 0; s < heights.count()-1; s++)
    for(int t = s+1; t < heights.count(); t++)
      if(heights[t] < heights[s])
	heights.swap(t,s);
  
  for(int k = 0; k < numHeights->count(); k++) {
    
    int currHeight = heights[k];  // in meters
    int goodVolumes = numHeights->value(currHeight);
    int* levelIndices = indexOfHeights->value(currHeight);

    //Message::toScreen("Level = "+QString().setNum(k)+" @ height "+QString().setNum(currHeight));
    
    // If the level we are examining is not part of the current 
    // simplex search then don't even bother
    //Message::toScreen("Working on level k = "+QString().setNum(k)+" @ "+QString().setNum(currHeight));
    if(levelIndices[lastTimeIndex]==-1){
      //Message::toScreen("Skipping Work On Level k = "+QString().setNum(k));
      // the last simplex had no data on this level
      continue;
    }
    int timeRefIndex = 0;
    firstTime = QDateTime();
    while(levelIndices[timeRefIndex]==-1) {
      timeRefIndex++;
    }
    firstTime = simplexResults->at(timeRefIndex).getTime();
    for(int i = timeRefIndex+1; i < simplexResults->count(); i++) {
      if(levelIndices[i]!=-1) {
	if(simplexResults->at(i).getTime()<firstTime) {
	  timeRefIndex = i;
	  firstTime = simplexResults->at(i).getTime();
	}
      }
    }

    // Do work that fixCenters is supposed to
    
    float xErrorSum = 0;
    float yErrorSum = 0;
    float radErrorSum = 0;
    float windErrorSum = 0;
    
    for(int i = 0; i < simplexResults->count(); i++) {
      if(levelIndices[i]!=-1){
	float x = 0;
	float y = 0;
	float rad = 0;
	float wind = 0;
	float finalStd = 0;
	float confidence = 0;
	float stdError = 0;
	float min = ((float)firstTime.secsTo(simplexResults->at(i).getTime())/60.0);
	//check out use of n up to best degree of fit based on assignment
	for(int n = 0; n <= bestFitDegree[0][k]; n++) {
	  x += bestFitCoeff[0][k][n]*pow(min,n);
	}
	for(int n = 0; n <= bestFitDegree[1][k]; n++) {
	  y += bestFitCoeff[1][k][n]*pow(min,n);
	}
	for(int n = 0; n <= bestFitDegree[2][k]; n++) {
	  rad += bestFitCoeff[2][k][n]*pow(min,n);
	}
	for(int n = 0; n <= bestFitDegree[3][k]; n++) {
	  wind += bestFitCoeff[3][k][n]*pow(min,n);
	}

	//Message::toScreen("Rad from fit is "+QString().setNum(rad)+"  x from fit "+QString().setNum(x)+"  y from fit "+QString().setNum(y)+"  wind from fit "+QString().setNum(wind));
	
	float minError = 0;
	float totalError = 0;
	float minXError = x*x;
	float minYError = y*y;
	float minRadError = rad*rad;
	float minWindError = wind*wind;
	
	for(int j = 0; j < simplexResults->at(i).getNumRadii(); j++) {
	  float xError, yError, radError, windError;
	  int howManyReturnNull = 0;
	  for(int l = 0; l < simplexResults->at(i).getNumPointsUsed(); l++) {
	    if(simplexResults->at(i).getCenter(levelIndices[i],j,l).isNull()) {
	      howManyReturnNull++;
	    }
	    else {
	      Center currCenter = simplexResults->at(i).getCenter(levelIndices[i],j,l);
	      float stdX = sqrt(bestFitVariance[0][k]);
	      xError = exp(-.5*pow((x-currCenter.getX())/stdX, 2));
	      float stdY = sqrt(bestFitVariance[1][k]);
	      yError = exp(-.5*pow((y-currCenter.getY())/stdY, 2));
	      float stdRad = sqrt(bestFitVariance[2][k]);
	      if(stdRad == 0) {
		radError = 1;
	      }
	      else {
		radError = exp(-.5*pow((rad-currCenter.getRadius())/stdRad, 2));  
	      }
	      float stdWind = sqrt(bestFitVariance[3][k]);
	      windError = exp(-.5*pow((wind-currCenter.getMaxVT())/stdWind, 2));
	      xError *= positionWeight;
	      yError *= positionWeight;
	      radError *= rmwWeight;
	      windError *= velWeight;
	      totalError = xError+yError+radError+windError;
	      
	      if(totalError > minError) {
		minError = totalError;
		minXError = xError;
		minYError = yError;
		minRadError = radError;
		minWindError = windError;
		newBestRadius[i][k] = j;
		newBestCenter[i][k] = l;
		
		int jBest = bestRadius[i][levelIndices[i]];
		float meanX=((simplexResults->at(i).getX(levelIndices[i],jBest)+x+currCenter.getX())/3.0);
		float meanY=((simplexResults->at(i).getY(levelIndices[i],jBest)+y+currCenter.getY())/3.0);
		//finalStd = pow((simplexResults->at(i).getX(levelIndices[i],jBest)-meanX),2);
		finalStd = 0;
		finalStd += pow((x-meanX), 2);
		finalStd += pow((currCenter.getX()-meanX), 2);
		finalStd += pow((simplexResults->at(i).getY(levelIndices[i],jBest)-meanY),2);
		finalStd += pow((y-meanY), 2);
		finalStd += pow((currCenter.getY()-meanY), 2);
		finalStd = sqrt(finalStd/3.0);
		confidence = (100*minError)/4.0;   // percent ? who uses this
		float avgError = minError/4.0;
		stdError = pow((avgError-minXError),2);
		stdError += pow((avgError-minYError),2);
		stdError += pow((avgError-minRadError),2);
		stdError += pow((avgError-minWindError),2);
		stdError /= 4.0;
		// finalStd and stdError are printed only for each level and 
		// volume, these may be desired as outputs
	      }
	    }
	  }
	}
	// new variables here but why?
	
	// get best simplex center;
	Center bestCenter = simplexResults->at(i).getCenter(levelIndices[i],
							newBestRadius[i][k],
							newBestCenter[i][k]);
	float xError, yError, radError, windError;
	
	//Message::toScreen("BestCenter is at level "+QString().setNum(levelIndices[i])+" radius index "+QString().setNum(newBestRadius[i][k])+" center index "+QString().setNum(newBestCenter[i][k])+" with a radius "+QString().setNum(bestCenter.getRadius())+" x = "+QString().setNum(bestCenter.getX())+" y = "+QString().setNum(bestCenter.getY()));
	
	xError = (x-bestCenter.getX())*(x-bestCenter.getX());
	yError = (y-bestCenter.getY())*(y-bestCenter.getY());
	radError = (rad-bestCenter.getRadius())*(rad-bestCenter.getRadius());
	windError = (wind-bestCenter.getMaxVT())*(wind-bestCenter.getMaxVT());
	xErrorSum += xError;
	yErrorSum += yError;
	radErrorSum += radError;
	windErrorSum += windError;
	
	if(i == lastTimeIndex) {
	  //Message::toScreen("Vortex Data level "+QString().setNum(levelIndices[i])+" @ height "+QString().setNum(simplexResults->at(i).getHeight(levelIndices[i]))+" time "+vortexData->getTime().toString()+" radius = "+QString().setNum(rad)+" radError "+QString().setNum(radError));
	  // if the volume we are looking at is the last one we will want to keep
	  // all the info in vortexData
	  
	  // We don't check to see that any of these levels are in the search
	  // zone for vortexData in vtd

	  int j = bestRadius[i][levelIndices[i]];
	  float centerLat = radarLat + y/fac_lat;
	  float centerLon = radarLon + x/fac_lon;
	  vortexData->setLat(levelIndices[i], centerLat);
	  vortexData->setLon(levelIndices[i], centerLon);
	  vortexData->setHeight(levelIndices[i], simplexResults->at(i).getHeight(levelIndices[i]));
	  vortexData->setRMW(levelIndices[i], rad);
	  vortexData->setRMWUncertainty(levelIndices[i], radError);
	  //Message::toScreen("Rad error for level "+QString().setNum(k)+" is "+QString().setNum(radError));
	  vortexData->setCenterStdDev(levelIndices[i], sqrt(xError*xError+yError*yError));
	  vortexData->setNumConvergingCenters(levelIndices[i], simplexResults->at(i).getNumConvergingCenters(levelIndices[i],j));
	}
      }
    }
  
    // newVariance is the variance related to the fit of the line?
    float degree = bestFitDegree[3][k];
    newVariance[0][k]=xErrorSum/(goodVolumes-degree);
    newVariance[1][k]=yErrorSum/(goodVolumes-degree);
    newVariance[2][k]=radErrorSum/(goodVolumes-degree);
    newVariance[3][k]=windErrorSum/(goodVolumes-degree);
    // the newVariance is never used in the perl code of choose center,
    // and the variable degree is never initialized either, it is just left at
    // the last degree checked for the last function, probably wind
    // is this intentional or not??
    // my best guess is that degree will be set to the high degree for wind
    // so that is what I will set it to here
  }

  for(int i = 0; i < 4; i++)
    delete [] newVariance[i];
  delete [] newVariance;
  //Message::toScreen("Finished Fix Centers");
  return true;
}
    
void ChooseCenter::useLastMean()
{
  // Fake fill of vortexData for testing purposes
  float radarLat = config->getParam(config->getConfig("radar"), QString("lat")).toFloat();
  float radarLon = config->getParam(config->getConfig("radar"), QString("lon")).toFloat();
  float radarLatRadians = radarLat * acos(-1.0)/180.0;
  float fac_lat = 111.13209 - 0.56605 * cos(2.0 * radarLatRadians)
	  + 0.00012 * cos(4.0 * radarLatRadians) - 0.000002 * cos(6.0 * radarLatRadians);
  float fac_lon = 111.41513 * cos(radarLatRadians)
	  - 0.09455 * cos(3.0 * radarLatRadians) + 0.00012 * cos(5.0 * radarLatRadians);
  
  int i = simplexResults->size() - 1;
  for(int k = 0; k < simplexResults->at(i).getNumLevels(); k++) {
	  int j = bestRadius[i][k];
	  float centerLat = radarLat + simplexResults->at(i).getY(k,j)/fac_lat;
	  float centerLon = radarLon + simplexResults->at(i).getX(k,j)/fac_lon;
	  vortexData->setLat(k, centerLat);
	  vortexData->setLon(k, centerLon);
	  vortexData->setHeight(k, simplexResults->at(i).getHeight(k));
	  vortexData->setRMW(k, simplexResults->at(i).getRadius(j));
	  vortexData->setRMWUncertainty(k, -999);
	  vortexData->setCenterStdDev(k, simplexResults->at(i).getCenterStdDev(k,j));
	  vortexData->setNumConvergingCenters(k, simplexResults->at(i).getNumConvergingCenters(k,j));
  } 
  
}

float ChooseCenter::getMinutesTo(const QDateTime &volTime)
{
  float min = ((float)firstTime.secsTo(volTime)/60.0);
  return min;
}

void ChooseCenter::catchLog(const Message& message)
{
  emit errorlog(message);
}

void ChooseCenter::findHeights()
{
  numHeights = new QHash<int, int>;
  indexOfHeights = new QHash<int,int*>;
  numHeights->clear();
  indexOfHeights->clear();
  for(int i = 0; i < simplexResults->count(); i++) {
    if((simplexResults->at(i).getTime() >= startTime) &&
       (simplexResults->at(i).getTime() <= endTime)) {
      for(int j = 0; j < simplexResults->at(i).getNumLevels(); j++) {
	int currHeight = int(simplexResults->at(i).getHeight(j)*1000+.5);
	if(numHeights->value(currHeight,-1)==-1) {
	  numHeights->insert(currHeight,1);
	  int* newHeightArray = new int[simplexResults->count()];
	  for(int kk = 0; kk < simplexResults->count(); kk++)
	    newHeightArray[kk] = -1;
	  indexOfHeights->insert(currHeight,newHeightArray);
	}
	else
	  numHeights->insert(currHeight, numHeights->value(currHeight)+1);
      }
    }
  }
  for(int i = 0; i < simplexResults->count(); i++) {
    if((simplexResults->at(i).getTime() >= startTime) &&
       (simplexResults->at(i).getTime() <= endTime)) {
      for(int j = 0; j < simplexResults->at(i).getNumLevels(); j++) {
	int currHeight = int(simplexResults->at(i).getHeight(j)*1000+.5);
	int* heightProfile = indexOfHeights->take(currHeight);
	heightProfile[i] = j;
	indexOfHeights->insert(currHeight,heightProfile);
      }
    }
  }
  QList<int> heights = numHeights->keys();
  for(int i = 0; i < heights.count(); i++) {
    if(numHeights->value(heights[i]) <= 3) {
      //Message::toScreen("Not enough values @ height "+QString().setNum(heights[i])+" removing this set.");
      numHeights->remove(heights[i]);
      indexOfHeights->remove(heights[i]);
      continue;
    }    
    QString message;
    //Message::toScreen(QString().setNum(numHeights->value(heights[i]))+" @ "+QString().setNum(heights[i]));
    for(int ii = 0; ii < simplexResults->count(); ii++) {
      message+= QString().setNum(indexOfHeights->value(heights[i])[ii])+", ";
    }
    //Message::toScreen(message);
  }
  
}
