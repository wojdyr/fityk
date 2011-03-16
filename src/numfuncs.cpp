// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include "common.h"
#include "numfuncs.h"

#include <algorithm>

using namespace std;

/// returns position pos in sorted vector of points, *pos and *(pos+1) are
/// required segment for interpolation
/// optimized for sequenced calling with slowly increasing x's
template<typename T>
typename vector<T>::iterator
get_interpolation_segment(vector<T> &bb,  double x)
{
    //optimized for sequence of x = x1, x2, x3, x1 < x2 < x3...
    static typename vector<T>::iterator pos = bb.begin();
    assert (size(bb) > 1);
    if (x <= bb.front().x)
        return bb.begin();
    if (x >= bb.back().x)
        return bb.end() - 2;
    if (pos < bb.begin() || pos >= bb.end())
        pos = bb.begin();
    // check if current pos is ok
    if (pos->x <= x) {
        //pos->x <= x and x < bb.back().x and bb is sorted  => pos < bb.end()-1
        if (x <= (pos+1)->x)
            return pos;
        // try again
        pos++;
        if (pos->x <= x && (pos+1 == bb.end() || x <= (pos+1)->x))
            return pos;
    }
    pos = lower_bound(bb.begin(), bb.end(), T(x, 0)) - 1;
    // pos >= bb.begin() because x > bb.front().x
    return pos;
}

void prepare_spline_interpolation (vector<PointQ> &bb)
{
    //first wroten for bb interpolation, then generalized
    const int n = bb.size();
    if (n == 0)
        return;
        //find d2y/dx2 and put it in .q
    bb[0].q = 0; //natural spline
    vector<double> u(n);
    for (int k = 1; k <= n-2; k++) {
        PointQ *b = &bb[k];
        double sig = (b->x - (b-1)->x) / ((b+1)->x - (b-1)->x);
        double t = sig * (b-1)->q + 2.;
        b->q = (sig - 1.) / t;
        u[k] = ((b+1)->y - b->y) / ((b+1)->x - b->x) - (b->y - (b-1)->y)
                            / (b->x - (b - 1)->x);
        u[k] = (6. * u[k] / ((b+1)->x - (b-1)->x) - sig * u[k-1]) / t;
    }
    bb.back().q = 0;
    for (int k = n-2; k >= 0; k--) {
        PointQ *b = &bb[k];
        b->q = b->q * (b+1)->q + u[k];
    }
}

double get_spline_interpolation(vector<PointQ> &bb, double x)
{
    if (bb.empty())
        return 0.;
    if (bb.size() == 1)
        return bb[0].y;
    vector<PointQ>::iterator pos = get_interpolation_segment(bb, x);
    // based on Numerical Recipes www.nr.com
    double h = (pos+1)->x - pos->x;
    double a = ((pos+1)->x - x) / h;
    double b = (x - pos->x) / h;
    double t = a * pos->y + b * (pos+1)->y + ((a * a * a - a) * pos->q
               + (b * b * b - b) * (pos+1)->q) * (h * h) / 6.;
    return t;
}

template <typename T>
double get_linear_interpolation_(vector<T> &bb, double x)
{
    if (bb.empty())
        return 0.;
    if (bb.size() == 1)
        return bb[0].y;
    typename vector<T>::iterator pos = get_interpolation_segment(bb, x);
    double a = ((pos + 1)->y - pos->y) / ((pos + 1)->x - pos->x);
    return pos->y + a * (x  - pos->x);
}

double get_linear_interpolation(vector<PointQ> &bb, double x)
{
    return get_linear_interpolation_(bb, x);
}

double get_linear_interpolation(vector<PointD> &bb, double x)
{
    return get_linear_interpolation_(bb, x);
}


// random number utilities
static const double TINY = 1e-12; //only for rand_gauss() and rand_cauchy()

/// normal distribution, mean=0, variance=1
double rand_gauss()
{
    static bool is_saved = false;
    static double saved;
    if (!is_saved) {
        double rsq, x1, x2;
        while(1) {
            x1 = rand_1_1();
            x2 = rand_1_1();
            rsq = x1 * x1 + x2 * x2;
            if (rsq >= TINY && rsq < 1)
                break;
        }
        double f = sqrt(-2. * log(rsq) / rsq);
        saved = x1 * f;
        is_saved = true;
        return x2 * f;
    }
    else {
        is_saved = false;
        return saved;
    }
}

double rand_cauchy()
{
    while (1) {
        double x1 = rand_1_1();
        double x2 = rand_1_1();
        double rsq = x1 * x1 + x2 * x2;
        if (rsq >= TINY && rsq < 1 && fabs(x1) >= TINY)
            return (x2 / x1);
    }
}


void SimplePolylineConvex::push_point(PointD const& p)
{
    if (vertices_.size() < 2
            || is_left(*(vertices_.end() - 2), *(vertices_.end() - 1), p))
        vertices_.push_back(p);
    else {
        // the middle point (the last one currently in vertices_) is not convex
        // remove it and check again the last three points
        vertices_.pop_back();
        push_point(p);
    }
}

