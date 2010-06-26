
Reference
#########

In this part of the manual:

- all features, with exception of the graphical interface features,
  are described,

- several concepts (such as *simple-variable* and *compound-variable*),
  that reflect the internal design of the program, are introduced,

- the syntax of the Fityk mini-language is explained,

- it is rarely mentioned, that in the GUI typing the commands can be usually
  avoided and most of the operations can be done with mouse clicking.

The Fityk mini-language consists of *commands*.

Basically, there is one command per line.  If for some reason it is more
comfortable to place more than one command in one line, they can be
separated with a semicolon (;).

Most of the commands can have arguments separated by a comma (,),
e.g. ``delete %a, %b, %c``.

Most of the commands can be shortened: e.g. you can type
``inf`` or ``in`` or ``i`` instead of ``info``.
See :ref:`shortenings` for details.

The symbol '#' starts a comment - everything from the
hash (#) to the end of the line is ignored.

Data from experiment
====================

.. _DataLoad:

Loading data
------------

Data files are read using the
`xylib library <http://www.unipress.waw.pl/fityk/xylib/>`_.

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
    @0 < foo.fii text first-line-header
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

   info data [in @0]

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

canberra_mca
    Spectral data stored by Canberra MCA systems

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

...
    what else would you like to have here?

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
in 4th and 5th column. Usually the latter is done, unless it seems that the
shorter lines should be ignored. The line is ignored:

* if it is the last line in the file and it contains less numbers than other
  lines (probably the program was terminated while writing the file),

* if it contains only one number, but the prior lines had more numbers,

* if all the (not ignored) prior lines and the next line are longer

.. note:: Xylib doesn't handle well nan's and inf's in the data. This will be
          improved in the future.

Data blocks and columns may have names. These names are used to set
a title of the dataset (see :ref:`multidata` for details).
If the option :option:`first-line-header` is given and the number of words
in the first line is equal to the number of data columns,
each word is used as a name of corresponding column.
If the number of words is different, the first line is used as a name of the
block.
If the :option:`last-line-header` option is given, the line preceding
the first data line is used to set either column names or the block name.

If the file starts with the "`LAMMPS (`" string,
the :option:`last-line-header` option is set implicitely.
This is very helpful when plotting data from LAMMPS log files.

Active and inactive points
--------------------------

We often have the situation that only a part of the data from a file is
of interest. In Fityk, each point is either *active* or *inactive*.
Inactive points are excluded from fitting and all calculations.
A data :ref:`transformation <transform>`::

   A = boolean-condition

can be used to change the state of points.

In the GUI, there is a ``Data-Range Mode`` that allows to activate and
disactivate points with mouse.

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
:ref:`read from file <DataLoad>` together with the *x* and *y*
coordinates. Otherwise, it is set either to max(*y*:sup:`1/2`, 1)
or to 1, depending on the value of :option:`data-default-sigma` option.
Setting std. dev. as a square root of the value is common
and has theoretical ground when *y* is the number of independent events.
You can always change standard deviation, e.g. make it equal for every
point with command: ``S=1``.
See :ref:`transform` for details.

.. note:: It is often the case that user is not sure what standard deviation
          should be assumed, but it is her responsibility to pick something.

.. _transform:

Data point transformations
--------------------------

Every data point has four properties: *x* coordinate, *y* coordinate,
standard deviation of *y* and active/inactive flag. Lower case
letters ``x``, ``y``, ``s``, ``a`` stand for these properties
before transformation,
and upper case ``X``, ``Y``, ``S``, ``A`` for the same properties
after transformation.
``M`` stands for the number of points.

Data can be transformed using assignments.
For example, the ``Y=-y`` command changes the sign of the *y* coordinate
of every point.

You can apply transformation to selected points:

* ``Y[3]=1.2`` changes the point with index 3
  (i.e., the 4th point, the first has index 0),
* ``Y[3..6]=1.2`` --- the points with indices 3, 4, 5, but not 6,
* ``Y[2..]=1.2`` --- the points with indices 2, 3, ...
* ``Y[..4]=1.2`` --- the points with indices 0, 1, 2, 3.
* ``Y=1.2`` --- all points

Most of operations are executed sequentially for points from the first
to the last one. ``n`` stands for the index of currently transformed point.
The sequance of commands::

    M=500; x=n/100; y=sin(x)

will generate the sinusoid dataset with 500 points.

If you have more than one dataset, you have to specify explicitly
which dataset transformation applies to. See :ref:`multidata` for details.

.. note:: Points are kept sorted according to their x coordinate,
   so changing x coordinate of points
   will also change the order and indices of points.

Expressions can contain:

- real numbers in normal or scientific format (e.g. ``1.23e5``),

- constant ``pi``,

- binary operators: ``+``, ``-``, ``*``, ``/``, ``^``,

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

  * ``min2``
  * ``max2`` (e.g. ``max2(3,5)`` will give 5),
  * ``randuniform(a, b)`` (random number from interval (a, b)),
  * ``randnormal(mu, sigma)`` (random number from normal distribution),
  * ``voigt(a, b)``
    = :math:`\frac{b}{\pi} \int_{-\infty}^{+\infty} \frac{\exp(-t^2)}{b^2+(a-t)^2} dt`

- ternary ``?:`` operator: ``condition ?  expression1 : expression2``,
  which performs *expression1* if condition is true
  and *expression2* otherwise.
  Conditions can be built using boolean operators and comparisions:
  ``AND``, ``OR``, ``NOT``, ``>``, ``>=``, ``<``, ``<=``, ``==``,
  ``!=`` (or ``<>``), ``TRUE``, ``FALSE``.

The value of a data expression can be shown using the command ``info``,
see examples at the end of this section. The precision of printed numbers
is governed by the option :ref:`info-numeric-format <info_numeric_format>`.

Linear interpolation of y (or any other property: s,a,X,Y,S,A)
between two points can be calculated using special syntax::

   y[x=expression]

If the given x is outside of the current data range, the value of
the first/last point is returned.

.. note:: All operations are performed on real numbers.

Two numbers that differ less than *ε*
(the value of *ε* is set by the :ref:`option epsilon <epsilon>`)
are considered equal.

Indices are also computed in the real number domain,
and then rounded to the nearest integer.

Transformations separated by commas (``,``) form a sequance of transformations.
For example, it is possible to swap axes with the command ::

   X=y, Y=x


Points are sorted according to their *x* coordinate. The sorting is performed
after each transformation.

In the sequance of transformations the order of points can be temporarily
changed with the ``order=t`` expression,
where *t* is one of ``x``, ``y``, ``s``, ``a``, ``-x``, ``-y``, ``-s``, ``-a``.
For example, half of the points with largest *σ* can be disactivated with::

   order=s, a = (n < M/2)

Points can be deleted using the following syntax::

   delete[index-or-range]

or ::

   delete(condition)

and created simply by increasing the value of ``M``.

There are also aggregate functions:

- ``min`` (the smallest value),

- ``max`` (the largest value),

- ``sum`` (sum of all values),

- ``avg`` (arithmetic mean of all values),

- ``stddev`` (standard deviation of all values),

- ``darea`` (``darea(y)`` gives the interpolated area under data points,
          and can be used to normalize the area.
          ``darea`` is implemented as *t\*(x[n+1]-x[n-1])/2*,
          where *t* is the value of the *expression*).

They have two forms::

   aggregatefunc(expression)

   aggregatefunc(expression if condition)

In the first form the value of *expression* is calculated for all points.
In the second, only the points for which the *condition* is true are
taken into account.

True value in data expression is represented numerically by 1.,
and false by 0, so ``sum`` can be also used to count points
that fulfil given criteria.

A few examples::

    # integrate
    Y[1...] = Y[n-1] + y[n] 

    # delete inactive points
    delete(not a) 

    # reduce twice the number of points, averaging x and adding y
    x[...-1] = (x[n]+x[n+1])/2
    y[...-1] = y[n]+y[n+1]
    delete(n%2==1)

    # change x scale of diffraction pattern (2theta -> Q)
    X = 4*pi * sin(x/2*pi/180) / 1.54051

    # make equal step, keep the number of points the same
    X = x[0] + n * (x[M-1]-x[0]) / (M-1),  Y = y[x=X], S = s[x=X], A = a[x=X]

    # take the first 2000 points, average them and subtract as background
    Y = y - avg(y if n<2000)

    # Fityk can be used as a simple calculator
    i 2+2 #4
    i sin(pi/4)+cos(pi/4) #1.41421
    i gamma(10) #362880

    # normalize data area
    Y = y / darea(y)

    # calculations that use aggregate functions
    i max(y) # the largest y value
    i max(y if a) # the largest y value in the active range
    i sum(y>100) # count the points that have y greater than 100
    i sum(y>avg(y)) # count the points that have y greater than the arithmetic mean
    i darea(y-F(x) if 20<x and x<25) # example of more complex syntax

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

In the *Baseline Mode* in the GUI, functions ``Spline()`` and ``Polyline()``
are used to substract background, that have been manually marked by the user.
Clicking ``Strip background`` results in a commands like this::

    %bg0 = Spline(14.2979,62.1253, 39.5695,35.0676, 148.553,49.9493)
    Y = y - %bg0(x)

.. note:: The GUI uses functions named ``%bgX``, where *X* is the index of the
          dataset, and the type of the function is either ``Spline``
          or ``Polyline``, to handle the baseline. This allows user to set
          the function manually (or in a script) and then edit the baseline
          in the *Baseline Mode*.

Values of the function parameters (e.g. ``%fun.a0``) and pseudo-parameters
Center, Height, FWHM and Area (e.g. ``%fun.Area``) can also be used.
Pseudo-parameters are supported only by functions, which know
how to calculate these properties.

It is also possible to calculate some properties of %functions:

- ``numarea(%f, x1, x2, n)`` gives area integrated numerically
  from *x1* to *x2* using trapezoidal rule with *n* equal steps.

- ``findx(%f, x1, x2, y)`` finds *x* in interval (*x1*, *x2*) such that
  %f(*x*)=\ *y* using bisection method combined with Newton-Raphson method.
  It is a requirement that %f(*x1*) < *y* < %f(*x2*).

- ``extremum(%f, x1, x2)`` finds *x* in interval (*x1*, *x2*)
  such that %f'(*x*)=0 using bisection method.
  It is a requirement that %f'(*x1*) and %f'(*x2*) have different signs.

A few examples::

    info numarea(%fun, 0, 100, 10000) # shows area of function %fun
    info %_1(extremum(%_1, 40, 50)) # shows extremum value
    
    # calculate FWHM numerically, value 50 can be tuned
    $c = {%f.Center}
    i findx(%f, $c, $c+50, %f.Height/2) - findx(%f, $c, $c-50, %f.Height/2)
    i %f.FWHM # should give almost the same.

.. _multidata:

Working with multiple datasets
------------------------------

Let us call a set of data that usually comes from one file --
a :dfn:`dataset`.
All operations described above assume only one dataset.
If there are more datasets created, it must be explicitly
stated which dataset the command is being applied to, e.g.
``M=500 in @0``.
Datasets have numbers and are referenced by '@' with the number,
e.g. ``@3``.
``@*`` means all datasets (e.g. ``Y=y/10 in @*``).

To load dataset from file, use one of commands::

   @n < filename:xcol:ycol:scol:block filetype options...

   @+ < filename:xcol:ycol:scol:block filetype options...

The first one uses existing data slot and the second one creates
a new slot.  Using @+ increases the number of datasets,
and command ``delete @n`` decreases it.

The dataset can be duplicate (``@+ = @n``) or transformed,
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

   set @n.title=new-title

To print the title of the dataset, type ``info title in @n``.

You calculate values of a data expression for each dataset and print
a list of results, e.g. ``i+ avg(y) in @*``.

.. _datasettr:

Dataset transformations
-----------------------

There is also another kind of transformations,
:dfn:`dataset tranformation`, which operate on a whole dataset,
not single points::

   @n = dataset-transformation @m

or more generally::

   @n = dataset-transformation @m + @k + ...

where *dataset-transformation* can be one of:

``sum_same_x``
    Merges points which distance in x is smaller than
    :ref:`epsilon <epsilon>`.
    x of a merged point is the average,
    and y and sigma are sums of components.

``avg_same_x``
    The same as sum_same_x, but y and sigma of a merged point
    is set as an average of components.

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

   info dataslot (expression , ...) > file.tsv

can export data to an ASCII TSV (tab separated values) file.

To export data in a 3-column (x, y and standard deviation) format, use::

   info @0 (x, y, s) > file.tsv

If ``a`` is not listed in the list of columns,
like in the example above, only the active points are exported.

All expressions that can be used on the right-hand side of data
transformations can also be used in the column list.
Additionally, F and Z can be used with dataset prefix, e.g. ::

   info @0 (n+1, x, y, F(x), y-F(x), Z(x), %foo(x), a, sin(pi*x)+y^2) > file.tsv

The option :ref:`info-numeric-format <info_numeric_format>`
can be used to change the format and precision of all numbers.

.. _model:

Model
=====

.. _modelintro:

Model - Introduction
--------------------

The :dfn:`model` *F* (the function that is fitted to the data) is computed
as a sum of :dfn:`component functions`, :math:`F = \sum_i f_i`.
Each component function is one of the functions defined in the program,
such as Gaussian or polynomial.

To avoid confusion we will always use:

- the name *model* when referring to the total function fitted to data.

- and the name *function* only when referring to a component function.

Function :math:`f_i=f_i(x; \boldsymbol{a})` is a function of *x*,
and depends on a vector of parameters :math:`\boldsymbol{a}`.
This vector contains all fitted parameters.

Because we often have the situation, that the error in the *x* coordinate
of data points can be modeled with function :math:`Z(x; \boldsymbol{a})`,
we introduce this term to the model, and the final formula is:

.. math::
    F(x; \boldsymbol{a}) = \sum_i f_i(x+Z(x; \boldsymbol{a}); \boldsymbol{a})

where :math:`Z(x; \boldsymbol{a}) = \sum_i z_i(x; \boldsymbol{a})`

Note that the same :dfn:`x-correction` *Z*
is used in all functions :math:`f_i`.

Now we will have a closer look at component functions.
Every function :math:`f_i` has a type chosen from the function types
available in the program. The same is true about functions :math:`z_i`.
One of these types is the *Gaussian*. It has the following formula:

.. math::
    f_G(x; a_0, a_1, a_2)=a_{0}\exp\left[-\ln(2)\left(\frac{x-a_{1}}{a_{2}}\right)^{2}\right]

There are three parameters of Gaussian. These parameters do not
depend on *x*. There must be one :dfn:`variable`
bound to each function's parameter.

.. _variables:

Variables
---------

Variables in Fityk have names prefixed with the dollar symbol ($).
A variable is created by assigning a value to it, e.g. ::

   $foo=~5.3
   $c=3.1
   $bar=5*sin($foo)

The variables like the first one, ``$foo``,
created by assigning to it a real number prefixed with '~',
will be called :dfn:`simple-variables`.
The '~' means that the value assigned to the variable can be changed
when fitting the model to the data.

Each simple-variable is independent. In optimization terms, it corresponds
to one dimension of the space where we will look for the minimum.

In the above example, the variable ``$c`` is actually a *constant*.
``$bar`` depends on the value of ``$foo``.
When ``$foo`` changes, the value of ``$bar`` also changes.
Variables like ``$bar`` will be called :dfn:`compound-variables`.
Compound-variables can be build using operators +, -, \*, /, ^
and the functions
``sqrt``,
``exp``,
``log10``,
``ln``,
``sin``,
``cos``,
``tan``,
``sinh``,
``cosh``,
``tanh``,
``atan``,
``asin``,
``acos``,
``erf``,
``erfc``,
``lgamma``,
``abs``,
``voigt``.
This is a subset of the functions used in
:ref:`data transformations <transform>`.

The value of the data expression can be used in the variable definition.
The expression must be in braces, e.g. ``$bleh={3+5}``.
The *simple variable* can be created by preceding the left brace
with the tilde (``$bleh=~{3+5}``). A few examples::

    $foo = {y[0]}
    $foo2 = {y[0] in @0}  # dataset can be given if necessary
    $foo3 = {min(y if a) in @0}

Sometimes it is useful to freeze a variable, i.e. to prevent it from
changing while fitting. There is no special syntax for it,
but it can be done using data expressions in this way::

    $a = ~12.3 # $a is fittable
    $a = {$a}  # $a is not fittable
    $a = ~{$a}  # $a is fittable again

It is also possible to define a variable as e.g. ``$bleh=~9.1*exp(~2)``.
In this case two simple-variables (with values 9.1 and 2) are created
automatically.

Automatically created variables are named ``$_1``, ``$_2``,
``$_3``, and so on.

Variables can be deleted using the command::

   delete $variable

.. _domain:

Some fitting algorithms need to randomize the parameters of the fitted
function (i.e. they need to randomize simple variables).
For this purpose, the simple variable can have a specified :dfn:`domain`.
Note that the domain does not imply any constraints on the value
the variable can have -- it is only a hint for fitting algorithms.
Domains are used by Nelder-Mead method and Genetic Algorithms.
The syntax is as follows::

    $a = ~12.3 [11 +- 5] # center and width of the domain are given
    $b = ~12.3 [ +- 5] # if the center of the domain is not specified,
                       # the value of the variable is used

If the domain is not specified, the value of
:option:`variable-domain-percent` option is used
(domain is +/- *value-of-variable* * :option:`variable-domain-percent` / 100)

Function types and functions
----------------------------

Let us go back to functions. Function types have names that start
with upper case letter, e.g. ``Linear`` or ``Voigt``. Functions
(i.e. function instances) have names prefixed with a percent symbol,
e.g. ``%func``. Every function has a type and variables bound to its
parameters.

``info types`` shows the list of available function types.
``info FunctionType`` (e.g. ``info Pearson7``) shows formula of the
*FunctionType*.

Functions can be created by giving the type and the correct
number of variables in brackets, e.g. ::

   %f1 = Gaussian(~66254., ~24.7, ~0.264)
   %f2 = Gaussian(~6e4, $ctr, $b+$c)
   %f3 = Gaussian(height=~66254., hwhm=~0.264, center=~24.7)

Every expression which is valid on the right-hand side of a variable
assignment can be used as a variable.
If it is not just a name of a variable, an automatic variable is created.
In the above examples, two variables were implicitely created for ``%f2``:
first for value ``6e4`` and the second for ``$b+$c``).

If the names of function's parameters are given (like for ``%f3``),
the variables can be given in any order.

Function types can can have specified default values for
some parameters. The variables for such parameters can be omitted,
e.g.::

   =-> i Pearson7
   Pearson7(height, center, hwhm, shape=2) = height/(1+((x-center)/hwhm)^2*(2^(1/shape)-1))^shape
   =-> %f4 = Pearson7(height=~66254., center=~24.7, fwhm=~0.264) # no shape is given
   New function %f4 was created.

A deep copy of function (i.e. all variables that it depends on
are also copied) can be made using the command::

   %function = copy(%another_function)

Functions can be also created with the command ``guess``,
as described in :ref:`guess`.

You can change a variable bound to any of the function parameters
in this manner::

    =-> %f = Pearson7(height=~66254., center=~24.7, fwhm=~0.264)
    New function %f was created.
    =-> %f.center=~24.8
    =-> $h = ~66254
    =-> %f.height=$h
    =-> info %f
    %f = Pearson7($h, $_5, $_3, $_4)
    =-> $h = ~60000 # variables are kept by name, so this also changes %f
    =-> %p1.center = %p2.center + 3 # keep fixed distance between %p1 and %p2

Functions can be deleted using the command::

   delete %function

Variadic functions
------------------

*Variadic* function types have variable number of parameters.
Two variadic function types are defined::

    Spline(x1, y1, x2, y2, ...)
    Polyline(x1, y1, x2, y2, ...)

For example ``%f``::

    %f = Spline(22.1, 37.9, 48.1, 17.2, 93.0, 20.7)

is the *cubic spline interpolation* through points
(22.1, 37.9), (48.1, 17.2), ....

The ``Polyline`` function is similar, but gives the *polyline interpolation*.

Both ``Spline`` and ``Polyline`` functions are primarily used
for the manual baseline subtraction via the GUI.

.. _udf:

User-defined functions (UDF)
----------------------------

User-defined function types can be created using command ``define``,
and then used in the same way as built-in functions.

Example::

   define MyGaussian(height, center, hwhm) = height*exp(-ln(2)*((x-center)/hwhm)^2)

- The name of new type must start with an upper-case letter,
  contain only letters and digits and have at least two characters.

- The name of the type is followed by parameters in brackets.

- Parameter name must start with lowercase letter and,
  contain only  lowercase letters, digit and the underscore ('_').

- The name "x" is reserved, do not put it into parameter list,
  just use it on the right-hand side of the definition.

- There are special names of parameters,
  that Fityk understands:

  * if the functions is peak-like:
    ``height``, ``center``, ``fwhm``, ``area``, ``hwhm``,

  * if the function is more like linear:
    ``slope``, ``intercept``, ``avgy``.

  Parameters with such names do not need default values.
  ``fwhm`` mean full width at half maximum (FWHM),
  ``hwhm`` means half width..., i.e. fwhm/2.

- Each parameter should have a default value (see examples below).
  Default values allow adding a peak with the command ``guess`` or with
  one click in the GUI.

- The default value can be a number or expression that contains
  the special names listed above with exeption of ``hwhm`` (use
  ``fwhm/2`` instead).

UDFs can be defined in a few ways:

- by giving a full formula, like in the example above,

- as a :dfn:`re-parametrization` of existing function
  (see the ``GaussianArea`` example below),

- as a sum of already defined functions
  (see the ``GLSum`` example below),

- ``x <`` *expression* ``?`` *Function1(...)* ``:`` *Function2(...)*
  (see the ``SplitL`` example below).

When giving a full formula, right-hand side of the equality sign
is similar to the :ref:`definiton of variable <variables>`,
but the formula can also depend on *x*.
Hopefully the examples at the end of this section make the syntax clear.

.. admonition:: How it works internally

    The formula is parsed,
    derivatives of the formula are calculated symbolically,
    all expressions are simplified (but there is a lot of space for
    optimization here)
    and bytecode for virtual machine (VM) is created.

    When fitting, the VM calculates the value of the function
    and derivatives for every point.

    Possible (i.e. not implemented) optimizations include
    Common Subexpression Elimination and JIT compilation.

There is a simple substitution mechanism that makes writing complicated
functions easier.
Substitutions must be assigned in the same line, after keyword ``where``.
Example::

    define ReadShockley(sigma0=1, a=1) = sigma0 * t * (a - ln(t)) where t=x*pi/180

    # more complicated example, with nested substitutions
    define FullGBE(k, alpha) = k * alpha * eta * (eta / tanh(eta) - ln (2*sinh(eta))) where eta = 2*pi/alpha * sin(theta/2), theta=x*pi/180

.. tip:: Use the :file:`init` file for often used definitions.
         See :ref:`invoking` for details.

Defined functions can be undefined using command ``undefine``.

Examples::

    # this is how some built-in functions could be defined
    define MyGaussian(height, center, hwhm) = height*exp(-ln(2)*((x-center)/hwhm)^2)
    define MyLorentzian(height, center, hwhm) = height/(1+((x-center)/hwhm)^2)
    define MyCubic(a0=height,a1=0, a2=0, a3=0) = a0 + a1*x + a2*x^2 + a3*x^3

    # supersonic beam arrival time distribution
    define SuBeArTiDi(c, s, v0, dv) = c*(s/x)^3*exp(-(((s/x)-v0)/dv)^2)/x

    # area-based Gaussian can be defined as modification of built-in Gaussian
    # (it is the same as built-in GaussianA function)
    define GaussianArea(area, center, hwhm) = Gaussian(area/hwhm/sqrt(pi/ln(2)), center, hwhm)

    # sum of Gaussian and Lorentzian, a.k.a. PseudoVoigt (should be in one line)
    define GLSum(height, center, hwhm, shape) = Gaussian(height*(1-shape), center, hwhm)
    + Lorentzian(height*shape, center, hwhm)

    # split-Gaussian, the same as built-in SplitGaussian (should be in one line)
    define SplitG(height, center, hwhm1=fwhm*0.5, hwhm2=fwhm*0.5) =
      x < center ? Lorentzian(height, center, hwhm1)
                 : Lorentzian(height, center, hwhm2)

    # to change definition of UDF, first undefine previous definition
    undefine GaussianArea

.. _speed:

Speed of computations
---------------------

With default settings, the value of every function is calculated
at every point. Functions such as Gaussian often have non-neglegible
values only in a small fraction of all points. To speed up the calculation,
set the option :option:`cut-function-level`
to a non-zero value. For each function the range with values
greater than :option:`cut-function-level`
will be estimated, and all values outside of this range are
considered to be equal zero.
Note that not all functions support this optimization.

If you have a number of loaded dataset, and the functions in different
datasets do not share parameters, it is faster to fit the datasets
sequentially (``fit in @0; fit in @1; ...``)
then parallelly (``fit in @*``).

Each simple-variable slows down the fitting, although
this is often negligible.

Model, F and Z
--------------

As already discussed, each dataset has a separate model
that can be fitted to the data.
As can be seen from the :ref:`formula above <modelintro>`,
the model is defined as a set functions :math:`f_i`
and a set of functions :math:`z_i`.
These sets are named *F* and *Z* respectively.
The model is constructed by specifying names of functions in these two sets.

In many cases :dfn:`x-correction` Z is not used.
The fitted curve is thus the sum of all functions in F.

Command ::

   F += %function

adds  *%function* to F, command ::

   Z += %function

adds *%function* to Z.

To remove *%function* from F (or Z) either do::

   F -= %function

or ``delete %function``.

If there is more than one dataset, F and Z must be prefixed
with the dataset number (e.g. ``@1.F += %function``).

The following syntax is also valid::

    # create and add funtion to F
    %g = Gaussian(height=~66254., hwhm=~0.264, center=~24.7)
    @0.F += %g

    # create automatically named function and add it to F
    @0.F += Gaussian(height=~66254., hwhm=~0.264, center=~24.7)

    # clear F
    @0.F = 0

    # clear F and put three functions in it
    @0.F = %a + %b + %c

    # show info about the first and the last function in @0.F
    info @0.F[0], @0.F[-1]

    # the same as %bcp = copy(%b)
    %bcp = copy(@0.F[1])

    # make @1.F the exact (shallow) copy of @0.F
    @1.F = @0.F

    # make @1.F a deep copy of @0.F (all functions and variables
    # are duplicated).
    @1.F = copy(@0.F)

It is often required to keep the width or shape of peaks constant
for all peaks in the dataset. To change the variables bound to parameters
with a given name for all functions in F, use the command::

   F.param = variable

Examples::

    # Set hwhm of all functions in F that have a parameter hwhm to $foo
    # (hwhm here means half-width-at-half-maximum)
    F.hwhm = $foo

    # Bound the variable used for the shape of peak %_1 to shapes of all
    # functions in F
    F.shape = %_1.shape  

    # Create a new simple-variable for each function in F and bound the
    # variable to parameter hwhm. All hwhm parameters will be independent.
    F.hwhm = ~0.2

.. _guess:

Guessing peak location
----------------------

It is possible to guess peak location and add it to F with the command::

   [%name =] guess PeakType [[x1:x2]] [initial values...] [in @n]

e.g. ::

   %f1 = guess Gaussian [22.1:30.5] in @0

   # the same, but assign function's name automatically
   guess Gaussian [22.1:30.5] in @0

   # the same, but search for the peak in the whole dataset
   guess Gaussian in @0

   # the same, but works only if there is exactly one dataset loaded
   guess Gaussian

   guess Linear in @* # adds a function to every dataset

   # guess width and height, but set center and shape explicitely
   guess PseudoVoigt [22.1:30.5] center=$ctr, shape=~0.3 in @0

- If the range is omitted, the whole dataset will be searched.

- Name of the function is optional.

- Some of the parameters can be specified with syntax *parameter*\ =\ *variable*.

- As an exception, if the range is omitted and the parameter *center*
  is given, the peak is searched around the *center*,
  +/- value of the option :option:`guess-at-center-pm`.

Fityk offers only a primitive algorithm for peak-detection.
It looks for the highest point in a given range, and than tries
to find the width of the peak.

If the highest point is found near the boundary of the given range,
it is very probable that it is not the peak top,
and, if the option :option:`can-cancel-guess` is set to true,
the guess is cancelled.

There are two real-number options related to ``guess``:
:option:`height-correction` and :option:`width-correction`.
The default value for them is 1.
The guessed height and width are multiplied by the values of these
options respectively.

Linear function is guessed using linear regression. It is actually
fitted (but weights of points are not used), not guessed.

Displaying information
----------------------

If you are using the GUI, most of the available information can be
displayed with mouse clicks. Alternatively, you can use the
``info`` command.
Using ``info+`` instead of ``info`` sometimes gives more verbose output.

Below is the list of arguments of ``info`` related
to this chapter. The full list is in :ref:`info`

``info guess [range]``
    Shows where the ``guess`` command would find a peak.

``info functions``
    Lists all defined functions.

``info variables``
    Lists all defined variables.

``info @n.F``
    Shows information about F in dataset *n*.

``info @n.Z``
    Shows information about Z in dataset *n*.

``info formula in @n``
    Shows the mathematical formula of the fitted model.
    Some primitive simplifications are applied to the formula.
    To prevent it, put plus sign (+) after ``info``.

``info @n.dF(x)``
    Compares the symbolic and numerical derivatives in *x*
    (useful for debugging).

``info peaks in @n``
    Show parameters of functions from dataset *n*.
    With the plus sign (+) after ``info``, uncertainties of the
    parameters are also included.


The model can be exported to file as data points, using the syntax
described in :ref:`dexport`, or as mathematical formula,
using the ``info`` command redirected to a file::

   info[+] formula in @n > filename

.. _formula_export_style:

The style of the formula output,
governed by the :option:`formula-export-style` option,
can be either ``normal`` (exp(-x^2)) or ``gnuplot`` (exp(-x**2)).

The list of parameters of functions can be exported using the command::

    info[+] peaks in @n > filename

With ``@*`` formulae or parameters used in all datasets are written.

Fitting
=======

.. _nonlinear:

Nonlinear optimization
----------------------

This is the core. We have a set of observations (data points), to which
we want to fit a *model* that depends on adjustable parameters.
Let me quote *Numerical Recipes*,
chapter 15.0, page 656 (if you do not know the book, visit
http://www.nr.com):

    The basic approach in all cases is usually the same: You choose or design
    a figure-of-merit function (merit function, for short) that measures the
    agreement between the data and the model with a particular choice of
    parameters. The merit function is conventionally arranged so that small
    values represent close agreement. The parameters of the model are then
    adjusted to achieve a minimum in the merit function, yielding best-fit
    parameters.  The adjustment process is thus a problem in minimization in
    many dimensions.  \[...] however, there exist special, more
    efficient, methods that are specific to modeling, and we will discuss
    these in this chapter. There are important issues that go beyond the mere
    finding of best-fit parameters. Data are generally not exact. They are
    subject to measurement errors (called noise in the context of
    signal-processing). Thus, typical data never exactly fit the model that
    is being used, even when that model is correct. We need the means to
    assess whether or not the model is appropriate, that is, we need to test
    the goodness-of-fit against some useful statistical standard. We usually
    also need to know the accuracy with which parameters are determined by
    the data set.  In other words, we need to know the likely errors of the
    best-fit parameters. Finally, it is not uncommon in fitting data to
    discover that the merit function is not unimodal, with a single minimum.
    In some cases, we may be interested in global rather than local
    questions. Not, "how good is this fit?" but rather, "how
    sure am I that there is not a very much better fit in some corner of
    parameter space?"

Our function of merit is WSSR - the weighted sum of
squared residuals, also called chi-square:

.. math::
  \chi^{2}(\mathbf{a})
    =\sum_{i=1}^{N} \left[\frac{y_i-y(x_i;\mathbf{a})}{\sigma_i}\right]^{2}
    =\sum_{i=1}^{N} w_{i}\left[y_{i}-y(x_{i};\mathbf{a})\right]^{2}

Weights are based on standard deviations, :math:`w_i=1/\sigma_i^2`.
You can learn why squares of residuals are minimized e.g. from
chapter 15.1 of *Numerical Recipes*.

So we are looking for a global minimum of :math:`\chi^2`.
This field of numerical research (looking for a minimum or maximum)
is usually called optimization; it is non-linear and global optimization.
Fityk implements three very different optimization methods.
All are well-known and described in many standard textbooks.

The standard deviations of the best-fit parameters are given by the square
root of the corresponding diagonal elements of the covariance matrix.
The covariance matrix is based on standard deviations of data points.
Formulae can be found e.g. in
`GSL Manual <http://www.gnu.org/software/gsl/manual/>`_,
chapter *Linear regression. Overview* (weighted data version).

.. _fitting_cmd:

Fitting related commands
------------------------

To fit model to data, use command

fit[+] [number-of-iterations] [in @n ...]

The plus sign (+) prevents initialization of the fitting method.
It is used to continue the previous fitting where it left off.

All non-linear fitting methods are iterative.
*number-of-iterations* is the maximum number of iterations.
There are also other stopping criteria, so the number of executed
iterations can be smaller.

``fit in @*`` fits all datasets simultaneously.

Fitting methods can be set using the set command::

  set fitting-method = method

where method is one of: ``Levenberg-Marquardt``, ``Nelder-Mead-simplex``,
``Genetic-Algorithms``.

All non-linear fitting methods are iterative, and there are two common
stopping criteria:

- the number of iterations and it can be specified after the ``fit`` command.

- and the number of evaluations of the objective function (WSSR), specified
  by the value of option :option:`max-wssr-evaluations` (0=unlimited).
  It is approximately proportional to the time of computations.

There are also other criteria, different for each method.

On Unix, fitting can be interrupted by sending the `INT` signal to the program.
This is usually done by pressing Ctrl-C in the terminal.

If you give too small *number-of-iterations* to the command ``fit``,
and fit is not converged, it makes sense to use command ``fit+``
to process further iterations.

Setting ``set autoplot = on-fit-iteration``
will plot a model after every iteration, to visualize progress.
(see :ref:`autoplot <autoplot>`)

``info fit`` shows goodness-of-fit.

Available methods can be mixed together, e.g. it is sensible
to obtain initial parameter estimates using the Simplex method,
and then fit it using Levenberg-Marquardt.

Values of all parameters are stored before and after fitting (if they
change). This enables simple undo/redo functionality.
If in the meantime some functions or variables where added or removed,
the program can still load the old parameters, but the result can be
unexpected. The following history-related commands are provided:

fit undo
    move back to the previous parameters (undo fitting).

fit redo
    move forward in the parameter history

info fit-history
    show number of items in the history

fit history *n*
    load the *n*-th set of parameters from history

fit history clear
    clear the history

Uncertainty in the model parameters
-----------------------------------

From the book J. Wolberg, *Data Analysis Using the Method of Least Squares: Extracting the Most Information from Experiments*, Springer, 2006, p.50:

   (...) we turn to the task of determining the uncertainties associated
   with the :math:`a_k`'s. The usual measures of uncertainty are standard
   deviation (i.e., *σ*) or variance (i.e., *σ*:sup:`2`) so
   we seek an expression that allows us to estimate the :math:`\sigma_{a_k}`'s.
   It can be shown (...) that the following expression gives us an unbiased
   estimate of :math:`\sigma_{a_k}`:

.. math::
  \sigma_{a_k}^{2}=\frac{S}{n-p}C_{kk}^{-1}

Note that :math:`\sigma_{a_k}` is a square root of the value above.
In this formula *n-p*, the number of (active) data points minus the number
of independent parameters, is equal to the number of degrees of freedom.
*S* is another symbol for :math:`\chi^2` (the latter symbol is used e.g. in
*Numerical Recipes*).

Terms of the *C* matrix are given as (p. 47 in the same book):

.. math::
  C_{jk}=\sum_{i=1}^n w_i \frac{\partial f}{\partial a_j} \frac{\partial f}{\partial a_k}

:math:`\sigma_{a_k}` above is often called a *standard error*.
Having standard errors, it is easy to calculate confidence intervals.
Now another book will be cited: H. Motulsky and A. Christopoulos,
*Fitting Models to Biological Data Using Linear and Nonlinear Regression:
A Practical Guide to Curve Fitting*, Oxford University Press, 2004.
This book can be `downloaded for free`__ as a manual to GraphPad Prism 4.

__ http://www.graphpad.com/manuals/prism4/RegressionBook.pdf

   The standard errors reported by most nonlinear regression programs (...)
   are "approximate" or "asymptotic". Accordingly, the confidence intervals
   computed using these errors should also be considered approximate.

   It would be a mistake to assume that the "95% confidence intervals" reported
   by nonlinear regression have exactly a 95% chance of enclosing the true
   parameter values. The chance that the true value of the parameter is within
   the reported confidence interval may not be exactly 95%. Even so, the
   asymptotic confidence intervals will give you a good sense of how precisely
   you have determined the value of the parameter.

   The calculations only work if nonlinear regression has converged on a
   sensible fit. If the regression converged on a false minimum, then the
   sum-of-squares as well as the parameter values will be wrong, so the
   reported standard error and confidence intervals won’t be helpful.


The book describes also more accurate ways to calculate confidence intervals,
such use Monte Carlo simulations.


In Fityk:

* ``info errors`` shows values of :math:`\sigma_{a_k}`.
* ``info+ errors`` additionally shows the matrix *C*:sup:`--1`.
* Individual symmetric errors of simple-variables can be accessed as
  ``$variable.error`` or e.g. ``%func.height.error``.
* confidence intervals are on the TODO list (in the meantime you can compute
  them by hand, see p.103 in the GraphPad book)

.. note:: In Fityk 0.9.0 and earlier ``info errors`` reported values of
          :math:`\sqrt{C_{kk}^{-1}}`, which makes sense if the standard
          deviations of *y*'s are set accurately. This formula is derived
          in *Numerical Recipes*.
 
.. _levmar:

Levenberg-Marquardt
-------------------

This is a standard nonlinear least-squares routine, and involves
computing the first derivatives of functions.  For a description
of the L-M method see *Numerical Recipes*, chapter 15.5
or Siegmund Brandt, *Data Analysis*, chapter 10.15.
Essentially, it combines an inverse-Hessian method with a steepest
descent method by introducing a |lambda| factor. When |lambda| is equal
to 0, the method is equivalent to the inverse-Hessian method.
When |lambda| increases, the shift vector is rotated toward the direction
of steepest descent and the length of the shift vector decreases. (The
shift vector is a vector that is added to the parameter vector.) If a
better fit is found on iteration, |lambda| is decreased -- it is divided by
the value of :option:`lm-lambda-down-factor` option (default: 10).
Otherwise, |lambda| is multiplied by the value of
:option:`lm-lambda-up-factor` (default: 10).
The initial |lambda| value is equal to
:option:`lm-lambda-start` (default: 0.0001).

The Marquardt method has two stopping criteria other than the common
criteria.

- If it happens twice in sequence, that the relative
  change of the value of the objective function (WSSR) is smaller than
  the value of the :option:`lm-stop-rel-change` option, the
  fit is considered to have converged and is stopped.

- If |lambda| is greater than the value of the :option:`lm-max-lambda`
  option (default: 10^15), usually when due to limited numerical precision
  WSSR is no longer changing, the fitting is also stopped.

.. |lambda| replace:: *λ*

.. COMMENT: <para>
      L-M method finds a minimum quickly. The question is, if it is the
      global minimum.  It can be a good idea to add a small random vector to
      the vector of parameters and try again. This small shift vector is added,
      when value of <parameter class="option">shake-before</parameter> option
      is positive (by default it is 0). Value of every parameter's shift
      is independent and randomly drawn from distribution of type specified by
      value of <parameter class="option">shake-type</parameter> option
      (see <link linkend="distribtype">option
      <parameter class="option">distrib-type</parameter></link>)
      in simplex method). The expected value of parameter shift is
      directly proportional to both value of
      <parameter class="option">shake-before</parameter> option and width of
      parameter's domain.
      </para>

.. _nelder:

Nelder-Mead downhill simplex method
-----------------------------------

To quote chapter 4.8.3, p. 86 of Peter Gans,
*Data Fitting in the Chemical Sciences by the Method of Least Squares*:

    A simplex is a geometrical entity that has n+1 vertices corresponding to
    variations in n parameters.  For two parameters the simplex is a
    triangle, for three parameters the simplex is a tetrahedron and so forth.
    The value of the objective function is calculated at each of the
    vertices. An iteration consists of the following process. Locate the
    vertex with the highest value of the objective function and replace this
    vertex by one lying on the line between it and the centroid of the other
    vertices. Four possible replacements can be considered, which I call
    contraction, short reflection, reflection and expansion.[...]
    It starts with an arbitrary simplex. Neither the shape nor position of
    this are critically important, except insofar as it may determine which
    one of a set of multiple minima will be reached. The simplex than expands
    and contracts as required in order to locate a valley if one exists. Then
    the size and shape of the simplex is adjusted so that progress may be
    made towards the minimum. Note particularly that if a pair of
    parameters are highly correlated, *both* will be
    simultaneously adjusted in about the correct proportion, as the shape of
    the simplex is adapted to the local contours.[...]
    Unfortunately it does not provide estimates of the parameter errors, etc.
    It is therefore to be recommended as a method for obtaining initial
    parameter estimates that can be used in the standard least squares
    method.

This method is also described in previously mentioned
*Numerical Recipes* (chapter 10.4) and *Data Analysis* (chapter 10.8).

There are a few options for tuning this method. One of these is a
stopping criterium :option:`nm-convergence`. If the value of the
expression 2(*M*-*m*)/(*M*+*m*), where *M* and *m* are the values of the
worst and best vertices respectively (values of objective functions of
vertices, to be precise!), is smaller then the value of
:option:`nm-convergence` option, fitting is stopped. In other words,
fitting is stopped if all vertices are almost at the same level.

The remaining options are related to initialization of the simplex.
Before starting iterations, we have to choose a set of points in space
of the parameters, called vertices.  Unless the option
:option:`nm-move-all` is set, one of these points will be the current
point -- values that parameters have at this moment. All but this one
are drawn as follows: each parameter of each vertex is drawn separately.
It is drawn from a distribution that has its center in the center of the
:ref:`domain <domain>` of the parameter, and a width proportional to
both width of the domain and value of the :option:`nm-move-factor`
parameter.  Distribution shape can be set using the option
:option:`nm-distribution` as one of: ``uniform``, ``gaussian``,
``lorentzian`` and ``bound``. The last one causes the value of the
parameter to be either the greatest or smallest value in the domain of
the parameter -- one of the two bounds of the domain (assuming that
:option:`nm-move-factor` is equal 1).

Genetic Algorithms
------------------

\[TODO]

.. _settings:

Settings
========

.. note:: This chapter is not about GUI settings (things like colors,
   fonts, etc.), but about settings that are common for both CLI and GUI
   version.

Command ``info set`` shows the syntax of the set command and lists all
possible options.

``set option`` shows the current value of the *option*.

``set option = value`` changes the *option*.

It is possible to change the value of the option temporarily using syntax::

    with option1=value1 [,option2=value2]  command args...

The examples at the end of this chapter should clarify this.

autoplot
    See :ref:`autoplot <autoplot>`.

can-cancel-guess
    See :ref:`guess`.

cut-function-level
    See :ref:`speed`.

data-default-sigma
    See :ref:`weights`.

.. _epsilon:

epsilon
    It is used for floating-point comparison:
    a and b are considered equal when
    \|a-b|<:option:`epsilon`.
    You may want to decrease it when you work with very small values,
    like 10^-10.

exit-on-warning
    If the option :option:`exit-on-warning`
    is set, any warning will close the program.
    This ensures that no warnings can be overlooked.

fitting-method
    See :ref:`fitting_cmd`.

formula-export-style
    See :ref:`details in the section "Model" <formula_export_style>`.

guess-at-center-pm
    See :ref:`guess`.

height-correction
    See :ref:`guess`.

.. _info_numeric_format:

info-numeric-format
    Format of numbers printed by the ``info`` command. It takes as a value
    a format string, the same as ``sprintf()`` in the C language.
    For example ``set info-numeric-format=%.3f`` changes the precision
    of numbers to 3 digits after the decimal point. Default value: ``%g``.

lm-*
    Setting to tune :ref:`Levenberg-Marquardt <levmar>`
    fitting method.

max-wssr-evaluations
    See :ref:`fitting_cmd`.

nm-*
    Setting to tune
    :ref:`Nelder-Mead downhill simplex <nelder>`
    fitting method.

pseudo-random-seed
    Some fitting methods and functions, such as
    ``randnormal`` in data expressions use a pseudo-random
    number generator.  In some situations one may want to have repeatable
    and predictable results of the fitting, e.g.  to make a presentation.
    Seed for a new sequence of pseudo-random numbers can be set using the
    option :option:`pseudo-random-seed`.  If it
    is set to 0, the seed is based on the current time and a sequence of
    pseudo-random numbers is different each time.

refresh-period
    During time-consuming computations (like fitting) user interface can
    remain not changed for this time (in seconds).
    This option was introduced, because on one hand frequent refreshing of
    the program's window notably slows down fitting, and on the other hand
    irresponsive program is a frustrating experience.

variable-domain-percent
    See :ref:`the section about variables <domain>`.

verbosity
    Possible values: quiet, normal, verbose, debug.

width-correction
    See :ref:`guess`.

Examples::

    set fitting-method  # show info
    set fitting-method = Nelder-Mead-simplex # change default method
    set verbosity = verbose
    with fitting-method = Levenberg-Marquardt fit 10
    with fitting-method=Levenberg-Marquardt, verbosity=only-warnings fit 10

Other commands
==============

plot: viewing data
------------------

In the GUI version there is hardly ever a need to use this command directly.

The command ``plot`` controls visualization of data and the model.
It is used to plot a given area - in GUI it is plotted
in the program's main window, in CLI the popular program
gnuplot is used, if available. ::

   plot xrange yrange in @n

*xrange* and *yrange* have one of two following syntaxes:

- ``[min:max]``

-  ``.``

The second is just a dot (.), and it implies that the appropriate range
is not to be changed.

Examples::

    plot [20.4:50] [10:20] # show x from 20.4 to 50 and y from 10 to 20
    plot [20.4:] # x from 20.4 to the end,
    # y range will be adjusted to encompass all data
    plot . [:10] # x range will not be changed, y from the lowest point to 10
    plot [:] [:] # all data will be shown
    plot         # all data will be shown
    plot . .     # nothing changes

.. _autoplot:

The value of the option :option:`autoplot`
changes the automatic plotting behaviour. By default, the plot is
refreshed automatically after changing the data or the model.
It is also possible to visualize each iteration of the fitting method by
replotting the peaks after every iteration.

.. _info:

info: show information
----------------------

First, there is an option :option:`verbosity`
(not related to command :command:`info`)
which sets the amount of messages displayed when executing commands.

If you are using the GUI, most information can be displayed
with mouse clicks. Alternatively, you can use the ``info``
command. Using the ``info+`` instead of ``info``
sometimes displays more detailed information.

The output of :command:`info` can be redirected to a file using syntax::

  info args > filename    # this truncates the file

  info args >> filename   # this appends to the file

The following ``info`` arguments are recognized:

+ variables

+ *$variable_name*

+ types

+ *TypeName*

+ functions

+ *%function_name*

+ datasets

+ data \[in @\ *n*]

+ title \[in @\ *n*]

+ filename \[in @\ *n*]

+ commands

+ commands \[n:m]

+ view

+ set

+ fit \[in @\ *n*]

+ fit-history

+ errors \[in @\ *n*]

+ formula \[in @\ *n*]

+ peaks \[in @\ *n*]

+ guess \[x-range] \[in @\ *n*]

+ *data-expression* [in @\ *n*]

+ [@\ *n*.]F

+ [@\ *n*.]Z

+ [@\ *n*.]dF(*data-expression*)

+ der *mathematic-function*

+ version

``info der`` shows derivatives of given function::

    =-> info der sin(a) + 3*exp(b/a)
    f(a, b) = sin(a)+3*exp(b/a)
    df / d a = cos(a)-3*exp(b/a)*b/a^2
    df / d b = 3*exp(b/a)/a


commands, dump, sleep, reset, quit, !
-------------------------------------

All commands given during program execution are stored in memory.
They can be listed by::

   info commands [n:m]

or written to file::

   info commands [n:m] > filename

To put all commands executed so far during the session into the
file :file:`foo.fit`, type::

   info commands[:] > foo.fit

With the plus sign (+) (i.e. ``info+ commands [n:m]``)
information about the exit status of each command will be added.

To log commands to a file when they are executed, use:
Commands can be logged when they are executed::

   commands > filename    # log commands
   commands+ > filename   # log both commands and output
   commands > /dev/null   # stop logging

Scripts can be executed using the command::

   commands < filename

You can select lines that are to be executed::

   commands < filename[m:n] # this executes lines from m to n

It is also possible to execute standard output from an external program::

   commands ! program [args...]

The command::

   dump > filename

writes the current state of the program
(including all datasets) to a single .fit file.

The command ``sleep sec`` makes the program wait *sec* seconds
before continuing.

The command ``quit`` works as expected.
If it is found in a script it quits the program, not only the script.

Commands that start with ``!`` are passed (without '!')
to the ``system()`` call.

..
  $Id$ 

