
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

FWHM is estimated using approximation by Olivero and Longbothum
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

The syntax of the fityk mini-language (it can be called a
:dfn:`domain-specific language`) will be defined formally during the work
on a new parser.

The syntax below (in extended BNF) is not complete and may change in the future.

Note that each line is parsed and executed separately and no new line
characters are expected. ::

  line ::= [{statement ';'} statement] [comment] |
           '!' { AllChars }
  (* TODO: line ::= statement  -- exactly one statement per line, no ';' *)

  comment ::= '#' { AllChars } 

  statement ::= [with_st] ( commands_st |
                            define_st |
                            delete_st |
                            fit_st |
                            guess_st |
                            info_st |
                            plot_st |
                            set_st |
                            undefine_st |
                            assign_st |
                            dataset_st |
                            dump_st |
                            "quit" |
                            "reset" |
                            "sleep" Number |
                            transform_st )

  with_st ::= With option {',' option}

  commands_st ::= Commands ...TODO
      (* TODO: commands > file -> set logfile file *)
      (* TODO: commands < file -> exec file *)
      (* TODO: commands ! shell command -> exec ! shell command *)

  define_st ::= Define ...TODO

  delete_st ::= Delete delete_arg {, delete_arg}
  delete_arg ::= Varname | func_id | Dataset

  fit_st ::= Fit fit_arg in_arg
  fit_arg ::= ['+'] Number | undo | redo | history Number | history clear

  guess_st ::= Guess ...TODO

  info_st ::= Info info_arg {',' info_arg} [redir]

  dump_st ::= "dump" redir (* to be replaced with info state *)

  plot_st ::= Plot ...TODO

  set_st ::= Set (option {',' option} | name)

  undefine_st ::= Undefine Word {, Word}

  assign_st ::= Varname '=' TODO |
                func_id '=' TODO |
                Funcname '.' Word '=' TODO |
                Dataset '.' 'F' '=' TODO |
                Dataset '.' 'F' '.' Word '=' TODO |
                Dataset '.' "title" '=' TODO

  dataset_st ::= (Dataset | "@+") ( '<' Filename {option} |
                                    '=' [Word] Dataset { '+' Dataset } {Word} )

  transform_st ::= transform_lhs '=' data_expression


  func_id = Funcname |
            (* TODO: Dataset '.' 'z' | *)
            Dataset '.' 'F' '[' Number ']'

  option ::= name '=' value

  string ::= QuotedString | Word


  QuotedString ::= "'" { AllChars - "'" } "'"

  Word ::= { AllChars - (Whitespace | ';' | ',' | '#' ) }
           
  AllChars ::= ? all characters ?

  Varname ::= '$' Word
  Funcname ::= '%' Word

  Commands ::= "c" | "co" | "com" | ... | "commands"
  Define ::= "def" | ... | "define"
  Delete ::= "del" | ... | "delete"
  Guess ::= "g" | ... | "guess"
  Info ::= "i" | ... | "info"
  Plot ::= "p" | ... | "plot"
  Set ::= "s" | "se" | "set"
  Undefine ::= "undef" | ... | "undefine"
  With ::= "w" | ... | "with"

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

