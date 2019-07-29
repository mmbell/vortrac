/*
 * GriddedData.cpp
 * VORTRAC
 *
 * Created by Michael Bell on 07/11/05
 * Copyright 2005 University Corporation for Atmospheric Research.
 * All rights reserved.
 *
 */


// Generic gridded data class
// To be subclassed with Cartesian, Earth, Radar,and Cylindrical
// Coordinate systems

#include "GriddedData.h"
#include "Message.h"
#include <cmath>

GriddedData::GriddedData()
{

    Pi = 3.141592653589793238462643;
    deg2rad = Pi/180.;
    rad2deg = 180./Pi;

    // Setting default values
    sphericalRangeSpacing = 1.0;
    sphericalAzimuthSpacing = 1.0;
    sphericalElevationSpacing = 1.0;
    cylindricalRadiusSpacing = 1.0;
    cylindricalAzimuthSpacing = 1.0;
    cylindricalHeightSpacing = 1.0;

    refPointI = 0;
    refPointJ = 0;
    refPointK = 0;
    originLat = 0;
    originLon = 0;

    iGridsp = 0;
    jGridsp = 0;
    kGridsp = 0;

    // TODO:
    kDisplayIndex = 0;
}

GriddedData::~GriddedData()
{

}

void GriddedData::writeAsi()
{
    Message::toScreen("Using unimplemented functions from GriddedData to try to write to unnamed file ");
}

bool GriddedData::writeAsi(const QString& fileName)
{
    Message::toScreen("Using unimplemented functions from GriddedData to try to write to file "+fileName);
    return false;
}

void GriddedData::setLatLonOrigin(float *knownLat, float *knownLon, float *relX, float *relY)
{
    // takes a Lat Lon point and its cooresponding grid coordinates in km
    // to set zero Lat Lon coordinates for later use.

    // Thanks to Peter Dodge for some code used here
    float knownLatRadians = *knownLat * acos(-1.0) / 180.0;
    float fac_lat = 111.13209 - 0.56605 * cos(2.0 * knownLatRadians)+ 0.00012 * cos(4.0 * knownLatRadians) - 0.000002 * cos(6.0 * knownLatRadians);
    float fac_lon = 111.41513 * cos(knownLatRadians)- 0.09455 * cos(3.0 * knownLatRadians) + 0.00012 * cos(5.0 * knownLatRadians);
    originLon = *knownLon - *relX / fac_lon;
    originLat = *knownLat - *relY / fac_lat;

    // testing Message::toScreen("Set Zero: ZeroLat = "+QString().setNum(zeroLat)+" ZeroLon = "+QString().setNum(zeroLon));
}

float GriddedData::fixAngle(float angle) {
    // Takes and angle in radians and puts it in the 0-2Pi range

    float fixangle = angle;
    if (fabs(angle) < 0.000001) { fixangle=0.; }
    if (angle > (2.* Pi))
    { fixangle = angle-(2.*Pi); }
    if (angle < 0.)
    { fixangle=angle+(2.*Pi); }
    return (fixangle);
}

void GriddedData::setReferencePoint(int ii, int jj, int kk)
{
    // Sets the reference point based on the index rather than the
    // assigned position relative to the storm

    //Message::toScreen("GriddedData: Using the Index Based Reference Assignment");

    if((ii > iDim)||(ii < 0)||(jj > jDim)||(jj < 0)||(kk > kDim)||(kk < 0))
        //Message::toScreen("GriddedData: trying to examine point outside cappi for setReferencePoint: i = "+QString().setNum(ii)+" j = "+QString().setNum(jj)+" k = "+QString().setNum(kk));
        refPointI = ii;
    refPointJ = jj;
    refPointK = kk;
}

void GriddedData::setCartesianReferencePoint(float ii, float jj, float kk)
{
    // The reference point is coming in in km
    refPointI = int(floor((ii- xmin)/iGridsp+.5));
    refPointJ = int(floor((jj -ymin)/jGridsp+.5));
    refPointK = int(floor((kk -zmin)/kGridsp+.5));
    //  Message::toScreen("idim = "+QString().setNum(iDim)+" jdim "+QString().setNum(jDim)+" kdim "+QString().setNum(kDim));
    //  Message::toScreen("refPointI = "+QString().setNum(refPointI)+" refPointJ = "+QString().setNum(refPointJ)+" refPointK = "+QString().setNum(refPointK));
    //  Message::toScreen("iGridsp = "+QString().setNum(iGridsp)+" jGridsp = "+QString().setNum(jGridsp)+" kGridSp = "+QString().setNum(kGridsp));
    /* if((refPointI > iDim)||(refPointI < 0) ||
     (refPointJ > jDim)||(refPointJ < 0) ||
     (refPointK > kDim)||(refPointK < 0))
    Message::toScreen("GriddedData: trying to examine point outside cappi for setCartesianReferencePoint: i = "+QString().setNum(ii)+" j = "+QString().setNum(jj)+" k = "+QString().setNum(kk)+" iRef = "+QString().setNum(refPointI)+" jRef = "+QString().setNum(refPointJ)+" kRef = "+QString().setNum(refPointK));
  */
    /*
   * Why is this giving errors here? I thought we set this up so that we
   * attached the radar position with the radar lat lon which is typically
   * our of cappi range?
   *
   */
}

// TODO What if Lat and/or Lon is -999. refPoint* will be non-sensical

void GriddedData::setAbsoluteReferencePoint(float Lat, float Lon, float Height) 
{
    // Overloaded version of setCartesianReferencePoint used when Latitude and
    // Longitude data is known.

  if(Lat == -999)
    std::cout << "** GriddedData::setAbsoluteReferencePoint: Lat is -999" << std::endl;
  if(Lon == -999)
    std::cout << "** GriddedData::setAbsoluteReferencePoint: Lon is -999" << std::endl;

  // This assumes that the originLat and originLon are the radar coordinates.
  
  float *locations = getCartesianPoint(&originLat, &originLon, &Lat, &Lon);
  
    // Floor is used to round to the nearest integer

    refPointI = int(floor((locations[0] - xmin) / iGridsp + .5));
    refPointJ = int(floor((locations[1] - ymin) / jGridsp + .5));
    refPointK = int(floor((Height - zmin) / kGridsp + .5));
    // testing Message::toScreen("I = "+QString().setNum(refPointI)+" J = "+QString().setNum(refPointJ)+" K = "+QString().setNum(refPointK));
    delete[] locations;
}

float* GriddedData::getCartesianPoint(float *Lat, float *Lon, float *relLat, float *relLon)
{
    // Returns the distance between the two (Lat,Lon) points in km
    // Thanks to Peter Dodge for some code used here

    float LatRadians = *Lat * acos(-1.0) / 180.0;
    float fac_lat = 111.13209 - 0.56605 * cos(2.0 * LatRadians) + 0.00012 * cos(4.0 * LatRadians) - 0.000002 * cos(6.0 * LatRadians);
    float fac_lon = 111.41513 * cos(LatRadians) - 0.09455 * cos(3.0 * LatRadians) + 0.00012 * cos(5.0 * LatRadians);

    float relX = (*relLon - *Lon) * fac_lon;
    float relY = (*relLat - *Lat) * fac_lat;

    float *relArray = new float[2];
    relArray[0] = relX;
    relArray[1] = relY;
    return relArray;
}

float GriddedData::getCartesianDistance(float Lat, float Lon, float relLat, float relLon)
{
    // Thanks to Peter Dodge for some code used here
    float LatRadians = Lat * acos(-1.0)/180.0;
    float fac_lat = 111.13209 - 0.56605 * cos(2.0 * LatRadians)+ 0.00012 * cos(4.0 * LatRadians) - 0.000002 * cos(6.0 * LatRadians);
    float fac_lon = 111.41513 * cos(LatRadians)- 0.09455 * cos(3.0 * LatRadians) + 0.00012 * cos(5.0 * LatRadians);

    float relX = (relLon - Lon) * fac_lon;
    float relY = (relLat - Lat) * fac_lat;
    float dist = sqrt(relX * relX + relY * relY);
    return dist;

    // This value is returned in KM ???? -LM
}

float* GriddedData::getAdjustedLatLon(const float startLat, const float startLon, const float changeInX, const float changeInY)
{
    // This function acts like the inverse of the one above.
    // If it is given an initial latitude and longitude
    // and the cartesian distance change (km) in each direction
    // it will return a new pair of points {newLat, newLon}

    float LatRadians = startLat * acos(-1.0)/180.0;
    float fac_lat = 111.13209 - 0.56605 * cos(2.0 * LatRadians)
            + 0.00012 * cos(4.0 * LatRadians) - 0.000002 * cos(6.0 * LatRadians);
    float fac_lon = 111.41513 * cos(LatRadians)
            - 0.09455 * cos(3.0 * LatRadians) + 0.00012 * cos(5.0 * LatRadians);
    float* newLatLon = new float[2];
    newLatLon[0] = changeInY/fac_lat +startLat;
    newLatLon[1] = changeInX/fac_lon +startLon;
    return newLatLon;
}


// These functions return indices
float GriddedData::getRefPointI ()
{
    return refPointI;
}

float GriddedData::getRefPointJ ()
{
    return refPointJ;
}

float GriddedData::getRefPointK ()
{
    return refPointK;
}

// These functions return cartesian points
// mins are all in km 
float GriddedData::getCartesianRefPointI ()
{
    return refPointI * iGridsp + xmin;
}

float GriddedData::getCartesianRefPointJ ()
{
    return refPointJ * jGridsp + ymin;
}

float GriddedData::getCartesianRefPointK ()
{
    return refPointK * kGridsp + zmin;
}

// These functions convert between indices and cartesian points

float GriddedData::getCartesianPointFromIndexI (const float& indexI) const
{
    //	return (indexI + xmin)*iGridsp;  xmin is in km -LM 2/24/07
    if((indexI >= 0)&&(indexI < iDim)) {
        float distance =  indexI*iGridsp+xmin;
        if((distance >= xmin) && (distance <= xmax)) {
            return distance;
        }
        Message::toScreen("GriddedData: getCartesianPointFromIndexI: trying to return a distance that is not within the range contained by dataGrid.... "+QString().setNum(distance)+" is not within "+QString().setNum(xmin)+" - "+QString().setNum(xmax));
        return -999.;
    }
    Message::toScreen("GriddedData: getCartesianPointFromIndexI: trying to access an index that is not within the range contained by dataGrid.... "+QString().setNum(indexI)+" is not within 0 - "+QString().setNum(iDim));
    return -999.;
}

float GriddedData::getCartesianPointFromIndexJ (const float& indexJ) const
{
    //return (indexJ + ymin)*jGridsp;
  
    if((indexJ >= 0)&&(indexJ < jDim)) {
        float distance =  indexJ*jGridsp+ymin;
        if((distance >= ymin) && (distance <= ymax)) {
            return distance;
        }
        Message::toScreen("GriddedData: getCartesianPointFromIndexJ: trying to return a distance that is not within the range contained by dataGrid.... "+QString().setNum(distance)+" is not within "+QString().setNum(ymin)+" - "+QString().setNum(ymax));
        return -999.;
    }
    Message::toScreen("GriddedData: getCartesianPointFromIndexJ: trying to access an index that is not within the range contained by dataGrid.... "+QString().setNum(indexJ)+" is not within 0 - "+QString().setNum(jDim));
    return -999.;
}

float GriddedData::getCartesianPointFromIndexK (const float& indexK) const
{
    //	return (indexK + zmin)*kGridsp;
    if((indexK >= 0) && (indexK < kDim)) {
        float distance =  indexK * kGridsp + zmin;
        if((distance >= zmin) && (distance <= zmax)) {
            return distance;
        }
        Message::toScreen("GriddedData: getCartesianPointFromIndexK: trying to return a distance that is not within the range contained by dataGrid.... "+QString().setNum(distance)+" is not within "+QString().setNum(zmin)+" - "+QString().setNum(zmax));
        return -999.;
    }
    Message::toScreen("GriddedData: getCartesianPointFromIndexK: trying to access an index that is not within the range contained by dataGrid.... "+QString().setNum(indexK)+" is not within 0 - "+QString().setNum(kDim));
    return -999.;
}

// These functions convert between cartesian points and indices

int GriddedData::getIndexFromCartesianPointI (const float& cartI) const
{

    // Takes a distance position in km in the grids coordinate system
    //   (where mins & maxs are in km and the radar is the zero point
    //    while the grid zero is set to the radar location, the stored
    //    area is centered about the vortex center guess for visibility)
    // and returns the index most closely related to that location.

    //  return (cartI/iGridsp) - xmin;  // removed 2/24/07 -LM
    if((cartI < xmin)||(cartI > xmax)){
        Message::toScreen("GriddedData: getIndexFromCartesianPointI: Attempting to access non-existant point..... "+QString().setNum(cartI)+" km the available gridded range is "+QString().setNum(xmin)+" - "+QString().setNum(xmax)+" km ");
        return -999;
    }
    int index = int(floor(((cartI - xmin)/iGridsp)+.5));
    if((index >= iDim)||(index < 0)) {
        Message::toScreen("GriddedData: getIndexFromCartesianPointI: Attempting to return an index outside of the dimensional range of the grid... "+QString().setNum(index)+" should be within the range 0 - "+QString().setNum(iDim));
        return -999;
    }

    return index;
    // added 2/24/07 -LM xmin is given units of km in CappiGrid:
    // member function gridRadarData... so we must unscale it as well
    // I am going to make the same changes to J & K functions below

}

int GriddedData::getIndexFromCartesianPointJ (const float& cartJ) const
{
    // return (cartJ/jGridsp) - ymin;  // removed 2/24/07 -LM

    if((cartJ < ymin)||(cartJ > ymax)){
        Message::toScreen("GriddedData: getIndexFromCartesianPointJ: Attempting to access non-existant point..... "+QString().setNum(cartJ)+" km the available gridded range is "+QString().setNum(ymin)+" - "+QString().setNum(ymax)+" km ");
        return -999;
    }
    int index = int(floor(((cartJ - ymin)/jGridsp)+.5));
    if((index >= jDim)||(index < 0)) {
        Message::toScreen("GriddedData: getIndexFromCartesianPointJ: Attempting to return an index outside of the dimensional range of the grid... "+QString().setNum(index)+" should be within the range 0 - "+QString().setNum(jDim));
        return -999;
    }

    return index;
}

int GriddedData::getIndexFromCartesianPointK (const float& cartK) const
{
    //return (cartK/kGridsp) - zmin;  // removed 2/24/07 -LM

    if((cartK < zmin)||(cartK > zmax)){
        Message::toScreen("GriddedData: getIndexFromCartesianPointK: Attempting to access non-existant point..... "+QString().setNum(cartK)+" km the available gridded range is "+QString().setNum(zmin)+" - "+QString().setNum(zmax)+" km ");
        return -999;
    }
    int index = int(floor(((cartK - zmin)/kGridsp)+.5));
    if((index >= kDim)||(index < 0)) {
        Message::toScreen("GriddedData: getIndexFromCartesianPointK: Attempting to return an index outside of the dimensional range of the grid... "+QString().setNum(index)+" should be within the range 0 - "+QString().setNum(kDim));
        return -999;
    }

    return index;
}

int GriddedData::getFieldIndex(const QString& fieldName) const
{
    int field = -1;
    if((fieldName == "dz")||(fieldName == "DZ"))
        field = 0;
    if((fieldName == "ve") || (fieldName == "VE") || (fieldName == "VU") ) // TODO: VU?
        field = 1;
    if((fieldName == "ht")||(fieldName == "HT"))
        field = 2;
    if(field == -1)
        Message::toScreen("GriddedData::GetFieldIndex: Failed to Select From Available Fields");
    return field;
}

float GriddedData::getIndexValue(QString& fieldName, float& ii, float& jj, float& kk) const
{

    // This returns a value from dataGrid based on an index rather than
    // a point on the defined cartesian grid in km.
    // It is a simple accessor function.

    if((ii > iDim)||(ii < 0)||(jj > jDim)||(jj < 0)||(kk > kDim)||(kk < 0))
        return -999.;
    int field = getFieldIndex(fieldName);
    return dataGrid[field][(int)ii][(int)jj][(int)kk];

}

float* GriddedData::getCartesianXslice(const QString& fieldName, 
                                       const float& y, const float& z)
{
    /*
   * Returns a list of interpolated vales to match the fieldName,y,z values
   * given for the entire range x values with the gridded data. The number
   * of points returned should be equivalent to the dimension in the
   * unspecified coordinate direction, in this case the x direction.
   *
   */

    int field = getFieldIndex(fieldName);
    float* values = new float[(int)iDim];

    float jjIndex = getIndexFromCartesianPointJ(y);
    float kkIndex = getIndexFromCartesianPointK(z);
    int jjMin = int(floor(jjIndex));
    int jjMax = int(floor(jjIndex)+1);
    int kkMin = int(floor(kkIndex));
    int kkMax = int(floor(kkIndex)+1);
    float jjMinDiff = jjIndex - jjMin;
    float jjMaxDiff = jjMax - jjIndex;
    float kkMinDiff = kkIndex - kkMin;
    float kkMaxDiff = kkMax - kkIndex;

    for(int i = 0; i < iDim; i++) {
        float ave = 0;
        ave += (1-jjMaxDiff)*(1-kkMinDiff)*dataGrid[field][i][jjMax][kkMin];
        ave += (1-jjMinDiff)*(1-kkMinDiff)*dataGrid[field][i][jjMin][kkMin];
        ave += (1-jjMaxDiff)*(1-kkMaxDiff)*dataGrid[field][i][jjMax][kkMax];
        ave += (1-jjMinDiff)*(1-kkMaxDiff)*dataGrid[field][i][jjMin][kkMax];
        values[i] = ave;
    }
    return values;
}

float* GriddedData::getCartesianYslice(const QString& fieldName,
                                       const float& x, const float& z)
{
    /*
   * Returns a list of interpolated vales to match the fieldName,x,z values
   * given for the entire range y values with the gridded data. The number
   * of points returned should be equivalent to the dimension in the
   * unspecified coordinate direction, in this case the y direction.
   *
   */

    int field = getFieldIndex(fieldName);
    float* values = new float[(int)jDim];

    float iiIndex = getIndexFromCartesianPointI(x);
    float kkIndex = getIndexFromCartesianPointK(z);
    int iiMin = int(floor(iiIndex));
    int iiMax = int(floor(iiIndex)+1);
    int kkMin = int(floor(kkIndex));
    int kkMax = int(floor(kkIndex)+1);

    float iiMinDiff = iiIndex - iiMin;
    float iiMaxDiff = iiMax - iiIndex;
    float kkMinDiff = kkIndex - kkMin;
    float kkMaxDiff = kkMax - kkIndex;

    for(int j = 0; j < jDim; j++) {
        float ave = 0;
        ave += (1-iiMinDiff)*(1-kkMaxDiff)*dataGrid[field][iiMin][j][kkMax];
        ave += (1-iiMaxDiff)*(1-kkMaxDiff)*dataGrid[field][iiMax][j][kkMax];
        ave += (1-iiMinDiff)*(1-kkMinDiff)*dataGrid[field][iiMin][j][kkMin];
        ave += (1-iiMaxDiff)*(1-kkMinDiff)*dataGrid[field][iiMax][j][kkMin];
        values[j] = ave;
    }
    return values;
}

float* GriddedData::getCartesianZslice(const QString& fieldName,
                                       const float& x, const float& y)
{
    /*
   * Returns a list of interpolated vales to match the fieldName,x,y values
   * given for the entire range z values with the gridded data. The number
   * of points returned should be equivalent to the dimension in the
   * unspecified coordinate direction, in this case the z direction.
   *
   */

    int field = getFieldIndex(fieldName);
    float* values = new float[(int)kDim];

    float jjIndex = getIndexFromCartesianPointJ(y);
    float iiIndex = getIndexFromCartesianPointI(x);
    int jjMin = int(floor(jjIndex));
    int jjMax = int(floor(jjIndex)+1);
    int iiMin = int(floor(iiIndex));
    int iiMax = int(floor(iiIndex)+1);
    float jjMinDiff = jjIndex - jjMin;
    float jjMaxDiff = jjMax - jjIndex;
    float iiMinDiff = iiIndex - iiMin;
    float iiMaxDiff = iiMax - iiIndex;

    for(int k = 0; k < kDim; k++) {
        float ave = 0;
        ave += (1-jjMinDiff)*(1-iiMaxDiff)*dataGrid[field][iiMax][jjMin][k];
        ave += (1-jjMaxDiff)*(1-iiMaxDiff)*dataGrid[field][iiMax][jjMax][k];
        ave += (1-jjMinDiff)*(1-iiMinDiff)*dataGrid[field][iiMin][jjMin][k];
        ave += (1-jjMaxDiff)*(1-iiMinDiff)*dataGrid[field][iiMin][jjMax][k];
        values[k] = ave;
    }
    return values;
}

float GriddedData::getCartesianValue(const QString& fieldName, const float& x, 
                                     const float& y, const float& z)
{

    /*
   * This returns the field value associated with the fieldName that
   * matches the x & y & z coordinates (km) most closely, interpolating between
   * data points.
   *
   */

    int field = getFieldIndex(fieldName);

    float jjIndex = getIndexFromCartesianPointJ(y);
    float iiIndex = getIndexFromCartesianPointI(x);
    float kkIndex = getIndexFromCartesianPointK(z);
    int kkMin = int(floor(kkIndex));
    int kkMax = int(floor(kkIndex)+1);
    int jjMin = int(floor(jjIndex));
    int jjMax = int(floor(jjIndex)+1);
    int iiMin = int(floor(iiIndex));
    int iiMax = int(floor(iiIndex)+1);
    float kkMinDiff = kkIndex - kkMin;
    float kkMaxDiff = kkMax -kkIndex;
    float jjMinDiff = jjIndex - jjMin;
    float jjMaxDiff = jjMax - jjIndex;
    float iiMinDiff = iiIndex - iiMin;
    float iiMaxDiff = iiMax - iiIndex;

    float ave = 0;
    ave += (1-jjMinDiff)*(1-iiMaxDiff)*(1-kkMinDiff)*dataGrid[field][iiMax][jjMin][kkMin];
    ave += (1-jjMaxDiff)*(1-iiMaxDiff)*(1-kkMinDiff)*dataGrid[field][iiMax][jjMax][kkMin];
    ave += (1-jjMinDiff)*(1-iiMinDiff)*(1-kkMinDiff)*dataGrid[field][iiMin][jjMin][kkMin];
    ave += (1-jjMaxDiff)*(1-iiMinDiff)*(1-kkMinDiff)*dataGrid[field][iiMin][jjMax][kkMin];
    ave += (1-jjMinDiff)*(1-iiMaxDiff)*(1-kkMaxDiff)*dataGrid[field][iiMax][jjMin][kkMax];
    ave += (1-jjMaxDiff)*(1-iiMaxDiff)*(1-kkMaxDiff)*dataGrid[field][iiMax][jjMax][kkMax];
    ave += (1-jjMinDiff)*(1-iiMinDiff)*(1-kkMaxDiff)*dataGrid[field][iiMin][jjMin][kkMax];
    ave += (1-jjMaxDiff)*(1-iiMinDiff)*(1-kkMaxDiff)*dataGrid[field][iiMin][jjMax][kkMax];
    return ave;

}

int GriddedData::getSphericalRangeLength(float azimuth, float elevation)
{
    int count = 0;

    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j ++) {
            for (int k = 0; k < kDim; k++) {
                float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
                if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.))
                        && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {

                    // QString test("pElevation " +QString().setNum( pElevation )+ " elevation " +QString().setNum( elevation )+ "sphericalElevationSpacing " +QString().setNum( sphericalElevationSpacing )+ "\n");
                    //Message::toScreen(test);
                    if((pElevation <=(elevation+sphericalElevationSpacing/2.))
                            && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
                        count++;
                    }
                }
            }
        }
    }
    return count;
}

float* GriddedData::getSphericalRangeData(QString& fieldName, float azimuth, 
                                          float elevation)
{
    int numPoints = getSphericalRangeLength(azimuth, elevation);
    return getSphericalRangeData(fieldName, azimuth, elevation, numPoints);
}

float* GriddedData::getSphericalRangeData(QString& fieldName, float azimuth, 
                                          float elevation, int numPoints)
{    
    int field = getFieldIndex(fieldName);
    float *values = new float[numPoints];

    int count = 0;
    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j++) {
            for (int k = 0; k < kDim; k++) {
                float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
                if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.))
                        && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {
                    if((pElevation <=(elevation+sphericalElevationSpacing/2.))
                            && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
                        values[count] = dataGrid[field][i][j][k];
                        count++;
                    }
                }
            }
        }
    }
    return values;
}

float* GriddedData::getSphericalRangePosition(float azimuth, float elevation)
{
    int numPoints = getSphericalRangeLength(azimuth, elevation);
    return getSphericalRangePosition(azimuth,elevation,numPoints);
}

float* GriddedData::getSphericalRangePosition(float azimuth, float elevation,
                                              int numPoints)
{
    float *positions = new float[numPoints];

    int count = 0;
    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j++) {
            for (int k = 0; k < kDim; k++) {
                float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
                if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.))
                        && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {
                    if((pElevation <=(elevation+sphericalElevationSpacing/2.))
                            && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
                        positions[count] = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
                        count++;
                    }
                }
            }
        }
    }
    return positions;
}


int GriddedData::getSphericalAzimuthLength(float range, float elevation)
{
    int count = 0;

    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j ++) {
            for (int k = 0; k < kDim; k++) {
                float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
                float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
                if((r <= (range+sphericalRangeSpacing/2.))
                        && (r > (range-sphericalRangeSpacing/2.))) {
                    if((pElevation <=(elevation+sphericalElevationSpacing/2.))
                            && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
                        count++;
                    }
                }
            }
        }
    }
    return count;
}

float* GriddedData::getSphericalAzimuthData(QString& fieldName, 
                                            float range, float elevation)
{
    int numPoints = getSphericalAzimuthLength(range, elevation);
    int field = getFieldIndex(fieldName);
    float *values = new float[numPoints];

    int count = 0;
    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j ++) {
            for (int k = 0; k < kDim; k++) {
                float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
                float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
                if((r <= (range+sphericalRangeSpacing/2.))
                        && (r > (range-sphericalRangeSpacing/2.))) {
                    if((pElevation <=(elevation+sphericalElevationSpacing/2.))
                            && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
                        values[count] = dataGrid[field][i][j][k];
                        count++;
                    }
                }
            }
        }
    }
    return values;
}

float* GriddedData::getSphericalAzimuthPosition(float range, float elevation)
{
    int numPoints = getSphericalAzimuthLength(range, elevation);
    float *positions = new float[numPoints];

    int count = 0;
    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j ++) {
            for (int k = 0; k < kDim; k++) {
                float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
                float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
                if((r <= (range+sphericalRangeSpacing/2.))
                        && (r > (range-sphericalRangeSpacing/2.))) {
                    if((pElevation <=(elevation+sphericalElevationSpacing/2.))
                            && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
                        float azimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                        positions[count] = azimuth;
                        count++;
                    }
                }
            }
        }
    }
    return positions;
}

int GriddedData::getSphericalElevationLength(float range, float azimuth)
{
    int count = 0;

    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j ++) {
            for (int k = 0; k < kDim; k++) {
                float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
                float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.))
                        && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {
                    if((r <= (range+sphericalRangeSpacing/2.))
                            && (r > (range-sphericalRangeSpacing/2.))) {
                        count++;
                    }
                }
            }
        }
    }
    return count;

}

float* GriddedData::getSphericalElevationData(QString& fieldName, float range, 
                                              float azimuth)
{
    int numPoints = getSphericalElevationLength(range, azimuth);
    int field = getFieldIndex(fieldName);
    float *values = new float[numPoints];

    int count = 0;
    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j ++) {
            for (int k = 0; k < kDim; k++) {
                float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
                float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.))
                        && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {
                    if((r <= (range+sphericalRangeSpacing/2.))
                            && (r > (range-sphericalRangeSpacing/2.))) {
                        values[count] = dataGrid[field][i][j][k];
                        count++;
                    }
                }
            }
        }
    }
    return values;
}

float* GriddedData::getSphericalElevationPosition(float range, float azimuth)
{
    int numPoints = getSphericalElevationLength(range, azimuth);
    float *positions = new float[numPoints];

    int count = 0;
    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j ++) {
            for (int k = 0; k < kDim; k++) {
                float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
                float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.))
                        && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {
                    if((r <= (range+sphericalRangeSpacing/2.))
                            && (r > (range-sphericalRangeSpacing/2.))) {
                        positions[count] = fixAngle(atan2((k-refPointK),rp))*rad2deg;
                        count++;
                    }
                }
            }
        }
    }
    return positions;
}


int GriddedData::getCylindricalRadiusLength(float azimuth, float height)
{
    int count = 0;

    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j ++) {
            for(int k = 0; k < kDim; k ++) {
                float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                if((pAzimuth <= (azimuth+cylindricalAzimuthSpacing/2.))
                        && (pAzimuth > (azimuth-cylindricalAzimuthSpacing/2.))) {
                    if((k*kGridsp <= ((height/kGridsp)-zmin+cylindricalHeightSpacing/2.))
                            && (k*kGridsp > ((height/kGridsp)-zmin-cylindricalHeightSpacing/2.))) {
                        count++;
                    }
                }
            }
        }
    }
    return count;
}

float* GriddedData::getCylindricalRadiusData(QString& fieldName, float azimuth, 
                                             float height)
{
    int numPoints = getCylindricalRadiusLength(azimuth, height);
    int field = getFieldIndex(fieldName);
    float *values = new float[numPoints];

    int count = 0;
    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j++) {
            for(int k = 0; k < kDim; k++) {
                float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                if((pAzimuth <= (azimuth+cylindricalAzimuthSpacing/2.))
                        && (pAzimuth > (azimuth-cylindricalAzimuthSpacing/2.))) {
                    if((k*kGridsp <= ((height/kGridsp)-zmin+cylindricalHeightSpacing/2.))
                            && (k*kGridsp > ((height/kGridsp)-zmin-cylindricalHeightSpacing/2.))) {
                        values[count] = dataGrid[field][i][j][k];
                        count++;
                    }
                }
            }
        }
    }
    return values;
}

float* GriddedData::getCylindricalRadiusPosition(float azimuth, float height)
{
    int numPoints = getCylindricalRadiusLength(azimuth, height);

    float *positions = new float[numPoints];

    int count = 0;
    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j++) {
            for(int k = 0; k < kDim; k++) {
                float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                if((pAzimuth <= (azimuth+cylindricalAzimuthSpacing/2.))
                        && (pAzimuth > (azimuth-cylindricalAzimuthSpacing/2.))) {
                    if((k*kGridsp <= ((height/kGridsp)-zmin+cylindricalHeightSpacing/2.))
                            && (k*kGridsp > ((height/kGridsp)-zmin-cylindricalHeightSpacing/2.))) {
                        positions[count] = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                        count++;
                    }
                }
            }
        }
    }
    return positions;
}

/*
// original function replaced on 6/14/07 -LM
int GriddedData::getCylindricalAzimuthLength(float radius, float height)
{
  int count = 0;
  float r = 0;
  
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for(int k = 0; k < kDim; k ++) {
 r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
 if((r <= (radius+cylindricalRadiusSpacing/2.))
    && (r > (radius-cylindricalRadiusSpacing/2.))) {
   if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
      && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
     count++;
   }
 }
      }
    }
  }
  return count;
}
*/

int GriddedData::getCylindricalAzimuthLength(float radius, float height)
{
    int count = 0;
    float r = 0;
    // 2 is for a little extra :)
    int iLow = int(refPointI)-int((radius+cylindricalRadiusSpacing)/iGridsp)-2;
    int iHigh = int(refPointI) + int((radius+cylindricalRadiusSpacing)/iGridsp) + 2;
    if(iLow < 0)
        iLow = 0;
    if(iHigh > iDim)
        iHigh = int(iDim);
    int jLow = int(refPointJ)-int((radius+cylindricalRadiusSpacing)/jGridsp)-2;
    int jHigh = int(refPointJ)+int((radius+cylindricalRadiusSpacing)/jGridsp)+2;
    if(jLow < 0)
        jLow = 0;
    if(jHigh > jDim)
        jHigh = int(jDim);
    // Do k too!
    for(int i = iLow; i < iHigh; i ++) {
        for(int j = jLow; j < jHigh; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        count++;
                    }
                }
            }
        }
    }
    return count;

}

int GriddedData::getCylindricalAzimuthLengthTest2(float radius, float height)
{
    int count = 0;
    float r = 0;
    float sqrt2 = sqrt(2);
    // 2 is for a little extra :)
    int iLow = int(refPointI)-int((radius+cylindricalRadiusSpacing)/iGridsp)-2;
    int iHigh = int(refPointI) + int((radius+cylindricalRadiusSpacing)/iGridsp) + 2;
    int iInLow = int(refPointI) - int((radius-cylindricalRadiusSpacing)/(sqrt2*iGridsp))+2;
    int iInHigh = int(refPointI) + int((radius-cylindricalRadiusSpacing)/(sqrt2*iGridsp))-2;
    if(iLow < 0)
        iLow = 0;
    if(iHigh > iDim)
        iHigh = int(iDim);
    if(iInLow < iLow)
        iInLow = iLow;
    //Message::toScreen("ILOWCOMPERROR: Radius: "+QString().setNum(radius)+" height: "+QString().setNum(height)+" iLow: "+QString().setNum(iLow)+" InLow: "+QString().setNum(iInLow));
    if(iHigh < iInHigh)
        iInHigh = iHigh;
    //Message::toScreen("IHIGHCOMPERROR: Radius: "+QString().setNum(radius)+" height: "+QString().setNum(height)+" iHigh: "+QString().setNum(iHigh)+" iInHigh: "+QString().setNum(iInHigh));
    if(iInLow >= iInHigh) {
        iInLow = int(refPointI);
        iInHigh = int(refPointI);
    }
    int jLow = int(refPointJ)-int((radius+cylindricalRadiusSpacing)/jGridsp)-2;
    int jHigh = int(refPointJ)+int((radius+cylindricalRadiusSpacing)/jGridsp)+2;
    int jInLow = int(refPointJ) - int((radius-cylindricalRadiusSpacing)/(sqrt2*jGridsp))+2;
    int jInHigh = int(refPointJ) + int((radius-cylindricalRadiusSpacing)/(sqrt2*jGridsp))-2;
    if(jLow < 0)
        jLow = 0;
    if(jHigh > jDim)
        jHigh = int(jDim);
    if(jInLow < jLow)
        jInLow = jLow;
    //Message::toScreen("JLOWCOMPERROR: Radius: "+QString().setNum(radius)+" height: "+QString().setNum(height)+" jLow: "+QString().setNum(jLow)+" jInLow: "+QString().setNum(jInLow));
    if(jHigh < jInHigh)
        jInHigh = jHigh;
    //Message::toScreen("JHIGHCOMPERROR: Radius: "+QString().setNum(radius)+" height: "+QString().setNum(height)+" jHigh: "+QString().setNum(jHigh)+" jInHigh: "+QString().setNum(jInHigh));
    if(jInLow >= jInHigh) {
        jInLow = int(refPointJ);
        jInHigh = int(refPointJ);
    }

    // Do k too!

    for(int i = iLow; i < iInLow; i ++) {
        for(int j = jLow; j < jHigh; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        count++;
                    }
                }
            }
        }
    }
    for(int i = iInLow; i < iInHigh; i ++) {
        for(int j = jLow; j < jInLow; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        count++;
                    }
                }
            }
        }
        for(int j = jInHigh; j < jHigh; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        count++;
                    }
                }
            }
        }
    }
    for(int i = iInHigh; i < iHigh; i ++) {
        for(int j = jLow; j < jHigh; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        count++;
                    }
                }
            }
        }
    }
    return count;

}

/*
// original function replaced on 6/14/07 -LM
void GriddedData::getCylindricalAzimuthData(QString& fieldName, int numPoints,
         float radius, float height, float* values)
{
  //  int numPoints = getCylindricalAzimuthLength(radius, height);
  int field = getFieldIndex(fieldName);

  //  float *values = new float[numPoints];

  int count = 0;
  float r = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for(int k = 0; k < kDim; k ++) {
 r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
 if((r <= (radius+cylindricalRadiusSpacing/2.))
    && (r > (radius-cylindricalRadiusSpacing/2.))) {
   if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
      && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
     values[count] = dataGrid[field][i][j][k];
     count++;
     if(count > numPoints) {
       // Memory overflow ... bail out
       Message::toScreen("GriddedData: getCylindricalAzimuthData: HUGE Problems!");
     }
   }
 }
      }
    }
  }
  //return values;
}
*/

void GriddedData::getCylindricalAzimuthData(QString& fieldName, 
                                            int numPoints,float radius,
                                            float height, float* values)
{
    //  int numPoints = getCylindricalAzimuthLength(radius, height);
    int field = getFieldIndex(fieldName);

    //  float *values = new float[numPoints];

    int count = 0;
    float r = 0;

    // 2 is for a little extra :)
    int iLow = int(refPointI)-int((radius+cylindricalRadiusSpacing)/iGridsp)-2;
    int iHigh = int(refPointI) + int((radius+cylindricalRadiusSpacing)/iGridsp) + 2;
    if(iLow < 0)
        iLow = 0;
    if(iHigh > iDim)
        iHigh = int(iDim);
    int jLow = int(refPointJ)-int((radius+cylindricalRadiusSpacing)/jGridsp)-2;
    int jHigh = int(refPointJ)+int((radius+cylindricalRadiusSpacing)/jGridsp)+2;
    if(jLow < 0)
        jLow = 0;
    if(jHigh > jDim)
        jHigh = int(jDim);
    // Do k too!
    for(int i = iLow; i < iHigh; i ++) {
        for(int j = jLow; j < jHigh; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        values[count] = dataGrid[field][i][j][k];
			// TODO debug
			// std::cout << "val[" << count << "] = " << values[count] << std::endl;
                        count++;
                        if(count > numPoints) {
                            // Memory overflow ... bail out
                            Message::toScreen("GriddedData: getCylindricalAzimuthData: HUGE Problems!");
                        }
                    }
                }
            }
        }
    }
    //return values;
}

void GriddedData::getCylindricalAzimuthDataTest2(QString& fieldName, 
                                                 int numPoints,float radius,
                                                 float height, float* values)
{
    //  int numPoints = getCylindricalAzimuthLength(radius, height);
    int field = getFieldIndex(fieldName);

    //  float *values = new float[numPoints];

    int count = 0;
    float r = 0;

    float sqrt2 = sqrt(2);
    // 2 is for a little extra :)
    int iLow = int(refPointI)-int((radius+cylindricalRadiusSpacing)/iGridsp)-2;
    int iHigh = int(refPointI) + int((radius+cylindricalRadiusSpacing)/iGridsp) + 2;
    int iInLow = int(refPointI) - int((radius-cylindricalRadiusSpacing)/(sqrt2*iGridsp))+2;
    int iInHigh = int(refPointI) + int((radius-cylindricalRadiusSpacing)/(sqrt2*iGridsp))-2;
    if(iLow < 0)
        iLow = 0;
    if(iHigh > iDim)
        iHigh = int(iDim);
    if(iInLow < iLow)
        iInLow = iLow;
    //Message::toScreen("ILOWCOMPERROR: Radius: "+QString().setNum(radius)+" height: "+QString().setNum(height)+" iLow: "+QString().setNum(iLow)+" InLow: "+QString().setNum(iInLow));
    if(iHigh < iInHigh)
        iInHigh = iHigh;
    //Message::toScreen("IHIGHCOMPERROR: Radius: "+QString().setNum(radius)+" height: "+QString().setNum(height)+" iHigh: "+QString().setNum(iHigh)+" iInHigh: "+QString().setNum(iInHigh));
    if(iInLow >= iInHigh) {
        iInLow = int(refPointI);
        iInHigh = int(refPointI);
    }
    int jLow = int(refPointJ)-int((radius+cylindricalRadiusSpacing)/jGridsp)-2;
    int jHigh = int(refPointJ)+int((radius+cylindricalRadiusSpacing)/jGridsp)+2;
    int jInLow = int(refPointJ) - int((radius-cylindricalRadiusSpacing)/(sqrt2*jGridsp))+2;
    int jInHigh = int(refPointJ) + int((radius-cylindricalRadiusSpacing)/(sqrt2*jGridsp))-2;
    if(jLow < 0)
        jLow = 0;
    if(jHigh > jDim)
        jHigh = int(jDim);
    if(jInLow < jLow)
        jInLow = jLow;
    //Message::toScreen("JLOWCOMPERROR: Radius: "+QString().setNum(radius)+" height: "+QString().setNum(height)+" jLow: "+QString().setNum(jLow)+" jInLow: "+QString().setNum(jInLow));
    if(jHigh < jInHigh)
        jInHigh = jHigh;
    //Message::toScreen("JHIGHCOMPERROR: Radius: "+QString().setNum(radius)+" height: "+QString().setNum(height)+" jHigh: "+QString().setNum(jHigh)+" jInHigh: "+QString().setNum(jInHigh));
    if(jInLow >= jInHigh) {
        jInLow = int(refPointJ);
        jInHigh = int(refPointJ);
    }

    // Do k too!

    for(int i = iLow; i < iInLow; i ++) {
        for(int j = jLow; j < jHigh; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        values[count] = dataGrid[field][i][j][k];
                        count++;
                        if(count > numPoints) {
                            // Memory overflow ... bail out
                            Message::toScreen("GriddedData: getCylindricalAzimuthData: HUGE Problems!");
                        }
                    }
                }
            }
        }
    }
    for(int i = iInLow; i < iInHigh; i ++) {
        for(int j = jLow; j < jInLow; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        values[count] = dataGrid[field][i][j][k];
                        count++;
                        if(count > numPoints) {
                            // Memory overflow ... bail out
                            Message::toScreen("GriddedData: getCylindricalAzimuthData: HUGE Problems!");
                        }
                    }
                }
            }
        }
        for(int j = jInHigh; j < jHigh; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        values[count] = dataGrid[field][i][j][k];
                        count++;
                        if(count > numPoints) {
                            // Memory overflow ... bail out
                            Message::toScreen("GriddedData: getCylindricalAzimuthData: HUGE Problems!");
                        }
                    }
                }
            }
        }
    }
    for(int i = iInHigh; i < iHigh; i ++) {
        for(int j = jLow; j < jHigh; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        values[count] = dataGrid[field][i][j][k];
                        count++;
                        if(count > numPoints) {
                            // Memory overflow ... bail out
                            Message::toScreen("GriddedData: getCylindricalAzimuthData: HUGE Problems!");
                        }
                    }
                }
            }
        }
    }
    //return values;
}

/*
// original function replaced on 6/14/07 -LM
void GriddedData::getCylindricalAzimuthPosition(int numPoints, float radius, float height, float* positions) 
{
//  int numPoints = getCylindricalAzimuthLength(radius, height);

//  float *positions = new float[numPoints];

  int count = 0;
  float r = 0;
  for(int i = 0; i < iDim; i ++) {
    for(int j = 0; j < jDim; j ++) {
      for(int k = 0; k < kDim; k ++) {
    r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)
       + jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
    if((r <= (radius+cylindricalRadiusSpacing/2.))
    && (r > (radius-cylindricalRadiusSpacing/2.))) {
     if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
     && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
      float azimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
      if (count > numPoints) {
       // Memory overflow, bail out
       return;
      } else {
       positions[count] = azimuth;
       count++;
      }
     }
    }
   }
 }
  }
}
*/

void GriddedData::getCylindricalAzimuthPosition(int numPoints, float radius, float height, float* positions) 
{
    //  int numPoints = getCylindricalAzimuthLength(radius, height);

    //  float *positions = new float[numPoints];

    int count = 0;
    float r = 0;


    // 2 is for a little extra :)
    int iLow = int(refPointI)-int((radius+cylindricalRadiusSpacing)/iGridsp)-2;
    int iHigh = int(refPointI) + int((radius+cylindricalRadiusSpacing)/iGridsp) + 2;
    if(iLow < 0)
        iLow = 0;
    if(iHigh > iDim)
        iHigh = int(iDim);
    int jLow = int(refPointJ)-int((radius+cylindricalRadiusSpacing)/jGridsp)-2;
    int jHigh = int(refPointJ)+int((radius+cylindricalRadiusSpacing)/jGridsp)+2;
    if(jLow < 0)
        jLow = 0;
    if(jHigh > jDim)
        jHigh = int(jDim);
    // Do k too!
    for(int i = iLow; i < iHigh; i ++) {
        for(int j = jLow; j < jHigh; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)
                         + jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        float azimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                        if (count > numPoints) {
                            // Memory overflow, bail out
                            return;
                        } else {
                            positions[count] = azimuth;
                            count++;
                        }
                    }
                }
            }
        }
    }
}

void GriddedData::getCylindricalAzimuthPositionTest2(int numPoints, float radius, float height, float* positions) 
{
    //  int numPoints = getCylindricalAzimuthLength(radius, height);

    //  float *positions = new float[numPoints];

    int count = 0;
    float r = 0;

    float sqrt2 = sqrt(2);
    // 2 is for a little extra :)
    int iLow = int(refPointI)-int((radius+cylindricalRadiusSpacing)/iGridsp)-2;
    int iHigh = int(refPointI) + int((radius+cylindricalRadiusSpacing)/iGridsp) + 2;
    int iInLow = int(refPointI) - int((radius-cylindricalRadiusSpacing)/(sqrt2*iGridsp))+2;
    int iInHigh = int(refPointI) + int((radius-cylindricalRadiusSpacing)/(sqrt2*iGridsp))-2;
    if(iLow < 0)
        iLow = 0;
    if(iHigh > iDim)
        iHigh = int(iDim);
    if(iInLow < iLow)
        iInLow = iLow;
    //Message::toScreen("ILOWCOMPERROR: Radius: "+QString().setNum(radius)+" height: "+QString().setNum(height)+" iLow: "+QString().setNum(iLow)+" InLow: "+QString().setNum(iInLow));
    if(iHigh < iInHigh)
        iInHigh = iHigh;
    //Message::toScreen("IHIGHCOMPERROR: Radius: "+QString().setNum(radius)+" height: "+QString().setNum(height)+" iHigh: "+QString().setNum(iHigh)+" iInHigh: "+QString().setNum(iInHigh));
    if(iInLow >= iInHigh) {
        iInLow = int(refPointI);
        iInHigh = int(refPointI);
    }
    int jLow = int(refPointJ)-int((radius+cylindricalRadiusSpacing)/jGridsp)-2;
    int jHigh = int(refPointJ)+int((radius+cylindricalRadiusSpacing)/jGridsp)+2;
    int jInLow = int(refPointJ) - int((radius-cylindricalRadiusSpacing)/(sqrt2*jGridsp))+2;
    int jInHigh = int(refPointJ) + int((radius-cylindricalRadiusSpacing)/(sqrt2*jGridsp))-2;
    if(jLow < 0)
        jLow = 0;
    if(jHigh > jDim)
        jHigh = int(jDim);
    if(jInLow < jLow)
        jInLow = jLow;
    //Message::toScreen("JLOWCOMPERROR: Radius: "+QString().setNum(radius)+" height: "+QString().setNum(height)+" jLow: "+QString().setNum(jLow)+" jInLow: "+QString().setNum(jInLow));
    if(jHigh < jInHigh)
        jInHigh = jHigh;
    //Message::toScreen("JHIGHCOMPERROR: Radius: "+QString().setNum(radius)+" height: "+QString().setNum(height)+" jHigh: "+QString().setNum(jHigh)+" jInHigh: "+QString().setNum(jInHigh));
    if(jInLow >= jInHigh) {
        jInLow = int(refPointJ);
        jInHigh = int(refPointJ);
    }

    // Do k too!

    for(int i = iLow; i < iInLow; i ++) {
        for(int j = jLow; j < jHigh; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)
                         + jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        float azimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                        if (count > numPoints) {
                            // Memory overflow, bail out
                            return;
                        } else {
                            positions[count] = azimuth;
                            count++;
                        }
                    }
                }
            }
        }
    }
    for(int i = iInLow; i < iInHigh; i ++) {
        for(int j = jLow; j < jInLow; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)
                         + jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        float azimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                        if (count > numPoints) {
                            // Memory overflow, bail out
                            return;
                        } else {
                            positions[count] = azimuth;
                            count++;
                        }
                    }
                }
            }
        }
        for(int j = jInHigh; j < jHigh; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)
                         + jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        float azimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                        if (count > numPoints) {
                            // Memory overflow, bail out
                            return;
                        } else {
                            positions[count] = azimuth;
                            count++;
                        }
                    }
                }
            }
        }
    }
    for(int i = iInHigh; i < iHigh; i ++) {
        for(int j = jLow; j < jHigh; j ++) {
            for(int k = 0; k < kDim; k ++) {
                r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)
                         + jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                if((r <= (radius+cylindricalRadiusSpacing/2.))
                        && (r > (radius-cylindricalRadiusSpacing/2.))) {
                    if((k <= (((height-zmin)/kGridsp)+cylindricalHeightSpacing/2))
                            && (k > (((height-zmin)/kGridsp)-cylindricalHeightSpacing/2))) {
                        float azimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                        if (count > numPoints) {
                            // Memory overflow, bail out
                            return;
                        } else {
                            positions[count] = azimuth;
                            count++;
                        }
                    }
                }
            }
        }
    }
}

int GriddedData::getCylindricalHeightLength(float radius, float azimuth)
{
    int count = 0;
    for(int i = 0; i < iDim; i++){
        for(int j = 0; j < jDim; j++){
            float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
            float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
            if((r <= (radius+cylindricalRadiusSpacing/2.))
                    && (r > (radius-cylindricalRadiusSpacing/2.))) {
                if((pAzimuth <= azimuth+cylindricalAzimuthSpacing/2.)
                        && (pAzimuth > azimuth-cylindricalAzimuthSpacing/2.)) {
                    for(int k = 0; k < kDim; k++){
                        count++;
                    }
                }
            }
        }
    }
    return count;
}

float* GriddedData::getCylindricalHeightData(QString& fieldName, float radius, 
                                             float azimuth)
{
    int numPoints = getCylindricalHeightLength(radius, azimuth);

    int field = getFieldIndex(fieldName);

    float *data = new float[numPoints];

    int count = 0;
    for(int i = 0; i < iDim; i++){
        for(int j = 0; j < jDim; j++){
            float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
            float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
            if((r <= (radius+cylindricalRadiusSpacing/2.))
                    && (r > (radius-cylindricalRadiusSpacing/2.))) {
                if((pAzimuth <= azimuth+cylindricalAzimuthSpacing/2.)
                        && (pAzimuth > azimuth-cylindricalAzimuthSpacing/2.)) {
                    for(int k = 0; k < kDim; k++){
                        data[count] = dataGrid[field][i][j][k];
                        count++;
                    }
                }
            }
        }
    }
    return data;
}

float* GriddedData::getCylindricalHeightPosition(float radius, float azimuth)
{
    int numPoints = getCylindricalHeightLength(radius, azimuth);
    float *positions = new float[numPoints];

    int count = 0;
    for(int i = 0; i < iDim; i++){
        for(int j = 0; j < jDim; j++){
            float r = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
            float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
            if((r <= (radius+cylindricalRadiusSpacing/2.))
                    && (r > (radius-cylindricalRadiusSpacing/2.))) {
                if((pAzimuth <= azimuth+cylindricalAzimuthSpacing/2.)
                        && (pAzimuth > azimuth-cylindricalAzimuthSpacing/2.)) {
                    for(int k = 0; k < kDim; k++){
                        positions[count] = kGridsp*k;
                        count++;
                    }
                }
            }
        }
    }
    return positions;
}

bool GriddedData::test()
{
    iDim = 10;
    jDim = 10;
    kDim = 10;
    ymin = -4;
    ymax = 5;
    xmin = -4;
    xmax = 5;
    zmin = 1;
    zmax = 10;
    iGridsp = 2;
    jGridsp = 2;
    kGridsp = 1;
    for(int i = 0; i < iDim; i++) {
        for(int j = 0; j < jDim; j++) {
            for(int k = 0; k < kDim; k++) {
                for(int dataField = 0; dataField < 3; dataField++) {
                    dataGrid[dataField][i][j][k] = dataField*j;
                }
            }
        }
    }
    for(int j = 0; j < jDim; j++) {
        for(int k = 0; k < kDim; k++) {
            QString fieldName("dz");
            Message::toScreen("field Name "+fieldName+" "+QString().setNum(getFieldIndex(fieldName)));
            float *xValues = new float[int(floor(iDim))];
            xValues = getCartesianXslice(fieldName,(j+ymin)*jGridsp+.5,(k+zmin)*kGridsp);
            for(int i = 0; i < iDim; i++) {

                if(getCartesianValue(fieldName,(i+xmin)*iGridsp,(j+ymin)*jGridsp, (k+zmin)*kGridsp)!=0) {
                    QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(xValues[i])+" from getCartesianValue");
                    Message::toScreen(message);
                }
                if(xValues[i]!=(dataGrid[0][i][j][k]+dataGrid[0][i][j+1][k])) {
                    QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(xValues[i])+" actual: "+QString().setNum(dataGrid[0][i][j][k]));
                    Message::toScreen(message);
                }
            }
            fieldName = QString("ve");
            Message::toScreen("field Name "+fieldName+" "+QString().setNum(getFieldIndex(fieldName)));
            xValues = getCartesianXslice(fieldName,(j+ymin)*jGridsp,
                                         (k+zmin)*kGridsp);
            for(int i = 0; i < iDim; i++) {
                if(xValues[i]!=dataGrid[1][i][j][k]) {
                    QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(j)+" value:"+QString().setNum(xValues[i])+" actual: "+QString().setNum(dataGrid[1][i][j][k]));
                    Message::toScreen(message);
                }
            }
            fieldName = QString("sw");
            Message::toScreen("field Name "+fieldName+" "+QString().setNum(getFieldIndex(fieldName)));
            xValues = getCartesianXslice(fieldName,(j+ymin)*jGridsp,
                                         (k+zmin)*kGridsp);
            for(int i = 0; i < iDim; i++) {
                if(xValues[i]!=dataGrid[2][i][j][k]) {
                    QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(xValues[i])+" actual: "+QString().setNum(dataGrid[2][i][j][k]));
                    Message::toScreen(message);
                }
            }
            Message::toScreen("------------------------------------------------");
        }
    }

    for(int i = 0; i < iDim; i++) {
        for(int k = 0; k < kDim; k++) {
            QString fieldName("dz");
            float *yValues = new float[int(floor(jDim))];
            yValues = getCartesianYslice(fieldName,(i+xmin)*iGridsp,
                                         (k+zmin)*kGridsp);
            for(int j = 0; j < jDim; j++) {
                if(yValues[j]!=dataGrid[0][i][j][k]) {
                    QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(yValues[j])+" actual: "+QString().setNum(dataGrid[0][i][j][k]));
                    Message::toScreen(message);
                }
            }
            fieldName = QString("ve");
            yValues = getCartesianYslice(fieldName,(i+xmin)*iGridsp,
                                         (k+zmin)*kGridsp);
            for(int j = 0; j < jDim; j++) {
                if(yValues[j]!=dataGrid[1][i][j][k]) {
                    QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(yValues[j])+" actual "+QString().setNum(dataGrid[1][i][j][k]));
                    Message::toScreen(message);
                }
            }
            fieldName = QString("sw");
            yValues = getCartesianYslice(fieldName,(i+xmin)*iGridsp,
                                         (k+zmin)*kGridsp);
            for(int j = 0; j < jDim; j++) {
                if(yValues[j]!=dataGrid[2][i][j][k]) {
                    QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(yValues[j])+" actual "+QString().setNum(dataGrid[2][i][j][k]));
                    Message::toScreen(message);
                }
            }
            Message::toScreen("********************************************");
        }
    }

    for(int i = 0; i < iDim; i++) {
        for(int j = 0; j < jDim; j++) {
            QString fieldName("dz");
            float *zValues = new float[int(floor(kDim))];
            zValues= getCartesianZslice(fieldName,(i+xmin)*iGridsp,(j+ymin)*jGridsp);
            for(int k = 0; k < kDim; k++) {
                if(zValues[k]!=dataGrid[0][i][j][k]) {
                    QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(zValues[k]));
                    Message::toScreen(message);
                }
            }
            fieldName = QString("ve");
            zValues = getCartesianZslice(fieldName,(i+xmin)*iGridsp,
                                         (j+ymin)*jGridsp);
            for(int k = 0; k < kDim; k++) {
                if(zValues[k]!=dataGrid[1][i][j][k]) {
                    QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(zValues[k]));
                    Message::toScreen(message);
                }
            }
            fieldName = QString("sw");
            zValues = getCartesianZslice(fieldName,(i+xmin)*iGridsp,
                                         (j+ymin)*jGridsp);
            for(int k = 0; k < kDim; k++) {
                if(zValues[k]!=dataGrid[2][i][j][k]) {
                    QString message("TEST: Value not what is expected "+fieldName+" x:"+QString().setNum(i)+" y:"+QString().setNum(j)+" z:"+QString().setNum(k)+" value:"+QString().setNum(zValues[k]));
                    Message::toScreen(message);
                }
            }
        }
    }

    Message::toScreen("Finished Checking");
    return true;
}



int GriddedData::getSphericalRangeLengthTest(float azimuth, float elevation)
{
    float te1;
    float ta1;
    float ta2;
    if((elevation-sphericalElevationSpacing/2)<=0)
        te1 = 0;
    else
        te1 = tan((elevation-sphericalElevationSpacing/2)*deg2rad);
    if((azimuth - sphericalAzimuthSpacing/2)<0)
        ta1 = tan((azimuth-sphericalAzimuthSpacing/2+360)*deg2rad);
    else
        ta1 = tan((azimuth-sphericalAzimuthSpacing/2)*deg2rad);
    if((azimuth + sphericalAzimuthSpacing/2)>=360)
        ta2 = tan((azimuth+sphericalAzimuthSpacing/2-360)*deg2rad);
    else
        ta2 = tan((azimuth+sphericalAzimuthSpacing/2)*deg2rad);

    int count = 0;

    for (int k = 0; k < kDim; k++) {
        float jDistMax = fabs(k/te1);
        float jMin = refPointJ-jDistMax;
        float jMax = jDistMax+refPointJ;
        if((jMin < 0)||(std::isnan(jMin)))
            jMin = 0;
        if((std::isnan(jMax))||(jMax > jDim))
            jMax = jDim;
        // if(jMin >= jMax)
        Message::toScreen("Terrible JLimits for k = "+QString().setNum(k)+" jMin = "+QString().setNum(jMin)+" jMax = "+QString().setNum(jMax));
        for(int j = int(jMin); j < jMax; j ++) {
            if(j == 0)
                Message::toScreen("In j loop jMin = "+QString().setNum(jMin));
            //if(iNegMin <= iNegMax)
            // Message::toScreen("Terrible Errors in INeg");
            //if(iPosMin >= iPosMax)
            // Message::toScreen("Terrible Errors in IPos");
            float iMin = (j-refPointJ)*ta1+refPointI;
            float iMax = (j-refPointJ)*ta2+refPointI;
            Message::toScreen("iMin = "+QString().setNum(iMin));
            Message::toScreen("iMax = "+QString().setNum(iMax));
            for(int i = int(iMin); (i < iDim)&&(i < iMax); i ++) {

                float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
                if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.))
                        && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {

                    //QString test("pElevation " +QString().setNum( pElevation )+ " elevation " +QString().setNum( elevation )+ "sphericalElevationSpacing " +QString().setNum( sphericalElevationSpacing )+ "\n");
                    //Message::toScreen(test);
                    if((pElevation <=(elevation+sphericalElevationSpacing/2.))
                            && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
                        count++;
                    }
                }
            }
        }
    }
    return count;
}

float* GriddedData::getSphericalRangeDataTest(QString& fieldName, 
                                              float azimuth,
                                              float elevation)
{    
    int numPoints = getSphericalRangeLengthTest(azimuth, elevation);
    int field = getFieldIndex(fieldName);
    float *values = new float[numPoints];

    int count = 0;
    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j++) {
            for (int k = 0; k < kDim; k++) {
                float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
                if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.))
                        && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {
                    if((pElevation <=(elevation+sphericalElevationSpacing/2.))
                            && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
                        values[count] = dataGrid[field][i][j][k];
                        count++;
                    }
                }
            }
        }
    }
    return values;
}

float* GriddedData::getSphericalRangePositionTest(float azimuth, 
                                                  float elevation)
{
    int numPoints = getSphericalRangeLengthTest(azimuth, elevation);

    float *positions = new float[numPoints];

    int count = 0;
    for(int i = 0; i < iDim; i ++) {
        for(int j = 0; j < jDim; j++) {
            for (int k = 0; k < kDim; k++) {
                float rp = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ));
                float pAzimuth = fixAngle(atan2((j-refPointJ),(i-refPointI)))*rad2deg;
                float pElevation = fixAngle(atan2((k-refPointK),rp))*rad2deg;
                if((pAzimuth <= (azimuth+sphericalAzimuthSpacing/2.))
                        && (pAzimuth > (azimuth-sphericalAzimuthSpacing/2.))) {
                    if((pElevation <=(elevation+sphericalElevationSpacing/2.))
                            && (pElevation > (elevation-sphericalElevationSpacing/2.))) {
                        positions[count] = sqrt(iGridsp*iGridsp*(i-refPointI)*(i-refPointI)+jGridsp*jGridsp*(j-refPointJ)*(j-refPointJ)+kGridsp*kGridsp*(k-refPointK)*(k-refPointK));
                        count++;
                    }
                }
            }
        }
    }
    return positions;
}

void GriddedData::setCylindricalAzimuthSpacing(const float& newSpacing)
{
    cylindricalAzimuthSpacing = newSpacing;
}
