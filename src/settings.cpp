// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#include "common.h"
#include "settings.h"
#include "logic.h"
#include "fit.h"
#include <ctype.h>
#include <algorithm>
#include <stdlib.h>
#include <ctime> //time()

using namespace std;

fp epsilon = 1e-12; // declared in common.h

/// small utility used only in constructor
void Settings::insert_enum(string const& name,
                           map<char,string> const& e, char value)
{
    epar.insert(pair<string, EnumString> (name, EnumString(e, value)));
}

Settings::Settings(Ftk const* F_)
    : F(F_)
{
    // general
    map<char, string> verbosity_enum;
    verbosity_enum [-1]= "quiet";
    verbosity_enum [0] = "normal";
    verbosity_enum [1] = "verbose";
    verbosity_enum [2] = "debug";
    insert_enum("verbosity", verbosity_enum, 0);
    verbosity_ = 0;

    map<char, string> autoplot_enum;
    autoplot_enum [1] = "never";
    autoplot_enum [2] = "on_plot_change";
    autoplot_enum [3] = "on_fit_iteration";
    insert_enum("autoplot", autoplot_enum, 2);
    autoplot_ = 2;

    bpar["exit_on_warning"] = false;

    fpar["epsilon"] = epsilon;

    map<char, string> data_sigma_enum;
    data_sigma_enum ['s'] = "sqrt";
    data_sigma_enum ['1'] = "one";
    insert_enum("data_default_sigma", data_sigma_enum, 's');

    // 0 -> time-based seed
    ipar["pseudo_random_seed"] = 0;

    map<char, string> sum_export_style_enum;
    sum_export_style_enum [0] = "normal";
    sum_export_style_enum [1] = "gnuplot";
    insert_enum("formula_export_style", sum_export_style_enum, 0);

    spar["info_numeric_format"] = "%g";

    // Function
    fpar["cut_function_level"] = cut_function_level_ = 0.;

    // guess
    bpar ["can_cancel_guess"] = true;
    fpar ["height_correction"] = 1.;
    fpar ["width_correction"] = 1.;
    fpar ["guess_at_center_pm"] = 1.;

    //Fit
    map<char, string> fitting_method_enum;
    vector<Fit*> const& fm = F->get_fit_container()->get_methods();
    for (int i = 0; i < size(fm); ++i)
        fitting_method_enum[i] = fm[i]->name;
    insert_enum("fitting_method", fitting_method_enum, 0);

    //  - common
    ipar["max_wssr_evaluations"] = 1000;
    ipar["refresh_period"] = 4;
    fpar["variable_domain_percent"] = 30.;

    //  - Lev-Mar
    fpar["lm_lambda_start"] = 0.001;
    fpar["lm_lambda_up_factor"] = 10;
    fpar["lm_lambda_down_factor"] = 10;
    fpar["lm_stop_rel_change"] = 1e-4;
    fpar["lm_max_lambda"] = 1e+15;

    //  - Nelder-Mead
    fpar["nm_convergence"] = 0.0001;
    bpar["nm_move_all"] = false;

    map<char, string> distrib_enum;
    distrib_enum ['u'] = "uniform";
    distrib_enum ['g'] = "gauss";
    distrib_enum ['l'] = "lorentz";
    distrib_enum ['b'] = "bound";
    insert_enum("nm_distribution", distrib_enum, 'b');

    fpar["nm_move_factor"] = 1;

    //  - Genetic Algorithms
    //TODO
}

string Settings::getp(string const& k) const
{
    if (ipar.count(k)) {
        return S(ipar.find(k)->second);
    }
    else if (fpar.count(k)) {
        return S(fpar.find(k)->second);
    }
    else if (bpar.count(k)) {
        return bpar.find(k)->second ? "1" : "0";
    }
    else if (irpar.count(k)) {
        return S(irpar.find(k)->second.v);
    }
    else if (epar.count(k)) {
        EnumString const& ens = epar.find(k)->second;
        return ens.e.find(ens.v)->second;
    }
    else if (spar.count(k)) {
        return spar.find(k)->second;
    }
    else
        throw ExecuteError("Unknown option: " +  k);
}

void Settings::setp_core(string const& k, string const& v)
{
    if (ipar.count (k)) {
        int d;
        if (istringstream (v) >> d) {
            ipar[k] = d;
            if (k == "pseudo_random_seed")
                do_srand();
            return;
        }
    }
    else if (fpar.count (k)){
        fp d;
        if (istringstream (v) >> d) {
            fpar[k] = d;
            //optimization
            if (k == "cut_function_level")
                cut_function_level_ = d;
            else if (k == "epsilon") {
                if (d <= 0.) {
                    throw ExecuteError("Value of epsilon must be positive.");
                }
                epsilon = d;
            }
            return;
        }
    }
    else if (bpar.count (k)){
        bool d;
        if (istringstream (v) >> d) {
            bpar[k] = d;
            return;
        }
    }
    else if (irpar.count (k)) {
        int d = -1;
        istringstream (v) >> d;
        if (irpar[k].l <= d && d <= irpar[k].u) {
            irpar[k].v = d;
            return;
        }
    }
    else if (epar.count (k)){
        EnumString& t = epar.find(k)->second;
        for (map<char,string>::const_iterator i = t.e.begin();
                                                         i != t.e.end(); i++) {
            if (i->second == v) {
                t.v = i->first;
                //optimization
                if (k == "verbosity")
                    verbosity_ = t.v;
                if (k == "autoplot")
                    autoplot_ = t.v;
                return;
            }
        }
    }
    else if (spar.count (k)){
        spar[k] = v;
        return;
    }
    throw ExecuteError("'" + v + "' is not a valid value for '" + k + "'");
}

string Settings::infop (string const& k)
{
    return k + " = " + getp(k) + "\ntype: " + typep(k);
}

void Settings::setp (string const& k, string const& v)
{
    string sp = getp(k);
    if (sp == v)
        F->msg("Option '" + k + "' already has value: " + v);
    else {
        setp_core (k, v);
        F->msg("Value for '" + k + "' changed from '" + sp + "' to '" + v+"'");
    }
}

string Settings::typep (string const& k) const
{
    if (ipar.count (k)){
        return "integer number";
    }
    else if (fpar.count (k)){
        return "real number";
    }
    else if (bpar.count (k)){
        return "boolean (0/1)";
    }
    else if (irpar.count (k)){
        int u = irpar.find(k)->second.u;
        int l = irpar.find(k)->second.l;
        assert(u - l >= 1);
        return "integer from range: " + S(l) + ", ..., " + S(u);
    }
    else if (epar.count (k)){
        map<char,string> const& e = epar.find(k)->second.e;
        return "one of: " + join_vector(get_map_values(e), ", ");
    }
    else if (spar.count (k)){
        return "string (a-zA-Z0-9+-.)";
    }
    else
        throw ExecuteError("Unknown option: " +  k);
}

Settings::ValueType Settings::get_value_type(std::string const& k) const
{
    if (ipar.count(k))
        return kInt;
    else if (fpar.count(k))
        return kFloat;
    else if (bpar.count(k))
        return kBool;
    else if (irpar.count(k))
        return kIntFromRange;
    else if (epar.count (k))
        return kStringEnum;
    else if (spar.count(k))
        return kString;
    else
        return kNotFound;
}

vector<string> Settings::expanp(string const& k) const
{
    vector<string> e;
    int len = k.size();
    for (map<string,int>::const_iterator i = ipar.begin(); i!=ipar.end();i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map<string,fp>::const_iterator i = fpar.begin(); i!=fpar.end(); i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map<string,bool>::const_iterator i = bpar.begin();i!=bpar.end();i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map<string, IntRange>::const_iterator i = irpar.begin();
                                                        i != irpar.end(); i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map<string, EnumString>::const_iterator i = epar.begin();
                                                          i != epar.end(); i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map<string, string>::const_iterator i = spar.begin();
                                                          i != spar.end(); i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    sort(e.begin(), e.end());
    return e;
}

vector<string> Settings::expand_enum(string const& k, string const& t) const
{
    vector<string> r;
    if (epar.count(k) == 0)
        throw ExecuteError("Unknown option: " +  k);
    map<char, string> const& es = epar.find(k)->second.e;
    for (map<char,string>::const_iterator i = es.begin(); i != es.end(); i++)
        if (!string(i->second, 0, t.size()).compare(t))
            r.push_back (i->second);
    return r;
}

string Settings::print_usage() const
{
    string s = "Usage: \n\tset option = value\n"
        "or, to see the current value: \n\t"
        "set option\n"
        "Available options:";
    vector<string> e = expanp();
    for (vector<string>::const_iterator i = e.begin(); i != e.end(); i++) {
        s += "\n " + *i + " = <" + typep(*i) + ">, current value: "+getp(*i);
    }
    return s;
}

/// it doesn't set autoplot and verbosity options
string Settings::set_script() const
{
    vector<string> e = expanp();
    string s;
    for (vector<string>::const_iterator i = e.begin(); i != e.end(); i++) {
        if (*i == "autoplot" || *i == "verbosity")
            continue;
        string v = getp(*i);
        s += "set " + *i + " = " + (v.empty() ? "\"\"" : v) + "\n";
    }
    return s;
}

void Settings::do_srand()
{
    int random_seed = get_i("pseudo_random_seed");
    int rs = random_seed > 0 ? random_seed : (int) time(NULL);
    srand(rs);
    F->vmsg("Seed for a sequence of pseudo-random numbers: " + S(rs));
}

void Settings::set_temporary(std::string const& k, std::string const& v)
{
    old_values.push_back(make_pair(k, getp(k)));
    setp_core(k, v);
}

void Settings::clear_temporary()
{
    while(!old_values.empty()) {
        setp_core(old_values.back().first, old_values.back().second);
        old_values.pop_back();
    }
}



