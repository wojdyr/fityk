// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: logic.cpp 322 2007-07-24 00:17:11Z wojdyr $

#include "view.h"
#include "common.h"
#include "data.h"
#include "model.h"
#include "logic.h"

using namespace std;


const fp View::relative_x_margin = 1./20.;
const fp View::relative_y_margin = 1./20.;


void View::set_datasets(vector<int> const& dd)
{
    datasets_ = dd;
}


string View::str() const
{
    char buffer[128];
    sprintf(buffer, "[%.12g:%.12g] [%.12g:%.12g]", left, right, bottom, top);
    return string(buffer);
}

void View::fit_zoom(const RealRange& hor, const RealRange& ver)
{
    assert(!datasets_.empty());

    // For the first dataset in `dataset' (@n, it doesn't contain @*) both
    // data points and models are considered.
    // For the next ones only data points.
    DataAndModel const* first = F->get_dm(datasets_[0]);
    vector<Model const*> models(1, first->model());
    vector<Data const*> datas(datasets_.size());
    datas[0] = first->data();
    for (size_t i = 1; i < datasets_.size(); ++i)
        datas[i] = F->get_dm(datasets_[i])->data();

    if (hor.from == RealRange::kInf || hor.to == RealRange::kInf) {
        fp x_min=0, x_max=0;
        get_x_range(datas, x_min, x_max);
        if (x_min == x_max) {
            x_min -= 0.1;
            x_max += 0.1;
        }
        if (log_x_) {
            x_min = max(epsilon, x_min);
            x_max = max(epsilon, x_max);
            fp margin = log(x_max / x_min) * relative_x_margin;
            if (hor.from == RealRange::kInf)
                left = exp(log(x_min) - margin);
            if (hor.to == RealRange::kInf)
                right = exp(log(x_max) + margin);
        }
        else {
            fp margin = (x_max - x_min) * relative_x_margin;
            if (hor.from == RealRange::kInf)
                left = x_min - margin;
            if (hor.to == RealRange::kInf)
                right = x_max + margin;
        }
    }

    if (ver.from == RealRange::kInf || ver.to == RealRange::kInf) {
        fp y_min=0, y_max=0;
        get_y_range(datas, models, y_min, y_max);
        if (y_min == y_max) {
            y_min -= 0.1;
            y_max += 0.1;
        }
        if (log_y_) {
            y_min = max(epsilon, y_min);
            y_max = max(epsilon, y_max);
            fp margin = log(y_max / y_min) * relative_y_margin;
            if (ver.from == RealRange::kInf)
                bottom = exp(log(y_min) - margin);
            if (ver.to == RealRange::kInf)
                top = exp(log(y_max) + margin);
        }
        else {
            fp margin = (y_max - y_min) * relative_y_margin;
            if (ver.from == RealRange::kInf)
                bottom = y_min - margin;
            if (ver.to == RealRange::kInf)
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


void View::get_y_range(vector<Data const*> datas, vector<Model const*> models,
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

    for (vector<Model const*>::const_iterator i = models.begin();
                                                     i != models.end(); ++i) {
        Model const* model = *i;
        if (!model->has_any_info())
            continue;
        // estimated model maximum
        fp model_y_max = model->approx_max(left, right);
        if (model_y_max > y_max)
            y_max = model_y_max;
        if (model_y_max < y_min)
            y_min = model_y_max;
    }

    // include or not include zero
    if (!log_y_ && y0_factor_ > 0) {
        fp dy = y_max - y_min;
        if (y_min > 0 && y0_factor_ * dy > y_max)
            y_min = 0;
        else if (y_max < 0 && y0_factor_ * dy > fabs(y_min))
            y_max = 0;
    }
}


void View::parse_and_set(const RealRange& hor, const RealRange& ver,
                         const vector<int>& dd)
{
    set_bounds(hor, ver);
    set_datasets(dd);
    fit_zoom(hor, ver);
}


void View::set_bounds(const RealRange& hor, const RealRange& ver)
{
    if (hor.from == RealRange::kNumber)
        left = hor.from_val;
    if (hor.to == RealRange::kNumber)
        right = hor.to_val;
    if (ver.from == RealRange::kNumber)
        bottom = ver.from_val;
    if (ver.to == RealRange::kNumber)
        top = ver.to_val;
}

