/*
 *  Ray.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef RAY_H
#define RAY_H

class Ray
{

 public:
  Ray();
  ~Ray();
  void setTime(const int &value);
  void setDate(const int &value);
  void setAzimuth(const float &value);
  void setElevation(const float &value);
  void setVelResolution(const int &value);
  void setRayIndex(const int &value);
  void setSweepIndex(const int &value);
  void allocateRefData(const short int numGates);
  void allocateVelData(const short int numGates);
  void allocateSwData(const short int numGates);
  void setUnambig_range(const float &value);
  void setNyquist_vel(const float &value);
  void setFirst_ref_gate(const int &value);
  void setFirst_vel_gate(const int &value);

  /* These used to take int, I changed them to float, is there any reason
     they should be ints, I get the impression we are getting a value in feet
     meters ... etc -LM
  */
  void setRef_gatesp(const float &value);
  void setVel_gatesp(const float &value);  // in meters
  
  void setRef_numgates(const int &value);
  void setVel_numgates(const int &value);
  void setVcp(const int &value);
  void emptyRefgates(const short int numGates);

  void setRefData(float *buffer) { refData = buffer; };
  void setVelData(float *buffer) { velData = buffer; };
  void setSwData(float *buffer)  { swData = buffer; };
  
  int getTime();
  int getDate();
  float getAzimuth();
  float getElevation();
  int getVelResolution();
  int getRayIndex();
  int getSweepIndex();
  float* getRefData();
  float* getVelData();
  float* getSwData();
  float getUnambig_range();
  float getNyquist_vel();
  int getFirst_ref_gate();
  int getFirst_vel_gate();
  float getRef_gatesp();
  float getVel_gatesp();
  int getRef_numgates();
  int getVel_numgates();
  int getVcp();

  void dump();
  void dumpRef();
  void dumpVel();
  void dumpFloat(int size, float *buf);
  
 private:
  int sweepIndex;
  int time;
  int date;
  float azimuth;
  float elevation;
  int velResolution;
  int rayIndex;
  float *refData;
  float *velData;
  float *swData;
  float unambig_range;
  float nyquist_vel;
  int first_ref_gate;
  int first_vel_gate;

  /* These used to take int, I changed them to float, is there any reason
     they should be ints, I get the impression we are getting a value in feet
     meters ... etc -LM
  */

  float ref_gatesp;
  float vel_gatesp;
  
  int ref_numgates;
  int vel_numgates;
  int vcp;

};

#endif
