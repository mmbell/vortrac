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
}

ChooseCenter::~ChooseCenter()
{
  delete config;
  
  for(int i = 0; i < simplexResults.count(); i++) {
    for(int j = 0; j < simplexResults[i].getNumRadii(); j++) {
      delete[] score[i][j];
    }
    delete[] score[i];
  }
  delete score;
  
  // We might not be able to delete these depending on weither they 
  // are passed out for further use or not after the ChooseCenter object is
  // distroyed

  for(int i = 0; i < simplexResults.count(); i++) {
    delete[] bestRadius[i];
    delete[] newBestRadius[i];
    delete[] newBestCenter[i];    
  }
  for(int m = 0; m < 4; m++) {
    for(int p = 0; p < simplexResults[0].getNumLevels(); p++) {
      delete[] bestFitCoeff[m][p];
    }
    delete[] bestFitCoeff[m];
    delete[] bestFitDegree[m];
    delete[] bestFitVariance[m];
  }
  delete[] centerDev;
  delete[] radiusDev;
  delete[] bestRadius;
  delete[] bestFitCoeff;
  
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
  //simplexResults.timeSort();   make sure this works first
  //Message::toScreen("ChooseCenter: Initializing Things");
  initialize();
  //Message::toScreen("ChooseCenter: Try Choose Mean Centers");
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
      Message::toScreen("Choose Center failed to Construct Polynomial");
      if(!fixCenters()) {
	Message::toScreen("Choose Center failed to find individual centers w/o fit");
	return false;
      }
    }
    else {
      useLastMean();
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
  
  minVolumes = 3;

  QDomElement ccElement = config->getConfig("choosecenter");
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
    for(int rad = 0; rad < simplexResults[i].getNumRadii(); rad++) 
      score[i][rad] = new float[simplexResults[i].getNumLevels()];

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
	peakWinds = winds;
      }
      
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

      //Message::toScreen("Made it to means");

      // calculate the mean radius and center scores over all levels
      float numLevels = simplexResults[i].getNumLevels(); 
      // what is the current level index verses height issue
      float meanRadius = radiusSum/numLevels;
      float meanXChoose = xSum/numLevels;
      float meanYChoose = ySum/numLevels;
      
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
	float bestScore = 0;
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
      centerDev[i] = 0;
      radiusDev[i] = 0;
      
      for(int k = 0; k <= simplexResults[i].getNumLevels(); k++) {
	int j = bestRadius[i][k];
	//Message::toScreen("simplexResults["+QString().setNum(i)+"].getNumRadii() = "+QString().setNum(simplexResults[i].getNumRadii()));
	//Message::toScreen("Best Radius (j = "+QString().setNum(j)+")");
	//Message::toScreen(" has value ("+QString().setNum(simplexResults[i].getRadius(j))+")");
	
	float radLength = simplexResults[i].getRadius(j);
	radiusDev[i] +=(meanRadius-radLength)*(meanRadius-radLength);
	centerDev[i] +=sqrt(centerDev[i]/numLevels);
	// Do these get put in the simplexData???
      }
    }
  } 
  return true; // ??? assign this other options........
}



bool ChooseCenter::constructPolynomial()
{
  int maxPoly = 0;

  // We want to find the eariliest time for this process;
  // we don't want to use (simplexList)sort this late in the game 
  // because it might ruin the member arrays
  QDateTime startTime = simplexResults[0].getTime();
  float timeRefIndex = 0;
  for(int i = 1; i < simplexResults.count(); i++) {
    if(simplexResults[i].getTime() < startTime) {
      timeRefIndex = i;
      startTime = simplexResults[i].getTime();
    }
  }

  // Construct a least squares polynomial to fit track
  // Export variance for analysis
  
  float bestFitVariance[4][simplexResults[0].getNumLevels()];
  float bestFitDegree[4][simplexResults[0].getNumLevels()];
  // Things are stored here so far but never used

  for(int k = 0; k < simplexResults[0].getNumLevels(); k++) {
    // had to pick a simplex data to get num levels from, this use requires 
    // that they all be the same, we can do some checking of this prior
    // using the height variables and total indexes to decide where 
    // there is overlap if volumes are pulled from multiple different
    // configurations

    for(int criteria = 0; criteria < 4; criteria++) {
      // Iterates through the procedure for the four different
      // curve fitting criteria
      
      // What should the highest order basis polynomial;
      if(simplexResults.count() > 20)
	maxPoly = 20;
      else 
	maxPoly = simplexResults.count()-1;

      if(maxPoly < 3){
	return false;
	// Expand this to get variables in the right places
	// So the next member function can gracefully handle things
      }

      float squareVariance[maxPoly];
      // This goes from 0 to less than maxPoly, because this is the number
      // of fits that we will examine the variance of.

      float bestFitCoeff[4][simplexResults[0].getNumLevels()][maxPoly+1];
      // Things are stored here so far but it is never used
      
      float *lastFitCoeff;
      lastFitCoeff = new float[maxPoly+1];
      float *currentCoeff; 
      currentCoeff = new float[maxPoly+1];
      
      for(int n = 1; n <=maxPoly; n++) {
	for(int m = 1; m < n; m++) {
	  // Copy the last set into the lastFit array
	  lastFitCoeff[m]= currentCoeff[m];
	  currentCoeff[m] = 0;
	}
	currentCoeff[n] = 0;
	// degree = n; what - no one uses this and it is the same as best degree
	float* B = new float[n+1];
	float** M = new float*[n+1];
	for(int r = 0; r <= n; r++) {
	  M[r] = new float[n+1];
	  for(int c = 0; c <= n; c++) {
	    float sum = 0;
	    for(int i = 0; i <simplexResults.count(); i++) {
	      float min = ((float)startTime.secsTo(simplexResults[i].getTime())/60.0);
	      sum += pow(min,(double)(r+c));
	    }
	    M[r][c] = sum;
	  }
	  float sum = 0;
	  for(int i = 0; i <simplexResults.count(); i++) {
	    float min = ((float)startTime.secsTo(simplexResults[i].getTime())/60.0);
	    int jBest = bestRadius[i][k];
	    float y;
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
	    sum += pow(min, r)*y;
	  }
	  B[r] = sum;
	}
	Matrix mathTool;
	float stDev;
	float *stError = new float[n];
	if(!mathTool.lls(n, n, M, B, stDev, currentCoeff, stError)) {
	  //  log(Message("least square fit failed in find center"));
	}
	float errorSum = 0;
	for(int i = 0; i < simplexResults.count(); i++) {
	  float min = ((float)startTime.secsTo(simplexResults[i].getTime())/60.0);
	  float func_y  = 0;
	  for(int m = 0; m <=n; m++) {
	    func_y += currentCoeff[m]*pow(min,m);
	  }
	  int jBest = bestRadius[i][k];
	  switch(criteria) {
	  case 0:
	    errorSum = pow(func_y-simplexResults[i].getX(jBest, k), 2); break;
	  case 1:
	    errorSum = pow(func_y-simplexResults[i].getY(jBest, k), 2); break;
	  case 2:
	    errorSum = pow(func_y-simplexResults[i].getRadius(jBest),2); break;
	  case 3:
	    errorSum = pow(func_y-simplexResults[i].getMaxVT(jBest, k), 2); 
	    break;
	  }
	    
	  int degFreedom = simplexResults.count()-n;
	  squareVariance[n] = errorSum/(float)degFreedom;
	  if(n > 1) {
	    if(squareVariance[n] > squareVariance[n-1]) {
	      // we found the best fit!!!!
	      bestFitVariance[criteria][k] = squareVariance[n-1];
	      bestFitDegree[criteria][k] = n-1;
	      // degree = n-1; phasing out
	      // Now we have to revert back to the old set
	      // This is why everything is kept in storage
	      for(int l = 0; l <= (n-1); l++) {
		bestFitCoeff[criteria][k][l] = lastFitCoeff[l];
	      }
	    }
	    else {
	      if(n == maxPoly) {
		// Maxed out
		bestFitVariance[criteria][k] = squareVariance[n];
		bestFitDegree[criteria][k] = n;
		for(int l = 0; l < n; l++) {
		  bestFitCoeff[criteria][k][l] = currentCoeff[l];
		}
	      }
	      else {
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
	    // Check to see if this really goes here???
	    // brackets are scary
	    // Doing a linear fit with only 3 points
	    bestFitVariance[criteria][k] = squareVariance[n];
	    bestFitDegree[criteria][k] = n;
	    // degree = n; phasing out
	    for(int l = 0; l < n; l++) {
	      bestFitCoeff[criteria][k][l] = currentCoeff[l];
	    }
	  }
	}
      }
    }
  }
  return true;
}



bool ChooseCenter::fixCenters()
{
  float **newVariance = new float*[4];
  for(int m = 0; m < 4; m++) 
    newVariance[m] = new float[simplexResults[0].getNumLevels()];

  for(int k = 0; k < simplexResults[0].getNumLevels(); k++) {
    float xErrorSum = 0;
    float yErrorSum = 0;
    float radErrorSum = 0;
    float windErrorSum = 0;
    for(int i = 0; i < simplexResults.count(); i++) {
      // should these guys be float?
      float x = 0;
      float y = 0;
      float rad = 0;
      float wind = 0;
      float finalStd = 0;
      float confidence = 0;
      float stdError = 0;
      float min = ((float)startTime.secsTo(simplexResults[i].getTime())/60.0);
      for(int n = 0; n < bestFitDegree[0][k]; n++) {
	x += bestFitCoeff[0][k][n]*pow(min,n);
      }
      for(int n = 0; n < bestFitDegree[1][k]; n++) {
	y += bestFitCoeff[1][k][n]*pow(min,n);
      }
      for(int n = 0; n < bestFitDegree[2][k]; n++) {
	rad += bestFitCoeff[2][k][n]*pow(min,n);
      }
      for(int n = 0; n < bestFitDegree[3][k]; n++) {
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
	for(int l = 0; l < simplexResults[i].getNumPointsUsed(); l++) {
	  if(!simplexResults[i].getCenter(j,k,l).isNull()) {
	    Center currCenter = simplexResults[i].getCenter(j,k,l);
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
	// new variables here but why?
	// check to make sure that all the ending } are in the right place here
	
	// get best simplex center;
	Center bestCenter = simplexResults[i].getCenter(k,newBestRadius[i][k],
							newBestCenter[i][k]);
	
	xError = (x-bestCenter.getX())*(x-bestCenter.getX());
	yError = (y-bestCenter.getY())*(y-bestCenter.getY());
	radError = (rad-bestCenter.getRadius())*(rad-bestCenter.getRadius());
	windError = (wind-bestCenter.getMaxVT())*(wind-bestCenter.getMaxVT());
	xErrorSum += xError;
	yErrorSum += yError;
	radErrorSum += radError;
	windErrorSum += windError;
      }
    }
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
  }
  for(int i = 0; i <= simplexResults.count(); i++) {
    float radiusSum = 0;
    float xSum = 0;
    float ySum = 0;
    float centerDeviation = 0;
    float radiusDeviation = 0;
    for(int k = 0; k < simplexResults[i].getNumLevels(); k++) {
      Center bestCenter = simplexResults[i].getCenter(k, newBestRadius[i][k],
						      newBestCenter[i][k]);
      radiusSum += bestCenter.getRadius();
      xSum = bestCenter.getX();
      ySum = bestCenter.getY();
    }
    float radiusMean = radiusSum/simplexResults[0].getNumLevels();
    float xMean = xSum/simplexResults[0].getNumLevels();
    float yMean = ySum/simplexResults[0].getNumLevels();

    for(int k = 0; k < simplexResults[i].getNumLevels(); k++) {
      Center bestCenter = simplexResults[i].getCenter(k,newBestRadius[i][k],
						      newBestCenter[i][k]);
      centerDeviation += pow((xMean - bestCenter.getX()),2);
      centerDeviation += pow((yMean - bestCenter.getY()),2);
      radiusDeviation +=pow((radiusMean -bestCenter.getRadius()),2);
    }
    centerDeviation = sqrt(centerDeviation/simplexResults[0].getNumLevels());
    radiusDeviation = sqrt(radiusDeviation/simplexResults[0].getNumLevels());
  }
  // I need to go through and figure out which variables are of some importance
  // after the program has run to completion
  return true;
}

bool ChooseCenter::fixCentersNoFit()
{
  float **newVariance = new float*[4];
  for(int m = 0; m < 4; m++) 
    newVariance[m] = new float[simplexResults[0].getNumLevels()];

  for(int k = 0; k < simplexResults[0].getNumLevels(); k++) {
    float xErrorSum = 0;
    float yErrorSum = 0;
    float radErrorSum = 0;
    float windErrorSum = 0;
    for(int i = 0; i < simplexResults.count(); i++) {
      // should these guys be float?
      float x = 0;
      float y = 0;
      float rad = 0;
      float wind = 0;
      float finalStd = 0;
      float confidence = 0;
      float stdError = 0;
      float min = ((float)startTime.secsTo(simplexResults[i].getTime())/60.0);
      //for(int n = 0; n < bestFitDegree[0][k]; n++) {
      x += bestFitCoeff[0][k][0];
      //}
      //for(int n = 0; n < bestFitDegree[1][k]; n++) {
      y += bestFitCoeff[1][k][0];
      //}
      //for(int n = 0; n < bestFitDegree[2][k]; n++) {
      rad += bestFitCoeff[2][k][0];
      //}
      //for(int n = 0; n < bestFitDegree[3][k]; n++) {
      wind += bestFitCoeff[3][k][0];
      //}
      // A whole bunch of stuff for forecasting that we are not dealing with
      float minError = 0;
      float totalError = 0;
      float minXError = x*x;
      float minYError = y*y;
      float minRadError = rad*rad;
      float minWindError = wind*wind;
      for(int j = 0; j < simplexResults[i].getNumRadii(); j++) {
	float xError, yError, radError, windError;
	for(int l = 0; l < simplexResults[i].getNumPointsUsed(); l++) {
	  if(!simplexResults[i].getCenter(j,k,l).isNull()) {
	    Center currCenter = simplexResults[i].getCenter(j,k,l);
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
	// new variables here but why?
	// check to make sure that all the ending } are in the right place here
	
	// get best simplex center;
	Center bestCenter = simplexResults[i].getCenter(k,newBestRadius[i][k],
							newBestCenter[i][k]);
	
	xError = (x-bestCenter.getX())*(x-bestCenter.getX());
	yError = (y-bestCenter.getY())*(y-bestCenter.getY());
	radError = (rad-bestCenter.getRadius())*(rad-bestCenter.getRadius());
	windError = (wind-bestCenter.getMaxVT())*(wind-bestCenter.getMaxVT());
	xErrorSum += xError;
	yErrorSum += yError;
	radErrorSum += radError;
	windErrorSum += windError;
      }
    }
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
  }
  for(int i = 0; i <= simplexResults.count(); i++) {
    float radiusSum = 0;
    float xSum = 0;
    float ySum = 0;
    float centerDeviation = 0;
    float radiusDeviation = 0;
    for(int k = 0; k < simplexResults[i].getNumLevels(); k++) {
      Center bestCenter = simplexResults[i].getCenter(k, newBestRadius[i][k],
						      newBestCenter[i][k]);
      radiusSum += bestCenter.getRadius();
      xSum = bestCenter.getX();
      ySum = bestCenter.getY();
    }
    float radiusMean = radiusSum/simplexResults[0].getNumLevels();
    float xMean = xSum/simplexResults[0].getNumLevels();
    float yMean = ySum/simplexResults[0].getNumLevels();

    for(int k = 0; k < simplexResults[i].getNumLevels(); k++) {
      Center bestCenter = simplexResults[i].getCenter(k,newBestRadius[i][k],
						      newBestCenter[i][k]);
      centerDeviation += pow((xMean - bestCenter.getX()),2);
      centerDeviation += pow((yMean - bestCenter.getY()),2);
      radiusDeviation +=pow((radiusMean -bestCenter.getRadius()),2);
    }
    centerDeviation = sqrt(centerDeviation/simplexResults[0].getNumLevels());
    radiusDeviation = sqrt(radiusDeviation/simplexResults[0].getNumLevels());
  }
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
	  vortexData->setCenterStdDev(k, simplexResults[i].getCenterStdDev(j,k));
	  vortexData->setNumConvergingCenters(k, simplexResults[i].getNumConvergingCenters(j,k));
  } 
  
}
