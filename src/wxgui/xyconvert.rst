
=========
xyConvert
=========

xyConvert is a simple utility that converts files in formats supported
by xylib_ into two- or three-column ASCII text file.

Output file can be read by (almost) any data analysis software as x,y points.
The optional third column is for errors (standard deviations) of y.

Licence: `GPL v2+ <http://www.opensource.org/licenses/gpl-2.0.php>`_

.. _xylib: http://www.unipress.waw.pl/fityk/xylib/

Installation
============

Linux, FreeBSD, etc.
--------------------

xyconvert is distributed with fityk_, but it requires wxWidgets (wxGTK) >= 2.9,
so it's not included in any distros yet.
To compile it, you need to install wxGTK, boost and xylib, download fityk
source and do ``./configure && cd src/wxgui/ && make xyconvert``.

.. _fityk: http://www.unipress.waw.pl/fityk/

Windows
-------

You can `download EXE file <xyconvert-0.6.zip>`_ and run it (no installation).

Mac OS X
--------

not tested, but should work (after compiling with wxMac)

Screenshots
===========

Linux:

.. image:: img/xyconvert-gtk.png
    :align: center

Windows:

.. image:: img/xyconvert-win.png
    :align: center

UI Design
=========

User interface is designed primarily for converting many files.
That is why only a directory and a new extension for the output file(s) can
be given.

It is also designed with reusability in mind - the same code (which is very
small, less than 1KLOC) is also used as ``Open File`` dialog in fityk_.


Contact
=======
Marcin Wojdyr wojdyr@gmail.com

Don't hesistate to drop me a line if you have any problems with this program.

