// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__GUESS__H__
#define FITYK__GUESS__H__

#include <vector>
#include <string>
#include <boost/array.hpp>

class DataAndModel;
class Tplate;
class Settings;


/// guessing initial parameters of functions
class Guess
{
public:
    static const boost::array<std::string, 3> linear_traits;
    static const boost::array<std::string, 4> peak_traits;

    Guess(Settings const *settings);
    //void initialize(std::vector<fp> const& xx, std::vector<fp> const& yy)
    //                                                 { xx_ = xx; yy_ = yy; }
    void initialize(const DataAndModel* dm, int lb, int rb, int ignore_idx);
    void guess(const Tplate* tp, std::vector<std::string>& par_names,
                                 std::vector<std::string>& par_values);
    void get_guess_info(std::string& result);

private:
    Settings const* settings_;
    std::vector<fp> xx_, yy_;

    void estimate_peak_parameters(fp *center, fp *height, fp *area, fp *hwhm);
    void estimate_linear_parameters(fp *slope, fp *intercept, fp *avgy);
    fp find_hwhm(int pos, fp *area);
};

#endif

