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

struct FunctionSum
{
    /// names of functions in F/Z, i.e. names of the component functions
    std::vector<std::string> names;
    /// indices corresponding to the names in names
    std::vector<int> idx;

    bool empty() const { return names.empty(); }
};

///  This class contains description of curve which we are trying to fit
///  to data. This curve is described simply by listing names of functions
///  in F and in Z (Z contains x-corrections)
class Model
{
public:
    Model(Ftk *F_);
    ~Model();

    // calculate model (single point)
    fp value(fp x) const;

    // calculate model (multiple points) without derivatives
    // the option to ignore one function in F is useful for "guessing".
    void compute_model(std::vector<fp> &x, std::vector<fp> &y,
                       int ignore_func=-1) const;

    // calculate model (multiple points) with derivatives
    void compute_model_with_derivs(std::vector<fp> &x, std::vector<fp> &y,
                                   std::vector<fp> &dy_da) const;


    fp approx_max(fp x_min, fp x_max) const;
    std::string get_formula(bool simplify, bool gnuplot_style) const;
    std::string get_peak_parameters(std::vector<fp> const& errors) const;
    std::vector<fp> get_symbolic_derivatives(fp x) const;
    std::vector<fp> get_numeric_derivatives(fp x, fp numerical_h) const;
    fp zero_shift (fp x) const;

    // ff_ and zz_ getters
    const FunctionSum& get_ff() const { return ff_; }
    FunctionSum& get_ff() { return ff_; }
    const FunctionSum& get_zz() const { return zz_; }
    FunctionSum& get_zz() { return zz_; }
    const FunctionSum& get_fz(char c) const { return (c == 'F' ? ff_ : zz_); }
    FunctionSum& get_fz(char c) { return (c == 'F' ? ff_ : zz_); }

    // throws SyntaxError if index `idx' is wrong
    std::string const& get_func_name(char c, int idx) const;

    fp numarea(fp x1, fp x2, int nsteps) const;
    bool is_dependent_on_var(int idx) const;


private:
    Ftk const* F;
    VariableManager &mgr;
    FunctionSum ff_, zz_;

    DISALLOW_COPY_AND_ASSIGN(Model);
};


#endif

