// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$
/// Built-in function definitions

#include "bfunc.h"
#include "voigt.h"
#include "numfuncs.h"

using namespace std;


#define FUNC_CALCULATE_VALUE_BEGIN(NAME) \
void Func##NAME::calculate_value(vector<fp> const &xx, vector<fp> &yy) const\
{\
    int first, last; \
    get_nonzero_idx_range(xx, first, last); \
    for (int i = first; i < last; ++i) {\
        fp x = xx[i]; 


#define FUNC_CALCULATE_VALUE_END(VAL) \
        yy[i] += (VAL);\
    }\
}

#define FUNC_CALCULATE_VALUE(NAME, VAL) \
    FUNC_CALCULATE_VALUE_BEGIN(NAME) \
    FUNC_CALCULATE_VALUE_END(VAL)


#define PUT_DERIVATIVES_AND_VALUE(VAL) \
        if (!in_dx) { \
            yy[i] += (VAL); \
            for (vector<Multi>::const_iterator j = multi.begin(); \
                    j != multi.end(); ++j) \
                dy_da[dyn*i+j->p] += dy_dv[j->n] * j->mult;\
            dy_da[dyn*i+dyn-1] += dy_dx;\
        }\
        else {  \
            for (vector<Multi>::const_iterator j = multi.begin(); \
                    j != multi.end(); ++j) \
                dy_da[dyn*i+j->p] += dy_da[dyn*i+dyn-1] * dy_dv[j->n]*j->mult;\
        }


#define FUNC_CALCULATE_VALUE_DERIV_BEGIN(NAME) \
void Func##NAME::calculate_value_deriv(vector<fp> const &xx, \
                                         vector<fp> &yy, \
                                         vector<fp> &dy_da, \
                                         bool in_dx) const \
{ \
    int first, last; \
    get_nonzero_idx_range(xx, first, last); \
    int dyn = dy_da.size() / xx.size(); \
    vector<fp> dy_dv(nv); \
    for (int i = first; i < last; ++i) { \
        fp x = xx[i]; \
        fp dy_dx;


#define FUNC_CALCULATE_VALUE_DERIV_END(VAL) \
        PUT_DERIVATIVES_AND_VALUE(VAL)  \
    } \
}




///////////////////////////////////////////////////////////////////////

const char *FuncConstant::formula 
= "Constant(a=avgy) = a"; 

void FuncConstant::calculate_value(vector<fp> const&/*xx*/, vector<fp>&yy) const
{
    for (vector<fp>::iterator i = yy.begin(); i != yy.end(); ++i)
        *i += vv[0];
}

void FuncConstant::calculate_value_deriv(vector<fp> const &xx, 
                                         vector<fp> &yy, 
                                         vector<fp> &dy_da,
                                         bool in_dx) const
{
    // dy_da.size() == xx.size() * (parameters.size()+1)
    int dyn = dy_da.size() / xx.size();
    vector<fp> dy_dv(nv);
    for (int i = 0; i < size(yy); ++i) {
        dy_dv[0] = 1.;
        fp dy_dx = 0;
        PUT_DERIVATIVES_AND_VALUE(vv[0]);
    }
}

///////////////////////////////////////////////////////////////////////

const char *FuncLinear::formula 
= "Linear(a0=intercept,a1=slope) = a0 + a1 * x"; 


FUNC_CALCULATE_VALUE(Linear, vv[0] + x*vv[1])

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Linear)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dx = vv[1];
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1])

///////////////////////////////////////////////////////////////////////

const char *FuncQuadratic::formula 
= "Quadratic(a0=avgy, a1=0, a2=0) = a0 + a1*x + a2*x^2"; 


FUNC_CALCULATE_VALUE(Quadratic, vv[0] + x*vv[1] + x*x*vv[2])

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Quadratic)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dx = vv[1] + 2*x*vv[2];
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2])

///////////////////////////////////////////////////////////////////////

const char *FuncCubic::formula 
= "Cubic(a0=avgy, a1=0, a2=0, a3=0) = a0 + a1*x + a2*x^2 + a3*x^3"; 


FUNC_CALCULATE_VALUE(Cubic, vv[0] + x*vv[1] + x*x*vv[2] + x*x*x*vv[3])

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Cubic)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dx = vv[1] + 2*x*vv[2] + 3*x*x*vv[3];
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2] + x*x*x*vv[3])

///////////////////////////////////////////////////////////////////////

const char *FuncPolynomial4::formula 
= "Polynomial4(a0=avgy, a1=0, a2=0, a3=0, a4=0) = "
                                 "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4"; 


FUNC_CALCULATE_VALUE(Polynomial4, vv[0] + x*vv[1] + x*x*vv[2] 
                                         + x*x*x*vv[3] + x*x*x*x*vv[4])

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Polynomial4)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dx = vv[1] + 2*x*vv[2] + 3*x*x*vv[3] + 4*x*x*x*vv[4];
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2] + x*x*x*vv[3]
                                      + x*x*x*x*vv[4])

///////////////////////////////////////////////////////////////////////

const char *FuncPolynomial5::formula 
= "Polynomial5(a0=avgy, a1=0, a2=0, a3=0, a4=0, a5=0) = "
                             "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5"; 


FUNC_CALCULATE_VALUE(Polynomial5, vv[0] + x*vv[1] + x*x*vv[2] 
                               + x*x*x*vv[3] + x*x*x*x*vv[4] + x*x*x*x*x*vv[5])

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Polynomial5)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dv[5] = x*x*x*x*x;
    dy_dx = vv[1] + 2*x*vv[2] + 3*x*x*vv[3] + 4*x*x*x*vv[4] + 5*x*x*x*x*vv[5];
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2] 
                               + x*x*x*vv[3] + x*x*x*x*vv[4] + x*x*x*x*x*vv[5])

///////////////////////////////////////////////////////////////////////

const char *FuncPolynomial6::formula 
= "Polynomial6(a0=avgy, a1=0, a2=0, a3=0, a4=0, a5=0, a6=0) = "
                     "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5 + a6*x^6"; 


FUNC_CALCULATE_VALUE(Polynomial6, vv[0] + x*vv[1] + x*x*vv[2] 
                               + x*x*x*vv[3] + x*x*x*x*vv[4] + x*x*x*x*x*vv[5]
                               + x*x*x*x*x*x*vv[6])

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Polynomial6)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dv[5] = x*x*x*x*x;
    dy_dv[6] = x*x*x*x*x*x;
    dy_dx = vv[1] + 2*x*vv[2] + 3*x*x*vv[3] + 4*x*x*x*vv[4] + 5*x*x*x*x*vv[5]
            + 6*x*x*x*x*x*vv[6];
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2] 
                               + x*x*x*vv[3] + x*x*x*x*vv[4] + x*x*x*x*x*vv[5]
                               + x*x*x*x*x*x*vv[6])

///////////////////////////////////////////////////////////////////////

const char *FuncGaussian::formula 
= "Gaussian(height, center, hwhm) = "
               "height*exp(-ln(2)*((x-center)/hwhm)^2)"; 

void FuncGaussian::more_precomputations() 
{ 
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
}

FUNC_CALCULATE_VALUE_BEGIN(Gaussian)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
FUNC_CALCULATE_VALUE_END(vv[0] * ex)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Gaussian)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    dy_dv[0] = ex;
    fp dcenter = 2 * M_LN2 * vv[0] * ex * xa1a2 / vv[2];
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dx = -dcenter;
FUNC_CALCULATE_VALUE_DERIV_END(vv[0]*ex)

bool FuncGaussian::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        fp w = sqrt (log (fabs(vv[0]/level)) / M_LN2) * vv[2]; 
        left = vv[1] - w;             
        right = vv[1] + w;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

const char *FuncSplitGaussian::formula 
= "SplitGaussian(height, center, hwhm1=fwhm*0.5, hwhm2=fwhm*0.5) = "
                   "height*exp(-ln(2)*((x-center)/(x<center?hwhm1:hwhm2))^2)#"; 

void FuncSplitGaussian::more_precomputations() 
{ 
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
    if (fabs(vv[3]) < EPSILON) 
        vv[3] = EPSILON; 
}

FUNC_CALCULATE_VALUE_BEGIN(SplitGaussian)
    fp hwhm = (x < vv[1] ? vv[2] : vv[3]);
    fp xa1a2 = (x - vv[1]) / hwhm;
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
FUNC_CALCULATE_VALUE_END(vv[0] * ex)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(SplitGaussian)
    fp hwhm = (x < vv[1] ? vv[2] : vv[3]);
    fp xa1a2 = (x - vv[1]) / hwhm;
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    dy_dv[0] = ex;
    fp dcenter = 2 * M_LN2 * vv[0] * ex * xa1a2 / hwhm;
    dy_dv[1] = dcenter;
    if (x < vv[1]) {
        dy_dv[2] = dcenter * xa1a2;
        dy_dv[3] = 0;
    }
    else {
        dy_dv[2] = 0;
        dy_dv[3] = dcenter * xa1a2;
    }
    dy_dx = -dcenter;
FUNC_CALCULATE_VALUE_DERIV_END(vv[0]*ex)

bool FuncSplitGaussian::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        fp w1 = sqrt (log (fabs(vv[0]/level)) / M_LN2) * vv[2]; 
        fp w2 = sqrt (log (fabs(vv[0]/level)) / M_LN2) * vv[3]; 
        left = vv[1] - w1;             
        right = vv[1] + w2;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

const char *FuncLorentzian::formula 
= "Lorentzian(height, center, hwhm) = "
                        "height/(1+((x-center)/hwhm)^2)"; 


void FuncLorentzian::more_precomputations()
{ 
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
}

FUNC_CALCULATE_VALUE_BEGIN(Lorentzian)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp inv_denomin = 1. / (1 + xa1a2 * xa1a2);
FUNC_CALCULATE_VALUE_END(vv[0] * inv_denomin)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Lorentzian)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp inv_denomin = 1. / (1 + xa1a2 * xa1a2);
    dy_dv[0] = inv_denomin;
    fp dcenter = 2 * vv[0] * xa1a2 / vv[2] * inv_denomin * inv_denomin;
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dx = -dcenter;
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] * inv_denomin)

bool FuncLorentzian::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        fp w = sqrt (fabs(vv[0]/level) - 1) * vv[2];
        left = vv[1] - w;             
        right = vv[1] + w;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

const char *FuncPearson7::formula 
= "Pearson7(height, center, hwhm, shape=2) = "
                   "height/(1+((x-center)/hwhm)^2*(2^(1/shape)-1))^shape"; 

void FuncPearson7::more_precomputations()
{ 
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
    if (vv.size() != 5)
        vv.resize(5);
    // not checking for vv[3]>0.5 nor even >0
    vv[4] = pow(2, 1. / vv[3]) - 1;
}

FUNC_CALCULATE_VALUE_BEGIN(Pearson7)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv[4]; //pow (2, 1. / a3) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow (denom_base, - vv[3]);
FUNC_CALCULATE_VALUE_END(vv[0] * inv_denomin)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Pearson7)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv[4]; //pow (2, 1. / a3) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow (denom_base, - vv[3]);
    dy_dv[0] = inv_denomin;
    fp dcenter = 2 * vv[0] * vv[3] * pow_2_1_a3_1 * xa1a2 * inv_denomin /
                                                      (denom_base * vv[2]);
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dv[3] = vv[0] * inv_denomin * (M_LN2 * (pow_2_1_a3_1 + 1)
                       * xa1a2sq / (denom_base * vv[3]) - log(denom_base));
    dy_dx = -dcenter;
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] * inv_denomin)


bool FuncPearson7::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        fp t = (pow(fabs(vv[0]/level), 1./vv[3]) - 1) / (pow (2, 1./vv[3]) - 1);
        fp w = sqrt(t) * vv[2];
        left = vv[1] - w;             
        right = vv[1] + w;
    }
    return true;
}

fp FuncPearson7::area() const
{
    if (vv[3] <= 0.5)
        return 0.;
    fp g = exp(lgammafn(vv[3] - 0.5) - lgammafn(vv[3]));
    return vv[0] * 2 * fabs(vv[2])
        * sqrt(M_PI) * g / (2 * sqrt (vv[4]));
    //in f_val_precomputations(): vv[4] = pow (2, 1. / a3) - 1;
}

///////////////////////////////////////////////////////////////////////

const char *FuncSplitPearson7::formula 
= "SplitPearson7(height, center, hwhm1=fwhm*0.5, hwhm2=fwhm*0.5, "
                                                        "shape1=2, shape2=2) = "
    "height/(1+((x-center)/(x<center?hwhm1:hwhm2))^2"
              "*(2^(1/(x<center?shape1:shape2))-1))^(x<center?shape1:shape2)#";

void FuncSplitPearson7::more_precomputations()
{ 
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
    if (fabs(vv[3]) < EPSILON) 
        vv[3] = EPSILON; 
    if (vv.size() != 8)
        vv.resize(8);
    // not checking for vv[3]>0.5 nor even >0
    vv[6] = pow(2, 1. / vv[4]) - 1;
    vv[7] = pow(2, 1. / vv[5]) - 1;
}

FUNC_CALCULATE_VALUE_BEGIN(SplitPearson7)
    int lr = x < vv[1] ? 0 : 1;
    fp xa1a2 = (x - vv[1]) / vv[2+lr];
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv[6+lr]; //pow(2, 1./shape) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow(denom_base, - vv[4+lr]);
FUNC_CALCULATE_VALUE_END(vv[0] * inv_denomin)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(SplitPearson7)
    int lr = x < vv[1] ? 0 : 1;
    fp hwhm = vv[2+lr];
    fp shape = vv[4+lr];
    fp xa1a2 = (x - vv[1]) / hwhm;
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv[6+lr]; //pow(2, 1./shape) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow (denom_base, -shape);
    dy_dv[0] = inv_denomin;
    fp dcenter = 2 * vv[0] * shape * pow_2_1_a3_1 * xa1a2 * inv_denomin /
                                                      (denom_base * hwhm);
    dy_dv[1] = dcenter;
    dy_dv[2] = dy_dv[3] = dy_dv[4] = dy_dv[5] = 0;
    dy_dv[2+lr] = dcenter * xa1a2;
    dy_dv[4+lr] = vv[0] * inv_denomin * (M_LN2 * (pow_2_1_a3_1 + 1)
                           * xa1a2sq / (denom_base * shape) - log(denom_base));
    dy_dx = -dcenter;
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] * inv_denomin)


bool FuncSplitPearson7::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        fp t1 = (pow(fabs(vv[0]/level), 1./vv[4]) - 1) / (pow(2, 1./vv[4]) - 1);
        fp w1 = sqrt(t1) * vv[2];
        fp t2 = (pow(fabs(vv[0]/level), 1./vv[5]) - 1) / (pow(2, 1./vv[5]) - 1);
        fp w2 = sqrt(t2) * vv[3];
        left = vv[1] - w1;             
        right = vv[1] + w2;
    }
    return true;
}

fp FuncSplitPearson7::area() const
{
    if (vv[4] <= 0.5 || vv[5] <= 0.5)
        return 0.;
    fp g1 = exp(lgammafn(vv[4] - 0.5) - lgammafn(vv[4]));
    fp g2 = exp(lgammafn(vv[5] - 0.5) - lgammafn(vv[5]));
    return vv[0] * fabs(vv[2]) * sqrt(M_PI) * g1 / (2 * sqrt (vv[6]))
         + vv[0] * fabs(vv[3]) * sqrt(M_PI) * g2 / (2 * sqrt (vv[7]));
}

///////////////////////////////////////////////////////////////////////

const char *FuncPseudoVoigt::formula 
= "PseudoVoigt(height, center, hwhm, shape=0.5) = "
                        "height*((1-shape)*exp(-ln(2)*((x-center)/hwhm)^2)"
                                 "+shape/(1+((x-center)/hwhm)^2))"; 

void FuncPseudoVoigt::more_precomputations() 
{ 
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
}

FUNC_CALCULATE_VALUE_BEGIN(PseudoVoigt)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    fp lor = 1. / (1 + xa1a2 * xa1a2);
    fp without_height =  (1-vv[3]) * ex + vv[3] * lor;
FUNC_CALCULATE_VALUE_END(vv[0] * without_height)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(PseudoVoigt)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    fp lor = 1. / (1 + xa1a2 * xa1a2);
    fp without_height =  (1-vv[3]) * ex + vv[3] * lor;
    dy_dv[0] = without_height;
    fp dcenter = 2 * vv[0] * xa1a2 / vv[2]
                    * (vv[3]*lor*lor + (1-vv[3])*M_LN2*ex);
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dv[3] =  vv[0] * (lor - ex);
    dy_dx = -dcenter;
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] * without_height)

bool FuncPseudoVoigt::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        // neglecting Gaussian part and adding 4.0 to compensate it
        fp w = (sqrt (vv[3] * fabs(vv[0]/level) - 1) + 4.) * vv[2];
        left = vv[1] - w;             
        right = vv[1] + w;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

const char *FuncVoigt::formula 
= "Voigt(height, center, gwidth=fwhm*0.4, shape=0.1) ="
                            " convolution of Gaussian and Lorentzian #";

void FuncVoigt::more_precomputations() 
{ 
    if (vv.size() != 6)
        vv.resize(6);
    float k, l, dkdx, dkdy;
    humdev(0, fabs(vv[3]), k, l, dkdx, dkdy);
    vv[4] = 1. / k;
    vv[5] = dkdy / k;

    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
}

FUNC_CALCULATE_VALUE_BEGIN(Voigt)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    float k;
    fp xa1a2 = (x - vv[1]) / vv[2];
    k = humlik(xa1a2, fabs(vv[3]));
FUNC_CALCULATE_VALUE_END(vv[0] * vv[4] * k)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Voigt)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    // here fabs(vv[3]) is used, and dy_dv[3] is negated if vv[3]<0.
    float k;
    fp xa1a2 = (x-vv[1]) / vv[2];
    fp a0a4 = vv[0] * vv[4];
    float l, dkdx, dkdy;
    humdev(xa1a2, fabs(vv[3]), k, l, dkdx, dkdy);
    dy_dv[0] = vv[4] * k;
    fp dcenter = -a0a4 * dkdx / vv[2];
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dv[3] = a0a4 * (dkdy - k * vv[5]);
    if (vv[3] < 0)
        dy_dv[3] = -dy_dv[3];
    dy_dx = -dcenter;
FUNC_CALCULATE_VALUE_DERIV_END(a0a4 * k)

bool FuncVoigt::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        //TODO
        return false; 
    }
    return true;
}

///estimation according to
///http://en.wikipedia.org/w/index.php?title=Voigt_profile&oldid=29250968
fp FuncVoigt::fwhm() const 
{ 
    fp gauss_fwhm = 2 * fabs(vv[2]);
    fp const c0 = 2.0056;
    fp const c1 = 1.0593;
    fp phi = vv[3];
    return gauss_fwhm * (1 - c0*c1 + sqrt(phi*phi + 2*c1*phi + c0*c0*c1*c1));
}

fp FuncVoigt::area() const
{
    return vv[0] * fabs(vv[2] * sqrt(M_PI) * vv[4]);
}
                                                            

///////////////////////////////////////////////////////////////////////

const char *FuncVoigtA::formula 
= "VoigtA(area, center, gwidth=fwhm*0.4, shape=0.1) = "
                            "convolution of Gaussian and Lorentzian #";

void FuncVoigtA::more_precomputations() 
{ 
    if (vv.size() != 6)
        vv.resize(6);
    vv[4] = 1. / humlik(0, fabs(vv[3]));

    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
}

FUNC_CALCULATE_VALUE_BEGIN(VoigtA)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    float k;
    fp xa1a2 = (x - vv[1]) / vv[2];
    k = humlik(xa1a2, fabs(vv[3]));
FUNC_CALCULATE_VALUE_END(vv[0] / (sqrt(M_PI) * vv[2]) * k)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(VoigtA)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    // here fabs(vv[3]) is used, and dy_dv[3] is negated if vv[3]<0.
    float k;
    fp xa1a2 = (x-vv[1]) / vv[2];
    fp f = vv[0] / (sqrt(M_PI) * vv[2]);
    float l, dkdx, dkdy;
    humdev(xa1a2, fabs(vv[3]), k, l, dkdx, dkdy);
    dy_dv[0] = k / (sqrt(M_PI) * vv[2]);
    fp dcenter = -f * dkdx / vv[2];
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2 - f * k / vv[2];
    dy_dv[3] = f * dkdy;
    if (vv[3] < 0)
        dy_dv[3] = -dy_dv[3];
    dy_dx = -dcenter;
FUNC_CALCULATE_VALUE_DERIV_END(f * k)

bool FuncVoigtA::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        //TODO
        return false; 
    }
    return true;
}

///estimation according to
///http://en.wikipedia.org/w/index.php?title=Voigt_profile&oldid=29250968
fp FuncVoigtA::fwhm() const 
{ 
    fp gauss_fwhm = 2 * fabs(vv[2]);
    fp const c0 = 2.0056;
    fp const c1 = 1.0593;
    fp phi = vv[3];
    return gauss_fwhm * (1 - c0*c1 + sqrt(phi*phi + 2*c1*phi + c0*c0*c1*c1));
}

fp FuncVoigtA::height() const
{
    return vv[0] / fabs(vv[2] * sqrt(M_PI) * vv[4]);
}
                                                            

///////////////////////////////////////////////////////////////////////

const char *FuncEMG::formula 
= "EMG(a=height, b=center, c=fwhm*0.4, d=fwhm*0.04) ="
                " a*c*(2*pi)^0.5/(2*d) * exp((b-x)/d + c^2/(2*d^2))"
                " * (sign(d) - erf((b-x)/(2^0.5*c) + c/(2^0.5*d)))";

void FuncEMG::more_precomputations() 
{ 
}

bool FuncEMG::get_nonzero_range(fp/*level*/, fp&/*left*/, fp&/*right*/) const
    { return false; }

FUNC_CALCULATE_VALUE_BEGIN(EMG)
    fp a = vv[0];
    fp bx = vv[1] - x;
    fp c = vv[2];
    fp d = vv[3];
    fp fact = a*c*sqrt(2*M_PI)/(2*d);
    fp ex = exp(bx/d + c*c/(2*d*d));
    fp erf_arg = bx/(M_SQRT2*c) + c/(M_SQRT2*d);
    fp t = fact * ex * (d >= 0 ? erfc(erf_arg) : -erfc(-erf_arg));
    //fp t = fact * ex * (d >= 0 ? 1-erf(erf_arg) : -1-erf(erf_arg));
FUNC_CALCULATE_VALUE_END(t)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(EMG)
    fp a = vv[0];
    fp bx = vv[1] - x;
    fp c = vv[2];
    fp d = vv[3];
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
FUNC_CALCULATE_VALUE_DERIV_END(a*t)

///////////////////////////////////////////////////////////////////////

const char *FuncDoniachSunjic::formula 
= "DoniachSunjic(h=height, a=0.1, F=1, E=center) ="
    "h * cos(pi*a/2 + (1-a)*atan((x-E)/F)) / (F^2+(x-E)^2)^((1-a)/2)";

bool FuncDoniachSunjic::get_nonzero_range(fp/*level*/, fp&/*left*/, 
                                          fp&/*right*/) const
{ return false; }

FUNC_CALCULATE_VALUE_BEGIN(DoniachSunjic)
    fp h = vv[0];
    fp a = vv[1];
    fp F = vv[2];
    fp xE = x - vv[3];
    fp t = h * cos(M_PI*a/2 + (1-a)*atan(xE/F)) / pow(F*F+xE*xE, (1-a)/2);
FUNC_CALCULATE_VALUE_END(t)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(DoniachSunjic)
    fp h = vv[0];
    fp a = vv[1];
    fp F = vv[2];
    fp xE = x - vv[3];
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
FUNC_CALCULATE_VALUE_DERIV_END(h*t)
///////////////////////////////////////////////////////////////////////


const char *FuncPielaszekCube::formula 
= "PielaszekCube(a=height*0.016, center, r=300, s=150) = ...#"; 


FUNC_CALCULATE_VALUE_BEGIN(PielaszekCube)
    fp height = vv[0];
    fp center = vv[1];
    fp R = vv[2];
    fp s = vv[3];
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
FUNC_CALCULATE_VALUE_END(t)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(PielaszekCube)
    fp height = vv[0];
    fp center = vv[1];
    fp R = vv[2];
    fp s = vv[3];
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
FUNC_CALCULATE_VALUE_DERIV_END(height*t);


