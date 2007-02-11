/*
 * DiagnosticPanel.cpp
 * VORTRAC
 *
 *  Created by Lisa Mauger on 04/09/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */


#include "DiagnosticPanel.h"
#include <QDateTime>
#include <QGroupBox>
#include <QTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QBrush>
#include <QGradient>
#include <QPen>
#include <QSize>
#include <QColorDialog>

DiagnosticPanel::DiagnosticPanel(QWidget *parent)
  :QWidget(parent)

  /*
   * This panel is used to display widgets that provide diagnostic
   * information on the operational state of the vortrac algorithm.
   * This includes the vcp, problem indicator and messages, and the time.
   */

{
  this->setObjectName("diagnosticPanel");

  // timer is used to create updates for the display clock

  timer = new QTimer(this);
  timer->setSingleShot(false);
  timer->setInterval(1000);
  timer->start();

  QGroupBox *clockBox = new QGroupBox("Current Time UTC");
  clockBox->setAlignment(Qt::AlignHCenter);
 
  clock = new QLCDNumber(clockBox);
  clock->setSegmentStyle(QLCDNumber::Flat);
  clock->resize(150,100);

  QGridLayout *clockLayout = new QGridLayout;
  clockLayout->addWidget(clock,0,1,1,1);
  //clockLayout->setRowMinimumHeight(1,100);
  clockLayout->setRowStretch(0,8);
  clockBox->setLayout(clockLayout);
  //clockBox->resize(230,150);

  //QPushButton *color = new QPushButton("color", this);
  //connect(color, SIGNAL(pressed()), this, SLOT(pickColor()));

  QPushButton *lightButton = new QPushButton("Next Signal Pattern");
  connect(lightButton, SIGNAL(pressed()), this, SLOT(testLight()));

  QPushButton *stormButton = new QPushButton("Next Storm Signal");
  connect(stormButton, SIGNAL(pressed()), this, SLOT(testStormSignal()));
  
  // StormSignal is used to alert user to 
  // noticable changes in vortex properties

  stormSignal = new StormSignal(QSize(225,225), this);
  connect(stormSignal, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));
  
  QHBoxLayout *stormLayout = new QHBoxLayout;
  stormLayout->addStretch();
  stormLayout->addWidget(stormSignal);
  stormLayout->addStretch();

  // Stoplight used to show operational status of the vortrac GUI

  lights = new StopLight(QSize(225,80), this);
  connect(lights, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));

  QHBoxLayout *lightsLayout = new QHBoxLayout;
  lightsLayout->addStretch();
  lightsLayout->addWidget(lights);
  lightsLayout->addStretch();

  // Displays current radar vcp
  QHBoxLayout *vcpLayout = new QHBoxLayout;
  QLabel *vcpLabel = new QLabel(tr("Current Radar VCP"));
  vcpString = new QString(tr("N/A"));
  vcp = new QLineEdit(*vcpString); 
  vcp->setReadOnly(true);
  vcpLayout->addWidget(vcpLabel);
  vcpLayout->addWidget(vcp);
  
  // Displays warning message that may accompany the change in stoplight
  // signal

  stopLightWarning = new QLineEdit();
  stopLightWarning->setReadOnly(true);

  // Displays meassage that may accompany the change in stormSignal

  stormSignalWarning = new QLineEdit();
  stormSignalWarning->setReadOnly(true);

  QVBoxLayout *main = new QVBoxLayout();
  // main->addStretch();
  main->addWidget(clockBox);
  main->addStretch();
  main->addLayout(vcpLayout);
  main->addStretch();
  main->addLayout(stormLayout);
  main->addWidget(stormSignalWarning);
  main->addStretch();
  main->addLayout(lightsLayout);
  main->addWidget(stopLightWarning);

  //main->addWidget(color); // I use this when I need to select colors
                            // for coding throw away in finished product -LM
  main->addWidget(lightButton);
  main->addWidget(stormButton);

  setLayout(main);
  
  // Used to set the initial color of the stoplight
  
  lights->changeColor(Green);

  dummy = 0;
  
  connect(timer, SIGNAL(timeout()), 
	  this, SLOT(updateClock()), Qt::DirectConnection);
  
  updateClock();
}

DiagnosticPanel::~DiagnosticPanel()
{
	delete vcpString;
	//delete cappiDisplay;
}

void DiagnosticPanel::updateClock()
{
  // Updates display clock
  QString displayTime;
  displayTime = QDateTime::currentDateTime().toUTC().toString("hh:mm");
  clock->display(displayTime);	
  //if(hasNewCappi && !cappiLaunch->isEnabled()) {
  //  cappiLaunch->setEnabled(true);
  //}
  update();

}

void DiagnosticPanel::catchLog(const Message& message)
{
  emit log(message);
}

void DiagnosticPanel::pickColor()

{
  QColorDialog::getColor(); 
}

void DiagnosticPanel::updateVCP(const int newVCP)
{
	vcpString->setNum(newVCP);
	// Still not fixed even though there is no direct connection across threads now
	vcp->clear();
	vcp->insert(*vcpString);
}
/*
void DiagnosticPanel::updateCappi(const GriddedData* newCappi)
{
        cappi=newCappi;
	// Got a cappi now, turn on the button
	cappiDisplay->constructImage(cappi);
	hasNewCappi = true;
}
*/
void DiagnosticPanel::testLight()
{
  StopLightColor testColor;
  switch(dummy) 
    {
    case 1:
      testColor = BlinkRed;
      break;
    case 2:
      testColor = Red;
      break;
    case 3:
      testColor = BlinkYellow;
      break;
    case 4:
      testColor = Yellow;
      break;
    case 5:
      testColor = BlinkGreen;
      break;
    case 6:
      testColor = Green;
      break;
    case 7:
      testColor = AllOn;
      break;
    default:
      testColor = AllOff;
      break;
    }
  lights->changeColor(testColor);
  if(dummy < 7)
    dummy++;
  else 
    dummy = 0;
}

void DiagnosticPanel::testStormSignal()
{
  StormSignalStatus testStatus;
  switch(stormDummy) 
    {
    case 1:
      testStatus = RapidDecrease;
      break;
    case 2:
      testStatus = RapidIncrease;
      break;
    case 3:
      testStatus = OutOfRange;
      break;
    case 4:
      testStatus = SimplexError;
      break;
    case 5:
      testStatus = Ok;
      break;
    default:
      testStatus = Nothing;
      break;
    }
  stormSignal->changeStatus(testStatus);
  if(stormDummy < 5)
    stormDummy++;
  else 
    stormDummy = 0;
}
/*
void DiagnosticPanel::launchCappi()
{
	// Fill the pixmap with the current cappi dat
	//cappiDisplay->constructImage(cappi);
	
	// Open the floating widget to look at the Cappi
	cappiDisplay->show();
	
}
*/

void DiagnosticPanel::changeStopLight(StopLightColor newColor,
				      const QString newMessage)
{
  lights->changeColor(newColor);

  if(newMessage!=QString()) {
    /*if(stopLightWarning->toPlainText()==QString())
      stopLightWarning->insertPlainText(newMessage);
    else {
      QString totalMessage = stopLightWarning->toPlainText()+newMessage;
      stopLightWarning->insertPlainText(totalMessage); 
    }
    */
    stopLightWarning->clear();
    stopLightWarning->insert(newMessage);
  }
}

void DiagnosticPanel::changeStormSignal(StormSignalStatus status, 
					const QString newMessage)
{
  stormSignal->changeStatus(status);
  if(newMessage!=QString()) {
    /*
    if(stormSignalWarning->toPlainText()==QString())
      stormSignalWarning->insertPlainText(newMessage);
    else {
      QTextDocument *oldDoc = stormSignalWarning->document();
      QString totalMessage = oldDoc->toPlainText()+newMessage;
      oldDoc->setPlainText(totalMessage);
      //QString totalMessage = stormSignalWarning->toPlainText()+"\n"+newMessage;
      Message::toScreen("DiagPanel: "+newMessage);
      //stormSignalWarning->clear();
      //stormSignalWarning->insertPlainText(totalMessage); 
    }
    */
    stormSignalWarning->clear();
    stormSignalWarning->insert(newMessage);
  }
}
