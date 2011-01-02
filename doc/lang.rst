
Fityk mini-language
###################

.. note::

   You do not need to learn the syntax of the fityk mini-language.
   It is possible to use menus and dialogs in the GUI
   instead of typing the commands.

Introduction
============

The fityk mini-language (or :dfn:`domain-specific language`) was designed
to be simple and to perform easily most common tasks.
The language has no flow control (but that is what Python, Lua and other
bindings are for).
Each line is parsed and executed separately. Typically, one line contains
one command, but the line can also be empty or contain multiple ``;``-separated
commands.

The hash (``#``) starts a comment -- everything from the hash
to the end of the line is ignored.

Some commands can be shortened: e.g. you can type
``inf`` or ``in`` or ``i`` instead of ``info``.

TO BE CONTINUED


Grammar
=======

.. warning::

   This section presents grammar from not-yet-released version 0.9.5.

The grammar is expressed in EBNF-like notation:

* ``(*this is a comment*)``
* ``A*`` means 0 or more occurrences of A.
* ``A+`` means 1 or more occurrences of A.
* ``A % B`` means ``A (B A)*`` and the ``%`` operator has the highest
  precedence. For example: ``term % "+" comment`` is the same as
  ``term ("+" term)* comment``.
* The colon ':' in quoted string means that the string can be shortened, e.g.
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
    : "p:lot" [`range`] [`range`] Dataset* | (*kCmdPlot*)
    : "pr:int" `print` [`redir`]           | (*kCmdPrint*)
    : "quit"                           | (*kCmdQuit*)
    : "reset"                          | (*kCmdReset*)
    : "s:et" (Lname "=" `value`) % ","   | (*kCmdSet*)
    : "sleep" `expr`                     | (*kCmdSleep*)
    : "title" "=" `filename`             | (*kCmdTitle*)
    : "undef:ine" Uname % ","          | (*kCmdUndef*)
    : "use" Dataset                    | (*kCmdUse*)
    : "!" RestOfLine                   | (*kCmdShell*)
    : Dataset "<" `load_arg`             | (*kCmdLoad*)
    : Dataset "=" `dataset_tr_arg`       | (*kCmdDatasetTr*)
    : Funcname "=" `func_rhs`            | (*kCmdNameFunc*)
    : `func_id` "." Lname "=" `v_expr`     | (*kCmdAssignParam*)
    : `model_id` "." Lname "=" `v_expr`    | (*kCmdAssignAll*)
    : Varname "=" `v_expr`               | (*kCmdNameVar*)
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
   delete_points: "(" p_expr ")"
   exec: `filename` |
       : "!" RestOfLine
   fit: [Number] [Dataset*] |
      : "+" Number |
      : "undo" |
      : "redo" |
      : "history" Number |
      : "clear_history"
   guess: [Funcname "="] Uname ["(" (Lname "=" `v_expr`) % "," ")"] [`range`]
   info_arg: ...TODO
   print: ...TODO
   redir: ">>" `filename`
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
   dataset_tr_arg: [Lname] (Dataset | "0") % "+"
   p_attr: ("X" | "Y" | "S" | "A")
   model_id: [Dataset "."] ("F"|"Z")
   func_id: Funcname |
          : `model_id` "[" Number "]"
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
   braced_expr: "{" [Dataset+ ":"] p_expr "}"

The ``atom`` rule also accepts some fityk expressions, such as $variable,
%function.parameter, %function(expr), etc.

``p_expr`` and ``v_expr`` are similar to ``expr``,
but they use additional variables in the ``atom`` rule.

``p_expr`` recognizes ``n``, ``M``, ``x``, ``y``, ``s``, ``a``, ``X``, ``Y``,
``S`` and ``A``. All of them but ``n`` and ``M`` can be indexed
(e.g.  ``x[4]``).  Example: ``(x+x[n-1])/2``.

``v_expr`` uses all unknown names (``Lname``) as variables. The tilde (``~``)
can be used to create simple-variables.
Only a subset of functions (``math_func``) from ``expr`` is supported.
Examples: ``a+b*x^2``, ``~5``.

Since ``v_expr`` is used to define variables and user-defined functions,
the program calculates symbolically derivatives of ``v_expr``.
That is why not all the function from ``expr`` are supported
(they may be added in the future).

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

