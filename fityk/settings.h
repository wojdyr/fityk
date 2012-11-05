// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__SETTINGS__H__
#define FITYK__SETTINGS__H__
#include <map>
#include <utility>
#include "common.h"

namespace fityk {

class Ftk;

// settings that can be changed using the set command
struct FITYK_API Settings
{
    // general
    int verbosity;
    bool autoplot;
    const char* on_error;
    double epsilon; // for now, there is also global epsilon
    const char* default_sigma;
    int pseudo_random_seed;
    std::string numeric_format;
    std::string logfile;
    bool log_full;
    double function_cutoff;
    std::string cwd; // current working directory

    // guess
    double height_correction;
    double width_correction;
    bool guess_uses_weights;

    // fitting
    const char* fitting_method;
    int max_wssr_evaluations;
    double max_fitting_time;
    int refresh_period;
    bool fit_replot;
    double domain_percent;
    // fitting - LM
    double lm_lambda_start;
    double lm_lambda_up_factor;
    double lm_lambda_down_factor;
    double lm_stop_rel_change;
    double lm_max_lambda;
    // fitting - NM
    double nm_convergence;
    bool nm_move_all;
    const char* nm_distribution;
    double nm_move_factor;
};

/// Wraps struct Settings
class FITYK_API SettingsMgr
{
public:
    enum ValueType
    {
        kInt,
        kDouble,
        kBool,
        kString,
        kEnum,
        kNotFound // used as a return value from get_value_type()
    };

    SettingsMgr(Ftk const* F);

    /// get all option keys that start with given string
    static std::vector<std::string> get_key_list (const std::string& start);

    /// returns NULL-terminated list of values for kEnum type, NULL otherwise
    static const char** get_allowed_values(const std::string& k);

    /// return value type of the option
    static ValueType get_value_type(const std::string& k);

    /// get text information about type of option k
    static std::string get_type_desc(const std::string& k);

    // getters
    const Settings& m() const { return m_; }
    /// get value of option as string
    std::string get_as_string(const std::string& k) const;
    /// get kEnum index
    int get_enum_index(const std::string& k) const;

    // setters
    void set_as_string(const std::string& k, const std::string& v);
    void set_as_number(const std::string& k, double v);
    void set_all(const Settings& s) { m_ = s; epsilon = s.epsilon; }

    // utilities that use settings
    void do_srand();
    std::string format_double(double d) const
            { return format1<double, 32>(m_.numeric_format.c_str(), d); }
#if USE_LONG_DOUBLE
    std::string format_double(long double d) const
            { return format1<long double, 64>(long_double_format_.c_str(), d); }
#endif

private:
    const Ftk* F_; // used for msg()
    Settings m_;
    std::string long_double_format_;

    void set_long_double_format(const std::string& double_fmt);
    DISALLOW_COPY_AND_ASSIGN(SettingsMgr);
};

} // namespace fityk
#endif

