#include "DateChecker.h"

QString DateChecker::baseName(QString path)
{
  QFileInfo fi(path);
  return fi.baseName();
}

bool NcdcLevelIIChecker::fileInRange(QString filePath, QString radarName,
				    QDateTime startDateTime, QDateTime endDateTime)
{
  QString timepart = baseName(filePath);
  // Replace the radarname so we just have timestamps
  timepart.replace(radarName, "");
  QStringList timestamp = timepart.split("_");
  QDate fileDate = QDate::fromString(timestamp.at(0), "yyyyMMdd");
  QTime fileTime = QTime::fromString(timestamp.at(1), "hhmmss");
  fileDateTime = QDateTime(fileDate, fileTime, Qt::UTC);
  return (fileDateTime >= startDateTime) && (fileDateTime <= endDateTime);
}

bool LdmLevelIIChecker::fileInRange(QString filePath, QString radarName,
				    QDateTime startDateTime, QDateTime endDateTime)
{
  QString timepart = baseName(filePath);

  QDate fileDate;
  QTime fileTime;

  if (timepart.contains("Level2")) {
    // UCAR Format
    timepart.replace(".ar2v", "");
    QStringList timestamp = timepart.split("_");
    fileDate = QDate::fromString(timestamp.at(2), "yyyyMMdd");
    fileTime = QTime::fromString(timestamp.at(3), "hhmm");
  } else if (timepart.contains('.')) {
    // NRL Format
    // Replace the radarname so we just have timestamps
    timepart.replace(radarName, "");
    QStringList timestamp = timepart.split(".");
    fileDate = QDate::fromString(timestamp.at(1).left(8), "yyyyMMdd");
    fileTime = QTime::fromString(timestamp.at(1).right(6), "hhmmss");
  }  else if (timepart.contains('_')) {
    // Purdue Format
    // Replace the radarname so we just have timestamps
    timepart.replace(radarName, "");
    QStringList timestamp = timepart.split("_");
    fileDate = QDate::fromString(timestamp.at(1), "yyyyMMdd");
    if (timestamp.size() > 2) {
      fileTime = QTime::fromString(timestamp.at(2), "hhmm");
    } else {
      std::cerr << "Problem with time in level_II filename '" << filePath.toLatin1().data()
		<< "'. This may be a NCDC file" << std::endl;
      return false;
    }
  } else {
      std::cerr << "Problem with time in level_II filename '" << filePath.toLatin1().data()
		<< "'. This may be a non standard file" << std::endl;
      return false;
  }

  fileDateTime = QDateTime(fileDate, fileTime, Qt::UTC);
  return  (fileDateTime >= startDateTime) && (fileDateTime <= endDateTime);
}

bool ModelChecker::fileInRange(QString, QString, QDateTime , QDateTime )
{
  // This one is for testing purpose. So don't worry about date tange.
  // TODO what do we set the date to?

  return true;
}

bool DoradeChecker::fileInRange(QString filePath, QString,
				QDateTime startDateTime, QDateTime endDateTime)
{
  // swp.yyymmddhhmmss.radarname.millisecs.fixedAngle_scantype_vnum
  QString file = baseName(filePath);
  QStringList parts = file.split(".");
  QString timepart = parts.at(1);

  if (timepart.isEmpty())
    return false;

  if (timepart.size() < 11)
    return false;

  int year = timepart.left(3).toInt() + 1900;
  int month = timepart.midRef(3, 2).toInt();
  int day = timepart.midRef(5, 2).toInt();
  QDate fileDate(year, month, day);
  QTime fileTime = QTime::fromString(timepart.midRef(7, 6).toString(), "hhmmss");
  fileDateTime = QDateTime(fileDate, fileTime, Qt::UTC);
  return  (fileDateTime >= startDateTime) && (fileDateTime <= endDateTime);
}

bool CfRadialChecker::fileInRange(QString filePath, QString,
				QDateTime startDateTime, QDateTime endDateTime)
{
  //    For example:
  //         KAMX_20161007_044754.nc
  //         <radar>_<date>_<time>.nc
  // File name should start with the radar ID. Any restriction on the suffix?

  QString base = baseName(filePath);
  QStringList parts = base.split("_");

  if(parts.size() >= 3) {
    // TODO: what does QDate::fromString do in case of invalid file name?
    QDate fileDate = QDate::fromString(parts.at(1), "yyyyMMdd");
    QTime fileTime = QTime::fromString(parts.at(2), "hhmmss");
    fileDateTime = QDateTime(fileDate, fileTime, Qt::UTC);
  }
  return  (fileDateTime >= startDateTime) && (fileDateTime <= endDateTime);
}

DateChecker *DateCheckerFactory::newChecker(RadarFactory::dataFormat fileFormat)
{
  switch (fileFormat) {

  case RadarFactory::ncdclevelII:
    return new NcdcLevelIIChecker();
  case RadarFactory::ldmlevelII:
    return new LdmLevelIIChecker();
  case RadarFactory::model:
    return new ModelChecker();
  case RadarFactory::dorade:
    return new DoradeChecker();
  case RadarFactory::cfradial:
    return new CfRadialChecker();
  default:
    return NULL;
  }
}
