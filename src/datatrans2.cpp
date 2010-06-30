// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

/// big grammars in Spirit take a lot of time and memory to compile
/// so they must be splitted into separate compilation units
/// that's the only reason why this file is not a part of datatrans.cpp

#include "datatrans2.h"
#include "datatrans3.h"
#include "model.h"
#include "fit.h"
using namespace datatrans;

namespace datatrans {

//----------------------   functors   -------------------------

void push_double::operator()(const double& d) const
{
    code.push_back(OP_NUMBER);
    code.push_back(size(numbers));
    numbers.push_back(d);
}

void push_op::push() const
{
    code.push_back(op);
    if (op2)
        code.push_back(op2);
}


#ifndef STANDALONE_DATATRANS
void push_func::operator()(char const* a, char const* b) const
{
    string t(a, b);
    if (t[0] == '%') {
        string fstr = strip_string(string(t, 1, t.find_first_of("(,")-1));
        int n = AL->find_function_nr(fstr);
        if (n == -1)
            throw ExecuteError("undefined function: %" + fstr);
        code.push_back(OP_FUNC);
        code.push_back(n);
    }
    else {
        int n = -1;
        if (t[0] == '@') {
            int dot = t.find('.');
            n = strtol(string(t,1,dot).c_str(), 0, 10);
            t = strip_string(string(t, dot+1));
        }
        if (t[0] == 'F')
            code.push_back(OP_SUM_F);
        else if (t[0] == 'Z')
            code.push_back(OP_SUM_Z);
        else
            assert(0);
        code.push_back(n);
    }
}

void push_var::operator()(char const* a, char const* b) const
{
    // string(a,b) is either $foo or $foo.error
    char const* dot = find(a, b, '.');
    Variable const* var = AL->find_variable(string(a+1, dot));
    fp value;
    if (dot == b) // variable
        value = var->get_value();
    else  // ".error"
        value = AL->get_fit_container()->get_standard_error(var);
    push_double::operator()(value);
}

void push_func_param::operator()(char const* a, char const* b) const
{
    string t(a, b);
    string::size_type error = t.find(".error");
    if (error != string::npos)
        t = t.erase(error);
    int dot = t.rfind(".");
    string fstr = strip_string(string(t, 0, dot));
    string pstr = strip_string(string(t, dot+1));
    Function const* f = AL->find_function_any(fstr);
    fp value;
    if (error == string::npos) // variable
        value = f->get_param_value(pstr);
    else { // ".error"
        if (!islower(pstr[0]))
            throw ExecuteError("Errors of pseudo-parameters (" + pstr
                               + ") can not be accessed. ");
        string varname = f->get_var_name(f->get_param_nr(pstr));
        Variable const* var = AL->find_variable(varname);
        value = AL->get_fit_container()->get_standard_error(var);
    }
    push_double::operator()(value);
}
#endif //not STANDALONE_DATATRANS

} //namespace

template <typename ScannerT>
DataExpressionGrammar::definition<ScannerT>::definition(
                                          DataExpressionGrammar const& /*self*/)
{
    rprec5
        =   DataE2G
            >> *(
                  ('^' >> DataE2G) [push_op(OP_POW)]
                )
        ;

    rprec4
        =   ('-' >> rprec5) [&push_neg_op]
        |   (!ch_p('+') >> rprec5)
        ;

    rprec3
        =   rprec4
            >> *(   ('*' >> rprec4)[push_op(OP_MUL)]
                |   ('/' >> rprec4)[push_op(OP_DIV)]
                )
        ;

    rprec2
        =   rprec3
            >> *(   ('+' >> rprec3) [push_op(OP_ADD)]
                |   ('-' >> rprec3) [push_op(OP_SUB)]
                )
        ;

    rbool
        = rprec2
           >> !( ("<=" >> rprec2) [push_op(OP_LE)]
               | (">=" >> rprec2) [push_op(OP_GE)]
               | (str_p("==") >> rprec2)  [push_op(OP_EQ)]
               | ((str_p("!=")|"<>") >> rprec2)  [push_op(OP_NEQ)]
               | ('<' >> rprec2)  [push_op(OP_LT)]
               | ('>' >> rprec2)  [push_op(OP_GT)]
               )
        ;

    rbool_not
        =  (as_lower_d["not"] >> rbool) [push_op(OP_NOT)]
        |  rbool
        ;

    rbool_and
        =  rbool_not
           >> *(
                as_lower_d["and"] [push_op(OP_AND)]
                >> rbool_not
               ) [push_op(OP_AFTER_AND)]
        ;

    rbool_or
        =  rbool_and
           >> *(
                as_lower_d["or"] [push_op(OP_OR)]
                >> rbool_and
               ) [push_op(OP_AFTER_OR)]
        ;


    rprec1
        =  rbool_or
           >> !(ch_p('?') [push_op(OP_TERNARY)]
                >> rbool_or
                >> ch_p(':') [push_op(OP_TERNARY_MID)]
                >> rbool_or
               ) [push_op(OP_AFTER_TERNARY)]
        ;
}

// explicit template instantiations
template DataExpressionGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(DataExpressionGrammar const&);

template DataExpressionGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<no_actions_action_policy<action_policy> > > > >::definition(DataExpressionGrammar const&);

template DataExpressionGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, action_policy> > >::definition(DataExpressionGrammar const&);

DataExpressionGrammar DataExpressionG;


