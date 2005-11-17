// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "func.h"
#include "var.h"
#include "voigt.h"
#include "ui.h"
#include <memory>
#include <boost/spirit/core.hpp>

using namespace std; 
using namespace boost::spirit;

std::vector<fp> Function::calc_val_xx(1); 
std::vector<fp> Function::calc_val_yy(1);

Function::Function (string const &name_, vector<string> const &vars,
                    string const &formula_, VariableManager *m)
    : VariableUser(name_, "%"), type_formula(formula_),
      type_name(strip_string(string(formula_,0,formula_.find_first_of("(")+1))),
      type_rhs(strip_string(string(formula_, formula_.find('=')+1))),
      cutoff_level(0.), vv(vars.size()), mgr(m)
{
    // parsing formula for every instance of the class is not effective 
    // but the overhead is negligible
    // the ease of adding new built-in function types is more important
    // is there a better way to do it? (MW)
    int eq = formula_.find('=');
    string all_t_names(formula_, formula_.find('(')+1, formula_.rfind(')', eq));
    type_var_names = split_string(all_t_names, ',');
    for (vector<string>::iterator i = type_var_names.begin(); 
            i != type_var_names.end(); ++i)
        *i = strip_string(*i);

    if (type_var_names.size() != vars.size())
        throw ExecuteError("Function " + type_name + " requires " 
                           + S(type_var_names.size()) + " parameters.");
    for (vector<string>::const_iterator i = vars.begin(); i != vars.end(); ++i){
        bool just_name = parse(i->c_str(), VariableLhsG).full;
        varnames.push_back(just_name ? string(*i, 1) 
                                     : m->assign_variable("", *i));
    }
    set_var_idx(mgr->get_variables());

    //?? gnuplot_formula =  //^->**, type_var_names->varnames
    //                        ln->...
    
}

Function* Function::factory (string const &name, string const &type_name,
                             vector<string> const &vars, VariableManager *m) 
{
    if (type_name == "Constant")
        return new FuncConstant(name, vars, m);
    else if (type_name == "Gaussian")
        return new FuncGaussian(name, vars, m);
#if 0
    else if (type_name == "Lorentzian")
        return new fLorentz(name, vars);
    else if (type_name == "PearsonVII")
        return new fPearson(name, vars);
    else if (type_name == "Pseudo-Voigt")
        return new fPsVoigt(name, vars);
    else if (type_name == "Voigt")
        return new fVoigt(name, vars);
    else if (type_name == "Polynomial5")
        return new fPolynomial5(name, vars);
#endif
    else 
        throw ExecuteError("Undefined type of function: " + type_name);
}

void Function::do_precomputations(vector<Variable*> const &variables)
{
    //precondition: recalculate() for all variables
    multi.clear();
    for (int i = 0; i < size(var_idx); ++i) {
        Variable const *v = variables[var_idx[i]];
        vv[i] = v->get_value(); 
        vector<Variable::ParMult> const &pm = v->get_recursive_derivatives();
        for (vector<Variable::ParMult>::const_iterator j = pm.begin();
                j != pm.end(); ++j)
            multi.push_back(Multi(i, *j));
    }
}

void Function::get_nonzero_idx_range(std::vector<fp> const &x,
                                     int &first, int &last) const
{
    //precondition: x is sorted
    fp left, right;
    bool r = get_nonzero_range(cutoff_level, left, right);
    if (r) {
        first = lower_bound(x.begin(), x.end(), left) - x.begin(); 
        last = upper_bound(x.begin(), x.end(), right) - x.begin(); 
    }
    else {
        first = 0; 
        last = x.size();
    }
}

fp Function::calculate_value(fp x) const
{
    calc_val_xx[0] = x;
    calc_val_yy[0] = 0.;
    calculate_value(calc_val_xx, calc_val_yy);
    return calc_val_yy[0];
}

string Function::get_info(vector<Variable*> const &variables, 
                          vector<fp> const &parameters, 
                          bool extended) const 
{ 
    vector<string> xvarnames; 
    for (vector<int>::const_iterator i = var_idx.begin(); 
            i != var_idx.end(); ++i)
        xvarnames.push_back(variables[*i]->xname);
    string s = xname+" = "+type_name+ "(" + join_vector(xvarnames, ", ") + ")";
    if (extended) {
        s += "\n" + type_formula;
        for (vector<int>::const_iterator i = var_idx.begin(); 
                i != var_idx.end(); ++i)
            s += "\n" + variables[*i]->get_info(parameters);
    }
    return s;
} 


///////////////////////////////////////////////////////////////////////

const char *FuncConstant::formula 
= "Constant(a) = a"; 

void FuncConstant::calculate_value(vector<fp> const &/*x*/, vector<fp> &y) const
{
    for (vector<fp>::iterator i = y.begin(); i != y.end(); ++i)
        *i += vv[0];
}

void FuncConstant::calculate_value_deriv(vector<fp> const &x, 
                                         vector<fp> &y, 
                                         vector<fp> &dy_da,
                                         bool in_dx) const
{
    // dy_da.size() == x.size() * (parameters.size()+1)
    int dyn = dy_da.size() / x.size();
    vector<fp> dy_dv(vv.size());
    for (int i = 0; i < size(y); ++i) {
        dy_dv[0] = 1.;
        fp dy_dx = 0;

        if (!in_dx) {
            y[i] += vv[0];

            for (vector<Multi>::const_iterator j = multi.begin(); 
                    j != multi.end(); ++j)
                dy_da[dyn*i+j->p] += dy_dv[j->n] * j->mult;
            dy_da[dyn*i+dyn-1] += dy_dx;
        }
        else { //in dx
            for (vector<Multi>::const_iterator j = multi.begin(); 
                    j != multi.end(); ++j)
                dy_da[dyn*i+j->p] += dy_da[dyn*i+dyn-1] * dy_dv[j->n] * j->mult;
        }
    }
}

///////////////////////////////////////////////////////////////////////

const char *FuncGaussian::formula 
= "Gaussian(height, center, HWHM) = "
                        "height*exp(-ln(2)*((x-center)/HWHM)^2)"; 


void FuncGaussian::calculate_value(vector<fp> const &x, vector<fp> &y) const
{
    int first, last;
    get_nonzero_idx_range(x, first, last);
    for (int i = first; i < last; ++i) {
        fp xa1a2 = (x[i] - vv[1]) / vv[2];
        fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
        y[i] += vv[0] * ex;
    }
}

void FuncGaussian::calculate_value_deriv(vector<fp> const &x, 
                                         vector<fp> &y, 
                                         vector<fp> &dy_da,
                                         bool in_dx) const
{
    int first, last;
    get_nonzero_idx_range(x, first, last);
    int dyn = x.size() / dy_da.size();
    vector<fp> dy_dv(vv.size());
    for (int i = first; i < last; ++i) {

        fp xa1a2 = (x[i] - vv[1]) / vv[2];
        fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
        dy_dv[0] = ex;
        fp dcenter = 2 * M_LN2 * vv[0] * ex * xa1a2 / vv[2];
        dy_dv[1] = dcenter;
        dy_dv[2] = dcenter * xa1a2;
        fp dy_dx = -dcenter;

        if (!in_dx) {
            y[i] += vv[0] * ex;

            for (vector<Multi>::const_iterator j = multi.begin(); 
                    j != multi.end(); ++j)
                dy_da[dyn*i+j->p] += dy_dv[j->n] * j->mult;
            dy_da[dyn*i+dyn-1] += dy_dx;
        }
        else { //in dx
            for (vector<Multi>::const_iterator j = multi.begin(); 
                    j != multi.end(); ++j)
                dy_da[dyn*i+j->p] += dy_da[dyn*i+dyn-1] * dy_dv[j->n] * j->mult;
        }
    }
}

bool FuncGaussian::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level <= 0)
        return false;
    else if (level >= vv[0])
        left = right = 0;
    else {
        fp w = sqrt (log (vv[0] / level) / M_LN2) * vv[2]; 
        left = vv[1] - w;             
        right = vv[1] + w;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////


