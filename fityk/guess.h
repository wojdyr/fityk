// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_GUESS_H_
#define FITYK_GUESS_H_

#include <vector>
#include <string>
#include "fityk.h" // realt, FITYK_API

namespace fityk {

class Data;
struct Settings;

/// guessing initial parameters of functions
class FITYK_API Guess
{
public:
    static const std::vector<std::string> linear_traits;
    static const std::vector<std::string> peak_traits;
    static const std::vector<std::string> sigmoid_traits;

    Guess(Settings const *settings) : settings_(settings) {}

    /// Use data points with indexes from lb to rb-1,
    /// substract the current model from the data, (optionally) with exception
    /// of function that has index `ignore_idx'.
    /// This exception is used in "Guess %f = ..." if %f is already defined.
    void set_data(const Data* data, const RealRange& range, int ignore_idx);

    /// returns values corresponding to linear_traits
    std::vector<double> estimate_linear_parameters() const;
    /// returns values corresponding to peak_traits
    std::vector<double> estimate_peak_parameters() const;
    /// returns values corresponding to sigmoid_traits
    std::vector<double> estimate_sigmoid_parameters() const;

private:
    Settings const* settings_;
    std::vector<realt> xx_, yy_, sigma_;

    double find_hwhm(int pos, double *area) const;
};

} // namespace fityk
#endif

