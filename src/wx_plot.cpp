// This file is part of fityk program. Copyright (C) Marcin Wojdyr

// wxwindows headers, see wxwindows samples for description
#ifdef __GNUG__
#pragma implementation
#endif
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
#include "data.h"
#include "sum.h"
#include "other.h" 
#include "ffunc.h"
#ifdef USE_XTAL
    #include "crystal.h"
#endif //USE_XTAL

using namespace std;

enum {
    ID_plot_popup_za                = 45001, 
    ID_plot_popup_m                 = 45003, //and next 2

    ID_plot_popup_data              = 45011,
    ID_plot_popup_sum                      ,
    ID_plot_popup_phase                    ,
    ID_plot_popup_peak                     ,
    ID_plot_popup_xaxis                    ,
    ID_plot_popup_tics                     ,

    ID_plot_popup_c_background             ,
    ID_plot_popup_c_active_data            ,
    ID_plot_popup_c_inactive_data          ,
    ID_plot_popup_c_sum                    ,
    ID_plot_popup_c_xaxis                  ,
    ID_plot_popup_c_phase_0         = 45100,
    ID_plot_popup_c_peak_0          = 45140,
    ID_plot_popup_c_phase           = 45190,
    ID_plot_popup_c_peak                   ,
    ID_plot_popup_c_inv                    ,

    ID_plot_popup_buffer                   ,
    ID_plot_popup_pt_size           = 45210,// and next 10 ,
    ID_plot_popup_conf_save         = 45225,
    ID_plot_popup_conf_reset               ,
    ID_plot_popup_conf_revert              ,
    ID_plot_popup_alt_conf_save            ,
    ID_plot_popup_alt_conf_reset           ,
    ID_plot_popup_alt_conf_revert          ,

    ID_plot_zpm_prev                = 45240,// and next 10
    ID_plot_zpm_fitv                = 45255,
    ID_plot_zpm_zall                       ,

    ID_diff_popup_plot_0            = 45310,
    ID_diff_popup_c_background      = 45340,
    ID_diff_popup_c_active_data            ,
    ID_diff_popup_c_inactive_data          ,
    ID_diff_popup_c_axis                   ,
    ID_diff_popup_color                    ,
    ID_diff_popup_yz_fit                   ,
    ID_diff_popup_yz_change                ,
    ID_diff_popup_yz_auto                  ,
    ID_diff_popup_y_zoom                   
};

fp scale_tics_step (fp beg, fp end, int max_tics);

//===============================================================
//                MyPlot (plot with data and fitted curves) 
//===============================================================

void MyPlot::draw_dashed_vert_lines (int x1, int x2)
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

bool MyPlot::vert_line_following_cursor (Mouse_act_enum ma, int x)
{
    switch(ma) {
        case start_ma_ty:
        case move_ma_ty:
            draw_dashed_vert_lines (vlfc_prev_x, x);
            vlfc_prev_x = x;
            break;
        case stop_ma_ty:
        case cancel_ma_ty:
            if (vlfc_prev_x == INVALID)
                return false;
            draw_dashed_vert_lines (vlfc_prev_x);
            vlfc_prev_x = INVALID;
            break;
        default: assert(0);
    }
    return true;
}

void MyPlot::move_view_horizontally (bool on_left)
{
    const Rect &vw = my_other->view;
    fp diff = vw.width() / 3;
    fp new_left, new_right;
    if (on_left) {
        new_left = vw.left - diff;
        new_right = vw.right - diff;
    }
    else { //on right
        new_left = vw.left + diff;
        new_right = vw.right + diff;
    }
    change_zoom ("[" + S(new_left) + " : " + S(new_right) + "] .");
}

void MyPlot::change_zoom (string s)
{
    const int max_length_of_zoom_history = 6;
    string cmd = "o.plot " + s;
    common.zoom_hist.push_back(my_other->view.str());
    if (size(common.zoom_hist) > max_length_of_zoom_history)
        common.zoom_hist.erase (common.zoom_hist.begin());
    exec_command (cmd);
    //frame->SetStatusText (""); 
}

void MyPlot::perhaps_it_was_silly_zoom_try() const
{
    frame->SetStatusText("You can zoom with middle button or using aux. plot.");
}

BEGIN_EVENT_TABLE(MyPlot, wxPanel)
END_EVENT_TABLE()

//-------------------------------------
//optimization: remembering sum values

const fp initial_buffered_value = -1.234565;

vector<fp> buffered_sum;

void clear_buffered_sum()
{
    int size = my_data->points().size();
    if (size) {
        buffered_sum.resize (size);
        fill (buffered_sum.begin(), buffered_sum.end(), initial_buffered_value);
    }
}

fp sum_value(vector<Point>::const_iterator pt)
{
    vector<fp>::iterator y = buffered_sum.begin() 
                                            + (pt - my_data->points().begin());
    if (*y == initial_buffered_value)
        *y = my_sum->value(pt->x);
    return *y;
}

//===============================================================
//                MainPlot (plot with data and fitted curves) 
//===============================================================
BEGIN_EVENT_TABLE(MainPlot, MyPlot)
    EVT_PAINT (           MainPlot::OnPaint)
    EVT_MOTION (          MainPlot::OnMouseMove)
    EVT_LEAVE_WINDOW (    MainPlot::OnLeaveWindow)
    EVT_LEFT_DOWN (       MainPlot::OnLeftDown)
    EVT_LEFT_UP   (       MainPlot::OnLeftUp)
    EVT_RIGHT_DOWN (      MainPlot::OnRightDown)
    EVT_RIGHT_UP (        MainPlot::OnRightUp)
    EVT_MIDDLE_DOWN (     MainPlot::OnMiddleDown)
    EVT_MIDDLE_UP (       MainPlot::OnMiddleUp)
    EVT_KEY_DOWN   (      MainPlot::OnKeyDown)
    EVT_SIZE (            MainPlot::OnSize)
    EVT_MENU (ID_plot_popup_za,     MainPlot::OnZoomPopup)
    EVT_MENU_RANGE (ID_plot_popup_m, ID_plot_popup_m + 3, MainPlot::OnPopupMode)
    EVT_MENU_RANGE (ID_plot_popup_data, ID_plot_popup_tics,  
                                    MainPlot::OnPopupShowXX)
    EVT_MENU (ID_plot_popup_buffer, MainPlot::OnPopupBuffer)

    EVT_MENU (ID_plot_popup_conf_save, MainPlot::OnPopupConfSave)
    EVT_MENU (ID_plot_popup_alt_conf_save, MainPlot::OnPopupConfSave)
    EVT_MENU (ID_plot_popup_conf_reset, MainPlot::OnPopupConfReset)
    EVT_MENU (ID_plot_popup_alt_conf_reset, MainPlot::OnPopupConfReset)
    EVT_MENU (ID_plot_popup_conf_revert, MainPlot::OnPopupConfRevert)
    EVT_MENU (ID_plot_popup_alt_conf_revert, MainPlot::OnPopupConfRevert) 

    EVT_MENU_RANGE (ID_plot_popup_c_background, ID_plot_popup_c_phase - 1, 
                                    MainPlot::OnPopupColor)
    EVT_MENU_RANGE (ID_plot_popup_pt_size, ID_plot_popup_pt_size + max_radius, 
                                    MainPlot::OnPopupRadius)
    EVT_MENU_RANGE (ID_plot_zpm_prev, ID_plot_zpm_zall,    
                                    MainPlot::OnZoomPopup)
    EVT_MENU (ID_plot_popup_c_inv,  MainPlot::OnInvertColors)
END_EVENT_TABLE()

MainPlot::MainPlot (wxWindow *parent, Plot_common &comm) 
    : MyPlot (parent, comm), mode (norm_pm_ty),
      mouse_pressed(pr_ty_none), over_peak(-1)
{ read_settings(); }

void MainPlot::OnSize(wxSizeEvent& WXUNUSED(event))
{
    //Refresh();
}

void MainPlot::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    vert_line_following_cursor (cancel_ma_ty);
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
    return my_other->plus_background ? i->orig_y : i->y;
}

void MainPlot::Draw(wxDC &dc)
{
    set_scale();
    if (colourTextForeground.Ok())
        dc.SetTextForeground (colourTextForeground);
    if (colourTextBackground.Ok())
        dc.SetTextBackground (colourTextBackground);
    if (my_data->is_empty())
        return;
    vector<Point>::const_iterator f =my_data->get_point_at(my_other->view.left),
                                l = my_data->get_point_at(my_other->view.right);
    if (data_visible)
        draw_data(dc, y_of_data_for_draw_data);
    if (!a_copy4plot.empty() && size(a_copy4plot) != my_sum->count_a()) {
        a_copy4plot = my_sum->current_a();
        frame->SetStatusText ("Number of parameters changed.");
    }
    my_sum->use_param_a_for_value (a_copy4plot);
    if (x_axis_visible) 
        draw_x_axis (dc, f, l);
    if (tics_visible)
        draw_tics(dc, my_other->view, 7, 7, 4, 4);

/*
    if (common.buffer_enabled) {
        //TODO
        buffer_peaks (f, l);
        if (sum_visible)
            ;//draw_sum_from_buffer (dc);
        if (phases_visible)
            ;//draw_phases_from_buffer (dc);
        if (peaks_visible)
            ;//draw_peaks_from_buffer (dc);
        
    }
    else 
*/
    {
        if (sum_visible)
            draw_sum (dc, f, l);
        if (phases_visible)
            draw_phases (dc, f, l);
        if (peaks_visible)
            draw_peaks (dc, f, l);
    }
    prepare_peaktops();
    switch (mode) {
        case norm_pm_ty: break;
        case sim_pm_ty: draw_peaktops(dc); break;
        case bg_pm_ty: draw_background_points(dc); break;
    }
}

void MainPlot::draw_x_axis (wxDC& dc, vector<Point>::const_iterator first,
                                      vector<Point>::const_iterator last)
{
    dc.SetPen (xAxisPen);
    if (my_other->plus_background) {
        int X = 0, Y = 0;
        for (vector<Point>::const_iterator i = first; i < last; i++) {
            int X_ = X, Y_ = Y;
            X = common.x2X(i->x);
            Y = y2Y (i->get_bg());
            if (i != first)
                dc.DrawLine (X_, Y_, X, Y); 
        }
    }
    else
        dc.DrawLine (0, y2Y(0), GetClientSize().GetWidth(), y2Y(0));
}

void MyPlot::draw_tics (wxDC& dc, const Rect &v, 
                          const int x_max_tics, const int y_max_tics, 
                          const int x_tic_size, const int y_tic_size)
{
    dc.SetPen (xAxisPen);
    dc.SetFont(*wxSMALL_FONT);
    dc.SetTextForeground(xAxisPen.GetColour());
    fp x_tic_step = scale_tics_step(v.left, v.right, x_max_tics);
    for (fp x = x_tic_step * ceil(v.left / x_tic_step); x < v.right; 
            x += x_tic_step) {
        int X = common.x2X(x);
        int Y = y2Y(0);
        dc.DrawLine (X, Y, X, Y - x_tic_size);
        wxString label = S(x).c_str();
        wxCoord w, h;
        dc.GetTextExtent (label, &w, &h);
        dc.DrawText (label, X - w/2, Y + 1);
    }
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

fp MyPlot::get_max_abs_y (fp (*compute_y)(vector<Point>::const_iterator))
{
    vector<Point>::const_iterator 
                            first = my_data->get_point_at(my_other->view.left),
                            last = my_data->get_point_at(my_other->view.right);
    fp max_abs_y = 0;
    for (vector<Point>::const_iterator i = first; i < last; i++) {
        if (i->is_active) {
            fp y = fabs(((*compute_y)(i)));
            if (y > max_abs_y) max_abs_y = y;
        }
    }
    return max_abs_y;
}

void MyPlot::draw_data (wxDC& dc, 
                        fp (*compute_y)(vector<Point>::const_iterator))
{
    vector<Point>::const_iterator 
                            first = my_data->get_point_at(my_other->view.left),
                            last = my_data->get_point_at(my_other->view.right);
    //if (last - first < 0) return;
    bool active = !first->is_active;//causes pens to be initialized in main loop
    int X_ = 0, Y_ = 0;
    // first line segment -- lines should be drawed towards points 
    //                                                 that are outside of plot 
    if (line_between_points) {
        dc.SetPen(first->is_active ? activeDataPen : inactiveDataPen);
        if (first > my_data->points().begin()) {
            X_ = common.x2X (my_other->view.left);
            int Y_l = y2Y ((*compute_y)(first - 1));
            int Y_r = y2Y ((*compute_y)(first));
            int X_l = common.x2X ((first - 1)->x);
            int X_r = common.x2X (first->x);
            if (X_r == X_l)
                Y_ = Y_r;
            else
                Y_ = Y_l + (Y_r - Y_l) * (X_ - X_l) / (X_r - X_l);
        }
        else {
            X_ = common.x2X(first->x);
            Y_ = y2Y ((*compute_y)(first));
        }
    }

    //drawing all points (and lines); main loop
    for (vector<Point>::const_iterator i = first; i < last; i++) {
        int X = common.x2X(i->x);
        int Y = y2Y ((*compute_y)(i));
        if (X == X_ && Y == Y_) continue;
        if (i->is_active != active) {
            active = i->is_active;
            if (active) {
                dc.SetPen (activeDataPen);
                dc.SetBrush (activeDataBrush);
            }
            else {
                dc.SetPen (inactiveDataPen);
                dc.SetBrush (inactiveDataBrush);
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
    if (line_between_points && last < my_data->points().end()) {
        int X = common.x2X (my_other->view.right);
        int Y_l = y2Y ((*compute_y)(last - 1));
        int Y_r = y2Y ((*compute_y)(last));
        int X_l = common.x2X ((last - 1)->x);
        int X_r = common.x2X (last->x);
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
    common.buf = vector<vector<fp> > (p + 1, vector<fp>(last - first));
    int j = 0;
    for (vector<Point>::const_iterator i = first; i < last; i++, j++) {
        common.buf[0][j] = i->x;
        for (int k = 0; k < my_sum->fzg_size(fType); k++) 
            common.buf[k][j] = my_sum->f_value(i->x, k);
    }
    */
}

void MainPlot::draw_sum (wxDC& dc, vector<Point>::const_iterator first,
                                   vector<Point>::const_iterator last)
{
    dc.SetPen (sumPen);
    int X = 0, Y = 0;
    int Y_zero = y2Y(0);
    for (vector<Point>::const_iterator i = first; i < last; i++) {
        int X_ = X, Y_ = Y;
        X = common.x2X(i->x);
        Y = y2Y (my_other->plus_background ? sum_value(i) +  i->get_bg()
                                           : sum_value(i));
        if (i != first 
                && (X != X_ || Y != Y_) 
                && (Y_ != Y_zero || Y != Y_zero //not drawing on x axis
                    || my_other->plus_background))
            dc.DrawLine (X_, Y_, X, Y); 
    }
}

void MainPlot::draw_phases (wxDC& dc, vector<Point>::const_iterator first,
                                      vector<Point>::const_iterator last)
{
#ifdef USE_XTAL
    for (int k = 0; k < my_crystal->get_nr_of_phases(); k++) {
        dc.SetPen (phasePen[k % max_phase_pens]);
        vector<int>peaks = my_crystal->get_funcs_in_phase(k);
        int X = 0, Y = 0;
        int Y_zero = y2Y(0);
        for (vector<Point>::const_iterator i = first; i < last; i++) {
            int X_ = X, Y_ = Y;
            X = common.x2X (i->x);
            Y = y2Y(my_sum->funcs_value (peaks, i->x));
            if (i != first 
                    && (X != X_ || Y != Y_) 
                    && (Y_ != Y_zero || Y != Y_zero)) //not drawing on x axis
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
        int X = common.x2X (first->x), 
            Y = y2Y (my_sum->f_value(first->x, k));
        int Y_zero = y2Y(0);
        for (vector<Point>::const_iterator i = first + 1; i < last; i++) {
            int X_ = X, Y_ = Y;
            X = common.x2X (i->x);
            Y = y2Y (my_sum->f_value (i->x, k));
            if (Y_ != Y_zero || Y != Y_zero //not drawing on x axis
                    && (X != X_ || Y != Y_))
                dc.DrawLine (X_, Y_, X, Y );
        }
    }
}

void MainPlot::draw_peaktops (wxDC& dc)
{
    dc.SetPen (xAxisPen);
    dc.SetBrush (*wxTRANSPARENT_BRUSH);
    for (vector<wxPoint>::const_iterator i = peaktops.begin(); 
         i != peaktops.end(); i++) 
        dc.DrawRectangle (i->x - 1, i->y - 1, 3, 3);
    int n = (frame->simsum_tb ? frame->simsum_tb->get_selected_peak() : -1);
    draw_selected_peaktop(n);
}

void MainPlot::draw_selected_peaktop (int new_sel)
{
    static int sel = -1;
    wxClientDC dc(this);
    dc.SetBrush (*wxTRANSPARENT_BRUSH);
    dc.SetPen (xAxisPen);
    if (sel >= 0 && sel < size(peaktops))
        dc.DrawRectangle (peaktops[sel].x - 1, peaktops[sel].y - 1, 3, 3);
    sel = new_sel;
    dc.SetPen (*wxRED_PEN);
    if (sel >= 0 && sel < size(peaktops))
        dc.DrawRectangle (peaktops[sel].x - 1, peaktops[sel].y - 1, 3, 3);
    //TODO draw points at half of maximum  
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
    peaktops.clear();
    for (int k = 0; k < my_sum->fzg_size(fType); k++) {
        const V_f *f = static_cast<const V_f*>(my_sum->get_fzg (fType, k));
        int X, Y;
        if (f->is_peak()) {
            fp x = f->center();
            X = common.x2X (x - my_sum->zero_shift(x));
            Y = y2Y (f->height());
        }
        else {
            X = k * 10;
            Y = y2Y (my_sum->f_value(common.X2x(X), k));
        }
        if (Y < 0 || Y > H) Y = Y0;
        peaktops.push_back(wxPoint(X, Y));
    }
}

void MainPlot::show_peak_info (int n)
{
    exec_command ("s.info ^" + S(n));
}

void MainPlot::draw_background_points (wxDC& dc)
{
    const vector<B_point>& bg = my_data->get_background_points(bg_ty);
    dc.SetPen (bg_pointsPen);
    dc.SetBrush (*wxTRANSPARENT_BRUSH);
    for (vector<B_point>::const_iterator i = bg.begin(); i != bg.end(); i++) {
        dc.DrawCircle (common.x2X(i->x), y2Y(i->y), 3);
    }
}

void MainPlot::read_settings()
{
    wxConfigBase *cf = wxConfig::Get();
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
    activeDataBrush.SetColour (active_data_col);
    wxColour inactive_data_col = read_color_from_config (cf, "inactive_data",
                                                      wxColour (128, 128, 128));
    inactiveDataPen.SetColour (inactive_data_col);
    inactiveDataBrush.SetColour (inactive_data_col);
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
    peaks_visible = read_bool_from_config (cf, "peaks", false); 
    phases_visible = read_bool_from_config (cf, "phases", false);  
    sum_visible = read_bool_from_config (cf, "sum", true);
    data_visible = read_bool_from_config (cf, "data", true); 
    x_axis_visible = read_bool_from_config (cf, "xAxis", true);  
    tics_visible = read_bool_from_config (cf, "tics", true);
    cf->SetPath("/MainPlot");
    point_radius = cf->Read ("point_radius", 1);
    line_between_points = read_bool_from_config(cf,"line_between_points",false);
}

void MainPlot::save_settings()
{
    wxConfigBase *cf = wxConfig::Get();
    cf->SetPath("/MainPlot");
    cf->Write ("point_radius", point_radius);
    cf->Write ("line_between_points", line_between_points);

    cf->SetPath("/MainPlot/Colors");
    write_color_to_config (cf, "text_fg", colourTextForeground);
    write_color_to_config (cf, "text_bg", colourTextBackground);
    write_color_to_config (cf, "bg", backgroundBrush.GetColour());
    write_color_to_config (cf, "active_data", activeDataBrush.GetColour());
    write_color_to_config (cf, "inactive_data", inactiveDataBrush.GetColour());
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
    cf->Write ("peaks", peaks_visible);
    cf->Write ("phases", phases_visible);
    cf->Write ("sum", sum_visible);
    cf->Write ("data", data_visible);
    cf->Write ("xAxis", x_axis_visible);
    cf->Write ("tics", tics_visible);
}

void MainPlot::OnLeaveWindow (wxMouseEvent& WXUNUSED(event))
{
    frame->SetStatusText ("", 1);
    vert_line_following_cursor (cancel_ma_ty);
}

void MainPlot::set_scale()
{
    const Rect &v = my_other->view;
    common.xUserScale = GetClientSize().GetWidth() / (v.right - v.left);
    int H =  GetClientSize().GetHeight();
    fp h = v.top - v.bottom;
    fp label_width = tics_visible && v.bottom <= 0 ?max(v.bottom * H/h + 12, 0.)
                                                   : 0;
    yUserScale = - (H - label_width) / h;
    common.xLogicalOrigin = v.left;
    yLogicalOrigin = v.top;
    common.plot_y_scale = yUserScale;
}
 
void MainPlot::show_popup_menu (wxMouseEvent &event)
{
    wxMenu popup_menu; //("main plot menu");

    popup_menu.Append(ID_plot_popup_za, "Zoom &All");
    popup_menu.AppendSeparator();
    wxMenu *mode_menu = new wxMenu;
    mode_menu->AppendRadioItem (ID_plot_popup_m + 0, "&Normal", "");
    mode_menu->AppendRadioItem (ID_plot_popup_m + 1, "&Background", "");
    mode_menu->AppendRadioItem (ID_plot_popup_m + 2, "&Simple", "");
    popup_menu.Append (wxNewId(), "&Mode", mode_menu);
    mode_menu->Check (ID_plot_popup_m + mode, true);
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

    /*
    popup_menu.Append (ID_plot_popup_buffer, "&Buffer data", 
                        "Keeping data in memory makes redrawing faster", true);
    popup_menu.Check (ID_plot_popup_buffer, common.buffer_enabled);
    */
    wxMenu *size_menu = new wxMenu;
    size_menu->AppendCheckItem (ID_plot_popup_pt_size, "&Line", "");
    size_menu->Check (ID_plot_popup_pt_size, line_between_points);
    size_menu->AppendSeparator();
    for (int i = 1; i <= max_radius; i++) 
        size_menu->AppendRadioItem (ID_plot_popup_pt_size + i, 
                                    wxString::Format ("&%d", i), "");
    size_menu->Check (ID_plot_popup_pt_size + point_radius, true);
    popup_menu.Append (wxNewId(), "Data point si&ze",size_menu);
    
    popup_menu.AppendSeparator();
    wxMenu *config_menu = new wxMenu;
    config_menu->Append (ID_plot_popup_conf_save, "&Save");
    config_menu->Append (ID_plot_popup_conf_revert, "&Read");
    config_menu->Append (ID_plot_popup_conf_reset, "Rese&t");
    wxMenu *alt_config_menu = new wxMenu;
    alt_config_menu->Append (ID_plot_popup_alt_conf_save, "&Save");
    alt_config_menu->Append (ID_plot_popup_alt_conf_revert, "&Read");
    alt_config_menu->Append (ID_plot_popup_alt_conf_reset, "Rese&t");
    popup_menu.Append (wxNewId(), "Configuration &1", config_menu);  
    popup_menu.Append (wxNewId(), "Configuration &2", alt_config_menu);  

    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void MainPlot::show_zoom_popup_menu (wxMouseEvent &event)
{
    wxMenu zoom_pm ("main plot zoom menu"); 
    for (int i = common.zoom_hist.size() - 1; i >= 0; i--) 
        zoom_pm.Append (ID_plot_zpm_prev + i, common.zoom_hist[i].c_str());
    zoom_pm.AppendSeparator();
    zoom_pm.Append (ID_plot_zpm_fitv, "&fit vertically");
    zoom_pm.Append (ID_plot_zpm_zall, "&zoom all");
    PopupMenu (&zoom_pm, event.GetX(), event.GetY()); 
}

// mouse usage
//
//  Legend:
//   1 = left      ||     A = no Ctrl/Alt/Shift buttons 
//   2 = middle    ||     B = Alt or Ctrl
//   3 = right     ||     C = Shift
//  ----------------------------------
//  Middle button emulation: 1C = 2, 1BC = 2B
//  ---
//  Usage:
//   1A   -- mode dependent 
//   1B*  -- mode dependent
//   2A*  -- rectangle zoom
//   2B   -- unused
//   3A   -- popup menu
//   3B*  -- mode dependent
//   3C   -- zoom popup menu
//  -----
//   * = both pressing and releasing coordinates are used.
//  Simple Rules:
//   1. When one button is down, pressing other button cancels action 
//   2. Releasing/keeping down/pressing Ctrl/Alt/Shift keys, when you keep 
//     mouse button down makes no difference.

void MainPlot::OnMouseMove(wxMouseEvent &event)
{
    //display coords in status bar 
    fp x = common.X2x (event.GetX());
    fp y = Y2y (event.GetY());
    if (my_other->plus_background && mode == bg_pm_ty)
        y -= my_data->get_bg_at (x);
    wxString str;
    str.Printf ("%.3f  %d", x, static_cast<int>(y + 0.5));
    frame->SetStatusText (str, 1);

    //other actions
    switch (mouse_pressed) {
        case pr_ty_none:
            if (event.AltDown() || event.ControlDown())
                vert_line_following_cursor (move_ma_ty, event.GetX());
            else
                vert_line_following_cursor (cancel_ma_ty);
            look_for_peaktop (event);
            break;
        case pr_ty_1B:
            if (mode == norm_pm_ty)
                vert_line_following_cursor (move_ma_ty, event.GetX());
            else if (mode == bg_pm_ty) //unused
                vert_line_following_cursor (cancel_ma_ty);
            else if (mode == sim_pm_ty) {
                vert_line_following_cursor (move_ma_ty, event.GetX());
            }
            break;
        case pr_ty_3B:
            if (mode == norm_pm_ty)
                vert_line_following_cursor (move_ma_ty, event.GetX());
            else if (mode == bg_pm_ty) //unused
                vert_line_following_cursor (cancel_ma_ty);
            else if (mode == sim_pm_ty) {
                vert_line_following_cursor (cancel_ma_ty);
                peak_draft (move_ma_ty, event);
            }
            break;
        case pr_ty_2A:
        case pr_ty_1Awas:
            rect_zoom (move_ma_ty, event);
            break;
        case pr_ty_1A:
            break;
        default: assert(0);
    }
}

void MainPlot::look_for_peaktop (wxMouseEvent& event)
{
    // searching the closest peak-top and distance from it, d = dx + dy < 10 
    int min_dist = 10;
    int nearest = -1;
    for (vector<wxPoint>::const_iterator i = peaktops.begin(); 
         i != peaktops.end(); i++) {
        int d = abs(event.GetX() - i->x) + abs(event.GetY() - i->y);
        if (d < min_dist) {
            min_dist = d;
            nearest = i - peaktops.begin();
        }
    }
    //changing cursor and statusbar text, if needed.
    if (over_peak == nearest) return;
    //else:
    over_peak = nearest;
    if (nearest != -1) {
        const V_f* f=static_cast<const V_f*>(my_sum->get_fzg(fType, over_peak));
        string s = f->short_type() + S(over_peak) + " ";  
        string ref = my_sum->descr_refs_to_f(over_peak);
        if (!ref.empty())
            s += "(" + ref + ")";
        s += " " + f->type_info()->name 
            + " " + my_sum->info_fzg_parameters(fType, over_peak, true)
            + " " + f->extra_description();
        frame->SetStatusText (s.c_str());
        SetCursor (wxCURSOR_CROSS);
    }
    else { //was over peak, but now is not 
        frame->SetStatusText ("");
        SetCursor (wxCURSOR_ARROW);
    }
}

void MainPlot::cancel_mouse_press()
{
    //if middle or right button already pressed -- cancel
    if (mouse_pressed == pr_ty_2A) {// if middle button was pressed
        rect_zoom (cancel_ma_ty); 
    }
    else if (mouse_pressed == pr_ty_1B 
             || mouse_pressed == pr_ty_3B && mode == norm_pm_ty) {
        draw_dashed_vert_lines (mouse_press_X);
        frame->SetStatusText ("");
    }
    else if (mouse_pressed == pr_ty_3B && mode == sim_pm_ty) {
        peak_draft (cancel_ma_ty);
        frame->SetStatusText ("");
    }
    else if (mouse_pressed == pr_ty_1Awas) {
        rect_zoom (cancel_ma_ty); 
        mouse_press_X = mouse_press_Y = INVALID;
        frame->SetStatusText ("");
    }
    mouse_pressed = pr_ty_none;
    SetCursor (wxCURSOR_ARROW);
}

void MainPlot::OnLeftDown (wxMouseEvent &event)
{
    if (event.ShiftDown()) { //if Shift -- emulate middle button 
        OnMiddleDown (event);
        return;
    }
    if (mouse_pressed != pr_ty_none && mouse_pressed != pr_ty_1Awas) {
        cancel_mouse_press();
        return;
    }

    bool ctrl = (event.AltDown() || event.ControlDown());
    fp x = common.X2x (event.GetX());
    switch (mode) {
        case norm_pm_ty:
            if (ctrl) { //d.range
                mouse_press_X = event.GetX();
                draw_dashed_vert_lines (mouse_press_X);
                mouse_pressed = pr_ty_1B;
                SetCursor(wxCURSOR_SIZEWE);
                frame->SetStatusText("Select data range to activate...");
            }
            else if (over_peak >= 0) {
                show_peak_info(over_peak);
            }
            else {//ctrl not pressed, not over peak 
                if (mouse_pressed != pr_ty_1Awas) {
                    mouse_press_X = event.GetX();
                    mouse_press_Y = event.GetY();
                    mouse_pressed = pr_ty_1A;
                }
                else { 
                    SetCursor(wxCURSOR_ARROW);
                    frame->SetStatusText ("");
                    rect_zoom (stop_ma_ty, event);
                    mouse_pressed = pr_ty_none;
                }
            }
            break;
        case bg_pm_ty:
            if (ctrl) {
                exec_command ("d.background  ! " + S(x));
            }
            else {
                fp y = Y2y (event.GetY());
                exec_command ("d.background  " + S(x) + "  " + S(y)); 
            }
            break;
        case sim_pm_ty:
            if (ctrl) {
                mouse_press_X = event.GetX();
                draw_dashed_vert_lines (mouse_press_X);
                mouse_pressed = pr_ty_1B;
                SetCursor(wxCURSOR_SIZEWE);
                frame->SetStatusText("Select range to add a peak in it..."); 
            }
            else if (over_peak >= 0) {
                frame->simsum_tb->set_peakspin_value (over_peak);
            }
            else {//ctrl not pressed, not over peak 
                fp y = Y2y(event.GetY());
                bool r = frame->simsum_tb->left_button_clicked (x, y);
                if (!r) perhaps_it_was_silly_zoom_try();
            }
            break;
        default: assert(0);
    }
}

void MainPlot::OnLeftUp (wxMouseEvent &event)
{
    if (event.ShiftDown()) {//if Shift -- emulate middle button 
        OnMiddleUp (event);
        return;
    }
    if (mouse_pressed == pr_ty_1A) {
        if (abs(event.GetX() - mouse_press_X) 
            + abs(event.GetY() - mouse_press_Y) > 20)  // cancel
            mouse_pressed = pr_ty_none;
        else {
            SetCursor(wxCURSOR_MAGNIFIER);
            frame->SetStatusText ("Select second corner to zoom...");
            mouse_pressed = pr_ty_1Awas;
            rect_zoom (start_ma_ty, event);
        }
    }
    if (mouse_pressed == pr_ty_1B) {
        SetCursor(wxCURSOR_ARROW);
        frame->SetStatusText ("");
        mouse_pressed = pr_ty_none;
        draw_dashed_vert_lines (mouse_press_X);
        if (abs (event.GetX() - mouse_press_X) < 6) { // cancel
            return;
        }
        fp xmin = common.X2x (min (event.GetX(), mouse_press_X));
        fp xmax = common.X2x (max (event.GetX(), mouse_press_X));
        switch (mode) {
            case norm_pm_ty:
                exec_command ("d.range + [ " + S(xmin) + " : " + S(xmax)+" ]");
                break;
            case sim_pm_ty:
                frame->simsum_tb->add_peak_in_range (xmin, xmax);
                break;
            default: assert(0);
        }
    }
}

void MainPlot::OnRightDown (wxMouseEvent &event)
{
    if (mouse_pressed != pr_ty_none) {
        cancel_mouse_press();
        return;
    }
    if (event.AltDown() || event.ControlDown()) { //3B
        switch (mode) {
            case norm_pm_ty:
                mouse_press_X = event.GetX();
                draw_dashed_vert_lines (mouse_press_X);
                mouse_pressed = pr_ty_3B;
                SetCursor(wxCURSOR_SIZEWE);
                frame->SetStatusText("Select data range to disactivate...");
                break;
            case bg_pm_ty:
                //unused
                break;
            case sim_pm_ty:
                mouse_press_X = event.GetX();
                peak_draft (start_ma_ty, event);
                mouse_pressed = pr_ty_3B;
                SetCursor(wxCURSOR_SIZING);
                frame->SetStatusText("Add drawed peak...");
                break;
        }
    }
    else if (event.ShiftDown()) { //3C
        vert_line_following_cursor (cancel_ma_ty);
        show_zoom_popup_menu (event);
    }
    else { //3A 
        vert_line_following_cursor (cancel_ma_ty);
        show_popup_menu (event);
    }
}

void MainPlot::OnRightUp (wxMouseEvent &event)
{
    if (mouse_pressed == pr_ty_3B) {
        mouse_pressed = pr_ty_none;
        switch (mode) {
            case norm_pm_ty:
                {
                int xmin = min (event.GetX(), mouse_press_X);
                int xmax = max (event.GetX(), mouse_press_X);
                draw_dashed_vert_lines (mouse_press_X);
                SetCursor(wxCURSOR_ARROW);
                frame->SetStatusText("");
                if (xmax - xmin > 5)  //else cancel
                    exec_command ("d.range - [ " + S(common.X2x(xmin)) + " : " 
                                    + S(common.X2x(xmax)) + " ]");
                break;
                }
            case bg_pm_ty:
                //unused
                break;
            case sim_pm_ty:
                SetCursor(wxCURSOR_ARROW);
                frame->SetStatusText("");
                peak_draft (stop_ma_ty, event);
                break;
        }
    }
}

void MainPlot::OnMiddleDown (wxMouseEvent &event)
{
    if (mouse_pressed != pr_ty_none) {
        cancel_mouse_press();
        return;
    }
    SetCursor(wxCURSOR_MAGNIFIER);  
    frame->SetStatusText ("Select second corner and release button to zoom...");
    rect_zoom (start_ma_ty, event);
    mouse_pressed = pr_ty_2A;
}

void MainPlot::OnMiddleUp (wxMouseEvent &event)
{
    frame->SetStatusText ("");
    SetCursor(wxCURSOR_ARROW);
    rect_zoom (stop_ma_ty, event);
    mouse_pressed = pr_ty_none;
}

void MainPlot::OnKeyDown (wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_ESCAPE) {
        cancel_mouse_press(); 
    }
    else if (event.GetKeyCode() == ' ' || event.GetKeyCode() == WXK_TAB) {
        cancel_mouse_press(); 
        frame->get_input_combo()->SetFocus();
    }
    else
        event.Skip();
}

void MainPlot::peak_draft (Mouse_act_enum ma, wxMouseEvent &event)
{
    static wxPoint prev(INVALID, INVALID);
    if (prev.x != INVALID)
        draw_peak_draft (mouse_press_X, prev.x, prev.y);
    prev.x = prev.y = INVALID;
    switch (ma) {
        case start_ma_ty:
        case move_ma_ty:
            prev.x = event.GetX(), prev.y = event.GetY();
            draw_peak_draft (mouse_press_X, prev.x, prev.y);
            break;
        case stop_ma_ty:
            if (!frame->simsum_tb) //should not happen
                break;
            if (abs (event.GetX() - mouse_press_X) < 5) //cancel
                return;
            frame->simsum_tb->add_peak (Y2y(event.GetY()), 
                                        common.X2x(mouse_press_X), 
                                        fabs(common.X2x(mouse_press_X) 
                                             - common.X2x(event.GetX())));
            break;
        case cancel_ma_ty:
            break;
        default: assert(0);
    }
}

void MainPlot::draw_peak_draft (int X_mid, int X_b, int Y)
{
    if (X_mid == INVALID || X_b == INVALID || Y == INVALID)
        return;
    wxClientDC dc(this);
    dc.SetLogicalFunction (wxINVERT);
    dc.SetPen(*wxBLACK_DASHED_PEN);
    int X_hwhm = X_mid - X_b; //sign of X_hwhm doesn't matter 
    int y0 = GetClientSize().GetHeight();
    dc.DrawLine (X_mid, y0, X_mid, Y);
    dc.DrawLine (X_mid - X_hwhm, (Y+y0)/2, X_mid + X_hwhm, (Y+y0)/2);
    dc.DrawLine (X_mid, Y, X_mid - 2 * X_hwhm, y0);
    dc.DrawLine (X_mid, Y, X_mid + 2 * X_hwhm, y0);
}

bool MainPlot::rect_zoom (Mouse_act_enum ma, wxMouseEvent &event) 
{
    static int X1 = INVALID, Y1 = INVALID, X2 = INVALID, Y2 = INVALID;
    switch (ma) {
        case start_ma_ty:
            rect_zoom (cancel_ma_ty, event);
            X1 = X2 = event.GetX();
            Y1 = Y2 = event.GetY();
            draw_rect (X1, Y1, X2, Y2);
            CaptureMouse();
            break;
        case stop_ma_ty: 
            {
            if (X1 == INVALID || Y1 == INVALID)
                return false;
            X2 = event.GetX(), Y2 = event.GetY(); 
            int Xmin = min (X1, X2), Xmax = max (X1, X2);
            int width = Xmax - Xmin;
            int Ymin = min (Y1, Y2), Ymax = max (Y1, Y2);
            int height = Ymax - Ymin;
            draw_rect (X1, Y1, X2, Y2);
            if (width > 5 && height > 5 && X1 != INVALID && Y1 != INVALID) {
                change_zoom ("[ " + S(common.X2x(Xmin)) + " : " 
                             + S(common.X2x(Xmax)) + " ] [ " + S(Y2y(Ymax)) 
                             + " : " + S(Y2y(Ymin)) + " ]");
                                           //Y2y(Ymax) < Y2y(Ymin)
            }
            X1 = Y1 = X2 = Y2 = INVALID;
            ReleaseMouse();
            }
            break;
        case move_ma_ty:
            if (X1 == INVALID || Y1 == INVALID)
                return false;
            draw_rect (X1, Y1, X2, Y2);
            X2 = event.GetX(), Y2 = event.GetY(); 
            draw_rect (X1, Y1, X2, Y2);
            break;
        case cancel_ma_ty:
            if (X1 == INVALID && Y1 == INVALID) 
                return false;
            draw_rect (X1, Y1, X2, Y2);
            X1 = Y1 = X2 = Y2 = INVALID;
            ReleaseMouse();
            break;
        default:
            assert(0);
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

void MainPlot::OnPopupMode (wxCommandEvent& event)
{
    if (event.GetId() == ID_plot_popup_m + 0)
        frame->set_mode (norm_pm_ty);
    else if (event.GetId() == ID_plot_popup_m + 1)
        frame->set_mode (bg_pm_ty);
    else if (event.GetId() == ID_plot_popup_m + 2) 
        frame->set_mode (sim_pm_ty);
    else
        assert(0);
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
        default: assert(0);
    }
    Refresh(false);
}

void MainPlot::OnPopupBuffer(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox ("This menu item is not working yet.");//TODO

    if (common.buffer_enabled) {
        common.buffer_enabled = false;
        common.buf.clear();
    }
    else {
        //TODO
        common.buffer_enabled = true;
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
        brush = &activeDataBrush;
        pen = &activeDataPen;
    }
    else if (n == ID_plot_popup_c_inactive_data) {
        brush = &inactiveDataBrush;
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
    activeDataBrush.SetColour (invert_colour (activeDataBrush.GetColour()));
    activeDataPen.SetColour (activeDataBrush.GetColour());
    inactiveDataBrush.SetColour (invert_colour (inactiveDataBrush.GetColour()));
    inactiveDataPen.SetColour (inactiveDataBrush.GetColour());
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

void MainPlot::OnPopupConfSave (wxCommandEvent& event)
{
    delete wxConfigBase::Set((wxConfigBase *) 0);
    wxString name = (event.GetId() == ID_plot_popup_conf_save ? 
                                                wxGetApp().conf_filename 
                                              : wxGetApp().alt_conf_filename);
    wxConfig::Set (new wxConfig("", "", name, "", wxCONFIG_USE_LOCAL_FILE));
    frame->save_all_settings();
    delete wxConfig::Set (new wxConfig("", "", wxGetApp().conf_filename, "", 
                                       wxCONFIG_USE_LOCAL_FILE));
}

void MainPlot::OnPopupConfRevert (wxCommandEvent& event)
{
    delete wxConfigBase::Set((wxConfigBase *) 0);
    wxString name = (event.GetId() == ID_plot_popup_conf_revert ? 
                                                wxGetApp().conf_filename 
                                              : wxGetApp().alt_conf_filename);
    wxConfig::Set (new wxConfig("", "", name, "", wxCONFIG_USE_LOCAL_FILE));
    frame->read_all_settings();
    delete wxConfig::Set (new wxConfig("", "", wxGetApp().conf_filename, "", 
                                       wxCONFIG_USE_LOCAL_FILE));
}

void MainPlot::OnPopupConfReset (wxCommandEvent& event)
{
    int r = wxMessageBox ("Do you want to clear your configuration \n"
                          "and use default settings?",
                          "Are you sure?", 
                          wxYES_NO | wxCANCEL | wxCENTRE | wxICON_QUESTION);
    if (r != wxYES)
        return;
    wxString name = (event.GetId() == ID_plot_popup_conf_reset ? 
                                                wxGetApp().conf_filename 
                                              : wxGetApp().alt_conf_filename);
    wxConfig *config = new wxConfig("", "", name, "", wxCONFIG_USE_LOCAL_FILE);
    config->DeleteAll();
    frame->read_all_settings();
}

void MainPlot::OnZoomPopup (wxCommandEvent& event)
{
    int id = event.GetId();
    string s;
    int n = -1;
    if (id == ID_plot_zpm_fitv)
        s = "[" + S(my_other->view.left) +" : "+ S(my_other->view.right) + "]";
    else if (id == ID_plot_zpm_zall || id == ID_plot_popup_za)
        s = "[]";
    else {
        n = id - ID_plot_zpm_prev;
        s = common.zoom_hist[n];
    }
    change_zoom (s);
    if (n != -1) //back in zoom history
        common.zoom_hist.erase (common.zoom_hist.begin() + n, 
                                common.zoom_hist.end());
}

//===============================================================
//                           DiffPlot (difference plot) 
//===============================================================
BEGIN_EVENT_TABLE (DiffPlot, MyPlot)
    EVT_PAINT (           DiffPlot::OnPaint)
    EVT_MOTION (          DiffPlot::OnMouseMove)
    EVT_LEAVE_WINDOW (    DiffPlot::OnLeaveWindow)
    EVT_LEFT_DOWN (       DiffPlot::OnLeftDown)
    EVT_LEFT_UP (         DiffPlot::OnLeftUp)
    EVT_RIGHT_DOWN (      DiffPlot::OnRightDown)
    EVT_MIDDLE_DOWN (     DiffPlot::OnMiddleDown)
    EVT_KEY_DOWN   (      DiffPlot::OnKeyDown)
    EVT_SIZE (            DiffPlot::OnSize)
    EVT_MENU_RANGE (ID_diff_popup_plot_0, ID_diff_popup_plot_0 + 10,
                                                    DiffPlot::OnPopupPlot)
    EVT_MENU_RANGE (ID_diff_popup_c_background, ID_diff_popup_color - 1, 
                                                      DiffPlot::OnPopupColor)
    EVT_MENU (ID_diff_popup_yz_change, DiffPlot::OnPopupYZoom)
    EVT_MENU (ID_diff_popup_yz_fit, DiffPlot::OnPopupYZoomFit)
    EVT_MENU (ID_diff_popup_yz_auto, DiffPlot::OnPopupYZoomAuto)
END_EVENT_TABLE()

void DiffPlot::OnSize(wxSizeEvent& WXUNUSED(event))
{
    //Refresh();
}

void DiffPlot::OnPaint(wxPaintEvent &WXUNUSED(event))
{
    vert_line_following_cursor (cancel_ma_ty);
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
    return (i->y - sum_value(i)) / i->y * 100;
}

void DiffPlot::Draw(wxDC &dc)
{
    if (auto_zoom_y)
        fit_y_zoom();
    set_scale();
    if (my_data->is_empty()) return;
    if (kind == empty_ty) return;
    dc.SetPen (xAxisPen);
    if (kind == peak_pos_ty) {
        //TODO
        return;
    }

    if (x_axis_visible) {
        dc.DrawLine (0, y2Y(0), GetClientSize().GetWidth(), y2Y(0));
        if (kind == diff_ty) draw_zoom_text(dc);
    }
    if (tics_visible) {
        Rect v (0, 0, Y2y(GetClientSize().GetHeight()), Y2y(0));
        draw_tics(dc, v, 0, 5, 0, 4);
    }

    if (kind == diff_ty)
        draw_data (dc, diff_of_data_for_draw_data);
    else if (kind == diff_stddev_ty)
        draw_data (dc, diff_stddev_of_data_for_draw_data);
    else if (kind == diff_y_proc_ty)
        draw_data (dc, diff_y_proc_of_data_for_draw_data);
}

void DiffPlot::draw_zoom_text(wxDC& dc)
{
    dc.SetTextForeground(xAxisPen.GetColour());
    dc.SetFont(*wxNORMAL_FONT);  
    string s = "x" + S(y_zoom);  
    wxCoord w, h;
    dc.GetTextExtent (s.c_str(), &w, &h); 
    dc.DrawText (s.c_str(), dc.MaxX() - w - 2, 2);
}

void DiffPlot::OnMouseMove(wxMouseEvent &event)
{
    vert_line_following_cursor (move_ma_ty, event.GetX());
    fp x = common.X2x (event.GetX());
    fp y = Y2y (event.GetY()); 
    wxString str;
    str.Printf ("%.3f  [%d]", x, static_cast<int>(y + 0.5));
    frame->SetStatusText (str, 1);
    wxCursor new_cursor;
    if (event.GetX() < move_plot_margin_width)
        new_cursor = wxCURSOR_POINT_LEFT;
    else if (event.GetX() > GetClientSize().GetWidth() - move_plot_margin_width)
        new_cursor = wxCURSOR_POINT_RIGHT;
    else
        new_cursor = wxCURSOR_CROSS;
    if (new_cursor != cursor) {
        cursor = new_cursor;
        SetCursor (new_cursor);
    }
}

void DiffPlot::OnLeaveWindow (wxMouseEvent& WXUNUSED(event))
{
    frame->SetStatusText ("", 1);
    vert_line_following_cursor (cancel_ma_ty);
}

void DiffPlot::set_scale()
{
    switch (kind) {
        case empty_ty:
        case peak_pos_ty:
            yUserScale = 1.; //y scale doesn't matter
            break; 
        case diff_ty: 
            yUserScale = common.plot_y_scale * y_zoom;
            break;
        case diff_stddev_ty:
        case diff_y_proc_ty:
            yUserScale = -1. * y_zoom_base * y_zoom;
            break;
    }
    int h = GetClientSize().GetHeight();
    yLogicalOrigin = - h / 2. / yUserScale;
}
 
void DiffPlot::read_settings()
{
    wxConfigBase *cf = wxConfig::Get();
    cf->SetPath("/AuxPlot");

    kind = static_cast <Aux_plot_kind_enum> (cf->Read ("kind", diff_ty));
    auto_zoom_y = false;
    line_between_points = read_bool_from_config(cf,"line_between_points", true);
    point_radius = cf->Read ("point_radius", 1);
    cf->SetPath("/AuxPlot/Visible");
    x_axis_visible = read_bool_from_config (cf, "xAxis", true);
    tics_visible = read_bool_from_config (cf, "tics", true);
    cf->SetPath("/AuxPlot/Colors");
    //backgroundBrush = *wxBLACK_BRUSH;
    backgroundBrush.SetColour (read_color_from_config (cf, "bg", 
                                                       wxColour(50, 50, 50)));
    wxColour active_data_col = read_color_from_config (cf, "active_data",
                                                       wxColour ("GREEN"));
    activeDataPen.SetColour (active_data_col);
    //activeDataPen.SetStyle (wxDOT);
    activeDataBrush.SetColour (active_data_col);
    wxColour inactive_data_col = read_color_from_config (cf, "inactive_data",
                                                      wxColour (128, 128, 128));
    inactiveDataPen.SetColour (inactive_data_col);
    inactiveDataBrush.SetColour (inactive_data_col);
    xAxisPen.SetColour (read_color_from_config(cf, "xAxis", wxColour("WHITE")));
}

void DiffPlot::save_settings()
{
    wxConfigBase *cf = wxConfig::Get();

    cf->SetPath("/AuxPlot");
    cf->Write ("kind", kind); 
    cf->Write ("line_between_points", line_between_points);
    cf->Write ("point_radius", point_radius);

    cf->SetPath("/AuxPlot/Visible");
    cf->Write ("xAxis", x_axis_visible);
    cf->Write ("tics", tics_visible);

    cf->SetPath("/AuxPlot/Colors");
    write_color_to_config (cf, "bg", backgroundBrush.GetColour()); 
    write_color_to_config (cf, "active_data", activeDataBrush.GetColour());
    write_color_to_config (cf, "inactive_data", inactiveDataBrush.GetColour());
    write_color_to_config (cf, "xAxis", xAxisPen.GetColour());
}

void DiffPlot::OnLeftDown (wxMouseEvent &event)
{
    cancel_mouse_left_press();
    if (event.ShiftDown()) { // the same as OnMiddleDown()
        change_zoom ("[]");
        return;
    }
    int X = event.GetPosition().x;
    // if mouse pointer is near to left or right border, move view
    if (X < move_plot_margin_width) 
        move_view_horizontally (true);  // <--
    else if (X > GetClientSize().GetWidth() - move_plot_margin_width) 
        move_view_horizontally (false); // -->
    else {
        draw_dashed_vert_lines (X);
        mouse_press_X = X;
        SetCursor(wxCURSOR_SIZEWE);  
        frame->SetStatusText ("Select x range and release button to zoom..."); 
        CaptureMouse();
    }
}

bool DiffPlot::cancel_mouse_left_press()
{
    if (mouse_press_X != INVALID) {
        draw_dashed_vert_lines (mouse_press_X);
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

void DiffPlot::OnLeftUp (wxMouseEvent &event)
{
    if (mouse_press_X == INVALID)
        return;
    int xmin = min (event.GetX(), mouse_press_X);
    int xmax = max (event.GetX(), mouse_press_X);
    cancel_mouse_left_press();

    if (xmax - xmin < 5) //cancel
        return;
    change_zoom ("[" + S(common.X2x(xmin)) + " : " + S(common.X2x(xmax)) + "]");
}

//popup-menu
void DiffPlot::OnRightDown (wxMouseEvent &event)
{
    if (cancel_mouse_left_press())
        return;

    wxMenu popup_menu ("aux. plot menu");
    wxMenu *kind_menu = new wxMenu;
    kind_menu->AppendRadioItem (ID_diff_popup_plot_0 + 0, "&empty", "nothing");
    kind_menu->AppendRadioItem (ID_diff_popup_plot_0 + 1, "&diff", "y_d - y_s");
    kind_menu->AppendRadioItem (ID_diff_popup_plot_0 + 2, "&weighted diff", 
                                "(y_d - y_s) / sigma");
    kind_menu->AppendRadioItem (ID_diff_popup_plot_0 + 3, "&proc diff", 
                                "(y_d - y_s) / y_d [%]");
    //kind_menu->AppendRadioItem (ID_diff_popup_plot_0 + 4, "p&eak positions", 
    //                            "mark centers of peaks");
    popup_menu.Append (wxNewId(), "&Plot", kind_menu);
    kind_menu->Check (ID_diff_popup_plot_0 + kind, true);
    popup_menu.AppendSeparator();
    wxMenu *color_menu = new wxMenu;
    color_menu->Append (ID_diff_popup_c_background, "&Background");
    color_menu->Append (ID_diff_popup_c_active_data, "&Active Data");
    color_menu->Append (ID_diff_popup_c_inactive_data, "&Inactive Data");
    color_menu->Append (ID_diff_popup_c_axis, "&X Axis");
    popup_menu.Append (ID_diff_popup_color, "&Color", color_menu);
    wxMenu *yzoom_menu = new wxMenu;
    yzoom_menu->Append (ID_diff_popup_yz_fit, "&Fit to window");
    yzoom_menu->Append (ID_diff_popup_yz_change, "&Change");
    yzoom_menu->AppendCheckItem (ID_diff_popup_yz_auto, "&Auto-fit");
    yzoom_menu->Check (ID_diff_popup_yz_auto, auto_zoom_y);
    popup_menu.Append (ID_diff_popup_y_zoom, "Y &zoom", yzoom_menu);
    popup_menu.Enable (ID_diff_popup_y_zoom, 
                                !(kind == empty_ty || kind == peak_pos_ty));
    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void DiffPlot::OnMiddleDown (wxMouseEvent& WXUNUSED(event))
{
    if (cancel_mouse_left_press())
        return;
    change_zoom ("[]");
}

void DiffPlot::OnKeyDown (wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_ESCAPE) {
        cancel_mouse_left_press();
    }
    else if (event.GetKeyCode() == ' ' || event.GetKeyCode() == WXK_TAB) {
        cancel_mouse_left_press();
        frame->get_input_combo()->SetFocus();
    }
    else
        event.Skip();
}

void DiffPlot::OnPopupPlot (wxCommandEvent& event)
{
    kind = static_cast<Aux_plot_kind_enum>(event.GetId()-ID_diff_popup_plot_0);
    fit_y_zoom();
}

void DiffPlot::OnPopupColor (wxCommandEvent& event)
{
    wxBrush *brush = 0;
    wxPen *pen = 0;
    int n = event.GetId();
    if (n == ID_diff_popup_c_background)
        brush = &backgroundBrush;
    else if (n == ID_diff_popup_c_active_data) {
        brush = &activeDataBrush;
        pen = &activeDataPen;
    }
    else if (n == ID_diff_popup_c_inactive_data) {
        brush = &inactiveDataBrush;
        pen = &inactiveDataPen;
    }
    else if (n == ID_diff_popup_c_axis)
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

void DiffPlot::OnPopupYZoom (wxCommandEvent& WXUNUSED(event))
{
    int r = wxGetNumberFromUser ("Set zoom in y direction [%]", 
                                 "", "", static_cast<int>(y_zoom * 100 + 0.5), 
                                 1, 10000000);
    if (r > 0)
        y_zoom = r / 100.;
    Refresh(false);
}

void DiffPlot::OnPopupYZoomFit (wxCommandEvent& WXUNUSED(event))
{
    fit_y_zoom();
}

void DiffPlot::fit_y_zoom()
{
    fp y = 0.;
    switch (kind) { // setting y_zoom
        case empty_ty:
        case peak_pos_ty:
            break; //y_zoom does not matter
        case diff_ty: 
            {
            y = get_max_abs_y (diff_of_data_for_draw_data);
            y_zoom = fabs (GetClientSize().GetHeight() / 
                                            (2 * y * common.plot_y_scale));
            fp order = pow (10, floor (log10(y_zoom)));
            y_zoom = floor(y_zoom / order) * order;
            }
            break;
        case diff_stddev_ty:
            y = get_max_abs_y (diff_stddev_of_data_for_draw_data);
            y_zoom_base = GetClientSize().GetHeight() / (2. * y);
            y_zoom = 0.9;
            break;
        case diff_y_proc_ty:
            y = get_max_abs_y (diff_y_proc_of_data_for_draw_data);
            y_zoom_base = GetClientSize().GetHeight() / (2. * y);
            y_zoom = 0.9;
            break;
        default:
            assert(0);
    }
    Refresh(false);
}

void DiffPlot::OnPopupYZoomAuto (wxCommandEvent& WXUNUSED(event))
{
    auto_zoom_y = !auto_zoom_y;
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

wxMouseEvent dummy_mouse_event;

