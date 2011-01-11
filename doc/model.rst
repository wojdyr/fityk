
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

Some fitting algorithms randomize the parameters of the model
(i.e. they randomize simple variables).
For this purpose, the simple variable can have a specified :dfn:`domain`.
Note that the domain does not imply any constraints on the value
the variable can have -- it is only a hint for fitting algorithms.
Domains are used by Nelder-Mead method and Genetic Algorithms.
The syntax is as follows::

    $a = ~12.3 [11 +- 5] # center and width of the domain are given
    $b = ~12.3 [ +- 5] # if the center of the domain is not specified,
                       # the value of the variable is used

If the domain is not specified, the value of
:option:`variable_domain_percent` option is used
(domain is +/- *value-of-variable* * :option:`variable_domain_percent` / 100)

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


.. _flist:

Built-in functions
------------------

The list of all functions can be obtained using
``i+ types``. Some formulae here have long parameter
names (like "height", "center" and "hwhm") replaced with
:math:`a_i`

**Gaussian:**

.. math::
   y = a_0
       \exp\left[-\ln(2)\left(\frac{x-a_1}{a_2}\right)^{2}\right]

**SplitGaussian:**

.. math:: 
   y(x;a_0,a_1,a_2,a_3) = \begin{cases}
   \textrm{Gaussian}(x;a_0,a_1,a_2) & x\leq a_1\\
   \textrm{Gaussian}(x;a_0,a_1,a_3) & x>a_1\end{cases}

**GaussianA:**

.. math:: 
   y = \sqrt{\frac{\ln(2)}{\pi}}\frac{a_0}{a_2}
       \exp\left[-\ln(2)\left(\frac{x-a_1}{a_2}\right)^{2}\right]

**Lorentzian:**

.. math:: 
   y = \frac{a_0}{1+\left(\frac{x-a_1}{a_2}\right)^2}

**SplitLorentzian:**

.. math:: 
   y(x;a_0,a_1,a_2,a_3) = \begin{cases}
   \textrm{Lorentzian}(x;a_0,a_1,a_2) & x\leq a_1\\
   \textrm{Lorentzian}(x;a_0,a_1,a_3) & x>a_1\end{cases}

**LorentzianA:**

.. math:: 
   y = \frac{a_0}{\pi a_2\left[1+\left(\frac{x-a_1}{a_2}\right)^2\right]}

**Pearson VII (Pearson7):**

.. math:: 
   y = \frac{a_0} {\left[1+\left(\frac{x-a_1}{a_2}\right)^2
                           \left(2^{\frac{1}{a_3}}-1\right)\right]^{a_3}}

**split Pearson VII (SplitPearson7):**

.. math:: 
   y(x;a_{0},a_{1},a_{2},a_{3},a_{4},a_{5}) = \begin{cases}
    \textrm{Pearson7}(x;a_0,a_1,a_2,a_4) & x\leq a_1\\
    \textrm{Pearson7}(x;a_0,a_1,a_3,a_5) & x>a_1\end{cases}

**Pearson VII Area (Pearson7A):**

.. math:: 
   y = \frac{a_0\Gamma(a_3)\sqrt{2^{\frac{1}{a_3}}-1}}
            {a_2\Gamma(a_3-\frac{1}{2})\sqrt{\pi} \left[
               1 + \left(\frac{x-a_1}{a_2}\right)^2
                   \left(2^{\frac{1}{a_3}}-1\right)
            \right]^{a_3}}

**Pseudo-Voigt (PseudoVoigt):**

.. math:: 
   y = a_0 \left[(1-a_3)\exp\left(-\ln(2)\left(\frac{x-a_1}{a_2}\right)^2\right)
                 + \frac{a_3}{1+\left(\frac{x-a_1}{a_2}\right)^2}
           \right]

Pseudo-Voigt is a name given to the sum of Gaussian and Lorentzian.
:math:`a_3` parameters in Pearson VII and Pseudo-Voigt
are not related.

**split Pseudo-Voigt (SplitPseudoVoigt):**

.. math:: 
   y(x;a_{0},a_{1},a_{2},a_{3},a_{4},a_{5}) = \begin{cases}
    \textrm{PseudoVoigt}(x;a_0,a_1,a_2,a_4) & x\leq a_1\\
    \textrm{PseudoVoigt}(x;a_0,a_1,a_3,a_5) & x>a_1\end{cases}

**Pseudo-Voigt Area (PseudoVoigtA):**

.. math:: 
   y = a_0 \left[\frac{(1-a_3)\sqrt{\ln(2)}}{a_2\sqrt{\pi}}
                 \exp\left(-\ln2\left(\frac{x-a_1}{a_2}\right)^2\right)
                 + \frac{a_3}{\pi a_2
                              \left[1+\left(\frac{x-a_1}{a_2}\right)^2\right]}
           \right]

**Voigt:**

.. math:: 
   y = \frac
       {a_0 \int_{-\infty}^{+\infty}
                \frac{\exp(-t^2)}{a_3^2+(\frac{x-a_1}{a_2}-t)^2} dt}
       {\int_{-\infty}^{+\infty}
                \frac{\exp(-t^2)}{a_3^2+t^2} dt}

The Voigt function is a convolution of Gaussian and Lorentzian functions.
:math:`a_0` = heigth,
:math:`a_1` = center,
:math:`a_2` is proportional to the Gaussian width, and
:math:`a_3` is proportional to the ratio of Lorentzian and Gaussian widths.

Voigt is computed according to R.J.Wells,
*Rapid approximation to the Voigt/Faddeeva function and its derivatives*,
Journal of Quantitative Spectroscopy & Radiative Transfer
62 (1999) 29-48.
(See also: http://www.atm.ox.ac.uk/user/wells/voigt.html).
The approximation is very fast, but not very exact.

FWHM is estimated using the approximation by Olivero and Longbothum
(`JQSRT 17, 233 (1977)`__):
:math:`0.5346 w_L + \sqrt{0.2169 w_L^2 + w_G^2}`.

__ http://dx.doi.org/10.1016/0022-4073(77)90161-3

**VoigtA:**

.. math:: 
   y = \frac{a_0}{\sqrt{\pi}a_2}
       \int_{-\infty}^{+\infty}
           \frac{\exp(-t^2)}{a_3^2+(\frac{x-a_1}{a_2}-t)^2} dt

**Exponentially Modified Gaussian (EMG):**

.. math:: 
   y = \frac{ac\sqrt{2\pi}}{2d}
       \exp\left(\frac{b-x}{d}+\frac{c^2}{2d^2}\right)
       \left[\frac{d}{\left|d\right|}
             -\textrm{erf}\left(\frac{b-x}{\sqrt{2}c}
                                + \frac{c}{\sqrt{2}d}\right)
       \right]

**LogNormal:**

.. math::
   y = h \exp\left\{ -\ln(2) \left[
                                   \frac{\ln\left(1+2b\frac{x-c}{w}\right)}{b}
                            \right]^{2} \right\}

**Doniach-Sunjic (DoniachSunjic):**

.. math:: 
   y = \frac{h\left[\frac{\pi a}{2} 
                    + (1-a)\arctan\left(\frac{x-E}{F}\right)\right]}
            {F+(x-E)^2}

**Polynomial5:**

.. math:: 
   y = a_0 + a_1 x +a_2 x^2 + a_3 x^3 + a_4 x^4 + a_5 x^5


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
         See the section :ref:`invoking` for details.

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
set the option :option:`cut_function_level`
to a non-zero value. For each function the range with values
greater than :option:`cut_function_level`
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
  +/- value of the option :option:`guess_at_center_pm`.

Fityk offers only a primitive algorithm for peak-detection.
It looks for the highest point in a given range, and than tries
to find the width of the peak.

If the highest point is found near the boundary of the given range,
it is very probable that it is not the peak top,
and, if the option :option:`can_cancel_guess` is set to true,
the guess is cancelled.

There are two real-number options related to ``guess``:
:option:`height_correction` and :option:`width_correction`.
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
governed by the :option:`formula_export_style` option,
can be either ``normal`` (exp(-x^2)) or ``gnuplot`` (exp(-x**2)).

The list of parameters of functions can be exported using the command::

    info[+] peaks in @n > filename

With ``@*`` formulae or parameters used in all datasets are written.

