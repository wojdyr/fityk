// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_BFUNC_H_
#define FITYK_BFUNC_H_

#include "func.h"
#include "numfuncs.h" // PointQ
#include "tplate.h" // Tplate::Ptr
#include "common.h" // DISALLOW_COPY_AND_ASSIGN

#define DECLARE_FUNC_OBLIGATORY_METHODS(NAME, PARENT) \
private:\
    DISALLOW_COPY_AND_ASSIGN(Func##NAME); \
public:\
    Func##NAME (const Settings* settings, \
                const std::string &fname, \
                Tplate::Ptr tp, \
                std::vector<std::string> const &vars) \
        : PARENT(settings, fname, tp, vars) {} \
    void calculate_value_in_range(std::vector<realt> const &xx, \
                                  std::vector<realt> &yy, \
                                  int first, int last) const;\
    void calculate_value_deriv_in_range(std::vector<realt> const &xx, \
                                        std::vector<realt> &yy, \
                                        std::vector<realt> &dy_da, \
                                        bool in_dx, \
                                        int first, int last) const;

//////////////////////////////////////////////////////////////////////////
namespace fityk {

class FuncConstant : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Constant, Function)
};

class FuncLinear : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Linear, Function)
};

class FuncQuadratic : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Quadratic, Function)
};

class FuncCubic : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Cubic, Function)
};

class FuncPolynomial4 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Polynomial4, Function)
};

class FuncPolynomial5 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Polynomial5, Function)
};


class FuncPolynomial6 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Polynomial6, Function)
};

class FuncGaussian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Gaussian, Function)
    void more_precomputations();
    bool get_nonzero_range(double level, realt &left, realt &right) const;
    bool is_symmetric() const { return true; }
    bool get_center(realt* a) const { *a = av_[1]; return true; }
    bool get_height(realt* a) const { *a = av_[0]; return true; }
    bool get_fwhm(realt* a) const { *a = 2 * fabs(av_[2]); return true; }
    bool get_area(realt* a) const;
};

class FuncSplitGaussian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(SplitGaussian, Function)
    void more_precomputations();
    bool get_nonzero_range(double level, realt &left, realt &right) const;
    bool get_center(realt* a) const { *a = av_[1]; return true; }
    bool get_height(realt* a) const { *a = av_[0]; return true; }
    bool get_fwhm(realt* a) const
                            { *a = fabs(av_[2])+fabs(av_[3]); return true; }
    bool get_area(realt* a) const;
};

class FuncLorentzian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Lorentzian, Function)
    void more_precomputations();
    bool get_nonzero_range(double level, realt &left, realt &right) const;
    bool is_symmetric() const { return true; }
    bool get_center(realt* a) const { *a = av_[1]; return true; }
    bool get_height(realt* a) const { *a = av_[0]; return true; }
    bool get_fwhm(realt* a) const { *a = 2 * fabs(av_[2]); return true; }
    bool get_area(realt* a) const
                            { *a = av_[0] * fabs(av_[2]) * M_PI; return true; }
};

class FuncPearson7 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Pearson7, Function)
    void more_precomputations();
    bool get_nonzero_range(double level, realt &left, realt &right) const;
    bool is_symmetric() const { return true; }
    bool get_center(realt* a) const { *a = av_[1]; return true; }
    bool get_height(realt* a) const { *a = av_[0]; return true; }
    bool get_fwhm(realt* a) const { *a = 2 * fabs(av_[2]); return true; }
    bool get_area(realt* a) const;
};

class FuncSplitPearson7 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(SplitPearson7, Function)
    void more_precomputations();
    bool get_nonzero_range(double level, realt &left, realt &right) const;
    bool get_center(realt* a) const { *a = av_[1]; return true; }
    bool get_height(realt* a) const { *a = av_[0]; return true; }
    bool get_fwhm(realt* a) const
                            { *a = fabs(av_[2])+fabs(av_[3]); return true; }
    bool get_area(realt* a) const;
};

class FuncPseudoVoigt : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(PseudoVoigt, Function)
    void more_precomputations();
    bool get_nonzero_range(double level, realt &left, realt &right) const;
    bool is_symmetric() const { return true; }
    bool get_center(realt* a) const { *a = av_[1]; return true; }
    bool get_height(realt* a) const { *a = av_[0]; return true; }
    bool get_fwhm(realt* a) const { *a = 2 * fabs(av_[2]); return true; }
    bool get_area(realt* a) const;
};

class FuncVoigt : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Voigt, Function)
    void more_precomputations();
    bool get_nonzero_range(double level, realt &left, realt &right) const;
    bool is_symmetric() const { return true; }
    bool get_center(realt* a) const { *a = av_[1]; return true; }
    bool get_height(realt* a) const { *a = av_[0]; return true; }
    bool get_fwhm(realt* a) const;
    bool get_area(realt* a) const;
    const std::vector<std::string>& get_other_prop_names() const;
    bool get_other_prop(std::string const& pname, realt* a) const;
};

class FuncVoigtA : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(VoigtA, Function)
    void more_precomputations();
    bool get_nonzero_range(double level, realt &left, realt &right) const;
    bool is_symmetric() const { return true; }
    bool get_center(realt* a) const { *a = av_[1]; return true; }
    bool get_height(realt* a) const;
    bool get_fwhm(realt* a) const;
    bool get_area(realt* a) const { *a = av_[0]; return true; }
};

class FuncEMG : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(EMG, Function)
    void more_precomputations();
    bool get_nonzero_range(double level, realt &left, realt &right) const;
    bool get_center(realt* a) const { *a = av_[1]; return true; }
    bool get_area(realt* a) const;
};

class FuncDoniachSunjic : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(DoniachSunjic, Function)
    bool get_nonzero_range(double level, realt &left, realt &right) const;
    bool get_center(realt* a) const { *a = av_[3]; return true; }
};

class FuncPielaszekCube : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(PielaszekCube, Function)
    bool get_center(realt* a) const { *a = av_[1]; return true; }
};

class FuncLogNormal : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(LogNormal, Function)
    void more_precomputations();
    bool get_nonzero_range(double level, realt &left, realt &right) const;
    bool get_center(realt* a) const { *a = av_[1]; return true; }
    bool get_height(realt* a) const { *a = av_[0]; return true; }
    bool get_fwhm(realt* a) const;
    bool get_area(realt* a) const;
};


class VarArgFunction : public Function
{
public:
    virtual std::string get_param(int n) const;
protected:
    VarArgFunction(const Settings* settings,
                   const std::string &fname,
                   Tplate::Ptr tp,
                   const std::vector<std::string> &vars)
        : Function(settings, fname, tp, vars) {}
    virtual void init() { center_idx_ = -1; }
};

class FuncSpline : public VarArgFunction
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Spline, VarArgFunction)
    void more_precomputations();
private:
    mutable std::vector<PointQ> q_;
};

class FuncPolyline : public VarArgFunction
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Polyline, VarArgFunction)
    void more_precomputations();
private:
    mutable std::vector<PointD> q_;
};


// macros for use in implementations

#define CALCULATE_VALUE_BEGIN(NAME) \
void NAME::calculate_value_in_range(vector<realt> const &xx, vector<realt> &yy,\
                                    int first, int last) const\
{\
    for (int i = first; i < last; ++i) {\
        realt x = xx[i];


#define CALCULATE_VALUE_END(VAL) \
        yy[i] += (VAL);\
    }\
}

#define CALCULATE_DERIV_BEGIN(NAME) \
void NAME::calculate_value_deriv_in_range(vector<realt> const &xx, \
                                          vector<realt> &yy, \
                                          vector<realt> &dy_da, \
                                          bool in_dx, \
                                          int first, int last) const \
{ \
    int dyn = dy_da.size() / xx.size(); \
    vector<realt> dy_dv(nv(), 0.); \
    for (int i = first; i < last; ++i) { \
        realt x = xx[i]; \
        realt dy_dx;


#define CALCULATE_DERIV_END(VAL) \
        if (!in_dx) { \
            yy[i] += (VAL); \
            v_foreach (Multi, j, multi_) \
                dy_da[dyn*i+j->p] += dy_dv[j->n] * j->mult;\
            dy_da[dyn*i+dyn-1] += dy_dx;\
        }\
        else {  \
            v_foreach (Multi, j, multi_) \
                dy_da[dyn*i+j->p] += dy_da[dyn*i+dyn-1] * dy_dv[j->n]*j->mult;\
        } \
    } \
}


} // namespace fityk
#endif
