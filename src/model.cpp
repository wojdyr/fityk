// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <algorithm>
#include <string>
#include <vector>
#include <ctype.h>
#include "common.h"
#include "model.h"
#include "func.h"
#include "var.h"
#include "mgr.h"
#include "logic.h"
#include "ast.h"

using namespace std;

namespace fityk {

Model::Model(Ftk *F)
    : F_(F), mgr(F->mgr)
{
    mgr.register_model(this);
}

Model::~Model()
{
    mgr.unregister_model(this);
}


/// checks if this model depends on the variable with index idx
bool Model::is_dependent_on_var(int idx) const
{
    const vector<Variable*>& vv = mgr.variables();
    v_foreach (int, i, ff_.idx)
        if (mgr.get_function(*i)->used_vars().depends_on(idx, vv))
            return true;
    v_foreach (int, i, zz_.idx)
        if (mgr.get_function(*i)->used_vars().depends_on(idx, vv))
            return true;
    return false;
}

realt Model::value(realt x) const
{
    x += zero_shift(x);
    realt y = 0;
    v_foreach (int, i, ff_.idx)
        y += mgr.get_function(*i)->calculate_value(x);
    return y;
}

realt Model::zero_shift(realt x) const
{
    realt z = 0;
    v_foreach (int, i, zz_.idx)
        z += mgr.get_function(*i)->calculate_value(x);
    return z;
}

void Model::compute_model(vector<realt> &x, vector<realt> &y,
                          int ignore_func) const
{
    // add x-correction to x
    v_foreach (int, i, zz_.idx)
        mgr.get_function(*i)->calculate_value(x, x);
    // add y-value to y
    v_foreach (int, i, ff_.idx)
        if (*i != ignore_func)
            mgr.get_function(*i)->calculate_value(x, y);
}

// returns y values in y, x is changed in place to x+Z,
// derivatives are returned in dy_da as a matrix:
// [ dy/da_1 (x_1)  dy/da_2 (x_1)  ...  dy/da_na (x_1)  dy/dx (x_1) ]
// [ dy/da_1 (x_2)  dy/da_2 (x_2)  ...  dy/da_na (x_2)  dy/dx (x_2) ]
// [ ...                                                            ]
// [ dy/da_1 (x_n)  dy/da_2 (x_n)  ...  dy/da_na (x_n)  dy/dx (x_n) ]
void Model::compute_model_with_derivs(vector<realt> &x, vector<realt> &y,
                                      vector<realt> &dy_da) const
{
    assert(y.size() == x.size());
    if (x.empty())
        return;
    fill (dy_da.begin(), dy_da.end(), 0);

    // add x-correction to x
    v_foreach (int, i, zz_.idx)
        mgr.get_function(*i)->calculate_value(x, x);

    // calculate value and derivatives
    v_foreach (int, i, ff_.idx)
        mgr.get_function(*i)->calculate_value_deriv(x, y, dy_da, false);
    v_foreach (int, i, zz_.idx)
        mgr.get_function(*i)->calculate_value_deriv(x, y, dy_da, true);
}

vector<realt> Model::get_symbolic_derivatives(realt x) const
{
    int n = mgr.parameters().size();
    vector<realt> dy_da(n+1); // last item is dy/dx
    vector<realt> xx(1, x);
    vector<realt> yy(1);
    compute_model_with_derivs(xx, yy, dy_da);
    return dy_da;
}

vector<realt> Model::get_numeric_derivatives(realt x, realt numerical_h) const
{
    vector<realt> av_numder = mgr.parameters();
    int n = av_numder.size();
    vector<realt> dy_da(n+1);
    const double small_number = 1e-10; //it only prevents h==0
    for (int k = 0; k < n; k++) {
        realt acopy = av_numder[k];
        realt h = max(fabs(acopy), small_number) * numerical_h;
        av_numder[k] -= h;
        mgr.use_external_parameters(av_numder);
        realt y_aless = value(x);
        av_numder[k] = acopy + h;
        mgr.use_external_parameters(av_numder);
        realt y_amore = value(x);
        dy_da[k] = (y_amore - y_aless) / (2 * h);
        av_numder[k] = acopy;
    }
    mgr.use_parameters();
    realt h = max(fabs(x), small_number) * numerical_h;
    dy_da[n] = (value(x+h) - value(x-h)) / (2 * h);
    return dy_da;
}

realt Model::approx_max(realt x_min, realt x_max) const
{
    mgr.use_parameters();
    realt x = x_min;
    realt y_max = value(x);
    vector<realt> xx;
    v_foreach (int, i, ff_.idx) {
        realt ctr;
        if (mgr.get_function(*i)->get_center(&ctr)
                && x_min < ctr && ctr < x_max)
            xx.push_back(ctr);
    }
    xx.push_back(x_max);
    sort(xx.begin(), xx.end());
    v_foreach (realt, i, xx) {
        realt x_between = (x + *i)/2.;
        x = *i;
        realt y = max(value(x_between), value(x));
        if (y > y_max)
            y_max = y;
    }
    return y_max;
}


string Model::get_peak_parameters(const vector<realt>& errors) const
{
    string s;
    const SettingsMgr *sm = F_->settings_mgr();
    s += "# PeakType\tCenter\tHeight\tArea\tFWHM\tparameters...\n";
    v_foreach (int, i, ff_.idx) {
        const Function* p = mgr.get_function(*i);
        s += "%" + p->name + "  " + p->tp()->name;
        realt a;
        if (p->get_center(&a))
            s += "\t" + sm->format_double(a);
        else
            s += "\tx";
        if (p->get_height(&a))
            s += "\t" + sm->format_double(a);
        else
            s += "\tx";
        if (p->get_area(&a))
            s += "\t" + sm->format_double(a);
        else
            s += "\tx";
        if (p->get_fwhm(&a))
            s += "\t" + sm->format_double(a);
        else
            s += "\tx";
        s += "\t";
        for (int j = 0; j < p->used_vars().get_count(); ++j) {
            s += " " + sm->format_double(p->av()[j]);
            if (!errors.empty()) {
                const Variable* var =
                    mgr.get_variable(p->used_vars().get_idx(j));
                if (var->is_simple()) {
                    realt err = errors[var->get_nr()];
                    s += " +/- " + sm->format_double(err);
                }
                else
                    s += " +/- ?";
            }
        }
        s += "\n";
    }
    return s;
}


string Model::get_formula(bool simplify, const char* num_fmt,
                          bool extra_breaks) const
{
    if (ff_.names.empty())
        return "0";
    string shift;
    v_foreach (int, i, zz_.idx) {
        string expr = mgr.get_function(*i)->get_current_formula("x", num_fmt);
        shift += "+" + (simplify ? simplify_formula(expr, num_fmt) : expr);
    }
    string x = "x";
    if (!shift.empty())
        x = "(x" + shift + ")";
    string formula;
    v_foreach (int, i, ff_.idx) {
        string expr = mgr.get_function(*i)->get_current_formula(x, num_fmt);
        if (i != ff_.idx.begin())
            formula += (extra_breaks ? " +\n" : " + ");
        formula += (simplify ? simplify_formula(expr, num_fmt) : expr);
    }
    return formula;
}

const string& Model::get_func_name(char c, int idx) const
{
    const vector<string>& names = get_fz(c).names;
    if (idx < 0)
        idx += names.size();
    if (!is_index(idx, names))
        throw ExecuteError("wrong [index]: " + S(idx));
    return names[idx];
}

realt Model::numarea(realt x1, realt x2, int nsteps) const
{
    x1 += zero_shift(x1);
    x2 += zero_shift(x2);
    realt a = 0;
    v_foreach (int, i, ff_.idx)
        a += mgr.get_function(*i)->numarea(x1, x2, nsteps);
    return a;
}

} // namespace fityk
