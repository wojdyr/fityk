
.. _getstarted:

Getting started
###############

Graphical interface
===================

The GUI window of fityk consists of (from the top): menu bar, toolbar,
main plot, helper (residual) plot, output window, input field, status bar
and of sidebar at the right-hand side. The input field allows you to type and
execute commands in a similar way as is done in the CLI version. The
output window shows the results.

All GUI commands are converted into text and are visible in the output window,
providing a simple way to learn the syntax.

The main plot can display data points, model that is to be fitted to the
data and component functions of the model. Use the pop-up menu (click
right button on the plot) to configure it. Some properties of the plot
(e.g. colors of data points) can be changed using the sidebar.

One of the most useful things which can be displayed by the helper
plot is the difference between the data and the model (it is also controlled
by a pop-up menu). Hopefully, a quick look at this menu and a minute or
two's worth of experiments will show the potential of this plot.

Configuration of the GUI (visible windows, colors, etc.) can be saved
using :menuselection:`GUI --> Save current config`.

.. image:: img/fityk-with-tooltip.png
   :align: center
   :scale: 50

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

The :abbr:`CLI (Command Line Interface)` version of the program is all
about typing commands. From time to time it is also handy to type
a command in the :abbr:`GUI (Graphical User Interface)`,
but usually the GUI provides more intuitive, mouse-driven way to perform
the same operation -- it is mentioned in the the grey boxes below.

.. admonition:: In the GUI

   select :menuselection:`Data --> Load File`
   from the menu (or |load-data-icon| from the toolbar) and choose the file.

.. |load-data-icon| image:: img/load_data_icon.png
   :alt: Load Data
   :class: icon

If you use the GUI, you can zoom-in to the biggest peak using left mouse
button on the residual plot (the plot below the main plot).
To zoom out, press the :guilabel:`View whole` toolbar button.

Now all data points are active. Because only the biggest peak is of
interest for the sake of this example, the remaining points can be
deactivated::

   A = (x > 23.0 and x < 26.0)
   
.. admonition:: In the GUI

   change to the :dfn:`range` mode (toolbar: |mode-range-icon|)
   and deactivate not needed points with the right mouse button.

.. |mode-range-icon| image:: img/mode_range_icon.png
   :alt: Data-Range Mode
   :class: icon

As our example data has no background to worry about, our next step is
to define a peak with reasonable initial values and fit it to the data.
We will use Gaussian.
To see its formula, type: ``info Gaussian`` (or ``i Gaussian``) or look for it
in the section :ref:`flist`.

To add a peak, either set the initial parameters manually::

   F += Gaussian(~60000, ~24.6, ~0.2)

.. admonition:: In the GUI

    it is also possible to set the initial parameters with the mouse:
    change the GUI mode to |mode-add-icon|,
    click on the plot and drag the mouse to select
    the position, height and width of a new peak.

.. |mode-add-icon| image:: img/mode_add_icon.png
   :alt: Add-Peak Mode
   :class: icon

or let the program guess it::

   guess Gaussian

.. admonition:: In the GUI

   select :guilabel:`Gaussian` from the list of functions on the toolbar
   and press |add-peak-icon|.

.. |add-peak-icon| image:: img/add_peak_icon.png
   :alt: Auto Add
   :class: icon

If the functions are not named explicitely (like in this example),
they get automatic names ``%_1``, ``%_2``, etc.


Now let us fit the function. Type: ``fit``.

.. admonition:: In the GUI

    select :menuselection:`Fit --> Run` from the menu or press |fit-icon|.

.. |fit-icon| image:: img/fit_icon.png
   :alt: Fit
   :class: icon

.. important::

    Fitting minimizes the **weighted** sum of squared residuals
    (see :ref:`nonlinear`).
    The default :ref:`weights of points <weights>` are not equal.

To see the peak parameters, type: ``info prop %_1``.

.. admonition:: In the GUI

   move the cursor to the top of the peak
   and try out the context menu (the right mouse button),
   or check the parameters on the sidebar.

That's it!

You can save all the issued commands to a file::

   info history > myscript.fit

and later use it as a macro::

   exec myscript.fit

.. admonition:: In the GUI

   use :menuselection:`Session --> Save History`
   and :menuselection:`Session --> Execute script`, correspondingly.

