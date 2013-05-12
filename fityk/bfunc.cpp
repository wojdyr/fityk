// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
/// Built-in function definitions

#define BUILDING_LIBFITYK
#include "bfunc.h"

#include <boost/math/special_functions/gamma.hpp>

#include "voigt.h"
#include "numfuncs.h"

using namespace std;
using boost::math::lgamma;

namespace fityk {

#include "gauss-legendre.h"

#define CALCULATE_VALUE_BEGIN(NAME) \
void NAME::calculate_value_in_range(vector<realt> const &xx, vector<realt> &yy,\
                                    int first, int last) const\
{\
    for (int i = first; i < last; ++i) {\
        realt x = xx[i];


#define CALCULATE_VALUE_END(VAL) \
        yy[i] += (VAL);\
    }\
}

#define CALCULATE_DERIV_BEGIN(NAME) \
void NAME::calculate_value_deriv_in_range(vector<realt> const &xx, \
                                          vector<realt> &yy, \
                                          vector<realt> &dy_da, \
                                          bool in_dx, \
                                          int first, int last) const \
{ \
    int dyn = dy_da.size() / xx.size(); \
    vector<realt> dy_dv(nv(), 0.); \
    for (int i = first; i < last; ++i) { \
        realt x = xx[i]; \
        realt dy_dx;


#define CALCULATE_DERIV_END(VAL) \
        if (!in_dx) { \
            yy[i] += (VAL); \
            v_foreach (Multi, j, multi_) \
                dy_da[dyn*i+j->p] += dy_dv[j->n] * j->mult;\
            dy_da[dyn*i+dyn-1] += dy_dx;\
        }\
        else {  \
            v_foreach (Multi, j, multi_) \
                dy_da[dyn*i+j->p] += dy_da[dyn*i+dyn-1] * dy_dv[j->n]*j->mult;\
        } \
    } \
}


///////////////////////////////////////////////////////////////////////

void FuncConstant::calculate_value_in_range(vector<realt> const&/*xx*/,
                                            vector<realt>& yy,
                                            int first, int last) const
{
    for (int i = first; i < last; ++i)
        yy[i] += av_[0];
}

CALCULATE_DERIV_BEGIN(FuncConstant)
    (void) x;
    dy_dv[0] = 1.;
    dy_dx = 0;
CALCULATE_DERIV_END(av_[0])

///////////////////////////////////////////////////////////////////////

CALCULATE_VALUE_BEGIN(FuncLinear)
CALCULATE_VALUE_END(av_[0] + x*av_[1])

CALCULATE_DERIV_BEGIN(FuncLinear)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dx = av_[1];
CALCULATE_DERIV_END(av_[0] + x*av_[1])

///////////////////////////////////////////////////////////////////////

CALCULATE_VALUE_BEGIN(FuncQuadratic)
CALCULATE_VALUE_END(av_[0] + x*av_[1] + x*x*av_[2])

CALCULATE_DERIV_BEGIN(FuncQuadratic)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dx = av_[1] + 2*x*av_[2];
CALCULATE_DERIV_END(av_[0] + x*av_[1] + x*x*av_[2])

///////////////////////////////////////////////////////////////////////

CALCULATE_VALUE_BEGIN(FuncCubic)
CALCULATE_VALUE_END(av_[0] + x*av_[1] + x*x*av_[2] + x*x*x*av_[3])

CALCULATE_DERIV_BEGIN(FuncCubic)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dx = av_[1] + 2*x*av_[2] + 3*x*x*av_[3];
CALCULATE_DERIV_END(av_[0] + x*av_[1] + x*x*av_[2] + x*x*x*av_[3])

///////////////////////////////////////////////////////////////////////

CALCULATE_VALUE_BEGIN(FuncPolynomial4)
CALCULATE_VALUE_END(av_[0] + x*av_[1] + x*x*av_[2] + x*x*x*av_[3]
                                          + x*x*x*x*av_[4])

CALCULATE_DERIV_BEGIN(FuncPolynomial4)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dx = av_[1] + 2*x*av_[2] + 3*x*x*av_[3] + 4*x*x*x*av_[4];
CALCULATE_DERIV_END(av_[0] + x*av_[1] + x*x*av_[2] + x*x*x*av_[3]
                                      + x*x*x*x*av_[4])

///////////////////////////////////////////////////////////////////////

CALCULATE_VALUE_BEGIN(FuncPolynomial5)
CALCULATE_VALUE_END(av_[0] + x*av_[1] + x*x*av_[2]
                       + x*x*x*av_[3] + x*x*x*x*av_[4] + x*x*x*x*x*av_[5])

CALCULATE_DERIV_BEGIN(FuncPolynomial5)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dv[5] = x*x*x*x*x;
    dy_dx = av_[1] + 2*x*av_[2] + 3*x*x*av_[3] + 4*x*x*x*av_[4]
               + 5*x*x*x*x*av_[5];
CALCULATE_DERIV_END(av_[0] + x*av_[1] + x*x*av_[2]
                          + x*x*x*av_[3] + x*x*x*x*av_[4] + x*x*x*x*x*av_[5])

///////////////////////////////////////////////////////////////////////

CALCULATE_VALUE_BEGIN(FuncPolynomial6)
CALCULATE_VALUE_END(av_[0] + x*av_[1] + x*x*av_[2] + x*x*x*av_[3] +
                        x*x*x*x*av_[4] + x*x*x*x*x*av_[5] + x*x*x*x*x*x*av_[6])

CALCULATE_DERIV_BEGIN(FuncPolynomial6)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dv[5] = x*x*x*x*x;
    dy_dv[6] = x*x*x*x*x*x;
    dy_dx = av_[1] + 2*x*av_[2] + 3*x*x*av_[3] + 4*x*x*x*av_[4]
                + 5*x*x*x*x*av_[5] + 6*x*x*x*x*x*av_[6];
CALCULATE_DERIV_END(av_[0] + x*av_[1] + x*x*av_[2] + x*x*x*av_[3] +
                        x*x*x*x*av_[4] + x*x*x*x*x*av_[5] + x*x*x*x*x*x*av_[6])

///////////////////////////////////////////////////////////////////////

void FuncGaussian::more_precomputations()
{
    if (fabs(av_[2]) < epsilon)
        av_[2] = epsilon;
}

CALCULATE_VALUE_BEGIN(FuncGaussian)
    realt xa1a2 = (x - av_[1]) / av_[2];
    realt ex = exp(- M_LN2 * xa1a2 * xa1a2);
CALCULATE_VALUE_END(av_[0] * ex)

CALCULATE_DERIV_BEGIN(FuncGaussian)
    realt xa1a2 = (x - av_[1]) / av_[2];
    realt ex = exp(- M_LN2 * xa1a2 * xa1a2);
    dy_dv[0] = ex;
    realt dcenter = 2 * M_LN2 * av_[0] * ex * xa1a2 / av_[2];
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dx = -dcenter;
CALCULATE_DERIV_END(av_[0]*ex)

bool FuncGaussian::get_nonzero_range(double level,
                                     realt &left, realt &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(av_[0]))
        left = right = 0;
    else {
        realt w = sqrt(log(fabs(av_[0]/level)) / M_LN2) * av_[2];
        left = av_[1] - w;
        right = av_[1] + w;
    }
    return true;
}

bool FuncGaussian::get_area(realt* a) const
{
    *a = av_[0] * fabs(av_[2]) * sqrt(M_PI / M_LN2);
    return true;
}

///////////////////////////////////////////////////////////////////////

void FuncSplitGaussian::more_precomputations()
{
    if (fabs(av_[2]) < epsilon)
        av_[2] = epsilon;
    if (fabs(av_[3]) < epsilon)
        av_[3] = epsilon;
}

CALCULATE_VALUE_BEGIN(FuncSplitGaussian)
    realt hwhm = (x < av_[1] ? av_[2] : av_[3]);
    realt xa1a2 = (x - av_[1]) / hwhm;
    realt ex = exp(- M_LN2 * xa1a2 * xa1a2);
CALCULATE_VALUE_END(av_[0] * ex)

CALCULATE_DERIV_BEGIN(FuncSplitGaussian)
    realt hwhm = (x < av_[1] ? av_[2] : av_[3]);
    realt xa1a2 = (x - av_[1]) / hwhm;
    realt ex = exp(- M_LN2 * xa1a2 * xa1a2);
    dy_dv[0] = ex;
    realt dcenter = 2 * M_LN2 * av_[0] * ex * xa1a2 / hwhm;
    dy_dv[1] = dcenter;
    if (x < av_[1]) {
        dy_dv[2] = dcenter * xa1a2;
        dy_dv[3] = 0;
    }
    else {
        dy_dv[2] = 0;
        dy_dv[3] = dcenter * xa1a2;
    }
    dy_dx = -dcenter;
CALCULATE_DERIV_END(av_[0]*ex)

bool FuncSplitGaussian::get_nonzero_range(double level,
                                          realt &left, realt &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(av_[0]))
        left = right = 0;
    else {
        realt w1 = sqrt (log (fabs(av_[0]/level)) / M_LN2) * av_[2];
        realt w2 = sqrt (log (fabs(av_[0]/level)) / M_LN2) * av_[3];
        left = av_[1] - w1;
        right = av_[1] + w2;
    }
    return true;
}

bool FuncSplitGaussian::get_area(realt* a) const
{
    *a = av_[0] * (fabs(av_[2])+fabs(av_[3])) / 2. * sqrt(M_PI/M_LN2);
    return true;
}

///////////////////////////////////////////////////////////////////////

void FuncLorentzian::more_precomputations()
{
    if (fabs(av_[2]) < epsilon)
        av_[2] = epsilon;
}

CALCULATE_VALUE_BEGIN(FuncLorentzian)
    realt xa1a2 = (x - av_[1]) / av_[2];
    realt inv_denomin = 1. / (1 + xa1a2 * xa1a2);
CALCULATE_VALUE_END(av_[0] * inv_denomin)

CALCULATE_DERIV_BEGIN(FuncLorentzian)
    realt xa1a2 = (x - av_[1]) / av_[2];
    realt inv_denomin = 1. / (1 + xa1a2 * xa1a2);
    dy_dv[0] = inv_denomin;
    realt dcenter = 2 * av_[0] * xa1a2 / av_[2] * inv_denomin * inv_denomin;
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dx = -dcenter;
CALCULATE_DERIV_END(av_[0] * inv_denomin)

bool FuncLorentzian::get_nonzero_range(double level,
                                       realt &left, realt &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(av_[0]))
        left = right = 0;
    else {
        realt w = sqrt(fabs(av_[0]/level) - 1) * av_[2];
        left = av_[1] - w;
        right = av_[1] + w;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

void FuncPearson7::more_precomputations()
{
    if (fabs(av_[2]) < epsilon)
        av_[2] = epsilon;
    if (av_.size() != 5)
        av_.resize(5);
    // not checking for av_[3]>0.5 nor even >0
    av_[4] = pow(2, 1. / av_[3]) - 1;
}

CALCULATE_VALUE_BEGIN(FuncPearson7)
    realt xa1a2 = (x - av_[1]) / av_[2];
    realt xa1a2sq = xa1a2 * xa1a2;
    realt pow_2_1_a3_1 = av_[4]; //pow (2, 1. / a3) - 1;
    realt denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    realt inv_denomin = pow(denom_base, - av_[3]);
CALCULATE_VALUE_END(av_[0] * inv_denomin)

CALCULATE_DERIV_BEGIN(FuncPearson7)
    realt xa1a2 = (x - av_[1]) / av_[2];
    realt xa1a2sq = xa1a2 * xa1a2;
    realt pow_2_1_a3_1 = av_[4]; //pow (2, 1. / a3) - 1;
    realt denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    realt inv_denomin = pow(denom_base, - av_[3]);
    dy_dv[0] = inv_denomin;
    realt dcenter = 2 * av_[0] * av_[3] * pow_2_1_a3_1 * xa1a2 * inv_denomin /
                                                      (denom_base * av_[2]);
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dv[3] = av_[0] * inv_denomin * (M_LN2 * (pow_2_1_a3_1 + 1)
                       * xa1a2sq / (denom_base * av_[3]) - log(denom_base));
    dy_dx = -dcenter;
CALCULATE_DERIV_END(av_[0] * inv_denomin)


bool FuncPearson7::get_nonzero_range(double level,
                                     realt &left, realt &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(av_[0]))
        left = right = 0;
    else {
        realt t = (pow(fabs(av_[0]/level), 1./av_[3]) - 1)
                  / (pow(2, 1./av_[3]) - 1);
        realt w = sqrt(t) * av_[2];
        left = av_[1] - w;
        right = av_[1] + w;
    }
    return true;
}

bool FuncPearson7::get_area(realt* a) const
{
    if (av_[3] <= 0.5)
        return false;
    realt g = exp(lgamma(av_[3] - 0.5) - lgamma(av_[3]));
    //in f_val_precomputations(): av_[4] = pow (2, 1. / a3) - 1;
    *a = av_[0] * 2 * fabs(av_[2]) * sqrt(M_PI) * g / (2 * sqrt(av_[4]));
    return true;
}

///////////////////////////////////////////////////////////////////////

void FuncSplitPearson7::more_precomputations()
{
    if (fabs(av_[2]) < epsilon)
        av_[2] = epsilon;
    if (fabs(av_[3]) < epsilon)
        av_[3] = epsilon;
    if (av_.size() != 8)
        av_.resize(8);
    // not checking for av_[3]>0.5 nor even >0
    av_[6] = pow(2, 1. / av_[4]) - 1;
    av_[7] = pow(2, 1. / av_[5]) - 1;
}

CALCULATE_VALUE_BEGIN(FuncSplitPearson7)
    int lr = x < av_[1] ? 0 : 1;
    realt xa1a2 = (x - av_[1]) / av_[2+lr];
    realt xa1a2sq = xa1a2 * xa1a2;
    realt pow_2_1_a3_1 = av_[6+lr]; //pow(2, 1./shape) - 1;
    realt denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    realt inv_denomin = pow(denom_base, - av_[4+lr]);
CALCULATE_VALUE_END(av_[0] * inv_denomin)

CALCULATE_DERIV_BEGIN(FuncSplitPearson7)
    int lr = x < av_[1] ? 0 : 1;
    realt hwhm = av_[2+lr];
    realt shape = av_[4+lr];
    realt xa1a2 = (x - av_[1]) / hwhm;
    realt xa1a2sq = xa1a2 * xa1a2;
    realt pow_2_1_a3_1 = av_[6+lr]; //pow(2, 1./shape) - 1;
    realt denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    realt inv_denomin = pow (denom_base, -shape);
    dy_dv[0] = inv_denomin;
    realt dcenter = 2 * av_[0] * shape * pow_2_1_a3_1 * xa1a2 * inv_denomin /
                                                      (denom_base * hwhm);
    dy_dv[1] = dcenter;
    dy_dv[2] = dy_dv[3] = dy_dv[4] = dy_dv[5] = 0;
    dy_dv[2+lr] = dcenter * xa1a2;
    dy_dv[4+lr] = av_[0] * inv_denomin * (M_LN2 * (pow_2_1_a3_1 + 1)
                           * xa1a2sq / (denom_base * shape) - log(denom_base));
    dy_dx = -dcenter;
CALCULATE_DERIV_END(av_[0] * inv_denomin)


bool FuncSplitPearson7::get_nonzero_range(double level,
                                          realt &left, realt &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(av_[0]))
        left = right = 0;
    else {
        realt t1 = (pow(fabs(av_[0]/level), 1./av_[4]) - 1)
                                                / (pow(2, 1./av_[4]) - 1);
        realt w1 = sqrt(t1) * av_[2];
        realt t2 = (pow(fabs(av_[0]/level), 1./av_[5]) - 1)
                                                / (pow(2, 1./av_[5]) - 1);
        realt w2 = sqrt(t2) * av_[3];
        left = av_[1] - w1;
        right = av_[1] + w2;
    }
    return true;
}

bool FuncSplitPearson7::get_area(realt* a) const
{
    if (av_[4] <= 0.5 || av_[5] <= 0.5)
        return false;
    realt g1 = exp(lgamma(av_[4] - 0.5) - lgamma(av_[4]));
    realt g2 = exp(lgamma(av_[5] - 0.5) - lgamma(av_[5]));
    *a =   av_[0] * fabs(av_[2]) * sqrt(M_PI) * g1 / (2 * sqrt(av_[6]))
         + av_[0] * fabs(av_[3]) * sqrt(M_PI) * g2 / (2 * sqrt(av_[7]));
    return true;
}

///////////////////////////////////////////////////////////////////////

void FuncPseudoVoigt::more_precomputations()
{
    if (fabs(av_[2]) < epsilon)
        av_[2] = epsilon;
}

CALCULATE_VALUE_BEGIN(FuncPseudoVoigt)
    realt xa1a2 = (x - av_[1]) / av_[2];
    realt ex = exp(- M_LN2 * xa1a2 * xa1a2);
    realt lor = 1. / (1 + xa1a2 * xa1a2);
    realt without_height =  (1-av_[3]) * ex + av_[3] * lor;
CALCULATE_VALUE_END(av_[0] * without_height)

CALCULATE_DERIV_BEGIN(FuncPseudoVoigt)
    realt xa1a2 = (x - av_[1]) / av_[2];
    realt ex = exp(- M_LN2 * xa1a2 * xa1a2);
    realt lor = 1. / (1 + xa1a2 * xa1a2);
    realt without_height =  (1-av_[3]) * ex + av_[3] * lor;
    dy_dv[0] = without_height;
    realt dcenter = 2 * av_[0] * xa1a2 / av_[2]
                        * (av_[3]*lor*lor + (1-av_[3])*M_LN2*ex);
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dv[3] =  av_[0] * (lor - ex);
    dy_dx = -dcenter;
CALCULATE_DERIV_END(av_[0] * without_height)

bool FuncPseudoVoigt::get_nonzero_range(double level,
                                        realt &left, realt &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(av_[0]))
        left = right = 0;
    else {
        // neglecting Gaussian part and adding 4.0 to compensate it
        realt w = (sqrt (av_[3] * fabs(av_[0]/level) - 1) + 4.) * av_[2];
        left = av_[1] - w;
        right = av_[1] + w;
    }
    return true;
}

bool FuncPseudoVoigt::get_area(realt* a) const
{
    *a = av_[0] * fabs(av_[2])
              * ((av_[3] * M_PI) + (1 - av_[3]) * sqrt(M_PI / M_LN2));
    return true;
}

///////////////////////////////////////////////////////////////////////
/* The FCJAsymm peakshape is that described in Finger, Cox and Jephcoat (1994)
   J. Appl. Cryst. vol 27, pp 892-900. */

realt FuncFCJAsymm::dfunc_int(realt twopsi, realt twotheta) const {
/* The integral of the FCJ weight function:
0.5(asin((2 cos (twotheta)^2 + 2 sin(twopsi)-2)/|2 sin twopsi - 2|sin(twotheta)) -
    asin((2 cos (twotheta)^2 - 2 sin(twopsi)-2)/|2 sin twopsi + 2|sin(twotheta)) )
Twotheta and twopsi are in radians.  We note that the limit as twopsi -> twotheta
is pi/2.

Note that callers will need to apply the 1/(2_hl) factor found in the FCJ paper.
*/
realt stwopsi = sin(twopsi);
realt stwoth  = sin(twotheta);
realt ctwoth = cos(twotheta);
if(twopsi == 0) return 0.0;
if(twopsi == twotheta) return M_PI/2.0;
return 0.5 * (asin((2.0*ctwoth*ctwoth + 2*stwopsi -2)/(abs(2*stwopsi-2)*stwoth)) -
       asin((2.0*ctwoth*ctwoth - 2*stwopsi -2)/(abs(2*stwopsi+2)*stwoth)));
}

void FuncFCJAsymm::more_precomputations()
{
denom=0.0;
radians = M_PI/180.0;
cent_rad = av_[1]*radians;
realt hfunc_neg, hfunc_pos;
// If either of the below give a cosine greater than one, set to 0
// Handle extrema by setting twopsimin to appropriate limit
 twopsimin = 0.0;
 if (cent_rad > M_PI/2) twopsimin = M_PI;
realt cospsimin = cos(cent_rad)*sqrt(pow(av_[4]+av_[5],2) + 1.0);
 if(fabs(cospsimin)<1.0) twopsimin = acos(cospsimin);
twopsiinfl = 0.0;
realt cospsiinfl = cos(cent_rad)*sqrt(pow(av_[4]-av_[5],2) + 1.0);
 if(fabs(cospsiinfl)<1.0) twopsiinfl = acos(cospsiinfl);
 if(av_[4] == 0 && av_[5] == 0) denom = 1.0;
 else {
/* The denominator for the FCJ expression can be calculated analytically. We define it in terms
of the integral of the weight function, dfunc_int:
 denom = 2* min(h_l,s_l) * (pi/2 - dfunc_int (twopsiinfl,twotheta)) +
         h_l * (v-u) + s_l * (v-u) - (extra_int (twopsiinfl,twotheta) - extra_int (twopsimin,twotheta))

where v = 1/(2h_l) * dfunc_int(twopsiinfl,twotheta) and u = 1/2h_l * dfunc_int(twopsimin,twotheta).
extra_int is the integral of 1/cos(psi).
*/
realt u = 0.5*dfunc_int(twopsimin,cent_rad)/av_[4];
realt v = 0.5*dfunc_int(twopsiinfl,cent_rad)/av_[4];
denom_unscaled = 2.0 * fmin(av_[5],av_[4]) * (M_PI/(4.0*av_[4]) - v) + (av_[4] + av_[5])* (v - u) -
   (1.0/(2*av_[4]))*0.5*(log(fabs(sin(twopsiinfl) + 1)) - log(fabs(sin(twopsiinfl)-1)) -
		     log(fabs(sin(twopsimin) + 1)) + log(fabs(sin(twopsimin)-1)));
denom = denom_unscaled * 2.0/fabs(cent_rad-twopsimin);     //Scale to [-1,1] interval of G-L integration
// The following two factors are the analytic derivatives of the integral of D with respect to
// h_l and s_l.
df_dh_factor = (1.0/(2*av_[4]))*(dfunc_int(twopsiinfl,cent_rad) - dfunc_int(twopsimin,cent_rad)) -
					     (1.0/av_[4])*denom_unscaled;
df_ds_factor = (1.0/(2*av_[4]))*(dfunc_int(twopsiinfl,cent_rad) - dfunc_int(twopsimin,cent_rad)) +
					    (1.0/av_[4])*(M_PI/2 - dfunc_int(twopsiinfl,cent_rad));
// Precalculate the x and hfunc coefficients
for(int pt=0;pt<512;pt++) {
    delta_n_neg[pt] = (cent_rad + twopsimin)/2.0 - (cent_rad-twopsimin)*x1024[pt]/2;
    delta_n_pos[pt] = (cent_rad + twopsimin)/2.0 + (cent_rad-twopsimin)*x1024[pt]/2;
    hfunc_neg = sqrt(pow(cos(delta_n_neg[pt]),2)/pow(cos(cent_rad),2) -1);
    hfunc_pos = sqrt(pow(cos(delta_n_pos[pt]),2)/pow(cos(cent_rad),2)-1);
    //Weights for negative half of G-L interval
    if(fabs(cos(delta_n_neg[pt])) > fabs(cos(twopsiinfl))) {
            weight_neg[pt] = av_[4] + av_[5] - hfunc_neg;
        } else {
            weight_neg[pt] = 2 * min(av_[4],av_[5]);
        }
    //Weights for positive half of G-L interval
    weight_neg[pt] = weight_neg[pt]/(2.0*av_[4]*hfunc_neg*abs(cos(delta_n_neg[pt])));
    if(fabs(cos(delta_n_pos[pt])) > fabs(cos(twopsiinfl))) {
        weight_pos[pt] = av_[4] + av_[5] - hfunc_pos;
        } else {
        weight_pos[pt] = 2 * min(av_[4],av_[5]);
        }
    weight_pos[pt] = weight_pos[pt]/(2.0*av_[4]*hfunc_pos*abs(cos(delta_n_pos[pt])));
    // Apply Gauss-Legendre weights
    weight_pos[pt]*=w1024[pt];
    weight_neg[pt]*=w1024[pt];
    }
 }
}

bool FuncFCJAsymm::get_nonzero_range(double level,
                                        realt &left, realt &right) const
{
if (level == 0)
        return false;
    else if (fabs(level) >= fabs(av_[0]))
        left = right = 0;
    else {
      // As for Pseudo-Voight, allow 4 half-widths for Gaussian
      realt pvw = av_[2]*(sqrt(fabs(av_[0]/(level*M_PI*av_[2])-1))+4.); //halfwidths for Lorenzian
      // The first non-zero point occurs when the convoluting PV reaches twopsimin. The
      // last value occurs when the convoluting PV moves completely past the centre
      if(av_[1] < 90) {
        left = twopsimin*180/M_PI - pvw;
        right = av_[1] + pvw;
      } else {
        left = av_[1] - pvw;
        right = twopsimin*180/M_PI + pvw;
      }
    }
    return true;
}

//Pseudo-Voight with scaling factors as used in crystallography.
realt FuncFCJAsymm::fcj_psv(realt x, realt location, realt fwhm, realt mixing) const {
    realt xa1a2 = (location - x) / fwhm;
    realt ex = exp(- 4.0 * M_LN2 * xa1a2 * xa1a2);
    ex *= sqrt(4.0*M_LN2/M_PI)/fwhm;
    realt lor = 2. / (M_PI * fwhm * (1 + 4* xa1a2 * xa1a2));
    return  (1-mixing) * ex + mixing * lor;
}

CALCULATE_VALUE_BEGIN(FuncFCJAsymm)
    realt numer = 0.0;
    realt fwhm_rad = av_[2]*2*M_PI/180.0;  // Fityk uses hwhm, we use fwhm
if((av_[4]==0 && av_[5]==0) || cent_rad==M_PI/2) {     // Plain PseudoVoight
  numer = fcj_psv(x*radians,cent_rad,fwhm_rad, av_[3]);
 } else {
    //do the sum over 1024 Gauss-Legendre points
    for(int pt=0; pt < 512; pt++) {
   /* Note that the Pseudo-Voight equation for this calculation is that used
      in powder diffraction, where the coefficients are chosen to give matching
      width parameters, i.e. only one width parameter is necessary and the
      width is expressed in degrees (which means a normalised height in
      degrees */
   // negative and positive sides
      realt psvval = 0.0;
    psvval = fcj_psv(delta_n_neg[pt],x*radians,fwhm_rad,av_[3]);
    numer += weight_neg[pt] * psvval;
    psvval = fcj_psv(delta_n_pos[pt],x*radians,fwhm_rad,av_[3]);
    numer += weight_pos[pt] * psvval;
    }
 }
    //Radians scale below to make up for fwhm_rad in denominator of PV function
CALCULATE_VALUE_END(av_[0]*M_PI/180 * numer/denom)

CALCULATE_DERIV_BEGIN(FuncFCJAsymm)
    realt fwhm_rad = av_[2]*2*M_PI/180.0;
    realt numer = 0.0;
realt hfunc_neg,hfunc_pos;  //FCJ hfunc for neg, pos regions of G-L integration
    realt sumWdGdh = 0.0;  // derivative with respect to H/L
    realt sumWdGds = 0.0;  // derivative with respect to S/L
    realt sumWdRdG = 0.0; // sum w_i * dR/dgamma * W(delta,twotheta)/H
    realt sumWdRde = 0.0 ;// as above with dR/deta
    realt sumWdRdt = 0.0; // as above with dR/d2theta

    //do the sum over 1024 points
    // parameters are height,centre,hwhm,eta (mixing),H/L,S/L
    //                  0      1     2      3          4   5
    for(int pt=0; pt < 512; pt++) {
      for(int side=0;side < 2; side++) {
    realt xa1a2 = 0.0;
    if(side == 0) xa1a2 = (x*radians - delta_n_neg[pt]) / fwhm_rad;
    else          xa1a2 = (x*radians - delta_n_pos[pt]) / fwhm_rad ;
    realt facta = -4.0 * M_LN2 * xa1a2 ;
    realt ex = exp(facta * xa1a2);
    ex *= sqrt(4.0*M_LN2/M_PI)/fwhm_rad;
    realt lor = 2. / (M_PI * fwhm_rad *(1 + 4* xa1a2 * xa1a2));
    realt without_height =  (1-av_[3]) * ex + av_[3] * lor;
    realt psvval = av_[0] * without_height;
    if(side == 0) {
        numer += weight_neg[pt] * psvval;
        hfunc_neg = 1/(2.0*av_[4]*sqrt(pow(cos(delta_n_neg[pt]),2)/pow(cos(cent_rad),2)-1));
    } else
    if(side == 1) {           //So hfunc nonzero
        numer += weight_pos[pt] * psvval;
        hfunc_pos = 1.0/(2.0*av_[4]*sqrt(pow(cos(delta_n_pos[pt]),2)/pow(cos(cent_rad),2)-1));
    }
    // pseudo-voight derivatives: first fwhm.  The parameter is expressed in
    // degrees, but our calculations use radians,so we remember to scale at the end.
    realt dRdg = ex/fwhm_rad * (-1.0 + -2.0*facta*xa1a2);  //gaussian part:checked
    dRdg = av_[0] * ((1-av_[3])* dRdg + av_[3]*(-1.0*lor/fwhm_rad +
						16*xa1a2*xa1a2/(M_PI*(fwhm_rad*fwhm_rad))*1.0/pow(1.0+4*xa1a2*xa1a2,2)));
    realt dRde =  av_[0] * (lor - ex);           //with respect to mixing
    realt dRdtt = -1.0* av_[0] * ((1.0-av_[3])*2.0*ex*facta/fwhm_rad - av_[3]*lor*8*xa1a2/(fwhm_rad*(1+4*xa1a2*xa1a2)));
    if(side==0) {
    /* We know that d(FCJ)/d(param) = sum w[i]*dR/d(param) * W/H(delta) */
    sumWdRdG += weight_neg[pt] * dRdg;
    sumWdRde += weight_neg[pt] * dRde;
    sumWdRdt += weight_neg[pt] * dRdtt;
    } else {
      sumWdRdG += weight_pos[pt] * dRdg;
      sumWdRde += weight_pos[pt] * dRde;
      sumWdRdt += weight_pos[pt] * dRdtt;
    }
    /* The derivative for h_l includes the convolution of dfunc with PV only up
       to twopsiinfl when s_l < h_l  as it is zero above this limit.  To save
       program bytes, we keep the same G-L interval and therefore quadrature
       points.  This is defensible as it is equivalent to including the zero
       valued points in the integration.
       The derivative for peak "centre" given here ignores the contribution
       from changes in the weighting function, as it is not possible to
       numerically integrate the derivative of dfunc that appears in the
       full expression.  It seems to work.
       W is 2 min(s_l,h_l) above twopsiinfl.  If W = 2h_l, the 2h_l factor
       cancels at top and bottom, leaving a derivative of zero with respect
       to both s_l and h_l.  If W=2s_l, the derivative with respect to
       s_l only is non-zero.*/
    if(fabs(cos(delta_n_pos[pt])) > fabs(cos(twopsiinfl)) && side == 1) {
      realt dconvol =  w1024[pt] * psvval * hfunc_pos / abs(cos(delta_n_pos[pt]));
    sumWdGdh += dconvol;
    sumWdGds += dconvol;
    }
    else if(side == 1 && av_[5] < av_[4]) {
      sumWdGds += 2.0* w1024[pt] * psvval *hfunc_pos / abs(cos(delta_n_pos[pt]));
    }
    if(fabs(cos(delta_n_neg[pt])) > fabs(cos(twopsiinfl)) && side == 0) {
      realt dconvol =  w1024[pt] * psvval * hfunc_neg / abs(cos(delta_n_neg[pt]));
    sumWdGdh += dconvol;
    sumWdGds += dconvol;
    }
    else if (side == 0 && av_[5] < av_[4]) {
      sumWdGds += 2.0* w1024[pt] * psvval * hfunc_neg / abs(cos(delta_n_neg[pt]));
    }
    }
    }
    // Note that we must scale any numerically integrated terms back to the correct interval
    // This scale factor cancels for numer/denom, but not for e.g. numer/denom^2
dy_dv[0] = M_PI/180 * numer/(av_[0]*denom);        // height derivative(note numer contains height)
dy_dv[1] = M_PI*M_PI/(180*180) * sumWdRdt/denom;                // peak position in degrees
dy_dv[2] = 2*M_PI*M_PI/(180*180) * sumWdRdG/denom;     // fwhm/2 is hwhm, in degrees
dy_dv[3] = M_PI/180 * sumWdRde/denom;     // mixing
dy_dv[4] = M_PI/180 * (sumWdGdh/denom - 1.0/av_[4] * numer/denom -
               df_dh_factor*numer/(denom_unscaled*denom));                // h_l
dy_dv[5] = M_PI/180* (sumWdGds/denom - df_ds_factor * numer/(denom*denom_unscaled)); // s_l
dy_dx = -1.0*dy_dv[1];
CALCULATE_DERIV_END(M_PI/180 * numer/denom)

///////////////////////////////////////////////////////////////////////

void FuncVoigt::more_precomputations()
{
    if (av_.size() != 6)
        av_.resize(6);
    float k, l, dkdx, dkdy;
    humdev(0, fabs(av_[3]), k, l, dkdx, dkdy);
    av_[4] = 1. / k;
    av_[5] = dkdy / k;

    if (fabs(av_[2]) < epsilon)
        av_[2] = epsilon;
}

CALCULATE_VALUE_BEGIN(FuncVoigt)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    float k;
    realt xa1a2 = (x - av_[1]) / av_[2];
    k = humlik(xa1a2, fabs(av_[3]));
CALCULATE_VALUE_END(av_[0] * av_[4] * k)

CALCULATE_DERIV_BEGIN(FuncVoigt)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    // here fabs(av_[3]) is used, and dy_dv[3] is negated if av_[3]<0.
    float k;
    realt xa1a2 = (x-av_[1]) / av_[2];
    realt a0a4 = av_[0] * av_[4];
    float l, dkdx, dkdy;
    humdev(xa1a2, fabs(av_[3]), k, l, dkdx, dkdy);
    dy_dv[0] = av_[4] * k;
    realt dcenter = -a0a4 * dkdx / av_[2];
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dv[3] = a0a4 * (dkdy - k * av_[5]);
    if (av_[3] < 0)
        dy_dv[3] = -dy_dv[3];
    dy_dx = -dcenter;
CALCULATE_DERIV_END(a0a4 * k)

bool FuncVoigt::get_nonzero_range(double level,
                                  realt &left, realt &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(av_[0]))
        left = right = 0;
    else {
        //TODO estimate Voigt's non-zero range
        return false;
    }
    return true;
}

///estimation according to
/// http://en.wikipedia.org/w/index.php?title=Voigt_profile&oldid=115518205
///
/// a2 = sqrt(2) * sigma
/// a3 = gamma / (sqrt(2) * sigma)
///
/// sigma = a2 / sqrt(2)
/// gamma = a2 * a3
static realt voigt_fwhm(realt a2, realt a3)
{
    realt sigma = fabs(a2) / M_SQRT2;
    realt gamma = fabs(a2) * a3;

    realt fG = 2 * sigma * sqrt(2 * M_LN2);
    realt fL = 2 * gamma;

    realt fV = 0.5346 * fL + sqrt(0.2166 * fL * fL + fG * fG);
    return fV;
}

bool FuncVoigt::get_fwhm(realt* a) const
{
    *a = voigt_fwhm(av_[2], av_[3]);
    return true;
}

bool FuncVoigt::get_area(realt* a) const
{
    *a = av_[0] * fabs(av_[2] * sqrt(M_PI) * av_[4]);
    return true;
}

const vector<string>& FuncVoigt::get_other_prop_names() const
{
    static const vector<string> p = vector2(string("GaussianFWHM"),
                                            string("LorentzianFWHM"));
    return p;
}

realt FuncVoigt::get_other_prop(string const& name) const
{
    if (name == "GaussianFWHM") {
        realt sigma = fabs(av_[2]) / M_SQRT2;
        return 2 * sigma * sqrt(2 * M_LN2);
    }
    else if (name == "LorentzianFWHM") {
        realt gamma = fabs(av_[2]) * av_[3];
        return 2 * gamma;
    }
    else
        return 0.;
}

///////////////////////////////////////////////////////////////////////

void FuncVoigtA::more_precomputations()
{
    if (av_.size() != 6)
        av_.resize(6);
    av_[4] = 1. / humlik(0, fabs(av_[3]));

    if (fabs(av_[2]) < epsilon)
        av_[2] = epsilon;
}

CALCULATE_VALUE_BEGIN(FuncVoigtA)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    float k;
    realt xa1a2 = (x - av_[1]) / av_[2];
    k = humlik(xa1a2, fabs(av_[3]));
CALCULATE_VALUE_END(av_[0] / (sqrt(M_PI) * av_[2]) * k)

CALCULATE_DERIV_BEGIN(FuncVoigtA)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    // here fabs(av_[3]) is used, and dy_dv[3] is negated if av_[3]<0.
    float k;
    realt xa1a2 = (x-av_[1]) / av_[2];
    realt f = av_[0] / (sqrt(M_PI) * av_[2]);
    float l, dkdx, dkdy;
    humdev(xa1a2, fabs(av_[3]), k, l, dkdx, dkdy);
    dy_dv[0] = k / (sqrt(M_PI) * av_[2]);
    realt dcenter = -f * dkdx / av_[2];
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2 - f * k / av_[2];
    dy_dv[3] = f * dkdy;
    if (av_[3] < 0)
        dy_dv[3] = -dy_dv[3];
    dy_dx = -dcenter;
CALCULATE_DERIV_END(f * k)

bool FuncVoigtA::get_nonzero_range(double level,
                                   realt &left, realt &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(av_[0]))
        left = right = 0;
    else {
        //TODO estimate Voigt's non-zero range
        return false;
    }
    return true;
}

bool FuncVoigtA::get_fwhm(realt* a) const
{
    *a = voigt_fwhm(av_[2], av_[3]);
    return true;
}

bool FuncVoigtA::get_height(realt* a) const
{
    *a = av_[0] / fabs(av_[2] * sqrt(M_PI) * av_[4]);
    return true;
}


///////////////////////////////////////////////////////////////////////

void FuncEMG::more_precomputations()
{
}

bool FuncEMG::get_nonzero_range(double/*level*/,
                                realt&/*left*/, realt&/*right*/) const
{
    return false;
}

// approximation to erfc(x) * exp(x*x) for |x| > 4;
// if x < -26.something exp(x*x) in the last line returns inf;
// based on "Rational Chebyshev approximations for the error function"
// by W.J. Cody, Math. Comp., 1969, 631-638.
static double erfcexp_x4(double x)
{
    double ax = fabs(x);
    assert(ax >= 4.);
    const double p[4] = { 3.05326634961232344e-1, 3.60344899949804439e-1,
                          1.25781726111229246e-1, 1.60837851487422766e-2 };
    const double q[4] = { 2.56852019228982242,    1.87295284992346047,
                          5.27905102951428412e-1, 6.05183413124413191e-2 };
    double rsq = 1 / (ax * ax);
    double xnum = 1.63153871373020978e-2 * rsq;
    double xden = rsq;
    for (int i = 0; i < 4; ++i) {
       xnum = (xnum + p[i]) * rsq;
       xden = (xden + q[i]) * rsq;
    }
    double t = rsq * (xnum + 6.58749161529837803e-4)
                   / (xden + 2.33520497626869185e-3);
    double v = (1/sqrt(M_PI) - t) / ax;
    return x >= 0 ? v : 2*exp(x*x) - v;
}

CALCULATE_VALUE_BEGIN(FuncEMG)
    realt a = av_[0];
    realt bx = av_[1] - x;
    realt c = av_[2];
    realt d = av_[3];
    realt fact = c*sqrt(M_PI/2)/d;
    realt erf_arg = (bx/c + c/d) / M_SQRT2;
    // e_arg == bx/d + c*c/(2*d*d)
    // erf_arg^2 == bx^2/(2*c^2) + bx/d  + c^2/(2*d^2)
    // e_arg == erf_arg^2 - bx^2/(2*c^2)
    realt t;
    // type double cannot handle erfc(x) for x >= 28
    if (fabs(erf_arg) < 20) {
        realt e_arg = bx/d + c*c/(2*d*d);
        // t = fact * exp(e_arg) * (d >= 0 ? 1-erf(erf_arg) : -1-erf(erf_arg));
        t = fact * exp(e_arg) * (d >= 0 ? erfc(erf_arg) : -erfc(-erf_arg));
    }
    else if ((d >= 0 && erf_arg > -26) || (d < 0 && -erf_arg > -26)) {
        realt h = exp(-bx*bx/(2*c*c));
        realt ee = d >= 0 ? erfcexp_x4(erf_arg) : -erfcexp_x4(-erf_arg);
        t = fact * h * ee;
    }
    else
        t = 0;
CALCULATE_VALUE_END(a*t)

CALCULATE_DERIV_BEGIN(FuncEMG)
    realt a = av_[0];
    realt bx = av_[1] - x;
    realt c = av_[2];
    realt d = av_[3];
    realt fact = c*sqrt(M_PI/2)/d;
    realt erf_arg = (bx/c + c/d) / M_SQRT2; //== (c*c+b*d-d*x)/(M_SQRT2*c*d);
    realt ee;
    if (fabs(erf_arg) < 20)
        ee = exp(erf_arg*erf_arg) * (d >= 0 ? erfc(erf_arg) : -erfc(-erf_arg));
    else if ((d >= 0 && erf_arg > -26) || (d < 0 && -erf_arg > -26))
        ee = d >= 0 ? erfcexp_x4(erf_arg) : -erfcexp_x4(-erf_arg);
    else
        ee = 0;
    realt h = exp(-bx*bx/(2*c*c));
    realt t = fact * h * ee;
    dy_dv[0] = t;
    dy_dv[1] = -a/d * h + a*t/d;
    dy_dv[2] = -a/(c*d*d) * (h * (c*c - bx*d) - t * (c*c + d*d));
    dy_dv[3] =  a/(d*d*d) * (h * c*c - t * (c*c + d*d + bx*d));
    dy_dx = - dy_dv[1];
CALCULATE_DERIV_END(a*t)


bool FuncEMG::get_area(realt* a) const
{
    *a = av_[0]*av_[2]*sqrt(2*M_PI);
    return true;
}

///////////////////////////////////////////////////////////////////////

bool FuncDoniachSunjic::get_nonzero_range(double/*level*/,
                                          realt&/*left*/, realt&/*right*/) const
{
    return false;
}

CALCULATE_VALUE_BEGIN(FuncDoniachSunjic)
    realt h = av_[0];
    realt a = av_[1];
    realt F = av_[2];
    realt xE = x - av_[3];
    realt t = h * cos(M_PI*a/2 + (1-a)*atan(xE/F)) / pow(F*F+xE*xE, (1-a)/2);
CALCULATE_VALUE_END(t)

CALCULATE_DERIV_BEGIN(FuncDoniachSunjic)
    realt h = av_[0];
    realt a = av_[1];
    realt F = av_[2];
    realt xE = x - av_[3];
    realt fe2 = F*F+xE*xE;
    realt ac = 1-a;
    realt p = pow(fe2, -ac/2);
    realt at = atan(xE/F);
    realt cos_arg = M_PI*a/2 + ac*at;
    realt co = cos(cos_arg);
    realt si = sin(cos_arg);
    realt t = co * p;
    dy_dv[0] = t;
    dy_dv[1] = h * p * (co/2 * log(fe2) + (at-M_PI/2) * si);
    dy_dv[2] = h * ac*p/fe2 * (xE*si - F*co);
    dy_dv[3] = h * ac*p/fe2 * (xE*co + si*F);
    dy_dx = -dy_dv[3];
CALCULATE_DERIV_END(h*t)
///////////////////////////////////////////////////////////////////////


CALCULATE_VALUE_BEGIN(FuncPielaszekCube)
    realt height = av_[0];
    realt center = av_[1];
    realt R = av_[2];
    realt s = av_[3];
    realt s2 = s*s;
    realt s4 = s2*s2;
    realt R2 = R*R;

    realt q = (x-center);
    realt q2 = q*q;
    realt t = height * (-3*R*(-1 - (R2*(-1 +
                          pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))
                          * cos(2*(-1.5 + R2/(2.*s2)) * atan((q*s2)/R))))/
           (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
      (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2);
CALCULATE_VALUE_END(t)

CALCULATE_DERIV_BEGIN(FuncPielaszekCube)
    realt height = av_[0];
    realt center = av_[1];
    realt R = av_[2];
    realt s = av_[3];
    realt s2 = s*s;
    realt s3 = s*s2;
    realt s4 = s2*s2;
    realt R2 = R*R;
    realt R4 = R2*R2;
    realt R3 = R*R2;

    realt q = (x-center);
    realt q2 = q*q;
    realt t = (-3*R*(-1 - (R2*(-1 +
                              pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))
                              * cos(2*(-1.5 + R2/(2.*s2)) * atan((q*s2)/R))))/
               (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
          (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2);

    realt dcenter = height * (
            (3*sqrt(2/M_PI)*R*(-1 -
                        (R2* (-1 + pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
   (q*q2*(-0.5 + R2/(2.*s2))*s2) - (3*R*((R2*(-1 +
            pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (q*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4) -
       (R2*((2*q*(1.5 - R2/(2.*s2))* s4*
               pow(1 + (q2*s4)/R2, 0.5 - R2/(2.*s2))*
               cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R)))/R2 -
            (2*(-1.5 + R2/(2.*s2))*s2* pow(1 + (q2*s4)/R2,
                0.5 - R2/(2.*s2))* sin(2*(-1.5 + R2/(2.*s2))*
                 atan((q*s2)/R)))/R))/
        (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
   (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2));

    realt dR = height * (
        (3*R2*(-1 - (R2* (-1 + pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))*
               atan((q*s2)/R))))/ (2.*q2*(-1.5 + R2/(2.*s2))*
          (-1 + R2/(2.*s2))*s4)))/ (sqrt(2*M_PI)*q2*pow(-0.5 + R2/(2.*s2),2)*
     s4) - (3*(-1 - (R2*(-1 + pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))*
               atan((q*s2)/R))))/ (2.*q2*(-1.5 + R2/(2.*s2))*
          (-1 + R2/(2.*s2))*s4)))/ (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))*
     s2) - (3*R*((R3* (-1 + pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))*
               atan((q*s2)/R))))/ (2.*q2*(-1.5 + R2/(2.*s2))*
          pow(-1 + R2/(2.*s2),2)*s4*s2) + (R3*(-1 + pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*pow(-1.5 + R2/(2.*s2),2)* (-1 + R2/(2.*s2))*(s4*s2)) -
       (R*(-1 + pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4) -
       (R2*(pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))*
             ((-2*q2*(1.5 - R2/(2.*s2))* s4)/ (R3*
                  (1 + (q2*s4)/R2)) - (R*log(1 + (q2*s4)/R2))/ s2) +
            pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))* ((2*q*(-1.5 + R2/(2.*s2))*
                  s2)/ (R2* (1 + (q2*s4)/R2)) - (2*R*atan((q*s2)/R))/s2)*
             sin(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
   (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2));

    realt ds = height * (
            (-3*R3*(-1 - (R2* (-1 + pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))*
               atan((q*s2)/R))))/ (2.*q2*(-1.5 + R2/(2.*s2))*
          (-1 + R2/(2.*s2))*s4)))/
   (sqrt(2*M_PI)*q2*pow(-0.5 + R2/(2.*s2),2)* (s4*s)) + (3*sqrt(2/M_PI)*R*
     (-1 - (R2*(-1 + pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
   (q2*(-0.5 + R2/(2.*s2))*s3) - (3*R*(-(R4*(-1 +
             pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
              cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*(-1.5 + R2/(2.*s2))* pow(-1 + R2/(2.*s2),2)*(s4*s3)) -
       (R4*(-1 + pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*pow(-1.5 + R2/(2.*s2),2)* (-1 + R2/(2.*s2))*(s4*s3)) +
       (2*R2*(-1 + pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))*
               atan((q*s2)/R))))/ (q2*(-1.5 + R2/(2.*s2))*
          (-1 + R2/(2.*s2))*(s4*s)) - (R2*(pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))*
               atan((q*s2)/R))* ((4*q2*(1.5 - R2/(2.*s2))* s3)/
                (R2* (1 + (q2*s4)/R2)) + (R2*log(1 +
                    (q2*s4)/R2))/ s3) +
            pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             ((-4*q*(-1.5 + R2/(2.*s2))*s)/ (R*(1 + (q2*s4)/R2)) +
               (2*R2*atan((q*s2)/R))/ s3)*
             sin(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
   (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2));

    dy_dv[0] = t;
    dy_dv[1] = -dcenter;
    dy_dv[2] = dR;
    dy_dv[3] = ds;
    dy_dx = dcenter;
CALCULATE_DERIV_END(height*t)

///////////////////////////////////////////////////////////////////////

// Implemented by Mirko Scholz. The formula is taken from:
// Bingemann, D.; Ernsting, N. P. J. Chem. Phys. 1995, 102, 2691–2700.

void FuncLogNormal::more_precomputations()
{
    if (av_.size() != 4)
        av_.resize(4);
    if (fabs(av_[2]) < epsilon)
        av_[2] = epsilon;
    if (fabs(av_[3]) < epsilon)
        av_[3] = 0.001;
}

CALCULATE_VALUE_BEGIN(FuncLogNormal)
    realt a = 2.0 * av_[3] * (x - av_[1]) / av_[2];
    realt ex = 0.0;
    if (a > -1.0) {
        realt b = log(1 + a) / av_[3];
        ex = av_[0] * exp(-M_LN2 * b * b);
    }
CALCULATE_VALUE_END(ex)

CALCULATE_DERIV_BEGIN(FuncLogNormal)
    realt a = 2.0 * av_[3] * (x - av_[1]) / av_[2];
    realt ex;
    if (a > -1.0) {
        realt b = log(1 + a) / av_[3];
        ex = exp(-M_LN2 * b * b);
        dy_dv[0] = ex;
        ex *= av_[0];
        dy_dv[1] = 4.0*M_LN2/(av_[2]*(a+1))*ex*b;
        dy_dv[2] = 4.0*(x-av_[1])*M_LN2/(av_[2]*av_[2]*(a+1))*ex*b;
        dy_dv[3] = ex*(2.0*M_LN2*b*b/av_[3]
            -4.0*(x-av_[1])*log(a+1)*M_LN2/(av_[2]*av_[3]*av_[3]*(a+1)));
        dy_dx = -4.0*M_LN2/(av_[2]*(a+1))*ex*b;
    }
    else {
        ex = 0.0;
        dy_dv[0] = 0.0;
        dy_dv[1] = 0.0;
        dy_dv[2] = 0.0;
        dy_dv[3] = 0.0;
        dy_dx = 0.0;
    }
CALCULATE_DERIV_END(ex)

bool FuncLogNormal::get_nonzero_range(double level,
                                      realt &left, realt &right) const
{ /* untested */
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(av_[0]))
        left = right = 0;
    else {
        //realt w = sqrt (log (fabs(av_[0]/level)) / M_LN2) * av_[2];
        realt w1 = (1-exp(sqrt(log(fabs(av_[0]/level))/M_LN2)*av_[3]))*av_[2]
            /2.0/av_[3]+av_[1];
        realt w0 = (1-exp(-sqrt(log(fabs(av_[0]/level))/M_LN2)*av_[3]))*av_[2]
            /2.0/av_[3]+av_[1];
        if (w1>w0) {
            left = w0;
            right = w1;
        }
        else {
            left = w1;
            right = w0;
        }
    }
    return true;
}

//cf. eq. 28 of Maroncelli, M.; Fleming, G.R. J. Phys. Chem. 1987, 86, 6221-6239
bool FuncLogNormal::get_fwhm(realt* a) const
{
   *a = av_[2]*sinh(av_[3])/av_[3];
   return true;
}

bool FuncLogNormal::get_area(realt* a) const
{
    *a = av_[0]/sqrt(M_LN2/M_PI) / (2.0/av_[2]) / exp(-av_[3]*av_[3]/4.0/M_LN2);
    return true;
}

///////////////////////////////////////////////////////////////////////

// so far all the va functions have parameters x1,y1,x2,y2,...
std::string VarArgFunction::get_param(int n) const
{
    if (is_index(n, av_))
        return (n % 2 == 0 ? "x" : "y") + S(n/2 + 1);
    else
        return "";
}

void FuncSpline::more_precomputations()
{
    q_.resize(nv() / 2);
    for (size_t i = 0; i < q_.size(); ++i) {
        q_[i].x = av_[2*i];
        q_[i].y = av_[2*i+1];
    }
    prepare_spline_interpolation(q_);

}

CALCULATE_VALUE_BEGIN(FuncSpline)
    realt t = get_spline_interpolation(q_, x);
CALCULATE_VALUE_END(t)

CALCULATE_DERIV_BEGIN(FuncSpline)
    dy_dx = 0; // unused
    //dy_dv[];
    realt t = get_spline_interpolation(q_, x);
CALCULATE_DERIV_END(t)

///////////////////////////////////////////////////////////////////////

void FuncPolyline::more_precomputations()
{
    q_.resize(nv() / 2);
    for (size_t i = 0; i < q_.size(); ++i) {
        q_[i].x = av_[2*i];
        q_[i].y = av_[2*i+1];
    }
}

CALCULATE_VALUE_BEGIN(FuncPolyline)
    realt t = get_linear_interpolation(q_, x);
CALCULATE_VALUE_END(t)

CALCULATE_DERIV_BEGIN(FuncPolyline)
    double value;
    if (q_.empty()) {
        dy_dx = 0;
        value = 0.;
    }
    else if (q_.size() == 1) {
        //dy_dv[0] = 0; // 0 -> p_x
        dy_dv[1] = 1; // 1 -> p_y
        dy_dx = 0;
        value = q_[0].y;
    }
    else {
        // value = p0.y + (p1.y - p0.y) / (p1.x - p0.x) * (x - p0.x);
        vector<PointD>::iterator pos = get_interpolation_segment(q_, x);
        double lx = (pos + 1)->x - pos->x;
        double ly = (pos + 1)->y - pos->y;
        double d = x - pos->x;
        double a = ly / lx;
        size_t npos = pos - q_.begin();
        dy_dv[2*npos+0] = a*d/lx - a; // p0.x
        dy_dv[2*npos+1] = 1 - d/lx; // p0.y
        dy_dv[2*npos+2] = -a*d/lx; // p1.x
        dy_dv[2*npos+3] = d/lx; // p1.y
        dy_dx = a;
        value = pos->y + a * d;
    }
CALCULATE_DERIV_END(value)

///////////////////////////////////////////////////////////////////////

} // namespace fityk
