.. _model:

Models
======

From `Numerical Recipes <http://www.nrbook.com/a/bookcpdf.php>`_,
chapter 15.0:

    Given a set of observations, one often wants to condense and summarize
    the data by fitting it to a "model" that depends on adjustable
    parameters. Sometimes the model is simply a convenient class of
    functions, such as polynomials or Gaussians, and the fit supplies the
    appropriate coefficients. Other times, the model's parameters come
    from some underlying theory that the data are supposed to satisfy;
    examples are coefficients of rate equations in a complex network of
    chemical reactions, or orbital elements of a binary star. Modeling can
    also be used as a kind of constrained interpolation, where you want to
    extend a few data points into a continuous function, but with some
    underlying idea of what that function should look like.

This chapter shows how to construct the model.

Complex models are often a sum of many functions. That is why in Fityk
the model *F* is constructed by summing a list of component functions: 
:math:`F = \sum_i f_i`.

Each component function :math:`f_i` is of a predefined type,
such as Gaussian or polynomial.
This is not a limitation, because the user can define new function type and add it 
to the predefined types.

To avoid confusion, the name *function* will be used only when referring
to a component function, not when when referring to the sum (model),
which mathematically is also a function. 
The *function types* will sometimes be called "predefined functions".

Function :math:`f_i=f_i(x; \boldsymbol{a})` is a function of *x*,
and depends on a vector of parameters :math:`\boldsymbol{a}`.
The parameters :math:`\boldsymbol{a}` will be fitted to achieve agreement
of the model and data.

In experiments we often have the situation that the measured *x* values
are subject to systematic errors caused, for example, by instrumental
zero shift or, in powder diffraction measurements,
by displacement of sample in the instrument.
If this is the case, such errors should be a part of the model.
In Fityk, this part of the model is called :dfn:`x-correction`.
The final formula for the model is:

.. _model_formula:

.. math::
    F(x; \boldsymbol{a}) = \sum_i f_i(x+Z(x; \boldsymbol{a}); \boldsymbol{a})

where :math:`Z(x; \boldsymbol{a}) = \sum_i z_i(x; \boldsymbol{a})`
is the *x*-correction. *Z* is constructed as a sum of components,
analogously to *F*, although in practice it has rarely more than
one component.

Each component function is created by specifying a *function type*
and binding *variables* to type's parameters. 

The next section explains
what are *variables* in Fityk, and then we get back to functions.

.. _variables:

Variables
---------

Variables have names prefixed with the dollar symbol ($)
and are created by assigning a value::

   $foo=~5.3           # simple-variable
   $bar=5*sin($foo)    # compound-variable
   $c=3.1              # constant (the simplest compound-variable)

Prefixing the number with the tilde (~) means that the variable will be adjusted/fitted when the model
is fitted to the data. Otherwise it is constant/frozen.
Variable created by assigning ``~`` followed by *number* or *{expression}* 
(like ``$foo`` in the example above) will be called a :dfn:`simple-variable`.

All other variables are called :dfn:`compound-variables`.
Compound variables either depend on other variables (``$bar`` above)
or are constant (``$c``).

.. important::

  Unlike in popular programming languages, variable can store either a single
  numeric (floating-point) value or a variable expression (dependency). Nothing else.
  In case of variable expression, if we define ``$b=2*$a``
  the value of ``$b`` will be recalculated every time ``$a`` changes.

To assign a value (constant) of another simple variable, use:
``$b={$a}``. Braces return the current value of the enclosed expression.
The left brace can be preceded by the tilde (``~``).
The assignment ``$b=~{$a}`` creates a simple variable.

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

The braces may contain any data expression::

    $x0 = {x[0]}
    $min_y = {min(y if a)}
    $c = {max2($a, $b)}
    $t = {max(x) < 78 ? $a : $b}

Sometimes it is useful to freeze a variable, i.e. to prevent it from
changing while fitting::

    $a = ~12.3 # $a is fittable (simple-variable)
    $a = {$a}  # $a is not fittable (constant)
    $a = ~{$a} # $a is fittable (simple-variable) again

.. admonition:: In the GUI

   a variable can be switched between constant and simple-variable
   by clicking the padlock button on the sidebar.
   The icons |open-lock-icon| and |lock-icon|
   show that the variable is fittable and frozen, respectively.

.. |open-lock-icon| image:: img/open_lock_icon.png
   :alt: open lock

.. |lock-icon| image:: img/lock_icon.png
   :alt: lock


If the assigned expression contains tildes but is not just ``~number``::

  $bleh=~9.1*exp(~2)

it automatically creates simple-variables corresponding
to the tilde-prefixed numbers and defines the left-hand-side as a compound variable as depending on them.
Automatically created variables are named ``$_1``, ``$_2``, ``$_3``, and so on.
In the example above two simple-variables (with values 9.1 and 2) are created and, if the names ``$_1``, ``$_2`` are not already taken, it defines ``$bleh=$_1*exp($_2)``.


Variables can be deleted using the command::

   delete $variable


.. _domain:

Domains
-------

Simple-variables may have a :dfn:`domain`,
which is used for two things when fitting.

Most importantly, fitting methods that support bound constraints
use the domain as lower and/or upper bounds.
See the section :ref:`bound_constraints` for details.

The other use is for randomizing parameters (simple-variables) of the model.
Methods that stochastically initialize or modify parameters
(usually generating a set of initial points) need well-defined
domains (minimum and maximum values for parameters) to work effectively.
Such methods include Nelder-Mead simplex and Genetic Algorithms,
but not the default Lev-Mar method, so in most cases you
do not need to worry about it.

The syntax is as follows::

    $a = ~12.3 [0:20] # initial values are drawn from the (0, 20) range
    $a = ~12.3 [0:]   # only lower bound
    $a = ~12.3 [:20]  # only upper bound
    $a = ~15.0        # domain stays the same
    $a = ~15.0 []     # remove the domain
    $a = ~{$a} [0:20] # domain is set again

If the domain is not specified but it is required (for the latter use)
by the fitting method, we assume it to be ±\ *p*\ % of the current value,
where *p* can be set using the :option:`domain_percent` option.

Function Types and Functions
----------------------------

Function types have names that start with upper case letter
(``Linear``, ``Voigt``).

Functions have names prefixed with the percent symbol (``%func``).
Every function has a type and variables bound to its parameters.
One way to create a function is to specify both function type, and the variables or their values::

   %f1 = Gaussian(~66254., ~24.7, ~0.264)
   %f2 = Gaussian(~6e4, $ctr, $b+$c)
   %f3 = Gaussian(height=~66254., hwhm=~0.264, center=$h)

Every expression which is valid on the right-hand side of a variable
assignment can be used as the value of a parameter-bound variable.
If the expression is not just a name of a variable, an automatic variable is created.
In the above examples, two variables were implicitely created for ``%f2``:
first for value ``6e4`` and the second for ``$b+$c``. Another 2 for %f3.

If the names of function's parameters are given (like for ``%f3`` above),
the variables (or their values) can be given in any order.

Function types can can have specified default values for
some parameters. The variables for such parameters can be omitted; 
but if so, the non-omitted variables must specify their parameter names
e.g.::

   =-> i Pearson7
   Pearson7(height, center, hwhm, shape=2) = height/(1+((x-center)/hwhm)^2*(2^(1/shape)-1))^shape
   =-> %f4 = Pearson7(height=~66254., center=~24.7, hwhm=~0.264) # no shape is given
   New function %f4 was created.

Functions can be copied. The following command creates a deep copy
(i.e. all variables are also duplicated) of %foo::

   %bar = copy(%foo)

Functions can be also created (and also added to the model F) with the command ``guess``, 
as described in :ref:`guess`.

Variables bound to the function parameters can be replaced at any time::

    =-> %f = Pearson7(height=~66254., center=~24.7, hwhm=~0.264)
    New function %f was created.
    =-> info %f
    %f = Pearson7($_1, $_2, $_3, $_4)
    =-> %f.center=~24.8
    =-> $h = ~66254
    =-> %f.height=$h
    =-> info %f
    %f = Pearson7($h, $_5, $_3, $_4)
    =-> info $_1
    Error: undefined variable: $_1   # it's been automatically deleted
    =-> $h = ~60000 # variables are kept by name, so this change affects %f
    
    =-> %p1.center = %p2.center + 3 # keep fixed distance between %p1 and %p2

Functions can be deleted using the command::

   delete %function


.. _flist:

Built-In Function Types
-----------------------

The list of all types of functions can be obtained using ``i types``.
Some formulae here have long parameter names
(like "height", "center" and "hwhm") replaced with :math:`a_i`

:ftype:`Gaussian`:

.. math::
   y = a_0
       \exp\left[-\ln(2)\left(\frac{x-a_1}{a_2}\right)^{2}\right]

:math:`a_2` here is half width at half maximum (HWHM=FWHM/2,
where FWHM stands for full width...), which is proportional to the standard
deviation: :math:`a_2=\sqrt{2\ln2}\sigma`.

:ftype:`SplitGaussian`:

.. math:: 
   y(x;a_0,a_1,a_2,a_3) = \begin{cases}
   \textrm{Gaussian}(x;a_0,a_1,a_2) & x\leq a_1\\
   \textrm{Gaussian}(x;a_0,a_1,a_3) & x>a_1\end{cases}

:ftype:`GaussianA`:

.. math:: 
   y = \sqrt{\frac{\ln(2)}{\pi}}\frac{a_0}{a_2}
       \exp\left[-\ln(2)\left(\frac{x-a_1}{a_2}\right)^{2}\right]

:ftype:`Lorentzian`:

.. math:: 
   y = \frac{a_0}{1+\left(\frac{x-a_1}{a_2}\right)^2}

:ftype:`SplitLorentzian`:

.. math:: 
   y(x;a_0,a_1,a_2,a_3) = \begin{cases}
   \textrm{Lorentzian}(x;a_0,a_1,a_2) & x\leq a_1\\
   \textrm{Lorentzian}(x;a_0,a_1,a_3) & x>a_1\end{cases}

:ftype:`LorentzianA`:

.. math:: 
   y = \frac{a_0}{\pi a_2\left[1+\left(\frac{x-a_1}{a_2}\right)^2\right]}

:ftype:`Pearson VII (Pearson7)`:

.. math:: 
   y = \frac{a_0} {\left[1+\left(\frac{x-a_1}{a_2}\right)^2
                           \left(2^{\frac{1}{a_3}}-1\right)\right]^{a_3}}

:ftype:`split Pearson VII (SplitPearson7)`:

.. math:: 
   y(x;a_{0},a_{1},a_{2},a_{3},a_{4},a_{5}) = \begin{cases}
    \textrm{Pearson7}(x;a_0,a_1,a_2,a_4) & x\leq a_1\\
    \textrm{Pearson7}(x;a_0,a_1,a_3,a_5) & x>a_1\end{cases}

:ftype:`Pearson VII Area (Pearson7A)`:

.. math:: 
   y = \frac{a_0\Gamma(a_3)\sqrt{2^{\frac{1}{a_3}}-1}}
            {a_2\Gamma(a_3-\frac{1}{2})\sqrt{\pi} \left[
               1 + \left(\frac{x-a_1}{a_2}\right)^2
                   \left(2^{\frac{1}{a_3}}-1\right)
            \right]^{a_3}}

:ftype:`Pseudo-Voigt (PseudoVoigt)`:

.. math:: 
   y = a_0 \left[(1-a_3)\exp\left(-\ln(2)\left(\frac{x-a_1}{a_2}\right)^2\right)
                 + \frac{a_3}{1+\left(\frac{x-a_1}{a_2}\right)^2}
           \right]

Pseudo-Voigt is a name given to the sum of Gaussian and Lorentzian.
:math:`a_3` parameters in Pearson VII and Pseudo-Voigt
are not related.

:ftype:`split Pseudo-Voigt (SplitPseudoVoigt)`:

.. math:: 
   y(x;a_{0},a_{1},a_{2},a_{3},a_{4},a_{5}) = \begin{cases}
    \textrm{PseudoVoigt}(x;a_0,a_1,a_2,a_4) & x\leq a_1\\
    \textrm{PseudoVoigt}(x;a_0,a_1,a_3,a_5) & x>a_1\end{cases}

:ftype:`Pseudo-Voigt Area (PseudoVoigtA)`:

.. math:: 
   y = a_0 \left[\frac{(1-a_3)\sqrt{\ln(2)}}{a_2\sqrt{\pi}}
                 \exp\left(-\ln2\left(\frac{x-a_1}{a_2}\right)^2\right)
                 + \frac{a_3}{\pi a_2
                              \left[1+\left(\frac{x-a_1}{a_2}\right)^2\right]}
           \right]

:ftype:`Voigt`:

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
The approximation is very fast, but not very exact.

FWHM is estimated using an approximation called *modified Whiting*
(`Olivero and Longbothum, 1977, JQSRT 17, 233`__):
:math:`0.5346 w_L + \sqrt{0.2169 w_L^2 + w_G^2}`,
where :math:`w_G=2\sqrt{\ln(2)} |a_2|, w_L=2 |a_2| a_3`.

__ http://dx.doi.org/10.1016/0022-4073(77)90161-3

:ftype:`VoigtA`:

.. math:: 
   y = \frac{a_0}{\sqrt{\pi}a_2}
       \int_{-\infty}^{+\infty}
           \frac{\exp(-t^2)}{a_3^2+(\frac{x-a_1}{a_2}-t)^2} dt

:ftype:`split Voigt (SplitVoigt)`:

.. math::
   y(x;a_{0},a_{1},a_{2},a_{3},a_{4},a_{5}) = \begin{cases}
    \textrm{Voigt}(x;a_0,a_1,a_2,a_4) & x\leq a_1\\
    \textrm{Voigt}(x;a_0,a_1,a_3,a_5) & x>a_1\end{cases}

:ftype:`Exponentially Modified Gaussian (EMG)`:

.. math:: 
   y = \frac{ac\sqrt{2\pi}}{2d}
       \exp\left(\frac{c^2}{2d^2}-\frac{x-b}{d}\right)
       \left[\frac{d}{\left|d\right|}
             +\textrm{erf}\left(\frac{x-b}{\sqrt{2}c}
                                - \frac{c}{\sqrt{2}d}\right)
       \right]

The exponentially modified Gaussian is a convolution of Gaussian and
exponential probability density.
*a* = Gaussian heigth,
*b* = location parameter (Gaussian center),
*c* = Gaussian width,
*d* = distortion parameter (a.k.a. modification factor or time constant).

:ftype:`LogNormal`:

.. math::
   y = h \exp\left\{ -\ln(2) \left[
                                   \frac{\ln\left(1+2b\frac{x-c}{w}\right)}{b}
                            \right]^{2} \right\}

:ftype:`Doniach-Sunjic (DoniachSunjic)`:

.. math:: 
   y = \frac{h\left[\frac{\pi a}{2} 
                    + (1-a)\arctan\left(\frac{x-E}{F}\right)\right]}
            {F+(x-E)^2}

:ftype:`Polynomial5`:

.. math:: 
   y = a_0 + a_1 x +a_2 x^2 + a_3 x^3 + a_4 x^4 + a_5 x^5

:ftype:`Sigmoid`:

.. math::
   y = L + \frac{U-L}{1+\exp\left(-\frac{x-x_{mid}}{w}\right)}

:ftype:`FCJAsymm`:

Axial asymmetry peak shape in the Finger, Cox and Jephcoat model, see
`J. Appl. Cryst. (1994) 27, 892 <http://dx.doi.org/10.1107/S0021889894004218>`_
and `J. Appl. Cryst. (2013) 46, 1219
<http://dx.doi.org/10.1107/S0021889813016233>`_.

Variadic Function Types
-----------------------

*Variadic* function types have variable number of parameters.
Two variadic function types are defined::

    Spline(x1, y1, x2, y2, ...)
    Polyline(x1, y1, x2, y2, ...)

This example::

    %f = Spline(22.1, 37.9, 48.1, 17.2, 93.0, 20.7)

creates a function that is a *natural cubic spline* interpolation
through points (22.1, 37.9), (48.1, 17.2), ....

The ``Polyline`` function is a polyline interpolation (spline of order 1).

Both ``Spline`` and ``Polyline`` functions are primarily used
for the manual baseline subtraction via the GUI.

The derivatives of Spline function are not calculated, so this function
is not refined by the default, derivative-based fitting algorithm.

Since the Polyline derivatives are calculated, it is possible to perform
weighted least squares approximation by broken lines, although
non-linear fitting algorithms are not optimal for this task.

.. _udf:

User-Defined Function Types (UDFT)
----------------------------------

User-defined function types can be added using command ``define``,
and then used in the same way as built-in function types. They are added to the GUI as well.

Example::

   define MyGaussian(height, center, hwhm) = height*exp(-ln(2)*((x-center)/hwhm)^2)

- The name of new type must start with an upper-case letter,
  contain only letters and digits and have at least two characters.

- The name of the type is followed by parameter names in brackets.

- Parameter name must start with lowercase letter and,
  contain only lowercase letters, digits and the underscore ('_').

- The name "x" is reserved, do not put it into parameter list,
  just use it on the right-hand side of the definition.

- There are special names of parameters that Fityk understands:

  * if the functions is peak-like (bell-shaped):
    ``height``, ``center``, ``hwhm``, ``area``,

  * if the functions is S-shaped (sigmoidal) or step-like:
    ``lower``, ``upper``, ``xmid``, ``wsig``,

  * if the function is more like linear:
    ``slope``, ``intercept``, ``avgy``.

  The initial values of these parameters can be guessed from data (see command ``info guess`` for what values they get).
  ``hwhm`` means half width at half maximum,
  the other names are self-explaining.

- Each parameter may have a default value (see the examples below).
  The default value can be either a number, or an expression that depends
  on the parameters listed above (e.g. ``0.8*hwhm``).
  The default value always binds a simple-variable to the parameter.

UDFTs can be defined in a few ways:

- by giving a full formula, like in the example above,

- as a :dfn:`re-parametrization` of existing function
  (see the ``GaussianArea`` example below),

- as a sum of already defined functions
  (see the ``GLSum`` example below),

- as a split (bifurcated) function:
  ``x <`` *expression* ``?`` *Function1(...)* ``:`` *Function2(...)*
  (see the ``SplitL`` example below).

When giving a full formula, the right-hand side of the equality sign
is similar to the :ref:`definiton of variable <variables>`,
but the formula can also depend on *x*.
Hopefully the examples can make the syntax clear::

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

There is a simple substitution mechanism that makes writing complicated
functions easier.
Substitutions must be assigned in the same line, after the keyword ``where``.

Example::

    define ReadShockley(sigma0=1, a=1) = sigma0 * t * (a - ln(t)) where t=x*pi/180

    # more complicated example, with nested substitutions
    define FullGBE(k, alpha) = k * alpha * eta * (eta / tanh(eta) - ln (2*sinh(eta))) where eta = 2*pi/alpha * sin(theta/2), theta=x*pi/180

.. admonition:: How it works internally

    The formula is parsed,
    derivatives of the formula are calculated symbolically,
    expressions are simplified
    and bytecode for virtual machine (VM) is created.

    When fitting, the VM calculates the value of the function
    and derivatives for every point.

Defined functions can be undefined using command ``undefine``::

    undefine GaussianArea

It is common to add own definitions to the :file:`init` file.
See the section :ref:`invoking` for details.

.. _function_cutoff:

Cutoff
------

With default settings, the value of every function is calculated
at every point. Peak functions, such as Gaussian, often have non-negligible
values only in a small fraction of all points,
so if you have many narrow peaks
(like `here <http://commons.wikimedia.org/wiki/File:Diff_NaBr.png>`_),
the basic optimization is to calculate values of each peak function
only near the function's center.
If the option :option:`function_cutoff` is set to a non-zero value,
each function is evaluated only in the range where its values are
greater than the :option:`function_cutoff`.

This optimization is supported only by some built-in functions.

Model, F and Z
--------------

As already discussed, each dataset has a separate model
that can be fitted to the data.
As can be seen from the :ref:`formula <model_formula>` at the beginning
of this chapter, the model is determined by a set functions :math:`f_i`
and a set of functions :math:`z_i`.
The sum of them are named *F* and *Z* respectively.
The model is constructed by specifying names of functions in those two sets.

In many cases :dfn:`x-correction` Z is not used.
The fitted curve is thus F.

Command::

   F += %function

adds  *%function* to the sum F, and

::

   Z += %function

adds *%function* to the sum Z.

A few examples::

    # create and add function to F
    %g = Gaussian(height=~66254., hwhm=~0.264, center=~24.7)
    F += %g

    # create unnamed function and add it to F
    F += Gaussian(height=~66254., hwhm=~0.264, center=~24.7)

    # clear F
    F = 0

    # clear F and add three functions in it
    F = %a + %b + %c

    # show info about the first and the last function in F
    info F[0], F[-1]

The next sections shows an easier way to add a function (command ``guess``).

If there is more than one dataset, F and Z can be prefixed
with the dataset number (e.g. ``@1.F``).

The model can be copied. To copy the model from ``@0`` to ``@1``
we type one of the two commands::

    @1.F = @0.F        # shallow copy
    @1.F = copy(@0.F)  # deep copy

The former command uses the same functions and variables in both models: if you shift
a peak in ``@1``, it will be also shifted in ``@0``. The latter command
(deep copy) duplicates all functions and variables and makes an independent
model.

.. admonition:: In the GUI

   click the button |copyfunc-icon| on the sidebar to make a deep copy.

.. |copyfunc-icon| image:: img/copyfunc_icon.png
   :alt: Copy-Model
   :class: icon

It is often required to keep the width or shape of peaks constant
for all peaks in the dataset. To change the variables bound to parameters
with a given name *param* for all functions in F, use the command::

   F[*].param = variable

Examples::

    # Set hwhm of all functions in F that have a parameter hwhm to $foo
    # (hwhm here means half-width-at-half-maximum)
    F[*].hwhm = $foo

    # Bound the variable used for the shape of peak %_1 to shapes of all
    # functions in F
    F[*].shape = %_1.shape

    # Create a new simple-variable for each function in F and bound the
    # variable to parameter hwhm. All hwhm parameters will be independent.
    F[*].hwhm = ~0.2

.. admonition:: In the GUI

   buttons |same-hwhm-icon| and |same-shape-icon| on the sidebar make,
   respectively, the HWHM and shape of all functions the same.
   Pressing the buttons again will make all the parameters independent.

.. |same-hwhm-icon| image:: img/eq_fwhm_icon.png
   :alt: =W
   :class: icon

.. |same-shape-icon| image:: img/eq_shape_icon.png
   :alt: =S
   :class: icon

.. _guess:

Guessing Initial Parameters
---------------------------

The program can automatically set initial parameters of peaks (using
peak-detection algorithm) and lines (using linear regression).
Choosing initial parameters of a function by the program
will be called :dfn:`guessing`.

It is possible to guess peak location and add it to *F* with the command::

   guess [%name =] PeakType [(initial values...)] [[x1:x2]]

Examples::

   # add Gaussian in the given range
   @0: guess Gaussian [22.1:30.5]

   # the same, but name the new function %f1
   @0: guess %f1 = Gaussian [22.1:30.5]

   # search for the peak in the whole dataset
   @0: guess Gaussian

   # add one Gaussian to each dataset
   @*: guess Gaussian

   # set the center and shape explicitely (determine height and width)
   guess PseudoVoigt(center=$ctr, shape=~0.3) [22.1:30.5]

- Name of the function is optional.
- Some of the parameters can be specified in brackets.
- If the range is omitted, the whole dataset will be searched.

Fityk offers a simple algorithm for peak-detection.
It finds the highest point in the given range (``center`` and ``height``),
and than tries to find the width of the peak (``hwhm``, and ``area``
= *height* × *hwhm*).

If the highest point is at boundary of the given range,
the points from the boundary to the nearest local minimum are ignored.

The values of height and width found by the algorithm
are multiplied by the values of options :option:`height_correction`
and :option:`width_correction`, respectively. The default value for both
options is 1.

Another simple algorithm can roughly estimate initial parameters of sigmoidal
functions.

The linear traits ``slope`` and ``intercept`` are calculated using linear
regression (without weights of points).
``avgy`` is calculated as average value of *y*.

.. admonition:: In the GUI

   select a function from the list of functions on the toolbar
   and press |add-peak-icon| to add (guess) the selected function.

   To choose a data range change the GUI mode to |mode-add-icon|
   and select the range with the right mouse button.

.. |add-peak-icon| image:: img/add_peak_icon.png
   :alt: Auto Add
   :class: icon

.. |mode-add-icon| image:: img/mode_add_icon.png
   :alt: Add-Peak Mode
   :class: icon


Displaying Information
----------------------

The ``info`` command can be show useful information when constructing
the model.

``info types``
    shows the list of available function types.

``info FunctionType``
    (e.g. ``info Pearson7``) shows the formula (definition).

``info guess [range]``
    shows the values of the special parameters that Fityk can guess, in particular where the ``guess`` command would locate a peak.

``info functions``
    lists all defined functions.

``info variables``
    lists all defined variables.

``info F``
    lists components in *F*.

``info Z``
    lists components in *Z*.

``info formula``
    shows the full mathematical formula of the fitted model.

``info simplified_formula``
    shows the same, but the formula is simplified.

``info gnuplot_formula``
    shows same as ``formula``, but the output is readable by gnuplot,
    e.g. ``x^2`` is replaced by  ``x**2``.

``info simplified_gnuplot_formula``
    shows the simplified formula in the gnuplot format.

``info peaks``
    show a formatted list of parameters of functions in *F*.

``info peaks_err``
    shows the same data, additionally including uncertainties of the parameters.

``info models``
    a script that reconstructs all variables, functions and models.

The last commands are often redirected to a file
(``info peaks > filename``).

The complete list of ``info`` arguments can be found in :ref:`info`.

.. admonition:: In the GUI

  most of the above commands has clickable equivalents.

Now you may want to go back to :ref:`funcindt` if you have skipped it to read this chapter first.
