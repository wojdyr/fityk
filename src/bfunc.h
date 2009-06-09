// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$
#ifndef FITYK__BFUNC__H__
#define FITYK__BFUNC__H__

#include "func.h"

// a new class can be derived from class-derived-from-Function,
// but it should use the first constructor (with formula)
#define DECLARE_FUNC_OBLIGATORY_METHODS(NAME) \
    friend class Function;\
protected:\
    Func##NAME (Ftk const* F, \
                std::string const &name, \
                std::vector<std::string> const &vars, \
                std::string const &formula_) \
        : Function(F, name, vars, formula_) {} \
private:\
    Func##NAME (Ftk const* F, \
                std::string const &name, \
                std::vector<std::string> const &vars) \
        : Function(F, name, vars, formula) {} \
    Func##NAME (const Func##NAME&); \
public:\
    static const char *formula; \
    void calculate_value(std::vector<fp> const &xx, std::vector<fp> &yy) const;\
    void calculate_value_deriv(std::vector<fp> const &xx, \
                               std::vector<fp> &yy, std::vector<fp> &dy_da, \
                               bool in_dx=false) const;

//////////////////////////////////////////////////////////////////////////

class FuncConstant : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Constant)
};

class FuncLinear : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Linear)
};

class FuncQuadratic : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Quadratic)
};

class FuncCubic : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Cubic)
};

class FuncPolynomial4 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Polynomial4)
};

class FuncPolynomial5 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Polynomial5)
};


class FuncPolynomial6 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Polynomial6)
};

class FuncGaussian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Gaussian)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    fp center() const { return vv[1]; }
    bool has_height() const { return true; }
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; }
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    bool has_area() const { return true; }
    fp area() const   { return vv[0] * fabs(vv[2]) * sqrt(M_PI / M_LN2); }
};

class FuncSplitGaussian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(SplitGaussian)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    fp center() const { return vv[1]; }
    bool has_height() const { return true; }
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; }
    fp fwhm() const   { return fabs(vv[2]) + fabs(vv[3]); }
    bool has_area() const { return true; }
    fp area() const   { return vv[0] * (fabs(vv[2]) + fabs(vv[3])) / 2.
                                     * sqrt(M_PI/M_LN2); }
};

class FuncLorentzian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Lorentzian)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    fp center() const { return vv[1]; }
    bool has_height() const { return true; }
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; }
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    bool has_area() const { return true; }
    fp area() const   { return vv[0] * fabs(vv[2]) * M_PI; }
};

class FuncPearson7 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Pearson7)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    fp center() const { return vv[1]; }
    bool has_height() const { return true; }
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; }
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    bool has_area() const { return vv[3] > 0.5; }
    fp area() const;
};

class FuncSplitPearson7 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(SplitPearson7)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    fp center() const { return vv[1]; }
    bool has_height() const { return true; }
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; }
    fp fwhm() const   { return fabs(vv[2]) + fabs(vv[3]); }
    bool has_area() const { return vv[4] > 0.5 && vv[5] > 0.5; }
    fp area() const;
};

class FuncPseudoVoigt : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(PseudoVoigt)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    fp center() const { return vv[1]; }
    bool has_height() const { return true; }
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; }
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    bool has_area() const { return true; }
    fp area() const { return vv[0] * fabs(vv[2])
                      * ((vv[3] * M_PI) + (1 - vv[3]) * sqrt (M_PI / M_LN2)); }
};

class FuncVoigt : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Voigt)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    fp center() const { return vv[1]; }
    bool has_height() const { return true; }
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; }
    fp fwhm() const;
    bool has_area() const { return true; }
    fp area() const;
    std::vector<std::string> get_other_prop_names() const;
    fp other_prop(std::string const& name) const;
};

class FuncVoigtA : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(VoigtA)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    fp center() const { return vv[1]; }
    bool has_height() const { return true; }
    fp height() const;
    bool has_fwhm() const { return true; }
    fp fwhm() const;
    bool has_area() const { return true; }
    fp area() const { return vv[0]; }
};

class FuncEMG : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(EMG)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    bool has_center() const { return true; }
    fp center() const { return vv[1]; }
};

class FuncDoniachSunjic : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(DoniachSunjic)
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    bool has_center() const { return true; }
    fp center() const { return vv[3]; }
};

class FuncPielaszekCube : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(PielaszekCube)
    fp center() const { return vv[1]; }
};

class FuncLogNormal : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(LogNormal)
    void more_precomputations();
    bool get_nonzero_range (fp level, fp &left, fp &right) const;
    fp center() const { return vv[1]; }
    bool has_height() const { return true; }
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; }
    fp fwhm() const   { return fabs(vv[2]); }
    bool has_area() const { return true; }
    fp area() const   { return vv[0]/sqrt(M_LN2/M_PI)
        /(2.0/vv[2])/exp(-vv[3]*vv[3]/4.0/M_LN2); }
};


#endif
