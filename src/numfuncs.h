// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__NUMFUNCS__H__
#define FITYK__NUMFUNCS__H__

#include <stdlib.h>
#include "common.h"


/// Points used for parametrized functions. They have q parameter, that
/// is used for cubic spline computation
struct PointQ
{
    fp x, y;
    fp q; /* q is used for spline */
    PointQ() {}
    PointQ(fp x_, fp y_) : x(x_), y(y_) {}
};

inline bool operator< (const PointQ& p, const PointQ& q)
{ return p.x < q.x; }


/// returns position pos in sorted vector of points, *pos and *(pos+1) are
/// required segment for interpolation
/// optimized for sequenced calling with slowly increasing x's
std::vector<PointQ>::iterator
get_interpolation_segment(std::vector<PointQ> &bb, fp x);

/// must be run before computing value of cubic spline in point x
/// results are written in PointQ::q
/// based on Numerical Recipes www.nr.com
void prepare_spline_interpolation (std::vector<PointQ> &bb);

fp get_spline_interpolation(std::vector<PointQ> &bb, fp x);
fp get_linear_interpolation(std::vector<PointQ> &bb, fp x);

// random number utilities
inline fp rand_1_1() { return 2.0 * rand() / RAND_MAX - 1.; }
inline fp rand_0_1() { return static_cast<fp>(rand()) / RAND_MAX; }
inline fp rand_uniform(fp a, fp b) { return a + rand_0_1() * (b-a); }
inline bool rand_bool() { return rand() < RAND_MAX / 2; }
fp rand_gauss();
fp rand_cauchy();


// Simple Polyline Convex Hull Algorithms
// takes as input a sequence of points (x,y), with increasing x coord (added
// in push_point()) and returns points of convex hull (get_vertices())
class SimplePolylineConvex
{
public:
    struct Point { double x, y; };
    void push_point(double x, double y) { Point p = {x, y}; push_point(p); }
    void push_point(Point const& p);
    std::vector<Point> const& get_vertices() const { return vertices_; }
    // test if point p2 left of the line through p0 and p1
    static bool is_left(Point const& p0, Point const& p1, Point const& p2)
        { return (p1.x - p0.x)*(p2.y - p0.y) > (p2.x - p0.x)*(p1.y - p0.y); }

private:
    std::vector<Point> vertices_;
};

#endif
