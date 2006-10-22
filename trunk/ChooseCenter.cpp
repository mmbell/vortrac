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

ChooseCenter::ChooseCenter(Configuration* newConfig, 
			   const SimplexList &newList, VortexData* vortexPtr)
{
  velNull = -999.;
  config = newConfig;
  simplexResults = newList;
  vortexData = vortexPtr;
  simplexResults.timeSort();
  score = NULL;
  bestRadius = NULL;
  centerDev = NULL;
  radiusDev = NULL;
  bestFitVariance = NULL;
  bestFitDegree = NULL;
  bestFitCoeff = NULL;
  newBestRadius = NULL;
  newBestCenter = NULL;
}

ChooseCenter::~ChooseCenter()
{
  config = NULL;
  delete config;
  vortexData = NULL;
  delete vortexData;
  
  
  for(int i = 0; i < simplexResults.count(); i++) {
    for(int j = 0; j < simplexResults[i].getNumRadii(); j++) {
      delete [] score[i][j];
    }
    delete [] score[i];
  }
  delete [] score;
  
  // We might not be able to delete these depending on weither they 
  // are passed out for further use or not after the ChooseCenter object is
  // distroyed

  for(int i = 0; i < simplexResults.count(); i++) {
    delete[] bestRadius[i];
    delete[] newBestRadius[i];
    delete[] newBestCenter[i];    
  }
  delete[] bestRadius;
  delete[] newBestRadius;
  delete[] newBestCenter;

  for(int m = 0; m < 4; m++) {
    for(int p = 0; p < numLevels; p++) {
      delete[] bestFitCoeff[m][p];
    }
    delete[] bestFitCoeff[m];
    delete[] bestFitDegree[m];
    delete[] bestFitVariance[m];
  }
  delete[] centerDev;
  delete[] radiusDev;
  delete[] bestFitCoeff;
  delete[] bestFitVariance;
  delete[] bestFitDegree;
  
}

void ChooseCenter::setConfig(Configuration* newConfig)
{
  config = newConfig; 
}

void ChooseCenter::setSimplexList(const SimplexList &newList)
{
  simplexResults = newList;
}

bool ChooseCenter::findCenter()
{
  
   initialize();
   if(!chooseMeanCenters()) {
     //emit log(Message("Choose Mean Centers Failed!"));
     Message::toScreen("Choose Mean Centers Failed!");
     return false;
   }
   //Message::toScreen("Returned From ChooseMeanCenters");
  
   if(simplexResults.count() > minVolumes) {
     Message::toScreen("ChooseCenter: Try Construct Polynomials");
     if(constructPolynomial()) {
       //emit log(Message("Choose Center failed to Construct Polynomial"));
       Message::toScreen("Choose Center Constructed Polynomial - Trying Fix Centers");
       if(!fixCenters()) {
	 Message::toScreen("Choose Center failed in fixCenters");
	 useLastMean();
	 return false;
       }
       else {
	 return true;
       }
     }
     else {
       useLastMean();
       return true;
     }
   }
   else {
     Message::toScreen("ChooseCenter: Using Last Mean Center For Guess");
     useLastMean();
   }
   return true;
}

void ChooseCenter::initialize()
{
  // Pulls all the necessary user parameters from the configuration panel
  //  and initializes the array used for fTesting

  numLevels = simplexResults[0].getNumLevels();
  for(int i = 1; i < simplexResults.count(); i++) {
    if(simplexResults[i].getNumLevels() < numLevels) 
      numLevels = simplexResults[i].getNumLevels();
  }

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
  startTime = QDateTime(sDate);
  startTime.setTime(sTime);
  endTime = QDateTime(eDate);
  endTime.setTime(eTime);

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
  
  //if(!simplexResults.isEmpty())
  //  checkHeights();
  //Message::toScreen("Made it out of initialize");
}

/*
void ChooseCenter::checkHeights()
{

}
*/

bool ChooseCenter::chooseMeanCenters()
{
  /*
   * Determines the radius of maximum wind for each level of each volume used
   *  based on the scoring system based on averaged parameters from all the
   *  converging centers found in the simplex run
   *
   */

  if(simplexResults.isEmpty())
    return false;
  
  //checkHeights();
  
  bestRadius = new int*[simplexResults.count()];
  centerDev = new float[simplexResults.count()];
  radiusDev = new float[simplexResults.count()];
  score = new float**[simplexResults.count()];
  for(int i = 0; i < simplexResults.count(); i++) {
    bestRadius[i] = new int[simplexResults[i].getNumLevels()];
    score[i] = new float*[simplexResults[i].getNumRadii()];
    for(int rad = 0; rad < simplexResults[i].getNumRadii(); rad++) {
      score[i][rad] = new float[simplexResults[i].getNumLevels()];
      for(int level = 0; level < simplexResults[i].getNumLevels(); level++){
	score[i][rad][level] = 0.0;
	bestRadius[i][level] = 0;
      }
    }}for(int i = 0; i < simplexResults.count(); i++) {
 
    // Now we zero out the variables used for finding the mean of each volume

    float radiusSum = 0;
    float xSum = 0; 
    float ySum = 0;
    for(int k = 0; k < simplexResults[i].getNumLevels(); k++) {
      int lastj = -1;
      float *winds = new float[simplexResults[i].getNumRadii()];
      float *stds = new float[simplexResults[i].getNumRadii()];
      float *pts = new float[simplexResults[i].getNumRadii()];
      float bestWind = 0.;
      float bestStd = 50.;
      float bestPts = 0.;
      float ptRatio = (float)simplexResults[i].getNumPointsUsed()/2.718281828;
      for(int j = 0; j < simplexResults[i].getNumRadii(); j++) {
	
	// Examine each radius based on index j, for the one containing the 
	//   highest tangential winds
	
	winds[j] = velNull;
	stds[j] = velNull;
	pts[j] = velNull;
	if(simplexResults[i].getMaxVT(k,j)!=velNull) {
	  winds[j] = simplexResults[i].getMaxVT(k,j);
	  if(winds[j] > bestWind)
	    bestWind = winds[j];
	}
	if(simplexResults[i].getCenterStdDev(k,j)!=velNull) {
	  stds[j] = simplexResults[i].getCenterStdDev(k,j);
	  if(stds[j] < bestStd)
	    bestStd = stds[j];
	}
	if(simplexResults[i].getNumConvergingCenters(k,j)!=velNull) {
	  pts[j] = simplexResults[i].getNumConvergingCenters(k,j);
	  if(pts[j] > bestPts)
	    bestPts = pts[j];
	}
      }
      
      // Formerlly know as fix winds
      //   which was a sub routine in the perl version of this algorithm
      //   zeros all wind entrys that are not a local maxima, or adjacent 
      //   to a local maxima
      
      int count = 0;
      float *peakWinds = new float[simplexResults[i].getNumRadii()];
      float *peaks = new float[simplexResults[i].getNumRadii()];
      for(int z = 0; z < simplexResults[i].getNumRadii(); z++) {
	peakWinds[z] = 0;
	peaks[z] = 0;
      }
      float meanPeak,meanStd;
      peaks[0] = 0;
      peaks[simplexResults[i].getNumRadii()-1] = 0;
      for(int a = 1; a < simplexResults[i].getNumRadii()-1; a++) {
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
      
      for(int jj = 0; jj < simplexResults[i].getNumRadii(); jj++) {
	if(((jj > 0)&&(jj < simplexResults[i].getNumRadii()-1))
	   &&((peaks[jj] == 1)||(peaks[jj+1] == 1)||(peaks[jj-1] == 1))) {
	  winds[jj] = simplexResults[i].getMaxVT(k,jj);
	  // Keep an eye out for the maxima 
	  if(winds[jj] > bestWind) {
	    bestWind = winds[jj];
	  }
	}
	else {
	  winds[jj] = velNull;
	}
      }
      
      float tempBest = 0;
      // int bestIndex = 0;  wrong place to declare this ?
      for(int j = 0; j < simplexResults[i].getNumRadii(); j++) {
	score[i][j][k] = velNull;
	float windScore = 0;
	float stdScore = 0;
	float ptsScore = 0;
	//for(int w = 0; w <= #weightSchemes; w++) {
	// Mike says we only need to run this once
	// assign windWeight, stdWeight, ptsWeight, but not
	// distWeight cause we are droping it
	if((bestWind!=0)&&(winds[j]!=velNull))
	  windScore = exp(winds[j]-bestWind)*windWeight;
	if(stds[j]!=velNull)
	  stdScore = bestStd/stds[j]*stdWeight;
	if((bestPts!=0)&&(pts[j]!=velNull))
	  ptsScore = log((float)pts[j]/ptRatio)*ptsWeight;
	
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
	     &&(k<=simplexResults[i].getNumLevels())) {
	    tempBest = score[i][j][k];
	    if(lastj == -1){
	      // sort through to find the highest scoring x,y,radius
	      // for each level, later we will sum those together over 
	      // all levels and take the mean; if this is the first acceptable
	      // radius found in a level we know because lastj is still set to 
	      // -1, otherwise the value that was formerly the best must be
	      // subtracted from the sums.
	      lastj = j;
	      radiusSum += simplexResults[i].getRadius(j);
	      // gives the actual distance of the radius not just index
	      xSum+= simplexResults[i].getX(j,k);
	      ySum+= simplexResults[i].getY(j,k);
	    }
	    else {
	      // if we have found a higher scoring radius on the same level
	      // remove the old one and add the new
	      // consider changing jLast to jHigh or something, jBest mayber
	      radiusSum -= simplexResults[i].getRadius(j);
	      // gives the actual distance of the radius not just index	     
	      xSum-=simplexResults[i].getX(lastj,k);
	      ySum-=simplexResults[i].getY(lastj,k);
	      
	      radiusSum += simplexResults[i].getRadius(j);
	      xSum+=simplexResults[i].getX(j,k);
	      ySum+=simplexResults[i].getY(j,k);
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
    float currentNumLevels = simplexResults[i].getNumLevels(); 
    // what is the current level index verses height issue
    float meanRadius = radiusSum/currentNumLevels;
    float meanXChoose = xSum/currentNumLevels;
    float meanYChoose = ySum/currentNumLevels;
    
    //if(opt_i){
    //   calc_radscore();   we don't need this it is only related to crazy
    //}                     weight scheme stuff
    
    
    for(int k = 0; k <= simplexResults[i].getNumLevels(); k++) {
      int bestIndex = 0;
      for(int j = 0; j < simplexResults[i].getNumRadii(); j++) {
	// Talked with mike the division below should be by 1
	//score[i][j][k] /=(numWeightSchemes+1);
	// totalscore[i][j][k] /=(numWeightSchemes+1);
	// This was 2 in the code at least cause we added the max VTC0 default
	// to whatever was in the file, I think this is one here
      }
      float bestScore = 0.0;
      for(int j = 0; j < simplexResults[i].getNumRadii(); j++) {
	//if(totalScore[i][j][k] > bestScore) {
	//  bestScore = totalScore[i][j][k];
	//  bestIndex = j;
	//}
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
    
    for(int k = 0; k <= simplexResults[i].getNumLevels(); k++) {
      int j = bestRadius[i][k];
      //Message::toScreen("simplexResults["+QString().setNum(i)+"].getNumRadii() = "+QString().setNum(simplexResults[i].getNumRadii()));
      //Message::toScreen("Best Radius (j = "+QString().setNum(j)+")");
      //Message::toScreen(" has value ("+QString().setNum(simplexResults[i].getRadius(j))+")");
      
      float radLength = simplexResults[i].getRadius(j);
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
  int maxPoly = 0;

  // We want to find the eariliest time for this process;
  // we don't want to use (simplexList)sort this late in the game 
  // because it might ruin the member arrays
  firstTime = simplexResults[0].getTime();
  //Message::toScreen("First time = "+firstTime.toString());
  float timeRefIndex = 0;
  for(int i = 1; i < simplexResults.count(); i++) {
     //Message::toScreen("Time of index i = "+QString().setNum(i)+" "+simplexResults[i].getTime().toString()+" time between start and this one = "+QString().setNum(firstTime.secsTo(simplexResults[i].getTime())));
    if(firstTime.secsTo(simplexResults[i].getTime()) < 0) {
      timeRefIndex = i;
      firstTime = simplexResults[i].getTime();
    }
  }
  //Message::toScreen("timeRefIndex = "+QString().setNum(timeRefIndex));

  //Message::toScreen("First time = "+firstTime.toString());

  // Now starttime is the first time in the set of volumes 

  // Construct a least squares polynomial to fit track
  // Export variance for analysis
  
  bestFitVariance = new float*[4];
  bestFitDegree = new int*[4];
  for(int criteria = 0; criteria < 4; criteria++) {
    bestFitVariance[criteria] = new float[numLevels];
    bestFitDegree[criteria] = new int[numLevels];
    for(int ii = 0;ii< numLevels; ii++){
      bestFitVariance[criteria][ii] = 0;
      bestFitDegree[criteria][ii] = 0;
    }
  }

  
  // What should the highest order basis polynomial;
  if(simplexResults.count() > 20)
    maxPoly = 20;
  else 
    maxPoly = simplexResults.count()-1;
  
  //Message::toScreen("maxPoly = "+QString().setNum(maxPoly));

  // Things are stored here so far but never used
  float* squareVariance = new float[maxPoly+1];
  
  // This goes from 0 to less than maxPoly, because this is the number
  // of fits that we will examine the variance of.

  float* BB = new float[maxPoly+1];
  float** MM = new float*[maxPoly+1];
  for(int rr = 0; rr <=maxPoly; rr++) {
    MM[rr] = new float[simplexResults.count()];
    BB[rr] = 0;
    for(int ii = 0; ii < simplexResults.count(); ii++) 
      MM[rr][ii] = 0;
  }
  
  if(maxPoly < 3){
    //Message::toScreen("Used to return false because the fitting order is limited to less than 3 by the number of volumes available");
    return false;
    // Expand this to get variables in the right places
    // So the next member function can gracefully handle things
  }
	
  float *lastFitCoeff;
  lastFitCoeff = new float[maxPoly+1];
  float *currentCoeff; 
  currentCoeff = new float[maxPoly+1];

  for(int ii = 0; ii <= maxPoly; ii++) {
    lastFitCoeff[ii] = 0;
    currentCoeff[ii] = 0;
  }
  
  bestFitCoeff = new float**[4];
  for(int criteria = 0; criteria < 4; criteria++) {
    bestFitCoeff[criteria] = new float*[numLevels];
    for (int k = 0; k < numLevels; k++) {
      bestFitCoeff[criteria][k] = new float[maxPoly+1];
      for(int b = 0; b <= maxPoly; b++)
	bestFitCoeff[criteria][k][b] = 0;
    }
  }
  // Things are stored here so far but it is never used
  
  for(int k = 0; k < numLevels; k++) {
    // had to pick a simplex data to get num levels from, this use requires 
    // that they all be the same, we can do some checking of this prior
    // using the height variables and total indexes to decide where 
    // there is overlap if volumes are pulled from multiple different
    // configurations

    //Message::toScreen("Level = "+QString().setNum(k));

    for(int criteria = 0; criteria < 4; criteria++) {
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
	// degree = n; what - no one uses this and it is the same as best degree
	for(int rr = 0; rr <=maxPoly; rr++) {
	  BB[rr] = 0;
	  for(int ii = 0; ii < simplexResults.count(); ii++)
	    MM[rr][ii]= 0;
	}
	
	for(int r = 0; r <= n; r++) {
	  for(int i = 0; i < simplexResults.count(); i++) {
	    float min = ((float)firstTime.secsTo(simplexResults[i].getTime())/60.0);
	    float othermin = getMinutesTo(simplexResults[i].getTime());
	    //if(othermin != min)
	      //Message::toScreen("Angry time issues! - 595");
	    MM[r][i] = pow(min,(double)(r));
	  }
	}
	
	for(int i = 0; i < simplexResults.count(); i++) {
	  float min = ((float)firstTime.secsTo(simplexResults[i].getTime())/60.0);
	  float othermin = getMinutesTo(simplexResults[i].getTime());
	  //if(othermin != min)
	  //Message::toScreen("Angry time issues! - 604");
	  int jBest = bestRadius[i][k];
	  float y = 0;
	  switch(criteria) {
	  case 0:
	    y = simplexResults[i].getX(k, jBest); break;
	  case 1:
	    y = simplexResults[i].getY(k, jBest); break;
	  case 2:
	    y = simplexResults[i].getRadius(jBest); break;
	  case 3:
	    y = simplexResults[i].getMaxVT(k, jBest); break;
	  }
	  //if(criteria == 2)
	    //Message::toScreen("Y for i = "+QString().setNum(i)+" is "+QString().setNum(y));
	  BB[i] = y;
	}
	float stDev;
	float *stError = new float[n+1];
	for(int ii = 0; ii <= n; ii++)
	  stError[ii] = 0;
	//Matrix::printMatrix(MM,n+1,simplexResults.count());
	//Matrix::printMatrix(BB,simplexResults.count());
	if(!Matrix::lls(n+1, simplexResults.count(), MM, BB, stDev, currentCoeff, stError)) {
	  Message::toScreen("least square fit failed in find center");
	}
       
	float errorSum = 0;
	delete [] stError;
	for(int i = 0; i < simplexResults.count(); i++) {
	  float min = ((float)firstTime.secsTo(simplexResults[i].getTime())/60.0);
	  float othermin = getMinutesTo(simplexResults[i].getTime());
	  //if(othermin != min)
	  //  Message::toScreen("Angry time issues! - 639");
	  float func_y  = 0;
	  for(int m = 0; m <=n; m++) {
	    func_y += currentCoeff[m]*pow(min,m);
	  }
	  int jBest = bestRadius[i][k];
	  switch(criteria) {
	  case 0:
	    errorSum +=pow(func_y-simplexResults[i].getX(jBest, k), 2); break;
	  case 1:
	    errorSum +=pow(func_y-simplexResults[i].getY(jBest, k), 2); break;
	  case 2:
	    errorSum +=pow(func_y-simplexResults[i].getRadius(jBest),2); break;
	  case 3:
	    errorSum +=pow(func_y-simplexResults[i].getMaxVT(jBest, k), 2); 
	    break;
	  }
	} 
	int degFreedom = simplexResults.count()-n;
	//if(degFreedom == 0)
	//  degFreedom == 1;
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
	  // degree = n; phasing out
	  for(int l = 0; l <= n; l++) {
	    bestFitCoeff[criteria][k][l] = currentCoeff[l];
	  }
	}
      }
      //if(criteria==2)
      //Matrix::printMatrix(squareVariance, maxPoly+1);
    }
  }
  //Message::toScreen("Got to deleting arrays");
  for(int ii = 0; ii <= maxPoly; ii++) 
    delete [] MM[ii];
  delete [] MM;
  delete [] BB;
  delete [] squareVariance;
  delete [] lastFitCoeff;
  delete [] currentCoeff;
  //delete [] stError;
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
  
  
  // Figure out which i is the most recent
  firstTime = simplexResults[0].getTime();
  float timeRefIndex = 0;
  for(int i = 1; i < simplexResults.count(); i++) {
    if(firstTime.secsTo(simplexResults[i].getTime()) < 0) {
      timeRefIndex = i;
      firstTime = simplexResults[i].getTime();
    }
  }

  // Find the last index for picking a center;
  int lastTimeIndex = 0;
  float longestTime = 0;
  for(int i = 0; i < simplexResults.count(); i++) {
    if(getMinutesTo(simplexResults[i].getTime()) > longestTime) {
      lastTimeIndex = i;
      longestTime = getMinutesTo(simplexResults[i].getTime());
      }
  }
  
  //Message::toScreen("Lat Lon Conversions and Time Resolved");
  
  float **newVariance = new float*[4];
  for(int m = 0; m < 4; m++) {
    newVariance[m] = new float[numLevels];
    for(int i = 0; i < numLevels; i++)
      newVariance[m][i] = 0;
  }
  newBestRadius = new int*[simplexResults.count()];
  newBestCenter = new int*[simplexResults.count()];
  for(int i = 0; i < simplexResults.count(); i++) {
    newBestRadius[i] = new int[numLevels];
    newBestCenter[i] = new int[numLevels];
    for(int j = 0; j < numLevels; j++) {
      newBestRadius[i][j] = 0;
      newBestCenter[i][j] = 0;
    }
  }

  for(int k = 0; k < numLevels; k++) {
    float xErrorSum = 0;
    float yErrorSum = 0;
    float radErrorSum = 0;
    float windErrorSum = 0;
    for(int i = 0; i < simplexResults.count(); i++) {
      float x = 0;
      float y = 0;
      float rad = 0;
      float wind = 0;
      float finalStd = 0;
      float confidence = 0;
      float stdError = 0;
      float min = ((float)firstTime.secsTo(simplexResults[i].getTime())/60.0);
      float othermin = getMinutesTo(simplexResults[i].getTime());
      //if(othermin != min)
      //  Message::toScreen("Angry time issues! - 799");
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
      // A whole bunch of stuff for forecasting that we are not dealing with
      float minError = 0;
      float totalError = 0;
      float minXError = x*x;
      float minYError = y*y;
      float minRadError = rad*rad;
      float minWindError = wind*wind;
      for(int j = 0; j < simplexResults[i].getNumRadii(); j++) {
	float xError, yError, radError, windError;
	int howManyReturnNull = 0;
	for(int l = 0; l < simplexResults[i].getNumPointsUsed(); l++) {
	  if(simplexResults[i].getCenter(k,j,l).isNull()) {
	    howManyReturnNull++;
	    //if(l%5==0)
	      //Message::toScreen("Center in simplexResults["+QString().setNum(i)+"] radius "+QString().setNum(j)+" level "+QString().setNum(k)+" centerIndex "+QString().setNum(l)+" is Null ?");
	  }
	  else {
	    Center currCenter = simplexResults[i].getCenter(k,j,l);
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
	      
	      int jBest = bestRadius[i][k];
	      float meanX=((simplexResults[i].getX(k,jBest)+x+currCenter.getX())/3.0);
	      float meanY=((simplexResults[i].getY(k,jBest)+y+currCenter.getY())/3.0);
	      finalStd = pow((simplexResults[i].getX(k, jBest)-meanX),2);
	      finalStd += pow((x-meanX), 2);
	      finalStd += pow((currCenter.getX()-meanX), 2);
	      finalStd += pow((simplexResults[i].getY(k, jBest)-meanY),2);
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
	//if(howManyReturnNull!=0)
	//Message::toScreen("Center in simplexResults["+QString().setNum(i)+"] radius "+QString().setNum(j)+" level "+QString().setNum(k)+" null count =  "+QString().setNum(howManyReturnNull)+" ?");
      }
      // new variables here but why?
      
      // get best simplex center;
      Center bestCenter = simplexResults[i].getCenter(k,newBestRadius[i][k],
							newBestCenter[i][k]);
      float xError, yError, radError, windError;

      //Message::toScreen("BestCenter is at level "+QString().setNum(k)+" radius index "+QString().setNum(newBestRadius[i][k])+" center index "+QString().setNum(newBestCenter[i][k])+" with a radius "+QString().setNum(bestCenter.getRadius())+" x = "+QString().setNum(bestCenter.getX())+" y = "+QString().setNum(bestCenter.getY()));

      xError = (x-bestCenter.getX())*(x-bestCenter.getX());
      yError = (y-bestCenter.getY())*(y-bestCenter.getY());
      radError = (rad-bestCenter.getRadius())*(rad-bestCenter.getRadius());
      //Message::toScreen("radError = "+QString().setNum(radError));
      windError = (wind-bestCenter.getMaxVT())*(wind-bestCenter.getMaxVT());
      xErrorSum += xError;
      yErrorSum += yError;
      radErrorSum += radError;
      windErrorSum += windError;
      
      if(i == lastTimeIndex) {
	//Message::toScreen("Vortex Data level "+QString().setNum(k)+" time "+vortexData->getTime().toString()+" radius = "+QString().setNum(rad)+" radError "+QString().setNum(radError));
	// if the volume we are looking at is the last one we will want to keep
	// all the info in vortexData
	
	int j = bestRadius[i][k];
	float centerLat = radarLat + y/fac_lat;
	float centerLon = radarLon + x/fac_lon;
	vortexData->setLat(k, centerLat);
	vortexData->setLon(k, centerLon);
	vortexData->setHeight(k, simplexResults[i].getHeight(k));
	vortexData->setRMW(k, rad);
	vortexData->setRMWUncertainty(k, radError);
	vortexData->setCenterStdDev(k, sqrt(xError*xError+yError*yError));
	vortexData->setNumConvergingCenters(k, simplexResults[i].getNumConvergingCenters(j,k));
      } 
      
    }
    
    // newVariance is the variance related to the fit of the line?
    float degree = bestFitDegree[3][k];
    newVariance[0][k]=xErrorSum/(simplexResults.count()-degree);
    newVariance[1][k]=yErrorSum/(simplexResults.count()-degree);
    newVariance[2][k]=radErrorSum/(simplexResults.count()-degree);
    newVariance[3][k]=windErrorSum/(simplexResults.count()-degree);
    // the newVariance is never used in the perl code of choose center,
    // and the variable degree is never initialized either, it is just left at
    // the last degree checked for the last function, probably wind
    // is this intentional or not??
    // my best guess is that degree will be set to the high degree for wind
    // so that is what I will set it to here
    
    /*
    
    // This part generates details on the average over all volumes
    // for each level examined - I am not sure exactly what these need to
    // be used for
    
    for(int i = 0; i <= simplexResults.count(); i++) {
      float radiusSum = 0;
      float xSum = 0;
      float ySum = 0;
      float centerDeviation = 0;
      float radiusDeviation = 0;
      for(int k = 0; k < numLevels; k++) {
	Center bestCenter = simplexResults[i].getCenter(k, newBestRadius[i][k],
							newBestCenter[i][k]);
	radiusSum += bestCenter.getRadius();
	xSum = bestCenter.getX();
	ySum = bestCenter.getY();
      }
      float radiusMean = radiusSum/numLevels;
      float xMean = xSum/numLevels;
      float yMean = ySum/numLevels;
      
      for(int k = 0; k < simplexResults[i].getNumLevels(); k++) {
	Center bestCenter = simplexResults[i].getCenter(k,newBestRadius[i][k],
							newBestCenter[i][k]);
	centerDeviation += pow((xMean - bestCenter.getX()),2);
	centerDeviation += pow((yMean - bestCenter.getY()),2);
	radiusDeviation +=pow((radiusMean -bestCenter.getRadius()),2);
      }
      centerDeviation = sqrt(centerDeviation/numLevels);
      radiusDeviation = sqrt(radiusDeviation/numLevels);
    }
    */ 
  }

  for(int i = 0; i < 4; i++)
    delete [] newVariance[i];
  delete [] newVariance;
  //Message::toScreen("Made it out of fixCenters");
  // I need to go through and figure out which variables are of some importance
  // after the program has run to completion
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
  
  int i = simplexResults.size() - 1;
  for(int k = 0; k < simplexResults[i].getNumLevels(); k++) {
	  int j = bestRadius[i][k];
	  float centerLat = radarLat + simplexResults[i].getY(j,k)/fac_lat;
	  float centerLon = radarLon + simplexResults[i].getX(j,k)/fac_lon;
	  vortexData->setLat(k, centerLat);
	  vortexData->setLon(k, centerLon);
	  vortexData->setHeight(k, simplexResults[i].getHeight(k));
	  vortexData->setRMW(k, simplexResults[i].getRadius(j));
	  vortexData->setRMWUncertainty(k, 1);
	  vortexData->setCenterStdDev(k, simplexResults[i].getCenterStdDev(k,j));
	  vortexData->setNumConvergingCenters(k, simplexResults[i].getNumConvergingCenters(j,k));
  } 
  
}

float ChooseCenter::getMinutesTo(const QDateTime &volTime)
{
  float min = ((float)firstTime.secsTo(volTime)/60.0);
  return min;
}
