.. _intro:

Introduction
############

Fityk is a program for nonlinear fitting of analytical functions
(especially peak-shaped) to data (usually experimental data).

To put it differently, it is primarily peak fitting software,
but can handle other types of functions as well.

Apart from the actual fitting, the program helps with data processing
and provides ergonomic graphical interface (and also command line interface
and scripting API -- but if the program is popular in some fields,
it's thanks to its graphical interface).

It is reportedly__ used in crystallography, chromatography,
photoluminescence and photoelectron spectroscopy, infrared and Raman
spectroscopy, to name but a few.

__ https://scholar.google.com/citations?view_op=view_citation&citation_for_view=aCtDUBMAAAAJ:u5HHmVD_uO8C

Fityk offers various nonlinear fitting methods, simple background
subtraction and other manipulations to the dataset, easy placement of
peaks and changing of peak parameters, support for analysis of series of
datasets, automation of common tasks with scripts, and much more.

In simple cases, the program can be operated with mouse only.
Let say that you want to model the data with multiple peaks or other
function shapes. You select a built-in function type
(such as Gaussian, Voigt, sigmoid, polynomial and dozens of others)
place it with the mouse, place other functions
and click a button to fit it.

But the program can also handle quite complex scenarios.
You can define your own function types.
You can specify sophisticated dependencies between parameters of the functions
(say, peak widths given as a function of peak positions).
You can fit multiple datasets together using common set of parameters.
You can model zero-shift in your instrument or do more complicated
refinement of the X scale.  And you can automate all this work.
If you don't know how to handle your case, do not hesistate
to ask on the `users group`__ or contact the author.

__ http://groups.google.com/group/fityk-users/

To download the latest version of the program or to contact the author
visit `fityk.nieto.pl <http://fityk.nieto.pl/>`_.

Reference for academic papers:
M. Wojdyr,
`J. Appl. Cryst. 43, 1126-1128 <http://dx.doi.org/10.1107/S0021889810030499>`_
(2010)
[`reprint <http://wojdyr.github.io/fityk-JAC-10-reprint.pdf>`_]

Open Source
===========

Fityk is open-source (`GPL2+ <http://creativecommons.org/licenses/GPL/2.0/>`_).
If you are interested, please find `the source code at GitHub`__.

__ https://github.com/wojdyr/fityk/

It uses a few open source projects:

* NLopt_ for several optional fitting methods

* one of the fitting methods uses MPFIT_ library (MINPACK-1 Least Squares
  Fitting Library in C), which includes software developed by
  the University of Chicago, as Operator of Argonne National Laboratory.

* xylib_ library handles reading data files

* Lua_ interpreter is embedded for scripting

* and a few popular libraries and tools that make programming much easier:
  wxWidgets (GUI), Boost.Math (special functions), zlib (compression),
  readline (CLI), SWIG (bindings), Catch (testing).

.. _NLopt: http://ab-initio.mit.edu/wiki/index.php/NLopt
.. _MPFIT: http://www.physics.wisc.edu/~craigm/idl/cmpfit.html
.. _Lua: http://www.lua.org/
.. _xylib: http://xylib.sourceforge.net/

About this manual
=================

This manual is written in ReStructuredText.
All corrections and improvements are welcome.
Go to `GitHub <https://github.com/wojdyr/fityk/tree/master/doc>`_,
open corresponding rst file,
press *Fork and edit this file* button, do edits in your web browser
and click *Propose file change*.

The following people have contributed to this manual (in chronological order):
Marcin Wojdyr (maintainer), Stan Gierlotka, Jaap Folmer, Michael Richardson.

