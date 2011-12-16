#!/bin/sh

cp vortrac deploy/linux/vortrac/bin/
cp /h/mrhome1/mmbell/lib/libQtXml.so.4 deploy/linux/vortrac/lib/
cp /h/mrhome1/mmbell/lib/libQtGui.so.4 deploy/linux/vortrac/lib/
cp /h/mrhome1/mmbell/lib/libQtCore.so.4 deploy/linux/vortrac/lib/
cp /h/mrhome1/mmbell/lib/libQtNetwork.so.4 deploy/linux/vortrac/lib/
cp *.xml deploy/linux/vortrac/Resources/
cp VORTRAC_Users_Guide_v1.pdf deploy/linux/vortrac/doc/ 
