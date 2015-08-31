
.. _getstarted:

Getting Started
###############

Graphical Interface
===================

That's how the :abbr:`GUI (Graphical User Interface)` looks like:

.. image:: img/fityk-with-tooltip.png
   :align: center
   :scale: 50

The input field with the output window provide a console-like interface
to the program. All important operation performed using the GUI
are translated to textual commands and displayed in the output windows.

The main plot can display data points, model that is to be fitted to the
data and component functions of the model. Use the pop-up menu (click
right button on the plot) to configure it. Some properties of the plot
(e.g. colors of data points) can be changed from the sidebar.

The helper plot below can show various statistics.
The default one one is the difference between the data and the model.
The plot can be configured from a pop-up menu.
If you do not find it useful you may hide it (:menuselection:`GUI --> Show`).

Configuration of the GUI (visible windows, colors, etc.) can be saved
using :menuselection:`GUI --> Save current config`.

On the **main plot**, the meaning of the left and right mouse button depends
on current :dfn:`mode`.
There are hints on the status bar. In normal mode, the left button is
used for zooming and the right invokes the pop-up menu.

On the **helper plot**, selecting a horizontal range with the left button
will show this range, automatically adjusting vertical range.
The middle button shows the whole dataset (like |zoom-all-icon| in the toolbar).
The right button displays a menu.

.. |zoom-all-icon| image:: img/zoom_all.png
   :alt: Zoom All
   :class: icon


Minimal Example
===============

Let us analyze a diffraction pattern of NaCl. Our goal is to determine
the position of the center of the highest peak. It is needed for
calculating the pressure under which the sample was measured, but this
later detail in the processing is irrelevent for the time being.

The data file used in this example is distributed with the program and
can be found in the :file:`samples` directory.

.. role:: cli-title

Textual commands that correspond to performed operations are shown
in this section in :cli-title:`CLI` boxes.

First load data from the :file:`nacl01.dat` file.
Select :menuselection:`Data --> Load File`
from the menu (or |load-data-icon| from the toolbar) and choose the file.

.. |load-data-icon| image:: img/load_data_icon.png
   :alt: Load Data
   :class: icon

.. admonition:: CLI

   @0 < nacl01.dat 


You can zoom-in to the biggest peak using left mouse
button on the residual plot (the plot below the main plot).
To zoom out, press the :guilabel:`View whole` toolbar button.

Now all data points are active. Only the biggest peak is of
our interest, so we want to deactivate the remaining points.
Change to the :dfn:`range` mode (toolbar: |mode-range-icon|)
and deactivate not needed points with the right mouse button.

.. |mode-range-icon| image:: img/mode_range_icon.png
   :alt: Data-Range Mode
   :class: icon

.. admonition:: CLI

   A = (x > 23.0 and x < 26.0)

As our example data has no background to worry about, our next step is
to define a peak with reasonable initial values and fit it to the data.
We will use Gaussian.
To see its formula, type: ``info Gaussian`` (or ``i Gaussian``) or look for it
in the section :ref:`flist`.

Select :guilabel:`Gaussian` from the list of functions on the toolbar
and press |add-peak-icon|.

.. |add-peak-icon| image:: img/add_peak_icon.png
   :alt: Auto Add
   :class: icon

.. admonition:: CLI

   guess Gaussian


If you'd like to set the initial parameters manually,
change the GUI mode to |mode-add-icon|,
click on the plot and drag the mouse to select
the position, height and width of a new peak.

.. |mode-add-icon| image:: img/mode_add_icon.png
   :alt: Add-Peak Mode
   :class: icon

.. admonition:: CLI

   F += Gaussian(~60000, ~24.6, ~0.2)


If the peaks/functions are not named explicitely (like in this example),
they get automatic names ``%_1``, ``%_2``, etc.

Now let us fit the function.
Select :menuselection:`Fit --> Run` from the menu or press |fit-icon|.

.. |fit-icon| image:: img/fit_icon.png
   :alt: Fit
   :class: icon

.. admonition:: CLI

   fit

.. important::

    Fitting minimizes the **weighted** sum of squared residuals
    (see :ref:`nonlinear`).
    The default :ref:`weights of points <weights>` are not equal.

To see the peak parameters,
move the cursor to the top of the peak
and select "Show Info" from the context menu (right mouse button).
Or check the parameters on the sidebar.


.. admonition:: CLI

   info prop %_1

That's it!

You can save all the issued commands to a file
(:menuselection:`Session --> Save History`)

.. admonition:: CLI

   info history > myscript.fit

and later use it as a macro (:menuselection:`Session --> Execute script`).

.. admonition:: CLI

   exec myscript.fit


.. _cli:

Command Line
============

Fityk comes with a small domain-specific language (DSL).
All operations in Fityk are driven by commands of this language.
Commands can be typed in the input box in the GUI, but if all you want
to do is to type commands, the program has a separate CLI version (cfityk)
for this.

.. admonition:: Do not worry

   you do not need to learn these commands.
   It is possible to use menus and dialogs in the GUI
   and avoid typing commands.

When you use the GUI and perform an action using the menu,
you can see the corresponding command in the output window.
Fityk has less than 30 commands. Each performs a single actions,
such as loading data from file, adding function, assigning variable,
fitting, or writing results to a file.

A sequence of commands written down in a file makes a script (macro),
which can automate common tasks. Complex tasks may need to be programmed
in a general-purpose language. That is why Fityk has embedded Lua interpreter
(Lua is a lightweight programming language).
It is also possible to use Fityk library from a program in Python, C, C++,
Java, Ruby or Perl, and possibly from other languages supported by SWIG.

Now a quick glimpse at the syntax. The ``=->`` prompt below marks an input::

  =-> print pi
  3.14159
  =-> # this is a comment -- from `#' to the end of line
  =-> p '2+3=', 2+3  # p stands for print
  2+3 = 5
  =-> set numeric_format='%.9f'  # show 9 digits after dot
  =-> pr pi, pi^2, pi^3  # pr, pri and prin also stand for print
  3.141592654 9.869604401 31.006276680

Usually, one line has one command, but if it is really needed,
two or more commands can be put in one line::

  =-> $a = 3; $b = 5  # two commands separated with `;'

or a backslash can be used to continue a command in the next line::

  =-> print \
  ... 'this'
  this

If the user works simultaneously with multiple datasets, she can refer to
a dataset using its number: the first dataset is ``@0``, the second -- ``@1``,
etc::

  =-> fit # perform fitting of the default dataset (the first one)
  =-> @2: fit # fit the third dataset (@2)
  =-> @2 @3: fit # fit the third dataset (@2) and then the fourth one (@3)
  =-> @*: fit # fit all datasets, one by one

Settings in the program are changed with the command ``set``::

  set key = value

For example::

  =-> set logfile = 'C:\log.fit' # log all commands to this file
  =-> set verbosity = 1 # make output from the program more verbose
  =-> set epsilon = 1e-14

The last example changes the *ε* value, which is used to test floating-point
numbers *a* and *b* for equality (it is well known that due to rounding
errors the equality test for two numbers should have some tolerance,
and the tolerance should be tailored to the application): \|\ *a−b*\ | < *ε*.

To run a single command with different settings, add ``with key=value`` before
the command::

  =-> print pi == 3.14  # default epsilon = 10^-12
  0
  =-> with epsilon = 0.1 print pi == 3.14  # abusing epsilon
  1

.. highlight:: none

Each line has a syntax::

  [[@...:] [with ...] command [";" command]...] [#comment]

All the commands are described in the next chapters.
