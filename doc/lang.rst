.. _lang:

Mini-Language
#############

Fityk comes with own domain-specific language (DSL), which is humbly
called mini-language. All operations are driven by commands of the
mini-language.

.. admonition:: Do not worry

   you do not need to learn the syntax of the mini-language.
   It is possible to use menus and dialogs in the GUI
   and avoid typing commands.

When you use the GUI and perform an action using the menu,
you can see the corresponding command in the output window.
It is one of less than 30 fityk commands. The commands have relatively
simple syntax and perform single actions, such as loading data from file,
adding function, assigning variable, fitting, or writing some values to a file.

It is possible to write a script (macro) as a sequence of such
commands. This can automate common tasks, although some complex tasks
still need to be programmed in a general-purpose language.
That is why Fityk comes with embedded Lua (lightweight programming language)
and also with bindings to Python and several other languages.

Now a quick glimpse at the syntax. Here, the ``=->`` prompt marks an input::

  =-> print pi
  3.14159
  =-> # this is a comment -- from `#' to the end of line
  =-> p '2+3=', 2+3 # p stands for print
  2+3 = 5
  =-> set numeric_format='%.9f' # show 9 digits after dot
  =-> pr pi, pi^2, pi^3 # pr, pri and prin also stand for print
  3.141592654 9.869604401 31.006276680

Usually, one line is one command, but if it is really needed,
two or more commands can be put in one line like this::

  =-> $a = 3; $b = 5 # two commands separated with `;'

If the user works simultaneously with multiple datasets, she can refer to
a dataset using its number: the first dataset is ``@0``, the second -- ``@1``,
etc::

  =-> fit # perform fitting of the default dataset (the first one)
  =-> @2: fit # fit the third dataset (@2)
  =-> @*: fit # fit all datasets, one by one

All settings in the program are changed using the command ``set``::

  set key = value

For example::

  =-> set logfile = 'C:\log.fit' # log all commands to this file
  =-> set verbosity = 1 # make output from the program more verbose
  =-> set epsilon = 1e-14

The last example changes the *ε* value, which is used to test floating-point
numbers *a* and *b* for equality (it is well known that due to rounding
errors the equality test for two numbers should have some tolerance,
and the tolerance should be tailored to the application): \|\ *a−b*\ | < *ε*.

To change a setting only for one command, add ``with key=value`` before
the command::

  =-> with epsilon = 0.1 print pi == 3.14 # abusing epsilon
  1
  =-> print pi == 3.14 # default epsilon = 10^-12
  0

.. highlight:: none

Writing informally, each line has a syntax::

  [[@...:] [with ...] command [";" command]...] [#comment]

In scripts and in the CLI backslash (\) at the end of the line means
that the next line is continuation.

All the commands are described in the next chapters.

.. important::

  The rest of this section can be useful as reference, but it is recommended
  to **skip it** when reading the manual for the first time.

To keep the description above concise, some details were skipped.

The datasets listed before the colon (``:``) make a *foreach* loop::

   =-> $a=0
   =-> @0 @0 @0: $a={$a+1}; print $a
   1
   2
   3

``@*`` stands for all datasets, from ``@0`` to the last one.


There is a small difference between two commands in two lines and two commands
separated by ``;``.
The whole line is parsed before the execution begins and some checks
for the second command are performed before the first command is run::

   =-> $a=4; print $a # print gives unexpected error
   Error: undefined variable: $a

   =-> $b=2
   =-> $b=4; print $b # $b is already defined at the check time
   4


Grammar
=======

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

