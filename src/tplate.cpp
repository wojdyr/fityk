// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

#include "tplate.h"
#include "common.h"
#include "func.h"
#include "udf.h"
#include "bfunc.h"
#include "ast.h"


using namespace std;

string Tplate::as_formula() const
{
    string r = name + "(";
    for (size_t i = 0; i != pars.size(); ++i) {
        if (i != 0)
            r += ", ";
        r += pars[i];
        if (!defvals[i].empty())
            r += "=" + defvals[i];
    }
    r += ") = " + rhs;
    return r;
}

void TplateMgr::add(const char* name,
               const char* cs_pars, // comma-separated parameters
               const char* cs_dv,   // comma-separated default values
               const char* rhs,
               bool linear_d, bool peak_d,
               Tplate::create_type create)
{
    Tplate* t = new Tplate;
    t->name = name;
    t->pars = split_string(cs_pars, ',');
    t->defvals = split_string(cs_dv, ',');
    t->rhs = rhs;
    t->linear_d = linear_d;
    t->peak_d = peak_d;
    t->create = create;
    assert(t->pars.size() == t->defvals.size());
    tpvec_.push_back(Tplate::Ptr(t));
}

#define FACTORY_FUNC(NAME) \
Function* create_##NAME(const Ftk* F, const std::string& name, \
                      Tplate::Ptr tp, const std::vector<std::string>& vars) \
{ return new NAME(F, name, tp, vars); }

FACTORY_FUNC(FuncConstant)
FACTORY_FUNC(FuncLinear)
FACTORY_FUNC(FuncQuadratic)
FACTORY_FUNC(FuncCubic)
FACTORY_FUNC(FuncPolynomial4)
FACTORY_FUNC(FuncPolynomial5)
FACTORY_FUNC(FuncPolynomial6)
FACTORY_FUNC(FuncGaussian)
FACTORY_FUNC(FuncSplitGaussian)
FACTORY_FUNC(FuncLorentzian)
FACTORY_FUNC(FuncPearson7)
FACTORY_FUNC(FuncSplitPearson7)
FACTORY_FUNC(FuncPseudoVoigt)
FACTORY_FUNC(FuncVoigt)
FACTORY_FUNC(FuncVoigtA)
FACTORY_FUNC(FuncEMG)
FACTORY_FUNC(FuncDoniachSunjic)
FACTORY_FUNC(FuncPielaszekCube)
FACTORY_FUNC(FuncLogNormal)
FACTORY_FUNC(FuncSpline)
FACTORY_FUNC(FuncPolyline)

FACTORY_FUNC(CustomFunction)
FACTORY_FUNC(CompoundFunction)
FACTORY_FUNC(SplitFunction)

void TplateMgr::add_builtin_types()
{
    tpvec_.reserve(32);

    //-------------------  coded in bfunc.cpp  ---------------------

    add("Constant", "a", "avgy",
        "a",
        /*linear_d=*/true, /*peak_d=*/false, &create_FuncConstant);

    add("Linear", "a0,a1", "intercept,slope",
        "a0 + a1 * x",
        /*linear_d=*/true, /*peak_d=*/false, &create_FuncLinear);

    add("Quadratic", "a0,a1,a2", "avgy,0,0",
        "a0 + a1*x + a2*x^2",
        /*linear_d=*/true, /*peak_d=*/false, &create_FuncQuadratic);

    add("Cubic", "a0,a1,a2,a3", "avgy,0,0,0",
        "a0 + a1*x + a2*x^2 + a3*x^3",
        /*linear_d=*/true, /*peak_d=*/false, &create_FuncCubic);

    add("Polynomial4", "a0,a1,a2,a3,a4", "avgy,0,0,0,0",
        "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4",
        /*linear_d=*/true, /*peak_d=*/false, &create_FuncPolynomial4);

    add("Polynomial5", "a0,a1,a2,a3,a4,a5", "avgy,0,0,0,0,0",
        "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5",
        /*linear_d=*/true, /*peak_d=*/false, &create_FuncPolynomial5);

    add("Polynomial6", "a0,a1,a2,a3,a4,a5,a6", "avgy,0,0,0,0,0,0",
        "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5 + a6*x^6",
        /*linear_d=*/true, /*peak_d=*/false, &create_FuncPolynomial6);

    add("Gaussian", "height,center,hwhm", ",,",
        "height*exp(-ln(2)*((x-center)/hwhm)^2)",
        /*linear_d=*/false, /*peak_d=*/true, &create_FuncGaussian);

    add("SplitGaussian", "height,center,hwhm1,hwhm2", ",,hwhm,hwhm",
    "x<center ? Gaussian(height,center,hwhm1) : Gaussian(height,center,hwhm2)",
        /*linear_d=*/false, /*peak_d=*/true, &create_FuncSplitGaussian);

    add("Lorentzian", "height,center,hwhm", ",,",
        "height/(1+((x-center)/hwhm)^2)",
        /*linear_d=*/false, /*peak_d=*/true, &create_FuncLorentzian);

    add("Pearson7", "height,center,hwhm,shape", ",,,2",
        "height/(1+((x-center)/hwhm)^2*(2^(1/shape)-1))^shape",
        /*linear_d=*/false, /*peak_d=*/true, &create_FuncPearson7);

    add("SplitPearson7",
        "height,center,hwhm1,hwhm2,shape1,shape2", ",,hwhm,hwhm,2,2",
        "x < center ? Pearson7(height, center, hwhm1, shape1)"
                  " : Pearson7(height, center, hwhm2, shape2)",
        /*linear_d=*/false, /*peak_d=*/true, &create_FuncSplitPearson7);

    add("PseudoVoigt", "height,center,hwhm,shape", ",,,0.5",
        "height*((1-shape)*exp(-ln(2)*((x-center)/hwhm)^2)"
                            "+shape/(1+((x-center)/hwhm)^2))",
        /*linear_d=*/false, /*peak_d=*/true, &create_FuncPseudoVoigt);

    add("Voigt", "height,center,gwidth,shape", ",,hwhm*0.8,0.1",
        "convolution of Gaussian and Lorentzian #",
        /*linear_d=*/false, /*peak_d=*/true, &create_FuncVoigt);

    add("VoigtA", "area,center,gwidth,shape", ",,hwhm*0.8,0.1",
        "convolution of Gaussian and Lorentzian #",
        /*linear_d=*/false, /*peak_d=*/true, &create_FuncVoigtA);

    add("EMG", "a,b,c,d", "height,center,hwhm*0.8,hwhm*0.08",
           "a*c*(2*pi)^0.5/(2*d) * exp((b-x)/d + c^2/(2*d^2))"
           " * (abs(d)/d - erf((b-x)/(2^0.5*c) + c/(2^0.5*d)))",
        /*linear_d=*/false, /*peak_d=*/true, &create_FuncEMG);

    add("DoniachSunjic", "h,a,f,e", "height,0.1,1,center",
       "h * cos(pi*a/2 + (1-a)*atan((x-e)/f)) / (f^2+(x-e)^2)^((1-a)/2)",
        /*linear_d=*/false, /*peak_d=*/true, &create_FuncDoniachSunjic);

    add("PielaszekCube", "a,center,r,s", "height*0.016,,300,150",
        "...#",
        /*linear_d=*/false, /*peak_d=*/true, &create_FuncPielaszekCube);

    add("LogNormal", "height,center,width,asym", ",,2*hwhm,0.1",
        "height*exp(-ln(2)*(ln(2.0*asym*(x-center)/width+1)/asym)^2)",
        /*linear_d=*/false, /*peak_d=*/true, &create_FuncLogNormal);

    add("Spline", "", "",
        "cubic spline #",
        /*linear_d=*/false, /*peak_d=*/false, &create_FuncSpline);

    add("Polyline", "", "",
        "linear interpolation #",
        /*linear_d=*/false, /*peak_d=*/false, &create_FuncPolyline);


    //------------------- interpreted functions ---------------------

    add("ExpDecay", "a,t", "0,1",
        "a*exp(-x/t)",
        /*linear_d=*/false, /*peak_d=*/false, &create_CustomFunction);

    add("GaussianA", "area,center,hwhm", ",,",
        "Gaussian(area/hwhm/sqrt(pi/ln(2)), center, hwhm)",
        /*linear_d=*/false, /*peak_d=*/true, &create_CompoundFunction);

    add("LogNormalA", "area,center,width,asym", ",,2*hwhm,0.1",
        "LogNormal(sqrt(ln(2)/pi)*(2*area/width)*exp(-asym^2/4/ln(2)), "
                   "center, width, asym)",
        /*linear_d=*/false, /*peak_d=*/true, &create_CompoundFunction);

    add("LorentzianA", "area,center,hwhm", ",,",
        "Lorentzian(area/hwhm/pi, center, hwhm)",
        /*linear_d=*/false, /*peak_d=*/true, &create_CompoundFunction);

    add("Pearson7A", "area,center,hwhm,shape", ",,,2",
        "Pearson7(area/(hwhm*exp(lgamma(shape-0.5)-lgamma(shape))"
                        "*sqrt(pi/(2^(1/shape)-1))), "
                 "center, hwhm, shape)",
        /*linear_d=*/false, /*peak_d=*/true, &create_CompoundFunction);

    add("PseudoVoigtA",
        "area,center,hwhm,shape",
        ",,,0.5",
        "GaussianA(area*(1-shape), center, hwhm)"
         " + LorentzianA(area*shape, center, hwhm)",
        /*linear_d=*/false, /*peak_d=*/true, &create_CompoundFunction);

    add("SplitLorentzian", "height,center,hwhm1,hwhm2", ",,hwhm,hwhm",
        "x < center ? Lorentzian(height, center, hwhm1)"
                  " : Lorentzian(height, center, hwhm2)",
        /*linear_d=*/false, /*peak_d=*/true, &create_SplitFunction);

    add("SplitPseudoVoigt",
        "height,center,hwhm1,hwhm2,shape1,shape2", ",,hwhm,hwhm,0.5,0.5",
        "x < center ? PseudoVoigt(height, center, hwhm1, shape1)"
                  " : PseudoVoigt(height, center, hwhm2, shape2)",
        /*linear_d=*/false, /*peak_d=*/true, &create_SplitFunction);

    add("SplitVoigt",
        "height,center,hwhm1,hwhm2,shape1,shape2", ",,hwhm,hwhm,0.5,0.5",
        "x < center ? Voigt(height, center, hwhm1, shape1)"
                  " : Voigt(height, center, hwhm2, shape2)",
        /*linear_d=*/false, /*peak_d=*/true, &create_SplitFunction);
}

namespace {


vector<OpTree*> make_op_trees(const Tplate* tp)
{
    string rhs = tp->rhs;
    tree_parse_info<> info = ast_parse(rhs.c_str(), FuncG >> end_p, space_p);
    assert(info.full);
    vector<string> vars = find_tokens_in_ptree(FuncGrammar::variableID, info);
    vector<string> lhs_vars = tp->pars;
    lhs_vars.push_back("x");
    vector_foreach (string, i, vars)
        if (find(lhs_vars.begin(), lhs_vars.end(), *i) == lhs_vars.end())
            throw ExecuteError("variable `" + *i
                                           + "' only at the right hand side.");
    vector<OpTree*> op_trees = calculate_deriv(info.trees.begin(), lhs_vars);
    return op_trees;
}

} // anonymous namespace

/*
///check for errors in function at RHS
void TplateMgr::check_cpd_rhs_function(const string& fun,
                                       const vector<string>& lhs_vars)
{
    //check if component function is known
    string t = Function::get_typename_from_formula(fun);
    string tf = get_formula(t);
    if (tf.empty())
        throw ExecuteError("definition based on undefined function `" + t +"'");
    //...and if it has proper number of parameters
    vector<string> tvars = Function::get_varnames_from_formula(tf);
    vector<string> gvars = Function::get_varnames_from_formula(fun);
    if (tvars.size() != gvars.size())
        throw ExecuteError("Function `" + t + "' requires "
                                      + S(tvars.size()) + " parameters.");
    // ... and check these parameters
    vector_foreach (string, j, gvars) {
        tree_parse_info<> info = ast_parse(j->c_str(), FuncG >> end_p, space_p);
        assert(info.full);
        vector<string> vars=find_tokens_in_ptree(FuncGrammar::variableID, info);
        if (contains_element(vars, "x"))
            throw ExecuteError("variable can not depend on x.");
        vector_foreach (string, k, vars)
            if ((*k)[0] != '~' && (*k)[0] != '{' && (*k)[0] != '$'
                    && (*k)[0] != '%' && !contains_element(lhs_vars, *k))
                throw ExecuteError("Improper variable given in parameter "
                              + S(j-gvars.begin()+1) + " of "+ t + ": " + *k);
    }
}
*/


/*
UDF::UDF(string const& formula_, bool is_builtin_)
    : formula(formula_)
{
    name = Function::get_typename_from_formula(formula_);
    type = get_udf_type(formula_);
    if (type == UDF::kCustom)
        op_trees = make_op_trees(formula);
}
*/

TplateMgr::TplateMgr()
{
    add_builtin_types();
}

/*
void TplateMgr::check_fudf_rhs(string const& rhs,
                                  vector<string> const& lhs_vars)
{
    if (rhs.empty())
        throw ExecuteError("No formula");
    tree_parse_info<> info = ast_parse(rhs.c_str(), FuncG >> end_p, space_p);
    if (!info.full)
        throw ExecuteError("Syntax error in formula");
    vector<string> vars = find_tokens_in_ptree(FuncGrammar::variableID, info);
    vector_foreach (string, i, vars)
        if (*i != "x" && !contains_element(lhs_vars, *i)) {
            throw ExecuteError("Unexpected parameter in formula: " + *i);
        }
    vector_foreach (string, i, lhs_vars)
        if (!contains_element(vars, *i)) {
            throw ExecuteError("Unused parameter in formula: " + *i);
        }
}

    if (contains_element(tp->pars, "x"))
        throw ExecuteError("no need to put explicitly x as parameter.");
*/

void TplateMgr::define(Tplate::Ptr tp)
{
    if (get_tp(tp->name) != NULL)
        throw ExecuteError(tp->name
                           + " is already defined. (undefine it first)");
    tpvec_.push_back(tp);
}

void TplateMgr::undefine(string const &type)
{
    vector<Tplate::Ptr>::iterator iter;
    for (iter = tpvec_.begin(); iter != tpvec_.end(); ++iter)
        if ((*iter)->name == type)
            break;
    if (iter == tpvec_.end())
        throw ExecuteError(type + " is not defined");
    if (!iter->unique())
        throw ExecuteError(type + " is currently used.");
    tpvec_.erase(iter);
}

const Tplate* TplateMgr::get_tp(string const &type) const
{
    vector_foreach (Tplate::Ptr, i, tpvec_)
        if ((*i)->name == type)
            return i->get();
    return NULL;
}

Tplate::Ptr TplateMgr::get_shared_tp(string const &type) const
{
    vector_foreach (Tplate::Ptr, i, tpvec_)
        if ((*i)->name == type)
            return *i;
    return Tplate::Ptr();
}

