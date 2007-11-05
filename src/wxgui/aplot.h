// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__WX_APLOT__H__
#define FITYK__WX_APLOT__H__

#include "plot.h"

enum Aux_plot_kind_enum 
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
public:
    AuxPlot(wxWindow *parent, FPlot *master_, wxString const& name_) 
        : FPlot(parent), master(master_), name(name_), 
          y_zoom(1.), y_zoom_base(1.), fit_y_once(false), cursor_id(-1) {}
    ~AuxPlot() {}
    void OnPaint(wxPaintEvent &event);
    void draw(wxDC &dc, bool monochrome=false);
    void OnLeaveWindow (wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent &event);
    void OnLeftDown (wxMouseEvent &event);
    void OnLeftUp (wxMouseEvent &event);
    void OnRightDown (wxMouseEvent &event);
    void OnMiddleDown (wxMouseEvent &event);
    void OnKeyDown (wxKeyEvent& event);
    bool cancel_mouse_left_press();
    void set_scale(int pixel_width, int pixel_height);
    bool is_zoomable(); //false if kind is eg. empty or peak-position
    void OnPopupPlot (wxCommandEvent& event);
    void OnPopupPlotCtr (wxCommandEvent& event);
    void OnPopupReversedDiff (wxCommandEvent& event);
    void OnPopupColor (wxCommandEvent& event);
    void OnPopupYZoom (wxCommandEvent& event);
    void OnPopupYZoomFit (wxCommandEvent& event);
    void OnPopupYZoomAuto (wxCommandEvent& event);
    void OnTicsFont (wxCommandEvent&) { change_tics_font(); }
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
    void set_cursor(int id) { if (id != cursor_id) SetCursor((cursor_id=id)); } 

private:
    FPlot* master;
    wxString name;
    Aux_plot_kind_enum kind;
    bool mark_peak_ctrs;
    bool reversed_diff;
    double y_zoom, y_zoom_base;
    bool auto_zoom_y;
    bool fit_y_once;
    int cursor_id;
    static const int move_plot_margin_width = 20;

    void draw_diff (wxDC& dc, std::vector<Point>::const_iterator first,
                                std::vector<Point>::const_iterator last);
    void draw_zoom_text(wxDC& dc, bool set_pen=true);
    void fit_y_zoom(Data const* data, Sum const* sum);

    DECLARE_EVENT_TABLE()
};

#endif
