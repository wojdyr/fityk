// This file is part of fityk program. 
// Licence: GNU General Public License version 2
// $Id$

// tests for bindings are in samples/ directory (*.py)

%module fityk
%feature("autodoc", "1");

%{
#include "fityk.h"
%}
%include "std_string.i"
%include "std_vector.i"
%include "std_except.i"
namespace std {
    %template(PointVector) vector<fityk::Point>;
    %template(DoubleVector) vector<double>;
}

// str() is used in class Point and exceptions
%rename(__str__) str();

// it's not easy to wrap this function
%ignore set_show_message;

#if defined(SWIGPYTHON)
    %typemap(in) std::FILE * {
        if (!PyFile_Check($input)) {
            PyErr_SetString(PyExc_TypeError, "expected PyFile");
            return NULL;
        }
        $1=PyFile_AsFile($input);
    }
#elif defined(SWIGLUA)
    namespace std
    {
        class runtime_error : public exception {
        public:
          explicit runtime_error (const string& what_arg);
        };
    }

    %typemap(throws) fityk::ExecuteError {
        lua_pushstring(L,$1.what()); SWIG_fail;
    }

#else
#warning \
    fityk.i was tested only with Python and Lua. If you use it with other \
    languages, or you had to modify it, please let me know - wojdyr@gmail.com
#endif

%include "fityk.h"


