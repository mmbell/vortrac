# VORTRAC

## Vortex Objective Radar Tracking And Circulation

The Vortex Objective Radar Tracking and Circulation software is a collection of radar algorithms designed to provide real-time information about hurricane location and structure from a single Doppler radar.
These algorithms are written in C++ and combined with a graphical user interface (GUI) that allows the user to control the software operation and display critical storm parameters for use in an operational environment.

The primary display shows a timeline of estimated central surface pressure and the radius of maximum wind (RMW) that updates as new radar volumes are processed. Additional information about the radar data and program operation is also displayed to the user, including a constant altitude plan-position indicator (CAPPI), maximum velocity, storm signal, status light, operation log, and progress bar.

This development was funded under grants from the NOAA Joint Hurricane Testbed program from 2005 – 2007, and 2011-2012.

## Compilation and Use

To compile, use [qmake](http://qt.nokia.com/) from the top-level directory:

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

## Copyright, License, and Patent

The VORTRAC code is governed under two copyrights and licenses, and contains a patented algorithm.

The majority of the code is licensed under the Apache License Version 2.0, with the exception of the code in the NRL subdirectory.
The code in the NRL subdirectory has contributions from Paul Harasti at the Naval Research Laboratory and is governed by a separate license.
The Generalized Velocity Track Display (GVTD) algorithm was patented in 2010 as “Method for generating a representation of an atmospheric vortex kinematic structure” – [US Patent No. 7728760 B2](http://patft.uspto.gov/netacgi/nph-Parser?Sect1=PTO1&Sect2=HITOFF&d=PALL&p=1&u=%2Fnetahtml%2FPTO%2Fsrchnum.htm&r=1&f=G&l=50&s1=7,728,760.PN.&OS=PN/7,728,760&RS=PN/7,728,760)

### Main code copyright and license

Copyright 2005 - 2018, Colorado State University and University Corporation for Atmospheric Research

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

## References

A manuscript describing the VORTRAC technique is currently in preparation.
Some of the algorithms used in VORTRAC are referenced below.

Harasti, P.R., 2014: An Expanded VVP Technique to Resolve Primary and Environmental Circulations in Hurricanes. J. Atmos. Oceanic Technol., 31, 249–271.

Jou, B. J.-D., W.-C. Lee, S.-P. Liu, and Y.-C. Kao, 2008: Generalized VTD retrieval of atmospheric vortex kinematic structure. Part I: Formulation and error analysis. Mon. Wea. Rev., 136 (3), 995–1012, doi: 10.1175/2007MWR2116.1.

Lee, W.-C., B. J.-D. Jou, P.-L. Chang, and S.-M. Deng, 1999: Tropical cyclone kinematic structure retrieved from single-Doppler radar observations. Part I: Interpretation of Doppler velocity patterns and the GBVTD technique. Mon. Wea. Rev., 127 (10), 2419–2439, doi: 10.1175/1520- 0493(1999)127<2419:TCKSRF>2.0.CO;2.

Lee, W.-C. and F. D. Marks, 2000: Tropical cyclone kinematic structure retrieved from single-Doppler radar observations. Part II: The GBVTD-Simplex center finding algorithm. Mon. Wea. Rev., 128 (6), 1925–1936, doi: 10.1175/1520-0493(2000)128<1925:TCKSRF>2.0.CO;2.
