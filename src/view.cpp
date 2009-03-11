// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id: logic.cpp 322 2007-07-24 00:17:11Z wojdyr $

#include "view.h"
#include "common.h"
#include "data.h"
#include "sum.h"
#include "logic.h"

using namespace std;


const fp View::relative_x_margin = 1./20.;
const fp View::relative_y_margin = 1./20.;


void View::set_datasets(vector<int> const& dd) 
{ 
    datasets = dd; 
}


string View::str() const
{ 
    return "[" + (S(left) + ":" + S(right)) + "] "
           "[" + (S(bottom) + ":" + S(top)) + "]";
}


void View::fit_zoom(int flag) 
{
    assert(!datasets.empty());

    // for the first dataset in `dataset' (@n, it doesn't contain @*) both 
    // data points and sum (functions) are considered, for the next ones only 
    // data points
    DataWithSum const* first = F->get_ds(datasets[0]);
    vector<Sum const*> sums(1, first->get_sum()); 
    vector<Data const*> datas(datasets.size()); 
    datas[0] = first->get_data();
    for (size_t i = 1; i < datasets.size(); ++i)
        datas[i] = F->get_ds(datasets[i])->get_data();
    
    if (flag&fit_left || flag&fit_right) {
        fp x_min=0, x_max=0;
        get_x_range(datas, x_min, x_max);
        if (x_min == x_max) {
            x_min -= 0.1; 
            x_max += 0.1;
        }
        if (log_x) {
            x_min = max(epsilon, x_min);
            x_max = max(epsilon, x_max);
            fp margin = log(x_max / x_min) * relative_x_margin;
            if (flag&fit_left)
                left = exp(log(x_min) - margin);
            if (flag&fit_right)
                right = exp(log(x_max) + margin);
        }
        else {
            fp margin = (x_max - x_min) * relative_x_margin;
            if (flag&fit_left)
                left = x_min - margin;
            if (flag&fit_right)
                right = x_max + margin;
        }
    }

    if (flag&fit_top || flag&fit_bottom) {
        fp y_min=0, y_max=0;
        get_y_range(datas, sums, y_min, y_max);
        if (y_min == y_max) {
            y_min -= 0.1; 
            y_max += 0.1;
        }
        if (log_y) {
            y_min = max(epsilon, y_min);
            y_max = max(epsilon, y_max);
            fp margin = log(y_max / y_min) * relative_y_margin;
            if (flag&fit_bottom)
                bottom = exp(log(y_min) - margin);
            if (flag&fit_top)
                top = exp(log(y_max) + margin);
        }
        else {
            fp margin = (y_max - y_min) * relative_y_margin;
            if (flag&fit_bottom)
                bottom = y_min - margin;
            if (flag&fit_top)
                top = y_max + margin;
        }
    }
}


void View::get_x_range(vector<Data const*> datas, fp &x_min, fp &x_max)
{
    if (datas.empty()) 
        throw ExecuteError("Can't find x-y axes ranges for plot");
    x_min = datas.front()->get_x_min();
    x_max = datas.front()->get_x_max();
    for (vector<Data const*>::const_iterator i = datas.begin()+1; 
            i != datas.end(); ++i) {
        x_min = min(x_min, (*i)->get_x_min());
        x_max = max(x_max, (*i)->get_x_max());
    }
}


void View::get_y_range(vector<Data const*> datas, vector<Sum const*> sums,
                       fp &y_min, fp &y_max)
{
    if (datas.empty()) 
        throw ExecuteError("Can't find x-y axes ranges for plot");
    y_min = y_max = (datas.front()->get_n() > 0 ? datas.front()->get_y(0) : 0);
    bool min_max_set = false;
    for (vector<Data const*>::const_iterator i = datas.begin(); 
            i != datas.end(); ++i) {
        vector<Point>::const_iterator f = (*i)->get_point_at(left);
        vector<Point>::const_iterator l = (*i)->get_point_at(right);
        //first we are searching for minimal and max. y in active points
        for (vector<Point>::const_iterator i = f; i < l; i++) {
            if (i->is_active && is_finite(i->y)) {
                min_max_set = true;
                if (i->y > y_max) 
                    y_max = i->y;
                if (i->y < y_min) 
                    y_min = i->y;
            }
        }
    }

    if (!min_max_set || y_min == y_max) { //none or 1 active point, so now we  
                                   // search for min. and max. y in all points 
        for (vector<Data const*>::const_iterator i = datas.begin(); 
                i != datas.end(); ++i) {
            vector<Point>::const_iterator f = (*i)->get_point_at(left);
            vector<Point>::const_iterator l = (*i)->get_point_at(right);
            for (vector<Point>::const_iterator i = f; i < l; i++) { 
                if (!is_finite(i->y))
                    continue;
                min_max_set = true;
                if (i->y > y_max) 
                    y_max = i->y;
                if (i->y < y_min) 
                    y_min = i->y;
            }
        }
    }

    for (vector<Sum const*>::const_iterator i = sums.begin(); 
                                                     i != sums.end(); ++i) {
        Sum const* sum = *i;
        if (!sum->has_any_info())
            continue;
        // estimated sum maximum
        fp sum_y_max = sum->approx_max(left, right);
        if (sum_y_max > y_max) 
            y_max = sum_y_max;
        if (sum_y_max < y_min) 
            y_min = sum_y_max;
    }

    // include or not include zero
    if (!log_y) {
        const fp show_zero_factor = 0.1;
        if (y_min > 0 && y_max - y_min > show_zero_factor * y_max)
            y_min = 0;
        else if (y_max < 0 && y_max - y_min > show_zero_factor * fabs(y_min))
            y_max = 0;
    }
}


void View::parse_and_set(std::vector<std::string> const& lrbt,
                         vector<int> const& dd) 
{
    assert(lrbt.size() == 4);
    string const &left = lrbt[0];
    string const &right = lrbt[1];
    string const &bottom = lrbt[2];
    string const &top = lrbt[3];
    fp l=0., r=0., b=0., t=0.;
    int flag = 0;
    if (left.empty())
        flag |= fit_left;
    else if (left != ".") {
        flag |= change_left;
        l = strtod(left.c_str(), 0);
    }
    if (right.empty())
        flag |= fit_right;
    else if (right != ".") {
        flag |= change_right;
        r = strtod(right.c_str(), 0);
    }
    if (bottom.empty())
        flag |= fit_bottom;
    else if (bottom != ".") {
        flag |= change_bottom;
        b = strtod(bottom.c_str(), 0);
    }
    if (top.empty())
        flag |= fit_top;
    else if (top != ".") {
        flag |= change_top;
        t = strtod(top.c_str(), 0);
    }
    set(l, r, b, t, flag);
    set_datasets(dd);
    fit_zoom(flag);
}


void View::set(fp l, fp r, fp b, fp t, int flag) 
{
    if (flag & change_left) 
        left = l;
    if (flag & change_right) 
        right = r;
    if (flag & change_top) 
        top = t;
    if (flag & change_bottom) 
        bottom = b;
}

