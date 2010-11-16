// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__GUESS__H__
#define FITYK__GUESS__H__

#include <vector>

class DataAndModel;
class Settings;


/// guessing initial parameters of functions
class Guess
{
public:
    enum Kind { kPeak, kLinear /*, kStep*/, kUnknown };

    Guess(Settings const *settings);
    //void initialize(std::vector<fp> const& xx, std::vector<fp> const& yy)
    //                                                 { xx_ = xx; yy_ = yy; }
    void initialize(const DataAndModel* dm, int lb, int rb, int ignore_idx);
    void guess(std::string const& function, std::vector<std::string>& par_names,
                                        std::vector<std::string>& par_values);
    void get_guess_info(std::string& result);

private:
    Settings const* settings_;
    std::vector<fp> xx_, yy_;

    void estimate_peak_parameters (fp *center, fp *height, fp *area, fp *fwhm);
    void estimate_linear_parameters(fp *slope, fp *intercept, fp *avgy);
    fp find_fwhm (int pos, fp *area);
};


bool is_function_guessable(std::string const& formula,
                           bool check_defvalue=true);

bool is_function_guessable(std::vector<std::string> const& vars,
                           std::vector<std::string> const& defv,
                           Guess::Kind* kind);

bool is_defvalue_guessable(std::string defvalue, Guess::Kind kind);
Guess::Kind get_function_kind(std::string const& formula);

#endif

