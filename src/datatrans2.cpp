// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

// big grammars in Spirit take a lot of time and memory to compile
// so they must be splitted into separate compilation units
// that's the only reason why this file is not a part of datatrans.cpp

#include "datatrans2.h"
using namespace datatrans;

namespace datatrans {

//----------------------  Parameterized Functions -------------------------
class InterpolateFunction : public ParameterizedFunction
{
public:
    InterpolateFunction(vector<fp> &params) {
        for (int i=0; i < size(params) - 1; i+=2)
            bb.push_back(B_point(params[i], params[i+1]));
    }

    fp calculate(fp x) { return get_linear_interpolation(bb, x); }

private:
    std::vector<B_point> bb;
};


class SplineFunction : public ParameterizedFunction
{
public:
    SplineFunction(vector<fp> &params) {
        for (int i=0; i < size(params) - 1; i+=2)
            bb.push_back(B_point(params[i], params[i+1]));
        prepare_spline_interpolation(bb);
    }

    fp calculate(fp x) { return get_spline_interpolation(bb, x); }

private:
    std::vector<B_point> bb;
};

//----------------------   functors   -------------------------

void push_double::operator()(const double& n) const
{
    code.push_back(OP_NUMBER);
    code.push_back(size(numbers));
    numbers.push_back(n);
}

void push_op::push() const 
{ 
    code.push_back(op); 
    if (op2)
        code.push_back(op2); 
}

void parameterized_op::push() const
{ 
    //DT_DEBUG("PARAMETERIZED " + S(op))  
    typedef vector<int>::iterator viit;
    viit first = find(code.begin(), code.end(), OP_PLIST_BEGIN);
    viit last = find(code.begin(), code.end(), OP_PLIST_END) + 1;
    vector<fp> params;
    for (viit i=first; i != last; ++i) 
        if (*i == OP_NUMBER) 
            params.push_back(numbers[*++i]);
    code.erase(first, last);
    code.push_back(OP_PARAMETERIZED); 
    code.push_back(size(parameterized));
    ParameterizedFunction *func = 0;
    switch (op) {
        case PF_INTERPOLATE:
            //TODO shared_ptr
            func = new InterpolateFunction(params);
            break;
        case PF_SPLINE:
            func = new SplineFunction(params);
            break;
        default:
            assert(0);
    }
    parameterized.push_back(func);
}

void push_the_func::operator()(char const* a, char const* b) const
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

} //namespace

template <typename ScannerT>
DataExpressionGrammar::definition<ScannerT>::definition(
                                          DataExpressionGrammar const& /*self*/)
{
    rprec5 
        =   DataE2G
        |   ('-' >> DataE2G) [push_op(OP_NEG)]
        |   ('+' >> DataE2G)
        ;

    rprec4 
        =   rprec5 
            >> *(
                  ('^' >> rprec5) [push_op(OP_POW)]
                )
        ;
        
    rprec3 
        =   rprec4
            >> *(   ('*' >> rprec4)[push_op(OP_MUL)]
                |   ('/' >> rprec4)[push_op(OP_DIV)]
                |   ('%' >> rprec4)[push_op(OP_MOD)]
                )
        ;

    rprec2 
        =   rprec3
            >> *(   ('+' >> rprec3) [push_op(OP_ADD)]
                |   ('-' >> rprec3) [push_op(OP_SUB)]
                )
        ;

    rbool
    // a hack for handling 10 < x < 20 
    // how it works:
    //  3 < 5.2
    //    op:    OP_NUMBER(3)    OP_NUMBER(5.2)   OP_LT
    //    stack:    ... 3         ... 3 5.2      ... 1 5.2
    //    stack end:    ^                 ^          ^
    //  3 < 5.2 < 4 (continued previous example)
    //    op:     OP_AND     OP_NCMP_HACK  OP_NUMBER(4) OP_LT  OP_AFTER_AND
    //    stack: ... 1 5.2   ... 5.2       ... 5.2 4   ... 0 4     (flag)
    //    s. end:  ^               ^               ^       ^  
        = rprec2
           >> !(( ("<=" >> rprec2) [push_op(OP_LE)]
                | (">=" >> rprec2) [push_op(OP_GE)]
                | ((str_p("==")|"=") >> rprec2)  [push_op(OP_EQ)]
                | ((str_p("!=")|"<>") >> rprec2)  [push_op(OP_NEQ)]
                | ('<' >> rprec2)  [push_op(OP_LT)]
                | ('>' >> rprec2)  [push_op(OP_GT)]
                )
                >> *( (str_p("<=")[push_op(OP_AND, OP_NCMP_HACK)] 
                        >> rprec2) [push_op(OP_LE, OP_AFTER_AND)]
                    | (str_p(">=")[push_op(OP_AND, OP_NCMP_HACK)] 
                        >> rprec2) [push_op(OP_GE, OP_AFTER_AND)]
                    | ((str_p("==")|"=")[push_op(OP_AND, OP_NCMP_HACK)] 
                        >> rprec2) [push_op(OP_EQ, OP_AFTER_AND)]
                    | ((str_p("!=")|"<>")[push_op(OP_AND, OP_NCMP_HACK)] 
                        >> rprec2)  [push_op(OP_NEQ, OP_AFTER_AND)]
                    | (ch_p('<')[push_op(OP_AND, OP_NCMP_HACK)] 
                        >> rprec2) [push_op(OP_LT, OP_AFTER_AND)]
                    | (ch_p('>')[push_op(OP_AND, OP_NCMP_HACK)] 
                        >> rprec2) [push_op(OP_GT, OP_AFTER_AND)]
                    )
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


