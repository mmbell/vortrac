/*
 * DriverBatch.h
 * VORTRAC
 *
 * Created by Annette Foerster in 2012
 * Copyright 2005 University Corporation for Atmospheric Research.
 * All rights reserved.
 *
 *
 */


#ifndef DRIVERBATCH_H
#define DRIVERBATCH_H

#include "Driver.h"

class DriverBatch  : public Driver
{

public:
    DriverBatch();
    ~DriverBatch();
    bool initialize(const QDomElement& configuration) = 0;
    bool run() = 0;
    bool finalize() = 0;
};

#endif // DRIVERBATCH_H
