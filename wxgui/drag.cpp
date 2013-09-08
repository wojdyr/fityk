// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// DraggedFunc - used for dragging function in MainPlot

#include "drag.h"
#include "fityk/func.h"
#include "fityk/tplate.h"
#include "fityk/mgr.h"

using namespace std;
using namespace fityk;

static
int find_parameter_with_name(const Tplate& tp, const string& name)
{
    // search for Function(..., height, ...)
    int idx = index_of_element(tp.fargs, name);
    if (idx == -1) {
        const vector<string>& defvals = tp.defvals;
        // search for Function(..., foo=height, ...)
        idx = index_of_element(defvals, name);
        // search for Function(..., foo=height*..., ...)
        if (idx == -1) {
            v_foreach (string, i, defvals)
                if (startswith(*i, name+"*")) {
                    idx = i - defvals.begin();
                    //string mult = defvals.substr(name.size()+1);
                    //double extra_multiplier = TODO(parse mult)
                    break;
                }
        }
    }
    return idx;
}

static
bool bind_param(const ModelManager& mgr,
                DraggedFunc::Drag &drag, const string& name,
                const Function* p, DraggedFunc::DragType how,
                double multiplier=1.)
{
    int idx = find_parameter_with_name(*p->tp(), name);
    if (idx == -1)
        return false;
    const Variable* var = mgr.get_variable(p->used_vars().get_idx(idx));
    if (var->is_simple()) {
        drag.how = how;
        drag.parameter_idx = idx;
        drag.parameter_name = p->get_param(idx);
        drag.variable_name = p->used_vars().get_name(idx);
        drag.value = drag.ini_value = p->av()[idx];
        drag.multiplier = multiplier;
        drag.ini_x = 0.;
    } else {
        drag.how = DraggedFunc::no_drag;
    }
    return true;
}

void DraggedFunc::start(const Function* p, int X, int Y, double x, double y,
                        DraggedFuncObserver* callback)
{
    callback_ = callback;
    drag_x_.how = drag_y_.how = drag_shift_x_.how = drag_shift_y_.how = no_drag;
    drag_x_.parameter_name = drag_y_.parameter_name
        = drag_shift_x_.parameter_name = drag_shift_y_.parameter_name = "-";

    bind_param(mgr_, drag_x_, "center", p, absolute_value)
      || bind_param(mgr_, drag_x_, "xmid", p, absolute_value);
    bind_param(mgr_, drag_y_, "height", p, absolute_value)
      || bind_param(mgr_, drag_y_, "area", p, relative_value)
      || bind_param(mgr_, drag_y_, "avgy", p, absolute_value)
      || bind_param(mgr_, drag_y_, "intercept", p, absolute_value)
      || bind_param(mgr_, drag_y_, "upper", p, absolute_value);
    bind_param(mgr_, drag_shift_x_, "hwhm", p, absolute_value)
      || bind_param(mgr_, drag_shift_x_, "fwhm", p, absolute_value, 2.)
      || bind_param(mgr_, drag_shift_x_, "wsig", p, absolute_value, 1.);
    bind_param(mgr_, drag_shift_y_, "lower", p, absolute_value);

    status_ = "Move to change: " + drag_x_.parameter_name + "/"
        + drag_y_.parameter_name + ", with [Shift]: "
        + drag_shift_x_.parameter_name + "/" + drag_shift_y_.parameter_name;

    pX_ = X;
    pY_ = Y;
    px_ = x;
    py_ = y;
}

void DraggedFunc::move(bool shift, int X, int Y, double x, double y)
{
    Drag &hor = shift ? drag_shift_x_ : drag_x_;
    change_value(&hor, x, x - px_, X - pX_);
    pX_ = X;
    px_ = x;

    Drag &vert = shift ? drag_shift_y_ : drag_y_;
    change_value(&vert, y, y - py_, Y - pY_);
    pY_ = Y;
    py_ = y;
}

void DraggedFunc::change_value(Drag *drag, double x, double dx, int dX)
{
    if (dx == 0. || dX == 0)
        return;

    switch (drag->how) {
        case no_drag:
            return;
        case relative_value:
            if (drag->ini_x == 0.) {
                drag->ini_x = x - dx;
                if (is_zero(drag->ini_x))
                    drag->ini_x += dx;
            }
            //drag->value += dx * fabs(value / x) * multiplier;
            drag->value = x / drag->ini_x * drag->ini_value;
            break;
        case absolute_value:
            drag->value += dx * drag->multiplier;
            break;
        case absolute_pixels:
            drag->value += dX * drag->multiplier;
            break;
    }
    callback_->change_parameter_value(drag->parameter_idx, drag->value);
    has_changed_ = true;
}

void DraggedFunc::stop()
{
    drag_x_.how = drag_y_.how = drag_shift_x_.how = drag_shift_y_.how = no_drag;
    has_changed_ = false;
}

string DraggedFunc::get_cmd() const
{
    const Drag* drags[] = { &drag_x_, &drag_y_, &drag_shift_x_, &drag_shift_y_,
                            NULL };
    string cmd;
    for (const Drag **d = drags; *d != NULL; ++d) {
        const Drag& drag = **d;
        if (drag.how != no_drag && drag.value != drag.ini_value) {
            if (!cmd.empty())
                cmd += "; ";
            cmd += "$" + drag.variable_name + " = ~" + eS(drag.value);
        }
    }
    return cmd;
}

