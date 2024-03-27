/*
 *  Matrix.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/11/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include <math.h>
#include <QString>
#include <QFile>
#include <QTextStream>
#include "Matrix.h"
#include "IO/Message.h"
#if QT_VERSION >= 0x060000
using namespace Qt;
#endif

Matrix::Matrix() 
{

}

Matrix::~Matrix()
{

}

bool Matrix::lls(const int &numCoeff,const int &numData,float** &x, float* &y, float &stDeviation, float* &coeff, float* &stError)
{
    /*
   * this function solve a problem xa=y
   * x is a matrix with numCoeff rows, and numData columns,
   * y is a matrix with numData rows,
   * coeff is the product containing the coefficient values (numCoeff rows)
   * stError is a product containing the estimated error for each coefficent in coeff, (numCoeff rows)
   * stDeviation is the estimated standard deviation of the regression
   *
   */

    if(numData < numCoeff) {
        //emit log(Message("Least Squares: Not Enough Data"));
        return false;
    }
    // We need at least one more data point than coefficient in order to
    // estimate the standard deviation of the fit.

    float** A = new float*[numCoeff];
    float** AA = new float*[numCoeff];
    float* B = new float[numCoeff];
    float** BB = new float*[numCoeff];
    for(int row = 0; row < numCoeff; row++) {
        A[row] = new float[numCoeff];
        AA[row] = new float[numCoeff];
        BB[row] = new float[1];
        for(int col = 0; col < numCoeff; col++) {
            A[row][col] = 0;
            AA[row][col] = 0;
            BB[row][0] = 0;
        }
        B[row] = 0;
        coeff[row] = 0;
    }

    // accumulate the covariances of all the data into the regression
    // matrices

    for(long i = 0; i < numData; i++) {
        for(int row = 0; row < numCoeff; row++) {
            for(int col = 0; col < numCoeff; col++) {
                A[row][col] +=(x[row][i]*x[col][i]);
                AA[row][col]+=(x[row][i]*x[col][i]);
            }
            B[row]     +=(x[row][i]*y[i]);
            BB[row][0] +=(x[row][i]*y[i]);
        }
    }


    float** Ainv = new float*[numCoeff];
    for(int p = 0; p < numCoeff; p++)
        Ainv[p] = new float[numCoeff];

    //The C++ Recipes Code Works so All this can be done away with

    /*
  // find the inverse of A
  if(!matrixInverse(A, numCoeff, numCoeff, Ainv)) {
    Message::toScreen("lls: matrix inverse failed");
    return false;
  }
  
  // use the inverse of A to find coeff
  if(!matrixMultiply(Ainv, numCoeff, numCoeff, B, numCoeff, coeff, numCoeff)) {
    Message::toScreen("lls: Matrix Multiply Failed");
    return false;
  }
  */


    //Message::toScreen("Printing AA");
    //printMatrix(AA, numCoeff, numCoeff);

    //Message::toScreen("Printing BB");
    //printMatrix(BB, numCoeff, 1);


    if(!gaussJordan(AA,BB, numCoeff, 1)) {
        // emit log(Message("Least Squares Fit Failed"));
        return false;
    }


    //printMatrix(BB, numCoeff, 1);

    /*
  Message::toScreen("CHECK: coeff[0] = "+QString().setNum(coeff[0])+" coeff[1] = "
   +QString().setNum(coeff[1])+" coeff[2] = "
   +QString().setNum(coeff[2]));
  Message::toScreen("     : BB[0][0] = "+QString().setNum(BB[0][0])+" BB[1][0] = "
   +QString().setNum(BB[1][0])+" BB[2][0] = "
   +QString().setNum(BB[2][0]));
  */

    for(int i = 0; i < numCoeff; i++) {
        coeff[i] = BB[i][0];
        for(int j = 0; j < numCoeff; j++) {
            Ainv[i][j] = AA[i][j];
        }
    }

    // calculate the stDeviation and stError
    float sum = 0;
    for(long i = 0; i < numData; i++) {
        float regValue = 0;
        for(int j = 0; j < numCoeff; j++) {
            regValue += coeff[j]*x[j][i];
        }
        sum +=((y[i]-regValue)*(y[i]-regValue));
    }


    if(numData!=numCoeff)
        stDeviation = sqrt(sum/float(numData-long(numCoeff)));
    else
        stDeviation = sqrt(sum);

    // Added this to avoid getting bad numbers, don't know if it is statistically
    // correct, made note of it and will check later - LM (Aug 13)

    // calculate the standard error for the coefficients

    for(int i = 0; i < numCoeff; i++) {
        stError[i] = stDeviation*sqrt(fabs(Ainv[i][i]));
    }

    // Clean up
    for(int row = 0; row < numCoeff; row++) {
        delete[] A[row];
        delete[] AA[row];
        delete[] BB[row];
    }
    delete[] A;
    delete[] AA;
    delete[] B;
    delete[] BB;

    for(int p = 0; p < numCoeff; p++)
        delete[] Ainv[p];
    delete[] Ainv;

    return true;
} 

bool Matrix::oldlls(const int &numCoeff,const long &numData, 
                    float** &x, float* &y,
                    float &stDeviation, float* &coeff, float* &stError,
                    QString& fileName)
{
    /*
   * x is a matrix with numCoeff rows, and numData columns,
   * y is a matrix with numData rows,
   * coeff is the product containing the coefficient values (numCoeff rows)
   * stError is a product containing the estimated error for each
   *  coefficent in coeff, (numCoeff rows)
   * stDeviation is the estimated standard deviation of the regression
   *
   */

    if(numData < long(numCoeff)) {
        //emit log(Message("Least Squares: Not Enough Data"));
        return false;
    }
    // We need at least one more data point than coefficient in order to
    // estimate the standard deviation of the fit.

    float** A = new float*[numCoeff];
    float** AA = new float*[numCoeff];
    float* B = new float[numCoeff];
    float** BB = new float*[numCoeff];
    for(int row = 0; row < numCoeff; row++) {
        A[row] = new float[numCoeff];
        AA[row] = new float[numCoeff];
        BB[row] = new float[1];
        for(int col = 0; col < numCoeff; col++) {
            A[row][col] = 0;
            AA[row][col] = 0;
            BB[row][0] = 0;
        }
        B[row] = 0;
        coeff[row] = 0;
    }

    // accumulate the covariances of all the data into the regression
    // matrices

    for(long i = 0; i < numData; i++) {
        for(int row = 0; row < numCoeff; row++) {
            for(int col = 0; col < numCoeff; col++) {
                A[row][col]+=(x[row][i]*x[col][i]);
                AA[row][col]+=(x[row][i]*x[col][i]);
            }
            B[row] +=(x[row][i]*y[i]);
            BB[row][0] +=(x[row][i]*y[i]);
        }
    }

    QFile* outputFile = new QFile(fileName);
    outputFile->open(QIODevice::WriteOnly);
    QTextStream out(outputFile);
    out << "Matrix A"<< endl << endl;
    for(int ll = 0; ll < numCoeff;ll++) {
        QString printLine = QString();
        for(int mm = 0; mm < numCoeff; mm++)
            printLine = printLine+QString().setNum(A[ll][mm])+QString(" ");
        out << printLine << endl;
    }
    out << endl;
    out << "Matrix B" << endl << endl;
    for(int ll = 0; ll < numCoeff;ll++) {
        out << B[ll] << endl;
    }
    out << endl;
    outputFile->close();

    float** Ainv = new float*[numCoeff];
    for(int p = 0; p < numCoeff; p++) {
        Ainv[p] = new float[numCoeff];
        for(int pp = 0; pp < numCoeff; pp++)
            Ainv[p][pp] = 0;
    }

    //The C++ Recipes Code Works so All this can be done away with


    // find the inverse of A
    if(!matrixInverse(A, numCoeff, numCoeff, Ainv)) {
        Message::toScreen("lls: matrix inverse failed");
        return false;
    }

    // use the inverse of A to find coeff
    if(!matrixMultiply(Ainv, numCoeff, numCoeff, B, numCoeff, coeff, numCoeff)) {
        Message::toScreen("lls: Matrix Multiply Failed");
        return false;
    }



    //Message::toScreen("Printing AA");
    //printMatrix(AA, numCoeff, numCoeff);

    //Message::toScreen("Printing BB");
    //printMatrix(BB, numCoeff, 1);

    /*
  if(!gaussJordan(AA,BB, numCoeff, 1)) {
    // emit log(Message("Least Squares Fit Failed"));
    return false;
  }
  */

    //printMatrix(BB, numCoeff, 1);

    /*
  Message::toScreen("CHECK: coeff[0] = "+QString().setNum(coeff[0])+" coeff[1] = "
   +QString().setNum(coeff[1])+" coeff[2] = "
   +QString().setNum(coeff[2]));
  Message::toScreen("     : BB[0][0] = "+QString().setNum(BB[0][0])+" BB[1][0] = "
   +QString().setNum(BB[1][0])+" BB[2][0] = "
   +QString().setNum(BB[2][0]));
  */

    /*

  for(int i = 0; i < numCoeff; i++) {
    coeff[i] = BB[i][0];
    for(int j = 0; j < numCoeff; j++) {
      Ainv[i][j] = AA[i][j];
    }
  }
  */
    // calculate the stDeviation and stError
    float sum = 0;
    for(long i = 0; i < numData; i++) {
        float regValue = 0;
        for(int j = 0; j < numCoeff; j++) {
            regValue += coeff[j]*x[j][i];
        }
        sum +=((y[i]-regValue)*(y[i]-regValue));
    }


    if(numData!=long(numCoeff))
        stDeviation = sqrt(sum/float(numData-long(numCoeff)));
    else
        stDeviation = sqrt(sum);

    // Added this to avoid getting bad numbers, don't know if it is statistically
    // correct, made note of it and will check later - LM (Aug 13)

    // calculate the standard error for the coefficients

    for(int i = 0; i < numCoeff; i++) {
        stError[i] = stDeviation*sqrt(fabs(Ainv[i][i]));
    }

    // Clean up
    for(int row = 0; row < numCoeff; row++) {
        delete[] A[row];
        delete[] AA[row];
        delete[] BB[row];
    }
    delete[] A;
    delete[] AA;
    delete[] B;
    delete[] BB;

    for(int p = 0; p < numCoeff; p++)
        delete[] Ainv[p];
    delete[] Ainv;

    return true;
} 

bool Matrix::matrixInverse(float **A, int M, int N, float** &Ainv)
{
    if(M!=N) {
        // emit log(Message("matrixInverse: Bad Demensions M!=N"));
        return false;
    }

    //Message::toScreen("Matrix before inverting");
    //printMatrix(A, M, N);

    for(int p = 0; p < M; p++) {
        for(int q = 0; q < M; q++)
            Ainv[p][q] = 0.0;
    }

    float **temp;
    temp = new float*[M];
    for(int p = 0; p < M; p++) {
        temp[p] = new float[2*M];
        for(int q = 0; q < 2*M; q++)
            temp[p][q] = 0.0;
    }

    for(int i = 0; i < M; i++) {
        for(int j = 0; j < M; j++) {
            temp[i][j] = A[i][j];
        }
        for(int j = M; j < 2*M; j++) {
            if(i == (j-M))
                temp[i][j]=1.0;
            else
                temp[i][j]=0.0;
        }
    }

    if(!reduceRow(temp, M, 2*M)) {
        // emit log(Message("matrixInverse: reduceRow failed"));
        for(int p = 0; p < M; p++)
            delete[] temp[p];
        delete[] temp;
        return false;
    }

    for(int i = 0; i < M; i++) {
        for(int j = 0; j < M; j++) {
            Ainv[i][j] = temp[i][j+M];
        }
    }

    for(int i = 0; i < M; i++) {
        for(int j = 0; j < M; j++) {
            if(i==j){
                if(temp[i][j]!=1.0) {
                    // emit log(Message("matrixInverse:WARNING:Matrix May Be Singular! :("));
                    for(int p = 0; p < M; p++)
                        delete[] temp[p];
                    delete[] temp;
                    return false;
                }
            }
            else{
                if(temp[i][j]!=0.0) {
                    // emit log(Message("matrixInverse:WARNING:Matrix May Be Singular! :<>"));
                    for(int p = 0; p < M; p++)
                        delete[] temp[p];
                    delete[] temp;
                    return false;
                }
            }
        }
    }

    //Message::toScreen("Matrix after inverting");
    //printMatrix(Ainv, M, M);
    for(int p = 0; p < M; p++)
        delete[] temp[p];
    delete[] temp;

    return true;
}

bool Matrix::reduceRow(float **A, int M, int N)
{

    /* Passed basic tests, need more thorough testing with tricky zeros included
   * Single implementation of gaussian jordan elimination
   * Architecture designed by Paul Harasti
   */

    //  printMatrix(A, M, N);
    float X, W;
    bool *allZero = new bool[N];

    for(int col = 0; col < N; col++) {
        allZero[col] = true;
    }

    for(int col = 0; col < N; col++)
    {
        for(int row = 0; row < M; row++)
        {
            if(A[row][col]!=0)
                allZero[col]=false;
        }
    }
    int colStart = 0;
    int rowStart = 0;
    while(colStart < N && rowStart < M)
    {
        int pivotRow = 0;
        float Y = 0;
        float Z = 0;
        if(allZero[colStart]) {
            colStart++;
            if (colStart == N) {
                //emit log(Message("rowReduce:WARNING:Matrix May Be Singular! AllZero"));
                delete[] allZero;
                return false;
            }
        }
        else {
            X =0;
            for(int row = rowStart; row < M; row++) {
                W = 0;
                for(int col = colStart; col < N; col++){
                    if(fabs(A[row][col])> W) {
                        W = fabs(A[row][col]);
                    }
                }
                if(W!=0) {
                    //Message::toScreen("Pivot value W: "+QString().setNum(W));
                    if(fabs(A[row][colStart])/W > X) {
                        X = fabs(A[row][colStart])/W;
                        //if(X < nearZero)
                        //Message::toScreen("Matrix:rowReduce:WARNING:Matrix May Be Singular!");
                        pivotRow = row;
                        //Message::toScreen("Pivot Row = "+QString().setNum(row));
                    }
                }
            }
        }
        //printMatrix(A, M, N);
        if (pivotRow!=rowStart) {
            for(int col = colStart; col < N; col++) {
                float temp = A[rowStart][col];
                A[rowStart][col] = A[pivotRow][col];
                A[pivotRow][col] = temp;
            }
        }
        //printMatrix(A, M, N);
        Y = A[rowStart][colStart];
        A[rowStart][colStart] = 1.0;
        if (colStart+1<N) {
            for(int col = colStart+1; col < N; col++) {
                A[rowStart][col] = A[rowStart][col]/Y;
            }
        }
        //printMatrix(A, M, N);
        for(int row = 0; row < M; row++) {
            if(row!=rowStart) {
                Z = A[row][colStart];
                A[row][colStart] = 0;
                if(colStart+1 < N) {
                    for(int col = colStart+1; col < N; col++) {
                        A[row][col] = A[row][col] - A[rowStart][col]*Z;
                    }
                }
            }
            //printMatrix(A, M, N);
        }
        rowStart++;
        colStart++;
    }
    //Message::toScreen("Out of Row Reduce");
    delete[] allZero;

    return true;
}

bool Matrix::matrixMultiply(float **A, int MA, int NA, 
                            float **B, int MB, int NB,
                            float **C, int MC, int NC)
{
    if(NA != MB || MA!=MC || NB!=NC)
        return false;

    float sum;
    for(int icol = 0; icol < NB; icol++) {
        for(int irow = 0; irow < MA; irow++) {
            sum = 0;
            for(int p = 0; p < NA; p++) {
                sum +=A[irow][p]*B[p][icol];
                /*
 Message::toScreen("C"+QString().setNum(sum)+" += A ("
         +QString().setNum(A[irow][p])+")*B("+
         QString().setNum(B[p][icol])+")");
 */
            }
            C[irow][icol] = sum;
            /*
      Message::toScreen("C["+QString().setNum(irow)+"]["+QString().setNum(icol)+
       "] = "+QString().setNum(sum));
      */
        }
    }
    return true;

}

bool Matrix::matrixMultiply(float **A, int MA, int NA, 
                            float *B, int MB,
                            float *C, int MC)
{
    if(NA != MB || MA!=MC)
        return false;

    float sum;
    for(int irow = 0; irow < MA; irow++) {
        sum = 0;
        for(int p = 0; p < NA; p++) {
            sum +=A[irow][p]*B[p];
        }
        C[irow] = sum;
    }

    return true;

}

bool Matrix::gaussJordan(float **a, float **b, int n, int m)
{

    // get this to do what lls does using cpp recipes
    // Gauss-Jordan Elemination
    // a = nxn coefficient matrix
    // b = nxm matrix

    int i, icol, irow, j, k, l, ll;
    float big, temp, pivinv;

    int indxc[n], indxr[n], ipiv[n];

    for(j = 0; j < n; j++) {
        ipiv[j] = 0;
    }

    for (i = 0; i < n; i++) {
        big = 0.0;
        icol = 0;
        irow = 0;
        for (j = 0;j < n; j++) {
            if(ipiv[j]!=1) {
                for (k = 0; k < n; k++) {
                    if (ipiv[k] ==0) {
                        if(fabs(a[j][k]) >= big) {
                            big = fabs(a[j][k]);
                            irow = j;
                            icol = k;
                        }
                    }
                }
            }
        }
        ++(ipiv[icol]);
        if(irow != icol) {
            for(l = 0; l < n; l++) {
                float dummy = a[irow][l];
                a[irow][l] = a[icol][l];
                a[icol][l] = dummy;
            }
            for(l = 0; l < m; l++) {
                float dummy = b[irow][l];
                b[irow][l] = b[icol][l];
                b[icol][l] = dummy;
            }
        }
        indxr[i] = irow;
        indxc[i] = icol;
        if (a[icol][icol]==0.0) {
            return false;
        }
        pivinv = 1/a[icol][icol];
        a[icol][icol] = 1.0;
        for(l = 0; l < n; l++) {
            a[icol][l] *=pivinv;
        }
        for(l = 0; l < m; l++) {
            b[icol][l] *= pivinv;
        }
        for(ll = 0; ll < n; ll++) {
            if(ll != icol) {
                temp = a[ll][icol];
                a[ll][icol] = 0.0;
                for(l = 0; l < n; l++) {
                    a[ll][l] -= a[icol][l]*temp;
                }
                for(l = 0; l < m; l++) {
                    b[ll][l] -= b[icol][l]*temp;
                }
            }
        }
    }
    for( l = n-1; l >= 0; l--) {
        if(indxr[l] != indxc[l]) {
            for(k = 0; k < n; k++) {
                float dummy = a[k][indxr[l]];
                a[k][indxr[l]] = a[k][indxc[l]];
                a[k][indxc[l]] = dummy;
            }
        }
    }


    return true;
}

void Matrix::printMatrix(float **A, int M, int N)
{
    for(int i = 0;i < M; i++ ) {
        QString print;
        for(int j = 0; j < N; j++) {
            print += (QString().setNum(A[i][j])+QString(", " ));
        }
        Message::toScreen(print);
    }
    Message::toScreen("-----------------------------------");
}

void Matrix::printMatrix(float *A, int M)
{   
    QString print;
    for(int j = 0; j < M; j++) {
        print += (QString().setNum(A[j])+QString(", " ));
    }
    Message::toScreen(print);
    Message::toScreen("-----------------------------------");
}
