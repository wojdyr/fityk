// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__GUESS__H__
#define FITYK__GUESS__H__

enum FunctionKind { fk_peak, fk_linear /*, fk_step*/, fk_unknown };

/// used for passing peak-estimation conditions 
/// to e.g. Manipul::estimate_peak_parameters() 
struct EstConditions
{
    // only these peaks/functions are considered
    std::vector<int> real_peaks;
};

class DataWithSum;

void estimate_peak_parameters (DataWithSum const* ds,
                               fp range_from, fp range_to, 
                               fp *center, fp *height, fp *area, fp *fwhm,
                               EstConditions const* ec=0);
std::string get_guess_info(DataWithSum const* ds, 
                              std::vector<std::string> const& range);
void guess_and_add(DataWithSum* ds,
                   std::string const& name, std::string const& function,
                   std::vector<std::string> const& range,
                   std::vector<std::string> vars);

bool is_function_guessable(std::string const& formula, 
                           bool check_defvalue=true);

bool is_function_guessable(std::vector<std::string> const& vars, 
                           std::vector<std::string> const& defv,
                           FunctionKind* fk);

bool is_defvalue_guessable(std::string defvalue, FunctionKind k);
FunctionKind get_function_kind(std::string const& formula);

#endif

