#!/bin/sh

macdeployqt vortrac.app
./deploy/setfileicon vortrac.icns vortrac.app
cp *.xml vortrac.app/Contents/Resources/
rm -rf deploy/mac/vortrac.app/
cp -R vortrac.app deploy/mac/
cp VORTRAC_Users_Guide_v1.pdf deploy/mac/ 
