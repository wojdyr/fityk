
.. _getstarted:

Getting started
###############

The minimal example
===================

Let us analyze a diffraction pattern of NaCl. Our goal is to determine
the position of the center of the highest peak. It is needed for
calculating the pressure under which the sample was measured, but this
later detail in the processing is irrelevent for the time being.

The data file used in this example is distributed with the program and
can be found in the :file:`samples` directory.

First load data from file :file:`nacl01.dat`.
You can do this by typing::

   @0 < nacl01.dat 

in the CLI version (or in the GUI version in the input box - at the bottom,
just above the status bar).
In the GUI, you can also select :menuselection:`Data --> Load File`
from the menu and choose the file.

If you use the GUI, you can zoom-in to the biggest peak using left mouse
button on the auxiliary plot (the plot below the main plot).
To zoom out, press the :guilabel:`View whole` toolbar button.
Other ways of zooming are described in :ref:`mouse`.
If you want the data to be drawn with larger points or a line,
or if you want to change the color of the line or background,
press right mouse button on the main plot and use
:guilabel:`Data point size` or :guilabel:`Color` menu from the pop-up
menu.  To change the color of data points, use the right-hand panel.

Now all data points are active. Because only the biggest peak is of
interest for the sake of this example, the remaining points can be
deactivated.
Type::

   A = (23.0 < x < 26.0)
   
or change to :dfn:`range` mode
(press :guilabel:`Data-Range Mode` button on toolbar)
and select range to be deactivated with right mouse button.

As our example data has no background to worry about, our next step is
to define a peak with reasonable initial values and fit it to the data.
We will use Gaussian.
To see its formula, type: ``info Gaussian`` or look for it
in the documentation (in :ref:`flist`).
Incidentally, most of the commands can be abbreviated, e.g. you can type:
``i Gaussian``.

To define peak, type::

   %p = Gaussian(~60000, ~24.6, ~0.2); F = %p

or::

   %p = guess Gaussian

or select :guilabel:`Gaussian` from the list of functions on the toolbar
and press the :guilabel:`auto-add` toolbar button.
There are also other ways to add peak in GUI such as :dfn:`add-peak` mode.
These mouse-driven ways give function a name like ``%_1``, ``%_2``, etc.

Now let us fit the function. Type: ``fit`` or select
:menuselection:`Fit --> Run` from the menu (or press the toolbar button).

When fitting, the weighted sum of squared residuals (see :ref:`nonlinear`)
is being minimized.

.. note:: The default :ref:`weights of points <weights>` are not equal.

To see the peak parameters, type: ``info+ %p``
or (in the GUI) move the cursor to the top of the peak
and try out the context menu (right button), or use the right-hand panel.

That's it! To do the same a second time (for example to a similar data
set) you can write all the commands to a file::

   commands > myscript.fit

and later use it as script::

   commands < myscript.fit

Alternatively, use
:menuselection:`Session --> Logging --> History dump`
and
:menuselection:`Session --> Execute script`.

If you start fityk from command line, you can load data and/or execute
scripts by giving filenames as arguments, e.g. ::

   bash$ fityk myscript.fit

.. _invoking:

Invoking fityk
==============

On startup, the program executes a script from the
:file:`$HOME/.fityk/init` file (on MS Windows XP:
:file:`C:\\Documents and Settings\\USERNAME\\.fityk\\init`).
Following this, the program executes command passed with ``--cmd``
option, if given, and processes command line arguments:

- if the argument starts with ``=->``, the string following ``=->``
  is regarded as a command and executed
  (otherwise, it is regarded as a filename).

- if the filename has extension ".fit" or the file begins with a "# Fityk"
  string, it is assumed to be a script and is executed.

- otherwise, it is assumed to be a data file.
  It is possible to specify columns in data file in this way:
  ``file.xy:1:4::``.
  Multiple y columns can be specified
  (``file.xy:1:3,4,5::`` or ``file.xy:1:3..5::``) -- it will load
  each y column as a separate dataset, with the same values of x.

There are also other parameters to the CLI and GUI versions of the program.
Option "-h" (on MS Windows "/h") gives the full listing::

    wojdyr@ubu:~/fityk/src$ ./fityk -h
    Usage: fityk \[-h] \[-V] \[-c <str>] \[-I] \[-r] \[script or data file...]
    -h, --help            show this help message
    -V, --version         output version information and exit
    -c, --cmd=<str>       script passed in as string
    -g, --config=<str>    choose GUI configuration
    -I, --no-init         don't process $HOME/.fityk/init file
    -r, --reorder         reorder data (50.xy before 100.xy)

The example of non-interactive using CLI version on Linux::

    wojdyr@ubu:~/foo$ cfityk -h
    Usage: cfityk \[-h] \[-V] \[-c <str>] \[script or data file...]
    -h, --help            show this help message
    -V, --version         output version information and exit
    -c, --cmd=<str>       script passed in as string
    -I, --no-init         don't process $HOME/.fityk/init file
    -q, --quit            don't enter interactive shell
    wojdyr@ubu:~/foo$ ls \*.rdf
    dat_a.rdf  dat_r.rdf  out.rdf
    wojdyr@ubu:~/foo$ cfityk -q -I "=-> set verbosity=quiet, autoplot=never" \\
    > \*.rdf "=-> i+ min(x if y > 0) in @*"
    in @0 dat_a: 1.8875
    in @1 dat_r: 1.5105
    in @2 out: 1.8305

Graphical interface
===================

Plots and other windows
-----------------------

The GUI window of fityk consists of (from the top): menu bar, toolbar,
main plot, auxiliary plot, output window, input field, status bar and of
sidebar at right-hand side. The input field allows you to type and
execute commands in a similar way as is done in the CLI version. The
output window (which is configurable through a pop-up menu) shows the
results. Incidentally, all GUI commands are converted into text and are
visible in the output window, providing a simple way to learn the
syntax.

The main plot can display data points, model that is to be fitted to the
data and component functions of the model. Use the pop-up menu (click
right button on the plot) to configure it. Some properties of the plot
(e.g. colors of data points) can be changed using the sidebar.

One of the most useful things which can be displayed by the auxiliary
plot is the difference between the data and the model (also controlled
by a pop-up menu). Hopefully, a quick look at this menu and a minute or
two's worth of experiments will show the potential of this auxiliary
plot.

Configuration of the GUI (visible windows, colors, etc.) can be saved
using :menuselection:`GUI --> Save current config`.
Two different configurations can be saved, which allows easy changing of
colors for printing. On Unix platforms, these configurations are stored
in a file in the user's home directory. On Windows - they are stored in
the registry (perhaps in the future they will also be stored in a file).

.. _mouse:

Mouse usage
-----------

The usage of the mouse on menu, dialog windows, input field and output
window is (hopefully) intuitive, so the only remaining topic to be
discussed here is how to effectively use the mouse on plots.

Let us start with the auxiliary plot.  The right button displays a
pop-up menu with a range of options, while the left allows you to select
the range to be displayed on the x-axis.  Clicking with the middle
button (or with left button and :kbd:`Shift` pressed simultaneously)
will zoom out to display all data.

On the main plot, the meaning of the left and right mouse button depends
on current :dfn:`mode` (selected using either the toolbar or menu).
There are hints on the status bar.  In normal mode, the left button is
used for zooming and the right invokes the pop-up menu.  The same
behaviour can be obtained in any mode by pressing :kbd:`Ctrl` (or
:kbd:`Alt`.).

The middle button can be used to select a rectangle that you want to
zoom in to.  If an operation has two steps, such as rectangle zooming
(i.e. first you press a button to select the first corner, then move the
mouse and release the button to select the second corner of the
rectangle), this can be cancelled by pressing another button when the
first one is pressed.

..
  $Id: $ 

