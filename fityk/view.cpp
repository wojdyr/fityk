// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "view.h"
#include <assert.h>
#include "common.h"
#include "data.h"
#include "model.h"
#include "logic.h"

using namespace std;

namespace fityk {

const double View::relative_x_margin = 1./20.;
const double View::relative_y_margin = 1./20.;

string View::str() const
{
    char buffer[128];
    sprintf(buffer, "[%.12g:%.12g] [%.12g:%.12g]",
            hor.lo, hor.hi, ver.lo, ver.hi);
    return string(buffer);
}

void View::change_view(const RealRange& hor_r, const RealRange& ver_r,
                       const vector<int>& datasets)
{
    assert(!datasets.empty());

    hor = hor_r;
    ver = ver_r;

    // For the first dataset in `dataset' (@n, it doesn't contain @*) both
    // data points and models are considered.
    // For the next ones only data points.
    vector<const Data*> datas(datasets.size());
    for (size_t i = 0; i != datasets.size(); ++i)
        datas[i] = dk_->data(datasets[i]);
    vector<const Model*> models(1, datas[0]->model());

    if (hor.lo_inf() || hor.hi_inf()) {
        double x_min=0, x_max=0;
        get_x_range(datas, x_min, x_max);
        if (x_min == x_max) {
            x_min -= 0.1;
            x_max += 0.1;
        }
        if (log_x_) {
            x_min = max(epsilon, x_min); // 'max' is intentional
            x_max = max(epsilon, x_max);
            double margin = log(x_max / x_min) * relative_x_margin;
            if (hor.lo_inf())
                hor.lo = exp(log(x_min) - margin);
            if (hor.hi_inf())
                hor.hi = exp(log(x_max) + margin);
        } else {
            double margin = (x_max - x_min) * relative_x_margin;
            if (hor.lo_inf())
                hor.lo = x_min - margin;
            if (hor.hi_inf())
                hor.hi = x_max + margin;
        }
    }

    if (ver.lo_inf() || ver.hi_inf()) {
        double y_min=0, y_max=0;
        get_y_range(datas, models, y_min, y_max);
        if (y_min == y_max) {
            y_min -= 0.1;
            y_max += 0.1;
        }
        if (log_y_) {
            y_min = max(epsilon, y_min); // 'max' is intentional
            y_max = max(epsilon, y_max);
            double margin = log(y_max / y_min) * relative_y_margin;
            if (ver.lo_inf())
                ver.lo = exp(log(y_min) - margin);
            if (ver.hi_inf())
                ver.hi = exp(log(y_max) + margin);
        } else {
            double margin = (y_max - y_min) * relative_y_margin;
            if (ver.lo_inf())
                ver.lo = y_min - margin;
            if (ver.hi_inf())
                ver.hi = y_max + margin;
        }
    }
}


void View::get_x_range(vector<Data const*> datas, double &x_min, double &x_max)
{
    if (datas.empty())
        throw ExecuteError("Can't find x-y axes ranges for plot");
    x_min = datas.front()->get_x_min();
    x_max = datas.front()->get_x_max();
    for (vector<Data const*>::const_iterator i = datas.begin() + 1;
            i != datas.end(); ++i) {
        x_min = min(x_min, (*i)->get_x_min());
        x_max = max(x_max, (*i)->get_x_max());
    }
}


void View::get_y_range(vector<Data const*> datas, vector<Model const*> models,
                       double &y_min, double &y_max)
{
    if (datas.empty())
        throw ExecuteError("Can't find x-y axes ranges for plot");
    bool min_max_set = false;
    v_foreach (Data const*, i, datas) {
        vector<Point>::const_iterator f = (*i)->get_point_at(hor.lo);
        vector<Point>::const_iterator l = (*i)->get_point_at(hor.hi);
        //first we are searching for minimal and max. y in active points
        for (vector<Point>::const_iterator j = f; j < l; ++j) {
            if (j->is_active && is_finite(j->y)) {
                if (min_max_set) {
                    if (j->y > y_max)
                        y_max = j->y;
                    if (j->y < y_min)
                        y_min = j->y;
                } else {
                    y_min = y_max= j->y;
                    min_max_set = true;
                }
            }
        }
    }

    if (!min_max_set || y_min == y_max) { //none or 1 active point, so now we
                                   // search for min. and max. y in all points
        v_foreach (Data const*, i, datas) {
            vector<Point>::const_iterator f = (*i)->get_point_at(hor.lo);
            vector<Point>::const_iterator l = (*i)->get_point_at(hor.hi);
            for (vector<Point>::const_iterator j = f; j < l; ++j) {
                if (!is_finite(j->y))
                    continue;
                // min_max_set = true;
                if (j->y > y_max)
                    y_max = j->y;
                if (j->y < y_min)
                    y_min = j->y;
            }
        }
    }

    v_foreach (Model const*, i, models) {
        Model const* model = *i;
        if (model->get_ff().empty())
            continue;
        // estimated model maximum
        double model_y_max = model->approx_max(hor.lo, hor.hi);
        if (model_y_max > y_max)
            y_max = model_y_max;
        if (model_y_max < y_min)
            y_min = model_y_max;
    }

    // include or not include zero
    if (!log_y_ && y0_factor_ > 0) {
        double dy = y_max - y_min;
        if (y_min > 0 && y0_factor_ * dy > y_max)
            y_min = 0;
        else if (y_max < 0 && y0_factor_ * dy > fabs(y_min))
            y_max = 0;
    }
}

} // namespace fityk
