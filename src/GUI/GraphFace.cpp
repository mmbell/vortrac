/*
 * GraphFace.cpp
 * VORTRAC
 *
 * Created by Lisa Mauger on 8/9/05
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#include<QtGui>

#include<QLabel>
#include<QGridLayout>
#include<QHelpEvent>
#include<QToolTip>
#include<QImage>
#include<math.h>

#include "GraphFace.h"

GraphFace::GraphFace(QWidget *parent, const QString& title)
  : QWidget(parent)
  //constructor to create the GraphFace object
{ 
  setMouseTracking(true);

  graphTitle = QString(title);
  //initialize some minimums and maximums so they compare properly later
  rmwMax = 0; autoRmwMax = 0;
  rGMax = 0; autoRGMax = 0;
  pressureMax = 0; autoPressureMax = 0;
  pGMax = 0; autoPGMax = 0;
  deficitMax = -1000; autoDeficitMax = -1000;
  dGMax = -1000; autoDGMax = -1000;
  rmwMin = 1000000; autoRmwMin = 100000;
  rGMin = 1000000; autoRGMin = 1000000;
  pressureMin = 1000000; autoPressureMin = 1000000;
  pGMin = 1000000; autoPGMin = 1000000;
  deficitMin = 100; autoDeficitMin = 100;
  dGMin = 100; autoDGMin = 100;
  rmwRange = 0;
  pressureRange = 0;
  deficitRange = 0;
  timeRange = -1;

  //sets the widgets's size to be the combined size of all the
  //component peices 
  //top, bottom, left, and right margins, 
  //and the graphable section of the widget

  setMinimumSize(QSize(LEFT_MARGIN_WIDTH+RIGHT_MARGIN_WIDTH+300, BOTTOM_MARGIN_HEIGHT+TOP_MARGIN_HEIGHT+250));
  
  graph_height = GRAPH_HEIGHT;
  graph_width = GRAPH_WIDTH;
  z1 = 0.67;
  z2 = 0.95;
  
  // Initialize empty lists since PollThread has not been created yet
  VortexDataList = NULL;
  dropList = NULL;
  setColors();
  first = QDateTime();
  last = QDateTime();
  
  imageAltered = true;
  showPressure = true;

  image = new QImage(graph_width+LEFT_MARGIN_WIDTH+RIGHT_MARGIN_WIDTH,graph_height+TOP_MARGIN_HEIGHT+BOTTOM_MARGIN_HEIGHT,QImage::Format_ARGB32_Premultiplied);

  workingDirectory = QDir(QDir::currentPath());

  autoImageName = QString("autoImage");

  imageFile = new QFile(workingDirectory.filePath(autoImageName+".png"));
  int i = 1;
  QString newName = autoImageName;
  while(imageFile->exists())
    {
      i++;
      newName = autoImageName+QString().setNum(i);
      imageFile = NULL;
      imageFile = new QFile(workingDirectory.filePath(newName+".png"));
    }

  autoImageName = newName + ".png";

  autoAxes = true; 

}

GraphFace::~GraphFace()
{
  imageFile->remove();

  delete image;
  delete imageFile;
  VortexDataList = NULL;
  delete VortexDataList;
  dropList = NULL;
  delete dropList;
  // delete key;
  
}

//************************************--Paint Event--**************************

void GraphFace::paintEvent(QPaintEvent *event)
  
  // This function does all the painting on the widget
  // paintEvent is an overwritten function inherited from QWidget
  // paintEvent is called when the widget is first created, 
  // and any time update is called

{
  
  QPainter *painter = new QPainter(this);

  if (imageAltered) {
    imageAltered = false;
    updateImage(painter);
    QBrush myBackground = painter->background();
    if (painter->isActive())
      painter->end();
    
    delete painter;
    
    // Now update the image too...  
    
    QPainter* imagePainter = new QPainter(image);
    imagePainter->setBackground(myBackground);
    imagePainter->fillRect(QRectF(QPointF(0,0),image->size()),
			 myBackground);
    updateImage(imagePainter);
    
    if (imagePainter->isActive())
      imagePainter->end();
    
    delete imagePainter;
    
  }
  else {
    painter->drawImage(QPoint(0,0), *image);
    resize(this->size());
      
    //Message::toScreen("Repaint Event");
  
    if (painter->isActive())
      painter->end();
    
    delete painter;
  }

  event->accept();

}



//-------------------------------event----------------------------------------

bool GraphFace::event(QEvent *event)
{
  if (event->type() == QEvent::ToolTip) {
    //    Message::toScreen("Got ToolTip Event!");
    QHelpEvent *find = static_cast<QHelpEvent *>(event);
    bool ONDropSonde = false;
    QString time, measurement;
    //Message::toScreen(" x = "+QString().setNum(find->pos().x())+" y = "+QString().setNum(find->pos().y()));
    int index = pointAt(find->pos(),ONDropSonde);
    //Message::toScreen("Index = "+QString().setNum(index));
    if(index != -1) {
      if(ONDropSonde) {
	// Drop Sonde Measurement
	measurement.setNum(dropList->value(index).getPressure());
	time = dropList->value(index).getTime().toString("dd-hh:mm");
	QString message("DropWindSonde\nPressure = "
			+ measurement + " mb\n" + time);
	QToolTip::showText(find->globalPos(), message, this);
      }
      else {
	if ((unScalePressure(find->y()) > (pGMin)) && showPressure) {
	  // Pressure Point
	  measurement.setNum(VortexDataList->value(index).getPressure(), 'f', 0);
	  time = VortexDataList->value(index).getTime().toString("dd-hh:mm");
	  QString message("Pressure Estimate\nPressure = "
			  + measurement + " mb\n"+ time);
	  //			  +"\nClick For More Info...");
	  QToolTip::showText(find->globalPos(), message, this);
	}
	else {
	  if((unScaleDeficit(find->y()) > (dGMin)) && !showPressure) {
	    // Deficit Point
	    measurement.setNum(VortexDataList->value(index).getPressureDeficit());
	    time = VortexDataList->value(index).getTime().toString("dd-hh:mm");
	    QString message("Pressure Deficit Estimate\nPressure Deficit = "
			    + measurement +" mb\n"+ time);
	    //              + "\nClick For More Info...");
	    QToolTip::showText(find->globalPos(), message, this);
	  }
	  else {
	    // RMW Point
	    measurement.setNum(VortexDataList->value(index).getAveRMWnm(), 'f', 0);
	    time = VortexDataList->value(index).getTime().toString("dd-hh:mm");
	    QString message("Radius of Maximum Wind Estimate\nRMW = "
			    +measurement+" nm\n"+time);
	    QToolTip::showText(find->globalPos(), message , this);
	  }
	}
      }
    }
  }  
  return QWidget::event(event); // Something wrong with this guy...
}



//**********************--resize event--***************************************

void GraphFace::resizeEvent(QResizeEvent * /* event */)
{
  graph_width = width()-RIGHT_MARGIN_WIDTH - LEFT_MARGIN_WIDTH;
  graph_height = height()-TOP_MARGIN_HEIGHT - BOTTOM_MARGIN_HEIGHT;
  imageAltered = true;
  QImage* oldImage = image;
  image = NULL;
  image =  new QImage(this->width(),this->height(),
		      QImage::Format_ARGB32_Premultiplied);
  delete oldImage;
  update();
}



//**********************--saveImage--******************************************

void GraphFace::saveImage()
  // Saves the image of the plot to file
{
  Message::toScreen("GraphFace:Made it into save image");
  
  // Create new image with white background
  // We must use the same size as widget because otherwise it messes up the
  // Scaling properties ? - LM 1/7/07

  QImage *visibleImage = new QImage(this->width(),this->height(),
				    QImage::Format_ARGB32_Premultiplied);
  
  QPainter *painter = new QPainter(visibleImage);
  painter = updateImage(painter);
  if (painter->isActive())
    painter->end();
  delete painter;

  // Pick out file name and handle file type logistics

  QList<QByteArray> byteList = QImageWriter::supportedImageFormats();
  int index = byteList.indexOf("png");
  byteList.move(index,0);
  QFileDialog *fd = new QFileDialog(this, QString("Save Image As.."),
				    workingDirectory.path());
  QString filter;
  QStringList fileName;
  fd->setAcceptMode(QFileDialog::AcceptSave);
  fd->setConfirmOverwrite(true);
  QStringList imageTypes;
  for(int i = 0; i < byteList.count(); i++) {
    imageTypes.append(QString("Image (*."+byteList[i]+")"));
  }
  fd->setFilters(imageTypes);
  //fd->setDefaultSuffix("png");
  if(fd->exec()==1) {
    fileName = fd->selectedFiles();
    filter = fd->selectedFilter();
    if (filter.isEmpty()) {
      emit log(Message("Vortrac Cannot Save to This File Type"));
      return; 
    }
    filter = filter.remove("Image (*.");
    filter = filter.remove(")");
    byteList.move(byteList.indexOf(filter.toAscii()),0);
    if(fileName.isEmpty())
      return;
    if(!visibleImage->save(fileName[0], byteList[0])) {
      emit log(Message("GraphFace was unable to save"));
    } 
  }
  Message::toScreen("Made it out of GraphFace:SaveImage()");
}

void GraphFace::saveImage(QString fileName)
{
  if(!image->save(fileName,"PNG")) {
    Message::toScreen("Couldn't Save GraphFace Image to "+fileName);
  }
}

//***********************--newInfo (SLOT) --************************************

void GraphFace::newInfo(VortexList* gList)
{ 
  if(gList==NULL){
    VortexDataList = NULL;
    
    // Reset all member variables
    rmwMax = 0; autoRmwMax = 0;
    rGMax = 0; autoRGMax = 0;
    pressureMax = 0; autoPressureMax = 0;
    pGMax = 0; autoPGMax = 0;
    deficitMax = -1000; autoDeficitMax = -1000;
    dGMax = -1000; autoDGMax = -1000;
    rmwMin = 1000000; autoRmwMin = 100000;
    rGMin = 1000000; autoRGMin = 1000000;
    pressureMin = 1000000; autoPressureMin = 1000000;
    pGMin = 1000000; autoPGMin = 1000000;
    deficitMin = 100; autoDeficitMin = 100;
    dGMin = 100; autoDGMin = 100;
    rmwRange = 0;
    pressureRange = 0;
    deficitRange = 0;
    timeRange = -1;
    imageAltered = true;
    emit update();
    return;
  }
  
  gList->timeSort();
  if(first.isNull()) 
    first = gList->at(0).getTime();
   
  for(int i = 0; i < gList->count(); i++) {
    VortexData new_point = gList->value(i);
 
    checkPressure(&new_point);
    checkDeficit(&new_point);
    checkRmw(&new_point);
    checkRanges();
    
    // set time range as the number of seconds between the time of the first 
    // point and the time of this point
    if(last == QDateTime()) {
      if(first.secsTo(new_point.getTime())> timeRange){
	timeRange = first.secsTo(new_point.getTime());
      }
    }
    else {
      timeRange = first.secsTo(last);
    }
  
    if (timeRange == 0)
      timeRange = 60;
  }
  VortexDataList = NULL;
  VortexDataList = gList;
  imageAltered = true;
  emit update(); 
  
  return;
}



//************************--newDropSonde (SLOT)--******************************
void GraphFace::newDropSonde(VortexList* dropPointer)
  // Checks the Drop Wind Sonde pressure values to make sure they don't 
  // change the range
{
  VortexData new_drop = dropPointer->last(); 
  checkPressure(&new_drop);
  checkDeficit(&new_drop);
  checkRanges();

  if(last == QDateTime()) {
    if(first.secsTo(new_drop.getTime())> timeRange){
      timeRange = first.secsTo(new_drop.getTime());
    }
  }
  else {
    timeRange = first.secsTo(last);
  }
  
  dropList = NULL;
  dropList = dropPointer;  
  //connects member list pointer with global location of list
  
  imageAltered = true;
  emit update();
}



//***********************--manualAxes--****************************************
void GraphFace::manualAxes(const QString& name, const bool change)
{
  if(name == QString("manualAxes")) {
    if(!change){
      pGMin = autoPGMin;
      pressureMin = autoPressureMin;
      pGMax = autoPGMax;
      pressureMax = autoPressureMax;
      rmwMax = autoRmwMax;
      rmwMin = autoRmwMin;
      rGMax = autoRGMax;
      rGMin = autoRGMin;
      dGMin = autoDGMin;
      deficitMin = autoDeficitMin;
      dGMax = autoDGMax;
      deficitMax = autoDeficitMax;
      if((VortexDataList!=NULL)&&(VortexDataList->count() > 0)) 
	first = VortexDataList->at(0).getTime();
      else
	first = QDateTime();
      last = QDateTime();
      checkRanges();
      imageAltered = true;
      emit update();
    }
    autoAxes = !change;
  }
  if(name == QString("show_pressure")) {
    showPressure = change;
    imageAltered = true;
    emit update();
  }
}



//***********************--manualParameter--**********************************
void GraphFace::manualParameter(const QString& name, const float num)
{
  if (!autoAxes) {
    if (name == QString("pressmin")) {
      pGMin = num;
      pressureMin = num;
    }
    if (name == QString("pressmax")) {
      pGMax = num;
      pressureMax = num;
    }
    if (name == QString("rmwmin")) {
      rGMin = num;
      rmwMin = num;
    }
    if (name == QString("rmwmax")) {
      rGMax = num;
      rmwMax = num;
    }
    if (name == QString("defmin")) {
      dGMin = -1*num;
      deficitMin = -1*num;
    }
    if (name == QString("defmax")) {
      dGMax = -1*num;
      deficitMax = -1*num;
    }
    checkRanges();
    imageAltered = true;
    emit update();
  }
}

//***********************--manualParameter--**********************************
void GraphFace::manualParameter(const QString& name, const QString& time)
{
  // This should be used if there is a change in one of the time
  // parameters in the Graphics section of the XML config

  if (!autoAxes) {
    if (name == QString("startdate")) {
      first.setDate(QDate::fromString(time, Qt::ISODate));
      first.setTimeSpec(Qt::UTC);
    }
    if (name == QString("starttime")) {
      first.setTime(QTime::fromString(time, Qt::ISODate));
      first.setTimeSpec(Qt::UTC);
    }
    if (name == QString("enddate")) {
      last.setDate(QDate::fromString(time, Qt::ISODate));
      last.setTimeSpec(Qt::UTC);
    }
    if (name == QString("endtime")) {
      last.setTime(QTime::fromString(time, Qt::ISODate));
      last.setTimeSpec(Qt::UTC);
    }
    /*
    if(last.isNull)
      timeRange = 60;
    else
      timeRange = first.secsTo(last);
    */
    checkRanges();  // ?
    imageAltered = true;  
    emit update();
  }
}

//************************--makeKey--(SLOT)--**********************************

void GraphFace::makeKey()
{  
  
  QDialog *key = new QDialog();
  
  key->resize(300, 100);
  
  QFont labelFont("Times",12,QFont::Bold);
  
  QString *confidence1 = new QString;
  *confidence1 = confidence1->setNum(z1*100);
  QString *confidence2 = new QString;
  *confidence2 = confidence2->setNum(z2*100);
      
  QLabel* dropSondeLabel = new QLabel(tr("Dropwindsonde Central Pressure Estimate"));
  dropSondeLabel->setFont(labelFont);
  dropSondeLabel->setAlignment(Qt::AlignLeft);
      
  QLabel* pressureLabel = new QLabel(tr("Radar Central Pressure Estimate (mb)"));
  pressureLabel->setFont(labelFont);
  pressureLabel->setAlignment(Qt::AlignLeft);
  
  QLabel* pressureErrorLabel = new QLabel(confidence1->append(tr("% Confidence Interval of Pressure Estimate")));
  pressureErrorLabel->setFont(labelFont);
  pressureErrorLabel->setAlignment(Qt::AlignLeft);
  confidence1->chop(42);
  
  QLabel* pressureErrorLabel2 = new QLabel(confidence2->append(tr("% Confidence Interval of Pressure Estimate")));
  pressureErrorLabel2->setFont(labelFont);
  pressureErrorLabel2->setAlignment(Qt::AlignLeft);
  confidence2->chop(42);
  
  QLabel* rmwLabel = new QLabel(tr("Radius of Maximum Wind Estimate(km)"));
  rmwLabel->setFont(labelFont);
  rmwLabel->setAlignment(Qt::AlignLeft);

  QLabel* rmwErrorLabel = new QLabel(confidence1->append(tr("% Confidence Interval of Radius of Maximum Wind Estimate")));
  rmwErrorLabel->setFont(labelFont);
  rmwErrorLabel->setAlignment(Qt::AlignLeft);
  
  QLabel* rmwErrorLabel2 = new QLabel(confidence2->append(tr("% Confidence Interval of Radius of Maximum Wind Estimate")));
  rmwErrorLabel2->setFont(labelFont);
  rmwErrorLabel2->setAlignment(Qt::AlignLeft);

  QGroupBox *dropBox = new QGroupBox;
  QGroupBox *pressureBox = new QGroupBox;
  QGroupBox *rmwBox = new QGroupBox;
  
  KeyPicture *dropSondePicture = new KeyPicture(1, dropBrush, dropPen,
						QSize(30,30), dropBox);
  connect(dropSondePicture, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));
      
  KeyPicture *pressurePicture = new KeyPicture(2, pressureBrush, pressurePen, 
					       pstd1, pstd2, QSize(30,150), 
					       pressureBox);
  connect(pressurePicture, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));
  
  KeyPicture *rmwPicture = new KeyPicture(3, rmwBrush, rmwPen, rstd1, 
					  rstd2, QSize(30,150), rmwBox);
  connect(rmwPicture, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));
  
  QVBoxLayout *layout = new QVBoxLayout;
  QHBoxLayout *dropLayout = new QHBoxLayout;
  QVBoxLayout *dropLabelLayout = new QVBoxLayout;
  dropLabelLayout->addWidget(dropSondeLabel, 0, Qt::AlignCenter);
  dropLayout->addLayout(dropLabelLayout, 100);
  dropLayout->addWidget(dropSondePicture, 0, Qt::AlignCenter);
  dropLayout->addStrut(dropSondePicture->sizeHint().height());
  dropBox->setLayout(dropLayout);

  QHBoxLayout *pressureLayout = new QHBoxLayout;
  QVBoxLayout *pressureLabelLayout = new QVBoxLayout;
  pressureLabelLayout->addWidget(pressureErrorLabel2, 0, Qt::AlignCenter);
  pressureLabelLayout->addWidget(pressureErrorLabel, 0, Qt::AlignCenter);
  pressureLabelLayout->addWidget(pressureLabel, 0, Qt::AlignCenter);
  pressureLabelLayout->addStretch(100);
  pressureLayout->addLayout(pressureLabelLayout, 100);
  pressureLayout->addStrut(pressurePicture->sizeHint().height());
  pressureLayout->addWidget(pressurePicture);
  pressureBox->setLayout(pressureLayout);

  QHBoxLayout *rmwLayout = new QHBoxLayout;
  QVBoxLayout *rmwLabelLayout = new QVBoxLayout;
  rmwLabelLayout->addWidget(rmwErrorLabel2, 0, Qt::AlignCenter);
  rmwLabelLayout->addWidget(rmwErrorLabel, 0, Qt::AlignCenter);
  rmwLabelLayout->addWidget(rmwLabel, 0, Qt::AlignCenter);
  rmwLabelLayout->addStretch(100);
  rmwLayout->addLayout(rmwLabelLayout, 100);
  rmwLayout->addWidget(rmwPicture, 0, Qt::AlignCenter);
  rmwLayout->addStrut(rmwPicture->sizeHint().height());
  rmwBox->setMinimumWidth(rmwErrorLabel2->size().width()+
			  rmwPicture->sizeHint().width());
  rmwBox->setLayout(rmwLayout);
  layout->addWidget(dropBox);
  layout->addWidget(pressureBox);

  layout->addWidget(rmwBox);
  layout->setSpacing(rmwLayout->spacing());
  layout->setMargin(rmwLayout->margin());
  key->setLayout(layout);
  
  key->window()->setWindowTitle(tr("Key"));
  key->show();

}



//************************--updateTitle--*************************************
 void GraphFace::updateTitle(QWidget* labelWidget, 
			     const QString& new_Label)
{
  // Why do we need labelWidget
  //Message::toScreen("GraphFace: Update Title Parent Widget Was "+labelWidget->objectName());
  // For compatibility with one of the Qt set signals :)
 
  graphTitle = new_Label;
  imageAltered = true;
  update();
}

//************************--setWorkingDirectory--*****************************

void GraphFace::setWorkingDirectory(QDir &newDir)
{ 

  Message::toScreen("in the lions den");

  if(newDir.exists()) {
    Message::toScreen("Path does exist");
  }
  else {
    Message::toScreen("Path does not exist yet");
    newDir.mkpath(newDir.path());
    if(newDir.exists())
      Message::toScreen("Path exists NOW?");
  }
  if(!newDir.isAbsolute())
    newDir.makeAbsolute();
  QString newImageName = QString("autoImage");
  
  QFile *newImageFile = new QFile(newDir.filePath(newImageName+".png"));
  int i = 1;
  QString newName = newImageName;
  while(newImageFile->exists())
    {
      i++;
      newName = newImageName+QString().setNum(i);
      newImageFile = new QFile(newDir.filePath(newName+".png"));
    }

  //  imageFile->remove();
  //imageFile = newImageFile;

  newImageName = newName + ".png";
  //  saveImage(newDir.filePath(newImageName));
  //QImage visibleImage(*image);
  //if(!visibleImage.save(newImageFile,"PNG")) 
  //  Message::toScreen("Couldn't save graphface image in set working dir");

  //workingDirectory = newDir;
  autoImageName = newImageName;
  delete newImageFile;
  
  Message::toScreen("Made it");
}


//************************--converters and QPointF constructors--*************

void GraphFace::checkRanges()
   // this function checks to see if the ranges need to be update
   // it will also update ranges when necessary
{
   // these will be more useful later if we adjust to not rescale 
   // all the points unless necessary

  if (rmwRange != (rGMax - rGMin)) { // See if the rmw range has changed
    rmwRange = rGMax-rGMin;
  }
  if (pressureRange != (pGMax - pGMin)) {
      pressureRange = pGMax - pGMin;
      //call the function that will remake all the pressure points
  }
  if (deficitRange != (dGMax - dGMin)) {
    deficitRange = dGMax-dGMin;
  }
  if (!last.isNull() && !first.isNull()) {
    if (timeRange != first.secsTo(last)) {
      timeRange = first.secsTo(last);
    }
  }
  else {
    if((!first.isNull())&&(VortexDataList!=NULL)&&(VortexDataList->count()>0)){
      QDateTime tempLast = VortexDataList->at(VortexDataList->count()-1).getTime();
      for(int i = 0; i < VortexDataList->count()-1; i++) {
	if(VortexDataList->at(i).getTime() > tempLast)
	  tempLast = VortexDataList->at(i).getTime();
      }
      timeRange = first.secsTo(tempLast);
    }
    else
      timeRange = -1;
  }
}

void GraphFace::checkPressure(VortexData* point)
{
	
  if ((point->getPressure() + 
       point->getPressureUncertainty())> autoPressureMax) {
    
    // Updates the Max and Min for pressure, 
    autoPressureMax = (point->getPressure()
		       + point->getPressureUncertainty());
    autoPGMax = (point->getPressure() + 
		 2*point->getPressureUncertainty() + 1);       
    // And add on an little bit so nothing hits the sides
  }
  if((point->getPressure()-point->getPressureUncertainty()) 
     < autoPressureMin) {
    
    autoPressureMin = (point->getPressure()-point->getPressureUncertainty());
    autoPGMin = point->getPressure()
      -1* 2*point->getPressureUncertainty() - 1;
  }
  if(autoAxes) {
    pressureMax = autoPressureMax;
    pressureMin = autoPressureMin;
    pGMax = autoPGMax;
    pGMin = autoPGMin;
  } 
}

void GraphFace::checkDeficit(VortexData* point)
{
	
  if ((-1*point->getPressureDeficit() + 
       point->getDeficitUncertainty())> autoDeficitMax) {
    
    // Updates the Max and Min for pressure, 
    autoDeficitMax = (-1*point->getPressureDeficit()
		       + point->getDeficitUncertainty());
    autoDGMax = (-1*point->getPressureDeficit() + 
		 2*point->getDeficitUncertainty() + 1);       
    // And add on an little bit so nothing hits the sides
  }
  if((-1*point->getPressureDeficit()-point->getDeficitUncertainty()) 
     < autoDeficitMin) {
    
    autoDeficitMin = (-1*point->getPressureDeficit()-point->getDeficitUncertainty());
    autoDGMin = -1*point->getPressureDeficit() -1* 2*point->getDeficitUncertainty() - 1;
  }
  if(autoAxes) {
    deficitMax = autoDeficitMax;
    deficitMin = autoDeficitMin;
    dGMax = autoDGMax;
    dGMin = autoDGMin;
  } 
}

void GraphFace::checkRmw(VortexData* point)
{
  
  // We want to get statistics on all the rmws and then take the average

  float aveRmw = int(point->getAveRMWnm() + 0.5);
  float aveRmwUn = point->getAveRMWUncertaintynm();

  if ((aveRmw + aveRmwUn) > autoRmwMax) {
    // Update the Max and Min for rmw
    
    autoRmwMax = (aveRmw + aveRmwUn);
    autoRGMax = aveRmw + 2*aveRmwUn +.5;
    // And add on a half meter so nothing hits the sides of graph
  }
  
  if ((aveRmw - aveRmwUn)< autoRmwMin) {
    autoRmwMin = aveRmw - aveRmwUn;
    autoRGMin = aveRmw -1*2*aveRmwUn -.5;
  }
  if (autoAxes) {
    rmwMax = autoRmwMax;
    rmwMin = autoRmwMin;
    rGMax = autoRGMax;
    rGMin = autoRGMin;
  }

}

QPointF GraphFace::makePressurePoint(VortexData d)
{
  // take in data from newInfo and creates graphable point using real data 
  // (mbar -> QPointF)

  QPointF temp;
  if((d.getPressure()<pGMax)&&(d.getPressure()>pGMin)) {
    float tempTime = scaleTime(d.getTime());
    if(tempTime != -999)
      temp = QPointF(tempTime, scalePressure(d.getPressure()));
  }
  return (temp);
}

QPointF GraphFace::makeDeficitPoint(VortexData d)
{
  // take in data from newInfo and creates graphable point using real data 
  // (mbar -> QPointF)

  QPointF temp;
  if((-1*d.getPressureDeficit()<dGMax)&&(-1*d.getPressureDeficit()>dGMin)) {
    float tempTime = scaleTime(d.getTime());
    if(tempTime != -999)
      temp = QPointF(tempTime, scaleDeficit(-1*d.getPressureDeficit()));
  }
  return (temp);
}

QPointF GraphFace::makeRmwPoint(VortexData d)
{

  // This constructs a RMW point in the right scale cooresponding to the
  // Zeroth level rmw
  // takes in information from a VortexData and creates a real point 
  // ready for graphing (meters -> QPointF)

  QPointF temp;  
  if((d.getAveRMWnm()< rGMax)&&(d.getAveRMWnm()>rGMin)) {
    float tempTime = scaleTime(d.getTime());
    if(tempTime != -999)
      temp = QPointF(tempTime, scaleRmw(d.getAveRMWnm()));
  }
  return(temp);
}

QPointF GraphFace::makeRmwPoint(VortexData d, int bestLevel)
{

  // This constructs a RMW point in the right scale cooresponding to the
  // level indicated by bestLevel

  // takes in information from a VortexData and creates a real point 
  // ready for graphing (meters -> QPointF)

  QPointF temp;  
  if((d.getRMWnm(bestLevel)< rGMax)&&(d.getRMWnm(bestLevel)>rGMin)) {
    float tempTime = scaleTime(d.getTime());
    if(tempTime != -999)
      temp = QPointF(tempTime ,scaleRmw(d.getRMWnm(bestLevel)));
  }
  return(temp);
}

QPointF GraphFace::makeRmwPoint(VortexData d, float rmw)
{
  // This constructs a RMW point in the right scale for a given radius of 
  // maximum wind (rmw) is the case that we are not using a specific level
  // This will be used to plot the mean rmw of the level available within 
  // a certain threshold

  QPointF temp;  
  if((rmw < rGMax)&&(rmw > rGMin)) {
    float tempTime = scaleTime(d.getTime());
    if(tempTime != -999)
      temp = QPointF(tempTime, scaleRmw(rmw));
  }
  return(temp);
}

float GraphFace::scaleTime(QDateTime unscaled_time)
{
  // scales time and offsets the range so that no points hit the edges

  // checks to see if the point is in the time range
  if(unscaled_time < first) {
    return -999;
  }
  if(last!=QDateTime()) {
    if(unscaled_time > last) {
      return -999;
    }
  }
  float temp;
  temp = first.secsTo(unscaled_time);
  temp = temp + 60;
  temp = temp *((graph_width)/(timeRange+120));
  return temp;
}

QDateTime GraphFace::unScaleTime(float x)
{
  int seconds;
  double exactTime;
  exactTime =((x - LEFT_MARGIN_WIDTH)*((timeRange+120)/graph_width)-60);
  seconds = int(floor(exactTime+.5));
  if (seconds < 0)
    return first;
  return first.addSecs(seconds);
}

float GraphFace::scalePressure(float unscaled_pressure)
  //scales a pressure value to graphable units
{
  return(-1*((graph_height/(2*pressureRange))*((unscaled_pressure-pGMin)
					       +pressureRange)));
}   

float GraphFace::unScalePressure(float y)
{
  return((graph_height-(y-TOP_MARGIN_HEIGHT))*2*pressureRange/graph_height)
    +pGMin-pressureRange;
}

float GraphFace::scaleDeficit(float unscaled_deficit)
  //scales a pressure value to graphable units
{
  return(-1*((graph_height/(2*deficitRange))*((unscaled_deficit-dGMin)
					       +deficitRange)));
}   

float GraphFace::unScaleDeficit(float y)
{
  return((graph_height-(y-TOP_MARGIN_HEIGHT))*2*deficitRange/graph_height)
    +dGMin-deficitRange;
}


float GraphFace::scaleRmw(float unscaled_rmw)
  // scales an rmw value to graphable units
{
  return(-1*(graph_height/(2*rmwRange))*(unscaled_rmw - rGMin));
}    

float GraphFace::unScaleRmw(float y)
{
  return ((y-graph_height-TOP_MARGIN_HEIGHT)*((-2*rmwRange)/graph_height)
	  +rGMin);
}

float GraphFace::scaleDPressure(float unscaled_dPressure)
  // scales a DPressure value, this is different because it is 
  // an absolute length to be graphed
  // so the Min value offset and half graph values are not included
{
  return(-1*(graph_height/(2*pressureRange))*unscaled_dPressure);
}

float GraphFace::scaleDRmw(float unscaled_dRmw)
  // scales a DRmw value, this is an absolute length
  // so the offset used on scaling point values are not included
{
  return((-1*graph_height*unscaled_dRmw)/(2*rmwRange));
}

float GraphFace::scaleDDeficit(float unscaled_dDeficit)
  // scales a DDeficit value, this is different because it is 
  // an absolute length to be graphed
  // so the Min value offset and half graph values are not included
{
  return(-1*(graph_height/(2*deficitRange))*unscaled_dDeficit);
}

//think about overwriting quit() slot to dump QList for good form



float GraphFace::getSTDMultiplier(VortexData p, float z)
{
  // do something with the probability z to find and return the corresponding 
  // number multiple of standard deviations to display
  // assuming the uncertainty is one standard deviation

  if(p==VortexData())
    return 0;
 
  if (z == .67) {
    return 1;
  }
  else {
    return 2;
  }
}

int GraphFace::pointAt(const QPointF & position, bool& ONDropSonde)
{
  
  if((VortexDataList == NULL))
    return -1;
  //Message::toScreen("Didn't fall out of pointAt");
  ONDropSonde = false;
  QDateTime tmin = unScaleTime(position.x()-5);
  QDateTime tmax = unScaleTime(position.x()+5);
  //Message::toScreen("tmax = "+tmax.toString("dd-hh:mm:ss")+" tmin "+tmin.toString("dd-hh:mm:ss"));
  float pmax = unScalePressure(position.y()-5);
  float pmin = unScalePressure(position.y()+5);
  //Message::toScreen("Pmax = "+QString().setNum(pmax)+" Pmin = "+QString().setNum(pmin));
  float rmax = unScaleRmw(position.y()-5);
  float rmin = unScaleRmw(position.y()+5);
  //Message::toScreen("Rax = "+QString().setNum(rmax)+" Rmin = "+QString().setNum(rmin));
  float dmax = unScaleDeficit(position.y()-5);
  float dmin = unScaleDeficit(position.y()+5);
  for (int i = 0; i < VortexDataList->size(); i++) {
    //if(i==0)
    //Message::toScreen("First: T~ "+VortexDataList->at(i).getTime().toString("dd-hh:mm:ss")+" P ~ "+QString().setNum(VortexDataList->at(i).getPressure()));
    if(VortexDataList->at(i).getTime()<=tmax)
      if(VortexDataList->at(i).getTime()>=tmin) {
	if((VortexDataList->at(i).getPressure() <= pmax)
	   && (VortexDataList->at(i).getPressure() >= pmin)
	   && showPressure) {
	  return i;
	}
	if((VortexDataList->at(i).getAveRMWnm() <= rmax) 
	   && (VortexDataList->at(i).getAveRMWnm() >= rmin)) {
	  return i;
	}
	if((-1*VortexDataList->at(i).getPressureDeficit() <= dmax)
	   &&(-1*VortexDataList->at(i).getPressureDeficit() >= dmin)
	   && !showPressure)
	  return i;
      }
      else;
    else break;
  }

  if(dropList==NULL)
    return -1;
  else {
    for(int i = 0; i < dropList->size(); i++) {
      if(dropList->at(i).getTime()<tmax)
	if(dropList->at(i).getTime()>tmin)
	  if((dropList->at(i).getPressure() < pmax)  
	     && (dropList->at(i).getPressure() > pmin)) {
	    ONDropSonde = true;
	    return i;
	  }
	  else;
	else;
	
      else break;
    }
    return -1;
  }
}


void GraphFace::setColors()
{
  //--------------------------------------Color Decisions---------------------
  // Hard Coding in colors and widths of all the member pens to be used

  // These initializations are used in paintEvent
  pressurePen.setColor(Qt::red);
  pressureBrush.setColor(Qt::red);
  pressureBrush.setStyle(Qt::SolidPattern);
  pstd1.setColor(Qt::magenta);
  pstd1.setWidth(2);
  pstd2.setColor(Qt::darkRed); 
  rmwPen.setColor(Qt::blue);
  rmwBrush.setColor(Qt::blue);
  rmwBrush.setStyle(Qt::SolidPattern);
  rstd1.setColor(Qt::cyan);
  rstd1.setWidth(2);
  rstd2.setColor(Qt::darkBlue);
  dropPen.setColor(Qt::black);
  dropBrush.setColor(Qt::black);
  dropBrush.setStyle(Qt::SolidPattern);
  square.setHeight(4);
  square.setWidth(4);
  drop.setHeight(6);
  drop.setWidth(6);
}


//--------------------------------UPDATEIMAGE--------------------------------
QPainter* GraphFace::updateImage(QPainter* painter)

  // QPainter object does the painting, 
  // all painting is done relative to the position of QPainter
  // The save and restore functions are used to reset the painter position 
  // to the origin of the graphable area

{
  //  QImage *imageTemp = new QImage(graph_width+LEFT_MARGIN_WIDTH+RIGHT_MARGIN_WIDTH,graph_height+TOP_MARGIN_HEIGHT+BOTTOM_MARGIN_HEIGHT,QImage::Format_ARGB32_Premultiplied);

  //QPainter* painter = new QPainter(imageTemp);
  //QPainter* painter = new QPainter(imageTemp);
  painter->setBackgroundMode(Qt::OpaqueMode);
  painter->setRenderHint(QPainter::Antialiasing);
  //this option makes lines appear smoother;

  //DRAW IN ALL AXES

  painter->translate(LEFT_MARGIN_WIDTH,TOP_MARGIN_HEIGHT);     
  // moves painter to top corner left axis
  painter->drawLine(0,0,0,graph_height);                       
  // draw left axis
  painter->drawLine(0,0,graph_width,0);                        
  // draw top line around graph space
  painter->translate(graph_width, graph_height);               
  // moves painter to bottom of right axis
  painter->drawLine(0,0,-1*graph_width, 0);                    
  // draw bottom axis
  painter->drawLine(0,0,0,-1*graph_height);                    
  // draw right axis

  //BACK TO ORIGIN OF AXISES
  painter->translate(-1*graph_width,0);

  //Draw in all Titles and Axis Lables - Main Titles/Lables - 

  //painter->save();
  QFont titleFont("Times", 12, QFont::Bold);
  painter->setFont(titleFont);
  painter->drawText(QRectF(0,(-1*(TOP_MARGIN_HEIGHT+graph_height)),
			   graph_width,TOP_MARGIN_HEIGHT),
		    graphTitle,QTextOption(Qt::AlignCenter));

  painter->save();
 
  painter->rotate(270);
  if(showPressure) {
  painter->setPen(pressurePen);
  painter->drawText(QRectF(0, -1*LEFT_MARGIN_WIDTH, graph_height,
			   .3*LEFT_MARGIN_WIDTH), tr("Central Pressure (mb)"), 
		    QTextOption(Qt::AlignCenter));
  }
  else {
    painter->setPen(pressurePen);
    painter->drawText(QRectF(0, -1*LEFT_MARGIN_WIDTH, graph_height,
			     .3*LEFT_MARGIN_WIDTH), 
		      tr("Central Pressure Deficit (mb)"), 
		      QTextOption(Qt::AlignCenter));
  }
  painter->restore();

  painter->save();
  painter->translate(graph_width+RIGHT_MARGIN_WIDTH, -1*graph_height);
  painter->rotate(90);
  painter->setPen(rmwPen);
  painter->drawText(QRectF(0,0,graph_height,.4*RIGHT_MARGIN_WIDTH), 
		    tr("Radius of Maximum Wind (km)"), 
		    QTextOption(Qt::AlignCenter)); 
  painter->restore();

  painter->save();
  painter->translate(0,.45*BOTTOM_MARGIN_HEIGHT);
  painter->drawText(QRectF(0,0,graph_width, .55*BOTTOM_MARGIN_HEIGHT), 
		    tr("Time (Day-Hours:Minutes)"),
		    QTextOption(Qt::AlignCenter));
  painter->restore();
  //  painter->restore();

  QFont tickFont("Times", 10);
  painter->setFont(tickFont);

  //--------Pressure Axis-----------------------------------------------------
  if(showPressure) {
    //creates pressure tics and labels for the pressure axis
    
    float pPosition = 800;          // this variable is created for incrementing 
    // through the possible pressure range
    
    // The loop creates a tic mark and a label for every 5 mbar 
    // multiple within the range of data given by the points
    
    // The painter keeps its position along the line and draws 
    // both tics and text to the left
    
    while(pPosition <= pGMax) {
      painter->save();
      if (pPosition >= (pGMin-pressureRange)) {
	painter->translate(0,scalePressure(pPosition));
	QString iString;
	iString.setNum(pPosition);
	painter->drawLine(0,0,-1*(LEFT_MARGIN_WIDTH/7),0);
	painter->drawText(QPointF(-.55*LEFT_MARGIN_WIDTH,0),iString);
      }
      painter->restore();
      if(pressureRange > 150) 
	pPosition+=20;
      else {
	if(pressureRange > 100)
	  pPosition +=15;
	else {
	  if(pressureRange > 50)
	    pPosition +=10;                            
	  else {
	    if(pressureRange > 25)
	      pPosition +=5;
	    else
	      pPosition +=3;
	  }
	}
      }
    } 
  }
  else {
    // For pressure deficit axis
    float dPosition = -300;       // this variable is created for incrementing 
                                  // through the possible deficit range
    
    // The loop creates a tic mark and a label for every 5 mbar 
    // multiple within the range of data given by the points
    
    // The painter keeps its position along the line and draws 
    // both tics and text to the left
    
    while(dPosition <= dGMax) {
      painter->save();
      if (dPosition >= (dGMin-deficitRange)) {
	painter->translate(0,scaleDeficit(dPosition));
	QString iString;
	iString.setNum(-1.0*dPosition);
	painter->drawLine(0,0,-1*(LEFT_MARGIN_WIDTH/7),0);
	painter->drawText(QPointF(-.55*LEFT_MARGIN_WIDTH,0),iString);
      }
      painter->restore();
      if(deficitRange >= 150)
	dPosition +=20;
      else {
	if(deficitRange >= 100)
	  dPosition +=15;
	else {
	  if(deficitRange >= 50)
	    dPosition +=10;                            
	  else {
	    if(deficitRange >= 25)
	      dPosition +=5;
	    else
	      dPosition +=3;
	  }
	}
      }
    } 
  }
  
  //-----------TIME Axis--------------------------------------------------

  //draws tics and labels for time margin
  int t_increment = 0;              
                        //the increment for time spacing in seconds
  QDateTime tPosition = QDateTime();
                       // a QDateTime for incrementing through the time range
  if (timeRange >= 0) {   
    // This if else tree decides when which time incrementing 
    // system should be used
    float timePerLabel = timeRange/(graph_width*(10.0/600.0));
    
    if (timePerLabel < 60)
      t_increment = 60;
    else{
      if(timePerLabel < 120)
	t_increment = 120;
      else{
	if(timePerLabel < 300)
	  t_increment = 300;
	else{
	  if(timePerLabel < 600)
	    t_increment = 600;
	  else{
	    if(timePerLabel < 900)
	      t_increment = 900;
	    else{
	      if(timePerLabel < 1800)
		t_increment = 1800;
	      else{
		int j = 1;
		while(t_increment == 0) {
		  if (timePerLabel < 3600*j) {
		    t_increment = 3600*j;
		  }
		  j++;}}}}}}}
    
    
    QDateTime temp = first;
    
    temp = temp.addSecs(-60);
    
    int initial = t_increment - (temp.time().second()
				 +(temp.time().minute()%(t_increment/60))*60);

                    // this allows the tic marks and lables to start
                    // at an even time, the nearest fitting increment
    if(initial==t_increment)  { 
      tPosition = temp;
    }
    else {
      tPosition = temp;
      tPosition = tPosition.addSecs(initial);
    }
    
    // allows for the drawing of tic marks and labels below the 
    // time axis as the painter
    // moves along the axis from left to right
    
    while (tPosition <= (first.addSecs((int)timeRange+60)))
	{
	  painter->save();
	  painter->translate(scaleTime(tPosition),0);
	  painter->drawLine(0,0,0,BOTTOM_MARGIN_HEIGHT/7);
	  //!! The date and time format is hardcoded in here!!
	  painter->drawText(QPointF(-25,(.45*BOTTOM_MARGIN_HEIGHT)),
			    tPosition.toString("dd-hh:mm"));
	  tPosition = tPosition.addSecs(t_increment);
	  painter->restore();
	}
    } 
  painter->translate(graph_width,0);
  

  //------------------------RMW AXIS-------------------------------------------
  //creates radius of maximum wind tics and labels for the rmw  axis
  
  float rPosition = 0;
              // used for incrementing through the range of values
  while(rPosition <= (rGMax+rmwRange)) {
    
    painter->save();
    if (rPosition >= (rGMin)) {
      painter->translate(0,scaleRmw(rPosition));
      QString iString;
      iString.setNum(rPosition);
      painter->drawLine(0,0,(RIGHT_MARGIN_WIDTH/7),0);
      painter->drawText(QPointF(.2*RIGHT_MARGIN_WIDTH,0),iString);
    }
    painter->restore();
    
    // if else chain for deciding the value to increment with 
    
    if(rmwRange > 15)
      rPosition +=5;
    else {
      if (rmwRange > 8)
	rPosition +=2;
      else {
	rPosition++;
      }
    }
  }
  
  painter->translate(-1*graph_width, 0);

  // this leaves the space between the axises at graph_width 
  // by graph_height tall for now


  //-------------------------------Draw Pressure Points--------------

  if(!(VortexDataList == NULL) 
     &&!VortexDataList->isEmpty()) {       // Process is used provided data 
                                           // points are already available
    if(showPressure) {
      painter->setPen(pressurePen);
      painter->setBrush(pressureBrush);
      for (int i=0;i<VortexDataList->size();i++) {
	
	//-------------------------------ErrorBars----------------------------
	
	// This draws the errorbars about the point 
	QPointF xypoint = makePressurePoint(VortexDataList->value(i));
	
	if(!xypoint.isNull()) {
	  if (VortexDataList->at(i).getPressureUncertainty()>0) {                           // if uncertainty = 0 there are no bars
	    float errorBarHeight = scaleDPressure(VortexDataList->at(i).getPressureUncertainty());
	    
	    float upper2, upper1, lower1, lower2;
	    bool upperBar2, upperBar1, lowerBar1, lowerBar2;
	    
	    
	    if((-1*xypoint.y()+(-2*errorBarHeight))>graph_height) { 
	      upper2 = graph_height -(-1*xypoint.y()); 
	      upperBar2 = false; }
	    else {
	      upper2 = -2*errorBarHeight;
	      upperBar2 = true; }
	    
	    if ((-1*xypoint.y()-(-2*errorBarHeight))<graph_height/2) {
	      lower2 = -1*xypoint.y()-graph_height/2;
	      lowerBar2 = false; }
	    else {
	      lower2 = -2*errorBarHeight;
	      lowerBar2 = true; }
	    
	    if((-1*xypoint.y()+(-1*errorBarHeight))>graph_height) {
	      upper1 = graph_height -(-1*xypoint.y());
	      upperBar1 = false; }
	    else {
	      upper1 = -1*errorBarHeight;
	      upperBar1 = true; }
	    
	    if ((-1*xypoint.y()-(-1*errorBarHeight))<graph_height/2) {
	      lower1 = -1*xypoint.y()-graph_height/2;
	      lowerBar1 = false; }
	    else {
	      lower1 = -1*errorBarHeight;
	      lowerBar1 = true; }
	    
	    painter->save();            // Save painter position at origin
	    painter->translate(xypoint);
	    
	    // Draws the second error bars, two standard deviations 
	    // (pressureUncertainty)
	    
	    painter->setPen(pstd2);
	    painter->drawLine(QPointF(0,0),
			      QPointF(0,-1*upper2));
	    if(upperBar2)
	      painter->drawLine(QPointF(-2,-1*upper2),
				QPointF(2,-1*upper2));
	    painter->drawLine(QPointF(0,0),
			      QPointF(0,lower2));
	    if(lowerBar2)
	      painter->drawLine(QPointF(-2,lower2),
				QPointF(2,lower2));
	    
	    // Draws the first error bars at one standard deviation 
	    // (pressureUncertainty)
	    painter->setPen(pstd1);
	    painter->drawLine(QPointF(0,0),
			      QPointF(0,-1*upper1));
	    if(upperBar1)
	      painter->drawLine(QPointF(-1.5,-1*upper1),
				QPointF(1.5,-1*upper1));
	    painter->drawLine(QPointF(0,0),
			      QPointF(0,lower1));
	    if(lowerBar1)
	      painter->drawLine(QPointF(-1.5,lower1),
				QPointF(1.5,lower1));
	    
	    painter->restore();    // restores the painters location 
	    // to saved address
	    
	  }		  
	  // This is where the point is drawn as a circle inside an box, 
	  // who's size is set in GraphFace.h
	  
	  square.moveCenter(xypoint);
	  painter->drawEllipse(square);
	}
      }
      
      // This loop connects all the pressure points in the VortexDataList 
      // to the previous one with a line
      int j = 1;
      while(j < VortexDataList->size())	{
	QPointF point1 = makePressurePoint(VortexDataList->value(j-1));
	QPointF point2 = makePressurePoint(VortexDataList->value(j));
	if(!point1.isNull()&&!point2.isNull())
	  painter->drawLine(point1, point2);
	j++;
      }
    }
    else {
      painter->setPen(pressurePen);
      painter->setBrush(pressureBrush);
      for (int i=0;i<VortexDataList->size();i++) {
	
	//-------------------------------ErrorBars----------------------------
	
	// This draws the errorbars about the point 
	QPointF xypoint = makeDeficitPoint(VortexDataList->value(i));
	
	if(!xypoint.isNull()) {
	  if (VortexDataList->at(i).getDeficitUncertainty()>0) {                           // if uncertainty = 0 there are no bars
	    float errorBarHeight = scaleDDeficit(VortexDataList->at(i).getDeficitUncertainty());
	    
	    float upper2, upper1, lower1, lower2;
	    bool upperBar2, upperBar1, lowerBar1, lowerBar2;
	    
	    
	    if((-1*xypoint.y()+(-2*errorBarHeight))>graph_height) { 
	      upper2 = graph_height -(-1*xypoint.y()); 
	      upperBar2 = false; }
	    else {
	      upper2 = -2*errorBarHeight;
	      upperBar2 = true; }
	    
	    if ((-1*xypoint.y()-(-2*errorBarHeight))<graph_height/2) {
	      lower2 = -1*xypoint.y()-graph_height/2;
	      lowerBar2 = false; }
	    else {
	      lower2 = -2*errorBarHeight;
	      lowerBar2 = true; }
	    
	    if((-1*xypoint.y()+(-1*errorBarHeight))>graph_height) {
	      upper1 = graph_height -(-1*xypoint.y());
	      upperBar1 = false; }
	    else {
	      upper1 = -1*errorBarHeight;
	      upperBar1 = true; }
	    
	    if ((-1*xypoint.y()-(-1*errorBarHeight))<graph_height/2) {
	      lower1 = -1*xypoint.y()-graph_height/2;
	      lowerBar1 = false; }
	    else {
	      lower1 = -1*errorBarHeight;
	      lowerBar1 = true; }
	    
	    painter->save();            // Save painter position at origin
	    painter->translate(xypoint);
	    
	    // Draws the second error bars, two standard deviations 
	    
	    painter->setPen(pstd2);
	    painter->drawLine(QPointF(0,0),
			      QPointF(0,-1*upper2));
	    if(upperBar2)
	      painter->drawLine(QPointF(-2,-1*upper2),
				QPointF(2,-1*upper2));
	    painter->drawLine(QPointF(0,0),
			      QPointF(0,lower2));
	    if(lowerBar2)
	      painter->drawLine(QPointF(-2,lower2),
				QPointF(2,lower2));
	    
	  // Draws the first error bars at one standard deviation 
	    
	    painter->setPen(pstd1);
	    painter->drawLine(QPointF(0,0),
			      QPointF(0,-1*upper1));
	    if(upperBar1)
	      painter->drawLine(QPointF(-1.5,-1*upper1),
				QPointF(1.5,-1*upper1));
	    painter->drawLine(QPointF(0,0),
			      QPointF(0,lower1));
	    if(lowerBar1)
	      painter->drawLine(QPointF(-1.5,lower1),
				QPointF(1.5,lower1));
	    
	    painter->restore();    // restores the painters location 
	    // to saved address
	    
	  }		  
	  // This is where the point is drawn as a circle inside an box, 
	  // who's size is set in GraphFace.h
	  
	  square.moveCenter(xypoint);
	  painter->drawEllipse(square);
	}
      }
      
      // This loop connects all the deficit points in the VortexDataList 
      // to the previous one with a line
      int j = 1;
      while(j < VortexDataList->size())	{
	QPointF point1 = makeDeficitPoint(VortexDataList->value(j-1));
	QPointF point2 = makeDeficitPoint(VortexDataList->value(j));
	if(!point1.isNull()&&!point2.isNull())
	  painter->drawLine(point1, point2);
	j++;
      }
    }
    //---------------------------------------Draw RMW Points------------------
    
      painter->setPen(rmwPen);
      painter->setBrush(rmwBrush);
      QPointF lastPoint;

      for (int i=0;i<VortexDataList->size();i++) {          
	
	// uses the loop to move through all data points

	float aveRmw = VortexDataList->at(i).getAveRMWnm();
	float aveRmwUn = VortexDataList->at(i).getAveRMWUncertaintynm();

	float rawErrorBarHeight = aveRmwUn;

	QPointF xypoint = makeRmwPoint(VortexDataList->at(i), aveRmw);
	if(!xypoint.isNull()) {
	  
	  //-------------------------------ErrorBars---------------------------
	  // Draws errorbars about the radius of max wind points based 
	  // on their uncertainties
	  
	  if (rawErrorBarHeight > 0) { 
	    // if uncertainty = 0 there are no bars

	    float errorBarHeight = scaleDRmw(rawErrorBarHeight);
	    
	    float upper2, upper1, lower1, lower2;
	    bool upperBar2, upperBar1, lowerBar1, lowerBar2;
	    
	    
	    if((-1*xypoint.y()+(-2*errorBarHeight))>graph_height/2) { 
	      upper2 = graph_height/2 -(-1*xypoint.y()); 
	      upperBar2 = false; }
	    else {
	      upper2 = -2*errorBarHeight;
	      upperBar2 = true; }
	    
	    if ((-1*xypoint.y()-(-2*errorBarHeight))<0) {
	      lower2 = -1*xypoint.y();
	      lowerBar2 = false; }
	    else {
	      lower2 = -2*errorBarHeight;
	      lowerBar2 = true; }
	    
	    if((-1*xypoint.y()+(-1*errorBarHeight))>graph_height/2) {
	      upper1 = graph_height/2 -(-1*xypoint.y());
	      upperBar1 = false; }
	    else {
	      upper1 = -1*errorBarHeight;
	      upperBar1 = true; }
	    
	    if ((-1*xypoint.y()-(-1*errorBarHeight))<0) {
	      lower1 = -1*xypoint.y();     
	      lowerBar1 = false; }
	    else {
	      lower1 = -1*errorBarHeight;
	      lowerBar1 = true; }
	    
	    painter->save();            // Save painter position at origin
	    painter->translate(xypoint);
	    
	    // Draws the second error bars, two standard deviations 
	    // (rmwUncertainty)
	    
	    painter->setPen(rstd2);
	    painter->drawLine(QPointF(0,0),
			      QPointF(0,-1*upper2));
	    if(upperBar2)
	      painter->drawLine(QPointF(-2,-1*upper2),
				QPointF(2,-1*upper2));
	    painter->drawLine(QPointF(0,0),
			      QPointF(0,lower2));
	    if(lowerBar2)
	      painter->drawLine(QPointF(-2,lower2),
				QPointF(2,lower2));
	      // Draws the first error bars at one standard deviation 
	      // (rmwUncertainty)
	    painter->setPen(rstd1);
	    painter->drawLine(QPointF(0,0),
			      QPointF(0,-1*upper1));
	    if(upperBar1)
	      painter->drawLine(QPointF(-1.5,-1*upper1),
				QPointF(1.5,-1*upper1));
	    painter->drawLine(QPointF(0,0),
			      QPointF(0,lower1));
	    if(lowerBar1)
	      painter->drawLine(QPointF(-1.5,lower1),
				QPointF(1.5,lower1));
	    
	    painter->restore();    // restores the painters location 
	    // to saved address
	      
	  }
       
       	  // The rmw point is similar to the pressrue point, 
	  // drawn as an ellipse in a box
	  
	  square.moveCenter(xypoint);
	  painter->drawEllipse(square);
	  if((!xypoint.isNull())&&(!lastPoint.isNull())) {
	    painter->drawLine(xypoint, lastPoint);
	  }
	  lastPoint = xypoint;
	}
      }
      
      /*
      //connects all the rmw points together with lines to the previous point
      int i = 1;
      while(i<VortexDataList->count()) {
	QPointF point1 = makeRmwPoint(VortexDataList->value(i-1));
	QPointF point2 = makeRmwPoint(VortexDataList->value(i));
	if(!point1.isNull()&&!point2.isNull())
	  painter->drawLine(point1,point2);
	i++;
      }
      */
  }
  //-----------------------------------Draw Drops-------------------------------
  
  if(!(dropList == NULL)
     &&!dropList->isEmpty())              // Goes through the same process 
                                          // of drawing drops but with
    {                                     // a different member box to draw 
                                          // an ellipse in 
      painter->setPen(dropPen);

      painter->setBrush(dropBrush);
      for (int i = 0; i < dropList->size();i++) {
	QPointF xypoint = makePressurePoint(dropList->value(i));
	if(!xypoint.isNull()) {
	  drop.moveCenter(xypoint);
	  painter->drawEllipse(drop);
	}
      }
    }
  //if (painter->isActive())
  //  painter->end();
  //delete painter;
  //image = NULL;
  
  //image = imageTemp;

  // Memory leak here?
  //imageTemp = NULL;
  //delete imageTemp;
  //autoSave();

  return painter;
}


bool GraphFace::autoSave()
{
  QImage visibleImage(*image);
  if(!imageFile->open(QIODevice::WriteOnly))
    Message::toScreen("can't open imagefile");
  if(!visibleImage.save(imageFile,"PNG")) {
    emit log(Message(tr("Failed to Auto-Save Graph Image")));
    imageFile->close();
    return false;
  }
  imageFile->close();
  return true;
}

void GraphFace::catchLog(const Message& message)
{
  emit log(message);
}


//--------------------------------ALTUPDATEIMAGE--------------------------------
  // QPainter object does the painting, 
  // all painting is done relative to the position of QPainter
  // The save and restore functions are used to reset the painter position 
  // to the origin of the graphable area


void GraphFace::setImageFileName(QString newName)
{
  autoImageName = newName;
}
