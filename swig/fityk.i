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

// it can be wrapped only using typemaps
%ignore set_show_message;

#if defined(SWIGPYTHON)
    // str() is used in class Point and exceptions
    %rename(__str__) str();

    %include "file.i"

    %typemap(check) PyObject *pyfunc {
        if (!PyCallable_Check($1))
            SWIG_exception(SWIG_TypeError,"Expected function.");
    }
    %{
    PyObject *_py_show_message_func = NULL;
    static void PythonShowMessageCallBack(std::string const& s)
    {
        PyObject *arglist = Py_BuildValue("(s)", s.c_str());
        PyEval_CallObject(_py_show_message_func, arglist);
        Py_DECREF(arglist);
    }
    %}
    %extend fityk::Fityk {
        void py_set_show_message(PyObject *pyfunc) {
            if (_py_show_message_func != NULL)
                Py_DECREF(_py_show_message_func);
            _py_show_message_func = pyfunc;
            self->set_show_message(PythonShowMessageCallBack);
            Py_INCREF(pyfunc);
        }
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

    %typemap(in) FILE * {
        FILE **f;
        if (lua_isnil(L, $input))
            $1=NULL;
        else {
            f = (FILE **)luaL_checkudata(L, $input, "FILE*");
            if (*f == NULL)
                luaL_error(L, "attempt to use a closed file");
            $1=*f;
        }
    }

    // set_show_message can be probably wrapped using swig1.3/lua/lua_fnptr.i

#else
#warning \
    fityk.i was tested only with Python and Lua. If you use it with other \
    languages, or you had to modify it, please let me know - wojdyr@gmail.com
#endif

%apply FILE* { std::FILE* };
%include "fityk.h"

