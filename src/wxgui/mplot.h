// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__WX_MPLOT__H__
#define FITYK__WX_MPLOT__H__

#include "plot.h"
#include "cmn.h"
#include "../numfuncs.h" // PointQ definition
#include "../guess.h" // enum FunctionKind 


/// it cares about visualization of spline / polyline background 
/// which can be set by selecting points on Plot

class Function;

class BgManager
{
public:
    typedef std::vector<PointQ>::iterator bg_iterator;
    typedef std::vector<PointQ>::const_iterator bg_const_iterator;

    BgManager(Scale const& x_scale_) : x_scale(x_scale_), min_dist(8), 
                                     spline_bg(true) {}
    void add_background_point(fp x, fp y);
    void rm_background_point(fp x);
    void clear_background();
    void forget_background() { clear_background(); bg_backup.clear(); }
    void strip_background();
    void undo_strip_background();
    bool can_strip() const { return !bg.empty(); }
    bool can_undo() const { return !bg_backup.empty(); }
    void set_spline_bg(bool s) { spline_bg=s; }
    void set_as_convex_hull();
    std::vector<int> calculate_bgline(int window_width);
    bg_const_iterator begin() const { return bg.begin(); }
    bg_const_iterator end() const { return bg.end(); }
protected:
    Scale const& x_scale;
    int min_dist; //minimal distance in X between bg points
    bool spline_bg;
    std::vector<PointQ> bg, bg_backup;
    std::string cmd_tail;
};


class ConfigureAxesDlg;

/// utility used in MainPlot for dragging function
class FunctionMouseDrag
{
public:
    enum drag_type { 
        no_drag, 
        relative_value, //eg. for area
        absolute_value,  //eg. for width
        absolute_pixels 
    };

    class Drag 
    {
    public:
        drag_type how;
        int parameter_idx;
        std::string parameter_name;
        std::string variable_name; /// name of variable that are to be changed 
        fp value; /// current value of parameter
        fp ini_value; /// initial value of parameter
        fp multiplier; /// increases or decreases changing rate
        fp ini_x;

        Drag() : how(no_drag) {}
        void set(Function const* p, int idx, drag_type how_, fp multiplier_);
        void change_value(fp x, fp dx, int dX);
        std::string get_cmd() const {
            return how != no_drag && value != ini_value ?
                "$" + variable_name + " = ~" + S(value) + "; " : "";
        }
    };

    FunctionMouseDrag() : sidebar_dirty(false) {}
    void start(Function const* p, int X, int Y, fp x, fp y);
    void move(bool shift, int X, int Y, fp x, fp y);
    void stop();
    std::vector<fp> const& get_values() const { return values; }
    std::string const& get_status() const { return status; }
    std::string get_cmd() const;

private:
    Drag drag_x; ///for horizontal dragging (x axis)
    Drag drag_y; /// y axis
    Drag drag_shift_x; ///x with [shift]
    Drag drag_shift_y; ///y with [shift]
    fp px, py;
    int pX, pY;
    std::vector<fp> values;
    std::string status;
    bool sidebar_dirty;

    void set_defined_drags();
    bool bind_parameter_to_drag(Drag &drag, std::string const& name,
                          Function const* p, drag_type how, fp multiplier=1.);
    void set_drag(Drag &drag, Function const* p, int idx,
                  drag_type how, fp multiplier=1.);
};

/// main plot, single in application, displays data, fitted peaks etc. 
class MainPlot : public FPlot
{
    friend class ConfigureAxesDlg;
    friend class ConfigurePLabelsDlg;
public:
    BgManager bgm;

    MainPlot(wxWindow *parent); 
    ~MainPlot() {}
    void OnPaint(wxPaintEvent &event);
    void draw(wxDC &dc, bool monochrome=false);
    void OnLeaveWindow (wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent &event);
    void OnButtonDown (wxMouseEvent &event);
    void OnLeftDClick (wxMouseEvent&) { PeakInfo(); }
    void OnButtonUp (wxMouseEvent &event);
    void OnKeyDown (wxKeyEvent& event);

    void OnPopupShowXX (wxCommandEvent& event);
    void OnPopupColor (wxCommandEvent& event);
    void OnInvertColors (wxCommandEvent& event);
    void OnPopupRadius (wxCommandEvent& event);
    void OnConfigureAxes (wxCommandEvent& event);
    void OnConfigurePLabels (wxCommandEvent& event);
    void OnZoomAll (wxCommandEvent& event);
    void PeakInfo ();
    void OnPeakInfo (wxCommandEvent&) { PeakInfo(); }
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
    void set_data_point_size(int /*n*/, int r) { point_radius = r; }
    void set_data_with_line(int /*n*/, bool b) { line_between_points = b; }
    void set_data_with_sigma(int /*n*/, bool b) { draw_sigma = b; }
    int get_data_point_size(int /*n*/) const { return point_radius; }
    bool get_data_with_line(int /*n*/) const { return line_between_points; }
    void set_func_color(int n, wxColour const& col) 
        { peakCol[n % max_peak_cols] = col; }
    bool get_x_reversed() const { return x_reversed; }
    void draw_xor_peak(Function const* func, std::vector<fp> const& p_values);
    void show_popup_menu(wxMouseEvent &event);

private:
    MouseModeEnum basic_mode, 
                  mode;  ///actual mode -- either basic_mode or mmd_peak
    static const int max_group_cols = 8;
    static const int max_peak_cols = 32;
    static const int max_data_cols = 64;
    static const int max_radius = 4; ///size of data point
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
    bool ctrl_on_down;
    bool shift_on_down;
    int over_peak; /// the cursor is over peaktop of this peak
    int limit1, limit2; /// for drawing function limits (vertical lines)
    FunctionMouseDrag fmd; //for function dragging
    FunctionKind func_draft_kind; // for function adding (with drawing draft)

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
    void prepare_peaktops(Sum const* sum, int Ymax);
    void prepare_peak_labels(Sum const* sum);
    void look_for_peaktop (wxMouseEvent& event);
    void show_peak_menu (wxMouseEvent &event);
    void peak_draft (MouseActEnum ma, int X_=0, int Y_=0);
    bool draw_moving_func(MouseActEnum ma, int X=0, int Y=0, bool shift=false);
    void draw_peak_draft (int X_mid, int X_hwhm, int Y);
    void draw_temporary_rect(MouseActEnum ma, int X_=0, int Y_=0);
    void draw_rect (int X1, int Y1, int X2, int Y2);
    bool visible_peaktops(MouseModeEnum mode);
    void add_peak_from_draft(int X, int Y);
    bool can_disactivate();

    DECLARE_EVENT_TABLE()
};

class ConfigureAxesDlg: public wxDialog
{
public:
    ConfigureAxesDlg(wxWindow* parent, wxWindowID id, MainPlot* plot_);
    void OnApply (wxCommandEvent& event);
    void OnClose (wxCommandEvent&) { close_it(this); }
    void OnChangeColor (wxCommandEvent&) { change_color_dlg(axis_color); }
    void OnChangeFont (wxCommandEvent& event); 
private:
    MainPlot *plot;
    wxColour axis_color;
    wxCheckBox *x_show_axis, *x_show_tics, *x_show_minor_tics, 
               *x_show_grid, *x_reversed_cb, *x_logarithm_cb; 
    wxCheckBox *y_show_axis, *y_show_tics, *y_show_minor_tics, 
               *y_show_grid, *y_reversed_cb, *y_logarithm_cb; 
    wxSpinCtrl *x_max_tics, *x_tics_size;
    wxSpinCtrl *y_max_tics, *y_tics_size;
    DECLARE_EVENT_TABLE()
};


class ConfigurePLabelsDlg: public wxDialog
{
public:
    ConfigurePLabelsDlg(wxWindow* parent, wxWindowID id, MainPlot* plot_);
    void OnApply (wxCommandEvent& event);
    void OnClose (wxCommandEvent&) { close_it(this); }
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
