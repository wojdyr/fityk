// This file is part of fityk program. Copyright (C) Marcin Wojdyr

// wxwindows headers, see wxwindows samples for description
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "common.h"
RCSID ("$Id$")

#include <wx/laywin.h>
#include <wx/sashwin.h>
#include <wx/colordlg.h>
#include <wx/gdicmn.h>
#include <wx/statline.h>
#include <algorithm>
#include <numeric>
#include <vector>
#include <map>

#include "wx_plot.h"
#include "wx_gui.h"
#include "wx_dlg.h"
#include "wx_pane.h"
#include "data.h"
#include "sum.h"
#include "pcore.h"
#include "other.h" 
#include "ffunc.h"
#include "manipul.h"
#include "ui.h"
#ifdef USE_XTAL
    #include "crystal.h"
#endif //USE_XTAL

using namespace std;

enum {
    ID_plot_popup_za                = 25001, 
    ID_plot_popup_data              = 25011,
    ID_plot_popup_sum                      ,
    ID_plot_popup_phase                    ,
    ID_plot_popup_peak                     ,
    ID_plot_popup_xaxis                    ,
    ID_plot_popup_tics                     ,
    ID_plot_popup_smooth                   ,

    ID_plot_popup_c_background             ,
    ID_plot_popup_c_active_data            ,
    ID_plot_popup_c_inactive_data          ,
    ID_plot_popup_c_sum                    ,
    ID_plot_popup_c_xaxis                  ,
    ID_plot_popup_c_phase_0         = 25100,
    ID_plot_popup_c_peak_0          = 25140,
    ID_plot_popup_c_phase           = 25190,
    ID_plot_popup_c_peak                   ,
    ID_plot_popup_c_inv                    ,

    ID_plot_popup_pt_size           = 25210,// and next 10 ,
    ID_peak_popup_info              = 25250,
    ID_peak_popup_del                      ,
    ID_peak_popup_tree                     ,
    ID_peak_popup_guess                    ,

    ID_aux_popup_plot_0            = 25310,
    ID_aux_popup_c_background      = 25340,
    ID_aux_popup_c_active_data            ,
    ID_aux_popup_c_inactive_data          ,
    ID_aux_popup_c_axis                   ,
    ID_aux_popup_color                    ,
    ID_aux_popup_yz_fit                   ,
    ID_aux_popup_yz_change                ,
    ID_aux_popup_yz_auto                  
};

fp scale_tics_step (fp beg, fp end, int max_tics);

//===============================================================
//                FPlot (plot with data and fitted curves) 
//===============================================================

void FPlot::draw_dashed_vert_lines (int x1, int x2)
{
    if (x1 != INVALID || x2 != INVALID) {
        wxClientDC dc(this);
        dc.SetLogicalFunction (wxINVERT);
        dc.SetPen(*wxBLACK_DASHED_PEN);
        int h = GetClientSize().GetHeight();
        if (x1 != INVALID)
            dc.DrawLine (x1, 0, x1, h);
        if (x2 != INVALID)
            dc.DrawLine (x2, 0, x2, h);
    }
}

void FPlot::draw_crosshair(int X, int Y)
{
    wxClientDC dc(this);
    dc.SetLogicalFunction (wxINVERT);
    dc.SetPen(*wxBLACK_DASHED_PEN);
    dc.CrossHair(X, Y);
}

bool FPlot::vert_line_following_cursor (Mouse_act_enum ma, int x, int x0)
{
    static int prev_x = INVALID, prev_x0 = INVALID;

    if (ma == mat_start) {
        draw_dashed_vert_lines(x0);
        prev_x0 = x0;
    }
    else {
        if (prev_x == INVALID) return false;
        draw_dashed_vert_lines(prev_x); //clear old line
    }
    if (ma == mat_move || ma == mat_start) {
        draw_dashed_vert_lines(x);
        prev_x = x;
    }
    else { // mat_stop, mat_cancel:
        draw_dashed_vert_lines(prev_x0); //clear
        prev_x = prev_x0 = INVALID;
    }
    return true;
}

BEGIN_EVENT_TABLE(FPlot, wxPanel)
END_EVENT_TABLE()


inline fp sum_value(vector<Point>::const_iterator pt)
{
    return my_sum->value(pt->x);
}

//===============================================================
//                MainPlot (plot with data and fitted curves) 
//===============================================================
BEGIN_EVENT_TABLE(MainPlot, FPlot)
    EVT_PAINT (           MainPlot::OnPaint)
    EVT_MOTION (          MainPlot::OnMouseMove)
    EVT_LEAVE_WINDOW (    MainPlot::OnLeaveWindow)
    EVT_LEFT_DOWN (       MainPlot::OnButtonDown)
    EVT_RIGHT_DOWN (      MainPlot::OnButtonDown)
    EVT_MIDDLE_DOWN (     MainPlot::OnButtonDown)
    EVT_LEFT_DCLICK (     MainPlot::OnLeftDClick)
    EVT_LEFT_UP   (       MainPlot::OnButtonUp)
    EVT_RIGHT_UP (        MainPlot::OnButtonUp)
    EVT_MIDDLE_UP (       MainPlot::OnButtonUp)
    EVT_KEY_DOWN   (      MainPlot::OnKeyDown)
    EVT_MENU (ID_plot_popup_za,     MainPlot::OnZoomAll)
    EVT_MENU_RANGE (ID_plot_popup_data, ID_plot_popup_smooth,  
                                    MainPlot::OnPopupShowXX)
    EVT_MENU_RANGE (ID_plot_popup_c_background, ID_plot_popup_c_phase - 1, 
                                    MainPlot::OnPopupColor)
    EVT_MENU_RANGE (ID_plot_popup_pt_size, ID_plot_popup_pt_size + max_radius, 
                                    MainPlot::OnPopupRadius)
    EVT_MENU (ID_plot_popup_c_inv,  MainPlot::OnInvertColors)
    EVT_MENU (ID_peak_popup_info,   MainPlot::OnPeakInfo)
    EVT_MENU (ID_peak_popup_del,    MainPlot::OnPeakDelete)
    EVT_MENU (ID_peak_popup_tree,   MainPlot::OnPeakShowTree)
    EVT_MENU (ID_peak_popup_guess,  MainPlot::OnPeakGuess)
END_EVENT_TABLE()

MainPlot::MainPlot (wxWindow *parent, Plot_shared &shar) 
    : FPlot (parent, shar), mode (mmd_zoom), basic_mode(mmd_zoom),
      pressed_mouse_button(0), ctrl(false), over_peak(-1)
{ }

void MainPlot::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    vert_line_following_cursor(mat_cancel);//erase XOR lines before repainting
    frame->draw_crosshair(-1, -1); 

    wxPaintDC dc(this);
    if (backgroundBrush.Ok())
        dc.SetBackground (backgroundBrush);
    dc.Clear();
    Draw(dc);
    frame->SetTitle (wxString("fityk ") + (my_data->is_empty() ? "" 
                                : ("- " + my_data->get_filename()).c_str()));
}

fp y_of_data_for_draw_data(vector<Point>::const_iterator i)
{
    return my_core->plus_background ? i->orig_y : i->y;
}


void MainPlot::Draw(wxDC &dc)
{
    set_scale();

    frame->draw_crosshair(-1, -1); //erase crosshair before redrawing plot

    if (!params4plot.empty() && size(params4plot) != my_sum->pars()->count_a()){
        params4plot = my_sum->pars()->values();
        frame->SetStatusText ("Number of parameters changed.");
    }
    my_sum->use_param_a_for_value (params4plot);
    prepare_peaktops();

    if (colourTextForeground.Ok())
        dc.SetTextForeground (colourTextForeground);
    if (colourTextBackground.Ok())
        dc.SetTextBackground (colourTextBackground);

    if (data_visible) {
        //TODO changable colors for each dataset
        wxPen unsel_active(activeDataPen);
        wxPen unsel_inactive(inactiveDataPen);
        unsel_active.SetColour(0, 90, 0);
        unsel_inactive.SetColour(30, 60, 30);
        for (int i = 0; i < my_core->get_data_count(); i++) {
            if (i != my_core->get_active_data_position())
                draw_data(dc, y_of_data_for_draw_data, 
                          my_core->get_data(i), &unsel_active, &unsel_inactive);
        }
        draw_data(dc, y_of_data_for_draw_data);
    }

    if (tics_visible)
        draw_tics(dc, my_core->view, 7, 7, 4, 4);

    if (my_data->is_empty())
        return;

    vector<Point>::const_iterator f =my_data->get_point_at(my_core->view.left),
                                l = my_data->get_point_at(my_core->view.right);
    if (l != my_data->points().end())
        ++l;

    if (peaks_visible)
        draw_peaks (dc, f, l);
    if (phases_visible)
        draw_phases (dc, f, l);
    if (sum_visible)
        draw_sum (dc, f, l);
    if (x_axis_visible) 
        draw_x_axis (dc, f, l);

    if (mode == mmd_bg) {
        draw_background_points(dc); 
    }
    else if (visible_peaktops(mode)) {
        draw_peaktops(dc); 
    }
}


bool MainPlot::visible_peaktops(Mouse_mode_enum mode)
{
    return (mode == mmd_zoom || mode == mmd_add || mode == mmd_peak);
}

void MainPlot::draw_x_axis (wxDC& dc, vector<Point>::const_iterator first,
                                      vector<Point>::const_iterator last)
{
    dc.SetPen (xAxisPen);
    if (my_core->plus_background) {
        int X = 0, Y = 0;
        for (vector<Point>::const_iterator i = first; i < last; i++) {
            int X_ = X, Y_ = Y;
            X = x2X(i->x);
            Y = y2Y (i->get_bg());
            if (i != first)
                dc.DrawLine (X_, Y_, X, Y); 
        }
    }
    else 
        dc.DrawLine (0, y2Y(0), GetClientSize().GetWidth(), y2Y(0));
}

void FPlot::draw_tics (wxDC& dc, const Rect &v, 
                          const int x_max_tics, const int y_max_tics, 
                          const int x_tic_size, const int y_tic_size)
{
    dc.SetPen (xAxisPen);
    dc.SetFont(*wxSMALL_FONT);
    dc.SetTextForeground(xAxisPen.GetColour());

    // x axis tics
    fp x_tic_step = scale_tics_step(v.left, v.right, x_max_tics);
    for (fp x = x_tic_step * ceil(v.left / x_tic_step); x < v.right; 
            x += x_tic_step) {
        int X = x2X(x);
        int Y = y2Y(0);
        dc.DrawLine (X, Y, X, Y - x_tic_size);
        wxString label = S(x).c_str();
        wxCoord w, h;
        dc.GetTextExtent (label, &w, &h);
        dc.DrawText (label, X - w/2, Y + 1);
    }

    // y axis tics
    fp y_tic_step = scale_tics_step(v.bottom, v.top, y_max_tics);
    for (fp y = y_tic_step * ceil(v.bottom / y_tic_step); y < v.top; 
            y += y_tic_step) {
        int X = 0;
        int Y = y2Y(y);
        dc.DrawLine (X, Y, X + y_tic_size, Y);
        wxString label = S(y).c_str();
        wxCoord w, h;
        dc.GetTextExtent (label, &w, &h);
        dc.DrawText (label, X + y_tic_size + 1, Y - h/2);
    }
}

fp FPlot::get_max_abs_y (fp (*compute_y)(vector<Point>::const_iterator))
{
    vector<Point>::const_iterator 
                            first = my_data->get_point_at(my_core->view.left),
                            last = my_data->get_point_at(my_core->view.right);
    fp max_abs_y = 0;
    for (vector<Point>::const_iterator i = first; i < last; i++) {
        if (i->is_active) {
            fp y = fabs(((*compute_y)(i)));
            if (y > max_abs_y) max_abs_y = y;
        }
    }
    return max_abs_y;
}

void FPlot::draw_data (wxDC& dc, 
                       fp (*compute_y)(vector<Point>::const_iterator),
                       const Data* dat, 
                       const wxPen *active_p, const wxPen *inactive_p)
{
    const Data *data = dat ? dat : my_data;
    const wxPen &activePen = active_p ? *active_p : activeDataPen;
    const wxPen &inactivePen = inactive_p ? *inactive_p : inactiveDataPen;
    const wxBrush activeBrush(activePen.GetColour(), wxSOLID);
    const wxBrush inactiveBrush(inactivePen.GetColour(), wxSOLID);
    if (data->is_empty()) return;
    vector<Point>::const_iterator 
                            first = data->get_point_at(my_core->view.left),
                            last = data->get_point_at(my_core->view.right);
    //if (last - first < 0) return;
    bool active = !first->is_active;//causes pens to be initialized in main loop
    int X_ = 0, Y_ = 0;
    // first line segment -- lines should be drawed towards points 
    //                                                 that are outside of plot 
    if (line_between_points) {
        dc.SetPen(first->is_active ? activePen : inactivePen);
        if (first > data->points().begin()) {
            X_ = x2X (my_core->view.left);
            int Y_l = y2Y ((*compute_y)(first - 1));
            int Y_r = y2Y ((*compute_y)(first));
            int X_l = x2X ((first - 1)->x);
            int X_r = x2X (first->x);
            if (X_r == X_l)
                Y_ = Y_r;
            else
                Y_ = Y_l + (Y_r - Y_l) * (X_ - X_l) / (X_r - X_l);
        }
        else {
            X_ = x2X(first->x);
            Y_ = y2Y ((*compute_y)(first));
        }
    }

    //drawing all points (and lines); main loop
    for (vector<Point>::const_iterator i = first; i < last; i++) {
        int X = x2X(i->x);
        int Y = y2Y ((*compute_y)(i));
        if (X == X_ && Y == Y_) continue;
        if (i->is_active != active) {
            active = i->is_active;
            if (active) {
                dc.SetPen (activePen);
                dc.SetBrush (activeBrush);
            }
            else {
                dc.SetPen (inactivePen);
                dc.SetBrush (inactiveBrush);
            }
            //half of line between points should be active and half not.
            //draw first half here and change X_, Y_; the rest will be drawed
            //as usually.
            if (line_between_points) {
                int X_mid = (X_ + X) / 2, Y_mid = (Y_ + Y) / 2;
                dc.DrawLine (X_, Y_, X_mid, Y_mid);
                X_ = X_mid, Y_ = Y_mid;
            }
        }
        if (point_radius > 1) 
            dc.DrawCircle (X, Y, point_radius - 1);
        if (line_between_points) {
            dc.DrawLine (X_, Y_, X, Y);
            X_ = X, Y_ = Y;
        }
        else {//no line_between_points
            if (point_radius == 1)
                dc.DrawPoint (X, Y);
        }
    }

    //the last line segment, toward next point
    if (line_between_points && last < data->points().end()) {
        int X = x2X (my_core->view.right);
        int Y_l = y2Y ((*compute_y)(last - 1));
        int Y_r = y2Y ((*compute_y)(last));
        int X_l = x2X ((last - 1)->x);
        int X_r = x2X (last->x);
        if (X_r != X_l) {
            int Y = Y_l + (Y_r - Y_l) * (X_ - X_l) / (X_r - X_l);
            dc.DrawLine (X_, Y_, X, Y);
        }
    }
}

void MainPlot::buffer_peaks (vector<Point>::const_iterator /*first*/,
                             vector<Point>::const_iterator /*last*/)
{
    /*
    //TODO
    //bool function_as_points = (i_l - i_f > smooth_limit);
    int p = my_sum->fzg_size(fType);
    shared.buf = vector<vector<fp> > (p + 1, vector<fp>(last - first));
    int j = 0;
    for (vector<Point>::const_iterator i = first; i < last; i++, j++) {
        shared.buf[0][j] = i->x;
        for (int k = 0; k < my_sum->fzg_size(fType); k++) 
            shared.buf[k][j] = my_sum->f_value(i->x, k);
    }
    */
}


void MainPlot::draw_sum(wxDC& dc, vector<Point>::const_iterator first,
                                  vector<Point>::const_iterator last)
{
    dc.SetPen (sumPen);
    int X = -1, Y = -1;
    for (vector<Point>::const_iterator i = first; i < last; i++) {
        int X_ = X, Y_ = Y;
        X = x2X(i->x);
        Y = y2Y(my_sum->value(i->x) 
                + (my_core->plus_background && smooth ? i->get_bg() : 0));
        if (smooth)
            while (X_ < X-1) {
                ++X_;
                int Y_p = Y_;
                Y_ = y2Y(my_sum->value(X2x(X_)));
                if (X_ > 0)
                    dc.DrawLine (X_-1, Y_p, X_, Y_); 
            }
        if (X_ >= 0 && (X != X_ || Y != Y_)) 
            dc.DrawLine (X_, Y_, X, Y); 
    }
}


void MainPlot::draw_phases (wxDC& dc, vector<Point>::const_iterator first,
                                      vector<Point>::const_iterator last)
                            
{
#ifdef USE_XTAL
    for (int k = 0; k < my_crystal->get_nr_of_phases(); k++) {
        dc.SetPen (phasePen[k % max_phase_pens]);
        vector<int> peaks = my_crystal->get_funcs_in_phase(k);
        int X = -1, Y = -1;
        for (vector<Point>::const_iterator i = first; i < last; i++) {
            int X_ = X, Y_ = Y;
            X = x2X(i->x);
            Y = y2Y(my_sum->funcs_value (peaks, i->x));
            if (smooth)
                while (X_ < X-1) {
                    ++X_;
                    int Y_p = Y_;
                    Y_ = y2Y(my_sum->funcs_value (peaks, X2x(X_)));
                    if (X_ > 0)
                        dc.DrawLine (X_-1, Y_p, X_, Y_); 
                }
            if (X_ >= 0 && (X != X_ || Y != Y_)) 
                dc.DrawLine (X_, Y_, X, Y); 
        }
    }
#endif //USE_XTAL
}

void MainPlot::draw_peaks (wxDC& dc, vector<Point>::const_iterator first,
                                     vector<Point>::const_iterator last)
{
    for (int k = 0; k < my_sum->fzg_size(fType); k++) {
        dc.SetPen (peakPen[k % max_peak_pens]);
        int X = -1, Y = -1;
        for (vector<Point>::const_iterator i = first; i < last; i++) {
            int X_ = X, Y_ = Y;
            X = x2X(i->x);
            Y = y2Y(my_sum->f_value (i->x, k));
            if (smooth)
                while (X_ < X-1) {
                    ++X_;
                    int Y_p = Y_;
                    Y_ = y2Y(my_sum->f_value (X2x(X_), k));
                    if (X_ > 0)
                        dc.DrawLine (X_-1, Y_p, X_, Y_); 
                }
            if (X_ >= 0 && (X != X_ || Y != Y_)) 
                dc.DrawLine (X_, Y_, X, Y); 
        }
    }
}

void MainPlot::draw_peaktops (wxDC& dc)
{
    dc.SetPen (xAxisPen);
    dc.SetBrush (*wxTRANSPARENT_BRUSH);
    for (vector<wxPoint>::const_iterator i = shared.peaktops.begin(); 
         i != shared.peaktops.end(); i++) 
        dc.DrawRectangle (i->x - 1, i->y - 1, 3, 3);
}


/*
static bool operator< (const wxPoint& a, const wxPoint& b) 
{ 
    return a.x != b.x ? a.x < b.x : a.y < b.y; 
}
*/

void MainPlot::prepare_peaktops()
{
    int H =  GetClientSize().GetHeight();
    int Y0 = y2Y(0);
    shared.peaktops.clear();
    for (int k = 0; k < my_sum->fzg_size(fType); k++) {
        const V_f *f = my_sum->get_f(k);
        int X, Y;
        if (f->is_peak()) {
            fp x = f->center();
            X = x2X (x - my_sum->zero_shift(x));
            Y = y2Y (f->height());
        }
        else {
            X = k * 10;
            Y = y2Y (my_sum->f_value(X2x(X), k));
        }
        if (Y < 0 || Y > H) Y = Y0;
        shared.peaktops.push_back(wxPoint(X, Y));
    }
}


void MainPlot::draw_background_points (wxDC& dc)
{
    const vector<B_point>& bg = my_data->get_background_points(bgc_bg);
    dc.SetPen (bg_pointsPen);
    dc.SetBrush (*wxTRANSPARENT_BRUSH);
    for (vector<B_point>::const_iterator i = bg.begin(); i != bg.end(); i++) {
        dc.DrawCircle (x2X(i->x), y2Y(i->y), 3);
    }
}

void MainPlot::read_settings(wxConfigBase *cf)
{
    cf->SetPath("/MainPlot/Colors");
    colourTextForeground = read_color_from_config (cf, "text_fg", 
                                                   wxColour("BLACK"));
    colourTextBackground = read_color_from_config (cf, "text_bg", 
                                                   wxColour ("LIGHT GREY"));
    backgroundBrush.SetColour (read_color_from_config (cf, "bg", 
                                                       wxColour("BLACK")));
    wxColour active_data_col = read_color_from_config (cf, "active_data",
                                                       wxColour ("GREEN"));
    activeDataPen.SetColour (active_data_col);
    //activeDataPen.SetStyle (wxSOLID);
    wxColour inactive_data_col = read_color_from_config (cf, "inactive_data",
                                                      wxColour (128, 128, 128));
    inactiveDataPen.SetColour (inactive_data_col);
    sumPen.SetColour (read_color_from_config (cf, "sum", wxColour("YELLOW")));
    xAxisPen.SetColour (read_color_from_config(cf, "xAxis", wxColour("WHITE")));
    bg_pointsPen.SetColour (read_color_from_config (cf, "BgPoints", 
                                                    wxColour("RED")));
    vector<wxColour> default_phase_col;
    default_phase_col.push_back (wxColour("CYAN"));
    default_phase_col.push_back (wxColour("RED"));
    default_phase_col.push_back (wxColour("LIGHT STEEL BLUE"));
    for (int i = 0; i < max_phase_pens; i++)
        phasePen[i].SetColour (read_color_from_config (cf, 
                                                      ("phase/" + S(i)).c_str(),
                              default_phase_col[i % default_phase_col.size()]));
    vector<wxColour> default_peak_col;
    default_peak_col.push_back (wxColour (0, 128, 128));
    default_peak_col.push_back (wxColour (0, 0, 128));
    default_peak_col.push_back (wxColour (128, 0, 128));
    default_peak_col.push_back (wxColour (128, 128, 0));
    default_peak_col.push_back (wxColour (128, 0, 0));
    default_peak_col.push_back (wxColour (0, 128, 0));
    default_peak_col.push_back (wxColour (0, 128, 128));
    default_peak_col.push_back (wxColour (0, 0, 128));
    default_peak_col.push_back (wxColour (128, 0, 128));
    default_peak_col.push_back (wxColour (128, 128, 0));
    for (int i = 0; i < max_peak_pens; i++)
        peakPen[i].SetColour (read_color_from_config (cf, 
                                                      ("peak/" + S(i)).c_str(),
                                default_peak_col[i % default_peak_col.size()]));
                            
    cf->SetPath("/MainPlot/Visible");
    smooth = read_bool_from_config(cf, "smooth", false);
    peaks_visible = read_bool_from_config (cf, "peaks", false); 
    phases_visible = read_bool_from_config (cf, "phases", false);  
    sum_visible = read_bool_from_config (cf, "sum", true);
    data_visible = read_bool_from_config (cf, "data", true); 
    x_axis_visible = read_bool_from_config (cf, "xAxis", true);  
    tics_visible = read_bool_from_config (cf, "tics", true);
    cf->SetPath("/MainPlot");
    point_radius = cf->Read ("point_radius", 1);
    line_between_points = read_bool_from_config(cf,"line_between_points",false);
    Refresh();
}

void MainPlot::save_settings(wxConfigBase *cf) const
{
    cf->SetPath("/MainPlot");
    cf->Write ("point_radius", point_radius);
    cf->Write ("line_between_points", line_between_points);

    cf->SetPath("/MainPlot/Colors");
    write_color_to_config (cf, "text_fg", colourTextForeground);
    write_color_to_config (cf, "text_bg", colourTextBackground);
    write_color_to_config (cf, "bg", backgroundBrush.GetColour());
    write_color_to_config (cf, "active_data", activeDataPen.GetColour());
    write_color_to_config (cf, "inactive_data", inactiveDataPen.GetColour());
    write_color_to_config (cf, "sum", sumPen.GetColour());
    write_color_to_config (cf, "xAxis", xAxisPen.GetColour());
    write_color_to_config (cf, "BgPoints", bg_pointsPen.GetColour());
    for (int i = 0; i < max_phase_pens; i++)
        write_color_to_config (cf, ("phase/" + S(i)).c_str(), 
                               phasePen[i].GetColour());
    for (int i = 0; i < max_peak_pens; i++)
        write_color_to_config (cf, ("peak/" + S(i)).c_str(), 
                               peakPen[i].GetColour());

    cf->SetPath("/MainPlot/Visible");
    cf->Write ("smooth", smooth);
    cf->Write ("peaks", peaks_visible);
    cf->Write ("phases", phases_visible);
    cf->Write ("sum", sum_visible);
    cf->Write ("data", data_visible);
    cf->Write ("xAxis", x_axis_visible);
    cf->Write ("tics", tics_visible);
}

void MainPlot::OnLeaveWindow (wxMouseEvent& WXUNUSED(event))
{
    frame->SetStatusText ("", sbf_coord);
    vert_line_following_cursor (mat_cancel);
    frame->draw_crosshair(-1, -1);
}

void MainPlot::set_scale()
{
    const Rect &v = my_core->view;
    shared.xUserScale = GetClientSize().GetWidth() / (v.right - v.left);
    int H =  GetClientSize().GetHeight();
    fp h = v.top - v.bottom;
    fp label_width = tics_visible && v.bottom <= 0 ?max(v.bottom * H/h + 12, 0.)
                                                   : 0;
    yUserScale = - (H - label_width) / h;
    shared.xLogicalOrigin = v.left;
    yLogicalOrigin = v.top;
    shared.plot_y_scale = yUserScale;
}
 
void MainPlot::show_popup_menu (wxMouseEvent &event)
{
    wxMenu popup_menu; //("main plot menu");

    popup_menu.Append(ID_plot_popup_za, "Zoom &All");
    popup_menu.AppendSeparator();

    wxMenu *show_menu = new wxMenu;
    show_menu->AppendCheckItem (ID_plot_popup_data, "&Data", "");
    show_menu->Check (ID_plot_popup_data, data_visible);
    show_menu->AppendCheckItem (ID_plot_popup_sum, "S&um", "");
    show_menu->Check (ID_plot_popup_sum, sum_visible);
    show_menu->AppendCheckItem (ID_plot_popup_phase, "P&hases", "");
    show_menu->Check (ID_plot_popup_phase, phases_visible);
    show_menu->AppendCheckItem (ID_plot_popup_peak, "&Peaks", "");
    show_menu->Check (ID_plot_popup_peak, peaks_visible);
    show_menu->AppendCheckItem (ID_plot_popup_xaxis, "&X axis", "");
    show_menu->Check (ID_plot_popup_xaxis, x_axis_visible);
    show_menu->AppendCheckItem (ID_plot_popup_tics, "&Tics", "");
    show_menu->Check (ID_plot_popup_tics, tics_visible);
    show_menu->AppendSeparator();
    show_menu->AppendCheckItem (ID_plot_popup_smooth, "Sm&ooth lines", "");
    show_menu->Check (ID_plot_popup_smooth, smooth);
    popup_menu.Append (wxNewId(), "&Show", show_menu);

    wxMenu *color_menu = new wxMenu;
    color_menu->Append (ID_plot_popup_c_background, "&Background");
    color_menu->Append (ID_plot_popup_c_active_data, "&Active Data");
    color_menu->Append (ID_plot_popup_c_inactive_data, "&Inactive Data");
    color_menu->Append (ID_plot_popup_c_sum, "&Sum");
    color_menu->Append (ID_plot_popup_c_xaxis, "&X Axis");
    wxMenu *color_phase_menu = new wxMenu;
    for (int i = 0; i < max_phase_pens; i++)
        color_phase_menu->Append (ID_plot_popup_c_phase_0 + i, 
                                  wxString::Format("&%d", i));
    color_menu->Append (ID_plot_popup_c_phase, "P&hases", color_phase_menu);
    wxMenu *color_peak_menu = new wxMenu;
    for (int i = 0; i < max_peak_pens; i++)
        color_peak_menu->Append (ID_plot_popup_c_peak_0 + i, 
                                 wxString::Format("%d", i));
    color_menu->Append (ID_plot_popup_c_peak, "&Peaks", color_peak_menu);
    color_menu->AppendSeparator(); 
    color_menu->Append (ID_plot_popup_c_inv, "&Invert colors"); 
    popup_menu.Append (wxNewId(), "&Color", color_menu);  

    wxMenu *size_menu = new wxMenu;
    size_menu->AppendCheckItem (ID_plot_popup_pt_size, "&Line", "");
    size_menu->Check (ID_plot_popup_pt_size, line_between_points);
    size_menu->AppendSeparator();
    for (int i = 1; i <= max_radius; i++) 
        size_menu->AppendRadioItem (ID_plot_popup_pt_size + i, 
                                    wxString::Format ("&%d", i), "");
    size_menu->Check (ID_plot_popup_pt_size + point_radius, true);
    popup_menu.Append (wxNewId(), "Data point si&ze",size_menu);
    
    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void MainPlot::show_peak_menu (wxMouseEvent &event)
{
    if (over_peak == -1) return;
    wxMenu peak_menu; 
    peak_menu.Append(ID_peak_popup_info, "&Info");
    peak_menu.Append(ID_peak_popup_tree, "&Show tree");
    peak_menu.Append(ID_peak_popup_guess, "&Guess");
    peak_menu.Append(ID_peak_popup_del, "&Delete");
    peak_menu.Enable(ID_peak_popup_del, my_sum->refs_to_f(over_peak) == 0);
    //peak_menu.AppendSeparator();
    //TODO? parameters: height, ...
    PopupMenu (&peak_menu, event.GetX(), event.GetY());
}

void MainPlot::PeakInfo()
{
    if (over_peak >= 0)
        exec_command("s.info ^" + S(over_peak));
}

void MainPlot::OnPeakDelete(wxCommandEvent &WXUNUSED(event))
{
    if (over_peak >= 0)
        exec_command("s.remove ^" + S(over_peak));
}

void MainPlot::OnPeakGuess(wxCommandEvent &WXUNUSED(event))
{
    if (over_peak >= 0)
        exec_command("s.guess ^" + S(over_peak));
}

void MainPlot::OnPeakShowTree(wxCommandEvent &WXUNUSED(event))
{
    if (over_peak >= 0) {
        FuncBrowserDlg *dialog = new FuncBrowserDlg(this, -1, 2);
        dialog->show_expanded(over_peak);
        dialog->ShowModal();
        dialog->Destroy();
    }
           
}

bool MainPlot::has_mod_keys(const wxMouseEvent& event)
{
    return (event.AltDown() || event.ControlDown() || event.ShiftDown());
}


// mouse usage
//
//  Simple Rules:
//   1. When one button is down, pressing other button cancels action 
//   2. Releasing / keeping down / pressing Ctrl/Alt/Shift keys, when you keep 
//     mouse button down makes no difference.
//   3. Ctrl, Alt and Shift buttons are equivalent. 
//  ----------------------------------
//  Usage:
//   Left/Right Button (no Ctrl)   -- mode dependent  
//   Ctrl + Left/Right Button      -- the same as Left/Right in normal mode 
//   Middle Button                 -- rectangle zoom 

void MainPlot::set_mouse_mode(Mouse_mode_enum m)
{
    if (pressed_mouse_button) cancel_mouse_press();
    Mouse_mode_enum old = mode;
    if (m != mmd_peak) basic_mode = m;
    mode = m;
    update_mouse_hints();
    if (old != mode && (old == mmd_bg || mode == mmd_bg 
                        || visible_peaktops(old) != visible_peaktops(mode)))
        Refresh(false);
}

void MainPlot::update_mouse_hints()
    // update mouse hint on status bar and cursor
{
    string left="", right="";
    switch (pressed_mouse_button) {
        case 1:
            left = "";       right = "cancel";
            break;
        case 2:
            left = "cancel"; right = "cancel";
            break;
        case 3:
            left = "cancel"; right = "";
            break;
        default:
            //button not pressed
            switch (mode) {
                case mmd_peak:
                    left = "move peak"; right = "peak menu";
                    SetCursor (wxCURSOR_CROSS);
                    break;
                case mmd_zoom: 
                    left = "rect zoom"; right = "plot menu";
                    SetCursor (wxCURSOR_ARROW);
                    break;
                case mmd_bg: 
                    left = "add point"; right = "del point";
                    SetCursor (wxCURSOR_ARROW);
                    break;
                case mmd_add: 
                    left = "draw-add";  right = "add-in-range";
                    SetCursor (wxCURSOR_ARROW);
                    break;
                case mmd_range: 
                    left = "activate";  right = "disactivate";
                    SetCursor (wxCURSOR_ARROW);
                    break;
                default: 
                    assert(0);
            }
    }
    frame->set_status_hint(left.c_str(), right.c_str());
}

void MainPlot::OnMouseMove(wxMouseEvent &event)
{
    //display coords in status bar 
    int X = event.GetX();
    int Y = event.GetY();
    fp x = X2x(X);
    fp y = Y2y(Y);
    if (my_core->plus_background) 
        y -= my_data->get_bg_at (x);
    wxString str;
    str.Printf ("%.3f  %d", x, static_cast<int>(y + 0.5));
    frame->SetStatusText (str, sbf_coord);

    if (pressed_mouse_button == 0) {
        if (mode == mmd_range) {
            if (!ctrl)
                vert_line_following_cursor (mat_move, event.GetX());
            else
                vert_line_following_cursor (mat_cancel);
        }
        if (visible_peaktops(mode)) 
            look_for_peaktop (event);
        frame->draw_crosshair(X, Y);
    }
    else {
        vert_line_following_cursor (mat_move, event.GetX());
        move_peak(mat_move, event);
        peak_draft (mat_move, event);
        rect_zoom (mat_move, event);
    }
}

void MainPlot::look_for_peaktop (wxMouseEvent& event)
{
    // searching the closest peak-top and distance from it, d = dx + dy < 10 
    int min_dist = 10;
    int nearest = -1;
    if (size(shared.peaktops) != my_sum->fzg_size(fType))
        prepare_peaktops();
    for (vector<wxPoint>::const_iterator i = shared.peaktops.begin(); 
         i != shared.peaktops.end(); i++) {
        int d = abs(event.GetX() - i->x) + abs(event.GetY() - i->y);
        if (d < min_dist) {
            min_dist = d;
            nearest = i - shared.peaktops.begin();
        }
    }
    if (over_peak == nearest) return;

    //if we are here, over_peak != nearest; changing cursor and statusbar text
    over_peak = nearest;
    if (nearest != -1) {
        const V_f* f = my_sum->get_f(over_peak);
        string s = f->short_type() + S(over_peak) + " ";  
        string ref = my_sum->descr_refs_to_f(over_peak);
        if (!ref.empty())
            s += "(" + ref + ")";
        s += " " + f->type_info()->name 
            + " " + my_sum->info_fzg_parameters(fType, over_peak, true)
            + " " + f->extra_description();
        frame->SetStatusText (s.c_str());
        set_mouse_mode(mmd_peak);
    }
    else { //was over peak, but now is not 
        frame->SetStatusText ("");
        set_mouse_mode(basic_mode);
    }
}

void MainPlot::cancel_mouse_press()
{
    if (pressed_mouse_button) {
        rect_zoom (mat_cancel); 
        move_peak(mat_cancel);
        peak_draft (mat_cancel);
        vert_line_following_cursor (mat_cancel);
        mouse_press_X = mouse_press_Y = INVALID;
        pressed_mouse_button = 0;
        frame->SetStatusText ("");
        update_mouse_hints();
    }
}


void MainPlot::OnButtonDown (wxMouseEvent &event)
{
    if (pressed_mouse_button) {
        cancel_mouse_press();
        return;
    }

    frame->draw_crosshair(-1, -1);
    int button = event.GetButton();
    pressed_mouse_button = button;
    ctrl = has_mod_keys(event);
    mouse_press_X = event.GetX();
    mouse_press_Y = event.GetY();
    fp x = X2x (event.GetX());
    fp y = Y2y (event.GetY());
    if (button == 1 && (ctrl || mode == mmd_zoom) || button == 2) {
        rect_zoom (mat_start, event);
        SetCursor(wxCURSOR_MAGNIFIER);  
        frame->SetStatusText ("Select second corner to zoom...");
    }
    else if (button == 3 && (ctrl || mode == mmd_zoom)) {
        show_popup_menu (event);
        cancel_mouse_press();
    }
    else if (button == 1 && mode == mmd_peak) {
        move_peak(mat_start, event);
        if (my_sum->get_f(over_peak)->is_peak()) {
            frame->SetStatusText(wxString("Moving peak ^") +S(over_peak).c_str()
                                 + " (press Shift to change width)");
        }
        else
            frame->SetStatusText("It is not a peak, it can't be dragged.");
    }
    else if (button == 3 && mode == mmd_peak) {
        show_peak_menu(event);
        cancel_mouse_press();
    }
    else if (button == 1 && mode == mmd_bg) {
        exec_command ("d.background  " + S(x) + "  " + S(y)); 
    }
    else if (button == 3 && mode == mmd_bg) {
        exec_command ("d.background  ! " + S(x)); 
    }
    else if (button == 1 && mode == mmd_add) {
        peak_draft (mat_start, event);
        SetCursor(wxCURSOR_SIZING);
        frame->SetStatusText("Add drawed peak...");
    }
    else if (button == 3 && mode == mmd_add) {
        vert_line_following_cursor(mat_start, mouse_press_X+1, mouse_press_X);
        SetCursor(wxCURSOR_SIZEWE);
        frame->SetStatusText("Select range to add a peak in it..."); 
    }
    else if (button != 2 && mode == mmd_range) {
        vert_line_following_cursor(mat_start, mouse_press_X+1, mouse_press_X);
        SetCursor(wxCURSOR_SIZEWE);
        frame->SetStatusText(button==1 ? "Select data range to activate..."
                                       : "Select data range to disactivate...");
    }
    update_mouse_hints();
}


void MainPlot::OnButtonUp (wxMouseEvent &event)
{
    int button = event.GetButton();
    if (button != pressed_mouse_button) {
        pressed_mouse_button = 0;
        return;
    }
    int dist_x = abs(event.GetX() - mouse_press_X);  
    int dist_y = abs(event.GetY() - mouse_press_Y);  
    // if Down and Up events are at the same position -> cancel
    if (button == 1 && (ctrl || mode == mmd_zoom) || button == 2) {
        rect_zoom(dist_x + dist_y < 10 ? mat_cancel: mat_stop, event);
        frame->SetStatusText("");
    }
    else if (mode == mmd_peak && button == 1) {
        move_peak(dist_x + dist_y < 2 ? mat_cancel: mat_stop, event);
        frame->SetStatusText ("");
    }
    else if (mode == mmd_range && button != 2) {
        vert_line_following_cursor(mat_cancel);
        if (dist_x >= 5) { 
            fp xmin = X2x (min (event.GetX(), mouse_press_X));
            fp xmax = X2x (max (event.GetX(), mouse_press_X));
            if (button == 1)
                exec_command ("d.range + [ " + S(xmin) + " : " + S(xmax)+" ]");
            else //button == 3
                exec_command ("d.range - [ " + S(xmin) + " : " + S(xmax)+" ]");
        }
        frame->SetStatusText ("");
    }
    else if (mode == mmd_add && button == 1) {
        frame->SetStatusText ("");
        peak_draft (dist_x + dist_y < 5 ? mat_cancel: mat_stop, event);
    }
    else if (mode == mmd_add && button == 3) {
        frame->SetStatusText ("");
        if (dist_x >= 5)  
            add_peak_in_range(X2x(mouse_press_X), X2x(event.GetX()));
        vert_line_following_cursor(mat_cancel);
    }
    else {
        ;// nothing - action done in OnButtonDown()
    }
    pressed_mouse_button = 0;
    update_mouse_hints();
}


void MainPlot::OnKeyDown (wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_ESCAPE) {
        cancel_mouse_press(); 
    }
    else if (event.GetKeyCode() == ' ' || event.GetKeyCode() == WXK_TAB) {
        cancel_mouse_press(); 
        frame->focus_input();
    }
    else
        event.Skip();
}


void MainPlot::move_peak (Mouse_act_enum ma, wxMouseEvent &event)
{
    static bool started = false;
    static wxPoint prev(INVALID, INVALID);
    static fp height, center, hwhm, shape;
    static bool c_height, c_center, c_hwhm, c_shape; //changable 
    static const V_f *p = 0;
    static const f_names_type *ft = 0;
    static wxCursor old_cursor = wxNullCursor;
    if (ma != mat_start) {
        if (!started) return;
        draw_peak_draft(x2X(center - my_sum->zero_shift(center)), 
                        shared.dx2dX(hwhm), y2Y(height),
                        shape, ft);//clear old
    }
    switch (ma) {
        case mat_start: {
            p = my_sum->get_f(over_peak);
            ft = p->type_info();
            if (!p->is_peak()) return;
            my_sum->use_param_a_for_value();
            height = p->height();
            center = p->center();
            hwhm = p->fwhm() / 2.;
            shape = p->g_size > 3 ? p->values_of_pags()[3] : 0;
            c_height = p->get_pag(0).is_a();
            c_center = p->get_pag(1).is_a();
            c_hwhm = p->get_pag(2).is_a();
            c_shape = p->g_size > 3 && p->get_pag(3).is_a();
            draw_peak_draft(x2X(center - my_sum->zero_shift(center)), 
                            shared.dx2dX(hwhm), y2Y(height),
                            shape, ft);
            prev.x = event.GetX(), prev.y = event.GetY();
            started = true;
            old_cursor = GetCursor();
            SetCursor(wxCURSOR_SIZENWSE);
            break;
        }
        case mat_move: {
            fp dx = X2x(event.GetX()) - X2x(prev.x);
            fp dy = Y2y(event.GetY()) - Y2y(prev.y);
            if (!has_mod_keys(event)) {
                if (c_center) center += dx;
                if (c_height) height += dy;
                frame->SetStatusText("[Shift=change width/shape] Ctr:" 
                                     + wxString(S(center).c_str())
                                     + " Height:" + S(height).c_str());
            }
            else {
                if (c_hwhm) hwhm = fabs(hwhm + dx);
                if (c_shape) shape *= (1 - 0.05 * (event.GetY() - prev.y)); 
                frame->SetStatusText("Width:" + wxString(S(hwhm*2).c_str())
                                   + " Shape:" + wxString(S(shape).c_str()));
            }
            prev.x = event.GetX(), prev.y = event.GetY();
            draw_peak_draft(x2X(center - my_sum->zero_shift(center)), 
                            shared.dx2dX(hwhm), y2Y(height), 
                            shape, ft);
            break;
        }
        case mat_stop:
            change_peak_parameters(vector4(height, center, hwhm, shape));
            //no break
        case mat_cancel:
            started = false;
            if (old_cursor.Ok()) {
                SetCursor(old_cursor);
                old_cursor = wxNullCursor;
            }
            break;
        default: assert(0);
    }
}


void MainPlot::peak_draft (Mouse_act_enum ma, wxMouseEvent &event)
{
    static wxPoint prev(INVALID, INVALID);
    if (ma != mat_start) {
        if (prev.x == INVALID) return;
        //clear old peak-draft
        draw_peak_draft(mouse_press_X, abs(mouse_press_X - prev.x), prev.y);
    }
    switch (ma) {
        case mat_start:
        case mat_move:
            prev.x = event.GetX(), prev.y = event.GetY();
            draw_peak_draft(mouse_press_X, abs(mouse_press_X-prev.x), prev.y);
            break;
        case mat_stop: 
            add_peak(Y2y(event.GetY()), X2x(mouse_press_X),
                     fabs(shared.dX2dx(mouse_press_X - event.GetX())));
            //no break
        case mat_cancel:
            prev.x = prev.y = INVALID;
            break;
        default: assert(0);
    }
}

void MainPlot::draw_peak_draft(int Ctr, int Hwhm, int Y, 
                               float Shape, const f_names_type *f)
{
    if (Ctr == INVALID || Hwhm == INVALID || Y == INVALID)
        return;
    wxClientDC dc(this);
    dc.SetLogicalFunction (wxINVERT);
    dc.SetPen(*wxBLACK_DASHED_PEN);
    int Y0 = y2Y(0);
    dc.DrawLine (Ctr, Y0, Ctr, Y); //vertical line
    dc.DrawLine (Ctr - Hwhm, (Y+Y0)/2, Ctr + Hwhm, (Y+Y0)/2); //horizontal line
    if (f) {
        vector<fp> hcw =  vector4(Y2y(Y), fp(Ctr), fp(Hwhm), fp(Shape));
        vector<fp> ini = V_f::get_default_peak_parameters(*f, hcw); 
        vector<Pag> ini_p(ini.begin(), ini.end());
        const int n = 40;
        vector<wxPoint> v(2*n+1);
        char type = f->type;
        V_f *peak = V_f::factory(0, type, ini_p);
        peak->pre_compute_value_only(vector<fp>(), vector<V_g*>());
        int pX_=0, pY_=0;
        for (int i = -n; i <= n; i++) {
            int X_ = int(Ctr + Hwhm * 5. * i / n); 
            int Y_ = y2Y(peak->compute(X_, 0));
            if (i+n != 0)
                dc.DrawLine(pX_, pY_, X_, Y_); 
            pX_ = X_;
            pY_ = Y_;
        }
        delete peak;
    }
    else {
        dc.DrawLine (Ctr, Y, Ctr - 2 * Hwhm, Y0); //left slope
        dc.DrawLine (Ctr, Y, Ctr + 2 * Hwhm, Y0); //right slope
    }
}

bool MainPlot::rect_zoom (Mouse_act_enum ma, wxMouseEvent &event) 
{
    static int X1 = INVALID, Y1 = INVALID, X2 = INVALID, Y2 = INVALID;

    if (ma == mat_start) {
        X1 = X2 = event.GetX(), Y1 = Y2 = event.GetY(); 
        draw_rect (X1, Y1, X2, Y2);
        CaptureMouse();
        return true;
    }
    else {
        if (X1 == INVALID || Y1 == INVALID) return false;
        draw_rect (X1, Y1, X2, Y2); //clear old rectangle
    }

    if (ma == mat_move) {
        X2 = event.GetX(), Y2 = event.GetY(); 
        draw_rect (X1, Y1, X2, Y2);
    }
    else if (ma == mat_stop) {
        X2 = event.GetX(), Y2 = event.GetY(); 
        int Xmin = min (X1, X2), Xmax = max (X1, X2);
        int width = Xmax - Xmin;
        int Ymin = min (Y1, Y2), Ymax = max (Y1, Y2);
        int height = Ymax - Ymin;
        if (width > 5 && height > 5) 
            frame->change_zoom("[ "+S(X2x(Xmin))+" : "+S(X2x(Xmax))+" ]"
                               "[ "+S(Y2y(Ymax))+" : "+S(Y2y(Ymin))+" ]"); 
                                    //Y2y(Ymax) < Y2y(Ymin)
    }

    if (ma == mat_cancel || ma == mat_stop) {
        X1 = Y1 = X2 = Y2 = INVALID;
        ReleaseMouse();
    }
    return true;
}

void MainPlot::draw_rect (int X1, int Y1, int X2, int Y2)
{
    if (X1 == INVALID || Y1 == INVALID || X2 == INVALID || Y2 == INVALID) 
        return;
    wxClientDC dc(this);
    dc.SetLogicalFunction (wxINVERT);
    dc.SetPen(*wxBLACK_DASHED_PEN);
    dc.SetBrush (*wxTRANSPARENT_BRUSH);
    int xmin = min (X1, X2);
    int width = max (X1, X2) - xmin;
    int ymin = min (Y1, Y2);
    int height = max (Y1, Y2) - ymin;
    dc.DrawRectangle (xmin, ymin, width, height);
}

void MainPlot::OnPopupShowXX (wxCommandEvent& event)
{
    switch (event.GetId()) {
        case ID_plot_popup_data :  data_visible = !data_visible;     break; 
        case ID_plot_popup_sum  :  sum_visible = !sum_visible;       break; 
        case ID_plot_popup_phase:  phases_visible = !phases_visible; break; 
        case ID_plot_popup_peak :  peaks_visible = !peaks_visible;   break;  
        case ID_plot_popup_xaxis:  x_axis_visible = !x_axis_visible; break; 
        case ID_plot_popup_tics :  tics_visible = !tics_visible;     break; 
        case ID_plot_popup_smooth :  smooth = !smooth;               break; 
        default: assert(0);
    }
    Refresh(false);
}

void MainPlot::OnPopupColor(wxCommandEvent& event)
{
    wxBrush *brush = 0;
    wxPen *pen = 0;
    int n = event.GetId();
    if (n == ID_plot_popup_c_background)
        brush = &backgroundBrush;
    else if (n == ID_plot_popup_c_active_data) {
        pen = &activeDataPen;
    }
    else if (n == ID_plot_popup_c_inactive_data) {
        pen = &inactiveDataPen;
    }
    else if (n == ID_plot_popup_c_sum)
        pen = &sumPen;
    else if (n == ID_plot_popup_c_xaxis)
        pen = &xAxisPen;
    else if (n >= ID_plot_popup_c_phase_0 
              && n < ID_plot_popup_c_phase_0 + max_phase_pens)
        pen = &phasePen[n - ID_plot_popup_c_phase_0];
    else if (n >= ID_plot_popup_c_peak_0
              && n < ID_plot_popup_c_peak_0 + max_peak_pens)
        pen = &peakPen[n - ID_plot_popup_c_peak_0];
    else 
        return;
    const wxColour &col = brush ? brush->GetColour() : pen->GetColour();
    wxColourData col_data;
    col_data.SetCustomColour (0, col);
    col_data.SetColour (col);
    wxColourDialog dialog (this, &col_data);
    if (dialog.ShowModal() == wxID_OK) {
        wxColour new_col = dialog.GetColourData().GetColour();
        if (brush) brush->SetColour (new_col);
        if (pen) pen->SetColour (new_col);
        Refresh();
    }
}

wxColour invert_colour(const wxColour& col)
{
    return wxColour(255 - col.Red(), 255 - col.Green(), 255 - col.Blue());
}

void MainPlot::OnInvertColors (wxCommandEvent& WXUNUSED(event))
{
    backgroundBrush.SetColour (invert_colour (backgroundBrush.GetColour()));
    activeDataPen.SetColour (invert_colour(activeDataPen.GetColour()));
    inactiveDataPen.SetColour (invert_colour(inactiveDataPen.GetColour()));
    sumPen.SetColour (invert_colour (sumPen.GetColour()));  
    xAxisPen.SetColour (invert_colour (xAxisPen.GetColour()));  
    for (int i = 0; i < max_phase_pens; i++)
        phasePen[i].SetColour (invert_colour (phasePen[i].GetColour()));
    for (int i = 0; i < max_peak_pens; i++)
        peakPen[i].SetColour (invert_colour (peakPen[i].GetColour()));
    Refresh();
}

void MainPlot::OnPopupRadius (wxCommandEvent& event)
{
    int nr = event.GetId() - ID_plot_popup_pt_size;
    if (nr == 0)
        line_between_points = !line_between_points;
    else
        point_radius = nr;
    Refresh(false);
}


void MainPlot::OnZoomAll (wxCommandEvent& WXUNUSED(event))
{
    frame->OnGViewAll(dummy_cmd_event);
}



void MainPlot::change_peak_parameters(const vector<fp> &peak_hcw)
{
    vector<string> changes;
    const V_f *peak = my_sum->get_f(over_peak);
    const f_names_type *f = peak->type_info();
    assert(peak_hcw.size() >= 3);
    fp height = peak_hcw[0]; 
    fp center = peak_hcw[1];
    fp hwhm = peak_hcw[2]; 
    for (int i = 0; i < f->psize; i++) {
        string pname = f->pnames[i];
        fp val = 0;
        if (i > 2 && size(peak_hcw) > i) val = peak_hcw[i];
        else if (pname == "height") val = height; 
        else if (pname == "center") val = center; 
        else if (pname == "HWHM")   val = hwhm;
        else if (pname == "FWHM")   val = 2*hwhm;
        else if (pname.find("width") < pname.size())  val = hwhm; 
        else continue;
        Pag pag = peak->get_pag(i);
        if (pag.is_a() && my_sum->pars()->get_a(pag.a()) != val)
            changes.push_back("@" + S(pag.a()) + " " + S(val));
    }
    if (!changes.empty()) {
        string cmd = "s.change " + changes[0]; 
        for (unsigned int i = 1; i < changes.size(); i++)
            cmd += ", " + changes[i];
        exec_command (cmd);
    }
}


//===============================================================
//                           AuxPlot (auxiliary plot) 
//===============================================================
BEGIN_EVENT_TABLE (AuxPlot, FPlot)
    EVT_PAINT (           AuxPlot::OnPaint)
    EVT_MOTION (          AuxPlot::OnMouseMove)
    EVT_LEAVE_WINDOW (    AuxPlot::OnLeaveWindow)
    EVT_LEFT_DOWN (       AuxPlot::OnLeftDown)
    EVT_LEFT_UP (         AuxPlot::OnLeftUp)
    EVT_RIGHT_DOWN (      AuxPlot::OnRightDown)
    EVT_MIDDLE_DOWN (     AuxPlot::OnMiddleDown)
    EVT_KEY_DOWN   (      AuxPlot::OnKeyDown)
    EVT_MENU_RANGE (ID_aux_popup_plot_0, ID_aux_popup_plot_0 + 10,
                                                    AuxPlot::OnPopupPlot)
    EVT_MENU_RANGE (ID_aux_popup_c_background, ID_aux_popup_color - 1, 
                                                      AuxPlot::OnPopupColor)
    EVT_MENU (ID_aux_popup_yz_change, AuxPlot::OnPopupYZoom)
    EVT_MENU (ID_aux_popup_yz_fit, AuxPlot::OnPopupYZoomFit)
    EVT_MENU (ID_aux_popup_yz_auto, AuxPlot::OnPopupYZoomAuto)
END_EVENT_TABLE()

void AuxPlot::OnPaint(wxPaintEvent &WXUNUSED(event))
{
    vert_line_following_cursor(mat_cancel);//erase XOR lines before repainting
    frame->draw_crosshair(-1, -1); 
    wxPaintDC dc(this);
    dc.SetLogicalFunction (wxCOPY);
    if (backgroundBrush.Ok())
        dc.SetBackground (backgroundBrush);
    dc.Clear();
    Draw(dc);
}

fp diff_of_data_for_draw_data (vector<Point>::const_iterator i)
{
    return i->y - sum_value (i);
}

fp diff_stddev_of_data_for_draw_data (vector<Point>::const_iterator i)
{
    return (i->y - sum_value(i)) / i->sigma;
}

fp diff_y_proc_of_data_for_draw_data (vector<Point>::const_iterator i)
{
    return i->y ? (i->y - sum_value(i)) / i->y * 100 : 0;
}

void AuxPlot::Draw(wxDC &dc)
{
    if (auto_zoom_y)
        fit_y_zoom();
    set_scale();
    if (kind == apk_empty || my_data->is_empty()) 
        return;
    dc.SetPen (xAxisPen);

    if (kind == apk_peak_pos) {
        int ymax = GetClientSize().GetHeight();
        for (vector<wxPoint>::const_iterator i = shared.peaktops.begin(); 
             i != shared.peaktops.end(); i++) 
            dc.DrawLine(i->x, 0, i->x, ymax);
        return;
    }

    if (x_axis_visible) {
        dc.DrawLine (0, y2Y(0), GetClientSize().GetWidth(), y2Y(0));
        if (kind == apk_diff) draw_zoom_text(dc);
    }
    if (tics_visible) {
        Rect v (0, 0, Y2y(GetClientSize().GetHeight()), Y2y(0));
        draw_tics(dc, v, 0, 5, 0, 4);
    }

    if (kind == apk_diff)
        draw_data (dc, diff_of_data_for_draw_data);
    else if (kind == apk_diff_stddev)
        draw_data (dc, diff_stddev_of_data_for_draw_data);
    else if (kind == apk_diff_y_proc)
        draw_data (dc, diff_y_proc_of_data_for_draw_data);
}

void AuxPlot::draw_zoom_text(wxDC& dc)
{
    dc.SetTextForeground(xAxisPen.GetColour());
    dc.SetFont(*wxNORMAL_FONT);  
    string s = "x" + S(y_zoom);  
    wxCoord w, h;
    dc.GetTextExtent (s.c_str(), &w, &h); 
    dc.DrawText (s.c_str(), dc.MaxX() - w - 2, 2);
}

void AuxPlot::OnMouseMove(wxMouseEvent &event)
{
    int X = event.GetX();
    fp x = X2x(X);
    fp y = Y2y (event.GetY()); 
    vert_line_following_cursor (mat_move, X);
    wxString str;
    str.Printf ("%.3f  [%d]", x, static_cast<int>(y + 0.5));
    frame->SetStatusText (str, sbf_coord);
    wxCursor new_cursor;
    if (X < move_plot_margin_width)
        new_cursor = wxCURSOR_POINT_LEFT;
    else if (X > GetClientSize().GetWidth() - move_plot_margin_width)
        new_cursor = wxCURSOR_POINT_RIGHT;
    else {
        frame->draw_crosshair(X, -1);
        new_cursor = wxCURSOR_CROSS;
    }
    if (new_cursor != cursor) {
        cursor = new_cursor;
        SetCursor (new_cursor);
    }
}

void AuxPlot::OnLeaveWindow (wxMouseEvent& WXUNUSED(event))
{
    frame->SetStatusText ("", sbf_coord);
    vert_line_following_cursor (mat_cancel);
    frame->draw_crosshair(-1, -1);
}

bool AuxPlot::is_zoomable()
{
    return kind == apk_diff || kind == apk_diff_stddev 
           || kind == apk_diff_y_proc;
}

void AuxPlot::set_scale()
{
    switch (kind) {
        case apk_empty:
        case apk_peak_pos:
            yUserScale = 1.; //y scale doesn't matter
            break; 
        case apk_diff: 
            yUserScale = shared.plot_y_scale * y_zoom;
            break;
        case apk_diff_stddev:
        case apk_diff_y_proc:
            yUserScale = -1. * y_zoom_base * y_zoom;
            break;
    }
    int h = GetClientSize().GetHeight();
    yLogicalOrigin = - h / 2. / yUserScale;
}
 
void AuxPlot::read_settings(wxConfigBase *cf)
{
    wxString path = "/AuxPlot_" + name;
    cf->SetPath(path);
    kind = static_cast <Aux_plot_kind_enum> (cf->Read ("kind", apk_diff));
    auto_zoom_y = false;
    line_between_points = read_bool_from_config(cf,"line_between_points", true);
    point_radius = cf->Read ("point_radius", 1);
    cf->SetPath(path + "/Visible");
    x_axis_visible = read_bool_from_config (cf, "xAxis", true);
    tics_visible = read_bool_from_config (cf, "tics", true);
    cf->SetPath(path + "/Colors");
    //backgroundBrush = *wxBLACK_BRUSH;
    backgroundBrush.SetColour (read_color_from_config (cf, "bg", 
                                                       wxColour(50, 50, 50)));
    wxColour active_data_col = read_color_from_config (cf, "active_data",
                                                       wxColour ("GREEN"));
    activeDataPen.SetColour (active_data_col);
    //activeDataPen.SetStyle (wxDOT);
    wxColour inactive_data_col = read_color_from_config (cf, "inactive_data",
                                                      wxColour (128, 128, 128));
    inactiveDataPen.SetColour (inactive_data_col);
    xAxisPen.SetColour (read_color_from_config(cf, "xAxis", wxColour("WHITE")));
    Refresh();
}

void AuxPlot::save_settings(wxConfigBase *cf) const
{
    wxString path = "/AuxPlot_" + name;
    cf->SetPath(path);
    cf->Write ("kind", kind); 
    cf->Write ("line_between_points", line_between_points);
    cf->Write ("point_radius", point_radius);

    cf->SetPath(path + "/Visible");
    cf->Write ("xAxis", x_axis_visible);
    cf->Write ("tics", tics_visible);

    cf->SetPath(path + "/Colors");
    write_color_to_config (cf, "bg", backgroundBrush.GetColour()); 
    write_color_to_config (cf, "active_data", activeDataPen.GetColour());
    write_color_to_config (cf, "inactive_data", inactiveDataPen.GetColour());
    write_color_to_config (cf, "xAxis", xAxisPen.GetColour());
}

void AuxPlot::OnLeftDown (wxMouseEvent &event)
{
    cancel_mouse_left_press();
    if (event.ShiftDown()) { // the same as OnMiddleDown()
        frame->OnGViewAll(dummy_cmd_event);
        return;
    }
    int X = event.GetPosition().x;
    // if mouse pointer is near to left or right border, move view
    if (X < move_plot_margin_width) 
        frame->scroll_view_horizontally(-0.33);  // <--
    else if (X > GetClientSize().GetWidth() - move_plot_margin_width) 
        frame->scroll_view_horizontally(+0.33); // -->
    else {
        mouse_press_X = X;
        vert_line_following_cursor(mat_start, mouse_press_X+1, mouse_press_X);
        SetCursor(wxCURSOR_SIZEWE);  
        frame->SetStatusText ("Select x range and release button to zoom..."); 
        CaptureMouse();
    }
}

bool AuxPlot::cancel_mouse_left_press()
{
    if (mouse_press_X != INVALID) {
        vert_line_following_cursor(mat_cancel);
        ReleaseMouse();
        mouse_press_X = INVALID;
        cursor = wxCURSOR_CROSS;
        SetCursor(wxCURSOR_CROSS);  
        frame->SetStatusText(""); 
        return true;
    }
    else
        return false;
}

void AuxPlot::OnLeftUp (wxMouseEvent &event)
{
    if (mouse_press_X == INVALID)
        return;
    int xmin = min (event.GetX(), mouse_press_X);
    int xmax = max (event.GetX(), mouse_press_X);
    cancel_mouse_left_press();

    if (xmax - xmin < 5) //cancel
        return;
    frame->change_zoom("[" + S(X2x(xmin)) + " : " + S(X2x(xmax)) + "]");
}

//popup-menu
void AuxPlot::OnRightDown (wxMouseEvent &event)
{
    if (cancel_mouse_left_press())
        return;

    wxMenu popup_menu ("aux. plot menu");
    //wxMenu *kind_menu = new wxMenu;
    popup_menu.AppendRadioItem(ID_aux_popup_plot_0 + 0, "&empty", "nothing");
    popup_menu.AppendRadioItem(ID_aux_popup_plot_0 + 1, "&diff", "y_d - y_s");
    popup_menu.AppendRadioItem(ID_aux_popup_plot_0 + 2, "&weighted diff", 
                                "(y_d - y_s) / sigma");
    popup_menu.AppendRadioItem(ID_aux_popup_plot_0 + 3, "&proc diff", 
                                "(y_d - y_s) / y_d [%]");
    popup_menu.AppendRadioItem (ID_aux_popup_plot_0 + 4, "p&eak positions", 
                                "mark centers of peaks");
    popup_menu.Check(ID_aux_popup_plot_0 + kind, true);
    popup_menu.AppendSeparator();
    popup_menu.Append (ID_aux_popup_yz_fit, "&Fit to window");
    popup_menu.Enable(ID_aux_popup_yz_fit, is_zoomable());
    popup_menu.Append (ID_aux_popup_yz_change, "&Change y scale");
    popup_menu.Enable(ID_aux_popup_yz_change, is_zoomable());
    popup_menu.AppendCheckItem (ID_aux_popup_yz_auto, "&Auto-fit");
    popup_menu.Check (ID_aux_popup_yz_auto, auto_zoom_y);
    popup_menu.Enable(ID_aux_popup_yz_auto, is_zoomable());
    popup_menu.AppendSeparator();
    wxMenu *color_menu = new wxMenu;
    color_menu->Append (ID_aux_popup_c_background, "&Background");
    color_menu->Append (ID_aux_popup_c_active_data, "&Active Data");
    color_menu->Append (ID_aux_popup_c_inactive_data, "&Inactive Data");
    color_menu->Append (ID_aux_popup_c_axis, "&X Axis");
    popup_menu.Append (ID_aux_popup_color, "&Color", color_menu);
    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void AuxPlot::OnMiddleDown (wxMouseEvent& WXUNUSED(event))
{
    if (cancel_mouse_left_press())
        return;
    frame->OnGViewAll(dummy_cmd_event);
}

void AuxPlot::OnKeyDown (wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_ESCAPE) {
        cancel_mouse_left_press();
    }
    else if (event.GetKeyCode() == ' ' || event.GetKeyCode() == WXK_TAB) {
        cancel_mouse_left_press();
        frame->focus_input();
    }
    else
        event.Skip();
}

void AuxPlot::OnPopupPlot (wxCommandEvent& event)
{
    kind = static_cast<Aux_plot_kind_enum>(event.GetId()-ID_aux_popup_plot_0);
    fit_y_zoom();
    Refresh(false);
}

void AuxPlot::OnPopupColor (wxCommandEvent& event)
{
    wxBrush *brush = 0;
    wxPen *pen = 0;
    int n = event.GetId();
    if (n == ID_aux_popup_c_background)
        brush = &backgroundBrush;
    else if (n == ID_aux_popup_c_active_data) {
        pen = &activeDataPen;
    }
    else if (n == ID_aux_popup_c_inactive_data) {
        pen = &inactiveDataPen;
    }
    else if (n == ID_aux_popup_c_axis)
        pen = &xAxisPen;
    else 
        return;
    const wxColour &col = brush ? brush->GetColour() : pen->GetColour();
    wxColourData col_data;
    col_data.SetCustomColour (0, col);
    col_data.SetColour (col);
    wxColourDialog dialog (this, &col_data);
    if (dialog.ShowModal() == wxID_OK) {
        wxColour new_col = dialog.GetColourData().GetColour();
        if (brush) brush->SetColour (new_col);
        if (pen) pen->SetColour (new_col);
        Refresh();
    }
}

void AuxPlot::OnPopupYZoom (wxCommandEvent& WXUNUSED(event))
{
    int r = wxGetNumberFromUser ("Set zoom in y direction [%]", 
                                 "", "", static_cast<int>(y_zoom * 100 + 0.5), 
                                 1, 10000000);
    if (r > 0)
        y_zoom = r / 100.;
    Refresh(false);
}

void AuxPlot::OnPopupYZoomFit (wxCommandEvent& WXUNUSED(event))
{
    fit_y_zoom();
    Refresh(false);
}

void AuxPlot::fit_y_zoom()
{
    if (!is_zoomable())
        return;
    fp y = 0.;
    switch (kind) { // setting y_zoom
        case apk_diff: 
            {
            y = get_max_abs_y (diff_of_data_for_draw_data);
            y_zoom = fabs (GetClientSize().GetHeight() / 
                                            (2 * y * shared.plot_y_scale));
            fp order = pow (10, floor (log10(y_zoom)));
            y_zoom = floor(y_zoom / order) * order;
            }
            break;
        case apk_diff_stddev:
            y = get_max_abs_y (diff_stddev_of_data_for_draw_data);
            y_zoom_base = GetClientSize().GetHeight() / (2. * y);
            y_zoom = 0.9;
            break;
        case apk_diff_y_proc:
            y = get_max_abs_y (diff_y_proc_of_data_for_draw_data);
            y_zoom_base = GetClientSize().GetHeight() / (2. * y);
            y_zoom = 0.9;
            break;
        default:
            assert(0);
    }
}

void AuxPlot::OnPopupYZoomAuto (wxCommandEvent& WXUNUSED(event))
{
    auto_zoom_y = !auto_zoom_y;
    if (auto_zoom_y) {
        fit_y_zoom();
        Refresh(false);
    }
}

fp scale_tics_step (fp beg, fp end, int max_tics)
{
    if (beg >= end || max_tics <= 0)
        return +INF;
    assert (max_tics > 0);
    fp min_step = (end - beg) / max_tics;
    fp s = pow(10, floor (log10 (min_step)));
    // now s < min_step
    if (s >= min_step)
        ;
    else if (s * 2 >= min_step)
        s *= 2;
    else if (s * 2.5 >= min_step)
        s *= 2.5;
    else if (s * 5 >=  min_step)
        s *= 5;
    else
        s *= 10;
    return s;
    //first = s * ceil(beg / s);
}

//simple utils declared in wx_common.h 
bool from_config_read_bool(wxConfigBase *cf, const wxString& key, bool def_val)
{ bool b; cf->Read(key, &b, def_val); return b; }

double from_config_read_double(wxConfigBase *cf, const wxString& key, 
                               double def_val)
{ double d; cf->Read(key, &d, def_val); return d; }

//dummy events declared in wx_common.h
wxMouseEvent dummy_mouse_event;
wxCommandEvent dummy_cmd_event;

