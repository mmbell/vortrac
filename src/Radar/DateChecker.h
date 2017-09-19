#ifndef DATE_CHECKER_H
#define DATE_CHECKER_H

#include <QDateTime>

#include "RadarFactory.h"
class DateChecker {

 public:

  virtual bool fileInRange(QString filePath, QString radarName,
			   QDateTime startDateTime, QDateTime endDateTime) = 0;

  QDateTime getTime() { return fileDateTime; }
  
  QString baseName(QString path);
  virtual ~DateChecker() {}

 protected:

  QDateTime fileDateTime;
  
};

class NcdcLevelIIChecker : public DateChecker {

 public:

  bool fileInRange(QString filePath, QString radarName,
		   QDateTime startDateTime, QDateTime endDateTime);
};

class LdmLevelIIChecker : public DateChecker {

 public:

  bool fileInRange(QString filePath, QString radarName,
		   QDateTime startDateTime, QDateTime endDateTime);
};

class ModelChecker : public DateChecker {

 public:

  bool fileInRange(QString filePath, QString radarName,
		   QDateTime startDateTime, QDateTime endDateTime);
};

class DoradeChecker : public DateChecker {

 public:

  bool fileInRange(QString filePath, QString radarName,
		   QDateTime startDateTime, QDateTime endDateTime);
};

class NetcdfChecker : public DateChecker {

 public:

  bool fileInRange(QString filePath, QString radarName,
			QDateTime startDateTime, QDateTime endDateTime);
};
  
class DateCheckerFactory {

 public:  

  static DateChecker *newChecker(RadarFactory::dataFormat);

 private:

  // Static class. Don't let anybody create instances
  
  DateCheckerFactory();
};

#endif
