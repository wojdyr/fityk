
Introduction
############

What is the program for?
========================

Fityk is a program for nonlinear fitting of analytical functions
(especially peak-shaped) to data (usually experimental data). The most
concise description: peak fitting software. There are also people using
it to remove the baseline from data, or to display data only.

It is reportedly used in crystallography, chromatography,
photoluminescence and photoelectron spectroscopy, infrared and Raman
spectroscopy, to name but a few. Although the author has a general
understanding only of experimental methods other than powder
diffraction, he would like to make it useful to as many people as
possible.

Fityk offers various nonlinear fitting methods, simple background
subtraction and other manipulations to the dataset, easy placement of
peaks and changing of peak parameters, support for analysis of series of
datasets, automation of common tasks with scripts, and much more.  The
main advantage of the program is flexibility - parameters of peaks can
be arbitrarily bound to each other, e.g. the width of a peak can be an
independent variable, the same as the width of another peak, or can be
given by complex (and general for all peaks) formula.

GUI vs CLI
==========

The program comes in two versions: the GUI (Graphical User Interface)
version - more comfortable for most users, and the CLI (Command Line
Interface) version (named *cfityk* to differentiate).

.. admonition:: technical note

  The GUI version is written using the
  `wxWidgets <http://www.wxwidgets.org>`_
  library. It runs on Unix species with GTK+, on MS Windows and (with
  some problems) on MacOS X.
  
  If the CLI version was compiled with the *GNU Readline Library*, command
  line editing, ``TAB``-expanding and command history will be available.
  On Unix, the *gnuplot* program can be used for data visualization.

About...
========

Fityk is free software; you can redistribute and modify it under the
terms of the `GPL <http://creativecommons.org/licenses/GPL/2.0/>`_,
version 2 or (at your option) any later version.

To download the latest version of the program or to contact the author
visit http://fityk.nieto.pl/.

This manual is written using ReStructuredText.
All corrections and improvements are welcome.
Use the ``Show Source`` link to get the source of the page, edit it
and send me either the modified version or a patch.

The following people have contributed to this manual (in chronological order):
Marcin Wojdyr (maintainer), Stan Gierlotka, Jaap Folmer, Michael Richardson.

