/*
 *  Sweep.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef SWEEP_H
#define SWEEP_H

class Sweep
{

public:
  Sweep();
  ~Sweep();
  void setSweepIndex(const int &value);
  void setElevation(const float &value);
  void setUnambig_range(const float &value);
  void setNyquist_vel(const float &value);
  void setFirst_ref_gate(const int &value);
  void setFirst_vel_gate(const int &value);

  /* These used to take int, I changed them to float, is there any reason
     they should be ints, I get the impression we are getting a value in feet
     meters ... etc -LM
  */
  void setRef_gatesp(const float &value);
  void setVel_gatesp(const float &value);
  
  void setRef_numgates(const int &value);
  void setVel_numgates(const int &value);
  void setVcp(const int &value);
  void setFirstRay(const int &value);
  void setLastRay(const int &value);

  int getSweepIndex();
  float getElevation();
  float getUnambig_range();
  float getNyquist_vel();
  int getFirst_ref_gate();
  int getFirst_vel_gate();
  int getVel_numgates();
  int getRef_numgates();
  
  /* These used to take int, I changed them to float, is there any reason
     they should be ints, I get the impression we are getting a value in feet
     meters ... etc -LM
  */
  float getRef_gatesp();
  float getVel_gatesp();
 
  int getVcp();
  int getFirstRay();
  int getLastRay();
  int getNumRays();

  void dump();
  
private:
  int sweepIndex;
  float elevation;
  float unambig_range;
  float nyquist_vel;
  int first_ref_gate;
  int first_vel_gate;
  float ref_gatesp;
  float vel_gatesp;
  int ref_numgates;
  int vel_numgates;
  int vcp;
  int firstRay;
  int lastRay;

};

#endif
