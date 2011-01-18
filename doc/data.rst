
Data
====

.. _dataload:

Loading data
------------

Data files are read using the `xylib library <http://xylib.sourceforge.net/>`_.

.. admonition:: In the GUI

   click |load-data-icon|. If it just works for your files, you may go
   straight to :ref:`activepoints`.

.. |load-data-icon| image:: img/load_data_icon.png
   :alt: Load Data
   :class: icon


Points are loaded from files using the command::

   dataslot < filename[:xcol:ycol:scol:block] [filetype options...]

where

- *dataslot* should be replaced with ``@0``, unless many datasets
  are to be used simultaneously (for details see: :ref:`multidata`),

- *xcol*, *ycol*, *scol* (supported only in text file) are columns
  corresponding to x, y and std. dev. of y.
  Column 0 means index of the point: 0 for the first point,
  1 for the second, etc.

- *block* is only supported by formats with multiple blocks of data.

- *filetype* usually can be omitted, because in most of the cases
  the filetype can be detected; the list of supported filetypes is
  at the end of this section

- *options* depend on a filetype and usually are omitted

If the filename contains blank characters, a semicolon or comma, it
should be put inside single quotation marks (together with colon-separated
indices, if any).

Multiple y columns and/or blocks can be specified, see the examples below::

    @0 < foo.vms
    @0 < foo.fii text first_line_header
    @0 < foo.dat:1:4:: # x,y - 1st and 4th columns
    @0 < foo.dat:1:3,4:: # load two dataset (with y in columns 3,4)
    @0 < foo.dat:1:3..5:: # load three dataset (with y in columns 3,4,5)
    @0 < foo.dat:1:4..6,2:: # load four dataset (y: 4,5,6,2)
    @0 < foo.dat:1:2..:: # load 2nd and all the next columns as y
    @0 < foo.dat:1:2:3: # read std. dev. of y from 3rd column
    @0 < foo.dat:0:1:: # x - 0,1,2,..., y - first column
    @0 < 'foo.dat:0:1::' # the same
    @0 < foo.raw::::0,1 # load two first blocks of data (as one dataset)

Information about loaded data can be obtained with::

   info data

Supported filetypes
~~~~~~~~~~~~~~~~~~~

text
    ASCII text, multicolumn numeric data.
    The details are given in the next section.

dbws
    format used by DBWS (program for Rietveld analysis)
    and DMPLOT.

cpi
    Sietronics Sieray CPI format

uxd
    Siemens/Bruker UXD format (powder diffraction data)

bruker_raw
    Simens-Bruker RAW format (version 1,2,3)

rigaku_dat
    Rigaku dat format (powder diffraction data)

vamas
    VAMAS ISO-14976
    (only experiment modes: "SEM" or "MAPSV" or "MAPSVDP" and
    only "REGULAR" scan mode are supported)

philips_udf
    Philips UDF (powder diffraction data)

philips_rd
    Philips RD raw scan format V3 (powder diffraction data)

spe
    Princeton Instruments WinSpec SPE format
    (only 1-D data is supported)

pdcif
    CIF for powder diffraction

The full list is available at: http://xylib.sourceforge.net/.

Reading text files
~~~~~~~~~~~~~~~~~~
The *xylib* library can read TSV or CSV formats (tab or comma separated
values). In fact, the values can be separated by any whitespace character
or by one of ,;: punctations, or by any combination of these.

Empty lines and comments that start with hash (#) are skipped.

Since there is a lot of files in the world that contain numeric data mixed
with text, unless the :option:`strict` option is given
any text that can not be interpreted as a number is regarded a start of
comment (the rest of the line is ignored).

Note that the file is parsed regardless of blocks and columns specified
by the user. The data read from the file are first stored in a table
with *m* columns and *n* rows.
If some of the lines have 3 numbers in it, and some have 5 numbers, we can
either discard the lines that have 3 numbers or we can discard the numbers
in 4th and 5th column. Usually the latter is done, but there are exceptions.
The shorter lines are ignored

* if it is the last line in the file
  (probably the program was terminated while writing the file),

* if it contains only one number, but the prior lines had more numbers
  (this may be a comment that starts with a number)

* if all the (not ignored) prior lines and the next line are longer

These rule were introduced to read free-format log files with
textual comments inserted between lines with numeric data.

For now, xylib does not handle well nan's and inf's in the data.

Data blocks and columns may have names. These names are used to set
a title of the dataset (see :ref:`multidata` for details).
If the option :option:`first_line_header` is given and the number of words
in the first line is equal to the number of data columns,
each word is used as a name of corresponding column.
If the number of words is different, the first line is used as a name of the
block.
If the :option:`last_line_header` option is given, the line preceding
the first data line is used to set either column names or the block name.

If the file starts with the "``LAMMPS (``" string,
the :option:`last_line_header` option is set automatically.
This is very helpful when plotting data from LAMMPS log files.

.. _activepoints:

Active and inactive points
--------------------------

We often have the situation that only a part of the data from a file is
of interest. In Fityk, each point is either *active* or *inactive*.
Inactive points are excluded from fitting and all calculations.
A data :ref:`transformation <transform>`::

   A = boolean-condition

can be used to change the state of points.

.. admonition:: In the GUI

   data points can be activated and disactivated with mouse
   in the data-range mode (toolbar: |mode-range-icon|).

.. |mode-range-icon| image:: img/mode_range_icon.png
   :alt: Data-Range Mode
   :class: icon


.. _weights:

Standard deviation (or weight)
------------------------------

When fitting data, we assume that only the *y* coordinate is subject to
statistical errors in measurement. This is a common assumption.
To see how the *y*'s standard deviation, *σ*, influences fitting
(optimization), look at the weighted sum of squared residuals formula
in :ref:`nonlinear`.
We can also think about weights of points -- every point has a weight
assigned, that is equal :math:`w_i=1/\sigma_i^2`.

Standard deviation of points can be
:ref:`read from file <dataload>` together with the *x* and *y*
coordinates. Otherwise, it is set either to max(*y*:sup:`1/2`, 1)
or to 1, depending on the value of :option:`data_default_sigma` option.
Setting std. dev. as a square root of the value is common
and has theoretical ground when *y* is the number of independent events.
You can always change the standard deviation, e.g. make it equal for every
point with the command: ``S=1``.
See :ref:`transform` for details.

.. note:: It is often the case that user is not sure what standard deviation
          should be assumed, but it is her responsibility to pick something.

.. _transform:

Data point transformations
--------------------------

Every data point has four properties: *x* coordinate, *y* coordinate,
standard deviation of *y* and active/inactive flag.
These properties can be changed using symbols ``X``, ``Y``, ``S`` and ``A``,
respectively. It is possible to either change a single point or apply
a transformation to all points. For example:

* ``Y[3]=1.2`` assigns the *y* coordinate of the 4th point (0 is first),
* ``Y = -y`` changes the sign of the *y* coordinate for all points.

On the left side of the equality sign you can have one of symbols ``X``, ``Y``,
``S``, ``A``, possibly with the index in brackets. The symbols on the left
side are case insensitive.

The right hand side is a mathematical expression that can have special
variables:

* lower case letters ``x``, ``y``, ``s``, ``a`` represent properties of data
  points before transformation,

* upper case ``X``, ``Y``, ``S``, ``A`` stand for the same properties
  after transformation,

* ``M`` stands for the number of points.

* ``n`` stands for the index of currently transformed point,
  e.g., ``Y=y[M-n-1]`` means that *n*-th point (*n*\ =0, 1, ... M-1)
  is assigned *y* value of the *n*-th point from the end.

Before the transformation a new array of points is created as a copy of the
old array.
Operations are applied sequentially from the first point to the last one,
so while ``Y[n+1]`` and ``y[n+1]`` have always the same value,
``Y[n-1]`` and ``y[n-1]`` may differ. For example, the two commands::

   Y = y[n] + y[n-1]
   Y = y[n] + Y[n-1]

differ. The first one adds to each point the value of the previous point.
The second one adds the value of the previous point *after* transformation,
so effectively it adds the sum of all previous points.
The index ``[n]`` could be omitted (``Y = y + y[n-1]``).
The value of undefined points, like ``y[-1]`` and ``Y[-1]``,
is explained later in this section.

Expressions can contain:

- real numbers in normal or scientific format (e.g. ``1.23e5``),

- constants ``pi``, ``true`` (1), ``false`` (0)

- binary operators: ``+``, ``-``, ``*``, ``/``, ``^``,

- boolean operators: ``and``, ``or``, ``not``,

- comparisions: ``>``, ``>=``, ``<``, ``<=``, ``==``, ``!=``.

- one argument functions:

  * ``sqrt``
  * ``exp``
  * ``log10``
  * ``ln``
  * ``sin``
  * ``cos``
  * ``tan``
  * ``sinh``
  * ``cosh``
  * ``tanh``
  * ``atan``
  * ``asin``
  * ``acos``
  * ``erf``
  * ``erfc``
  * ``gamma``
  * ``lgamma`` (=ln(\|\ ``gamma()``\ \|))
  * ``abs``
  * ``round`` (rounds to the nearest integer)

- two argument functions:

  * ``mod`` (modulo)
  * ``min2``
  * ``max2`` (e.g. ``max2(3,5)`` will give 5),
  * ``randuniform(a, b)`` (random number from interval (a, b)),
  * ``randnormal(mu, sigma)`` (random number from normal distribution),
  * ``voigt(a, b)``
    = :math:`\frac{b}{\pi} \int_{-\infty}^{+\infty} \frac{\exp(-t^2)}{b^2+(a-t)^2} dt`

- ternary ``?:`` operator: ``condition ?  expression1 : expression2``,
  which returns *expression1* if condition is true and *expression2* otherwise.

A few examples.

* The *x* scale of diffraction pattern can be changed from 2\ *θ* to *Q*::

    X = 4*pi * sin(x/2*pi/180) / 1.54051 # Cu 2θ -> Q

* Negative *y* values can be zeroed::

    Y = max2(y, 0)

* All standard deviations can be set to 1::

    S = 1
    
* It is possible to select active range of data::

    A = x > 40 and x < 60 # select range (40, 60)

All operations are performed on **real numbers**.
Two numbers that differ less than *ε*
(the value of *ε* is set by the :ref:`option epsilon <epsilon>`)
are considered equal.

Points can be created or deleted by changing the value of ``M``.
For example, the following commands::

    M=500; x=n/100; y=sin(x)

create 500 points and generate a sinusoid.

Points are kept sorted according to their *x* coordinate.
The sorting is performed after each transformation.

.. note:: Changing the *x* coordinate may change the order
          and indices of points.

Indices, like all other values, are computed in the real number domain.
If the index is not integer (it is compared using *ε* to the rounded value):

* ``x``, ``y``, ``s``, ``a`` are interpolated linearly.
  For example, ``y[2.5]`` is equal to ``(y[2]+[3])/2``.
  If the index is less than 0 or larger than M-1, the value for the first
  or the last point, respectively, is returned.

* For ``X``, ``Y``, ``S``, ``A`` the index is rounded to integer.
  If the index is less than 0 or larger than M-1, 0 is returned.

Transformations separated by commas (``,``) form a sequance of transformations.
During the sequance, the vectors ``x``, ``y``, ``s`` and ``a`` that contain
old values are not changed. This makes possible to swap the axes::

   X=y, Y=x

The special ``index(arg)`` function returns the index of point that has
*x* equal *arg*, or, if there is no such point, the linear interpolation
of two neighbouring indices. This enables equilibrating the step of data
(with interpolation of *y* and *σ*)::

   X = x[0] + n * (x[M-1]-x[0]) / (M-1), Y = y[index(X)], S = s[index(X)]

It is possible to delete points for which given condition is true,
using expression ``delete(condition)``, e.g.::
    
    delete(not a) # delete inactive points

    # reduce twice the number of points, averaging x and adding y
    x = (x[n]+x[n+1])/2
    y = y[n]+y[n+1]
    delete(mod(n,2) == 1)

If you have more than one dataset, you may need to specify to which
dataset the transformation applies. See :ref:`multidata` for details.

The value of a data expression can be shown using the ``print`` command.
The precision of printed numbers is governed by the
:ref:`numeric_format <numeric_format>` option.

::

    print M # the number of points
    print y[index(20)] # value of y for x=20


Aggregate functions
-------------------

Aggregate functions have syntax::

   aggregate(expression [if condition])

and return a single value, calculated from values of all points
for which the given condition is true. If the condition is omitted, all points
in the dataset are taken into account.

The following aggregate functions are recognized:

* ``min()`` --- the smallest value,

* ``max()`` --- the largest value,

* ``sum()`` --- the sum,

* ``avg()`` --- the arithmetic mean,

* ``stddev()`` --- the standard deviation,

* ``darea()`` --- a function used to normalize the area (see the example below).
          It returns the sum of
          *expression*\ \*(*x*\ [*n*\ +1]-*x*\ [*n*-1])/2.
          In particular, ``darea(y)`` returns the interpolated area under
          data points.

.. note:: There is no ``count`` function, use ``sum(1 if criterium)`` instead.

Examples::

    p avg(y) # print the average y value
    p max(y) # the largest y value
    p max(y if x > 40 and x < 60)   # the largest y value for x in (40, 60)
    p max(y if a) # the largest y value in the active range
    p sum(1 if y>100) # the number of points that have y above 100
    p sum(1 if y>avg(y)) # aggregate functions can be nested
    p y[min(n if y > 100)] # the first (from the left) value of y above 100

    # take the first 2000 points, average them and subtract as background
    Y = y - avg(y if n<2000)

    Y = y / darea(y) # normalize data area


.. _funcindt:

Functions and variables in data transformation
----------------------------------------------

You may postpone reading this section and read about the :ref:`model` first.

Variables ($foo) and functions (%bar) can be used in data transformations,
e.g.::

    Y = y / $foo  # divides all y's by $foo
    Y = y - %f(x) # subtracts function %f from data
    Y = y - @0.F(x) # subtracts all functions in F

    # Fit constant x-correction (e.g. a shift in the scale of the instrument
    # collecting data), correct the data and remove the correction from the model.
    Z = Constant(~0)
    fit
    X = x + @0.Z(x) # data transformation is here
    Z = 0

.. admonition:: In the GUI

   in the *Baseline Mode* (|mode-bg-icon|),
   functions ``Spline`` and ``Polyline``
   are used to subtract manually selected background.
   Clicking |strip-bg-icon| results in a command like this::

    %bg0 = Spline(14.2979,62.1253, 39.5695,35.0676, 148.553,49.9493)
    Y = y - %bg0(x)

   Clicking the same button again will undo the subtraction by::

    Y = y + %bg0(x)

   The function edited in the *Baseline Mode* is always named ``%bgX``,
   where *X* is the index of the dataset.

.. |mode-bg-icon| image:: img/mode_bg_icon.png
   :alt: Baseline Mode
   :class: icon

.. |strip-bg-icon| image:: img/strip_bg_icon.png
   :alt: Strip Background
   :class: icon

Values of the function parameters (e.g. ``%fun.a0``) and pseudo-parameters
``Center``, ``Height``, ``FWHM`` and ``Area`` (e.g. ``%fun.Area``)
can also be used.
Pseudo-parameters are supported only by functions, which know
how to calculate these properties.

It is also possible to calculate some properties of %functions:

- ``%f.numarea(x1, x2, n)`` gives area integrated numerically
  from *x1* to *x2* using trapezoidal rule with *n* equal steps.

- ``%f.findx(x1, x2, y)`` finds *x* in interval (*x1*, *x2*) such that
  %f(*x*)=\ *y* using bisection method combined with Newton-Raphson method.
  It is a requirement that %f(*x1*) < *y* < %f(*x2*).

- ``%f.extremum(x1, x2)`` finds *x* in interval (*x1*, *x2*)
  such that %f'(*x*)=0 using bisection method.
  It is a requirement that %f'(*x1*) and %f'(*x2*) have different signs.

A few examples::

    print %fun.numarea(, 0, 100, 10000) # shows area of function %fun
    print %_1(%_1.extremum(40, 50)) # shows extremum value
    
    # calculate FWHM numerically, value 50 can be tuned
    $c = {%f.Center}
    p %f.findx($c, $c+50, %f.Height/2) - %f.findx($c, $c-50, %f.Height/2)
    p %f.FWHM # should give almost the same.

.. _multidata:

Working with multiple datasets
------------------------------

Let us call a set of data that usually comes from one file --
a :dfn:`dataset`. It is possible to work simultaneously with multiple datasets.
Datasets have numbers and are referenced by ``@`` with the number,
(e.g. ``@3``).
The user can specify which dataset the command should be applied to::

   @0: M=500    # change the number of points in the first dataset
   @1 @2: M=500 # the same command applied to two datasets
   @*: M=500    # and the same applied to all datasets

If the dataset is not specified, the command applies to the default dataset,
which is initially @0. The ``use`` command changes the default dataset::

   use @2 # set @2 as default

To load dataset from file, use one of the commands::

   @n < filename:xcol:ycol:scol:block filetype options...

   @+ < filename:xcol:ycol:scol:block filetype options...

The first one uses existing data slot and the second one creates
a new slot.  Using ``@+`` increases the number of datasets,
and the command ``delete @n`` decreases it.

The dataset can be duplicated (``@+ = @n``) or transformed,
more on this in :ref:`the next section <datasettr>`.

Each dataset has a separate :ref:`model <model>`,
that can be fitted to the data. This is explained in the next chapter.

Each dataset also has a title (it does not have to be unique, however).
When loading file, a title is automatically created:

* if there is a name associated with the column *ycol*, the title
  is based on it;
* otherwise, if there is a name associated with the data block read from file,
  the title is set to this name;
* otherwise, the title is based on the filename

Titles can be changed using the command::

   @n.title = 'new-title'

To print the title of the dataset, type ``@n: info title``.

.. _datasettr:

Dataset transformations
-----------------------

Transformations that are defined for a whole dataset, not for each point
separately, will be called :dfn:`dataset tranformations`.
They have the following syntax::

   @n = dataset-transformation @m

or more generally::

   @n = dataset-transformation @m + @k + ...

where *dataset-transformation* can be one of:

``sum_same_x``
    Merges points which have distance in *x* is smaller than
    :ref:`epsilon <epsilon>`.
    *x* of the merged point is the average,
    and *y* and *σ* are sums of components.

``avg_same_x``
    The same as ``sum_same_x``, but *y* and *σ* are set as the average
    of components.

``shirley_bg``
    Calculates Shirley background
    (useful in X-ray photoelectron spectroscopy).

``rm_shirley_bg``
    Calculates data with removed Shirley background.

A sum of datasets (``@n + @m + ...``) contains all points from all component
datasets. If datasets have the same x values, the sum of y values can be
obtained using ``@+ = sum_same_x @n + @m + ...``.

Examples::

  @+ = @0 # duplicate the dataset
  @+ = @0 + @1 # create a new dataset from @0 and @1
  @0 = rm_shirley_bg @0 # remove Shirley background 


.. _dexport:

Exporting data
--------------

Command::

   print all: expression, ... > file.tsv

can export data to an ASCII TSV (tab separated values) file.

.. admonition:: In the GUI

    :menuselection:`Data --> Export`

To export data in a 3-column (x, y and standard deviation) format, use::

   print all: x, y, s > file.tsv

Any expressions can be printed out::

   p all: n+1, x, y, F(x), y-F(x), %foo(x), sin(pi*x)+y^2 > file.tsv

It is possible to select which points are to be printed by replacing ``all``
with ``if`` followed by a condition::

   print if a: x, y # only active points are printed
   print if x > 30 and x < 40: x, y # only points in (30,40)

The option :ref:`numeric_format <numeric_format>`
controls the format and precision of all numbers.

