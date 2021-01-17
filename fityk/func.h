// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_FUNC_H_
#define FITYK_FUNC_H_

#include "tplate.h"
#include "var.h"

namespace fityk {

struct Settings;

class FITYK_API Function : public Func
{
public:
    struct Multi
    {
        int p; int n; realt mult;
        Multi(int n_, const Variable::ParMult& pm)
            : p(pm.p), n(n_), mult(pm.mult) {}
    };

    Function(const Settings* settings, const std::string &name_,
             const Tplate::Ptr tp, const std::vector<std::string> &vars);
    virtual ~Function() {}
    virtual void init();

    const Tplate::Ptr& tp() const { return tp_; }

    /// number of variables
    int nv() const {return tp_->fargs.empty() ? av_.size() : tp_->fargs.size();}

    /// calculate value at x[i] and _add_ the result to y[i] (for each i)
    virtual void calculate_value_in_range(const std::vector<realt> &x,
                                          std::vector<realt> &y,
                                          int first, int last) const = 0;
    void calculate_value(const std::vector<realt> &x,
                         std::vector<realt> &y) const;
    realt calculate_value(realt x) const; /// wrapper around array version

    virtual void calculate_value_deriv_in_range(const std::vector<realt> &x,
                                                std::vector<realt> &y,
                                                std::vector<realt> &dy_da,
                                                bool in_dx,
                                                int first, int last) const = 0;
    void calculate_value_deriv(const std::vector<realt> &x,
                               std::vector<realt> &y,
                               std::vector<realt> &dy_da,
                               bool in_dx=false) const;

    void do_precomputations(const std::vector<Variable*> &variables);
    virtual void more_precomputations() {}
    void erased_parameter(int k);
    virtual bool get_nonzero_range(double /*level*/,
                      realt& /*left*/, realt& /*right*/) const { return false; }

    virtual bool is_symmetric() const { return false; }
    virtual bool get_center(realt* a) const;
    virtual bool get_height(realt* /*a*/) const { return false; }
    virtual bool get_fwhm(realt* /*a*/) const { return false; }
    virtual bool get_area(realt* /*a*/) const { return false; }
    /// integral width := area / height
    bool get_ibreadth(realt* a) const;
    /// get list of other properties (e.g. like Lorentzian-FWHM of Voigt)
    virtual const std::vector<std::string>& get_other_prop_names() const
                { static const std::vector<std::string> empty; return empty; }
    /// if defined, returns true and sets second parameter to the value
    virtual bool get_other_prop(const std::string&, realt*) const { return 0; }

    const std::vector<realt>& av() const { return av_; }
    std::string get_basic_assignment() const;
    std::string get_current_assignment(const std::vector<Variable*> &variables,
                                    const std::vector<realt> &parameters) const;
    virtual std::string get_current_formula(const std::string& x,
                                            const char* num_fmt) const;

    // VarArgFunction overrides this defintion (that's why value is returned)
    virtual std::string get_param(int n) const
        { return is_index(n, tp_->fargs) ? tp_->fargs[n] : std::string(); }

    int get_param_nr(const std::string& param) const;
// C++ exception specifications are used by SWIG bindings.
// They are deprecated (in this form) in C++-11.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#endif
    virtual realt get_param_value(const std::string& param) const
                            throw(ExecuteError); // exc. spec. is used by SWIG

    realt numarea(realt x1, realt x2, int nsteps) const;

    virtual std::string get_bytecode() const { return "No bytecode"; }
    virtual void update_var_indices(const std::vector<Variable*>& variables)
            { used_vars_.update_indices(variables); }
    void set_param_name(int n, const std::string &new_p)
            { used_vars_.set_name(n, new_p); }
    const IndexedVars& used_vars() const { return used_vars_; }

    // implementation of members of Func
    virtual const std::string& get_template_name() const { return tp_->name; }
    virtual const std::string& var_name(const std::string& param) const
                        throw(ExecuteError) // exc. spec. is used by SWIG
                        { return used_vars_.get_name(get_param_nr(param)); }
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    virtual realt value_at(realt x) const { return calculate_value(x); }
    int max_param_pos() const;

    realt calculate_value_and_deriv(realt x, std::vector<realt> &dy_da) const {
        bufx_[0] = x, bufy_[0] = 0.;
        calculate_value_deriv_in_range(bufx_, bufy_, dy_da, false, 0, 1);
        return bufy_[0];
    }

protected:
    void replace_symbols_with_values(std::string &t, const char* num_fmt) const;

    IndexedVars used_vars_;
    const Settings* settings_;
    Tplate::Ptr tp_;
    /// current values of arguments,
    /// the vector can be extended by derived classes to store temporary values
    std::vector<realt> av_;
    std::vector<Multi> multi_;
    int center_idx_;

private:
    static std::vector<realt> bufx_;
    static std::vector<realt> bufy_;
};

} // namespace fityk
#endif

