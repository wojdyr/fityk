// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "settings.h"

#include <algorithm>
#include <assert.h>
#include <stdlib.h>
#include <ctime> //time()
#ifdef _WIN32
#include <windows.h> // SetCurrentDirectoryA()
#else
#include <unistd.h> // chdir()
#endif
// <config.h> is included from common.h from settings.h
#if HAVE_LIBNLOPT
# include <nlopt.h>
#endif
#include "logic.h"
#include "fit.h"

using namespace std;

namespace fityk {

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

double epsilon = 1e-12; // declared in common.h

static const char* on_error_enum[] =
{ "nothing", "stop", "exit", NULL };

static const char* default_sigma_enum[] =
{ "sqrt", "one", NULL };

static const char* nm_distribution_enum[] =
{ "bound", "uniform", "gauss", "lorentz", NULL };

// note: omitted elements are set to 0
static const char* fit_method_enum[20] = { NULL };

#define OPT(name, type, ini, allowed) \
{ #name, SettingsMgr::type, OptVal(&Settings::name, ini), allowed }

static const Option options[] = {
    OPT(verbosity, kInt, 0, NULL),
    OPT(autoplot, kBool, true, NULL),
    OPT(on_error, kEnum, on_error_enum[1], on_error_enum),
    OPT(epsilon, kDouble, 1e-12, NULL),
    OPT(default_sigma, kEnum, default_sigma_enum[0], default_sigma_enum),
    OPT(pseudo_random_seed, kInt, 0, NULL),
    OPT(numeric_format, kString, "%g", NULL),
    OPT(logfile, kString, "", NULL),
    OPT(log_output, kBool, false, NULL),
    OPT(function_cutoff, kDouble, 0., NULL),
    OPT(cwd, kString, "", NULL),

    OPT(height_correction, kDouble, 1., NULL),
    OPT(width_correction, kDouble, 1., NULL),
    OPT(guess_uses_weights, kBool, true, NULL),

    OPT(fitting_method, kEnum, FitManager::method_list[0][0], fit_method_enum),
    OPT(max_wssr_evaluations, kInt, 1000, NULL),
    OPT(max_fitting_time, kDouble, 0., NULL),
    OPT(refresh_period, kInt, 4, NULL),
    OPT(fit_replot, kBool, false, NULL),
    OPT(domain_percent, kDouble, 30., NULL),
    OPT(box_constraints, kBool, true, NULL),

    OPT(lm_lambda_start, kDouble, 0.001, NULL),
    OPT(lm_lambda_up_factor, kDouble, 10, NULL),
    OPT(lm_lambda_down_factor, kDouble, 10, NULL),
    OPT(lm_max_lambda, kDouble, 1e+15, NULL),
    OPT(lm_stop_rel_change, kDouble, 1e-7, NULL),
    OPT(ftol_rel, kDouble, 0, NULL),
    OPT(xtol_rel, kDouble, 0, NULL),
    //OPT(mpfit_gtol, kDouble, 1e-10, NULL),

    OPT(nm_convergence, kDouble, 0.0001, NULL),
    OPT(nm_move_all, kBool, false, NULL),
    OPT(nm_distribution, kEnum, nm_distribution_enum[0], nm_distribution_enum),
    OPT(nm_move_factor, kDouble, 1., NULL),
};

static
const Option& find_option(const string& name)
{
    size_t len = sizeof(options) / sizeof(options[0]);
    for (size_t i = 0; i != len; ++i)
        if (options[i].name == name)
            return options[i];
    if (name == "log_full") // old name used in fityk 1.2.9 and older
        return find_option("log_output");
    throw ExecuteError("Unknown option: " +  name);
}

static
void change_current_working_dir(const char* path)
{
#ifdef _WIN32
    BOOL ok = SetCurrentDirectoryA(path);
#else
    bool ok = (chdir(path) == 0);
#endif
    if (!ok)
        throw ExecuteError("Changing current working directory failed.");
}

SettingsMgr::SettingsMgr(BasicContext const* ctx)
    : ctx_(ctx)
{
    for (int i = 0; FitManager::method_list[i][0]; ++i)
        fit_method_enum[i] = FitManager::method_list[i][0];
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
    set_long_double_format(m_.numeric_format);
}

void SettingsMgr::set_long_double_format(const string& double_fmt)
{
    long_double_format_ = double_fmt;
    size_t pos = double_fmt.find_last_of("aAeEfFgG");
    if (pos != string::npos && double_fmt[pos] != 'L')
        long_double_format_.insert(pos, "L");
}

string SettingsMgr::get_as_string(string const& k, bool quote_str) const
{
    const Option& opt = find_option(k);
    if (opt.vtype == kInt)
        return S(m_.*opt.val.i.ptr);
    else if (opt.vtype == kDouble)
        return S(m_.*opt.val.d.ptr);
    else if (opt.vtype == kBool)
        return m_.*opt.val.b.ptr ? "1" : "0";
    else if (opt.vtype == kString) {
        string v = m_.*opt.val.s.ptr;
        return quote_str ? "'" + v + "'" : v;
    } else if (opt.vtype == kEnum)
        return S(m_.*opt.val.e.ptr);
    assert(0);
    return "";
}

double SettingsMgr::get_as_number(string const& k) const
{
    const Option& opt = find_option(k);
    if (opt.vtype == kInt)
        return double(m_.*opt.val.i.ptr);
    else if (opt.vtype == kDouble)
        return m_.*opt.val.d.ptr;
    else if (opt.vtype == kBool)
        return double(m_.*opt.val.b.ptr);
    throw ExecuteError("Not a number: option " +  k);
    //return 0; // avoid compiler warning
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
        ctx_->msg("Option '" + k + "' already has value: " + v);
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
        } else if (k == "numeric_format") {
            if (count(v.begin(), v.end(), '%') != 1)
                throw ExecuteError("Exactly one `%' expected, e.g. '%.9g'");
            set_long_double_format(v);
        } else if (k == "cwd") {
            change_current_working_dir(v.c_str());
        }
        m_.*opt.val.s.ptr = v;
    } else { // if (opt.vtype == kEnum)
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
        ctx_->msg("Option '" + k + "' already has value: " + sp);
        return;
    }
    const Option& opt = find_option(k);
    assert(opt.vtype == kInt || opt.vtype == kDouble || opt.vtype == kBool);
    if (opt.vtype == kInt) {
        m_.*opt.val.i.ptr = iround(d);
        if (k == "pseudo_random_seed")
            do_srand();
    } else if (opt.vtype == kDouble) {
        if (k == "epsilon") {
            if (d <= 0.)
                throw ExecuteError("Value of epsilon must be positive.");
            epsilon = d;
        }
        m_.*opt.val.d.ptr = d;
    } else // if (opt.vtype == kBool)
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
#if HAVE_LIBNLOPT
    nlopt_srand(seed);
#endif
}

} // namespace fityk
