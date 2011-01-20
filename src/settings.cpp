// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include "common.h"
#include "settings.h"
#include "logic.h"
#include "fit.h"
#include <ctype.h>
#include <algorithm>
#include <stdlib.h>
#include <ctime> //time()

using namespace std;

union OptVal
{
    struct { int Settings::*ptr; int ini; } i;
    struct { double Settings::*ptr; double ini; } d;
    struct { bool Settings::*ptr; bool ini; } b;
    struct { string Settings::*ptr; const char* ini; } s;
    struct { const char* Settings::*ptr; const char* ini; } e;

    OptVal(int Settings::*p, int ini) { i.ptr = p; i.ini = ini; }
    OptVal(double Settings::*p, double ini) { d.ptr = p; d.ini = ini; }
    OptVal(bool Settings::*p, bool ini) { b.ptr = p; b.ini = ini; }
    OptVal(string Settings::*p, const char* ini) { s.ptr = p; s.ini = ini; }
    OptVal(const char* Settings::*p, const char* ini) { e.ptr = p; e.ini = ini;}
};

struct Option
{
    const char* name;
    SettingsMgr::ValueType vtype;
    OptVal val;
    const char** allowed_values; // used only for kStringEnum
};

fp epsilon = 1e-12; // declared in common.h

static const char* default_sigma_enum[] =
{ "sqrt", "one", NULL };

static const char* fitting_method_enum[] =
{ "levenberg_marquardt", "nelder_mead_simplex", "genetic_algorithms", NULL };

static const char* nm_distribution_enum[] =
{ "bound", "uniform", "gauss", "lorentz", NULL };

#define OPT(name, type, ini, allowed) \
{ #name, SettingsMgr::type, OptVal(&Settings::name, ini), allowed }

static const Option options[] = {
    OPT(verbosity, kInt, 0, NULL),
    OPT(autoplot, kBool, true, NULL),
    OPT(exit_on_warning, kBool, false, NULL),
    OPT(epsilon, kDouble, 1e-12, NULL),
    OPT(default_sigma, kEnum, default_sigma_enum[0], default_sigma_enum),
    OPT(pseudo_random_seed, kInt, 0, NULL),
    OPT(numeric_format, kString, "%g", NULL),
    OPT(logfile, kString, "", NULL),
    OPT(log_full, kBool, false, NULL),
    OPT(cut_function_level, kDouble, 0., NULL),

    OPT(can_cancel_guess, kBool, true, NULL),
    OPT(height_correction, kDouble, 1., NULL),
    OPT(width_correction, kDouble, 1., NULL),

    OPT(fitting_method, kEnum, fitting_method_enum[0], fitting_method_enum),
    OPT(max_wssr_evaluations, kInt, 1000, NULL),
    OPT(refresh_period, kInt, 4, NULL),
    OPT(fit_replot, kBool, false, NULL),
    OPT(variable_domain_percent, kDouble, 30., NULL),

    OPT(lm_lambda_start, kDouble, 0.001, NULL),
    OPT(lm_lambda_up_factor, kDouble, 10, NULL),
    OPT(lm_lambda_down_factor, kDouble, 10, NULL),
    OPT(lm_stop_rel_change, kDouble, 1e-4, NULL),
    OPT(lm_max_lambda, kDouble, 1e+15, NULL),

    OPT(nm_convergence, kDouble, 0.0001, NULL),
    OPT(nm_move_all, kBool, false, NULL),
    OPT(nm_distribution, kEnum, nm_distribution_enum[0], nm_distribution_enum),
    OPT(nm_move_factor, kDouble, 1., NULL),
};

const Option& find_option(const string& name)
{
    size_t len = sizeof(options) / sizeof(options[0]);
    for (size_t i = 0; i != len; ++i)
        if (options[i].name == name)
            return options[i];
    throw ExecuteError("Unknown option: " +  name);
}

SettingsMgr::SettingsMgr(Ftk const* F)
    : F_(F)
{
    size_t len = sizeof(options) / sizeof(options[0]);
    for (size_t i = 0; i != len; ++i) {
        const Option& opt = options[i];
        if (opt.vtype == kInt)
            m_.*opt.val.i.ptr = opt.val.i.ini;
        else if (opt.vtype == kDouble)
            m_.*opt.val.d.ptr = opt.val.d.ini;
        else if (opt.vtype == kBool)
            m_.*opt.val.b.ptr = opt.val.b.ini;
        else if (opt.vtype == kString)
            m_.*opt.val.s.ptr = opt.val.s.ini;
        else if (opt.vtype == kEnum)
            m_.*opt.val.e.ptr = opt.val.e.ini;
    }
}

string SettingsMgr::get_as_string(string const& k) const
{
    const Option& opt = find_option(k);
    if (opt.vtype == kInt)
        return S(m_.*opt.val.i.ptr);
    else if (opt.vtype == kDouble)
        return S(m_.*opt.val.d.ptr);
    else if (opt.vtype == kBool)
        return m_.*opt.val.b.ptr ? "1" : "0";
    else if (opt.vtype == kString)
        return "'" + S(m_.*opt.val.s.ptr) + "'";
    else if (opt.vtype == kEnum)
        return S(m_.*opt.val.e.ptr);
    assert(0);
    return "";
}

int SettingsMgr::get_enum_index(string const& k) const
{
    const Option& opt = find_option(k);
    assert(opt.vtype == kEnum);
    const char* val = m_.*opt.val.e.ptr;
    const char** av = opt.allowed_values;
    int n = 0;
    while (*av[n]) {
        if (val == av[n])
            break;
        ++n;
    }
    assert(*av[n]);
    return n;
}

void SettingsMgr::set_as_string(string const& k, string const& v)
{
    string sp = get_as_string(k);
    if (sp == v) {
        F_->msg("Option '" + k + "' already has value: " + v);
        return;
    }
    const Option& opt = find_option(k);
    assert(opt.vtype == kString || opt.vtype == kEnum);
    if (opt.vtype == kString) {
        if (k == "logfile" && !v.empty()) {
            FILE* f = fopen(v.c_str(), "a");
            if (!f)
                throw ExecuteError("Cannot open file for writing: " + v);
            // time_now() ends with "\n"
            fprintf(f, "%s. LOG START: %s", fityk_version_line,
                                              time_now().c_str());
            fclose(f);
        }
        m_.*opt.val.s.ptr = v;
    }
    else { // if (opt.vtype == kEnum)
        const char **ptr = opt.allowed_values;
        while (*ptr) {
            if (*ptr == v) {
                m_.*opt.val.e.ptr = *ptr;
                return;
            }
            ++ptr;
        }
        throw ExecuteError("`" + v + "' is not a valid value for `" + k + "'");
    }
}

void SettingsMgr::set_as_number(string const& k, double d)
{
    string sp = get_as_string(k);
    if (sp == S(d)) {
        F_->msg("Option '" + k + "' already has value: " + sp);
        return;
    }
    const Option& opt = find_option(k);
    assert(opt.vtype == kInt || opt.vtype == kDouble || opt.vtype == kBool);
    if (opt.vtype == kInt) {
        m_.*opt.val.i.ptr = iround(d);
        if (k == "pseudo_random_seed")
            do_srand();
    }
    else if (opt.vtype == kDouble) {
        if (k == "epsilon") {
            if (d <= 0.)
                throw ExecuteError("Value of epsilon must be positive.");
            epsilon = d;
        }
        m_.*opt.val.d.ptr = d;
    }
    else // if (opt.vtype == kBool)
        m_.*opt.val.b.ptr = (fabs(d) >= 0.5);
}

SettingsMgr::ValueType SettingsMgr::get_value_type(const string& k)
{
    try {
        return find_option(k).vtype;
    }
    catch (ExecuteError&) {
        return kNotFound;
    }
}

string SettingsMgr::get_type_desc(const string& k)
{
    const Option& opt = find_option(k);
    switch (opt.vtype) {
        case kInt: return "integer number";
        case kDouble: return "real number";
        case kBool: return "boolean (0/1)";
        case kString: return "'string'";
        case kEnum: {
            const char **ptr = opt.allowed_values;
            string s = "one of: " + S(*ptr);
            while (*++ptr)
                s += S(", ") + *ptr;
            return s;
        }
        case kNotFound: assert(0);
    }
    return "";
}

vector<string> SettingsMgr::get_key_list(const string& start)
{
    vector<string> v;
    size_t len = sizeof(options) / sizeof(options[0]);
    for (size_t i = 0; i != len; ++i)
        if (startswith(options[i].name, start))
            v.push_back(options[i].name);
    sort(v.begin(), v.end());
    return v;
}

const char** SettingsMgr::get_allowed_values(const std::string& k)
{
    return find_option(k).allowed_values;
}

void SettingsMgr::do_srand()
{
    int seed = m_.pseudo_random_seed == 0 ? (int) time(NULL)
                                          : m_.pseudo_random_seed;
    srand(seed);
    F_->vmsg("Seed for pseudo-random numbers: " + S(seed));
}

