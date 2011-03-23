// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__GUESS__H__
#define FITYK__GUESS__H__

#include <vector>
#include <string>
#include <boost/array.hpp>
#include "common.h" // realt

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

    /// Use data points with indexes from lb to rb-1,
    /// substract the current model from the data, (optionally) with exception
    /// of function that has index `ignore_idx'.
    /// This exception is used in "Guess %f = ..." if %f is already defined.
    void initialize(const DataAndModel* dm, int lb, int rb, int ignore_idx);

    /// returns values corresponding to linear_traits
    boost::array<double,3> estimate_linear_parameters();
    /// returns values corresponding to peak_traits
    boost::array<double,4> estimate_peak_parameters();

    void get_guess_info(std::string& result);

private:
    Settings const* settings_;
    std::vector<realt> xx_, yy_, sigma_;

    double find_hwhm(int pos, double *area);
};

#endif

