// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

// BgManager - background (baseline) manager. Used for manual, interactive
// background setting.

#include <wx/wx.h>

#include "bgm.h"
#include "frame.h"
#include "plot.h"
#include "fityk/logic.h"
#include "fityk/data.h"
#include "fityk/func.h"

using namespace std;
using fityk::PointQ;
using fityk::PointD;
using fityk::ModelManager;

BgManager::BgManager(const Scale& x_scale)
    : x_scale_(x_scale), spline_(true), data_idx_(-1)
{
    read_recent_baselines();
}

BgManager::~BgManager()
{
    write_recent_baselines();
}

void BgManager::update_focused_data(int idx)
{
    if (data_idx_ == idx)
        return;
    define_bg_func();
    data_idx_ = idx;
    bg_from_func();
    frame->update_toolbar();
}

string BgManager::get_bg_name() const
{
    return "bg" + S(data_idx_);
}

void BgManager::set_stripped(bool value)
{
    stripped_.resize(ftk->dk.count());
    stripped_[data_idx_] = value;
}

bool BgManager::stripped() const
{
    return is_index(data_idx_, stripped_) && stripped_[data_idx_];
}

const wxString& BgManager::get_recent_bg_name(int n) const
{
    static const wxString empty;
    int idx = recent_bg_.size() - 1 - n;
    return (is_index(idx, recent_bg_) ? recent_bg_[idx].first : empty);
}

void BgManager::bg_from_func()
{
    if (data_idx_ == -1 || stripped()) {
        bg_.clear();
        return;
    }
    string name = get_bg_name();
    int nr = ftk->mgr.find_function_nr(name);
    if (nr == -1) {
        bg_.clear();
        return;
    }
    const fityk::Function *f = ftk->mgr.get_function(nr);
    if (f->tp()->name != "Spline" && f->tp()->name != "Polyline") {
        bg_.clear();
        return;
    }
    int len = f->nv() / 2;
    bg_.resize(len);
    for (int i = 0; i < len; ++i) {
        bg_[i].x = f->av()[2*i];
        bg_[i].y = f->av()[2*i+1];
    }
}

void BgManager::add_background_point(double x, double y)
{
    if (bg_.empty() && ftk->mgr.find_function_nr(get_bg_name()) >= 0) {
        int r = wxMessageBox(wxT("Function %") + s2wx(get_bg_name())
                             + wxT(" already exists\n")
                             wxT("and your actions may overwrite it.\n")
                             wxT("Continue?"),
                             wxT("Start a new background?"),
                             wxICON_QUESTION|wxYES_NO);
        if (r != wxYES)
            return;
        set_stripped(false);
        frame->update_toolbar();

    }
    rm_background_point(x);
    PointQ t(x, y);
    vector<PointQ>::iterator l = lower_bound(bg_.begin(), bg_.end(), t);
    bg_.insert (l, t);
}

void BgManager::rm_background_point (double x)
{
    int X = x_scale_.px(x);
    double lower = x_scale_.val(X - min_dist);
    double upper = x_scale_.val(X + min_dist);
    if (lower > upper)
        swap(lower, upper);
    vector<PointQ>::iterator l = lower_bound(bg_.begin(), bg_.end(),
                                             PointQ(lower, 0));
    vector<PointQ>::iterator u = upper_bound(bg_.begin(), bg_.end(),
                                             PointQ(upper, 0));
    if (u > l)
        bg_.erase(l, u);
}

void BgManager::clear_background()
{
    bg_.clear();
    string name = get_bg_name();
    int nr = ftk->mgr.find_function_nr(name);
    if (nr != -1)
        exec("delete %" + name);
}

void BgManager::define_bg_func()
{
    if (bg_.empty())
        return;

    string name = get_bg_name();
    string ftype = (spline_ ? "Spline" : "Polyline");

    // if the function already exists and if it's exactly the same, return
    int nr = ftk->mgr.find_function_nr(name);
    if (nr != -1) {
        const fityk::Function *f = ftk->mgr.get_function(nr);
        if (f->tp()->name == ftype && f->nv() == 2 * (int) bg_.size()) {
            bool the_same = true;
            for (size_t i = 0; i != bg_.size(); ++i) {
                const fityk::Variable *vx =
                    ftk->mgr.find_variable(f->used_vars().get_name(2*i));
                const fityk::Variable *vy =
                    ftk->mgr.find_variable(f->used_vars().get_name(2*i+1));
                if (!ModelManager::is_auto(vx->name) || !vx->is_constant() ||
                        S(vx->value()) != S(bg_[i].x) ||
                    !ModelManager::is_auto(vy->name) || !vy->is_constant() ||
                        S(vy->value()) != S(bg_[i].y)) {
                    the_same = false;
                    break;
                }
            }
            if (the_same)
                return;
        }
    }

    string cmd = "%" + name + " = " + ftype + "(";
    v_foreach (PointQ, i, bg_)
        cmd += S(i->x) + "," + S(i->y) + (i+1 == bg_.end() ? ")" : ", ");
    exec(cmd);
}

void BgManager::strip_background()
{
    if (bg_.empty())
        return;
    wxString name = wxDateTime::Now().Format(wxT("%Y-%m-%d %T"));
    name += wxString::Format(wxT(" (%d points)"), (int) bg_.size());
    recent_bg_.push_back(make_pair(name, bg_));
    define_bg_func();
    bg_.clear();
    set_stripped(true);
    exec(frame->get_datasets() + "Y = y - %" + get_bg_name() + "(x)");
}

void BgManager::add_background()
{
    string name = get_bg_name();
    int nr = ftk->mgr.find_function_nr(name);
    if (nr == -1)
        return;
    set_stripped(false);
    bg_from_func();
    exec(frame->get_datasets() + "Y = y + %" + name + "(x)");
}

vector<double> BgManager::calculate_bgline(int window_width,
                                           const Scale& y_scale)
{
    vector<double> bgline(window_width);
    if (spline_)
        prepare_spline_interpolation(bg_);
    for (int i = 0; i < window_width; i++) {
        double x = x_scale_.val(i);
        double y = spline_ ? get_spline_interpolation(bg_, x)
                           : get_linear_interpolation(bg_, x);
        bgline[i] = y_scale.px_d(y);
    }
    return bgline;
}

void BgManager::set_as_recent(int n)
{
    int idx = recent_bg_.size() - 1 - n;
    if (!is_index(idx, recent_bg_))
        return;
    bg_ = recent_bg_[idx].second;
}

void BgManager::set_as_convex_hull()
{
    fityk::SimplePolylineConvex convex;
    const fityk::Data* data = ftk->dk.data(data_idx_);
    for (int i = 0; i < data->get_n(); ++i)
        convex.push_point(data->get_x(i), data->get_y(i));
    const vector<PointD>& vertices = convex.get_vertices();
    bg_.resize(vertices.size());
    for (size_t i = 0; i != bg_.size(); ++i) {
        bg_[i].x = vertices[i].x;
        bg_[i].y = vertices[i].y;
    }
}

bool BgManager::has_fn() const
{
    string name = get_bg_name();
    return ftk->mgr.find_function_nr(name) != -1;
}

void BgManager::write_recent_baselines()
{
    wxConfigBase *c = wxConfig::Get();
    if (!c)
        return;
    wxString t = wxT("/RecentBaselines");
    if (c->HasGroup(t))
        c->DeleteGroup(t);

    int len = recent_bg_.size();
    int start = max(len-10, 0);
    for (int i = start; i < len; ++i) {
        wxString group = t + wxString::Format(wxT("/%d"), i-start);
        c->Write(group + wxT("/Name"), recent_bg_[i].first);
        wxString points;
        for (size_t j = 0; j != recent_bg_[i].second.size(); ++j) {
            const PointQ& p = recent_bg_[i].second[j];
            points += wxString::Format(wxT("%g %g "), p.x, p.y);
        }
        c->Write(group + wxT("/Points"), points);
    }
}

void BgManager::read_recent_baselines()
{
    recent_bg_.clear();
    wxConfigBase *c = wxConfig::Get();
    wxString t = wxT("/RecentBaselines");
    if (!c || !c->HasGroup(t))
        return;
    for (int i = 0; i < 10; i++) {
        wxString group = t + wxString::Format(wxT("/%d"), i);
        if (c->HasEntry(group + wxT("/Name"))) {
            wxString name = c->Read(group + wxT("/Name"), wxT("?"));
            wxString points = c->Read(group + wxT("/Points"), wxT(""));
            vector<string> pp = split_string(wx2s(points), ' ');
            vector<PointQ> q;
            for (size_t j = 0; j < pp.size() / 2; ++j) {
                double x = strtod(pp[2*j].c_str(), NULL);
                double y = strtod(pp[2*j+1].c_str(), NULL);
                q.push_back(PointQ(x, y));
            }
            recent_bg_.push_back(make_pair(name, q));
        }
    }
}
