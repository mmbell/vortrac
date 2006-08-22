/*
 *  Matrix.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/11/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef MATRIX_H
#define MATRIX_H

class Matrix
{

public:

  Matrix();
  ~Matrix();
  
  static bool lls(const int &numCoeff, const int &numData, 
		  float** &x, float* &y, 
		  float &stDeviation, float* &coeff, float* &stError);
  // Preforms a least squares regression on the velocity values
  // on the selected VAD ring to deduce the environmental wind

  static bool gaussJordan(float **a, float **b, int n, int m);
  // Preforms a least squares fit, this algorithm was borrowed from 
  // Numericical Recipes in C++ Second Edition,
  // Authors: 
  //       William H. Press       Willaim T. Vetterling
  //       Saul A. Teukolsky      Brian P. Flannery
  // Cambridge University Press: 2002,
  // Chapter 2.1 Gauss-Jordan Elimination

  static void printMatrix(float **A, int M, int N);
  static void printMatrix(float *A, int M);

private:
  bool matrixInverse(float **A, int M, int N, float** &Ainv);
  bool reduceRow(float **A,int M, int N);
  bool matrixMultiply(float **A, int MA, int NA, 
		      float **B, int MB, int NB, 
		      float **C, int MC, int NC);
  bool matrixMultiply(float **A, int MA, int NA, 
		      float *B, int MB, 
		      float *C, int MC);
  
};

#endif
