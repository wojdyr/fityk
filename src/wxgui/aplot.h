// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_APLOT_H_
#define FITYK_WX_APLOT_H_

#include "plot.h"

enum AuxPlotKind
{
    apk_empty,
    apk_diff,
    apk_diff_stddev,
    apk_diff_y_perc,
    apk_cum_chi2
};

//Auxiliary plot, usually shows residuals or peak positions
class AuxPlot : public FPlot
{
    friend class AuxPlotConfDlg;
public:
    AuxPlot(wxWindow *parent, FPlot *master, wxString const& name);
    ~AuxPlot() {}
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
    void show_pref_dialog();

private:
    static const int move_plot_margin_width = 20;

    FPlot* master_;
    const wxString name_;
    AuxPlotKind kind_;
    bool mark_peak_pos_;
    bool reversed_diff_;
    double y_zoom_, y_zoom_base_;
    bool auto_zoom_y_;
    bool fit_y_once_;

    void OnPaint(wxPaintEvent &event);
    virtual void draw(wxDC &dc, bool monochrome=false);
    void OnLeaveWindow (wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent &event);
    void OnLeftDown(wxMouseEvent &event);
    void OnLeftUp(wxMouseEvent &event);
    void OnRightDown(wxMouseEvent &event);
    void OnMiddleDown(wxMouseEvent &event);
    // function called when Esc is pressed
    virtual void cancel_action() { cancel_mouse_left_press(); }
    void cancel_mouse_left_press();
    void set_scale(int pixel_width, int pixel_height);
    bool is_zoomable(); //false if kind is eg. empty or peak-position
    void OnPrefs(wxCommandEvent&) { show_pref_dialog(); }
    void OnPopupPlot(wxCommandEvent& event);
    void OnMarkPeakPositions(wxCommandEvent& event);
    void OnPopupYZoom(wxCommandEvent& event);
    void OnPopupYZoomFit(wxCommandEvent& event);
    void OnAutoZoom(wxCommandEvent& event);

    void draw_diff (wxDC& dc, std::vector<Point>::const_iterator first,
                                std::vector<Point>::const_iterator last);
    void draw_zoom_text(wxDC& dc, bool set_pen=true);
    void fit_y_zoom(fityk::Data const* data, fityk::Model const* model);
    void change_plot_kind(AuxPlotKind kind);

    DECLARE_EVENT_TABLE()
};

#endif
