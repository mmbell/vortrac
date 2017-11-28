#ifndef VTDFACTORY_H
#define VTDFACTORY_H

#include "VTD.h"
#include "GVTD.h"
#include "GBVTD.h"

class VTDFactory {
  
 public:

  // Factory method to create the appropriate VTD object
  static VTD *createVTD(QString& initGeometry, QString& initClosure,
			int& wavenumbers, float*& gaps,
			float hvvpwind = 0.0);
};

#endif
