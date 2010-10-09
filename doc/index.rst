
.. title:: fityk --- curve fitting software

Overview
========

.. image:: fityk076.png
   :alt: [screenshot]
   :align: right
   :scale: 50
   :class: screenshot

Fityk *[fi:tik]* is a program for nonlinear least squares **curve fitting**.
It is also useful for data processing and visualization.

Fityk is primarily used...

* by scientists who analyse data from powder diffraction, chromatography,
  photoluminescence and photoelectron spectroscopy,
  infrared and Raman spectroscopy, and other experimental techniques,

..

* to fit bell-shaped functions (Gaussian, Lorentzian, Voigt,
  Pearson VII, bifurcated Gaussian, EMG, Doniach-Sunjic, etc.),

... but any functions can be fitted to any 2D (x-y) data.

Features
========

* intuitive graphical interface (and also command line interface),
* support for many data file formats, thanks to
  `xylib library <http://www.unipress.waw.pl/fityk/xylib/>`_,
* dozens of built-in functions and support for user-defined functions,
* equality constraints,
* modelling errors of the *x* coordinate of points (that can be caused by
  instrumental zero-shift or by sample displacement in powder diffraction),
* peak detection algorithm,
* various optimization methods (standard Marquardt least-squares algorithm,
  Genetic Algorithms, Nelder-Mead simplex),
* handling series of datasets,
* automation with scripts,
* an add-on for powder diffraction data (Pawley refinement)
* modular :wiki:`architecture <architecture>`,
* open source licence (GPL),
* portability.

Download
========

* Source code: :sf-download-source:`.tar.bz2`
* MS **Windows** Installer: :sf-download-msw:`.exe`
* Fresh **Linux** RPMs from OBS_, DEBs from PPA_ or debian-xray_

* Mac **OS X**: see details :wiki:`here <MacOSX>`.

* `Daily builds <http://fityk.sourceforge.net/daily/>`_

* The latest code:
  ``svn co https://fityk.svn.sourceforge.net/svnroot/fityk/trunk fityk``

.. * `Older versions
  <http://sourceforge.net/project/showfiles.php?group_id=79434>`_
  of the program


.. _OBS: http://download.opensuse.org/repositories/home://wojdyr/
.. _PPA: https://launchpad.net/~wojdyr/+archive/fityk
.. _debian-xray: http://debian-xray.iit.edu/

Version 0.9.4 was released on 2010-10-09
(`changelog <http://fityk.svn.sourceforge.net/svnroot/fityk/trunk/NEWS>`_).

FreshMeat provides new version **notifications**:
`emails <http://freshmeat.net/projects/fityk/>`_ and
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


Documentation
=============

`The manual <fityk-manual.html>`_
(`PDF <http://www.unipress.waw.pl/fityk/fityk-manual.pdf>`_)
documents mainly commands of the fityk mini-language.

`Trac Wiki <http://sourceforge.net/apps/trac/fityk/>`_
contains all other informations.
You are also welcome to contribute.

Citing fityk in academic papers:
M. Wojdyr,
`J. Appl. Cryst. 43, 1126-1128 <http://dx.doi.org/10.1107/S0021889810030499>`_
(2010)
[`reprint <http://www.unipress.waw.pl/fityk/fityk-JAC-10-reprint.pdf>`_]

Questions or comments?
======================

Join the Google group
`fityk-users <http://groups.google.com/group/fityk-users/>`_.
You may select option "no mail" and use the web interface to send messages.
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

   <script type="text/javascript"> <!--
   if (window != top) top.location.href = location.href;
   $(document).ready(function(){
     $("#features").hide();
     $("#overview").append(
      "<p id='expand_features'><a href=''>More &raquo;</a></p>");
     $("#expand_features a").click(function(event){
       $(this).parent().hide();
       $("#features").show('slow');
       event.preventDefault();
     });
   });
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


