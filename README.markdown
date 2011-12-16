# VORTRAC

## Vortex Objective Radar Tracking And Circulation

The Vortex Objective Radar Tracking and Circulation software is a collection of radar algorithms designed to provide real-time information about hurricane location and structure from a single Doppler radar.
These algorithms are written in C++ and combined with a graphical user interface (GUI) that allows the user to control the software operation and display critical storm parameters for use in an operational environment.

The primary display shows a timeline of estimated central surface pressure and the radius of maximum wind (RMW) that updates as new radar volumes are processed. Additional information about the radar data and program operation is also displayed to the user, including a constant altitude plan-position indicator (CAPPI), maximum velocity, storm signal, status light, operation log, and progress bar. 

This development was funded under grants from the NOAA Joint Hurricane Testbed program from 2005 – 2007, and 2011-2012.

## Compilation and Use

To compile, use [qmake] (http://qt.nokia.com/) from the top-level directory:

     $ qmake

to create a Makefile or Xcode project for your machine. Run `make` or build via Xcode to create the `vortrac` binary.

A User's Guide is included in the `doc` subdirectory.

Several utility scripts for creating a deployable application or viewing the VORTRAC output offline are included in the `util` subdirectory.

## Contributing to VORTRAC

* Check out the latest master to make sure the feature hasn't been implemented or the bug hasn't been fixed yet
* Check out the [issue tracker](http://github.com/mmbell/vortrac/issues) to make sure someone already hasn't requested it and/or contributed it
* Fork the project
* Start a feature/bugfix branch
* Commit and push until you are happy with your contribution, then open a Pull Request.
* Make sure to add tests for the feature/bugfix. This is important so we don't break it in a future version unintentionally.

## Copyright

Copyright (c) 2011 Wen-Chau Lee, Paul Harasti, Michael Bell, Lisa Mauger

See License for details.

### References

A manuscript describing the VORTRAC technique is currently in preparation.

