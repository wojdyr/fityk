
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

    =-> @*: lua F:executef("info peaks >'%s.out'", F:get_info("filename"))

This and other methods of ``F`` are documented in the next section.

.. highlight:: lua

Here is an example of Lua-Fityk interactions::

    -- load data from files file01.dat, file02.dat, ... file13.dat
    for i=1,13 do
        F:executef("@+ < file%02d.dat:0:1::", i)
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

and its variant that may save some typing:

.. method:: Fityk.executef(fmt, ...)

    The same as ``F:execute(string.format(fmt, ...))``.
    Example: ``F:executef("@+ < '%s:0:1::'", filename)``.


Input / output
--------------

.. method:: Fityk.input(prompt)

.. method:: Fityk.out(s)


Settings
--------

TODO


Changing data
-------------

.. method:: Fityk.load_data(d, xx, yy, sigmas, title)

.. method:: Fityk.add_point(x, y, sigma [, d])


General Info
------------

.. method:: Fityk.get_info(s [, d])

.. method:: Fityk.calculate_expr(s, [, d])

.. method:: Fityk.get_view_boundary(side)


Data info
---------

.. method:: Fityk.get_dataset_count()

.. method:: Fityk.get_default_dataset()

.. method:: Fityk.get_data([d])

Model info
----------

.. method:: Fityk.get_parameter_count()

.. method:: Fityk.all_parameters()

.. method:: Fityk.all_variables()

.. method:: Fityk.all_functions()

.. method:: Fityk.get_components(d [, fz])

.. method:: Fityk.get_var(func, parameter)

.. method:: Fityk.get_model_value(x [, d])

.. method:: Fityk.get_model_vector(xx [, d])

.. method:: Fityk.get_variable_nr(name)


Fit statistics
--------------

.. method:: Fityk.get_wssr([d])

.. method:: Fityk.get_ssr([d])

.. method:: Fityk.get_rsquared([d])

.. method:: Fityk.get_dof([d])

.. method:: Fityk.get_covariance_matrix([d])


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

