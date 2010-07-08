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
  //delete [] refData;
  //delete [] velData;
  //delete [] swData;
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

void Ray::setRefData(float *data) {
  refData = data;
}

void Ray::setVelData(float *data) {
  velData = data;
}

void Ray::setSwData(float *data) {
  swData = data;
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
  Message::toScreen("Does this function get used? - getDate");
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
