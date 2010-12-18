// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$
#ifndef FITYK__BFUNC__H__
#define FITYK__BFUNC__H__

#include "func.h"
#include "numfuncs.h" // PointQ
#include "tplate.h" // Tplate::Ptr
#include "common.h" // DISALLOW_COPY_AND_ASSIGN

#define DECLARE_FUNC_OBLIGATORY_METHODS(NAME, PARENT) \
private:\
    DISALLOW_COPY_AND_ASSIGN(Func##NAME); \
public:\
    Func##NAME (const Settings* settings, \
                const std::string &name, \
                Tplate::Ptr tp, \
                std::vector<std::string> const &vars) \
        : PARENT(settings, name, tp, vars) {} \
    void calculate_value_in_range(std::vector<fp> const &xx, \
                                  std::vector<fp> &yy, \
                                  int first, int last) const;\
    void calculate_value_deriv_in_range(std::vector<fp> const &xx, \
                                        std::vector<fp> &yy, \
                                        std::vector<fp> &dy_da, \
                                        bool in_dx, \
                                        int first, int last) const;

//////////////////////////////////////////////////////////////////////////

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
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    bool get_center(fp* a) const { *a = av_[1]; return true; }
    bool get_height(fp* a) const { *a = av_[0]; return true; }
    bool get_fwhm(fp* a) const { *a = 2 * fabs(av_[2]); return true; }
    bool get_area(fp* a) const;
};

class FuncSplitGaussian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(SplitGaussian, Function)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    bool get_center(fp* a) const { *a = av_[1]; return true; }
    bool get_height(fp* a) const { *a = av_[0]; return true; }
    bool get_fwhm(fp* a) const { *a = fabs(av_[2])+fabs(av_[3]); return true; }
    bool get_area(fp* a) const;
};

class FuncLorentzian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Lorentzian, Function)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    bool get_center(fp* a) const { *a = av_[1]; return true; }
    bool get_height(fp* a) const { *a = av_[0]; return true; }
    bool get_fwhm(fp* a) const { *a = 2 * fabs(av_[2]); return true; }
    bool get_area(fp* a) const {*a = av_[0] * fabs(av_[2]) * M_PI; return true;}
};

class FuncPearson7 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Pearson7, Function)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    bool get_center(fp* a) const { *a = av_[1]; return true; }
    bool get_height(fp* a) const { *a = av_[0]; return true; }
    bool get_fwhm(fp* a) const { *a = 2 * fabs(av_[2]); return true; }
    bool get_area(fp* a) const;
};

class FuncSplitPearson7 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(SplitPearson7, Function)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    bool get_center(fp* a) const { *a = av_[1]; return true; }
    bool get_height(fp* a) const { *a = av_[0]; return true; }
    bool get_fwhm(fp* a) const { *a = fabs(av_[2])+fabs(av_[3]); return true; }
    bool get_area(fp* a) const;
};

class FuncPseudoVoigt : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(PseudoVoigt, Function)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    bool get_center(fp* a) const { *a = av_[1]; return true; }
    bool get_height(fp* a) const { *a = av_[0]; return true; }
    bool get_fwhm(fp* a) const { *a = 2 * fabs(av_[2]); return true; }
    bool get_area(fp* a) const;
};

class FuncVoigt : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Voigt, Function)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    bool get_center(fp* a) const { *a = av_[1]; return true; }
    bool get_height(fp* a) const { *a = av_[0]; return true; }
    bool get_fwhm(fp* a) const;
    bool get_area(fp* a) const;
    const std::vector<std::string>& get_other_prop_names() const;
    fp get_other_prop(std::string const& name) const;
};

class FuncVoigtA : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(VoigtA, Function)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    bool get_center(fp* a) const { *a = av_[1]; return true; }
    bool get_height(fp* a) const;
    bool get_fwhm(fp* a) const;
    bool get_area(fp* a) const { *a = av_[0]; return true; }
};

class FuncEMG : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(EMG, Function)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    bool get_center(fp* a) const { *a = av_[1]; return true; }
};

class FuncDoniachSunjic : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(DoniachSunjic, Function)
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    bool get_center(fp* a) const { *a = av_[3]; return true; }
};

class FuncPielaszekCube : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(PielaszekCube, Function)
    bool get_center(fp* a) const { *a = av_[1]; return true; }
};

class FuncLogNormal : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(LogNormal, Function)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    bool get_center(fp* a) const { *a = av_[1]; return true; }
    bool get_height(fp* a) const { *a = av_[0]; return true; }
    bool get_fwhm(fp* a) const;
    bool get_area(fp* a) const;
};


class VarArgFunction : public Function
{
protected:
    VarArgFunction(const Settings* settings,
                   const std::string &name,
                   Tplate::Ptr tp,
                   const std::vector<std::string> &vars)
        : Function(settings, name, tp, vars) {}
    virtual void init() { center_idx_ = -1; }

    // so far all the va functions have parameters x1,y1,x2,y2,...
    virtual const std::string get_param(int n) const
            { return (n % 2 == 0 ? "x" : "y") + S(n/2 + 1); }
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


#endif
