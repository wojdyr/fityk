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

void ParameterizedFunction::prepare_parameters(vector<Point> const& points)
{
    for (map<int, vector<int> >::const_iterator i = pcodes.begin();
            i != pcodes.end(); ++i) {
        vector<int> code_ = i->second;
        fp v = get_transform_expr_value(code_, points);
        assert(is_index(i->first, params));
        params[i->first] = v;
    }
    do_prepare();
}

//----------------------  Parameterized Functions -------------------------
class InterpolateFunction : public ParameterizedFunction
{
public:
    InterpolateFunction(vector<fp> const& params_,
                        map<int, vector<int> > const& pcodes_)
        : ParameterizedFunction(params_, pcodes_) {}

    void do_prepare() {
        for (int i = 0; i < size(params) - 1; i += 2)
            bb.push_back(PointD(params[i], params[i+1]));
    }

    fp calculate(fp x) { return get_linear_interpolation(bb, x); }

    const char *get_name() const { return "interpolate"; }

private:
    std::vector<PointD> bb;
};


class SplineFunction : public ParameterizedFunction
{
public:
    SplineFunction(vector<fp> const& params_,
                   map<int, vector<int> > const& pcodes_)
        : ParameterizedFunction(params_, pcodes_) {}

    void do_prepare() {
        for (int i = 0; i < size(params) - 1; i += 2)
            bb.push_back(PointQ(params[i], params[i+1]));
        prepare_spline_interpolation(bb);
    }

    fp calculate(fp x) { return get_spline_interpolation(bb, x); }

    const char *get_name() const { return "spline"; }

private:
    std::vector<PointQ> bb;
};

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

void parameterized_op::push() const
{
    //DT_DEBUG("PARAMETERIZED " + S(op))
    typedef vector<int>::iterator viit;
    const viit plbegin = find(code.begin(), code.end(), OP_PLIST_BEGIN);
    const viit plend = find(code.begin(), code.end(), OP_PLIST_END) + 1;
    if (find(plbegin+1, plend, OP_PLIST_BEGIN) != plend)
        throw ExecuteError("Parametrized functions can not be nested.");
    vector<fp> params;
    map<int, vector<int> > pcodes; //codes for parameters of eg. spline
    viit start = plbegin+1;
    while (start != plend) {
        viit finish = find(start, plend, OP_PLIST_SEP);
        if (finish == plend)
            finish = plend-1; // at OP_PLIST_END
        if (finish == start+2 && *start == OP_NUMBER) { //the most common case
            params.push_back(numbers[*(start+1)]);
        }
        else {
            pcodes[params.size()] = vector<int>(start, finish);
            params.push_back(0.); //placeholder
        }
        start = finish+1;
    }
    code.erase(plbegin, plend);
    code.push_back(OP_PARAMETERIZED);
    code.push_back(size(parameterized));
    ParameterizedFunction *func = 0;
    switch (op) {
        case PF_INTERPOLATE:
            //TODO shared_ptr
            func = new InterpolateFunction(params, pcodes);
            break;
        case PF_SPLINE:
            func = new SplineFunction(params, pcodes);
            break;
        default:
            assert(0);
    }
    parameterized.push_back(func);
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
                | (str_p("==") >> rprec2)  [push_op(OP_EQ)]
                | ((str_p("!=")|"<>") >> rprec2)  [push_op(OP_NEQ)]
                | ('<' >> rprec2)  [push_op(OP_LT)]
                | ('>' >> rprec2)  [push_op(OP_GT)]
                )
                >> *( (str_p("<=")[push_op(OP_AND, OP_NCMP_HACK)]
                        >> rprec2) [push_op(OP_LE, OP_AFTER_AND)]
                    | (str_p(">=")[push_op(OP_AND, OP_NCMP_HACK)]
                        >> rprec2) [push_op(OP_GE, OP_AFTER_AND)]
                    | (str_p("==")[push_op(OP_AND, OP_NCMP_HACK)]
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


