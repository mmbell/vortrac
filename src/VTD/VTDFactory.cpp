#include <iostream>

#include "VTDFactory.h"

// Object Factory

VTD *VTDFactory::createVTD(QString& initGeometry, QString& initClosure,
			   int& waveNumbers, float*& gaps, float hvvpWind)
{
  if (initGeometry == "GBVTD")
    return new GBVTD(initClosure, waveNumbers, gaps, hvvpWind);
  else if (initGeometry == "GVTD")
    return new GVTD(initClosure, waveNumbers, gaps, hvvpWind);
  else
    std::cerr << "Unsupported geometry " << initGeometry.toLatin1().data() << std::endl;
  
  return NULL;
}

