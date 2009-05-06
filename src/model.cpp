// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <ctype.h>
#include "common.h"
#include "model.h"
#include "func.h"
#include "var.h"
#include "mgr.h"
//#include "guess.h" //estimate_peak_parameters() in guess_f()
#include "logic.h"

using namespace std;

Model::Model(Ftk *F_) 
    : F(F_), mgr(*F_)
{
    mgr.register_model(this);
}

Model::~Model()
{
    mgr.unregister_model(this);
}


void Model::do_find_function_indices(vector<string> &names,
                                     vector<int> &idx)
{
    idx.clear();
    for (int i = 0; i < size(names); ){
        int k = mgr.find_function_nr(names[i]);
        if (k == -1)
            names.erase(names.begin() + i);
        else {
            idx.push_back(k);
            ++i;
        }
    }
}

void Model::find_function_indices()
{
    do_find_function_indices(ff_names, ff_idx);
    do_find_function_indices(zz_names, zz_idx);
}

/// checks if this model depends on the variable with index idx
bool Model::is_dependent_on_var(int idx) const
{
    std::vector<Variable*> const& vv = mgr.get_variables();
    for (vector<int>::const_iterator i = ff_idx.begin(); i != ff_idx.end(); i++)
        if (mgr.get_function(*i)->is_dependent_on(idx, vv))
            return true;
    for (vector<int>::const_iterator i = zz_idx.begin(); i != zz_idx.end(); i++)
        if (mgr.get_function(*i)->is_dependent_on(idx, vv))
            return true;
    return false;
}

void Model::remove_all_functions_from(FuncSet fset)
{
    if (fset == kF) {
        ff_names.clear();
        ff_idx.clear();
    }
    else { // (fset == kZ) 
        zz_names.clear();
        zz_idx.clear();
    }
}

void Model::remove_function_from(string const &name, FuncSet fset)
{
    string only_name = !name.empty() && name[0]=='%' ? string(name,1) : name;
    // first remove function if it is already in ff or zz
    int idx = index_of_element(get_names(fset), only_name);
    if (idx == -1)
        throw ExecuteError("function %" + only_name + " is not in "+str(fset));
    if (fset == kF) {
        ff_names.erase(ff_names.begin() + idx);
        ff_idx.erase(ff_idx.begin() + idx);
    }
    else { // (fset == kZ) 
        zz_names.erase(zz_names.begin() + idx);
        zz_idx.erase(zz_idx.begin() + idx);
    }
}

void Model::add_function_to(string const &name, FuncSet fset)
{
    string only_name = !name.empty() && name[0]=='%' ? string(name,1) : name;
    int idx = mgr.find_function_nr(only_name);
    if (idx == -1)
        throw ExecuteError("function %" + only_name + " not found.");
    if (contains_element(get_names(fset), only_name)) {
        F->msg("function %" + only_name + " already in " + str(fset) + ".");
        return;
    }
    if (fset == kF) {
        ff_names.push_back(only_name);
        ff_idx.push_back(idx);
    }
    else if (fset == kZ) {
        zz_names.push_back(only_name);
        zz_idx.push_back(idx);
    }
}

fp Model::value(fp x) const
{
    x += zero_shift(x);
    fp y = 0;
    for (vector<int>::const_iterator i = ff_idx.begin(); i != ff_idx.end(); i++)
        y += mgr.get_function(*i)->calculate_value(x);
    return y;
}

fp Model::zero_shift(fp x) const
{
    fp z = 0;
    for (vector<int>::const_iterator i = zz_idx.begin(); i != zz_idx.end(); i++)
        z += mgr.get_function(*i)->calculate_value(x);
    return z;
}

void Model::compute_model(vector<fp> &x, vector<fp> &y) const
{
    // add x-correction to x
    for (vector<int>::const_iterator i = zz_idx.begin(); i != zz_idx.end(); i++)
        mgr.get_function(*i)->calculate_value(x, x);
    // add y-value to y
    for (vector<int>::const_iterator i = ff_idx.begin(); i != ff_idx.end(); i++)
        mgr.get_function(*i)->calculate_value(x, y);
}

// returns y values in y, x is changed in place to x+Z,
// derivatives are returned in dy_da as a matrix:
// [ dy/da_1 (x_1)  dy/da_2 (x_1)  ...  dy/da_na (x_1)  dy/dx (x_1) ]
// [ dy/da_1 (x_2)  dy/da_2 (x_2)  ...  dy/da_na (x_2)  dy/dx (x_2) ]
// [ ...                                                            ]
// [ dy/da_1 (x_n)  dy/da_2 (x_n)  ...  dy/da_na (x_n)  dy/dx (x_n) ]
void Model::compute_model_with_derivs(vector<fp> &x, vector<fp> &y,
                                      vector<fp> &dy_da) const
{
    assert(y.size() == x.size());
    if (x.empty())
        return;
    fill (dy_da.begin(), dy_da.end(), 0);
    // add x-correction to x
    for (vector<int>::const_iterator i = zz_idx.begin(); i != zz_idx.end(); i++)
        mgr.get_function(*i)->calculate_value(x, x);
    // calculate value and derivatives
    for (vector<int>::const_iterator i = ff_idx.begin(); i != ff_idx.end(); i++)
        mgr.get_function(*i)->calculate_value_deriv(x, y, dy_da, false);
    for (vector<int>::const_iterator i = zz_idx.begin(); i != zz_idx.end(); i++)
        mgr.get_function(*i)->calculate_value_deriv(x, y, dy_da, true);
}

vector<fp> 
Model::get_symbolic_derivatives(fp x) const
{
    int n = mgr.get_parameters().size();
    vector<fp> dy_da(n+1);
    vector<fp> xx(1, x);
    vector<fp> yy(1);
    compute_model_with_derivs(xx, yy, dy_da);
    dy_da.resize(n); //throw out last item (dy/dx)
    return dy_da;
}

vector<fp> 
Model::get_numeric_derivatives(fp x, fp numerical_h) const
{
    std::vector<fp> av_numder = mgr.get_parameters();
    int n = av_numder.size();
    vector<fp> dy_da(n);
    const fp small_number = 1e-10; //it only prevents h==0
    for (int k = 0; k < n; k++) {
        fp acopy = av_numder[k];
        fp h = max (fabs(acopy), small_number) * numerical_h;
        av_numder[k] -= h;
        mgr.use_external_parameters(av_numder);
        fp y_aless = value(x);
        av_numder[k] = acopy + h;
        mgr.use_external_parameters(av_numder);
        fp y_amore = value(x);
        dy_da[k] = (y_amore - y_aless) / (2 * h);
        av_numder[k] = acopy;
    }
    mgr.use_parameters();
    return dy_da;
}

// estimate max. value in given range (probe at peak centers and between)
fp Model::approx_max(fp x_min, fp x_max) const
{
    mgr.use_parameters();
    fp x = x_min;
    fp y_max = value(x);
    vector<fp> xx; 
    for (vector<int>::const_iterator i=ff_idx.begin(); i != ff_idx.end(); i++) {
        fp ctr = mgr.get_function(*i)->center();
        if (x_min < ctr && ctr < x_max)
            xx.push_back(ctr);
    }
    xx.push_back(x_max);
    sort(xx.begin(), xx.end());
    for (vector<fp>::const_iterator i = xx.begin(); i != xx.end(); i++) {
        fp x_between = (x + *i)/2.;
        x = *i;
        fp y = max(value(x_between), value(x));
        if (y > y_max) 
            y_max = y;
    }
    return y_max;
}


string Model::get_peak_parameters(vector<fp> const& errors) const
{
    string s;
    s += "# Peak Type     Center  Height  Area    FWHM    parameters...\n"; 
    for (vector<int>::const_iterator i=ff_idx.begin(); i != ff_idx.end(); i++){
        Function const* p = mgr.get_function(*i);
        s += p->xname + "  " + p->type_name  
            + "  "+ S(p->center()) + " " + S(p->height()) + " " + S(p->area())
            + " " + S(p->fwhm()) + "  ";
        for (int j = 0; j < p->get_vars_count(); ++j) {
            s += " " + S(p->get_var_value(j));
            if (!errors.empty()) {
                Variable const* var = mgr.get_variable(p->get_var_idx(j));
                if (var->is_simple())
                    s += " +/- " + S(errors[var->get_nr()]);
                else
                    s += " +/- ?";
            }
        }
        s += "\n";
    }
    return s;
}


string Model::get_formula(bool simplify, bool gnuplot_style) const
{
    if (ff_names.empty())
        return "0";
    string shift;
    for (vector<int>::const_iterator i = zz_idx.begin(); i != zz_idx.end(); i++)
        shift += "+(" + mgr.get_function(*i)->get_current_formula() + ")";
    string x = "(x" + shift + ")";
    string formula;
    for (vector<int>::const_iterator i = ff_idx.begin(); i != ff_idx.end(); i++)
        formula += (i==ff_idx.begin() ? "" : "+") 
                   + mgr.get_function(*i)->get_current_formula(x); 
    if (simplify) {
        // check if formula has not-expanded-functions, like Voigt(2,3,4,5)
        bool has_upper = false;
        for (size_t i = 0; i < formula.size(); ++i)
            if (isupper(formula[i])) {
                has_upper = true;
                break;
            }
        // the simplify_formula() is not working with not-expanded-functions
        if (!has_upper) 
            formula = simplify_formula(formula);
    }
    if (gnuplot_style) { //gnuplot format is a bit different
        replace_all(formula, "^", "**");
        replace_words(formula, "ln", "log");
        // avoid integer division (1/2 == 0)
        string::size_type pos = 0; 
        while ((pos = formula.find('/', pos)) != string::npos) {
            ++pos;
            if (!isdigit(formula[pos]))
                continue;
            while (pos < formula.length() && isdigit(formula[pos]))
                ++pos;
            if (pos == formula.length())
                formula += ".";
            else if (pos != '.')
                formula.insert(pos, ".");
        }
    }
    return formula;
}


fp Model::numarea(fp x1, fp x2, int nsteps) const
{
    x1 += zero_shift(x1);
    x2 += zero_shift(x2);
    fp a = 0;
    for (vector<int>::const_iterator i = ff_idx.begin(); i != ff_idx.end(); i++)
        a += mgr.get_function(*i)->numarea(x1, x2, nsteps);
    return a;
}

