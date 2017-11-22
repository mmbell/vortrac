/*
 *  Ray.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "Ray.h"
#include "Message.h"

Ray::Ray()
{
  sweepIndex = -999;
  time = -999;
  date = -999;
  azimuth = -999;
  elevation = -999;
  velResolution = -999;
  rayIndex = -999;
  refData = NULL;
  velData = NULL;
  swData = NULL;
  unambig_range = -999;
  nyquist_vel = -999;
  first_ref_gate = -999;
  first_vel_gate = -999;
  ref_gatesp = -999;
  vel_gatesp = -999;
  ref_numgates = -999;
  vel_numgates = -999;
  vcp = -999;
}

Ray::~Ray()
{
  if (refData != NULL) delete [] refData;
  if (velData != NULL) delete [] velData;
  if (swData != NULL) delete [] swData;
}

void Ray::setTime(const int &value) {
  //  Message::toScreen("Does This Function Get Used? - setTime");
  time = value;
}

void Ray::setDate(const int &value) {
  //Message::toScreen("Does This Function Get Used? - set Date");
  date = value;
}

void Ray::setAzimuth(const float &value) {
  azimuth = value;
}

void Ray::setElevation(const float &value) {
  elevation = value;
}

void Ray::setVelResolution(const int &value) {
  velResolution = value;
}

void Ray::setRayIndex(const int &value) {
  rayIndex = value;
}

void Ray::setSweepIndex(const int &value) {
  sweepIndex = value;
}

void Ray::allocateRefData(const short int numGates) {
  refData = new float[numGates];
}

void Ray::allocateVelData(const short int numGates) {
  velData = new float[numGates];
}

void Ray::allocateSwData(const short int numGates) {
  swData = new float[numGates];
}

void Ray::setUnambig_range(const float &value) {
  unambig_range = value;
}

void Ray::setNyquist_vel(const float &value) {
  nyquist_vel = value;
}

void Ray::setFirst_ref_gate(const int &value) {
  first_ref_gate = value;
}

void Ray::setFirst_vel_gate(const int &value) {
  first_vel_gate = value;
}

void Ray::setRef_gatesp(const float &value) {
  ref_gatesp = value;
}

void Ray::setVel_gatesp(const float &value) {
  vel_gatesp = value;
}

void Ray::setRef_numgates(const int &value) {
  ref_numgates = value;
}

void Ray::setVel_numgates(const int &value) {
  vel_numgates = value;
}

void Ray::setVcp(const int &value) {
  vcp = value;
}

// Get values

int Ray::getTime() {
  Message::toScreen("Does this function get used? - getTime");
  return time;
}

int Ray::getDate() {
  return date;
}

float Ray::getAzimuth() {
  return azimuth;
}

float Ray::getElevation() {
  return elevation;
}

int Ray::getVelResolution() {
  return velResolution;
}

int Ray::getRayIndex() {
  return rayIndex;
}

int Ray::getSweepIndex() {
  return sweepIndex;
}

float* Ray::getRefData() {
  return refData;
}

float* Ray::getVelData() {
  return velData;
}

float* Ray::getSwData() {
  return swData;
}

float Ray::getUnambig_range() {
  return unambig_range ;
}

float Ray::getNyquist_vel() {
  return nyquist_vel ;
}

int Ray::getFirst_ref_gate() {
  return first_ref_gate ;
}

int Ray::getFirst_vel_gate() {
  return first_vel_gate ;
}

float Ray::getRef_gatesp() {
  return ref_gatesp ;
}

float Ray::getVel_gatesp() {
  return vel_gatesp ;
}

int Ray::getRef_numgates() {
  return ref_numgates ;
}

int Ray::getVel_numgates() {
  return vel_numgates ;
}

int Ray::getVcp() {
  return vcp ;
}

void Ray::emptyRefgates(const short int numGates) {
	if (ref_numgates == 0) {
	  allocateRefData(numGates);
	  for (int i = 0; i < numGates; i++) {
		  refData[i] = -999.0;
	  }
	  ref_numgates = numGates;
	}

}

void Ray::dump()
{
  std::cout << "------ Ray " << rayIndex << " --------" << std::endl;
  std::cout << "\tsweep Index: " << sweepIndex << std::endl;
  std::cout << "\t      time: " << time << std::endl;
  std::cout << "\t      date: " << date << std::endl;
  std::cout << "\t	azimuth: " << azimuth << std::endl;
  std::cout << "\t    elevation: " << elevation << std::endl;
  std::cout << "\tvelResolution: " << velResolution << std::endl;
  std::cout << "\t     rayIndex: " << rayIndex << std::endl;
  std::cout << "\tunambig range: " << unambig_range << std::endl;
  std::cout << "\t  nyquist_vel: " << nyquist_vel << std::endl;
  std::cout << "\t 1st ref gate: " << first_ref_gate << std::endl;
  std::cout << "\t 1st vel gate: " << first_vel_gate << std::endl;
  std::cout << "\t  ref gate sp: " << ref_gatesp << std::endl;
  std::cout << "\t  vel gate sp: " << vel_gatesp << std::endl;
  std::cout << "\t ref numgates: " << ref_numgates << std::endl;
  std::cout << "\t vel numgates: " << vel_numgates << std::endl;
  std::cout << "\t          vcp: " << vcp << std::endl;
  
}

void Ray::dumpFloat(int size, float *buf)
{

  for(int i = 0; i < size; i++) {
    if ( (i % 10) == 0)
      std::cout << std::endl << i << ":\t";
    std::cout << buf[i] << "\t";
  }
  std::cout << std::endl;
}

void Ray::dumpRef()
{
  int max = getRef_numgates();
  float *data = getRefData();
  dumpFloat(max, data);
}

void Ray::dumpVel()
{
  int max = getVel_numgates();
  float *data = getVelData();
  dumpFloat(max, data);
}
