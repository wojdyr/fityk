// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_NUMFUNCS_H_
#define FITYK_NUMFUNCS_H_

#include <stdlib.h>
#include "fityk.h"
#include "common.h" // S

namespace fityk {

/// Point used for linear interpolation and polyline convex hull algorithm.
struct PointD
{
    double x, y;
    PointD() {}
    PointD(double x_, double y_) : x(x_), y(y_) {}
    bool operator< (const PointD& b) const { return x < b.x; }
};


/// Point used for spline/polyline interpolation.
/// The q parameter is used for cubic spline computation.
struct PointQ
{
    double x, y;
    double q; /* q is used for spline */
    PointQ() {}
    PointQ(double x_, double y_) : x(x_), y(y_) {}
    bool operator< (const PointQ& b) const { return x < b.x; }
};

/// must be run before computing value of cubic spline in point x
/// results are written in PointQ::q
/// based on Numerical Recipes www.nr.com
FITYK_API void prepare_spline_interpolation (std::vector<PointQ> &bb);

// instantiated for T = PointQ, PointD
template<typename T>
typename std::vector<T>::iterator
get_interpolation_segment(std::vector<T> &bb,  double x);

FITYK_API double get_spline_interpolation(std::vector<PointQ> &bb, double x);

FITYK_API double get_linear_interpolation(std::vector<PointD> &bb, double x);
FITYK_API double get_linear_interpolation(std::vector<PointQ> &bb, double x);

// random number utilities
inline double rand_1_1() { return 2.0 * rand() / RAND_MAX - 1.; }
inline double rand_0_1() { return static_cast<double>(rand()) / RAND_MAX; }
inline double rand_uniform(double a, double b) { return a + rand_0_1()*(b-a); }
inline bool rand_bool() { return rand() < RAND_MAX / 2; }
double rand_gauss();
double rand_cauchy();

// very simple matrix utils
void jordan_solve(std::vector<realt>& A, std::vector<realt>& b, int n);
FITYK_API void invert_matrix(std::vector<realt>&A, int n);
// format (for printing) matrix m x n stored in vec. `mname' is name/comment.
std::string format_matrix(const std::vector<realt>& vec,
                          int m, int n, const char *mname);

// Simple Polyline Convex Hull Algorithms
// takes as input a sequence of points (x,y), with increasing x coord (added
// in push_point()) and returns points of convex hull (get_vertices())
class FITYK_API SimplePolylineConvex
{
public:
    void push_point(double x, double y) { push_point(PointD(x, y)); }
    void push_point(PointD const& p);
    std::vector<PointD> const& get_vertices() const { return vertices_; }
    // test if point p2 left of the line through p0 and p1
    static bool is_left(PointD const& p0, PointD const& p1, PointD const& p2)
        { return (p1.x - p0.x)*(p2.y - p0.y) > (p2.x - p0.x)*(p1.y - p0.y); }

private:
    std::vector<PointD> vertices_;
};


/// search x in [x1, x2] for which %f(x)==val,
/// x1, x2, val: f(x1) <= val <= f(x2) or f(x2) <= val <= f(x1)
/// bisection + Newton-Raphson
template<typename T>
realt find_x_with_value(T *func, realt x1, realt x2, realt val)
{
    const int max_iter = 1000;
    std::vector<realt> dy_da(func->max_param_pos() + 1, 0);
    // we don't need derivatives here, but to make it simpler..
    realt y1 = func->calculate_value_and_deriv(x1, dy_da) - val;
    realt y2 = func->calculate_value_and_deriv(x2, dy_da) - val;
    if ((y1 > 0 && y2 > 0) || (y1 < 0 && y2 < 0))
        throw ExecuteError("Value " + S(val) + " is not bracketed by "
                           + S(x1) + "(" + S(y1+val) + ") and "
                           + S(x2) + "(" + S(y2+val) + ").");
    if (y1 == 0)
        return x1;
    if (y2 == 0)
        return x2;
    if (y1 > 0)
        std::swap(x1, x2);
    realt t = (x1 + x2) / 2.;
    for (int i = 0; i < max_iter; ++i) {
        //check if converged
        if (is_eq(x1, x2))
            return (x1+x2) / 2.;

        // calculate f and df
        dy_da.back() = 0;
        realt f = func->calculate_value_and_deriv(t, dy_da) - val;
        realt df = dy_da.back();

        // narrow the range
        if (f == 0.)
            return t;
        else if (f < 0)
            x1 = t;
        else // f > 0
            x2 = t;

        // select new guess point
        realt dx = -f/df * 1.02; // 1.02 is to jump to the other side of point
        if ((t+dx > x2 && t+dx > x1) || (t+dx < x2 && t+dx < x1)  // outside
                            || i % 20 == 19) {                 // precaution
            //bisection
            t = (x1 + x2) / 2.;
        } else {
            t += dx;
        }
    }
    throw ExecuteError("The search has not converged.");
}

/// finds root of derivative, using bisection method
template<typename T>
realt find_extremum(T *func, realt x1, realt x2)
{
    const int max_iter = 1000;
    std::vector<realt> dy_da(func->max_param_pos() + 1, 0);

    // calculate df
    dy_da.back() = 0;
    func->calculate_value_and_deriv(x1, dy_da);
    realt y1 = dy_da.back();

    dy_da.back() = 0;
    func->calculate_value_and_deriv(x2, dy_da);
    realt y2 = dy_da.back();

    if ((y1 > 0 && y2 > 0) || (y1 < 0 && y2 < 0))
        throw ExecuteError("Derivatives at " + S(x1) + " and " + S(x2)
                           + " have the same sign.");
    if (y1 == 0)
        return x1;
    if (y2 == 0)
        return x2;
    if (y1 > 0)
        std::swap(x1, x2);
    for (int i = 0; i < max_iter; ++i) {
        realt t = (x1 + x2) / 2.;

        // calculate df
        dy_da.back() = 0;
        func->calculate_value_and_deriv(t, dy_da);
        realt df = dy_da.back();

        // narrow the range
        if (df == 0.)
            return t;
        else if (df < 0)
            x1 = t;
        else // df > 0
            x2 = t;

        //check if converged
        if (is_eq(x1, x2))
            return (x1+x2) / 2.;
    }
    throw ExecuteError("The search has not converged.");
}

} // namespace fityk
#endif
