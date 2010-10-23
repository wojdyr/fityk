
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
* ``{A}`` means repetition (0 or more occurrences of A).
* ``A % B`` means ``A {B A}`` and the ``%`` operator has the highest precedence.
* The colon ':' in quoted string means that the string can be shortened, e.g.
  ``"del:ete"`` mean that any of ``del``, ``dele``, ``delet`` and ``delete``
  can be used.

The functions that can be used in ``point_rhs`` and ``var_rhs`` are available
:ref:`here <transform>` and :ref:`here <variables>`, respectively.
``var_rhs`` contains only a subset of functions from ``point_rhs`` (partly,
because we need to calculate symbolical derivatives of ``var_rhs``)

::

  (* 
  planned changes (already included in the grammar below):
   commands > file            ->   set logfile file
   commands < file            ->   exec file
   commands ! shell command   ->   exec ! shell command
   dump > file                ->   info state > file
   @n.Z[0]                    ->   @n.z
   guess Func [:] center=30   ->   guess Func(center=30) [:]
   info in @0 @1              ->   info in @0, @1

   %f = guess Func in @0      ->   guess %f=Func in @0
  *)
  

  line ::= [statement {';' statement}] [comment]

  comment ::= '#' { AllChars } 

  statement ::= "w:ith" set % ','
                ( "def:ine" define                   |
                  "del:ete" (delete | delete_points) |
                  "e:xecute" exec                    |
                  "f:it" fit                         |
                  "g:uess" guess                     |
                  "i:nfo" info                       |
                  "p:lot" [range [range]]            |
                  "quit"                             |
                  "reset"                            |
                  "s:et" set % ','                   |
                  "sleep" numeric                    |
                  "undef:ine" Uname % ','            |
                  '!' { AllChars }                   |
                  DatasetL '<' load_arg              |
                  DatasetL '=' dataset_tr_arg        |
                  assign                             |
                  assign_func                        |
                  assign_var                         |
                  change_model                       |
                  point_tr {, point_tr}              )     
                [ "in" DatasetR  % ',' ]

  define ::= Uname '(' (Lname | kwarg) % ',' ')'
             '=' ( var_rhs |
                   func_rhs % '+' |
                   "x <" var_rhs '?' func_rhs ':' func_rhs
                 )

  delete ::= (Varname | func_id | DatasetR) % ','

  delete_points ::= '(' point_rhs ')'

  exec ::= ( filename | '!' {AllChars} )

  fit ::= ['+'] Number |
          "undo" |
          "redo" |
          "history" Number |
          "clear_history"

  guess ::= [Funcname '='] Uname ['(' kwarg % ',' ')'] [range]

  info ::= info_arg % ',' [('>'|">>") filename]

  info_arg ::= ...

  set ::= Lname '=' (Lname | QuotedString | numeric)

  assign ::= model_id '.' Lname '=' var_rhs |
             Dataset '.' "title" '=' filename

  assign_var ::= var_id '=' var_rhs

  assign_func ::= func_id '=' func_rhs

  change_model ::= model_id ('=' | "+=") model_rhs % '+' |
                   model_id "-=" func_id

  model_rhs ::= 0 |
                func_id |
                func_rhs |
                model_id |
                "copy" '(' model_id ")" 

  func_rhs ::= Uname '(' ([Lname '='] var_rhs) % ',' ')' |
               "copy" '(' func_id ')'

  kwarg ::= Lname '=' var_rhs

  load_arg ::= filename {option} | '.'

  dataset_tr_arg ::= [Lname] (Dataset | 0) % '+'

  point_tr ::= point_lhs '=' point_rhs

  point_lhs ::= M |
                (X | Y | S | A) [ '[' numeric ']' ]

  point_rhs ::= ? Mathematical expression that may use n, M,
                  x[], y[], s[], a[], X[], Y[], S[] and A[]
                  variables.
                  The reference section lists all the operators
                  and functions that can be used.
                ?

  var_rhs ::= ? Mathematical expression that uses unknown names
                as variables.
                Only a subset of functions from point_rhs.
                Supports point_rhs inside {}, e.g. {max(y) in @0}.
                Creates variables using '~', like ~10 or ~{2+$foo}.
              ?

  numeric ::= ? point_rhs without data variables/arrays,
                usually just a number
              ?

  (* used in math expressions, translated to numeric constant *)
  (* example: {max(y) in @0}                                  *)
  braced_expr ::= '{' point_rhs [ "in" DatasetR % ',' ] '}'

  model_id = [Dataset '.'] ('F'|'Z')

  func_id ::= Funcname |
              model_id '[' Number ']'

  var_id ::= Varname |
             func_id '.' Lname

  range ::= '[' [numeric|'.'] ':' [numeric|'.'] ']' |
                 '.'
  filename ::= QuotedString | { AllChars - (Whitespace | ';' | '#' ) }

  Dataset ::= '@' Integer
  DatasetL ::= Dataset | "@+"
  DatasetR ::= Dataset | "@*"

  Varname ::= '$' Lname
  Funcname ::= '%' Lname

  QuotedString ::= "'" { AllChars - "'" } "'"
  Lname ::= (Lowercase | '_') {Lowercase | Digit | '_'}
  (* Uname is used for type names and pseudo-parameters (%f.Area) *)
  Uname ::= Uppercase (Alpha | Digit) {Alpha | Digit}

  AllChars  ::= ? all characters ?
  Alpha     ::= ? a-zA-Z ?
  Lowercase ::= ? a-z ?
  Uppercase ::= ? A-Z ?
  Digit     ::= ? 0-9 ?


.. _license:

Appendix C. License
###################

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

