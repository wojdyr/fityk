.. _fit:

Curve Fitting
=============

.. _nonlinear:

Nonlinear Optimization
----------------------

This is the core. We have a set of observations (data points), to which
we want to fit a *model* that depends on adjustable parameters.
Let me quote `Numerical Recipes <http://www.nrbook.com/a/bookcpdf.php>`_,
chapter 15.0, page 656):

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

Our function of merit is the weighted sum of squared residuals (WSSR),
also called chi-square:

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
chapter *Least-Squares Fitting. Fitting Overview* (weighted data version).

.. _fitting_cmd:

Fitting Related Commands
------------------------

To fit model to data, use command

fit [+] [number-of-iterations] [@n ...]

The plus sign (+) prevents initialization of the fitting method.
It is used to continue the previous fitting where it left off
(if the previous fitting was stopped before it converged).

All non-linear fitting methods are iterative.
*number-of-iterations* is the maximum number of iterations.
There are also other stopping criteria, so the number of executed
iterations can be smaller.

Like with all commands, the generic dataset specification (``@n: fit``)
can be used, but in special cases the datasets can be given at the end
of the command. The difference is that
``fit @*`` fits all datasets simultaneously, while
``@*: fit`` fits all datasets one by one, separately.

The fitting method can be set using the set command::

  set fitting_method = method

where method is one of: ``levenberg_marquardt``, ``nelder_mead_simplex``,
``genetic_algorithms``.

All non-linear fitting methods are iterative, and there are three common
stopping criteria:

- the number of iterations; it can be specified after the ``fit`` command.

- the number of evaluations of the objective function (WSSR); set
  using the option :option:`max_wssr_evaluations` (0=unlimited).

- and the processor time, in seconds
  (:option:`max_fitting_time`,  0=unlimited).

There are also other criteria, different for each method.

On Unix, fitting can be interrupted by sending the ``INT`` signal to the
program. This is usually done by pressing Ctrl-C in the terminal.

Setting ``set fit_replot = 1`` will plot a model after every iteration,
to visualize progress.

``info fit`` shows measures of goodness-of-fit, including :math:`\chi^2`,
reduced :math:`\chi^2` and R-squared:

.. math::
   R^2 \equiv 1 - {{\sum_i (y_i - f_i)^2} \over {\sum_i (y_i-\bar{y})^2}}

Values of all parameters are stored before and after fitting (if they
change). This enables simple undo/redo functionality.
If in the meantime some functions or variables where added or removed,
the program can still load the old parameters, but the result can be
unexpected. The following history-related commands are provided:

fit undo
    move back to the previous parameters (undo fitting).

fit redo
    move forward in the parameter history

info fit_history
    show number of items in the history

fit history *n*
    load the *n*-th set of parameters from history

fit clear_history
    clear the history

Uncertainty of Parameters
-------------------------

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


In Fityk:

* ``info errors`` shows values of :math:`\sigma_{a_k}`.
* ``info confidence 95`` shows confidence limits for confidence level 95%
  (any level can be choosen)
* ``info cov`` shows the matrix *C*:sup:`--1`.
* Individual symmetric errors of simple-variables can be accessed as
  ``$variable.error`` or e.g. ``%func.height.error``.

.. admonition:: In the GUI

    select :menuselection:`Fit --> Info` from the menu to see uncertainties,
    confidence intervals and and the covariance matrix.

.. note::

    In Fityk 0.9.0 and earlier ``info errors`` reported values of
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
the value of :option:`lm_lambda_down_factor` option (default: 10).
Otherwise, |lambda| is multiplied by the value of
:option:`lm_lambda_up_factor` (default: 10).
The initial |lambda| value is equal to
:option:`lm_lambda_start` (default: 0.0001).

The Marquardt method has two stopping criteria other than the common
criteria.

- If it happens twice in sequence, that the relative
  change of the value of the objective function (WSSR) is smaller than
  the value of the :option:`lm_stop_rel_change` option, the
  fit is considered to have converged and is stopped.

- If |lambda| is greater than the value of the :option:`lm_max_lambda`
  option (default: 10^15), usually when due to limited numerical precision
  WSSR is no longer changing, the fitting is also stopped.

.. |lambda| replace:: *λ*

.. _nelder:

Nelder-Mead Downhill Simplex
----------------------------

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
stopping criterium :option:`nm_convergence`. If the value of the
expression 2(*M*\ −\ *m*)/(*M*\ +\ *m*), where *M* and *m* are the values
of the worst and best vertices respectively (values of objective functions of
vertices, to be precise!), is smaller then the value of
:option:`nm_convergence` option, fitting is stopped. In other words,
fitting is stopped if all vertices are almost at the same level.

The remaining options are related to initialization of the simplex.
Before starting iterations, we have to choose a set of points in space
of the parameters, called vertices.  Unless the option
:option:`nm_move_all` is set, one of these points will be the current
point -- values that parameters have at this moment. All but this one
are drawn as follows: each parameter of each vertex is drawn separately.
It is drawn from a distribution that has its center in the center of the
:ref:`domain <domain>` of the parameter, and a width proportional to
both width of the domain and value of the :option:`nm_move_factor`
parameter.  Distribution shape can be set using the option
:option:`nm_distribution` as one of: ``uniform``, ``gaussian``,
``lorentzian`` and ``bound``. The last one causes the value of the
parameter to be either the greatest or smallest value in the domain of
the parameter -- one of the two bounds of the domain (assuming that
:option:`nm_move_factor` is equal 1).

Genetic Algorithms
------------------

\[TODO]

