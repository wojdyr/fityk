// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef NUMFUNCS__H__
#define NUMFUNCS__H__

#include "common.h"

class B_point;

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

#endif //NUMFUNCS__H__
