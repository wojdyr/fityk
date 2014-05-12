// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "tplate.h"

#include <assert.h>    // for assert

#include "common.h"
#include "func.h"
#include "udf.h"
#include "bfunc.h"
#include "f_fcjasym.h"
#include "lexer.h"
#include "cparser.h"
#include "eparser.h"
#include "guess.h"

using namespace std;

namespace fityk {

string Tplate::as_formula() const
{
    string r = name + "(";
    for (size_t i = 0; i != fargs.size(); ++i) {
        if (i != 0)
            r += ", ";
        r += fargs[i];
        if (!defvals[i].empty())
            r += "=" + defvals[i];
    }
    r += ") = " + rhs;
    return r;
}

bool Tplate::is_coded() const
{
    return create != create_CompoundFunction &&
           create != create_SplitFunction &&
           create != create_CustomFunction &&
           create != NULL; // return false for empty Tplate
}

vector<string> Tplate::get_missing_default_values() const
{
    vector<string> gkeys;
    if (traits & kLinear)
        gkeys.insert(gkeys.end(), Guess::linear_traits.begin(),
                                  Guess::linear_traits.end());
    if (traits & kPeak)
        gkeys.insert(gkeys.end(), Guess::peak_traits.begin(),
                                  Guess::peak_traits.end());
    if (traits & kSigmoid)
        gkeys.insert(gkeys.end(), Guess::sigmoid_traits.begin(),
                                  Guess::sigmoid_traits.end());
    ExpressionParser ep(NULL);
    vector<string> missing;
    for (size_t i = 0; i < fargs.size(); ++i) {
        string dv = defvals[i].empty() ? fargs[i] : defvals[i];
        ep.clear_vm();
        Lexer lex(dv.c_str());
        ep.parse_expr(lex, 0, &gkeys, &missing);
    }
    return missing;
}

#define FACTORY_FUNC(NAME) \
Function* create_##NAME(const Settings* settings, const std::string& name, \
                      Tplate::Ptr tp, const std::vector<std::string>& vars) \
{ return new NAME(settings, name, tp, vars); }

static FACTORY_FUNC(FuncConstant)
static FACTORY_FUNC(FuncLinear)
static FACTORY_FUNC(FuncQuadratic)
static FACTORY_FUNC(FuncCubic)
static FACTORY_FUNC(FuncPolynomial4)
static FACTORY_FUNC(FuncPolynomial5)
static FACTORY_FUNC(FuncPolynomial6)
static FACTORY_FUNC(FuncGaussian)
static FACTORY_FUNC(FuncSplitGaussian)
static FACTORY_FUNC(FuncLorentzian)
static FACTORY_FUNC(FuncPearson7)
static FACTORY_FUNC(FuncSplitPearson7)
static FACTORY_FUNC(FuncPseudoVoigt)
static FACTORY_FUNC(FuncVoigt)
static FACTORY_FUNC(FuncVoigtA)
static FACTORY_FUNC(FuncEMG)
static FACTORY_FUNC(FuncDoniachSunjic)
static FACTORY_FUNC(FuncPielaszekCube)
static FACTORY_FUNC(FuncLogNormal)
static FACTORY_FUNC(FuncSpline)
static FACTORY_FUNC(FuncPolyline)
static FACTORY_FUNC(FuncFCJAsymm)

FACTORY_FUNC(CustomFunction)
FACTORY_FUNC(CompoundFunction)
FACTORY_FUNC(SplitFunction)


void TplateMgr::add(const char* name,
               const char* cs_fargs, // comma-separated parameters
               const char* cs_dv,    // comma-separated default values
               const char* rhs,
               int traits,
               Tplate::create_type create,
               Parser* parser,
               bool documented)
{
    Tplate* tp = new Tplate;
    tp->name = name;
    if (cs_fargs[0] != '\0') {
        tp->fargs = split_string(cs_fargs, ',');
        tp->defvals = split_string(cs_dv, ',');
    }
    tp->rhs = rhs;
    tp->traits = traits;
    tp->create = create;
    tp->docs_fragment = documented ? name : NULL;
    assert(tp->fargs.size() == tp->defvals.size());
    tpvec_.push_back(Tplate::Ptr(tp));

    if (parser) {
        Lexer lex(rhs);
        parser->parse_define_rhs(lex, tp);
    }
}


void TplateMgr::add_builtin_types(Parser* p)
{
    tpvec_.reserve(32);

    //-------------------  coded in bfunc.cpp  ---------------------

    add("Constant", "a", "avgy",
        "a",
        Tplate::kLinear, &create_FuncConstant);

    add("Linear", "a0,a1", "intercept,slope",
        "a0 + a1 * x",
        Tplate::kLinear, &create_FuncLinear);

    add("Quadratic", "a0,a1,a2", "intercept,slope,0",
        "a0 + a1*x + a2*x^2",
        Tplate::kLinear, &create_FuncQuadratic);

    add("Cubic", "a0,a1,a2,a3", "intercept,slope,0,0",
        "a0 + a1*x + a2*x^2 + a3*x^3",
        Tplate::kLinear, &create_FuncCubic);

    add("Polynomial4", "a0,a1,a2,a3,a4", "intercept,slope,0,0,0",
        "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4",
        Tplate::kLinear, &create_FuncPolynomial4);

    add("Polynomial5", "a0,a1,a2,a3,a4,a5", "intercept,slope,0,0,0,0",
        "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5",
        Tplate::kLinear, &create_FuncPolynomial5, NULL, true);

    add("Polynomial6", "a0,a1,a2,a3,a4,a5,a6", "intercept,slope,0,0,0,0,0",
        "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5 + a6*x^6",
        Tplate::kLinear, &create_FuncPolynomial6);

    add("Gaussian", "height,center,hwhm", ",,",
        "height*exp(-ln(2)*((x-center)/hwhm)^2)",
        Tplate::kPeak, &create_FuncGaussian, NULL, true);

    add("SplitGaussian", "height,center,hwhm1,hwhm2", ",,hwhm,hwhm",
    "x<center ? Gaussian(height,center,hwhm1) : Gaussian(height,center,hwhm2)",
        Tplate::kPeak, &create_FuncSplitGaussian, NULL, true);

    add("Lorentzian", "height,center,hwhm", ",,",
        "height/(1+((x-center)/hwhm)^2)",
        Tplate::kPeak, &create_FuncLorentzian, NULL, true);

    add("Pearson7", "height,center,hwhm,shape", ",,,2",
        "height/(1+((x-center)/hwhm)^2*(2^(1/shape)-1))^shape",
        Tplate::kPeak, &create_FuncPearson7, NULL, true);

    add("SplitPearson7",
        "height,center,hwhm1,hwhm2,shape1,shape2", ",,hwhm,hwhm,2,2",
        "x < center ? Pearson7(height, center, hwhm1, shape1)"
                  " : Pearson7(height, center, hwhm2, shape2)",
        Tplate::kPeak, &create_FuncSplitPearson7, NULL, true);

    add("PseudoVoigt", "height,center,hwhm,shape", ",,,0.5",
        "height*((1-shape)*exp(-ln(2)*((x-center)/hwhm)^2)"
                            "+shape/(1+((x-center)/hwhm)^2))",
        Tplate::kPeak, &create_FuncPseudoVoigt, NULL, true);

    add("FCJAsymm","height,center,hwhm,shape,h_l,s_l",",,,0.5,,",
        "Finger-Cox-Jephcoat asymmetry with PseudoVoight peakshape",
        Tplate::kPeak, &create_FuncFCJAsymm, NULL, true);

    add("Voigt", "height,center,gwidth,shape", ",,hwhm*0.8,0.1",
        "convolution of Gaussian and Lorentzian #",
        Tplate::kPeak, &create_FuncVoigt, NULL, true);

    add("VoigtA", "area,center,gwidth,shape", ",,hwhm*0.8,0.1",
        "convolution of Gaussian and Lorentzian #",
        Tplate::kPeak, &create_FuncVoigtA, NULL, true);

    add("EMG", "a,b,c,d", "height,center,hwhm*0.8,hwhm*0.08",
           "a*c*(2*pi)^0.5/(2*d) * exp((b-x)/d + c^2/(2*d^2))"
           " * (abs(d)/d - erf((b-x)/(2^0.5*c) + c/(2^0.5*d)))",
        Tplate::kPeak, &create_FuncEMG, NULL, true);

    add("DoniachSunjic", "h,a,f,e", "height,0.1,1,center",
       "h * cos(pi*a/2 + (1-a)*atan((x-e)/f)) / (f^2+(x-e)^2)^((1-a)/2)",
        Tplate::kPeak, &create_FuncDoniachSunjic, NULL, true);

    add("PielaszekCube", "a,center,r,s", "height*0.016,,300,150",
        "...#",
        Tplate::kPeak, &create_FuncPielaszekCube);

    add("LogNormal", "height,center,width,asym", ",,2*hwhm,0.1",
        "height*exp(-ln(2)*(ln(2.0*asym*(x-center)/width+1)/asym)^2)",
        Tplate::kPeak, &create_FuncLogNormal, NULL, true);

    add("Spline", "", "",
        "cubic spline #",
        0, &create_FuncSpline);

    add("Polyline", "", "",
        "linear interpolation #",
        0, &create_FuncPolyline);


    //------------------- interpreted functions ---------------------

    add("ExpDecay", "a,t", "0,1",
        "a*exp(-x/t)",
        0, &create_CustomFunction, p);

    add("GaussianA", "area,center,hwhm", ",,",
        "Gaussian(area/hwhm/sqrt(pi/ln(2)), center, hwhm)",
        Tplate::kPeak, &create_CompoundFunction, p, true);

    add("LogNormalA", "area,center,width,asym", ",,2*hwhm,0.1",
        "LogNormal(sqrt(ln(2)/pi)*(2*area/width)*exp(-asym^2/4/ln(2)), "
                   "center, width, asym)",
        Tplate::kPeak, &create_CompoundFunction, p);

    add("LorentzianA", "area,center,hwhm", ",,",
        "Lorentzian(area/hwhm/pi, center, hwhm)",
        Tplate::kPeak, &create_CompoundFunction, p, true);
    assert(tpvec_.back()->components[0].cargs.size() == 3);
    assert(tpvec_.back()->components[0].cargs[1].code().size() == 2);

    add("Pearson7A", "area,center,hwhm,shape", ",,,2",
        "Pearson7(area/(hwhm*exp(lgamma(shape-0.5)-lgamma(shape))"
                        "*sqrt(pi/(2^(1/shape)-1))), "
                 "center, hwhm, shape)",
        Tplate::kPeak, &create_CompoundFunction, p, true);

    add("PseudoVoigtA",
        "area,center,hwhm,shape",
        ",,,0.5",
        "GaussianA(area*(1-shape), center, hwhm)"
         " + LorentzianA(area*shape, center, hwhm)",
        Tplate::kPeak, &create_CompoundFunction, p, true);

    add("Sigmoid", "lower,upper,xmid,wsig", ",,,",
        "lower + (upper-lower)/(1+exp((xmid-x)/wsig))",
        Tplate::kSigmoid, &create_CustomFunction, p, true);

    add("SplitLorentzian", "height,center,hwhm1,hwhm2", ",,hwhm,hwhm",
        "x < center ? Lorentzian(height, center, hwhm1)"
                  " : Lorentzian(height, center, hwhm2)",
        Tplate::kPeak, &create_SplitFunction, p, true);

    add("SplitPseudoVoigt",
        "height,center,hwhm1,hwhm2,shape1,shape2", ",,hwhm,hwhm,0.5,0.5",
        "x < center ? PseudoVoigt(height, center, hwhm1, shape1)"
                  " : PseudoVoigt(height, center, hwhm2, shape2)",
        Tplate::kPeak, &create_SplitFunction, p, true);

    add("SplitVoigt",
        "height,center,gwidth1,gwidth2,shape1,shape2",
        ",,hwhm*0.8,hwhm*0.8,0.5,0.5",
        "x < center ? Voigt(height, center, gwidth1, shape1)"
                  " : Voigt(height, center, gwidth2, shape2)",
        Tplate::kPeak, &create_SplitFunction, p, true);
}

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
    if (iter->use_count() > 1)
        throw ExecuteError(type + " is currently used ("
                           + S(iter->use_count() - 1) + ").");
    tpvec_.erase(iter);
}

const Tplate* TplateMgr::get_tp(string const &type) const
{
    v_foreach (Tplate::Ptr, i, tpvec_)
        if ((*i)->name == type)
            return i->get();
    return NULL;
}

Tplate::Ptr TplateMgr::get_shared_tp(string const &type) const
{
    v_foreach (Tplate::Ptr, i, tpvec_)
        if ((*i)->name == type)
            return *i;
    return Tplate::Ptr();
}

vector<VMData*> reorder_args(Tplate::Ptr tp, const vector<string> &keys,
                                             const vector<VMData*> &values)
{
    assert (keys.size() == values.size());
    int n = tp->fargs.size();
    vector<VMData*> vv(n, (VMData*) NULL);
    for (int i = 0; i < n; ++i) {
        int idx = index_of_element(keys, tp->fargs[i]);
        if (idx != -1)
            vv[i] = values[idx];
    }
    return vv;
}

} // namespace fityk
