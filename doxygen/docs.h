/*! \mainpage Documentation of Fityk Source 
 *
 * \par
 * This is a documentation of source for developers. 
 * If you just want to use the program, see 
 * <a href="http://www.unipress.waw.pl/fityk/doc/html/index.html">
 * user's manual</a>.
 * \par
 * Most of the source doesn't have doxygen comments yet, but I'll be adding
 * it when writting new code or refactoring old one.
 * \par
 * You may also look at 
 * <a href="http://cvs.sourceforge.net/viewcvs.py/fityk/fityk/TODO?view=markup">
 * TODO</a> file.
 * \par
 * Fityk is using Boost::Spirit for parsing commands and data transformations.
 * The grammar of commands is in cmd* files. Unfortunatelly, it was necessary
 * to split grammar into several files. Otherwise, compilation would require
 * gigabytes of memory. Some grammar rules are repeated in two or more files.
 * Data transformation grammar can be found in datatrans* files.
 *
 * \par
 * Marcin Wojdyr
 */


// A dummy STL definition for doxygen 
namespace std
{
    template<class T> class vector { public: T data; };
    template<class T> class deque { public: T data; };
    template<class T> class list { public: T data; };
    template<class T> class slist { public: T data; };
    template<class Key, class Data> class map { public: Key key; Data data; };
} 

