// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__WX_MPLOT__H__
#define FITYK__WX_MPLOT__H__

#include "plot.h"
#include "cmn.h"
#include "../numfuncs.h" // PointQ definition
#include "../guess.h" // enum Guess::Kind


/// it cares about visualization of spline / polyline background
/// which can be set by selecting points on Plot

class Function;
class HintReceiver;

// operation started by pressing mouse button
enum MouseOperation
{
    // press-move-release operation
    kRectangularZoom,
    kVerticalZoom,
    kHorizontalZoom,
    kActivateSpan,
    kDisactivateSpan,
    kActivateRect,
    kDisactivateRect,
    kAddPeakInRange,
    kAddPeakTriangle,
    kDragPeak,
    // one-click operation
    kShowPlotMenu,
    kShowPeakMenu,
    kAddBgPoint,
    kDeleteBgPoint,
    // nothing
    kNoMouseOp
};

class BgManager
{
public:
    //minimal distance in X between bg points
    static const int min_dist = 8;

    BgManager(const Scale& x_scale);
    ~BgManager();
    void update_focused_data(int idx);
    void add_background_point(fp x, fp y);
    void rm_background_point(fp x);
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
    std::vector<int> calculate_bgline(int window_width, const Scale& y_scale);
    const std::vector<PointQ>& get_bg() const { return bg_; }
    bool stripped() const;
    const wxString& get_recent_bg_name(int n) const;
    void read_recent_baselines();
    void write_recent_baselines();

protected:
    const Scale& x_scale_;
    bool spline_;
    std::vector<PointQ> bg_;
    std::vector<std::pair<wxString, std::vector<PointQ> > > recent_bg_;
    std::vector<bool> stripped_;
    int data_idx_;

    std::string get_bg_name() const;
    void set_stripped(bool value);
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
        void set(const Function* p, int idx, drag_type how_, fp multiplier_);
        void change_value(fp x, fp dx, int dX);
        std::string get_cmd() const;
    };

    FunctionMouseDrag() : sidebar_dirty(false) {}
    void start(const Function* p, int X, int Y, fp x, fp y);
    void move(bool shift, int X, int Y, fp x, fp y);
    void stop();
    const std::vector<fp>& get_values() const { return values; }
    const std::string& get_status() const { return status; }
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
    bool bind_parameter_to_drag(Drag &drag, const std::string& name,
                          const Function* p, drag_type how, fp multiplier=1.);
    void set_drag(Drag &drag, const Function* p, int idx,
                  drag_type how, fp multiplier=1.);
};

/// main plot, single in application, displays data, fitted peaks etc.
class MainPlot : public FPlot
{
    friend class ConfigureAxesDlg;
    friend class ConfigurePLabelsDlg;
public:
    BgManager bgm;
    enum Kind { kLinear, kPeak };

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
    void OnConfigureAxes (wxCommandEvent& event);
    void OnConfigurePLabels (wxCommandEvent& event);
    void OnZoomAll (wxCommandEvent& event);
    void PeakInfo ();
    void OnPeakInfo (wxCommandEvent&) { PeakInfo(); }
    void OnPeakDelete (wxCommandEvent& event);
    void OnPeakGuess(wxCommandEvent &event);
    // function called when Esc is pressed
    virtual void cancel_action() { cancel_mouse_press(); }
    void cancel_mouse_press();
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
    void update_mouse_hints();
    void set_cursor();
    void set_mouse_mode(MouseModeEnum m);
    MouseModeEnum get_mouse_mode() const { return mode_; }
    const wxColour& get_data_color(int n) const
        { return dataCol[n % max_data_cols]; }
    const wxColour& get_func_color(int n) const
        { return peakCol[n % max_peak_cols]; }
    void set_data_color(int n, const wxColour& col)
        { dataCol[n % max_data_cols] = col; }
    void set_data_point_size(int /*n*/, int r) { point_radius = r; }
    void set_data_with_line(int /*n*/, bool b) { line_between_points = b; }
    void set_data_with_sigma(int /*n*/, bool b) { draw_sigma = b; }
    int get_data_point_size(int /*n*/) const { return point_radius; }
    bool get_data_with_line(int /*n*/) const { return line_between_points; }
    void set_func_color(int n, const wxColour& col)
        { peakCol[n % max_peak_cols] = col; }
    bool get_x_reversed() const { return x_reversed_; }
    void draw_xor_peak(const Function* func, const std::vector<fp>& p_values,
                       bool erase_previous);
    void redraw_xor_peak(bool clear=false);
    void show_popup_menu(wxMouseEvent &event);
    void set_hint_receiver(HintReceiver *hr)
        { hint_receiver_ = hr; update_mouse_hints(); }
    void set_auto_freeze(bool value) { auto_freeze_ = value; }

private:
    MouseModeEnum basic_mode_,
                  mode_;  ///actual mode -- either basic_mode_ or mmd_peak
    //static const int max_group_cols = 8;
    static const int max_peak_cols = 32;
    static const int max_data_cols = 64;
    bool peaks_visible_, /*groups_visible_,*/ model_visible_,
         plabels_visible_, x_reversed_;
    wxFont plabelFont;
    std::string plabel_format_;
    bool vertical_plabels_;
    std::vector<std::string> plabels_;
    wxColour modelCol, bg_pointsCol;
    //wxColour groupCol[max_group_cols];
    wxColour peakCol[max_peak_cols];
    wxColour dataCol[max_data_cols];
    int pressed_mouse_button_;
    MouseOperation mouse_op_;
    int over_peak_; /// the cursor is over peaktop of this peak
    int limit1_, limit2_; /// for drawing function limits (vertical lines)
    FunctionMouseDrag fmd_; //for function dragging
    Kind func_draft_kind_; // for function adding (with drawing draft)
    HintReceiver *hint_receiver_; // used to set mouse hints, probably statusbar
    // variables used in draw_xor_peak() and redraw_xor_peak()
    int draw_xor_peak_n_;
    wxPoint* draw_xor_peak_points_;
    bool auto_freeze_;

    void draw_x_axis (wxDC& dc, bool set_pen=true);
    void draw_y_axis (wxDC& dc, bool set_pen=true);
    void draw_background(wxDC& dc, bool set_pen=true);
    void draw_model (wxDC& dc, const Model* model, bool set_pen=true);
    //void draw_groups (wxDC& dc, const Model* model, bool set_pen=true);
    void draw_peaks (wxDC& dc, const Model* model, bool set_pen=true);
    void draw_peaktops (wxDC& dc, const Model* model);
    void draw_peaktop_selection(wxDC& dc, const Model* model);
    void draw_plabels (wxDC& dc, const Model* model, bool set_pen=true);
    void draw_dataset(wxDC& dc, int n, bool set_pen=true);
    void prepare_peaktops(const Model* model, int Ymax);
    void prepare_peak_labels(const Model* model);
    void look_for_peaktop (wxMouseEvent& event);
    void show_peak_menu (wxMouseEvent &event);
    void peak_draft (MouseActEnum ma, int X_=0, int Y_=0);
    bool draw_moving_func(MouseActEnum ma, int X=0, int Y=0, bool shift=false);
    void draw_peak_draft (int X_mid, int X_hwhm, int Y);
    void draw_temporary_rect(MouseActEnum ma, int X_=0, int Y_=0);
    void draw_rect (int X1, int Y1, int X2, int Y2);
    static bool visible_peaktops(MouseModeEnum mode);
    void add_peak_from_draft(int X, int Y);
    bool can_activate();
    MouseOperation what_mouse_operation(const wxMouseEvent& event);

    DECLARE_EVENT_TABLE()
};

class ConfigureAxesDlg: public wxDialog
{
public:
    ConfigureAxesDlg(wxWindow* parent, wxWindowID id, MainPlot* plot_);
    void OnApply (wxCommandEvent& event);
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
