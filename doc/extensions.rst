
Extensions
##########

How to add your own built-in function
-------------------------------------

.. note:: Add built-in function only if
   :ref:`user-defined function (UDF) <udf>`
   is too slow or too limited.

To add a built-in function, you have to change the source of the program
and then recompile it. Users who want to do this should be able to compile
the program from source and know the basics of C, C++ or another
programming language.

The description that follows is not complete. If something is not clear,
you can always send me e-mail, etc.

"fp" you can see in fityk source means a real (floating point) number
(typedef double fp).

The name of your function should start with uppercase letter and contain
only letters and digits.  Let us add function Foo with the formula:
Foo(height, center, hwhm) = height/(1+((x-center)/hwhm)^2).
C++ class representing Foo will be named FuncFoo.

In src/func.cpp you will find a list of functions:

::

    ...
    FACTORY_FUNC(Polynomial6)
    FACTORY_FUNC(Gaussian)
    ...

Now, add:

::

    FACTORY_FUNC(Foo)

Then find another list:

::

    ...
    FuncPolynomial6::formula,
    FuncGaussian::formula,
    ...

and add the line

::

    FuncFoo::formula,

Note that in the second list
all items but the last are followed by comma.

In the file :file:`src/bfunc.h` you can now begin writing the definition
of your class:

::

    class FuncFoo : public Function
    {
    DECLARE_FUNC_OBLIGATORY_METHODS(Foo, Function)

If you want to make some calculations every time parameters of the function
are changed, you can do it in method do_precomputations.
This possibility is provided for calculating expressions,
which do not depend on x. Write the declaration here:

::

    void do_precomputations(std::vector<Variable*> const &variables);

and provide a proper definition of this method
in :file:`src/bfunc.cpp`.

If you want to optimize the calculation of your function by neglecting
its value outside of a given range
(see option :option:`cut_function_level`
in the program),
you will need to use the method:

::

    bool get_nonzero_range (fp level, fp &left, fp &right) const;

This method takes the level below which the value of the function
can be approximated by zero, and should set the left and right variables
to proper values of x,
such that if x<left or x>right than \|f(x)|<level.
If the function sets left and right, it should return true.

If your function does not have a "center" parameter, and there is a
center-like point where you want the peak top to be drawn, write:

::

    bool has_center() const { return true; }
    fp center() const { return vv[1]; }

In the second line, between return and the semicolon, there is an expression
for the x coordinate of peak top; vv[0] is the first parameter of function,
vv[1] is the second, etc.

Finally, close the definition of the class with:

::

    };

Now go to file :file:`src/bfunc.cpp`.

Write the function formula in this way:

::

    const char *FuncFoo::formula
    = "Foo(height, center, hwhm) = height/(1+((x-center)/hwhm)^2)";

The syntax of the formula is the similar as that of
the :ref:`UDF <udf>`, but
for built-in functions only the left hand side of the formula is parsed.
The right hand side is for documentation only.

Write how to calculate the value of the function:

::

    CALCULATE_VALUE_BEGIN(Foo)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp inv_denomin = 1. / (1 + xa1a2 * xa1a2);
    CALCULATE_VALUE_END(vv[0] * inv_denomin)

The expression at the end (i.e. vv[0]*inv_denomin) is the calculated value.
xa1xa2 and inv_denomin are variables introduced to simplify the
expression. Note the "fp" (you can also use "double") at the beginning
and semicolon at the end of both lines. The meaning of vv has
already been explained.

Usually it is more difficult to calculate derivatives:

::

    CALCULATE_DERIV_BEGIN(Foo)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp inv_denomin = 1. / (1 + xa1a2 * xa1a2);
    dy_dv[0] = inv_denomin;
    fp dcenter = 2 * vv[0] * xa1a2 / vv[2] * inv_denomin * inv_denomin;
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dx = -dcenter;
    CALCULATE_DERIV_END(vv[0] * inv_denomin)

You must set derivatives
dy_dv[n] for n=0,1,...,(number of parameters of your function - 1)
and dy_dx. In the last brackets there is a value of the function again.

If you declared ``do_precomputations`` or
``get_nonzero_range`` methods,
do not forget to write definitions for them.

After compilation of the program check if the derivatives are calculated
correctly using command "info dF(x)", e.g. i dF(30.1).
You can also use ``numarea``,
``findx`` and ``extremum``
(see :ref:`funcindt` for details)
to verify center, area, height and FWHM properties.

Hope this helps.
Do not hesistate to change this description or ask questions
if you have any. Consider sharing your function with other users (using
:wiki:`FitykWiki <>` or mailing list).

..
  $Id$ 

