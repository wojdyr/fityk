// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id: logic.h 322 2007-07-24 00:17:11Z wojdyr $

#ifndef FITYK__VIEW__H__
#define FITYK__VIEW__H__

#include <string>
#include <vector>

#include "common.h"

class DataWithSum;
class Data;
class Sum;
class Ftk;

/// manages view, i.e. x and y range visible currently to the user 
/// user can set view in `plot' command, using string like "[20:][-100:1000]" 
/// plot command requires also to specify dataset(s), if there is more than
/// one dataset. This is necessary in case the visible range is to be fitted
/// to data.
/// Applications using libfityk can ignore datasets stored in this class or use
/// only the first one, or all. 
/// most difficult part here is finding an "auto" view for given data and sum
class View 
{
public:
    enum {
        change_left = 1,
        change_right = 2,
        change_top = 4,
        change_bottom = 8,
        change_all = change_left|change_right|change_top|change_bottom,
        fit_left = 16,
        fit_right = 32,
        fit_horizontally=fit_left|fit_right,
        fit_top = 64,
        fit_bottom = 128,
        fit_vertically = fit_top|fit_bottom,
        fit_all= fit_horizontally|fit_vertically
    };
    static const fp relative_x_margin, relative_y_margin;
    fp left, right, bottom, top;

    // F is used only in fit_zoom(), can be NULL 
    View(Ftk const* F_)
        : left(0), right(180.), bottom(-50), top(1e3), F(F_), datasets(1,0) {}
    fp width() const { return right - left; }
    fp height() const { return top - bottom; }
    std::string str() const;
    void parse_and_set(std::vector<std::string> const& lrbt,
                       std::vector<int> const& dd); 
    /// fit specified edges to the data range 
    void fit_zoom(int flag=fit_all); 
    std::vector<int> const& get_datasets() const { return datasets; }
    // set range
    void set(fp l, fp r, fp b, fp t, int flag=change_all);
private:
    Ftk const* F;
    std::vector<int> datasets;

    void get_x_range(std::vector<Data const*> datas, fp &x_min, fp &x_max);
    void get_y_range(std::vector<Data const*> datas, 
                     std::vector<Sum const*> sums, 
                     fp &y_min, fp &y_max);

    /// set datasets that are to be used when fitting viewed area to data
    void set_datasets(std::vector<int> const& dd); 
};

#endif 
