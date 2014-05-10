
Scripts
#######

Working with Scripts
====================

Scripts can be executed using the command::

    exec filename

The file can be either a fityk script (usually with extension ``fit``),
or a Lua script (extension ``lua``).

.. note::

    Fityk can save its state to a script (``info state > file.fit``).
    It can also save all commands executed (directly or via GUI) in the session
    to a script (``info history > file.fit``).

Since a Fityk script with all the data inside can be a large file,
the files may be stored compressed and it is possible to directly read
gzip-compressed fityk script (``.fit.gz``).

Embedded Lua interpreter can execute any program in Lua 5.1.
One-liners can be run with command ``lua``::

    =-> lua print(_VERSION)
    Lua 5.1
    =-> lua print(os.date("Today is %A."))
    Today is Thursday.
    =-> lua for n,f in F:all_functions() do print(n, f, f:get_template_name()) end
    0       %_1     Constant
    1       %_2     Cycle

(The Lua ``print`` function in fityk is redefined to show the output
in the GUI instead of writing to ``stdout``).

Like in the Lua interpreter, ``=`` at the beginning of line can be used
to save some typing::

    =-> = os.date("Today is %A.")
    Today is Thursday.

Similarly, ``exec=`` also interprets the rest of line
as Lua expressions, but this time the resulting string is executed
as a fityk command::

    =-> = string.format("fit @%d", math.random(0,5))
    fit 17
    =-> exec= string.format("fit @%d", math.random(0,5))
    # fitting random dataset (useless example)

The Lua interpreter in Fityk has defined global object ``F`` which
enables interaction with the program::

    =-> = F:get_info("version")
    Fityk 1.2.1

Now the first example that can be useful. For each dataset write output
of the ``info peaks`` command to a file named after the data file,
with appended ".out"::

    =-> @*: lua F:execute("info peaks >'%s.out'" % F:get_info("filename"))

This and other methods of ``F`` are documented in the next section.

.. highlight:: lua

Here is an example of Lua-Fityk interactions::

    -- load data from files file01.dat, file02.dat, ... file13.dat
    for i=1,13 do
        F:execute("@+ < file%02d.dat:0:1::" % i)
    end

    -- print some statistics about the loaded data
    n = F:get_dataset_count()
    print(n .. " datasets loaded.")

    total_max_y = 0
    for i=0, n-1 do
        max_y = F:calculate_expr("max(y)", i)
        if max_y > total_max_y then
            total_max_y = max_y
        end
    end
    print("The largest y: " .. total_max_y)

If a fityk command executed from Lua script fails, the whole script is
stopped, unless you catch the error::

    -- wrap F:execute() in pcall to handle possible errors
    status, err = pcall(function() F:execute("fit") end)
    if status == false then
        print("Error: " .. err)
    end

The Lua interpreter was added in ver. 1.0.3. If you have any questions
about it, feel free to ask.

Older, but still supported way to automate Fityk is to prepare
a stand-alone program that writes a valid fityk script to the standard output.
To run such program and execute the output use command:

.. code-block:: fityk

    exec ! program [args...]


Fityk library API
=================

Fityk comes with embedded Lua interpreter and this language
is used in this section. The API for other supported languages is similar.
Lua communicates with Fityk using object ``F`` of type ``Fityk``.
To call the methods listed below use ``F:method()``, for example
``F:get_dof()`` (not ``Fityk.get_dof()``).

.. note::

    Other supported languages include C++, C, Python, Perl, Ruby and Java.
    Except for C, all APIs are similar.

    Unlike in built-in Lua, in other cases it is necessary to create
    an instance of the Fityk class first. Then you use this object
    in the same way as ``F`` is used below.

    The `fityk.h`_ header file is the best reference.
    Additionally, C++ and Python have access to functions from
    the `ui_api.h`_ header. These functions are used in command line
    versions of fityk (``cfityk`` or its equivalent -- ``samples/cfityk.py``).

    Examples of scripts in all the listed languages and in the `samples`_
    directory.

.. _fityk.h: https://github.com/wojdyr/fityk/blob/master/src/fityk.h
.. _ui_api.h: https://github.com/wojdyr/fityk/blob/master/src/ui_api.h
.. _samples: https://github.com/wojdyr/fityk/blob/master/samples/

Here is the most general function:

.. method:: Fityk.execute(cmd)

    Executes a fityk command. Example: ``F:execute("fit")``.

The ``%`` operator for the string type is pre-set to support Python-like
formatting::

    = "%d pigs" % 3
    = "%d %s" % {3, "pigs"}

Input / output
--------------

.. method:: Fityk.input(prompt)

    Query user. In the CLI user is asked for input in the command line,
    and in the GUI in a pop-up box. As a special case,
    if the prompt contains string "[y/n]" the GUI shows Yes/No buttons
    instead of text entry.

    Example: TODO

.. method:: Fityk.out(s)

    Print string in the output area. The ``print()`` function in built-in Lua
    is redefined to do the same.


Settings
--------

.. method:: Fityk.set_option_as_string(opt, val)

   Set option *opt* to value *val*.
   Equivalent of fityk command ``set opt=val``.

.. method:: Fityk.set_option_as_number(opt, val)

   Set option *opt* to numeric value *val*.

.. method:: Fityk.get_option_as_string(opt)

   Returns value of *opt* (string).

.. method:: Fityk.get_option_as_number(opt)

   Returns value of *opt* (real number).


Data
----

.. method:: Fityk.load_data(d, xx, yy, sigmas [, title])

    Load data to @*d* slot. *xx* and *yy* must be numeric arrays
    of the same size, *sigmas* must either be empty or have the same size.
    *title* is an optional data title (string).

.. method:: Fityk.add_point(x, y, sigma [, d])

    Add one data point ((*x*, *y*) with std. dev. set to *sigma*)
    to an existing dataset *d*.
    If *d* is not specified, the default dataset is used.

    Example: ``F:add_point(30, 7.5, 1)``.

.. method:: Fityk.get_dataset_count()

   Returns number of datasets (n >= 1).

.. method:: Fityk.get_default_dataset()

   Returns default dataset. Default dataset is set by the "use @n" command.

.. method:: Fityk.get_data([d])

   Returns points for dataset *d*.

   * in C++ -- returns vector<Point>
   * in Lua -- userdata with array-like methods, indexed from 0.

   Each point has 4 properties:
   ``x``, ``y``, ``sigma`` (real numbers) and ``is_active`` (bool).

   Example::

       points = F:get_data()
       for i = 0, #points-1 do
           p = points[i]
           if p.is_active then
               print(i, p.x, p.y, p.sigma)
           end
       end

       1       4.24    1.06    1
       2       6.73    1.39    1
       3       8.8     1.61    1
       ...



General Info
------------

.. method:: Fityk.get_info(s [, d])

    Returns output of the fityk ``info`` command as a string.
    If *d* is not specified, the default dataset is used (the dataset
    is relevant for few arguments of the ``info`` command).

    Example: ``F:get_info("history")`` -- returns a multiline string
    containing all fityk commands issued in this session.

.. method:: Fityk.calculate_expr(s, [, d])

    Returns output of the fityk ``print`` command as a number.
    If *d* is not specified, the default dataset is used.

    Example: ``F:calculate_expr("argmax(y)", 0)``.

.. method:: Fityk.get_view_boundary(side)

    Get coordinates of the plotted rectangle,
    which is set by the ``plot`` command.
    Return numeric value corresponding to given *side*, which should be
    a letter ``L``\ (eft), ``R``\ (ight), ``T``\ (op) or ``B``\ (ottom).


Model info
----------

.. method:: Fityk.get_parameter_count()

    Returns number of simple-variables (parameters that can be fitted)

.. method:: Fityk.all_parameters()

    Returns array of simple-variables.

    * in C++ -- vector<double>
    * in Lua -- userdata with array-like methods, indexed from 0.

.. method:: Fityk.all_variables()

    Returns array of all defined variables.

    * in C++ -- vector<Var*>
    * in Lua -- userdata with array-like methods, indexed from 0.

   Example::

       variables = F:all_variables()
       for i = 0, #variables-1 do
           v = variables[i]
           print(i, v.name, v:value(), v.domain.lo, v.domain.hi,
                 v:gpos(), v:is_simple())
       end

   ``Var.is_simple()`` returns true for simple-variables.

   ``Var.gpos()`` returns position of the variable in the global array
   of parameters (Fityk.all_parameters()), or -1 for compound variables.

.. method:: Fityk.get_variable(name)

    Returns variable *$name*.


.. method:: Fityk.all_functions()

    Returns array of functions.

    * in C++ -- vector<Func*>
    * in Lua -- userdata with array-like methods, indexed from 0.

    Example::

      f = F:all_functions()[0] -- first functions
      print(f.name, f:get_template_name())          -- _1        Gaussian
      print(f:get_param(0), f:get_param(1))         -- height  center
      print("$" .. f:var_name("height"))            -- $_4
      print("center:", f:get_param_value("center")) -- center: 24.72235945525
      print("f(25)=", f:value_at(25))               -- f(25)=  4386.95533969

.. method:: Fityk.get_function(name)

    Return the function with given *name*, or NULL if there is no such
    function.

    Example::

      f = F:get_function("_1")
      print("f(25)=", f:value_at(25))               -- f(25)=  4386.95533969

.. method:: Fityk.get_components(d [, fz])

    Returns %functions used in dataset *d*. If *fz* is ``Z``, returns
    zero-shift functions.

    Example::

      func = F:get_components(1)[3] -- get 4th (index 3) function in @1
      print(func)                   -- <Func %_6>
      vname = func:var_name("hwhm")
      print(vname)                  -- _21
      v = get_variable(vname)
      print(v, v:value())           -- <Var $_21>       0.1406587

.. method:: Fityk.get_model_value(x [, d])

    Returns the value of the model for dataset ``@``\ *d* at *x*.


Fit statistics
--------------

.. method:: Fityk.get_wssr([d])

    Returns WSSR (weighted sum of squared residuals).

.. method:: Fityk.get_ssr([d])

    Returns SSR (sum of squared residuals).

.. method:: Fityk.get_rsquared([d])

    Returns R-squared.

.. method:: Fityk.get_dof([d])

    Returns the number of degrees of freedom (#points - #parameters).

.. method:: Fityk.get_covariance_matrix([d])

    Returns covariance matrix.


Examples
========

List peak center of series of data::

    -- file list-max.lua
    prev_x = nil
    for n = 0, F:get_dataset_count()-1 do
        local path = F:get_info("filename", n)
        local filename = string.match(path, "[^/\\]+$") or ""
        -- local x = F:calculate_expr("argmax(y)", n)
        local x = F:calculate_expr("F[0].center", n)
        s = string.format("%s: max at x=%.4f", filename, x)
        if prev_x ~= nil then
            s = s .. string.format("  (%+.4f)", x-prev_x)
        end
        prev_x = x
        print(s)
    end

.. code-block:: fityk

    =-> exec list-max.lua
    frame-000.dat: max at x=-0.0197
    frame-001.dat: max at x=-0.0209  (-0.0012)
    frame-002.dat: max at x=-0.0216  (-0.0007)
    frame-003.dat: max at x=-0.0224  (-0.0008)

Write to file values of the model F(x) at chosen x's
(in this example x = 0, 1.5, 3, ... 150)::

    -- file tabular-f.lua
    file = io.open("output.dat", "w")
    for x = 0, 150, 1.5 do
        file:write(string.format("%g\t%g\n", x, F:get_model_value(x)))
    end
    file:close()

.. code-block:: fityk

    =-> exec tabular-f.lua
    =-> !head -5 output.dat
    0       12.1761
    1.5     12.3004
    3       10.9096
    4.5     9.12635
    6       8.27044

