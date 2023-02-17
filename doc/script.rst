.. _scripts:

Scripts
#######

Working with Scripts
====================

Fityk can run two kinds of scripts:

- Fityk scripts (extension ``.fit``) composed of the commands described in previous sections,
- and Lua scripts (extension ``.lua``), in the Lua language.

Scripts are executed using the `exec` command::

    exec file1.fit
    exec file2.lua
    exec file3.fit.gz  # read script compressed with gzip

.. note::

    Fityk can save its state (session) to a script (``info state > file.fit``).
    It can also save all commands executed (directly or via GUI) in the session
    to a script (``info history > file.fit``). These are the equivalents of GUI Session menu's "Save Session..." and "Save History..." items. 

Embedded Lua interpreter can execute any program in Lua 5.2.
One-liners can be run with command ``lua``::

    =-> lua print(_VERSION)
    Lua 5.1
    =-> lua print(os.date("Today is %A."))
    Today is Thursday.
    
(The Lua ``print`` function in fityk is redefined to show the output
in the GUI instead of writing to ``stdout``).

The embedded Lua interpreter interacts with the rest of the program
through the global object ``F``::

    =-> lua print(F:get_info("version"))
    Fityk 1.2.1
    =-> lua for n,f in F:all_functions() do print(n, f, f:get_template_name()) end
    0       %_1     Constant
    1       %_2     Cycle

All methods of ``F`` are documented in the section :ref:`api`.

Like in the Lua interpreter, ``=`` at the beginning of line can be used
to save some typing::

    =-> = os.date("Today is %A.")
    Today is Thursday.

Similarly, ``exec=`` also interprets the rest of line
as Lua expressions, but this time the resulting string is executed
as a fityk command::

    =-> = string.format("fit @%d", math.random(0,5))
    fit @4
    =-> exec= string.format("fit @%d", math.random(0,5))
    # fitting random dataset (useless example)

A few more examples.

Let's say that we work with a number of datasets, and for each of them
we want to save output of the ``info peaks`` command to a file
named *original-data-filename*\ .out. This can be done in one line::

    =-> @*: lua F:execute("info peaks >'%s.out'" % F:get_info("filename"))

Now a more complex script that would need to be put into a file
(with extension ``.lua``) and run with ``exec``.
:

.. code-block:: lua

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
stopped, unless you catch the error:

.. code-block:: lua

    -- wrap F:execute() in pcall to handle possible errors
    status, err = pcall(function() F:execute("fit") end)
    if status == false then
        print("Error: " .. err)
    end

The Lua interpreter was added in ver. 1.0.3. If you need help with writing
Lua scripts - feel free to ask. Usage scenarios give us a better idea
what functions should be available from the Lua interface.

Fityk also has a simple mechanism to interact with external programs.
It is useful mostly on Unix systems.  ``!`` runs a program,
``exec!`` runs a program, reads its standard output and executes it
as a Fityk script.
Here is an example of using Unix utilties ``echo``, ``ls`` and ``head``
to load the newest CIF file from the current directory::

    =-> ! pwd
    /home/wojdyr/fityk/data
    =-> ! ls -t *.cif | head -1
    lab6_3-2610-q.cif
    =-> exec! echo "@+ < $(ls -t *.cif | head -1)"
    > @+ < lab6_3-2610-q.cif
    2300 points. No explicit std. dev. Set as sqrt(y)

Fityk DSL
=========

As was described in :ref:`cli`, each line has a syntax:

  [[@...:] [with ...] command [";" command]...] [#comment]

The datasets listed before the colon (``:``) make a *foreach* loop.
Here is a silly example::

   =-> $a=0
   =-> @0 @0 @0: $a={$a+1}; print $a
   1
   2
   3

Commands that follow the colon on the same line are run for each specified dataset
in the context of that dataset. This is to say that::

   =-> @2 @4: guess Voigt

is equivalent to::

   =-> use @2
   =-> guess Voigt
   =-> use @4
   =-> guess Voigt

(except that the latter sets permenently the default dataset to ``@4``.

``@*`` stands for all datasets, from ``@0`` to the last one.

Usually, when working with multiple datasets, one executes a command
either for a single dataset or for all of them::

   =-> @3: guess Voigt  # just for @3
   =-> @*: guess Voigt  # for all datasets

The whole line is parsed and partly validated before the execution.
This may lead to unexpected errors when the line has
multiple semicolon-separated commands::

   =-> $a=4; print $a  # print gives unexpected error
   Error: undefined variable: $a

   =-> $b=2
   =-> $b=4; print $b  # $b is already defined at the check time
   4

Therefore, it is recommended to have one command in one line.

Grammar
-------

The grammar is expressed in EBNF-like notation:

* ``(*this is a comment*)``
* ``A*`` means 0 or more occurrences of A.
* ``A+`` means 1 or more occurrences of A.
* ``A % B`` means ``A (B A)*`` and the ``%`` operator has the highest
  precedence. For example: ``term % "+" comment`` is the same as
  ``term ("+" term)* comment``.
* The colon ``:`` in quoted string means that the string can be shortened, e.g.
  ``"del:ete"`` means that any of ``del``, ``dele``, ``delet`` and ``delete``
  can be used.

The functions that can be used in ``p_expr`` and ``v_expr`` are available
:ref:`here <transform>` and :ref:`here <variables>`, respectively.
``v_expr`` contains only a subset of functions from ``p_expr`` (partly,
because we need to calculate symbolical derivatives of ``v_expr``)

**Line structure**

.. productionlist::
   line: [`statement`] [`comment`]
   statement: [Dataset+ ":"] [`with_opts`] `command` % ";"
   with_opts: "w:ith" (Lname "=" `value`) % ","
   comment: "#" AllChars*

**Commands**

The kCmd* names in the comments correspond to constants in the code.

.. productionlist::
   command: (
    : "deb:ug" RestOfLine              | (*kCmdDebug*)
    : "def:ine" `define`                 | (*kCmdDefine*)
    : "del:ete" `delete`                 | (*kCmdDelete*)
    : "del:ete" `delete_points`          | (*kCmdDeleteP*)
    : "e:xecute" `exec`                  | (*kCmdExec*)
    : "f:it" `fit`                       | (*kCmdFit*)
    : "g:uess" `guess`                   | (*kCmdGuess*)
    : "i:nfo" `info_arg` % "," [`redir`]   | (*kCmdInfo*)
    : "l:ua" RestOfLine                | (*kCmdLua*)
    : "=" RestOfLine                   | (*kCmdLua*)
    : "pl:ot" [`range`] [`range`] Dataset* [`redir`] | (*kCmdPlot*)
    : "p:rint" `print_args` [`redir`]      | (*kCmdPrint*)
    : "quit"                           | (*kCmdQuit*)
    : "reset"                          | (*kCmdReset*)
    : "s:et" (Lname "=" `value`) % ","   | (*kCmdSet*)
    : "sleep" `expr`                     | (*kCmdSleep*)
    : "title" "=" `filename`             | (*kCmdTitle*)
    : "undef:ine" Uname % ","          | (*kCmdUndef*)
    : "use" Dataset                    | (*kCmdUse*)
    : "!" RestOfLine                   | (*kCmdShell*)
    : Dataset "<" `load_arg`             | (*kCmdLoad*)
    : Dataset "=" `dataset_expr`         | (*kCmdDatasetTr*)
    : Funcname "=" `func_rhs`            | (*kCmdNameFunc*)
    : `param_lhs` "=" `v_expr`             | (*kCmdAssignParam*)
    : Varname "=" `v_expr`               | (*kCmdNameVar*)
    : Varname "=" "copy" "(" `var_id` ")" | (*kCmdNameVar*)
    : `model_id` ("="|"+=") `model_rhs`    | (*kCmdChangeModel*)
    : (`p_attr` "[" `expr` "]" "=" `p_expr`) % "," | (*kCmdPointTr*)
    : (`p_attr` "=" `p_expr`) % ","        | (*kCmdAllPointsTr*)
    : "M" "=" `expr`                     ) (*kCmdResizeP*)

**Other rules**

.. productionlist::
   define: Uname "(" (Lname [ "=" `v_expr`]) % "," ")" "="
         :    ( `v_expr` |
         :      `component_func` % "+" |
         :      "x" "<" `v_expr` "?" `component_func` ":" `component_func`
         :    )
   component_func: Uname "(" `v_expr` % "," ")"
   delete: (Varname | `func_id` | Dataset | "file" `filename`) % ","
   delete_points: "(" `p_expr` ")"
   exec: `filename` |
       : "!" RestOfLine |
       : "=" RestOfLine
   fit: [Number] [Dataset*] |
      : "undo" |
      : "redo" |
      : "history" Number |
      : "clear_history"
   guess: [Funcname "="] Uname ["(" (Lname "=" `v_expr`) % "," ")"] [`range`]
   info_arg: ...TODO
   print_args: [("all" | ("if" `p_expr` ":")]
             : (`p_expr` | QuotedString | "title" | "filename") % ","
   redir: (">"|">>") `filename`
   value: (Lname | QuotedString | `expr`) (*value type depends on the option*)
   model_rhs: "0" |
            : `func_id` |
            : `func_rhs` |
            : `model_id` |
            : "copy" "(" `model_id` ")"
   func_rhs: Uname "(" ([Lname "="] `v_expr`) % "," ")" |
           : "copy" "(" `func_id` ")"
   load_arg: `filename` Lname* |
           : "."
   p_attr: ("X" | "Y" | "S" | "A")
   model_id: [Dataset "."] ("F"|"Z")
   func_id: Funcname |
          : `model_id` "[" Number "]"
   param_lhs: Funcname "." Lname |
            : `model_id` "[" (Number | "*") "]" "." Lname
   var_id: Varname |
         : `func_id` "." Lname
   range: "[" [`expr`] ":" [`expr`] "]"
   filename: QuotedString | NonblankString

**Mathematical expressions**

.. productionlist::
   expr: expr_or ? expr_or : expr_or
   expr_or: expr_and % "or"
   expr_and: expr_not % "and"
   expr_not: "not" expr_not | comparison
   comparison: arith % ("<"|">"|"=="|">="|"<="|"!=")
   arith: term % ("+"|"-")
   term: factor % ("*"|"/")
   factor: ('+'|'-') factor | power
   power: atom ['**' factor]
   atom: Number | "true" | "false" | "pi" |
       : math_func | braced_expr | ?others?
   math_func: "sqrt" "(" expr ")" |
            : "gamma" "(" expr ")" |
            :  ...
   braced_expr: "{" [Dataset+ ":"] `p_expr` "}"

The ``atom`` rule also accepts some fityk expressions, such as $variable,
%function.parameter, %function(expr), etc.

``p_expr`` and ``v_expr`` are similar to ``expr``,
but they use additional variables in the ``atom`` rule.

``p_expr`` recognizes ``n``, ``M``, ``x``, ``y``, ``s``, ``a``, ``X``, ``Y``,
``S`` and ``A``. All of them but ``n`` and ``M`` can be indexed
(e.g.  ``x[4]``).  Example: ``(x+x[n-1])/2``.

``v_expr`` uses all unknown names (``Lname``) as variables
(example: ``a+b*x^2``).
Only a subset of functions (``math_func``) from ``expr`` is supported.
The tilde (``~``) can be used to create simple-variables (``~5``),
optionally with a domain in square brackets (``~5[1:6]``).

Since ``v_expr`` is used to define variables and user-defined functions,
the program calculates symbolically derivatives of ``v_expr``.
That is why not all the function from ``expr`` are supported
(they may be added in the future).

``dataset_expr`` supports very limited set of operators and a few functions
that take Dataset token as argument (example: ``@0 - shirley_bg(@0)``).

**Lexer**

Below, some of the tokens produced by the fityk lexer are defined.

The lexer is context-dependend: ``NonblankString`` and ``RestOfLine``
are produced only when they are expected in the grammar.

``Uname`` is used only for function types (Gaussian)
and pseudo-parameters (%f.Area).

.. productionlist::
   Dataset: "@"(Digit+|"+"|"*")
   Varname: "$" Lname
   Funcname: "%" Lname
   QuotedString: "'" (AllChars - "'")* "'"
   Lname: (LowerCase | "_") (LowerCase | Digit | "_")*
   Uname: UpperCase AlphaNum+
   Number: ?number read by strtod()?
   NonblankString: (AllChars - (WhiteSpace | ";" | "#" ))*
   RestOfLine: AllChars*

.. _api:

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

    Examples of scripts in all the listed languages are in the `samples`_
    directory of the project.

.. _fityk.h: https://github.com/wojdyr/fityk/blob/master/src/fityk.h
.. _ui_api.h: https://github.com/wojdyr/fityk/blob/master/src/ui_api.h
.. _samples: https://github.com/wojdyr/fityk/blob/master/samples/

Here is the most general function:

.. method:: Fityk.execute(cmd)

    Executes a fityk command. Example: ``F:execute("fit")``.

.. highlight:: lua

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

.. method:: Fityk.load(spec [, d])

    Load data to @*d* slot. The first argument is either a string with path
    or LoadSpec struct that apart from the ``path`` has also the following
    optional members: ``x_col``, ``y_col``, ``sig_col``, ``blocks``,
    ``format``, ``options``. The meaning of these parameters is the same
    as described in :ref:`dataload`.

    For example, due to limitations of the Fityk DSL a file with
    the ``'`` character in the path must be loaded through Lua::

        lua F:load([[Kat's file.dat]])

    LoadSpec is used to specify also format of the file, columns, etc::

        spec = fityk.LoadSpec('my.csv')
        spec.x_col = 1
        spec.y_col = 3
        spec.format = 'csv'
        spec.options = 'decimal_comma'
        F:load(spec)

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

    .. code-block:: none

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
      v = F:get_variable(vname)
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


Examples in Lua
===============

Show how the peak center moves between datasets::

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

