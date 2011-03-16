// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include "view.h"
#include "common.h"
#include "data.h"
#include "model.h"
#include "logic.h"

using namespace std;


const double View::relative_x_margin = 1./20.;
const double View::relative_y_margin = 1./20.;


string View::str() const
{
    char buffer[128];
    sprintf(buffer, "[%.12g:%.12g] [%.12g:%.12g]",
            hor.from, hor.to, ver.from, ver.to);
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
    DataAndModel const* first = F->get_dm(datasets[0]);
    vector<Model const*> models(1, first->model());
    vector<Data const*> datas(datasets.size());
    datas[0] = first->data();
    for (size_t i = 1; i < datasets.size(); ++i)
        datas[i] = F->get_dm(datasets[i])->data();

    if (hor.from_inf() || hor.to_inf()) {
        double x_min=0, x_max=0;
        get_x_range(datas, x_min, x_max);
        if (x_min == x_max) {
            x_min -= 0.1;
            x_max += 0.1;
        }
        if (log_x_) {
            x_min = max(epsilon, x_min);
            x_max = max(epsilon, x_max);
            double margin = log(x_max / x_min) * relative_x_margin;
            if (hor.from_inf())
                hor.from = exp(log(x_min) - margin);
            if (hor.to_inf())
                hor.to = exp(log(x_max) + margin);
        }
        else {
            double margin = (x_max - x_min) * relative_x_margin;
            if (hor.from_inf())
                hor.from = x_min - margin;
            if (hor.to_inf())
                hor.to = x_max + margin;
        }
    }

    if (ver.from_inf() || ver.to_inf()) {
        double y_min=0, y_max=0;
        get_y_range(datas, models, y_min, y_max);
        if (y_min == y_max) {
            y_min -= 0.1;
            y_max += 0.1;
        }
        if (log_y_) {
            y_min = max(epsilon, y_min);
            y_max = max(epsilon, y_max);
            double margin = log(y_max / y_min) * relative_y_margin;
            if (ver.from_inf())
                ver.from = exp(log(y_min) - margin);
            if (ver.to_inf())
                ver.to = exp(log(y_max) + margin);
        }
        else {
            double margin = (y_max - y_min) * relative_y_margin;
            if (ver.from_inf())
                ver.from = y_min - margin;
            if (ver.to_inf())
                ver.to = y_max + margin;
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
    y_min = y_max = (datas.front()->get_n() > 0 ? datas.front()->get_y(0) : 0);
    bool min_max_set = false;
    v_foreach (Data const*, i, datas) {
        vector<Point>::const_iterator f = (*i)->get_point_at(hor.from);
        vector<Point>::const_iterator l = (*i)->get_point_at(hor.to);
        //first we are searching for minimal and max. y in active points
        for (vector<Point>::const_iterator j = f; j < l; j++) {
            if (j->is_active && is_finite(j->y)) {
                min_max_set = true;
                if (j->y > y_max)
                    y_max = j->y;
                if (j->y < y_min)
                    y_min = j->y;
            }
        }
    }

    if (!min_max_set || y_min == y_max) { //none or 1 active point, so now we
                                   // search for min. and max. y in all points
        v_foreach (Data const*, i, datas) {
            vector<Point>::const_iterator f = (*i)->get_point_at(hor.from);
            vector<Point>::const_iterator l = (*i)->get_point_at(hor.to);
            for (vector<Point>::const_iterator j = f; j < l; j++) {
                if (!is_finite(j->y))
                    continue;
                min_max_set = true;
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
        double model_y_max = model->approx_max(hor.from, hor.to);
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

