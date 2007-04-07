/*! \mainpage Documentation of Fityk Source 
 *
 * \par
 * This is a documentation of source for developers. 
 * If you just want to use the program, see 
 * <a href="http://www.unipress.waw.pl/fityk/doc/html/index.html">
 * user's manual</a>.
 * \par
 * If you want to use a fityk library in your program, see
 *  the namespace #fityk, where the public API is documented.
 * \par
 * Source code subdirectories:
 * - src/ - fityk library (libfityk)
 * - src/wxgui/ - wxWidgets-based GUI
 * - src/cli/   - readline/gnuplot based CLI
 * - src/python/   - python bindings to libfityk
 * \par
 * libfityk - parsers
 *
 * Fityk is using Boost::Spirit for parsing commands and data transformations.
 * The grammar of fityk scripts consists of three main parts:
 * Unfortunatelly, it was necessary to split grammar into more files. 
 * Otherwise, compilation would require gigabytes of memory. 
 * Some grammar rules are repeated in two or more files.
 * - The grammar of commands is in cmd* files. 
 *   example:  with fitting-method=Nelder-Mead-simplex fit 10
 * - Data transformation grammar can be found in datatrans* files.
 *   example: X = sin(3*x) + y
 * - function grammar (FuncGrammar) is the shortest one, in var.cpp 
 *   it uses ast from Spirit.
 *   example: $h * exp(-(pi*x)^2)
 * Compilation of any file shouldn't require more than 0.5 GB of memory.
 * \par
 * libfityk - symbolic calculation of derivatives
 *
 * That's most funny part. Fityk, unlike most of other program (e.g. gnuplot),
 * calculates derivatives of functions symbolically, not numerically.
 * It takes Spirit AST produced from FuncGrammar, 
 * translates it to own AST (based on OpTree),
 * calculates recursively derivatives, tries to simplify a bit all expressions
 * (these steps are performed in calculate_deriv()),
 * and then produces bytecode (AnyFormula::tree_to_bytecode())
 * that can be executed by VM (AnyFormula::run_vm()).
 * In case of $variables, values and derivatives are always calculated together.
 * In case of UDFs, when using non-derivative algorithm, 
 * it's significantly more efficient to skip calculation of derivatives,
 * that's what class AnyFormulaO was introduced for. 
 * Note, that UDF bytecode is executed n times more than $variable bytecode,
 * where n is the number of data points, and usually n >> 1.
 * 
 * \par
 * reusability
 *
 * There are small pieces of code (1 or a few files) designed to be reusable. 
 * They can have more liberal licence than GPL.
 *
 * - GUI input line, see: http://wxforum.shadonet.com/viewtopic.php?t=13005
 * - GUI FancyRealCtrl, see: http://wxforum.shadonet.com/viewtopic.php?t=13471
 * - optional-suffix-parser for Boost.Spirit in src/optional_suffix.h
 *   see also: http://article.gmane.org/gmane.comp.parsers.spirit.general/7797
 */

