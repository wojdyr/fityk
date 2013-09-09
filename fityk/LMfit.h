// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// Simple implementation of the Levenberg-Marquardt method,
/// uses Jordan elimination with partial pivoting.

#ifndef FITYK_LMFIT_H_
#define FITYK_LMFIT_H_
#include <vector>
#include "fityk.h"
#include "fit.h"

namespace fityk {

class LMfit : public Fit
{
public:
    LMfit(Full* F, const char* name) : Fit(F, name) {}
    virtual double run_method(std::vector<realt>* best_a);

    // the same methods that were used for all methods up to ver. 1.2.1
    // (just for backward compatibility)
    virtual std::vector<double>
        get_covariance_matrix(const std::vector<Data*>& datas);
    virtual std::vector<double>
        get_standard_errors(const std::vector<Data*>& datas);

private:
    std::vector<realt> alpha_; // matrix
    std::vector<realt> beta_;  // and vector

    // working arrays in do_iteration()
    std::vector<realt> temp_alpha_, temp_beta_;

    void prepare_next_parameters(double lambda, const std::vector<realt> &a);
};

} // namespace fityk
#endif
