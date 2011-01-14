
Miscellaneous
#############

.. _settings:

set: options
============

.. note:: This manual does not cover the GUI configuration (colors,
   fonts, etc.).

Command ``info set`` shows the syntax of the set command and lists all
possible options.

``set option`` shows the current value of the *option*.

``set option = value`` changes the *option*.

It is possible to change the value of the option temporarily using syntax::

    with option1=value1 [,option2=value2]  command args...

The examples at the end of this chapter should clarify this.

autoplot
    See :ref:`autoplot <autoplot>`.

can_cancel_guess
    See :ref:`guess`.

cut_function_level
    See :ref:`speed`.

data_default_sigma
    See :ref:`weights`.

.. _epsilon:

epsilon
    It is used for floating-point comparison:
    a and b are considered equal when
    \|a−b|<:option:`epsilon`.
    You may want to decrease it when you work with very small values,
    like 10\ :sup:`−10`.

exit_on_warning
    If the option :option:`exit_on_warning`
    is set, any warning will close the program.
    This ensures that no warnings can be overlooked.

fitting_method
    See :ref:`fitting_cmd`.

formula_export_style
    See :ref:`details in the section "Model" <formula_export_style>`.

guess_at_center_pm
    See :ref:`guess`.

height_correction
    See :ref:`guess`.

.. _numeric_format:

numeric_format
    Format of numbers printed by the ``info`` command. It takes as a value
    a format string, the same as ``sprintf()`` in the C language.
    For example ``set numeric_format=%.3f`` changes the precision
    of numbers to 3 digits after the decimal point. Default value: ``%g``.

lm_*
    Setting to tune the :ref:`Levenberg-Marquardt <levmar>` fitting method.

max_wssr_evaluations
    See :ref:`fitting_cmd`.

nm_*
    Setting to tune the :ref:`Nelder-Mead downhill simplex <nelder>`
    fitting method.

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
    Possible values: quiet, normal, verbose, debug.

width_correction
    See :ref:`guess`.

Examples::

    set fitting_method  # show info
    set fitting_method = nelder_mead_simplex # change default method
    set verbosity = verbose
    with fitting_method = levenberg_marquardt fit 10
    with fitting_method=levenberg_marquardt, verbosity=quiet fit 10

plot: viewing data
==================

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
======================

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

+ fit_history

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
=====================================

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

It is also possible to execute the standard output from external program::

   commands ! program [args...]

The command::

   dump > filename

writes the current state of the program
(including all datasets) to a single .fit file.

The command ``sleep sec`` makes the program wait *sec* seconds.

The command ``quit`` works as expected.
If it is found in a script it quits the program, not only the script.

Commands that start with ``!`` are passed (without '!')
to the ``system()`` call.

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
  (otherwise, it is regarded as a filename).

- if the filename has extension ".fit" or the file begins with a "# Fityk"
  string, it is assumed to be a script and is executed.

- otherwise, it is assumed to be a data file.
  (columns and data blocks can be specified in the normal way, see <dataload>).

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

