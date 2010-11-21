
.. title:: Appendices

.. raw:: latex

   \appendix

.. _flist:

Appendix A. List of functions
#############################

The list of all functions can be obtained using
``i+ types``. Some formulae here have long parameter
names (like "height", "center" and "hwhm") replaced with
:math:`a_i`

**Gaussian:**

.. math::
   y = a_0
       \exp\left[-\ln(2)\left(\frac{x-a_1}{a_2}\right)^{2}\right]

**SplitGaussian:**

.. math:: 
   y(x;a_0,a_1,a_2,a_3) = \begin{cases}
   \textrm{Gaussian}(x;a_0,a_1,a_2) & x\leq a_1\\
   \textrm{Gaussian}(x;a_0,a_1,a_3) & x>a_1\end{cases}

**GaussianA:**

.. math:: 
   y = \sqrt{\frac{\ln(2)}{\pi}}\frac{a_0}{a_2}
       \exp\left[-\ln(2)\left(\frac{x-a_1}{a_2}\right)^{2}\right]

**Lorentzian:**

.. math:: 
   y = \frac{a_0}{1+\left(\frac{x-a_1}{a_2}\right)^2}

**SplitLorentzian:**

.. math:: 
   y(x;a_0,a_1,a_2,a_3) = \begin{cases}
   \textrm{Lorentzian}(x;a_0,a_1,a_2) & x\leq a_1\\
   \textrm{Lorentzian}(x;a_0,a_1,a_3) & x>a_1\end{cases}

**LorentzianA:**

.. math:: 
   y = \frac{a_0}{\pi a_2\left[1+\left(\frac{x-a_1}{a_2}\right)^2\right]}

**Pearson VII (Pearson7):**

.. math:: 
   y = \frac{a_0} {\left[1+\left(\frac{x-a_1}{a_2}\right)^2
                           \left(2^{\frac{1}{a_3}}-1\right)\right]^{a_3}}

**split Pearson VII (SplitPearson7):**

.. math:: 
   y(x;a_{0},a_{1},a_{2},a_{3},a_{4},a_{5}) = \begin{cases}
    \textrm{Pearson7}(x;a_0,a_1,a_2,a_4) & x\leq a_1\\
    \textrm{Pearson7}(x;a_0,a_1,a_3,a_5) & x>a_1\end{cases}

**Pearson VII Area (Pearson7A):**

.. math:: 
   y = \frac{a_0\Gamma(a_3)\sqrt{2^{\frac{1}{a_3}}-1}}
            {a_2\Gamma(a_3-\frac{1}{2})\sqrt{\pi} \left[
               1 + \left(\frac{x-a_1}{a_2}\right)^2
                   \left(2^{\frac{1}{a_3}}-1\right)
            \right]^{a_3}}

**Pseudo-Voigt (PseudoVoigt):**

.. math:: 
   y = a_0 \left[(1-a_3)\exp\left(-\ln(2)\left(\frac{x-a_1}{a_2}\right)^2\right)
                 + \frac{a_3}{1+\left(\frac{x-a_1}{a_2}\right)^2}
           \right]

Pseudo-Voigt is a name given to the sum of Gaussian and Lorentzian.
:math:`a_3` parameters in Pearson VII and Pseudo-Voigt
are not related.

**split Pseudo-Voigt (SplitPseudoVoigt):**

.. math:: 
   y(x;a_{0},a_{1},a_{2},a_{3},a_{4},a_{5}) = \begin{cases}
    \textrm{PseudoVoigt}(x;a_0,a_1,a_2,a_4) & x\leq a_1\\
    \textrm{PseudoVoigt}(x;a_0,a_1,a_3,a_5) & x>a_1\end{cases}

**Pseudo-Voigt Area (PseudoVoigtA):**

.. math:: 
   y = a_0 \left[\frac{(1-a_3)\sqrt{\ln(2)}}{a_2\sqrt{\pi}}
                 \exp\left(-\ln2\left(\frac{x-a_1}{a_2}\right)^2\right)
                 + \frac{a_3}{\pi a_2
                              \left[1+\left(\frac{x-a_1}{a_2}\right)^2\right]}
           \right]

**Voigt:**

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
(See also: http://www.atm.ox.ac.uk/user/wells/voigt.html).
The approximation is very fast, but not very exact.

FWHM is estimated using the approximation by Olivero and Longbothum
(`JQSRT 17, 233 (1977)`__):
:math:`0.5346 w_L + \sqrt{0.2169 w_L^2 + w_G^2}`.

__ http://dx.doi.org/10.1016/0022-4073(77)90161-3

**VoigtA:**

.. math:: 
   y = \frac{a_0}{\sqrt{\pi}a_2}
       \int_{-\infty}^{+\infty}
           \frac{\exp(-t^2)}{a_3^2+(\frac{x-a_1}{a_2}-t)^2} dt

**Exponentially Modified Gaussian (EMG):**

.. math:: 
   y = \frac{ac\sqrt{2\pi}}{2d}
       \exp\left(\frac{b-x}{d}+\frac{c^2}{2d^2}\right)
       \left[\frac{d}{\left|d\right|}
             -\textrm{erf}\left(\frac{b-x}{\sqrt{2}c}
                                + \frac{c}{\sqrt{2}d}\right)
       \right]

**LogNormal:**

.. math::
   y = h \exp\left\{ -\ln(2) \left[
                                   \frac{\ln\left(1+2b\frac{x-c}{w}\right)}{b}
                            \right]^{2} \right\}

**Doniach-Sunjic (DoniachSunjic):**

.. math:: 
   y = \frac{h\left[\frac{\pi a}{2} 
                    + (1-a)\arctan\left(\frac{x-E}{F}\right)\right]}
            {F+(x-E)^2}

**Polynomial5:**

.. math:: 
   y = a_0 + a_1 x +a_2 x^2 + a_3 x^3 + a_4 x^4 + a_5 x^5

.. _shortenings:

Appendix B. Grammar
###################

The fityk mini-language (or :dfn:`domain-specific language`) was designed
to perform easily most common tasks.
The language has no flow control (but that's what Python, Lua and other
bindings are for).
Each line is parsed and executed separately. Typically, one line contains
one statement. It can also be empty or contain multiple ';'-separated
statements.

The grammar below is not complete and may change in the future.

The grammar is expressed in EBNF-like notation:

* ``(*this is a comment*)``
* ``A*`` means 0 or more occurrences of A.
* ``A+`` means 1 or more occurrences of A.
* ``A % B`` means ``A (B A)*`` and the ``%`` operator has the highest
  precedence. For example: ``statement % ";" comment`` is the same as
  ``statement (";" statement)* comment``.
* The colon ':' in quoted string means that the string can be shortened, e.g.
  ``"del:ete"`` mean that any of ``del``, ``dele``, ``delet`` and ``delete``
  can be used.

The functions that can be used in ``p_expr`` and ``v_expr`` are available
:ref:`here <transform>` and :ref:`here <variables>`, respectively.
``v_expr`` contains only a subset of functions from ``p_expr`` (partly,
because we need to calculate symbolical derivatives of ``v_expr``)


::

   (* 
   planned changes (already included in the grammar below):
   commands > file            ->   set logfile file
   commands < file            ->   exec file
   commands ! shell-command   ->   exec ! shell-command
   dump > file                ->   info state > file
   guess Func [:] center=30   ->   guess Func(center=30) [:]

   %f = guess Func in @0      ->   guess %f=Func in @0
   
   numarea(%f, 10, 30, 100)   ->   %f.numarea(10, 30, 100)

   cmd in @m, @n              -> @m @n: cmd

   @0 @1: fit # one by one
   fit @0 @1 # all together

   info @n (x, y) >> file     -> @n: print all: x, y >> file
   info @n (x, y, a) >> file  -> @n: print if a: x, y, a >> file
   info pi+1                  -> print pi+1

   F -= %f                    -> not possible (use delete %f or F = ...)
   *)

**Line structure**

.. productionlist::
   line: `statement` % ";" [`comment`]
   statement: [Dataset+ ":"] [`options`] `command`
   options: "w:ith" `set` % ","
   comment: "#" AllChars* 

**Commands**

The kCmd* names in the comments correspond to constants in the code.

.. productionlist::
   command: (
    : "deb:ug" RestOfLine           | (*kCmdDebug*)
    : "def:ine" `define`              | (*kCmdDefine*)
    : "del:ete" `delete`              | (*kCmdDelete*)
    : "del:ete" `delete_points`       | (*kCmdDeleteP*)
    : "e:xecute" `exec`               | (*kCmdExec*)
    : "f:it" `fit`                    | (*kCmdFit*)
    : "g:uess" `guess`                | (*kCmdGuess*)
    : "i:nfo" `info_arg` % "," `redir`  | (*kCmdInfo*)
    : "p:lot" [(`range`|".") [`range`]] | (*kCmdPlot*)
    : "pr:int" `print` `redir`          | (*kCmdPrint*)
    : "quit"                        | (*kCmdQuit*)
    : "reset"                       | (*kCmdReset*)
    : "s:et" `set` % ","              | (*kCmdSet*)
    : "sleep" `expr`                  | (*kCmdSleep*)
    : "title" "=" `filename`          | (*kCmdTitle*)
    : "undef:ine" Uname % ","       | (*kCmdUndef*)
    : "!" RestOfLine                | (*kCmdShell*)
    : Dataset "<" `load_arg`          | (*kCmdLoad*)
    : Dataset "=" `dataset_tr_arg`    | (*kCmdDatasetTr*)
    : Funcname "=" `func_rhs`         | (*kCmdNameFunc*)
    : `func_id` "." Lname "=" `v_expr`  | (*kCmdAssignParam*)
    : `model_id` "." Lname "=" `v_expr` | (*kCmdAssignAll*)
    : Varname "=" `v_expr`            | (*kCmdNameVar*)
    : `model_id` ("="|"+=") `model_rhs` | (*kCmdChangeModel*)
    : (`p_attr` "[" `expr` "]" "=" `p_expr`) % "," | (*kCmdPointTr*)
    : (`p_attr` "=" `p_expr`) % ","     | (*kCmdAllPointsTr*)
    : "M" "=" `expr`                  | (*kCmdResizeP*)
    : ""                            ) (*kCmdNull*)

**Other rules**

.. productionlist::
   define: Uname "(" (Lname [ "=" `v_expr`]) % "," ")" "="
         :    ( `v_expr` |
         :      `type_inst` % "+" |
         :      "x" "<" `v_expr` "?" `type_inst` ":" `type_inst`
         :    )
   delete: (Varname | `func_id` | Dataset) % ","
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
   redir: [(">" | ">>") `filename`]
   set: Lname "=" (Lname | QuotedString | `expr`)
   model_rhs: "0" |
            : `func_id` |
            : `func_rhs` |
            : `model_id` |
            : "copy" "(" `model_id` ")" 
   func_rhs: `type_inst` |
           : "copy" "(" `func_id` ")"
   type_inst: Uname "(" ([Lname "="] `v_expr`) % "," ")" |
   load_arg: `filename` Lname* |
           :   "."
   dataset_tr_arg: [Lname] (Dataset | "0") % "+"
   p_attr: ("X" | "Y" | "S" | "A")
   point_lhs:  `point_attr` [ "[" expr `"]"` ]
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
(e.g.  ``x[4]``), and any expression can be given as an index.
Example: ``(x+x[n-1])/2``.

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
   Lname: (Lowercase | "_") (Lowercase | Digit | "_")*
   Uname: Uppercase AlphaNum+
   Number: ?number read by strtod()?
   NonblankString: (AllChars - (Whitespace | ";" | "#" ))*
   RestOfLine: AllChars*


.. _license:

Appendix C. Program license
###########################

Fityk is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

Text of the license is distributed with the program
in the file :file:`COPYING`.

Appendix D. About this manual
#############################

This manual is written using ReStructuredText.
All changes, improvements, corrections, etc. are welcome.
Use the ``Show Source`` link to get the source of the page, save it,
edit, and send me either modified version or patch containing changes.

Following people have contributed to this manual (in chronological order):
Marcin Wojdyr (maintainer), Stan Gierlotka, Jaap Folmer, Michael Richardson.

..
  $Id$ 

