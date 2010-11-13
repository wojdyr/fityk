// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: logic.h 322 2007-07-24 00:17:11Z wojdyr $

#ifndef FITYK__VIEW__H__
#define FITYK__VIEW__H__

#include <string>
#include <vector>

#include "common.h"

class DataAndModel;
class Data;
class Model;
class Ftk;

// used for "info guess" now,
// TODO: use it in all places where real-number range is passed
struct RealRange
{
    enum What { kNone, kInf, kNumber };
    What from, to;
    fp from_val, to_val;

    RealRange() : from(kInf), to(kInf) {}
};

struct Rect
{
    fp left, right, bottom, top;

    Rect(fp l, fp r, fp b, fp t) : left(l), right(r), bottom(b), top(t) {}
    fp width() const { return right - left; }
    fp height() const { return top - bottom; }
};

/// manages view, i.e. x and y range visible currently to the user
/// user can set view in `plot' command, using string like "[20:][-100:1000]"
/// plot command requires also to specify dataset(s), if there is more than
/// one dataset. This is necessary in case the visible range is to be fitted
/// to data.
/// Applications using libfityk can ignore datasets stored in this class or use
/// only the first one, or all.
/// most difficult part here is finding an "auto" view for given data and model
class View: public Rect
{
public:
    static const fp relative_x_margin, relative_y_margin;

    // F is used only in fit_zoom(), can be NULL
    View(Ftk const* F_)
        : Rect(0, 180., -50, 1e3), F(F_), datasets_(1,0),
          log_x_(false), log_y_(false), y0_factor_(10.) {}
    std::string str() const;
    void parse_and_set(const RealRange& hor, const RealRange& ver,
                       std::vector<int> const& dd);
    /// fit specified edges to the data range
    void fit_zoom(const RealRange& hor, const RealRange& ver);
    std::vector<int> const& get_datasets() const { return datasets_; }
    // set range
    void set_bounds(const RealRange& hor, const RealRange& ver);
    void set_log_scale(bool log_x, bool log_y) { log_x_=log_x; log_y_=log_y; }
    fp y0_factor() const { return y0_factor_; }
    void set_y0_factor(fp f) { y0_factor_ = f; }
private:
    Ftk const* F;
    std::vector<int> datasets_;
    bool log_x_, log_y_;
    fp y0_factor_;

    void get_x_range(std::vector<Data const*> datas, fp &x_min, fp &x_max);
    void get_y_range(std::vector<Data const*> datas,
                     std::vector<Model const*> models,
                     fp &y_min, fp &y_max);

    /// set datasets that are to be used when fitting viewed area to data
    void set_datasets(std::vector<int> const& dd);
};

#endif
