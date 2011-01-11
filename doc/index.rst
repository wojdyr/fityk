
.. title:: fityk --- curve fitting software

.. role:: smallfont
   :class: smallfont

.. image:: fityk076.png
   :alt: [screenshot]
   :align: right
   :scale: 50
   :class: screenshot

Fityk *[fi:tik]* is a program for data processing
and nonlinear **curve fitting**.

It is primarily used...

* by scientists who analyse data from powder diffraction, chromatography,
  photoluminescence and photoelectron spectroscopy,
  infrared and Raman spectroscopy, and other experimental techniques,

..

* to fit bell-shaped functions (Gaussian, Lorentzian, Voigt,
  Pearson VII, bifurcated Gaussian, EMG, Doniach-Sunjic, etc.),

... but any functions can be fitted to any 2D (*x*,\ *y*) data.

Features
========

* intuitive graphical interface (and also command line interface),
* support for many data file formats, thanks to
  the `xylib library <http://xylib.sourceforge.net/>`_,
* dozens of built-in functions and support for user-defined functions,
* equality constraints,
* modelling error of the *x* coordinate of points (that can be caused by
  instrumental zero-shift or by sample displacement in powder diffraction),
* peak detection algorithm,
* various optimization methods (standard Marquardt least-squares algorithm,
  Genetic Algorithms, Nelder-Mead simplex),
* handling series of datasets,
* automation with scripts,
* an add-on for powder diffraction data (Pawley refinement)
* modular :wiki:`architecture <Architecture>`,
* open source licence (`GPL <http://creativecommons.org/licenses/GPL/2.0/>`_),
* portability.

Download
========

|ico-win| MS Windows: :download-msw:`.exe`

|ico-tux| Fresh Linux RPMs from OBS_ and DEBs from PPA_ or debian-xray_

|ico-osx| Mac OS X: work in progress...

.. _OBS: http://download.opensuse.org/repositories/home://wojdyr/
.. _PPA: https://launchpad.net/~wojdyr/+archive/fityk
.. _debian-xray: http://debian-xray.iit.edu/
.. |ico-win| image:: ico-win.png
.. |ico-tux| image:: ico-tux.png
.. |ico-osx| image:: ico-osx.png

Source code: `GitHub <https://github.com/wojdyr/fityk>`_

Version 0.9.4 was released on 2010-10-09
(`changelog <https://github.com/wojdyr/fityk/raw/master/NEWS>`_).
FreshMeat provides new version **notifications**:
`emails <http://freshmeat.net/projects/fityk/>`_ and
`feeds <http://freshmeat.net/projects/fityk/releases.atom>`_.

Subscriptions
-------------

*This is an attempt to make this software self-sustaining and actively
developed in the future.*

After releasing ver. 1.0 new binaries will be available to subscribers only.
Distribution of the source code and the licence will not change.

Subscription cost (includes support via e-mail):

* a `single download <https://www.plimus.com/jsp/buynow.jsp?contractId=2918496>`_: €90 / 115 USD

* `1 year subscription <https://www.plimus.com/jsp/buynow.jsp?contractId=2918202>`_: €210 / 265 USD

* `1 year subscription + 20 hours of coding <https://www.plimus.com/jsp/buynow.jsp?contractId=2918292>`_ €630 / 795 USD.
  :smallfont:`The maintainer of Fityk will devote up to 20 hours to implement
  feature(s) requested by the customer. (Bugs are fixed as soon as possible
  regardless of who reports them). 20 hours can be enough to add
  a simple data file format (2-3 page long spec), or to add a new function
  (like, say, Voigt), or to tweak the GUI.
  If you have a particular feature in mind, please contact us first.`

.. raw:: html

   <form action="https://www.paypal.com/cgi-bin/webscr" method="post" style="display:inline; margin:0; padding: 0;">
   <p>Alternatively, students and home users may
   donate 10% of the normal price to wojdyr@gmail.com using PayPal
   <input type="hidden" name="cmd" value="_donations">
   <input type="hidden" name="business" value="E98FRTPDBQ3L6">
   <input type="hidden" name="lc" value="US">
   <input type="hidden" name="currency_code" value="USD">
   <input type="hidden" name="bn" value="PP-DonationsBF:btn_donate_SM.gif:NonHosted">
   <input type="image" src="https://www.paypal.com/en_US/i/btn/btn_donate_SM.gif" border="0" name="submit" alt="PayPal - The safer, easier way to pay online!">
   <img alt="" border="0" src="https://www.paypal.com/en_US/i/scr/pixel.gif" width="1" height="1">
  or <a href="http://moneybookers.com">MoneyBookers</a>.
  </p>
  </form>

You are welcome to make a purchase now, the subscription time will be counted
since the release of ver. 1.0.


Documentation
=============

`The manual <fityk-manual.html>`_
(`PDF <http://www.unipress.waw.pl/fityk/fityk-manual.pdf>`_)
documents mainly commands of the fityk mini-language.

`Fityk Wiki <https://github.com/wojdyr/fityk/wiki>`_
contains all other informations.
You are welcome to contribute.

Citing fityk in academic papers:
M. Wojdyr,
`J. Appl. Cryst. 43, 1126-1128 <http://dx.doi.org/10.1107/S0021889810030499>`_
(2010)
[`reprint <http://www.unipress.waw.pl/fityk/fityk-JAC-10-reprint.pdf>`_]

Questions?
==========

Join the Google group
`fityk-users <http://groups.google.com/group/fityk-users/>`_.
You may select the option "no mail" and use the web interface to send messages.
Feel free to send questions, comments, bug reports, new feature requests
and success stories.

Asking for a new feature usually results in adding the request to
the `TODO list <https://github.com/wojdyr/fityk/raw/master/NEWS>`_
or, if it already is in the list, in assigning higher priority to it.

If for some reasons you do not want to use the group,
you may contact directly the maintainer of the program: wojdyr@gmail.com.


.. raw:: html

   <script type="text/javascript"> <!--
   if (window != top) top.location.href = location.href;
   $(document).ready(function(){
     $("#features").hide();
     $("#features").prev().after(
      "<p id='expand_features'><a href=''>More &raquo;</a></p>");
     $("#expand_features a").click(function(event){
       $(this).parent().hide();
       $("#features").show('slow');
       event.preventDefault();
     });
   });
   //--> </script>


