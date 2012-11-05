// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__VIEW__H__
#define FITYK__VIEW__H__

#include <string>
#include <vector>

#include "common.h"

namespace fityk {

class DataAndModel;
class Data;
class Model;
class Ftk;

struct Rect
{
    RealRange hor, ver;

    Rect(double l, double r, double b, double t)
        { hor.from = l; hor.to = r; ver.from = b; ver.to = t; }
    double left() const { return hor.from; }
    double right() const { return hor.to; }
    double bottom() const { return ver.from; }
    double top() const { return ver.to; }
    double width() const { return hor.to - hor.from; }
    double height() const { return ver.to - ver.from; }
};

/// manages view, i.e. x and y range visible currently to the user
/// user can set view in `plot' command, using string like "[20:][-100:1000]"
/// If the visible range is to be fitted to data/model, given datasets are used.
class FITYK_API View: public Rect
{
public:
    static const double relative_x_margin, relative_y_margin;

    View(Ftk const* F_)
        : Rect(0, 180., -50, 1e3), F(F_),
          log_x_(false), log_y_(false), y0_factor_(10.) {}
    std::string str() const;
    /// fit specified edges to the data range
    void change_view(const RealRange& hor_r, const RealRange& ver_r,
                     const std::vector<int>& datasets);
    void set_log_scale(bool log_x, bool log_y) { log_x_=log_x; log_y_=log_y; }
    double y0_factor() const { return y0_factor_; }
    void set_y0_factor(double f) { y0_factor_ = f; }
private:
    Ftk const* F;
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
