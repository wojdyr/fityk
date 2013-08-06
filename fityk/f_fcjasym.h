// Author: James Hester
// Licence: GNU General Public License ver. 2+

// peak shape in Finger, Cox and Jephcoat model, J. Appl. Cryst. (1994) 27, 892
// with some improvements to the original FCJ code's approach to derivative
// calculations, see:
// J. R. Hester, Improved asymmetric peak parameter refinement,
// J. Appl. Cryst. (2013) 46, 1219-1220
// http://dx.doi.org/10.1107/S0021889813016233
// and http://dx.doi.org/10.1107/S0021889813016233/ks5355sup4.txt

#ifndef FITYK_F_FCJASYM_H_
#define FITYK_F_FCJASYM_H_

#include "bfunc.h"

namespace fityk {

class FuncFCJAsymm : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(FCJAsymm, Function)
    void more_precomputations();
    bool get_nonzero_range(double level, realt &left, realt &right) const;
    bool get_center(realt* a) const { *a = av_[1]; return true; }
    bool get_height(realt* a) const { *a = av_[0]; return true; }
    bool get_fwhm(realt* a) const { *a = 2 * fabs(av_[2]); return true; }
    realt dfunc_int(realt angle1, realt angle2) const;
    realt fcj_psv(realt x, realt location, realt fwhm, realt mixing) const;
    static const double x100[];
    static const double w100[];
    static const double x1024[];
    static const double w1024[];
    realt twopsiinfl;
    realt twopsimin;
    realt cent_rad;
    realt radians;
    realt delta_n_neg[1024];      //same number of points as x1024 and w1024
    realt delta_n_pos[1024];
    realt weight_neg[1024];
    realt weight_pos[1024];
    realt denom;                 //denominator constant for given parameters
    realt denom_unscaled;        //denominator for x-axis in radians
    realt df_ds_factor;          //derivative with respect to denominator
    realt df_dh_factor;          //derivative with respect to denominator
};

} // namespace fityk
#endif
