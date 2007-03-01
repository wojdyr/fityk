// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

// it contains also declarations of functions from nmath.cpp

#ifndef FITYK__NUMFUNCS__H__
#define FITYK__NUMFUNCS__H__

#include "common.h"


struct B_point;
/// Points used for parametrized functions. They have q parameter, that
/// is used for cubic spline computation
struct B_point 
{ 
    fp x, y;
    fp q; /* q is used for spline */ 
    B_point (fp x_, fp y_) : x(x_), y(y_) {}
    std::string str() { return "(" + S(x) + "; " + S(y) + ")"; }
};

inline bool operator< (const B_point& p, const B_point& q) 
{ return p.x < q.x; }
    

/// returns position pos in sorted vector of points, *pos and *(pos+1) are
/// required segment for interpolation
/// optimized for sequenced calling with slowly increasing x's
std::vector<B_point>::iterator 
get_interpolation_segment(std::vector<B_point> &bb, fp x);

/// must be run before computing value of cubic spline in point x
/// results are written in B_point::q
/// based on Numerical Recipes www.nr.com
void prepare_spline_interpolation (std::vector<B_point> &bb);

fp get_spline_interpolation(std::vector<B_point> &bb, fp x);
fp get_linear_interpolation(std::vector<B_point> &bb, fp x);

//fp LnGammaE (fp x); /// log_e of Gamma function
double digamma(double x);//in nmath.cpp
double gammafn(double x);//in nmath.cpp
double lgammafn(double x);//in nmath.cpp

// random number utilities
inline fp rand_1_1() { return 2.0 * rand() / RAND_MAX - 1.; }
inline fp rand_0_1() { return static_cast<fp>(rand()) / RAND_MAX; }
inline fp rand_uniform(fp a, fp b) { return a + rand_0_1() * (b-a); }
inline bool rand_bool() { return rand() < RAND_MAX / 2; }
fp rand_gauss();
fp rand_cauchy();

#endif 
