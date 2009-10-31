
.. title:: fityk --- free peak fitting software

Overview
========

.. image:: fityk076.png
   :alt: [screenshot]
   :align: right
   :scale: 50

**Fityk** *[fi:tik]* is for **peak fitting**.

Longer version: fityk is a program for nonlinear fitting of analytical
functions (especially peak-shaped) to data (usually experimental data). There
are also people using it to remove the baseline from data, or to display data
only.

It is reportedly used in crystallography, chromatography, photoluminescence and
photoelectron spectroscopy, infrared and Raman spectroscopy, to name but a few.

Fityk knows about common peak-shaped functions (Gaussian, Lorentzian, Voigt,
Pearson VII, bifurcated Gaussian, EMG, Doniach-Sunjic, etc.) and polynomials.
It also supports user-defined functions.

Fityk offers intuitive graphical interface (and also command line interface),
variouse optimization methods (standard Marquardt least-square algorithm,
Genetic Algorithms, Nelder-Mead simplex), equality constraints, modelling error
of x coordinate of points (eg. zero-shift of instrument), handling series of
datasets, automation of common tasks with scripts, and more.

It is a portable (Linux, FreeBSD, MS Windows, MacOS X) and free software
(`GPL <http://www.gnu.org/copyleft/gpl.html>`_).

Both graphical and command line interfaces use *libfityk* library.
The library is written in C++, and comes with Python bindings.

`Xylib library <http://www.unipress.waw.pl/fityk/xylib/>`_
is used for reading data files.


Download
========

* **Source code**: :sf-download-source:`.tar.bz2`
* **MS Windows** Installer: :sf-download-msw:`.exe`
* Fresh **Linux RPMs** `from OBS <http://download.opensuse.org/repositories/home://wojdyr/>`_.
  Good Linux distros and FreeBSD ports also have fityk packaged.

* A bit old **MacOSX**
  `universal binary <http://agni.phys.iit.edu/~kmcivor/fityk/>`_
  (ver. 0.8.2) compiled by Ken McIvor. Read more in the :wiki:`wiki <MacOSX>`.

* `Older versions
  <http://sourceforge.net/project/showfiles.php?group_id=79434>`_
  of the program

* `Daily builds <http://fityk.sourceforge.net/daily/>`_

* The latest code:
  ``svn co https://fityk.svn.sourceforge.net/svnroot/fityk/trunk fityk``

New version **notifications** from FreshMeat:
`emails <http://freshmeat.net/projects/fityk/>`_ or
`feeds <http://freshmeat.net/projects/fityk/releases.atom>`_.

.. raw:: html

   <div align="center" style="font-size:x-small; margin-top:20px;">
   <form action="https://www.paypal.com/cgi-bin/webscr" method="post">
   <input type="hidden" name="cmd" value="_donations">
   <input type="hidden" name="business" value="wojdyr@gmail.com">
   <input type="hidden" name="lc" value="US">
   <input type="hidden" name="item_name" value="Fityk">
   <input type="hidden" name="currency_code" value="USD">
   <input type="hidden" name="bn" value="PP-DonationsBF:btn_donateCC_LG.gif:NonHosted">
   <input type="image" src="https://www.paypal.com/en_US/i/btn/btn_donateCC_LG.gif" border="0" name="submit" alt="PayPal">
   <img alt="" border="0" src="https://www.paypal.com/en_US/i/scr/pixel.gif" width="1" height="1">
   </form>
   <p>
   Alternatively, you may donate via
   <a href="http://moneybookers.com">moneybookers</a> to wojdyr@gmail.com
   </p>
   </div>


History
=======

* 2009-08-20 Version 0.8.9
* 2009-06-21 Version 0.8.8 (no Windows exe this time)
* 2009-06-11 Version 0.8.7
* 2008-04-15 Version 0.8.6

See the full history (and what's new in each release) in the
`NEWS file <http://fityk.svn.sourceforge.net/svnroot/fityk/trunk/NEWS>`_.

Documentation
=============

`The manual <fityk-manual.html>`_
(`PDF <http://www.unipress.waw.pl/fityk/fityk-manual.pdf>`_)
documents mostly commands of the fityk mini-language.

`Trac Wiki <http://sourceforge.net/apps/trac/fityk/>`_
contains all other informations.
You are also welcome to contribute.

Questions or comments?
======================

Join the Google group
`fityk-users <http://groups.google.com/group/fityk-users/>`_.
You may select option "no mail" when joining and use the web interface to read
and send messages.
Feel free to send questions, comments, bug reports, new feature requests
or success stories.

Asking for a new feature usually results in adding the request to
the `TODO list <http://fityk.svn.sourceforge.net/svnroot/fityk/trunk/TODO>`_
or, if it already is on the list, in assigning higher priority to it.

If for some reasons you do not want to use the group,
you may contact directly the maintainer of the program:
`Marcin Wojdyr <http://www.unipress.waw.pl/~wojdyr/>`_  wojdyr@gmail.com.


.. raw:: html

   <p>&nbsp;</p>
   <p>
   Thanks to:
   <a href="http://www.unipress.waw.pl">
   <img src="_static/unipress-button.png" alt="Developed in Unipress" title="Developed in Unipress" />
   </a>
   <a href="http://www.wxwidgets.org">
   <img src="_static/wxwidgets_powered.png" alt="Built with wxWidgets" title="Built with wxWidgets" />
   </a>
   <a href="http://sourceforge.net/projects/fityk">
   <img src="http://sflogo.sourceforge.net/sflogo.php?group_id=79434&type=10" alt="Get Fityk at SourceForge.net" title="Hosted at SourceForge.net" />
   </a>
   </p>

   <script language="JavaScript" type="text/javascript"> <!--
   if (window != top) top.location.href = location.href;
   //--> </script>

..
   <script type="text/javascript"><!--
   google_ad_client = "pub-6047722981051633";
   google_ad_slot = "7961920150";
   google_ad_width = 728;
   google_ad_height = 15;
   //--></script>
   <script type="text/javascript"
    src="http://pagead2.googlesyndication.com/pagead/show_ads.js">
   </script>


