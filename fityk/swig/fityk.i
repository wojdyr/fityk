// This file is part of fityk program.
// Licence: GNU General Public License version 2

// tests for bindings are in the samples/ directory

#if defined(SWIGPERL)
// Perl has convention of capitalized module names
%module Fityk
#elif defined(SWIGJAVA)
// module name fityk creates filename conflicts with Fityk class
// on case-insensitive filesystems
%module FitykModule
#else
%module fityk
#endif

%feature("autodoc", "1");

%{
#include "stddef.h" // bug workaround, needed for Swig1.3 + GCC4.6
#include "fityk.h"

/* this is necessary with swig <= 2.0.4 */
#if LUA_VERSION_NUM == 502
# define lua_strlen lua_rawlen
# define luaL_reg luaL_Reg
#endif

%}
%include "std_string.i"
%include "std_vector.i"
%include "std_except.i"
namespace std {
    %template(PointVector) vector<fityk::Point>;
    /* temporarily realt is replaced by double as a workaround of SWIG bug.
     * It's likely the same bug as in:
     * http://article.gmane.org/gmane.comp.programming.swig.devel/21772
     */
    //%template(RealVector) vector<realt>;
    %template(RealVector) vector<double>;
    %template(VarVector) vector<fityk::Var*>;
    %template(FuncVector) vector<fityk::Func*>;
}

// implementation, not api
%ignore get_ftk;
%ignore get_covariance_matrix_as_array;

#if defined(SWIGLUA) || defined(SWIGJAVA)
    namespace std
    {
        class runtime_error : public exception {
        public:
          explicit runtime_error (const string& what_arg);
        };

        class invalid_argument : public exception {
        public:
          explicit invalid_argument (const string& what_arg);
        };
    }
#endif

#if defined(SWIGPYTHON)
    %extend fityk::Point { std::string __str__() { return $self->str(); } }
    %extend fityk::SyntaxError
        { const char* __str__() { return $self->what(); } }
    %extend fityk::ExecuteError
        { const char* __str__() { return $self->what(); } }
    %include "file.i"

#elif defined(SWIGLUA)
    %typemap(throws) fityk::ExecuteError {
        lua_pushstring(L,$1.what()); SWIG_fail;
    }

    %typemap(throws) fityk::SyntaxError {
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

#elif defined(SWIGPERL)
    %typemap(in) FILE * {
        if (!SvTRUE($input))
            $1=NULL;
        else
            $1 = PerlIO_exportFILE(IoIFP(sv_2io($input)), NULL);
    }

    %typemap(throws) fityk::ExecuteError {
        std::string msg = std::string("Runtime error: ") + $1.what() + "\n ";
        croak(msg.c_str());
    }

    %typemap(throws) fityk::SyntaxError {
        std::string msg = std::string("Syntax error: ") + $1.what() + "\n ";
        croak(msg.c_str());
    }

#elif defined(SWIGRUBY)
    %extend fityk::Point { std::string to_s() { return $self->str(); } }
    %extend fityk::SyntaxError { const char* to_s() { return $self->what(); } }
    %extend fityk::ExecuteError { const char* to_s() { return $self->what(); } }
    %include "file.i"

#elif defined(SWIGJAVA)
    %rename(isLesser) operator<;

#else
#warning \
    fityk.i supports Python, Perl, Ruby and Lua.\
    If you use another language, please let me know - wojdyr@gmail.com
#endif

#if !defined(SWIGJAVA)
    %apply FILE* { std::FILE* };
#endif

/* ui_api.h is wrapped only by Python now, let me know if you'd like to use
 * it from another language.
 */
#if defined(SWIGPYTHON)
    namespace std {
        %template(StringVector) vector<string>;
    }
    %typemap(check) PyObject *pyfunc {
        if (!PyCallable_Check($1))
            SWIG_exception(SWIG_TypeError,"Expected function.");
    }

    %{
    #include "ui_api.h"

    PyObject *_py_show_message_func = NULL;
    static void PythonShowMessageCallBack(fityk::UiApi::Style style,
                                          std::string const& s)
    {
        PyObject *arglist = Py_BuildValue("(is)", style, s.c_str());
        PyEval_CallObject(_py_show_message_func, arglist);
        Py_DECREF(arglist);
    }

    PyObject *_py_draw_plot_func = NULL;
    static void PythonDrawPlotCallBack(fityk::UiApi::RepaintMode mode)
    {
        PyObject *arglist = Py_BuildValue("(i)", mode);
        PyEval_CallObject(_py_draw_plot_func, arglist);
        Py_DECREF(arglist);
    }
    %}

    %extend fityk::UiApi {
        void connect_show_message_py(PyObject *pyfunc) {
            if (_py_show_message_func != NULL)
                Py_DECREF(_py_show_message_func);
            _py_show_message_func = pyfunc;
            self->connect_show_message(PythonShowMessageCallBack);
            Py_INCREF(pyfunc);
        }

        void connect_draw_plot_py(PyObject *pyfunc) {
            if (_py_draw_plot_func != NULL)
                Py_DECREF(_py_draw_plot_func);
            _py_draw_plot_func = pyfunc;
            self->connect_draw_plot(PythonDrawPlotCallBack);
            Py_INCREF(pyfunc);
        }
    }

    %include "ui_api.h"
#else
    %ignore get_ui_api;
#endif

%include "fityk.h"

