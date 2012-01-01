// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// wrapper around MPFIT (cmpfit) library,
/// http://www.physics.wisc.edu/~craigm/idl/cmpfit.html
/// which is Levenberg-Marquardt implementation based on MINPACK-1

#ifndef FITYK_MPFIT_H_
#define FITYK_MPFIT_H_
#include "fit.h"
#include "cmpfit/mpfit.h"


struct mp_config_struct;

/// Wrapper around CMPFIT
class MPfit : public Fit
{
public:
    MPfit(Ftk* F);
    ~MPfit();
    virtual void init(); // called before do_iteration()/autoiter()
    void autoiter();

    // implementation (must be public to be called inside callback function)
    int calculate(int m, int npar, double *par, double *deviates,
                  double **derivs);
    int on_iteration();
private:
    mp_config_struct mp_conf_;
    mp_result result_;
    int start_iter_;
};

#endif

