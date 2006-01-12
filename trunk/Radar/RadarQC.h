/*
 *  RadarQC.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/11/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef RADARQC_H
#define RADARQC_H

#include "RadarData.h"
#include "Message.h"
#include <QWidget>
#include <QDomElement>
#include <QObject>

class RadarQC : public QObject
{ 

 Q_OBJECT

 public:
  RadarQC(RadarData *radarPtr = 0, QObject *parent = 0);
  ~RadarQC();
  
  RadarData* getRadarData() {return radarData;}
  void getConfig(QDomElement qcConfig);
  // Retreves user parameters from the XML configuration file

  bool dealias();
  // Preforms basic quality control algorithms and velocity dealiasing
  // on a single radar volume
  
 private:
  
  RadarData *radarData;
  // Volume of Radar Data to be dealiased

  float specWidthLimit;
  float velThresholdLimit;
  // Parameters used to threshold radar data prior to dealiasing

  float *envWind; 
  float *envDir;  
  // Speed and Direction of mean environmental wind determined by 
  // VAD algorithms, user input, or related AWIPS data

  int maxFold;
  int numVGatesAveraged;
  // User specified parameters used in velocity dealiasing

  int vadLevels;
  int numCoEff;
  // User specified parameters used in VAD algorithms

  QString AWIPSDir;
  // Location of relavent AWIPS data to be used in determining mean
  // environmental winds.

  QString wind_method;
  // Parameter indicating which method will be used to determine environmenal
  // winds, the default method is VAD algorithms

  bool useVADWinds;
  bool useUserWinds;
  bool useAWIPSWinds;

  float deg2rad;
  // Degrees to radians conversion factor
  float velNull;
  // Value that indicates a null, or unuseful doppler velocity value


  void thresholdData();
  // Primary Quality Control
  // Eliminates velocity value within the data that do not meet the user
  // specified values for velocity and spectral width

  bool terminalVelocity();
  // Uses reflectivity data to approximate the terminal velocity component
  // for each gate, these values are subtracted from the doppler velocity 
  // readings in the radar volume

  bool findEnvironmentalWind();
  // Provides environmental wind according to user specified methods
  
  bool findVADStart(bool useGVAD);
  // This method is used to run either the VAD or GVAD algorithms including
  // any intiailization or preparation algorithms
  
  void vadPrep();
  // This method is used to initialize many VAD variables

  bool VAD(float* &vel, Sweep* &currentSweep, 
	   float &speed, float &direction, float &rms);
  bool GVAD(float* &vel, Sweep* &currentSweep, 
	    float &speed, float &direction, float &rms);
  // These methods determine environmental crosswinds used to initiate the
  // Bargen and Brown process

  float getStart(Ray* currentRay);
  // Returns the environmental wind to be used for a specific ray 
  // in the dealiasing algorithm

  bool BB();
  // The method used the Bargen and Brown (1980) velocity dealiasing method
  // algorithm B
 
  float findHeight(Ray* currentRay, int gateIndex);
  // Uses the 4/3 earth radius model to return the height of a specific gate

  float bilinear(float value_high, float value_low, 
		 float deltaH_high, float deltaH_low);
  // Takes a weighted average of two values relative to their distance
  // from the point of interest
  

  bool lls(const int &numCoEff, const int &numData, 
	   float** &x, float* &y, 
	   float &stDeviation, float* &coEff, float* &stError);
  // Preforms a least squares regression on the velocity values
  // on the selected VAD ring to deduce the environmental wind


  bool leastSquaresRegression(float **a, float **b, int n, int m);
  // Preforms a least squares fit, this algorithm was borrowed from 
  // Numericical Recipes in C++ Second Edition,
  // Authors: 
  //       William H. Press       Willaim T. Vetterling
  //       Saul A. Teukolsky      Brian P. Flannery
  // Cambridge University Press: 2002,
  // Chapter 2.1 Gauss-Jordan Elimination


  void crazyCheck();
  bool matrixInverse(float **A, int M, int N, float** &Ainv);
  bool reduceRow(float **A,int M, int N);
  bool matrixMultiply(float **A, int MA, int NA, 
		      float **B, int MB, int NB, 
		      float **C, int MC, int NC);
  bool matrixMultiply(float **A, int MA, int NA, 
		      float *B, int MB, 
		      float *C, int MC);
  void printMatrix(float **A, int M, int N);
  void printMatrix(float *A, int M);


  // Variables used by VAD
  float **validBinCount, **last_count_up, **last_count_low;
  bool *vadFound, **hasVelData;
  float *sumwt, *vadRMS;
  int **highVelGate, **lowVelGate; 
  bool grad_vad, vad_found, gvad_found;
  int thr, vadthr, gvadthr;

 public slots:
  void catchLog(const Message& message);

 signals:
  void log(const Message& message);

};


#endif
