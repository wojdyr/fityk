// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

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
    Model(Ftk *F);
    ~Model();

    /// calculate model (single point)
    realt value(realt x) const;

    /// calculate model (multiple points) without derivatives
    /// the option to ignore one function in F is useful for "guessing".
    void compute_model(std::vector<realt> &x, std::vector<realt> &y,
                       int ignore_func=-1) const;

    /// calculate model (multiple points) with derivatives
    void compute_model_with_derivs(std::vector<realt> &x, std::vector<realt> &y,
                                   std::vector<realt> &dy_da) const;


    /// estimate max. value in given range (probe at peak centers and between)
    realt approx_max(realt x_min, realt x_max) const;

    std::string get_formula(bool simplify) const;
    std::string get_peak_parameters(const std::vector<realt>& errors) const;
    std::vector<realt> get_symbolic_derivatives(realt x) const;
    std::vector<realt> get_numeric_derivatives(realt x, realt numerical_h)const;
    realt zero_shift(realt x) const;

    // ff_ and zz_ getters
    const FunctionSum& get_ff() const { return ff_; }
    FunctionSum& get_ff() { return ff_; }
    const FunctionSum& get_zz() const { return zz_; }
    FunctionSum& get_zz() { return zz_; }
    const FunctionSum& get_fz(char c) const { return (c == 'F' ? ff_ : zz_); }
    FunctionSum& get_fz(char c) { return (c == 'F' ? ff_ : zz_); }

    // throws SyntaxError if index `idx' is wrong
    const std::string& get_func_name(char c, int idx) const;

    realt numarea(realt x1, realt x2, int nsteps) const;
    bool is_dependent_on_var(int idx) const;


private:
    const Ftk* F_;
    VariableManager &mgr;
    FunctionSum ff_, zz_;

    DISALLOW_COPY_AND_ASSIGN(Model);
};


#endif

