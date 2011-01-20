
Miscellaneous
#############

.. _settings:

Settings
========

The syntax is simple:

* ``set option = value`` changes the *option*,
* ``info set option`` shows the current value,
* ``info set`` lists all available options.

.. admonition:: In the GUI

    the options can be set in a dialog (:menuselection:`Session --> Settings`).

    The GUI configuration (colors, fonts, etc.) is changed in a different
    way (:menuselection:`GUI --> ...`) and is not covered here.

It is possible to change the value of the option temporarily::

    with option1=value1 [,option2=value2]  command args...

For example::

    info set fitting_method  # show the current fitting method
    set fitting_method = nelder_mead_simplex # change the method
    # change the method only for this one fit command
    with fitting_method = levenberg_marquardt fit 10
    # and now the default method is back Nelder-Mead

    # multiple comma-separated options can be given
    with fitting_method=levenberg_marquardt, verbosity=quiet fit 10

The list of available options:

autoplot
    See :ref:`autoplot <autoplot>`.

can_cancel_guess
    See :ref:`guess`.

cut_function_level
    See :ref:`description in the chapter about model <cut_function_level>`.

default_sigma
    Default *y* standard deviation. See :ref:`weights`.
    Possible values: ``sqrt`` max(*y*:sup:`1/2`, 1) and ``one`` (1).

.. _epsilon:

epsilon
    The *ε* value used to test floating-point numbers *a* and *b* for equality
    (it is well known that due to rounding errors the equality test for two
    numbers should have some tolerance, and the tolerance should be tailored
    to the application): \|\ *a−b*\ | < *ε*. Default value: 10\ :sup:`-12`.
    You may need to decrease it when working with very small numbers.

exit_on_warning
    If the option :option:`exit_on_warning`
    is set, any warning will close the program.
    This ensures that no warnings can be overlooked.

fit_replot
    Refresh the plot when fitting (0/1).

fitting_method
    See :ref:`fitting_cmd`.

height_correction
    See :ref:`guess`.

lm_*
    Setting to tune the :ref:`Levenberg-Marquardt <levmar>` fitting method.

log_full
    Log output together with input (0/1).

logfile
    String. File where the commands are logged. Empty -- no logging.

max_wssr_evaluations
    See :ref:`fitting_cmd`.

nm_*
    Setting to tune the :ref:`Nelder-Mead downhill simplex <nelder>`
    fitting method.

.. _numeric_format:

numeric_format
    Format of numbers printed by the ``info`` command. It takes as a value
    a format string, the same as ``sprintf()`` in the C language.
    For example ``set numeric_format=%.3f`` changes the precision
    of numbers to 3 digits after the decimal point. Default value: ``%g``.

pseudo_random_seed
    Some fitting methods and functions, such as
    ``randnormal`` in data expressions use a pseudo-random
    number generator.  In some situations one may want to have repeatable
    and predictable results of the fitting, e.g.  to make a presentation.
    Seed for a new sequence of pseudo-random numbers can be set using the
    option :option:`pseudo_random_seed`.  If it
    is set to 0, the seed is based on the current time and a sequence of
    pseudo-random numbers is different each time.

refresh_period
    During time-consuming computations (like fitting) user interface can
    remain not changed for this time (in seconds).
    This option was introduced, because on one hand frequent refreshing of
    the program's window notably slows down fitting, and on the other hand
    irresponsive program is a frustrating experience.

variable_domain_percent
    See :ref:`the section about variables <domain>`.

verbosity
    Possible values: -1 (silent), 0 (normal), 1 (verbose), 2 (very verbose).

width_correction
    See :ref:`guess`.

Data view
=========

The command ``plot`` controls the region of the graph that is displayed::

   plot [[xrange] yrange] [@n, ...]

*xrange* and *yrange* has syntax ``[min:max]``. If the boundaries
are skipped, they are automatically determined using the given datasets.

.. admonition:: In the GUI

   there is hardly ever a need to use this command directly.

The CLI version on Unix systems visualizes the data using the ``gnuplot``
program, which has similar syntax for the plot range.

Examples::

    plot [20.4:50] [10:20] # show x from 20.4 to 50 and y from 10 to 20
    plot [20.4:] # x from 20.4 to the end,
                 # y range will be adjusted to encompass all data
    plot         # all data will be shown

.. _autoplot:

The values of the options :option:`autoplot` and :option:`fit_replot`
change the automatic plotting behaviour. By default, the plot is
refreshed automatically after changing the data or the model (``autoplot=1``).
It is also possible to replot the model when fitting, to show the progress
(see the options :option:`fit_replot` and :option:`refresh_period`).

.. _info:

Information display
===================

First, there is an option :option:`verbosity`
which sets the amount of messages displayed when executing commands.

There are three commands that print explicitely requested information:

* ``info`` -- used to show preformatted information
* ``print`` -- mainly used to output numbers (expression values)
* ``debug`` -- used for testing the program itself

The output of ``info`` and ``print`` can be redirected to a file::

  info args > filename    # truncate the file
  info args >> filename   # append to the file
  info args > 'filename'  # the filename can (and sometimes must) be in quotes

The redirection can create a file, so there is also a command to delete it::

  delete file filename

info
----

The following ``info`` arguments are recognized:

* *TypeName* -- definition
* *$variable_name* -- formula and value
* *%function_name* -- formula
* ``F`` -- the list of functions in *F*
* ``Z`` -- the list of functions in *Z*
* ``compiler`` -- options used when compiling the program
* ``cov @n`` -- covariance matrix
* ``data`` -- number of points, data filename and title
* ``dataset_count`` -- number of datasets
* ``errors @n`` -- estimated uncertainties of parameters
* ``filename`` -- dataset filename
* ``fit`` -- goodness of fit
* ``fit_history`` -- info about recorded parameter sets
* ``formula`` -- full formula of the model
* ``functions`` -- the list of %functions
* ``gnuplot_formula`` -- full formula of the model, gnuplot style
* ``guess`` -- peak-detection and linear regression info
* ``guess [from:to]`` -- the same, but in the given range
* ``history`` -- the list of all the command issued in this session
* ``history [m:n]`` -- selected commands from the history
* ``history_summary`` -- the summary of command history
* ``peaks`` -- formatted list of parameters of functions in *F*.
* ``peaks_err`` -- the same as peaks + uncertainties
* ``prop`` *%function_name* -- parameters of the function
* ``refs`` *$variable_name* -- references to the variable
* ``set`` -- the list of settings
* ``set`` *option* -- the current value of the option
* ``simplified_formula`` -- simplified formula
* ``simplified_gnuplot_formula`` -- simplified formula, gnuplot style
* ``state`` -- generates a script that can reproduce the current state
  of the program. The scripts embeds all datasets.
* ``title`` -- dataset title
* ``types`` -- the list of function types
* ``variables`` -- the list of variables
* ``version`` -- version number
* ``view`` -- boundaries of the visualized rectangle

Both ``info state`` and ``info history`` can be used to restore the current
session.

.. admonition:: In the GUI

    :menuselection:`Session --> Save State` and
    :menuselection:`Session --> Save History`.

print
-----

The print command is followed by a comma-separated list of expressions
and/or strings::

   =-> p pi, pi^2, pi^3
   3.14159 9.8696 31.0063
   =-> with numeric_format='%.15f' print pi
   3.141592653589793
   =-> p '2+3 =', 2+3
   2+3 = 5

The other valid arguments are ``filename`` and ``title``.
They are useful for listing the same values for multiple datasets, e.g.::

   =-> @*: print filename, F[0].area, F[0].area.error

``print`` can also print a list where each line corresponds to one data point,
as described in the section :ref:`dexport`.

As an exception, ``print expression > filename`` does not work
if the filename is not enclosed in single quotes. That is because the parser
interprets ``>`` as a part of the expression.
Just use quotes (``print 2+3 > 'tmp.dat'``).

debug
-----

Only a few ``debug`` sub-commands are documented here:

* ``der`` *mathematic-function* -- shows derivatives::

    =-> debug der sin(a) + 3*exp(b/a)
    f(a, b) = sin(a)+3*exp(b/a)
    df / d a = cos(a)-3*exp(b/a)*b/a^2
    df / d b = 3*exp(b/a)/a

* ``df`` *x* -- compares the symbolic and numerical derivatives of *F* in *x*.
* ``lex`` *command* -- the list of tokens from the Fityk lexer
* ``parse`` *command* -- show the command as stored after parsing
* ``expr`` *expression* -- VM code from the expression
* ``rd`` -- derivatives for all variables
* ``%function`` -- bytecode, if available
* ``$variable`` -- derivatives

Other commands
==============

* ``exec`` -- Scripts can be executed using the command::

    exec filename

  It is also possible to execute the standard output from external program::

    exec ! program [args...]

* ``reset`` -- reset the session

* ``sleep`` *sec* -- makes the program wait *sec* seconds.

* ``quit`` -- works as expected; if it is found in a script it quits
  the program, not only the script.

* ``!`` -- commands that start with ``!`` are passed (without the ``!``)
  to the ``system()`` call (i.e. to the operating system).


.. _invoking:

Starting the program
====================

On startup, the program runs a script from the
:file:`$HOME/.fityk/init` file (on MS Windows XP:
:file:`C:\\Documents and Settings\\USERNAME\\.fityk\\init`).
Following this, the program executes command passed with the ``--cmd``
option, if given, and processes command line arguments:

- if the argument starts with ``=->``, the string following ``=->``
  is regarded as a command and executed
  (otherwise, it is regarded as a filename),

- if the filename has extension ".fit" or the file begins with a "# Fityk"
  string, it is assumed to be a script and is executed,

- otherwise, it is assumed to be a data file;
  columns and data blocks can be specified in the normal way,
  see :ref:`dataload`.

.. highlight:: none

There are also other parameters to the CLI and GUI versions of the program.
Option "-h" ("/h" on MS Windows) gives the full listing::

    wojdyr@ubu:~/fityk/src$ ./fityk -h
    Usage: fityk \[-h] \[-V] \[-c <str>] \[-I] \[-r] \[script or data file...]
    -h, --help            show this help message
    -V, --version         output version information and exit
    -c, --cmd=<str>       script passed in as string
    -g, --config=<str>    choose GUI configuration
    -I, --no-init         don't process $HOME/.fityk/init file
    -r, --reorder         reorder data (50.xy before 100.xy)


    wojdyr@ubu:~/foo$ cfityk -h
    Usage: cfityk \[-h] \[-V] \[-c <str>] \[script or data file...]
    -h, --help            show this help message
    -V, --version         output version information and exit
    -c, --cmd=<str>       script passed in as string
    -I, --no-init         don't process $HOME/.fityk/init file
    -q, --quit            don't enter interactive shell

The example of non-interactive using CLI version on Linux::

    wojdyr@ubu:~/foo$ ls *.rdf
    dat_a.rdf  dat_r.rdf  out.rdf
    wojdyr@ubu:~/foo$ cfityk -q -I "=-> set verbosity=quiet, autoplot=never" \
    > *.rdf "=-> i+ min(x if y > 0) in @*"
    in @0 dat_a: 1.8875
    in @1 dat_r: 1.5105
    in @2 out: 1.8305

