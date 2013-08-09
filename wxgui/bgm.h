// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_BGM_H_
#define FITYK_WX_BGM_H_

#include "fityk/numfuncs.h" // PointQ definition
class Scale;

class BgManager
{
public:
    //minimal distance in X between bg points
    static const int min_dist = 8;

    BgManager(const Scale& x_scale);
    ~BgManager();
    void update_focused_data(int idx);
    void add_background_point(double x, double y);
    void rm_background_point(double x);
    void clear_background();
    void strip_background();
    // reverses strip_background(), unless %bgX was changed in the meantime  
    void add_background();
    void define_bg_func();
    void bg_from_func();
    bool can_strip() const { return !bg_.empty(); }
    bool has_fn() const;
    void set_spline_bg(bool s) { spline_ = s; }
    void set_as_recent(int n);
    void set_as_convex_hull();
    std::vector<double> calculate_bgline(int window_width,
                                         const Scale& y_scale);
    const std::vector<fityk::PointQ>& get_bg() const { return bg_; }
    bool stripped() const;
    const wxString& get_recent_bg_name(int n) const;
    void read_recent_baselines();
    void write_recent_baselines();

    // handle "ui bg_subtracted_from = index_list"
    void set_bg_subtracted(const std::string& index_list, bool value);
    std::string get_bg_subtracted() const;

private:
    const Scale& x_scale_;
    bool spline_;
    std::vector<fityk::PointQ> bg_;
    std::vector<std::pair<wxString, std::vector<fityk::PointQ> > > recent_bg_;
    std::vector<bool> stripped_;
    int data_idx_;

    std::string get_bg_name() const;
    void set_stripped(bool value);
};

#endif

