// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_MPLOT_H_
#define FITYK_WX_MPLOT_H_

#include "plot.h"
#include "fityk/tplate.h" // Tplate::Kind

/// it cares about visualization of spline / polyline background
/// which can be set by selecting points on Plot

namespace fityk { class Function; }
class HintReceiver;
class BgManager;
class DraggedFunc;

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

/// main plot, single in application, displays data, fitted peaks etc.
class MainPlot : public FPlot
{
    friend class MainPlotConfDlg;
public:
    static const int kMaxDataColors = 256;

    MainPlot(wxWindow *parent);
    ~MainPlot();
    void OnPaint(wxPaintEvent &event);
    virtual void draw(wxDC &dc, bool monochrome=false);
    void OnLeaveWindow (wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent &event);
    void OnButtonDown (wxMouseEvent &event);
    void OnLeftDClick (wxMouseEvent&) { PeakInfo(); }
    void OnButtonUp (wxMouseEvent &event);
    void OnAuxDown(wxMouseEvent &event);
    void OnMouseWheel(wxMouseEvent &event);

    void OnConfigure(wxCommandEvent&);
    void OnZoomAll(wxCommandEvent& event);
    void PeakInfo();
    void OnPeakInfo(wxCommandEvent&) { PeakInfo(); }
    void OnPeakDelete(wxCommandEvent& event);
    void OnPeakGuess(wxCommandEvent &event);
    // function called when Esc is pressed
    virtual void cancel_action()
        { cancel_mouse_press(); overlay.draw_overlay(); }
    void cancel_mouse_press();
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
    void update_mouse_hints();
    void set_cursor();
    void switch_to_mode(MouseModeEnum m);
    void set_mode(MouseModeEnum m) { mode_ = m; }
    MouseModeEnum get_mouse_mode() const { return mode_; }
    const wxColour& get_data_color(int n) const
        { return data_colors_[n < (int) data_colors_.size() ? n : 0]; }
    const wxColour& get_func_color(int n) const
        { return peakCol[n % max_peak_cols]; }
    void set_data_color(int n, const wxColour& col);
    void set_data_point_size(int /*n*/, int r) { point_radius = r; }
    void set_data_with_line(int /*n*/, bool b) { line_between_points = b; }
    void set_data_with_sigma(int /*n*/, bool b) { draw_sigma = b; }
    int get_data_point_size(int /*n*/) const { return point_radius; }
    bool get_data_with_line(int /*n*/) const { return line_between_points; }
    void set_func_color(int n, const wxColour& col)
        { peakCol[n % max_peak_cols] = col; }
    bool get_x_reversed() const { return x_reversed_; }
    void show_popup_menu(wxMouseEvent &event);
    void set_hint_receiver(HintReceiver *hr)
        { hint_receiver_ = hr; update_mouse_hints(); }
    void set_auto_freeze(bool value) { auto_freeze_ = value; }
    BgManager* bgm() { return bgm_; }
    void draw_overlay_func(const fityk::Function* f,
                           const std::vector<realt>& p_values);
    void draw_overlay_limits(const fityk::Function* f);
    bool crosshair_cursor() const { return crosshair_cursor_; }
    void set_crosshair_cursor(bool c)
        { crosshair_cursor_ = c; overlay.switch_mode(get_normal_ovmode()); }
    Overlay::Mode get_normal_ovmode() const
        { return crosshair_cursor_ ? Overlay::kCrossHair : Overlay::kNone; }

private:
    BgManager* bgm_;
    DraggedFunc *dragged_func_; // for mouse-driven function dragging
    MouseModeEnum basic_mode_,
                  mode_;  ///actual mode -- either basic_mode_ or mmd_peak

    // plot properties stored in config
    //static const int max_group_cols = 8;
    static const int max_peak_cols = 32;
    bool peaks_visible_, /*groups_visible_,*/ model_visible_,
         plabels_visible_, desc_visible_, x_reversed_;
    wxFont plabelFont;
    std::string plabel_format_, desc_format_;
    bool vertical_plabels_;
    wxColour modelCol, bg_pointsCol;
    int model_line_width_;
    //wxColour groupCol[max_group_cols];
    wxColour peakCol[max_peak_cols];
    std::vector<wxColour> data_colors_;
    bool crosshair_cursor_;

    std::vector<std::string> plabels_; // peak labels
    int pressed_mouse_button_;
    MouseOperation mouse_op_;
    int over_peak_; /// the cursor is over peaktop of this peak
    fityk::Tplate::Kind func_draft_kind_; // for function adding (drawing draft)
    HintReceiver *hint_receiver_; // used to set mouse hints, probably statusbar
    bool auto_freeze_;

    void draw_x_axis (wxDC& dc, bool set_pen=true);
    void draw_y_axis (wxDC& dc, bool set_pen=true);
    void draw_baseline(wxDC& dc, bool set_pen=true);
    void draw_model (wxDC& dc, const fityk::Model* model, bool set_pen=true);
    //void draw_groups (wxDC& dc, const fityk::Model* model, bool set_pen=true);
    void draw_peaks (wxDC& dc, const fityk::Model* model, bool set_pen=true);
    void draw_peaktops (wxDC& dc, const fityk::Model* model);
    void draw_peaktop_selection(wxDC& dc, const fityk::Model* model);
    void draw_desc(wxDC& dc, int dataset, bool set_pen=true);
    void draw_plabels (wxDC& dc, const fityk::Model* model, bool set_pen=true);
    void draw_dataset(wxDC& dc, int n, bool set_pen=true);
    void prepare_peaktops(const fityk::Model* model, int Ymax);
    void prepare_peak_labels(const fityk::Model* model);
    void look_for_peaktop (wxMouseEvent& event);
    void show_peak_menu (wxMouseEvent &event);
    static bool visible_peaktops(MouseModeEnum mode);
    void add_peak_from_draft(int X, int Y);
    bool can_activate();
    MouseOperation what_mouse_operation(const wxMouseEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif
