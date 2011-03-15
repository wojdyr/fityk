/*! \mainpage Documentation of libfityk internals. 
 *
 * This is a documentation of source for developers. 
 *
 * If you want to use libfityk in your program, first see
 *  the namespace #fityk, where the public API is documented.
 *
 * \b directories
 * - src/ - fityk library (libfityk)
 * - src/wxgui/ - wxWidgets-based GUI
 * - src/cli/   - readline/gnuplot based CLI
 * - swig/   - bindings to libfityk
 *
 * <b>symbolic calculation of derivatives</b>
 *
 * (can be outdated)
 * Fityk calculates derivatives of functions symbolically.
 * It parses expression to AST based on struct OpTree,
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
 * \b reusability
 *
 * There are small pieces of code designed to be reusable. 
 * They can have more liberal licence than GPL.
 *
 * - GUI input line, see: http://wxforum.shadonet.com/viewtopic.php?t=13005
 * - controls for input of real numbers in wxgui/fancyrc.* 
 *   (see outdated post at http://wxforum.shadonet.com/viewtopic.php?t=13471)
 */

