// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__GUESS__H__
#define FITYK__GUESS__H__

enum FunctionKind { fk_peak, fk_linear /*, fk_step*/, fk_unknown };

class DataWithSum;
class Ftk;
class Data;

/// guessing initial parameters of functions
class Guess
{
public:
    Guess(Ftk const *F_, DataWithSum const* ds_);
    std::string get_guess_info(std::vector<std::string> const& range);
    void guess(std::string const& name, std::string const& function,
               std::vector<std::string> const& range,
               std::vector<std::string>& vars);
protected:
    Ftk const* const F;
    Data const* const data;
    // only these peaks/functions are considered
    std::vector<int> real_peaks;

    void estimate_peak_parameters (fp range_from, fp range_to, 
                                   fp *center, fp *height, fp *area, fp *fwhm);
    void estimate_linear_parameters(fp range_from, fp range_to,
                                    fp *slope, fp *intercept, fp *avgy);
    // helper used by estimate_xxxx_parameters()
    void get_point_range(fp range_from, fp range_to, int &l_bor, int &r_bor);

    fp my_y (int n);
    fp data_area (int from, int to); 
    int max_data_y_pos (int from, int to);
    fp compute_data_fwhm (int from, int max_pos, int to, fp level);
    void parse_range(std::vector<std::string> const& range, 
                     fp& range_from, fp& range_to);
    void remove_peak(std::string const& name);
};


bool is_function_guessable(std::string const& formula, 
                           bool check_defvalue=true);

bool is_function_guessable(std::vector<std::string> const& vars, 
                           std::vector<std::string> const& defv,
                           FunctionKind* fk);

bool is_defvalue_guessable(std::string defvalue, FunctionKind k);
FunctionKind get_function_kind(std::string const& formula);

#endif

