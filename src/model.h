// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__SUM__H__
#define FITYK__SUM__H__
#include <vector>
#include <string>
#include <utility>
#include <memory>
#include "common.h"

class Function;
class Data;
class VariableManager;
class Ftk;

///  This class contains description of curve which we are trying to fit
///  to data. This curve is described simply by listing names of functions
///  in F and in Z (Z contains x-corrections)
class Model
{
public:
    // set of functions, F and Z
    enum FuncSet { kF, kZ };

    Model(Ftk *F_);
    ~Model();
    void find_function_indices();
    void add_function_to(std::string const &name, FuncSet fset);
    void remove_function_from(std::string const &name, FuncSet fset);
    void remove_all_functions_from(FuncSet fset);
    fp value(fp x) const;
    // calculate value of model (to be used when derivatives are not needed)
    void compute_model(std::vector<fp> &x, std::vector<fp> &y) const;
    // calculate value of model and its derivatives, see definition for details
    void compute_model_with_derivs(std::vector<fp> &x, std::vector<fp> &y,
                                   std::vector<fp> &dy_da) const;

    fp funcs_value (const std::vector<int>& fn, fp x) const;

    fp value_and_put_deriv (fp x, std::vector<fp>& dy_da) const;
    fp value_and_add_numeric_deriv (fp x, bool both_sides,
                                    std::vector<fp>& dy_da) const;
    fp approx_max(fp x_min, fp x_max) const;
    std::string general_info() const;
    std::string get_formula(bool simplify, bool gnuplot_style) const;
    std::string get_peak_parameters(std::vector<fp> const& errors) const;
    std::vector<fp> get_symbolic_derivatives(fp x) const;
    std::vector<fp> get_numeric_derivatives(fp x, fp numerical_h) const;
    fp zero_shift (fp x) const;
    std::vector<int> const& get_ff_idx() const { return ff_idx; }
    std::vector<int> const& get_zz_idx() const { return zz_idx; }
    std::vector<std::string> const &get_ff_names() const { return ff_names; }
    std::vector<std::string> const &get_zz_names() const { return zz_names; }
    std::vector<std::string> const &get_names(FuncSet fset) const
        { return (fset == kF ? ff_names : zz_names); }
    std::vector<int> const &get_indices(FuncSet fset) const
        { return (fset == kF ? ff_idx : zz_idx); }
    bool has_any_info() const { return !ff_names.empty() || !zz_names.empty(); }
    fp numarea(fp x1, fp x2, int nsteps) const;
    bool is_dependent_on_var(int idx) const;
    static std::string str(FuncSet fset) { return fset == kF ? "F" : "Z"; }
    static FuncSet parse_funcset(char c)
        { assert(c == 'F' || c == 'Z'); return c == 'F' ? kF : kZ; }

private:
    Ftk const* F;
    VariableManager &mgr;
    std::vector<std::string> ff_names;
    std::vector<std::string> zz_names;
    std::vector<int> ff_idx;
    std::vector<int> zz_idx;

    void do_find_function_indices(std::vector<std::string> &names,
                                  std::vector<int> &idx);
    DISALLOW_COPY_AND_ASSIGN(Model);
};


#endif

