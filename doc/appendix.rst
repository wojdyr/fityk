
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

The syntax below (in extended BNF) is not complete and may change in the future.

The colon ':' in quoted strings means that the string can be shortened, e.g.
"del:ete" mean that any of "del", "dele", "delet" and "delete" can be used.


::

  (* 
  TODO
   commands > file            ->   set logfile file
   commands < file            ->   exec file
   commands ! shell command   ->   exec ! shell command
   dump > file                ->   info state > file
   @n.Z[0]                    ->   @n.z
  *)

  line ::= [statement {';' statement}] [comment]

  comment ::= '#' { AllChars } 

  statement ::= "w:ith" set
                ( "def:ine" define                   |
                  "del:ete" (delete | delete_points) |
                  "e:xecute" exec                    |
                  "f:it" fit                         |
                  "g:uess" guess                     |
                  "i:nfo" info                       |
                  "p:lot" plot                       |
                  "s:et" set                         |
                  "undef:ine" undef                  |
                  "quit"                             |
                  "reset"                            |
                  "sleep" Number                     |
                  '!' { AllChars }                   |
                  DatasetL '<' load_arg              |
                  DatasetL '=' dataset_tr_arg        |
                  assign                             |
                  point_tr                           )     
                [ 'in' dataset  {',' dataset} ]

  with_st ::= With option {',' option}

  define ::= Typename '(' ...TODO

  delete ::= (Varname | func_id | DatasetR) {',' (Varname | func_id | DatasetR)}

  delete_points ::= '(' point_rhs ')'

  exec ::= ( filename | '!' { AllChars } )

  fit ::= ['+'] Number |
          "undo" |
          "redo" |
          "history" Number |
          "clear_history"

  guess ::= Typename [real_range] [assign_par {, assign_par }]
  TODO:
  guess ::= Typename ['(' assign_par {, assign_par } ')'] [real_range]

  info ::= info_arg {',' info_arg} [redir]

  plot ::= [real_range [real_range]]

  set ::= Key '=' value {',' Key '=' value}

  undefine ::= Typename {, Typename}

  assign ::= var_id '=' var_rhs |
             func_id '=' func_rhs |
             model_id '=' model_rhs { + model_rhs} |
             model_id '+=' model_rhs { + model_rhs } |
             model_id '-=' func_id |
             model_id '.' Key '=' var_rhs |
             Dataset '.' "title" '=' filename

  model_rhs ::= 0 |
                func_id |
                func_rhs |
                model_id |
                "copy" '(' model_id ")" 

  func_rhs ::= Typename '(' assign_par {, assign_par} ')' |
               "copy" '(' func_id ')'

  assign_par ::= Key '=' var_rhs

  load_arg ::= filename {option} | '.'

  dataset_tr_arg ::= [Key] (Dataset | 0) { '+' Dataset }

  point_tr ::= point_lhs '=' point_rhs

  point_lhs ::= M |
                (X | Y | S | A) [ '[' var_rhs ']' ]

  point_rhs ::= TODO

  var_rhs ::= TODO

  model_id = [Dataset '.'] ('F'|'Z')

  func_id ::= Funcname |
              model_id '[' Number ']'

  var_id ::= Varname |
             func_id '.' Key

  real_range ::= '[' [var_rhs|'.'] ':' [var_rhs|'.'] ']' |
                 '.'

  filename ::= QuotedString | { AllChars - (Whitespace | ';' | '#' ) }

  Dataset ::= '@' Integer
  DatasetL ::= Dataset | "@+"
  DatasetR ::= Dataset | "@*"

  Varname ::= '$' Key
  Funcname ::= '%' Key
  Typename ::= Uppercase {Alpha | Digit}

  QuotedString ::= "'" { AllChars - "'" } "'"
  Key ::= (Lowercase | '_') {Lowercase | Digit | '_'}

  AllChars  ::= ? all characters ?
  Alpha     ::= ? a-zA-Z ?
  Lowercase ::= ? a-z ?
  Uppercase ::= ? A-Z ?
  Digit     ::= ? 0-9 ?



..
  TODO
  
  "in @" comma issue: "fit in @0, @1" but "info in @0 @1"



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

