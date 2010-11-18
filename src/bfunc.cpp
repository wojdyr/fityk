// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$
/// Built-in function definitions

#include "bfunc.h"

#include <boost/math/special_functions/gamma.hpp>

#include "voigt.h"
#include "numfuncs.h"

using namespace std;
using boost::math::lgamma;


#define CALCULATE_VALUE_BEGIN(NAME) \
void NAME::calculate_value_in_range(vector<fp> const &xx, vector<fp> &yy,\
                                    int first, int last) const\
{\
    for (int i = first; i < last; ++i) {\
        fp x = xx[i];


#define CALCULATE_VALUE_END(VAL) \
        yy[i] += (VAL);\
    }\
}

#define CALCULATE_DERIV_BEGIN(NAME) \
void NAME::calculate_value_deriv_in_range(vector<fp> const &xx, \
                                          vector<fp> &yy, \
                                          vector<fp> &dy_da, \
                                          bool in_dx, \
                                          int first, int last) const \
{ \
    int dyn = dy_da.size() / xx.size(); \
    vector<fp> dy_dv(nv()); \
    for (int i = first; i < last; ++i) { \
        fp x = xx[i]; \
        fp dy_dx;


#define CALCULATE_DERIV_END(VAL) \
        if (!in_dx) { \
            yy[i] += (VAL); \
            vector_foreach (Multi, j, multi_) \
                dy_da[dyn*i+j->p] += dy_dv[j->n] * j->mult;\
            dy_da[dyn*i+dyn-1] += dy_dx;\
        }\
        else {  \
            vector_foreach (Multi, j, multi_) \
                dy_da[dyn*i+j->p] += dy_da[dyn*i+dyn-1] * dy_dv[j->n]*j->mult;\
        } \
    } \
}


///////////////////////////////////////////////////////////////////////

const char *FuncConstant::formula
= "Constant(a=avgy) = a";

void FuncConstant::calculate_value_in_range(vector<fp> const&/*xx*/,
                                            vector<fp>& yy,
                                            int first, int last) const
{
    for (int i = first; i < last; ++i)
        yy[i] += vv_[0];
}

CALCULATE_DERIV_BEGIN(FuncConstant)
    (void) x;
    dy_dv[0] = 1.;
    dy_dx = 0;
CALCULATE_DERIV_END(vv_[0])

///////////////////////////////////////////////////////////////////////

const char *FuncLinear::formula
= "Linear(a0=intercept,a1=slope) = a0 + a1 * x";


CALCULATE_VALUE_BEGIN(FuncLinear)
CALCULATE_VALUE_END(vv_[0] + x*vv_[1])

CALCULATE_DERIV_BEGIN(FuncLinear)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dx = vv_[1];
CALCULATE_DERIV_END(vv_[0] + x*vv_[1])

///////////////////////////////////////////////////////////////////////

const char *FuncQuadratic::formula
= "Quadratic(a0=avgy, a1=0, a2=0) = a0 + a1*x + a2*x^2";


CALCULATE_VALUE_BEGIN(FuncQuadratic)
CALCULATE_VALUE_END(vv_[0] + x*vv_[1] + x*x*vv_[2])

CALCULATE_DERIV_BEGIN(FuncQuadratic)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dx = vv_[1] + 2*x*vv_[2];
CALCULATE_DERIV_END(vv_[0] + x*vv_[1] + x*x*vv_[2])

///////////////////////////////////////////////////////////////////////

const char *FuncCubic::formula
= "Cubic(a0=avgy, a1=0, a2=0, a3=0) = a0 + a1*x + a2*x^2 + a3*x^3";


CALCULATE_VALUE_BEGIN(FuncCubic)
CALCULATE_VALUE_END(vv_[0] + x*vv_[1] + x*x*vv_[2] + x*x*x*vv_[3])

CALCULATE_DERIV_BEGIN(FuncCubic)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dx = vv_[1] + 2*x*vv_[2] + 3*x*x*vv_[3];
CALCULATE_DERIV_END(vv_[0] + x*vv_[1] + x*x*vv_[2] + x*x*x*vv_[3])

///////////////////////////////////////////////////////////////////////

const char *FuncPolynomial4::formula
= "Polynomial4(a0=avgy, a1=0, a2=0, a3=0, a4=0) = "
                                 "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4";


CALCULATE_VALUE_BEGIN(FuncPolynomial4)
CALCULATE_VALUE_END(vv_[0] + x*vv_[1] + x*x*vv_[2] + x*x*x*vv_[3]
                                          + x*x*x*x*vv_[4])

CALCULATE_DERIV_BEGIN(FuncPolynomial4)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dx = vv_[1] + 2*x*vv_[2] + 3*x*x*vv_[3] + 4*x*x*x*vv_[4];
CALCULATE_DERIV_END(vv_[0] + x*vv_[1] + x*x*vv_[2] + x*x*x*vv_[3]
                                      + x*x*x*x*vv_[4])

///////////////////////////////////////////////////////////////////////

const char *FuncPolynomial5::formula
= "Polynomial5(a0=avgy, a1=0, a2=0, a3=0, a4=0, a5=0) = "
                             "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5";


CALCULATE_VALUE_BEGIN(FuncPolynomial5)
CALCULATE_VALUE_END(vv_[0] + x*vv_[1] + x*x*vv_[2]
                       + x*x*x*vv_[3] + x*x*x*x*vv_[4] + x*x*x*x*x*vv_[5])

CALCULATE_DERIV_BEGIN(FuncPolynomial5)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dv[5] = x*x*x*x*x;
    dy_dx = vv_[1] + 2*x*vv_[2] + 3*x*x*vv_[3] + 4*x*x*x*vv_[4]
               + 5*x*x*x*x*vv_[5];
CALCULATE_DERIV_END(vv_[0] + x*vv_[1] + x*x*vv_[2]
                          + x*x*x*vv_[3] + x*x*x*x*vv_[4] + x*x*x*x*x*vv_[5])

///////////////////////////////////////////////////////////////////////

const char *FuncPolynomial6::formula
= "Polynomial6(a0=avgy, a1=0, a2=0, a3=0, a4=0, a5=0, a6=0) = "
                     "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5 + a6*x^6";


CALCULATE_VALUE_BEGIN(FuncPolynomial6)
CALCULATE_VALUE_END(vv_[0] + x*vv_[1] + x*x*vv_[2] + x*x*x*vv_[3] +
                        x*x*x*x*vv_[4] + x*x*x*x*x*vv_[5] + x*x*x*x*x*x*vv_[6])

CALCULATE_DERIV_BEGIN(FuncPolynomial6)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dv[5] = x*x*x*x*x;
    dy_dv[6] = x*x*x*x*x*x;
    dy_dx = vv_[1] + 2*x*vv_[2] + 3*x*x*vv_[3] + 4*x*x*x*vv_[4]
                + 5*x*x*x*x*vv_[5] + 6*x*x*x*x*x*vv_[6];
CALCULATE_DERIV_END(vv_[0] + x*vv_[1] + x*x*vv_[2] + x*x*x*vv_[3] +
                        x*x*x*x*vv_[4] + x*x*x*x*x*vv_[5] + x*x*x*x*x*x*vv_[6])

///////////////////////////////////////////////////////////////////////

const char *FuncGaussian::formula
= "Gaussian(height, center, hwhm) = "
               "height*exp(-ln(2)*((x-center)/hwhm)^2)";

void FuncGaussian::more_precomputations()
{
    if (fabs(vv_[2]) < epsilon)
        vv_[2] = epsilon;
}

CALCULATE_VALUE_BEGIN(FuncGaussian)
    fp xa1a2 = (x - vv_[1]) / vv_[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
CALCULATE_VALUE_END(vv_[0] * ex)

CALCULATE_DERIV_BEGIN(FuncGaussian)
    fp xa1a2 = (x - vv_[1]) / vv_[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    dy_dv[0] = ex;
    fp dcenter = 2 * M_LN2 * vv_[0] * ex * xa1a2 / vv_[2];
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dx = -dcenter;
CALCULATE_DERIV_END(vv_[0]*ex)

bool FuncGaussian::get_nonzero_range (fp level, fp &left, fp &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv_[0]))
        left = right = 0;
    else {
        fp w = sqrt (log (fabs(vv_[0]/level)) / M_LN2) * vv_[2];
        left = vv_[1] - w;
        right = vv_[1] + w;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

const char *FuncSplitGaussian::formula
= "SplitGaussian(height, center, hwhm1=fwhm*0.5, hwhm2=fwhm*0.5) = "
                   "if x < center then Gaussian(height, center, hwhm1)"
                   " else Gaussian(height, center, hwhm2)";

void FuncSplitGaussian::more_precomputations()
{
    if (fabs(vv_[2]) < epsilon)
        vv_[2] = epsilon;
    if (fabs(vv_[3]) < epsilon)
        vv_[3] = epsilon;
}

CALCULATE_VALUE_BEGIN(FuncSplitGaussian)
    fp hwhm = (x < vv_[1] ? vv_[2] : vv_[3]);
    fp xa1a2 = (x - vv_[1]) / hwhm;
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
CALCULATE_VALUE_END(vv_[0] * ex)

CALCULATE_DERIV_BEGIN(FuncSplitGaussian)
    fp hwhm = (x < vv_[1] ? vv_[2] : vv_[3]);
    fp xa1a2 = (x - vv_[1]) / hwhm;
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    dy_dv[0] = ex;
    fp dcenter = 2 * M_LN2 * vv_[0] * ex * xa1a2 / hwhm;
    dy_dv[1] = dcenter;
    if (x < vv_[1]) {
        dy_dv[2] = dcenter * xa1a2;
        dy_dv[3] = 0;
    }
    else {
        dy_dv[2] = 0;
        dy_dv[3] = dcenter * xa1a2;
    }
    dy_dx = -dcenter;
CALCULATE_DERIV_END(vv_[0]*ex)

bool FuncSplitGaussian::get_nonzero_range (fp level, fp &left, fp &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv_[0]))
        left = right = 0;
    else {
        fp w1 = sqrt (log (fabs(vv_[0]/level)) / M_LN2) * vv_[2];
        fp w2 = sqrt (log (fabs(vv_[0]/level)) / M_LN2) * vv_[3];
        left = vv_[1] - w1;
        right = vv_[1] + w2;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

const char *FuncLorentzian::formula
= "Lorentzian(height, center, hwhm) = "
                        "height/(1+((x-center)/hwhm)^2)";


void FuncLorentzian::more_precomputations()
{
    if (fabs(vv_[2]) < epsilon)
        vv_[2] = epsilon;
}

CALCULATE_VALUE_BEGIN(FuncLorentzian)
    fp xa1a2 = (x - vv_[1]) / vv_[2];
    fp inv_denomin = 1. / (1 + xa1a2 * xa1a2);
CALCULATE_VALUE_END(vv_[0] * inv_denomin)

CALCULATE_DERIV_BEGIN(FuncLorentzian)
    fp xa1a2 = (x - vv_[1]) / vv_[2];
    fp inv_denomin = 1. / (1 + xa1a2 * xa1a2);
    dy_dv[0] = inv_denomin;
    fp dcenter = 2 * vv_[0] * xa1a2 / vv_[2] * inv_denomin * inv_denomin;
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dx = -dcenter;
CALCULATE_DERIV_END(vv_[0] * inv_denomin)

bool FuncLorentzian::get_nonzero_range (fp level, fp &left, fp &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv_[0]))
        left = right = 0;
    else {
        fp w = sqrt (fabs(vv_[0]/level) - 1) * vv_[2];
        left = vv_[1] - w;
        right = vv_[1] + w;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

const char *FuncPearson7::formula
= "Pearson7(height, center, hwhm, shape=2) = "
                   "height/(1+((x-center)/hwhm)^2*(2^(1/shape)-1))^shape";

void FuncPearson7::more_precomputations()
{
    if (fabs(vv_[2]) < epsilon)
        vv_[2] = epsilon;
    if (vv_.size() != 5)
        vv_.resize(5);
    // not checking for vv_[3]>0.5 nor even >0
    vv_[4] = pow(2, 1. / vv_[3]) - 1;
}

CALCULATE_VALUE_BEGIN(FuncPearson7)
    fp xa1a2 = (x - vv_[1]) / vv_[2];
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv_[4]; //pow (2, 1. / a3) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow (denom_base, - vv_[3]);
CALCULATE_VALUE_END(vv_[0] * inv_denomin)

CALCULATE_DERIV_BEGIN(FuncPearson7)
    fp xa1a2 = (x - vv_[1]) / vv_[2];
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv_[4]; //pow (2, 1. / a3) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow (denom_base, - vv_[3]);
    dy_dv[0] = inv_denomin;
    fp dcenter = 2 * vv_[0] * vv_[3] * pow_2_1_a3_1 * xa1a2 * inv_denomin /
                                                      (denom_base * vv_[2]);
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dv[3] = vv_[0] * inv_denomin * (M_LN2 * (pow_2_1_a3_1 + 1)
                       * xa1a2sq / (denom_base * vv_[3]) - log(denom_base));
    dy_dx = -dcenter;
CALCULATE_DERIV_END(vv_[0] * inv_denomin)


bool FuncPearson7::get_nonzero_range (fp level, fp &left, fp &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv_[0]))
        left = right = 0;
    else {
        fp t = (pow(fabs(vv_[0]/level), 1./vv_[3]) - 1)
               / (pow (2, 1./vv_[3]) - 1);
        fp w = sqrt(t) * vv_[2];
        left = vv_[1] - w;
        right = vv_[1] + w;
    }
    return true;
}

fp FuncPearson7::area() const
{
    if (vv_[3] <= 0.5)
        return 0.;
    fp g = exp(lgamma(vv_[3] - 0.5) - lgamma(vv_[3]));
    return vv_[0] * 2 * fabs(vv_[2])
        * sqrt(M_PI) * g / (2 * sqrt (vv_[4]));
    //in f_val_precomputations(): vv_[4] = pow (2, 1. / a3) - 1;
}

///////////////////////////////////////////////////////////////////////

const char *FuncSplitPearson7::formula
= "SplitPearson7(height, center, hwhm1=fwhm*0.5, hwhm2=fwhm*0.5, "
                                                        "shape1=2, shape2=2) = "
    "if x < center then Pearson7(height, center, hwhm1, shape1)"
    " else Pearson7(height, center, hwhm2, shape2)";

void FuncSplitPearson7::more_precomputations()
{
    if (fabs(vv_[2]) < epsilon)
        vv_[2] = epsilon;
    if (fabs(vv_[3]) < epsilon)
        vv_[3] = epsilon;
    if (vv_.size() != 8)
        vv_.resize(8);
    // not checking for vv_[3]>0.5 nor even >0
    vv_[6] = pow(2, 1. / vv_[4]) - 1;
    vv_[7] = pow(2, 1. / vv_[5]) - 1;
}

CALCULATE_VALUE_BEGIN(FuncSplitPearson7)
    int lr = x < vv_[1] ? 0 : 1;
    fp xa1a2 = (x - vv_[1]) / vv_[2+lr];
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv_[6+lr]; //pow(2, 1./shape) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow(denom_base, - vv_[4+lr]);
CALCULATE_VALUE_END(vv_[0] * inv_denomin)

CALCULATE_DERIV_BEGIN(FuncSplitPearson7)
    int lr = x < vv_[1] ? 0 : 1;
    fp hwhm = vv_[2+lr];
    fp shape = vv_[4+lr];
    fp xa1a2 = (x - vv_[1]) / hwhm;
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv_[6+lr]; //pow(2, 1./shape) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow (denom_base, -shape);
    dy_dv[0] = inv_denomin;
    fp dcenter = 2 * vv_[0] * shape * pow_2_1_a3_1 * xa1a2 * inv_denomin /
                                                      (denom_base * hwhm);
    dy_dv[1] = dcenter;
    dy_dv[2] = dy_dv[3] = dy_dv[4] = dy_dv[5] = 0;
    dy_dv[2+lr] = dcenter * xa1a2;
    dy_dv[4+lr] = vv_[0] * inv_denomin * (M_LN2 * (pow_2_1_a3_1 + 1)
                           * xa1a2sq / (denom_base * shape) - log(denom_base));
    dy_dx = -dcenter;
CALCULATE_DERIV_END(vv_[0] * inv_denomin)


bool FuncSplitPearson7::get_nonzero_range (fp level, fp &left, fp &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv_[0]))
        left = right = 0;
    else {
        fp t1 = (pow(fabs(vv_[0]/level), 1./vv_[4]) - 1) / (pow(2, 1./vv_[4]) - 1);
        fp w1 = sqrt(t1) * vv_[2];
        fp t2 = (pow(fabs(vv_[0]/level), 1./vv_[5]) - 1) / (pow(2, 1./vv_[5]) - 1);
        fp w2 = sqrt(t2) * vv_[3];
        left = vv_[1] - w1;
        right = vv_[1] + w2;
    }
    return true;
}

fp FuncSplitPearson7::area() const
{
    if (vv_[4] <= 0.5 || vv_[5] <= 0.5)
        return 0.;
    fp g1 = exp(lgamma(vv_[4] - 0.5) - lgamma(vv_[4]));
    fp g2 = exp(lgamma(vv_[5] - 0.5) - lgamma(vv_[5]));
    return vv_[0] * fabs(vv_[2]) * sqrt(M_PI) * g1 / (2 * sqrt (vv_[6]))
         + vv_[0] * fabs(vv_[3]) * sqrt(M_PI) * g2 / (2 * sqrt (vv_[7]));
}

///////////////////////////////////////////////////////////////////////

const char *FuncPseudoVoigt::formula
= "PseudoVoigt(height, center, hwhm, shape=0.5) = "
                        "height*((1-shape)*exp(-ln(2)*((x-center)/hwhm)^2)"
                                 "+shape/(1+((x-center)/hwhm)^2))";

void FuncPseudoVoigt::more_precomputations()
{
    if (fabs(vv_[2]) < epsilon)
        vv_[2] = epsilon;
}

CALCULATE_VALUE_BEGIN(FuncPseudoVoigt)
    fp xa1a2 = (x - vv_[1]) / vv_[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    fp lor = 1. / (1 + xa1a2 * xa1a2);
    fp without_height =  (1-vv_[3]) * ex + vv_[3] * lor;
CALCULATE_VALUE_END(vv_[0] * without_height)

CALCULATE_DERIV_BEGIN(FuncPseudoVoigt)
    fp xa1a2 = (x - vv_[1]) / vv_[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    fp lor = 1. / (1 + xa1a2 * xa1a2);
    fp without_height =  (1-vv_[3]) * ex + vv_[3] * lor;
    dy_dv[0] = without_height;
    fp dcenter = 2 * vv_[0] * xa1a2 / vv_[2]
                    * (vv_[3]*lor*lor + (1-vv_[3])*M_LN2*ex);
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dv[3] =  vv_[0] * (lor - ex);
    dy_dx = -dcenter;
CALCULATE_DERIV_END(vv_[0] * without_height)

bool FuncPseudoVoigt::get_nonzero_range (fp level, fp &left, fp &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv_[0]))
        left = right = 0;
    else {
        // neglecting Gaussian part and adding 4.0 to compensate it
        fp w = (sqrt (vv_[3] * fabs(vv_[0]/level) - 1) + 4.) * vv_[2];
        left = vv_[1] - w;
        right = vv_[1] + w;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

const char *FuncVoigt::formula
= "Voigt(height, center, gwidth=fwhm*0.4, shape=0.1) ="
                            " convolution of Gaussian and Lorentzian #";

void FuncVoigt::more_precomputations()
{
    if (vv_.size() != 6)
        vv_.resize(6);
    float k, l, dkdx, dkdy;
    humdev(0, fabs(vv_[3]), k, l, dkdx, dkdy);
    vv_[4] = 1. / k;
    vv_[5] = dkdy / k;

    if (fabs(vv_[2]) < epsilon)
        vv_[2] = epsilon;
}

CALCULATE_VALUE_BEGIN(FuncVoigt)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    float k;
    fp xa1a2 = (x - vv_[1]) / vv_[2];
    k = humlik(xa1a2, fabs(vv_[3]));
CALCULATE_VALUE_END(vv_[0] * vv_[4] * k)

CALCULATE_DERIV_BEGIN(FuncVoigt)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    // here fabs(vv_[3]) is used, and dy_dv[3] is negated if vv_[3]<0.
    float k;
    fp xa1a2 = (x-vv_[1]) / vv_[2];
    fp a0a4 = vv_[0] * vv_[4];
    float l, dkdx, dkdy;
    humdev(xa1a2, fabs(vv_[3]), k, l, dkdx, dkdy);
    dy_dv[0] = vv_[4] * k;
    fp dcenter = -a0a4 * dkdx / vv_[2];
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dv[3] = a0a4 * (dkdy - k * vv_[5]);
    if (vv_[3] < 0)
        dy_dv[3] = -dy_dv[3];
    dy_dx = -dcenter;
CALCULATE_DERIV_END(a0a4 * k)

bool FuncVoigt::get_nonzero_range (fp level, fp &left, fp &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv_[0]))
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
static fp voigt_fwhm(fp a2, fp a3)
{
    fp sigma = fabs(a2) / M_SQRT2;
    fp gamma = fabs(a2) * a3;

    fp fG = 2 * sigma * sqrt(2 * M_LN2);
    fp fL = 2 * gamma;

    fp fV = 0.5346 * fL + sqrt(0.2166 * fL * fL + fG * fG);
    return fV;
}

fp FuncVoigt::fwhm() const
{
    return voigt_fwhm(vv_[2], vv_[3]);
}

fp FuncVoigt::area() const
{
    return vv_[0] * fabs(vv_[2] * sqrt(M_PI) * vv_[4]);
}

vector<string> FuncVoigt::get_other_prop_names() const
{
    return vector2(string("GaussianFWHM"), string("LorentzianFWHM"));
}

fp FuncVoigt::other_prop(string const& name) const
{
    if (name == "GaussianFWHM") {
        fp sigma = fabs(vv_[2]) / M_SQRT2;
        return 2 * sigma * sqrt(2 * M_LN2);
    }
    else if (name == "LorentzianFWHM") {
        fp gamma = fabs(vv_[2]) * vv_[3];
        return 2 * gamma;
    }
    else
        return 0.;
}

///////////////////////////////////////////////////////////////////////

const char *FuncVoigtA::formula
= "VoigtA(area, center, gwidth=fwhm*0.4, shape=0.1) = "
                            "convolution of Gaussian and Lorentzian #";

void FuncVoigtA::more_precomputations()
{
    if (vv_.size() != 6)
        vv_.resize(6);
    vv_[4] = 1. / humlik(0, fabs(vv_[3]));

    if (fabs(vv_[2]) < epsilon)
        vv_[2] = epsilon;
}

CALCULATE_VALUE_BEGIN(FuncVoigtA)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    float k;
    fp xa1a2 = (x - vv_[1]) / vv_[2];
    k = humlik(xa1a2, fabs(vv_[3]));
CALCULATE_VALUE_END(vv_[0] / (sqrt(M_PI) * vv_[2]) * k)

CALCULATE_DERIV_BEGIN(FuncVoigtA)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    // here fabs(vv_[3]) is used, and dy_dv[3] is negated if vv_[3]<0.
    float k;
    fp xa1a2 = (x-vv_[1]) / vv_[2];
    fp f = vv_[0] / (sqrt(M_PI) * vv_[2]);
    float l, dkdx, dkdy;
    humdev(xa1a2, fabs(vv_[3]), k, l, dkdx, dkdy);
    dy_dv[0] = k / (sqrt(M_PI) * vv_[2]);
    fp dcenter = -f * dkdx / vv_[2];
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2 - f * k / vv_[2];
    dy_dv[3] = f * dkdy;
    if (vv_[3] < 0)
        dy_dv[3] = -dy_dv[3];
    dy_dx = -dcenter;
CALCULATE_DERIV_END(f * k)

bool FuncVoigtA::get_nonzero_range (fp level, fp &left, fp &right) const
{
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv_[0]))
        left = right = 0;
    else {
        //TODO estimate Voigt's non-zero range
        return false;
    }
    return true;
}

fp FuncVoigtA::fwhm() const
{
    return voigt_fwhm(vv_[2], vv_[3]);
}

fp FuncVoigtA::height() const
{
    return vv_[0] / fabs(vv_[2] * sqrt(M_PI) * vv_[4]);
}


///////////////////////////////////////////////////////////////////////

const char *FuncEMG::formula
= "EMG(a=height, b=center, c=fwhm*0.4, d=fwhm*0.04) ="
                " a*c*(2*pi)^0.5/(2*d) * exp((b-x)/d + c^2/(2*d^2))"
                " * (abs(d)/d - erf((b-x)/(2^0.5*c) + c/(2^0.5*d)))";

void FuncEMG::more_precomputations()
{
}

bool FuncEMG::get_nonzero_range(fp/*level*/, fp&/*left*/, fp&/*right*/) const
    { return false; }

CALCULATE_VALUE_BEGIN(FuncEMG)
    fp a = vv_[0];
    fp bx = vv_[1] - x;
    fp c = vv_[2];
    fp d = vv_[3];
    fp fact = a*c*sqrt(2*M_PI)/(2*d);
    fp ex = exp(bx/d + c*c/(2*d*d));
    //fp erf_arg = bx/(M_SQRT2*c) + c/(M_SQRT2*d);
    fp erf_arg = (bx/c + c/d) / M_SQRT2;
    fp t = fact * ex * (d >= 0 ? erfc(erf_arg) : -erfc(-erf_arg));
    //fp t = fact * ex * (d >= 0 ? 1-erf(erf_arg) : -1-erf(erf_arg));
CALCULATE_VALUE_END(t)

CALCULATE_DERIV_BEGIN(FuncEMG)
    fp a = vv_[0];
    fp bx = vv_[1] - x;
    fp c = vv_[2];
    fp d = vv_[3];
    fp cs2d = c/(M_SQRT2*d);
    fp cc = c*sqrt(M_PI/2)/d;
    fp ex = exp(bx/d + cs2d*cs2d); //==exp((c^2+2bd-2dx) / 2d^2)
    fp bx2c = bx/(M_SQRT2*c);
    fp erf_arg = bx2c + cs2d; //== (c*c+b*d-d*x)/(M_SQRT2*c*d);
    //fp er = erf(erf_arg);
    //fp d_sign = d >= 0 ? 1 : -1;
    //fp ser = d_sign - er;
    fp ser = (d >= 0 ? erfc(erf_arg) : -erfc(-erf_arg));
    fp t = cc * ex * ser;
    fp eee = exp(erf_arg*erf_arg);
    dy_dv[0] = t;
    dy_dv[1] = -a/d * ex / eee + a*t/d;
    dy_dv[2] = - a/(2*c*d*d*d)*exp(-bx2c*bx2c)
                  * (2*d*(c*c-bx*d) - sqrt(2*M_PI)*c*(c*c+d*d) * eee * ser);
    //dy_dv[3] = a*c/(d*d*d)*ex * (c/eee
    //                             - d_sign * (c*cc + sqrt(M_PI/2)*(b+d-x))
    //                             + sqrt(M_PI/2) * (c*c/d + b+d-x) * er);
    dy_dv[3] = a*c/(d*d*d)*ex * (c/eee - ser * (c*cc + sqrt(M_PI/2)*(bx+d)));
    dy_dx = - dy_dv[1];
CALCULATE_DERIV_END(a*t)

///////////////////////////////////////////////////////////////////////

const char *FuncDoniachSunjic::formula
= "DoniachSunjic(h=height, a=0.1, F=1, E=center) ="
    "h * cos(pi*a/2 + (1-a)*atan((x-E)/F)) / (F^2+(x-E)^2)^((1-a)/2)";

bool FuncDoniachSunjic::get_nonzero_range(fp/*level*/, fp&/*left*/,
                                          fp&/*right*/) const
{ return false; }

CALCULATE_VALUE_BEGIN(FuncDoniachSunjic)
    fp h = vv_[0];
    fp a = vv_[1];
    fp F = vv_[2];
    fp xE = x - vv_[3];
    fp t = h * cos(M_PI*a/2 + (1-a)*atan(xE/F)) / pow(F*F+xE*xE, (1-a)/2);
CALCULATE_VALUE_END(t)

CALCULATE_DERIV_BEGIN(FuncDoniachSunjic)
    fp h = vv_[0];
    fp a = vv_[1];
    fp F = vv_[2];
    fp xE = x - vv_[3];
    fp fe2 = F*F+xE*xE;
    fp ac = 1-a;
    fp p = pow(fe2, -ac/2);
    fp at = atan(xE/F);
    fp cos_arg = M_PI*a/2 + ac*at;
    fp co = cos(cos_arg);
    fp si = sin(cos_arg);
    fp t = co * p;
    dy_dv[0] = t;
    dy_dv[1] = h * p * (co/2 * log(fe2) + (at-M_PI/2) * si);
    dy_dv[2] = h * ac*p/fe2 * (xE*si - F*co);
    dy_dv[3] = h * ac*p/fe2 * (xE*co + si*F);
    dy_dx = -dy_dv[3];
CALCULATE_DERIV_END(h*t)
///////////////////////////////////////////////////////////////////////


const char *FuncPielaszekCube::formula
= "PielaszekCube(a=height*0.016, center, r=300, s=150) = ...#";


CALCULATE_VALUE_BEGIN(FuncPielaszekCube)
    fp height = vv_[0];
    fp center = vv_[1];
    fp R = vv_[2];
    fp s = vv_[3];
    fp s2 = s*s;
    fp s4 = s2*s2;
    fp R2 = R*R;

    fp q = (x-center);
    fp q2 = q*q;
    fp t = height * (-3*R*(-1 - (R2*(-1 +
                          pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))
                          * cos(2*(-1.5 + R2/(2.*s2)) * atan((q*s2)/R))))/
           (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
      (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2);
CALCULATE_VALUE_END(t)

CALCULATE_DERIV_BEGIN(FuncPielaszekCube)
    fp height = vv_[0];
    fp center = vv_[1];
    fp R = vv_[2];
    fp s = vv_[3];
    fp s2 = s*s;
    fp s3 = s*s2;
    fp s4 = s2*s2;
    fp R2 = R*R;
    fp R4 = R2*R2;
    fp R3 = R*R2;

    fp q = (x-center);
    fp q2 = q*q;
    fp t = (-3*R*(-1 - (R2*(-1 +
                              pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))
                              * cos(2*(-1.5 + R2/(2.*s2)) * atan((q*s2)/R))))/
               (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
          (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2);

    fp dcenter = height * (
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

    fp dR = height * (
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

    fp ds = height * (
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
CALCULATE_DERIV_END(height*t);

///////////////////////////////////////////////////////////////////////

// Implemented by Mirko Scholz. The formula is taken from:
// Bingemann, D.; Ernsting, N. P. J. Chem. Phys. 1995, 102, 2691–2700.
const char *FuncLogNormal::formula
= "LogNormal(height, center, width=fwhm, asym = 0.1) = "
"height*exp(-ln(2)*(ln(2.0*asym*(x-center)/width+1)/asym)^2)";

void FuncLogNormal::more_precomputations()
{
    if (vv_.size() != 4)
        vv_.resize(4);
    if (fabs(vv_[2]) < epsilon)
        vv_[2] = epsilon;
    if (fabs(vv_[3]) < epsilon)
        vv_[3] = 0.001;
}

CALCULATE_VALUE_BEGIN(FuncLogNormal)
    fp a = 2.0 * vv_[3] * (x - vv_[1]) / vv_[2];
    fp ex = 0.0;
    if (a > -1.0) {
        fp b = log(1 + a) / vv_[3];
        ex = vv_[0] * exp_(-M_LN2 * b * b);
    }
CALCULATE_VALUE_END(ex)

CALCULATE_DERIV_BEGIN(FuncLogNormal)
    fp a = 2.0 * vv_[3] * (x - vv_[1]) / vv_[2];
    fp ex;
    if (a > -1.0) {
        fp b = log(1 + a) / vv_[3];
        ex = exp_(-M_LN2 * b * b);
        dy_dv[0] = ex;
        ex *= vv_[0];
        dy_dv[1] = 4.0*M_LN2/(vv_[2]*(a+1))*ex*b;
        dy_dv[2] = 4.0*(x-vv_[1])*M_LN2/(vv_[2]*vv_[2]*(a+1))*ex*b;
        dy_dv[3] = ex*(2.0*M_LN2*b*b/vv_[3]
            -4.0*(x-vv_[1])*log(a+1)*M_LN2/(vv_[2]*vv_[3]*vv_[3]*(a+1)));
        dy_dx = -4.0*M_LN2/(vv_[2]*(a+1))*ex*b;
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

bool FuncLogNormal::get_nonzero_range (fp level, fp &left, fp &right) const
{ /* untested */
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv_[0]))
        left = right = 0;
    else {
        //fp w = sqrt (log (fabs(vv_[0]/level)) / M_LN2) * vv_[2];
        fp w1 = (1-exp_(sqrt(log(fabs(vv_[0]/level))/M_LN2)*vv_[3]))*vv_[2]
            /2.0/vv_[3]+vv_[1];
        fp w0 = (1-exp_(-sqrt(log(fabs(vv_[0]/level))/M_LN2)*vv_[3]))*vv_[2]
            /2.0/vv_[3]+vv_[1];
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

// cf. eq. 28 of Maroncelli, M.; Fleming, G.R. J. Phys. Chem. 1987, 86, 6221-6239.
fp FuncLogNormal::fwhm() const
{
   return vv_[2]*sinh(vv_[3])/vv_[3];
}

///////////////////////////////////////////////////////////////////////

const char *FuncSpline::formula
= "Spline() = cubic spline #";

void FuncSpline::more_precomputations()
{
    q_.resize(nv() / 2);
    for (size_t i = 0; i < q_.size(); ++i) {
        q_[i].x = vv_[2*i];
        q_[i].y = vv_[2*i+1];
    }
    prepare_spline_interpolation(q_);

}

CALCULATE_VALUE_BEGIN(FuncSpline)
    fp t = get_spline_interpolation(q_, x);
CALCULATE_VALUE_END(t)

CALCULATE_DERIV_BEGIN(FuncSpline)
    dy_dx = 0; // unused
    fp t = get_spline_interpolation(q_, x);
CALCULATE_DERIV_END(t)

///////////////////////////////////////////////////////////////////////

const char *FuncPolyline::formula
= "Polyline() = linear interpolation #";

void FuncPolyline::more_precomputations()
{
    q_.resize(nv() / 2);
    for (size_t i = 0; i < q_.size(); ++i) {
        q_[i].x = vv_[2*i];
        q_[i].y = vv_[2*i+1];
    }
}

CALCULATE_VALUE_BEGIN(FuncPolyline)
    fp t = get_linear_interpolation(q_, x);
CALCULATE_VALUE_END(t)

CALCULATE_DERIV_BEGIN(FuncPolyline)
    dy_dx = 0; // unused
    fp t = get_linear_interpolation(q_, x);
CALCULATE_DERIV_END(t)

///////////////////////////////////////////////////////////////////////

