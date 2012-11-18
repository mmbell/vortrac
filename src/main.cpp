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
#include "Batch/BatchWindow.h"

int main(int argc, char *argv[])
{

    // Read the command line argument to get the XML configuration file
    if (argc >=2) {

        // Check to make sure the argument has the right suffix
        QString xmlfile(argv[1]);
        if (xmlfile.right(3) != "xml") {
            std::cout << xmlfile.toStdString() << " does not look like an XML file\n";
            return EXIT_FAILURE;
        }

        // Open the file
        QFile file(xmlfile);
        if (!file.open(QIODevice::ReadOnly)) {
            std::cout << "Error Opening Configuration File, Check Permissions on " << xmlfile.toStdString() << "\n";
            return EXIT_FAILURE;
        }

        // Create a DOM document with contents from the configuration file
        QDomDocument domDoc;
        QString errorStr;
        int errorLine;
        int errorColumn;
        if (!domDoc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
            // Exit on malformed XML
            QString errorReport = QString("XML Parse Error in "+xmlfile+" at Line %1, Column %2:\n%3")
            .arg(errorLine)
            .arg(errorColumn)
            .arg(errorStr);
            std::cout << errorReport.toStdString() << "\n";
            file.close();
            return EXIT_FAILURE;
        }

        // Successful file read
        file.close();

        // Check the root node to make sure this is really a SAMURAI configuration file
        QDomElement root = domDoc.documentElement();
        if (root.tagName() != "vortrac") {
            std::cout << "The XML file " << xmlfile.toStdString() << " is not an VORTRAC configuration file\n.";
            return EXIT_FAILURE;
        }


        std::cout << "Batch Mode started for " << xmlfile.toStdString() << " ...\n";
        QApplication app(argc,argv);
        BatchWindow mainWin(0, domDoc);

        app.exec();



    //If no xml file is parsed, start GUI
    } else if (argc ==1) {

        // Q_INIT_RESOURCE(vortrac);
        QApplication app(argc, argv);
        MainWindow mainWin;
        mainWin.show();
        return app.exec();
    } else {
        std::cout << "Usage: vortrac <vortrac_configuration.xml>\n";
        return EXIT_SUCCESS;
    }
}
