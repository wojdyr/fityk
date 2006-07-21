// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_MPLOT__H__
#define FITYK__WX_MPLOT__H__

#include "wx_plot.h"
#include "numfuncs.h" // B_point definition


/// it cares about visualization of spline / polyline background 
/// which can be set by selecting points on Plot

struct t_xy { fp x, y; };
class Function;

class BgManager
{
public:
    BgManager(PlotShared &x_calc_) : x_calc(x_calc_), min_dist(8), 
                                     spline_bg(true) {}
    void add_background_point(fp x, fp y);
    void rm_background_point(fp x);
    void clear_background();
    void forget_background() { clear_background(); bg_backup.clear(); }
    void strip_background();
    void undo_strip_background();
    bool can_strip() const { return !bg.empty(); }
    bool can_undo() const { return !bg_backup.empty(); }
    void set_spline_bg(bool s) { spline_bg=s; recompute_bgline(); }
protected:
    PlotShared &x_calc;
    int min_dist; //minimal distance in X between bg points
    bool spline_bg;
    typedef std::vector<B_point>::iterator bg_iterator;
    typedef std::vector<B_point>::const_iterator bg_const_iterator;
    std::vector<B_point> bg, bg_backup;
    std::vector<t_xy> bgline;
    std::string cmd_tail;

    void recompute_bgline();
};


class ConfigureAxesDlg;

/// main plot, single in application, displays data, fitted peaks etc. 
class MainPlot : public FPlot, public BgManager
{
    friend class ConfigureAxesDlg;
    friend class ConfigurePLabelsDlg;
public:
    MainPlot (wxWindow *parent, PlotShared &shar); 
    ~MainPlot() {}
    void OnPaint(wxPaintEvent &event);
    void Draw(wxDC &dc, bool monochrome=false);
    void OnLeaveWindow (wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent &event);
    void OnButtonDown (wxMouseEvent &event);
    void OnLeftDClick (wxMouseEvent& WXUNUSED(event)) { PeakInfo(); }
    void OnButtonUp (wxMouseEvent &event);
    void OnKeyDown (wxKeyEvent& event);
    void set_scale();

    void OnPopupShowXX (wxCommandEvent& event);
    void OnPopupColor (wxCommandEvent& event);
    void OnInvertColors (wxCommandEvent& event);
    void OnPopupRadius (wxCommandEvent& event);
    void OnConfigureAxes (wxCommandEvent& event);
    void OnConfigurePLabels (wxCommandEvent& event);
    void OnZoomAll (wxCommandEvent& event);
    void PeakInfo ();
    void OnPeakInfo (wxCommandEvent& WXUNUSED(event)) { PeakInfo(); }
    void OnPeakDelete (wxCommandEvent& event);
    void OnPeakGuess(wxCommandEvent &event);
    void cancel_mouse_press();
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
    void update_mouse_hints();
    void set_mouse_mode(MouseModeEnum m);
    MouseModeEnum get_mouse_mode() const { return mode; }
    wxColour const& get_data_color(int n) const
        { return dataColour[n % max_data_cols]; }
    wxColour const& get_func_color(int n) const
        { return peakCol[n % max_peak_cols]; }
    void set_data_color(int n, wxColour const& col) 
        { dataColour[n % max_data_cols] = col; }
    void set_func_color(int n, wxColour const& col) 
        { peakCol[n % max_peak_cols] = col; }
    bool get_x_reversed() const { return x_reversed; }
    void draw_xor_peak(Function const* func, std::vector<fp> const& p_values);
    void show_popup_menu(wxMouseEvent &event);

private:
    MouseModeEnum basic_mode, 
                    mode;  //actual mode -- either basic_mode or mmd_peak
    static const int max_group_cols = 8;
    static const int max_peak_cols = 32;
    static const int max_data_cols = 64;
    static const int max_radius = 4; //size of data point
    bool peaks_visible, groups_visible, sum_visible,  
         plabels_visible, x_reversed; 
    wxFont plabelFont;
    std::string plabel_format;
    bool vertical_plabels;
    std::vector<std::string> plabels;
    wxColour sumCol, bg_pointsCol;
    wxColour groupCol[max_group_cols], peakCol[max_peak_cols];
    wxColour dataColour[max_data_cols];
    int pressed_mouse_button;
    bool ctrl;
    int over_peak;
    int limit1, limit2;

    void draw_x_axis (wxDC& dc, bool set_pen=true);
    void draw_y_axis (wxDC& dc, bool set_pen=true);
    void draw_background(wxDC& dc, bool set_pen=true); 
    void draw_sum (wxDC& dc, Sum const* sum, bool set_pen=true);
    void draw_groups (wxDC& dc, Sum const* sum, bool set_pen=true);
    void draw_peaks (wxDC& dc, Sum const* sum, bool set_pen=true);
    void draw_peaktops (wxDC& dc, Sum const* sum);
    void draw_peaktop_selection(wxDC& dc, Sum const* sum);
    void draw_plabels (wxDC& dc, Sum const* sum, bool set_pen=true);
    void draw_dataset(wxDC& dc, int n, bool set_pen=true);
    void prepare_peaktops(Sum const* sum);
    void prepare_peak_labels(Sum const* sum);
    void look_for_peaktop (wxMouseEvent& event);
    void show_peak_menu (wxMouseEvent &event);
    void peak_draft (Mouse_act_enum ma, wxMouseEvent &event =dummy_mouse_event);
    void move_peak (Mouse_act_enum ma, wxMouseEvent &event = dummy_mouse_event);
    void draw_peak_draft (int X_mid, int X_hwhm, int Y);
    bool rect_zoom (Mouse_act_enum ma, wxMouseEvent &event = dummy_mouse_event);
    void draw_rect (int X1, int Y1, int X2, int Y2);
    bool has_mod_keys(const wxMouseEvent& event); 
    bool visible_peaktops(MouseModeEnum mode);

    DECLARE_EVENT_TABLE()
};

class ConfigureAxesDlg: public wxDialog
{
public:
    ConfigureAxesDlg(wxWindow* parent, wxWindowID id, MainPlot* plot_);
    void OnApply (wxCommandEvent& event);
    void OnClose (wxCommandEvent& event) { OnCancel(event); }
    void OnChangeColor (wxCommandEvent& WXUNUSED(event)) 
                                               { change_color_dlg(axis_color); }
    void OnChangeFont (wxCommandEvent& event); 
private:
    MainPlot *plot;
    wxColour axis_color;
    wxCheckBox *x_show_axis, *x_show_tics, *x_show_grid, *x_reversed; 
    wxSpinCtrl *x_max_tics, *x_tics_size;
    wxCheckBox *y_show_axis, *y_show_tics, *y_show_grid, *y_logarithm; 
    wxSpinCtrl *y_max_tics, *y_tics_size;
    DECLARE_EVENT_TABLE()
};


class ConfigurePLabelsDlg: public wxDialog
{
public:
    ConfigurePLabelsDlg(wxWindow* parent, wxWindowID id, MainPlot* plot_);
    void OnApply (wxCommandEvent& event);
    void OnClose (wxCommandEvent& event) { OnCancel(event); }
    void OnChangeLabelFont (wxCommandEvent& event); 
    void OnChangeLabelText (wxCommandEvent& event); 
    void OnCheckShowLabel (wxCommandEvent& event); 
    void OnRadioLabel (wxCommandEvent& event); 
private:
    MainPlot *plot;
    bool in_onradiolabel;
    wxCheckBox *show_plabels;
    wxRadioBox *label_radio, *vertical_rb;
    wxTextCtrl *label_text;
    DECLARE_EVENT_TABLE()
};


#endif 
