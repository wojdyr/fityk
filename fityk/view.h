// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_VIEW_H_
#define FITYK_VIEW_H_

#include <string>
#include <vector>

#include "fityk.h"// for RealRange, FITYK_API

namespace fityk {

class Data;
class Model;
class DataKeeper;

struct FITYK_API Rect
{
    RealRange hor, ver;

    Rect(double l, double r, double b, double t)
        { hor.lo = l; hor.hi = r; ver.lo = b; ver.hi = t; }
    double left() const { return hor.lo; }
    double right() const { return hor.hi; }
    double bottom() const { return ver.lo; }
    double top() const { return ver.hi; }
    double width() const { return hor.hi - hor.lo; }
    double height() const { return ver.hi - ver.lo; }
};

/// manages view, i.e. x and y range visible currently to the user
/// user can set view in `plot' command, using string like "[20:][-100:1000]"
/// If the visible range is to be fitted to data/model, given datasets are used.
class FITYK_API View: public Rect
{
public:
    static const double relative_x_margin;
    static const double relative_y_margin;

    View(const DataKeeper* dk)
        : Rect(0, 180., -50, 1e3), dk_(dk),
          log_x_(false), log_y_(false), y0_factor_(10.) {}
    std::string str() const;
    /// fit specified edges to the data range
    void change_view(const RealRange& hor_r, const RealRange& ver_r,
                     const std::vector<int>& datasets);
    void set_log_scale(bool log_x, bool log_y) { log_x_=log_x; log_y_=log_y; }
    double y0_factor() const { return y0_factor_; }
    void set_y0_factor(double f) { y0_factor_ = f; }
private:
    const DataKeeper* dk_;
    bool log_x_, log_y_;
    double y0_factor_;

    void get_x_range(std::vector<Data const*> datas,
                     double &x_min, double &x_max);
    void get_y_range(std::vector<Data const*> datas,
                     std::vector<Model const*> models,
                     double &y_min, double &y_max);
};

} // namespace fityk
#endif
