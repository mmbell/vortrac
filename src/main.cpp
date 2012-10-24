/*
 *  VORTRAC
 *  Vortex Structure Position and Intensity from Doppler Radar
 *
 *  Created by Michael Bell on 6/17/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include <QApplication>
#include <QtCore>
#include <QtXml>
#include <iostream>

#include "GUI/MainWindow.h"
#include "DriverBatch.h"

using namespace std;

int main(int argc, char *argv[])
{

    // Read the command line argument to get the XML configuration file
    if (argc >=2) {
        DriverBatch *driver = NULL;

        cout << "Batch Mode started" << endl;

    //If no xml file is parsed, start GUI
    } else {
        // Q_INIT_RESOURCE(vortrac);

        QApplication app(argc, argv);
        MainWindow mainWin;
        mainWin.show();
        return app.exec();
    }
}
