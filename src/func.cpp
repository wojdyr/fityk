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


vector<Function*> functions;

void assign_func(std::string const &name, std::string const &function, 
                 std::vector<std::string> const &vars)
{
    Function *func = Function::factory(name, function, vars);
    //if there is already function with the same name -- replace
    bool found = false;
    for (int i = 0; i < size(functions); ++i) {
        if (functions[i]->name == func->name) {
            delete functions[i];
            functions[i] = func;
            mesg("New function %"+func->name+" replaced the old one.");
            remove_unreffered();
            found = true;
            break;
        }
    }
    if (!found) {
        functions.push_back(func);
        info("New function %" + func->name + " was created.");
    }
}


Function::Function (string const &name_, vector<string> const &vars,
                    string const &formula_)
    : name(name_), type_formula(formula_),
      type_name(strip_string(string(formula_, 0, formula_.find_first_of("(")))),
      type_rhs(strip_string(string(formula_, formula_.find('=')+1))),
      cutoff_level(0.), vv(vars.size())
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
        varnames.push_back(just_name ? string(*i, 1) : assign_variable("", *i));
    }

    //?? gnuplot_formula =  //^->**, type_var_names->varnames
    //                        ln->...
    
}

Function* Function::factory (string const &name, string const &function,
                             vector<string> const &vars) 
{
    if (name == "Constant")
        return new FuncConstant(name, vars);
    else if (name == "Gaussian")
        return new FuncGaussian(name, vars);
#if 0
    else if (name == "Lorentzian")
        return new fLorentz(name, vars);
    else if (name == "PearsonVII")
        return new fPearson(name, vars);
    else if (name == "Pseudo-Voigt")
        return new fPsVoigt(name, vars);
    else if (name == "Voigt")
        return new fVoigt(name, vars);
    else if (name == "Polynomial5")
        return new fPolynomial5(name, vars);
#endif
    else 
        throw ExecuteError("Undefined type of function: " + function);
}

void Function::do_precomputations()
{
    //precondition: recalculate_variables() 
    multi.clear();
    for (int i = 0; i < size(varnames); ++i) {
        vv[i] = variables[i]->get_value(); 
        vector<Variable::ParMult> const &pm 
                                  = variables[i]->get_recursive_derivatives();
        for (vector<Variable::ParMult>::const_iterator j = pm.begin();
                j != pm.end(); ++j)
            multi.push_back(Multi(i, *j));
    }
}

void Function::get_nonzero_idx_range(std::vector<fp> const &x,
                                     int &first, int &last)
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


///////////////////////////////////////////////////////////////////////

const char *FuncConstant::formula 
= "Constant(a) = a"; 

void FuncConstant::calculate_value(vector<fp> const &/*x*/, vector<fp> &y) 
{
    for (vector<fp>::iterator i = y.begin(); i != y.end(); ++i)
        *i += vv[0];
}

void FuncConstant::calculate_value_deriv(vector<fp> const &x, 
                                         vector<fp> &y, 
                                         vector<fp> &dy_da,
                                         bool in_dx)
{
    int dyn = x.size() / dy_da.size();
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


void FuncGaussian::calculate_value(vector<fp> const &x, vector<fp> &y) 
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
                                         bool in_dx)
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


