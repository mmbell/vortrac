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

#include <sys/resource.h>
#include <unistd.h>

#include "GUI/MainWindow.h"
#include "Batch/BatchWindow.h"

void usage(const char *s) {
  std::cout << "Usage: " << std::endl
	    << "  " << s << " [no arguments] (GUI mode, set config file from GUI)"
	    << std::endl
	    << "  " << s << " [-c] <config file>.xml (BATCH mode)"
	    << std::endl
    	    << "  " << s << " [-c <config file>.xml input_files ...] (just run on these files)"
	    << std::endl
	    << "  " << s << " [-g] <config file>.xml (GUI mode with config file on command line)"
    	    << std::endl
	    << std::endl
	    << "Optional arguments:"
    	    << std::endl
	    << "  -d: Turn on debug flag"
	    << std::endl
	    << "  -h: Display help screen and exit"
    	    << std::endl;
}

int main(int argc, char *argv[])
{
    // Increase the size of the stack to account for some of the large 3D array local variables
    // (unless the user has already done it from the command line
    // limit.rlim_max / 2 is arbitrary. But that works both on iMac and Linux
  
    struct rlimit limit;
    getrlimit(RLIMIT_STACK, &limit);
    if (limit.rlim_cur < limit.rlim_max / 2) {
      limit.rlim_cur = limit.rlim_max / 2;
      if ( setrlimit(RLIMIT_STACK, &limit) )
	std::cout << "setrlimit failed. Set 'ulimit -s 48000' "
		  << "in the shell if Vortrac crashes after reading the config file"
		  << std::endl;
    }

    // Handle options
    
    int opt;
    char *conf_file_path = NULL;
    bool debug = false;
    bool gui_mode = true;
    
    while( (opt = getopt(argc, argv, "c:g:hd")) != -1)
    switch(opt){
    case 'd':
      debug = true;
      break;
    case 'c':
      conf_file_path = strdup(optarg);
      gui_mode = false;
      break;
    case 'g':
      conf_file_path = strdup(optarg);
      gui_mode = true;
      break;
    case 'h':
    case '?':
      usage(argv[0]);
      exit(0);
    }

    if (debug) {
      std::cerr << "==>> conf_file_path: " << conf_file_path << std::endl;
    }

    // A bit more complex than I'd like, but this preserves the historical usage
    // vortrac             <- GUI mode, set config file via GUI
    // vortrac file.xml    <- Batch mode
    // vortrac -c file.xml <- Batch mode
    // vortrac -c file.xml file1 [file2, ....]
    // vortrac -g file.xml <- GUI mode with config file
    
    if (optind == argc) { // All options consumed
      if (gui_mode) {
	std::cout << "GUI mode" << std::endl;
      } else {
	std::cout << "Batch mode with config " << conf_file_path << std::endl;
      }
    } else {
      if (conf_file_path == NULL) {
	conf_file_path = strdup(argv[optind++]);
	if (optind == argc)
	  std::cout << "Batch mode with config " << conf_file_path << std::endl;
	else {
	  std::cerr << "Unexpected arguments. Did you forget the '-c' ?" << std::endl;
	  usage(argv[0]);
	}
      } else {
	std::cout << "Command mode. Running on these files:" << std::endl;
	for(int index = optind; index < argc; index++)
	  std::cout << "\t" << argv[index];
      }
    }
    
    // Read the command line argument to get the XML configuration file

    if (conf_file_path != NULL) {

        // Check to make sure the argument has the right suffix
        QString xmlfile(conf_file_path);
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

        // Check the root node to make sure this is really a VORTRAC configuration file
        QDomElement root = domDoc.documentElement();
        if (root.tagName() != "vortrac") {
            std::cout << "The XML file " << xmlfile.toStdString() << " is not an VORTRAC configuration file\n.";
            return EXIT_FAILURE;
        }

         //Check to see if folders for output exist. If not create them

         QFileInfo fileInfo = QFileInfo(file);
         QString filePath = fileInfo.absolutePath();
         std::cout << filePath.toStdString() << "\n";
         QList<QString> dirnames;
         dirnames << "cappi" << "pressure" << "center" << "choosecenter" << "vtd";
         for (int i = 0; i < dirnames.size(); ++i) {
             if (!QDir(filePath + "/" + dirnames.at(i)).exists()) {
                 QDir().mkdir(filePath + "/" + dirnames.at(i));
             }
         }

         if (gui_mode) {

           QApplication app(argc, argv);
           MainWindow mainWin;
           mainWin.open(conf_file_path);
           mainWin.show();
           return app.exec();

         } else {

           // batch mode
           
           std::cout << "Batch Mode started for " << xmlfile.toStdString() << " ...\n";
           QApplication app(argc,argv);
           BatchWindow mainWin(0, xmlfile);
           // TODO Is the batch window meant to be invisible?
           app.exec();

        }

    } else if (argc == 1) {

        // If no xml file is parsed, start GUI
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
