
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

Fityk is free software; you can redistribute and modify it under the
terms of the GPL, version 2 or (at your option) any later version.  See
:ref:`license` for details.  You can download the latest version of
fityk from http://www.unipress.waw.pl/fityk (or http://fityk.sf.net).
To contact the author, visit the same page.

How to read this manual
=======================

After this introduction, you may read the :ref:`getstarted`,
look for tutorials in :wiki:`wiki <>`
and postpone reading the manual until you need to write a script, put
constraints on variables, add user-defined function or understand better
how the program works.

In case you are not familiar with the term
:dfn:`weighted sum of squared residuals`
or you are not sure how it is weighted, have a look at :ref:`nonlinear`.
Remember that you must set correctly :ref:`standard deviations <weights>`
of y's of points, otherwise you will get wrong results.

GUI vs CLI
==========

The program comes in two versions: the GUI (Graphical User Interface)
version - more comfortable for most users, and the CLI (Command Line
Interface) version (named *cfityk* to differentiate, Unix only).

If the CLI version was compiled with the *GNU Readline Library*, command
line editing and command history as per *bash* will be available.
Especially useful is ``TAB``-expanding.  Data and curves fitted to data
are visualized with *gnuplot* (if it is installed).

The GUI version is written using the
`wxWidgets <http://www.wxwidgets.org>`_
library  and can be run on Unix species with GTK+ and on MS Windows.
There are also people using it on MacOS X (have a look at the
fityk-users mailing list archives for details).

..
  $Id$ 

