
.. title:: Fityk --- curve fitting and peak fitting software

.. meta::
   :description: Fityk. Open-source curve-fitting and data analysis software. Linux, Windows, Mac OS X.
   :keywords: curve fitting, peak fitting, software, Voigt, Doniach-Sunjic

.. role:: smallfont
   :class: smallfont

.. role:: html(raw)
   :format: html

.. image:: fityk-banner.png
   :align: center
   :target: http://fityk.nieto.pl
   :class: banner

.. raw:: html

  <div class="clearer" style="height:10px"></div>
  <div align="right" class="screenshot">
   <p class="quote">
    <i>Excellent GUI and command-line curve fitting tool</i><br />
    <span class="quote-author">
     - John Allspaw in
     <a href="http://oreilly.com/catalog/9780596518585"><i>The art of capacity planning</i></a>
    </span>
   </p>
   <a href="screens.html#mac-os-x">
    <img alt="[screenshot]" src="_images/fityk-1.0.1-osx-so.png"
    style="width: 436px; height: 303px;" />
    </a>
   <p class="caption">see <a href="screens.html">more screenshots</a></p>
  </div>

Fityk *[fi:tik]* is a program for data processing
and nonlinear **curve fitting**.

Primarily used

* by scientists_
  who analyse data from powder diffraction, chromatography,
  photoluminescence and photoelectron spectroscopy,
  infrared and Raman spectroscopy, and other experimental techniques,

..

* to fit peaks -- bell-shaped functions (Gaussian, Lorentzian, Voigt,
  Pearson VII, bifurcated Gaussian, EMG, Doniach-Sunjic, etc.),

but it is suitable for fitting any curve to 2D (*x*,\ *y*) data.

.. _scientists: https://scholar.google.com/scholar?cites=1686729773533771289

.. _contents:

:ref:`Features` |
:ref:`Download` |
:ref:`Documentation` |
:ref:`Support <Support>`

.. _Features:

Features
========

* intuitive graphical interface (and also command line interface),
* support for many data file formats, thanks to
  the `xylib library <http://xylib.sourceforge.net/>`_,
* dozens of built-in functions and support for user-defined functions,
* equality constraints,
* fitting systematic errors of the *x* coordinate of points (for example
  instrumental zero error or sample displacement correction
  in powder diffraction),
* manual, graphical placement of peaks and auto-placement using peak detection
  algorithm,
* various optimization methods (standard Marquardt least-squares algorithm,
  Genetic Algorithms, Nelder-Mead simplex),
* handling series of datasets,
* automation with macros (scripts) and embedded Lua for more complex scripting
* the accuracy of nonlinear regression :wiki:`verified <NIST-certified-data>`
  with reference datasets from NIST,
* an add-on for powder diffraction data (Pawley refinement)
* modular :wiki:`architecture <Architecture>`,
* open source licence (GPLv2+).

.. _Download:

Download
========

|ico-win| MS Windows: :github_download:`-setup.exe`

|ico-osx| Mac OS X (10.6 or later): :github_download:`-osx.zip`

|ico-tux| Linux: binary
`RPM and deb packages <https://software.opensuse.org/download?project=home:wojdyr&package=fityk>`_
(`files <https://download.opensuse.org/repositories/home://wojdyr/>`_)
:smallfont:`for about 10 distros (incl. Ubuntu, Fedora, Suse), 32- and 64-bit.`

Source code: `GitHub <https://github.com/wojdyr/fityk>`_
(`releases <https://github.com/wojdyr/fityk/releases>`_)

Installers for ver. 1.0+ used to be available to paid subscribers only.
This made `version 0.9.8 <https://github.com/wojdyr/fityk/downloads>`_
more popular than more recent releases.
To change this situation the latest binaries are no longer
`paywalled </subscribers>`_.
The author is grateful to all people who supported Fityk with subscriptions.

Version 1.3.1 was released on 2016-12-19
(`changelog <https://github.com/wojdyr/fityk/raw/master/NEWS>`_).
New version `notifications <http://fityk-announce.nieto.pl/>`_
are delivered via email and feeds.

.. |ico-win| image:: img/ico-win.png
.. |ico-tux| image:: img/ico-tux.png
.. |ico-osx| image:: img/ico-osx.png

.. _Documentation:

Documentation
=============

* `Manual <fityk-manual.html>`_
  (chapters :ref:`intro`, :ref:`getstarted`, :ref:`data`,
  :ref:`model`, :ref:`fit`, :ref:`scripts`, :ref:`ref`).
* PDF, ePUB and older versions of the manual can be downloaded
  `from Read the Docs <https://readthedocs.org/projects/fityk/downloads/>`_.

* `Fityk Wiki <https://github.com/wojdyr/fityk/wiki>`_
  (you are welcome to contribute).

Citing Fityk in academic papers:
M. Wojdyr,
`J. Appl. Cryst. 43, 1126-1128 <http://dx.doi.org/10.1107/S0021889810030499>`_
(2010)
[`reprint <http://wojdyr.github.io/fityk-JAC-10-reprint.pdf>`_]

.. _Support:

Questions?
==========

* Google group `fityk-users <http://groups.google.com/group/fityk-users/>`_
  (you may select "no mail" and use it like forum)

* or wojdyr@gmail.com

Feel free to send questions, comments, requests, bug reports,
and success stories.
Asking for a new feature usually results in adding the request to
the `TODO list <https://github.com/wojdyr/fityk/raw/master/TODO>`_
or, if it already is in the list, in assigning higher priority to it.

.. raw:: html

   <script type="text/javascript"> <!--
   if (window != top) top.location.href = location.href;
   $(document).ready(function(){
     $("#features").hide();
     $("#features").prev().after(
      "<p id='expand_features'><a href=''><span class='h1'>Features</span> &nbsp; <span>[show]</span></a></p>");
     $("#expand_features a").click(function(event){
       $(this).parent().hide();
       $("#features").show('slow');
       event.preventDefault();
     });
   });
   //--> </script>

