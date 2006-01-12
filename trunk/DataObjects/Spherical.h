/*
 *  Spherical.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/11/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#ifndef SPHERICAL_H
#define SPHERICAL_H

class Spherical : public GriddedData
{

 public:
  Spherical();
  Spherical(const int &initYdim, const int &initYdim, const int &initZdim,
	    const float &initXgridsp, const float &initYgridsp, 
	    const float &initZgridsp);
  ~Spherical();

