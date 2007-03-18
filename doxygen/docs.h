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
 * Fityk is using Boost::Spirit for parsing commands and data transformations.
 * The grammar of commands is in cmd* files. Unfortunatelly, it was necessary
 * to split grammar into several files. Otherwise, compilation would require
 * gigabytes of memory. Some grammar rules are repeated in two or more files.
 * Data transformation grammar can be found in datatrans* files.
 *
 */

