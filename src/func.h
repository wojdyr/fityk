// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$
#ifndef FITYK__FUNC__H__
#define FITYK__FUNC__H__

#include <map>
#include <assert.h>

#include "tplate.h"
#include "var.h"

class Settings;
class VariableManager;

class Function : public VariableUser
{
public:

    struct Multi
    {
        int p; int n; fp mult;
        Multi(int n_, const Variable::ParMult& pm)
            : p(pm.p), n(n_), mult(pm.mult) {}
    };

    Function(const Settings* settings, const std::string &name,
             const Tplate::Ptr tp, const std::vector<std::string> &vars);
    virtual ~Function() {}
    virtual void init();

    static Function* factory(const Settings* settings, const std::string &name,
                             const Tplate::Ptr tp,
                             const std::vector<std::string> &vars);

    const Tplate::Ptr& tp() const { return tp_; }

    /// number of variables
    int nv() const {return tp_->fargs.empty() ? av_.size() : tp_->fargs.size();}

    /// calculate value at x[i] and _add_ the result to y[i] (for each i)
    virtual void calculate_value_in_range(const std::vector<fp> &x,
                                          std::vector<fp> &y,
                                          int first, int last) const = 0;
    void calculate_value(const std::vector<fp> &x, std::vector<fp> &y) const;
    fp calculate_value(fp x) const; /// wrapper around array version

    virtual void calculate_value_deriv_in_range(const std::vector<fp> &x,
                                                std::vector<fp> &y,
                                                std::vector<fp> &dy_da,
                                                bool in_dx,
                                                int first, int last) const = 0;
    void calculate_value_deriv(const std::vector<fp> &x,
                               std::vector<fp> &y,
                               std::vector<fp> &dy_da,
                               bool in_dx=false) const;

    void do_precomputations(const std::vector<Variable*> &variables);
    virtual void more_precomputations() {}
    void erased_parameter(int k);
    virtual bool get_nonzero_range(fp/*level*/, fp&/*left*/, fp&/*right*/) const
                                                              { return false; }

    // properties

    virtual bool get_center(fp* a) const;
    virtual bool get_height(fp* /*a*/) const { return false; }
    virtual bool get_fwhm(fp* /*a*/) const { return false; }
    virtual bool get_area(fp* /*a*/) const { return false; }
    /// integral width := area / height
    bool get_iwidth(fp* a) const;

    /// get list of other properties (e.g. like Lorentzian-FWHM of Voigt)
    virtual const std::vector<std::string>& get_other_prop_names() const
                { static const std::vector<std::string> empty; return empty; }
    /// returns value of the property, or 0 if not defined
    virtual fp get_other_prop(const std::string&) const { return 0; }

    const std::vector<fp>& av() const { return av_; }
    std::string get_basic_assignment() const;
    std::string get_current_assignment(const std::vector<Variable*> &variables,
                                       const std::vector<fp> &parameters) const;
    virtual std::string get_current_formula(const std::string& x = "x") const;

    // VarArgFunction overrides this defintion (that's why value is returned)
    virtual std::string get_param(int n) const { return tp_->fargs[n]; }

    int get_param_nr(const std::string& param) const;
    fp get_param_value(const std::string& param) const;

    fp numarea(fp x1, fp x2, int nsteps) const;
    fp find_x_with_value(fp x1, fp x2, fp val, int max_iter=1000) const;
    fp find_extremum(fp x1, fp x2, int max_iter=1000) const;

    virtual std::string get_bytecode() const { return "No bytecode"; }
protected:
    const Settings* settings_;
    Tplate::Ptr tp_;
    /// current values of arguments,
    /// the vector can be extended by derived classes to store temporary values
    std::vector<fp> av_;
    std::vector<Multi> multi_;
    int center_idx_;

private:
    static std::vector<fp> calc_val_xx, calc_val_yy;
};

#endif

