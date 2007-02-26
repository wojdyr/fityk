// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

/// In this file:
///  Auxiliary Plot, for displaying residuals, peak positions, etc. (AuxPlot)

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/numdlg.h>

#include "aplot.h"
#include "gui.h"
#include "../sum.h"
#include "../data.h"
#include "../logic.h"

using namespace std;

enum {
    ID_aux_plot0            = 25310,
    ID_aux_plot_ctr         = 25340,
    ID_aux_revd                    ,
    ID_aux_c_background            ,
    ID_aux_c_active_data           ,
    ID_aux_c_inactive_data         ,
    ID_aux_c_axis                  ,
    ID_aux_color                   ,
    ID_aux_m_tfont                 ,
    ID_aux_yz_fit                  ,
    ID_aux_yz_change               ,
    ID_aux_yz_auto                  
};


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
    EVT_MENU_RANGE (ID_aux_plot0, ID_aux_plot0+10, AuxPlot::OnPopupPlot)
    EVT_MENU (ID_aux_plot_ctr, AuxPlot::OnPopupPlotCtr)
    EVT_MENU (ID_aux_revd, AuxPlot::OnPopupReversedDiff)
    EVT_MENU_RANGE (ID_aux_c_background, ID_aux_color-1, AuxPlot::OnPopupColor)
    EVT_MENU (ID_aux_m_tfont, AuxPlot::OnTicsFont)
    EVT_MENU (ID_aux_yz_change, AuxPlot::OnPopupYZoom)
    EVT_MENU (ID_aux_yz_fit, AuxPlot::OnPopupYZoomFit)
    EVT_MENU (ID_aux_yz_auto, AuxPlot::OnPopupYZoomAuto)
END_EVENT_TABLE()

void AuxPlot::OnPaint(wxPaintEvent &WXUNUSED(event))
{
    frame->draw_crosshair(-1, -1); 
    //wxPaintDC dc(this);
    //dc.SetLogicalFunction (wxCOPY);
    //dc.SetBackground(wxBrush(backgroundCol));
    //dc.Clear();
    //draw(dc);
    buffered_draw();
    vert_line_following_cursor(mat_redraw);//draw, if necessary, vertical lines
}

inline double sum_value(vector<Point>::const_iterator pt, Sum const* sum)
{
    return sum->value(pt->x);
}

double diff_of_data_for_draw_data (vector<Point>::const_iterator i, 
                                   Sum const* sum)
{
    return i->y - sum_value(i, sum);
}

double rdiff_of_data_for_draw_data (vector<Point>::const_iterator i, 
                                    Sum const* sum)
{
    return sum_value(i, sum) - i->y;
}

double diff_stddev_of_data_for_draw_data (vector<Point>::const_iterator i, 
                                          Sum const* sum)
{
    return (i->y - sum_value(i, sum)) / i->sigma;
}

double rdiff_stddev_of_data_for_draw_data (vector<Point>::const_iterator i, 
                                           Sum const* sum)
{
    return (sum_value(i, sum) - i->y) / i->sigma;
}

double diff_chi2_of_data_for_draw_data (vector<Point>::const_iterator i, 
                                        Sum const* sum)
{
    double t = (i->y - sum_value(i, sum)) / i->sigma;
    return t*t;
}

double diff_y_perc_of_data_for_draw_data (vector<Point>::const_iterator i, 
                                          Sum const* sum)
{
    return i->y ? (i->y - sum_value(i, sum)) / i->y * 100 : 0;
}

double rdiff_y_perc_of_data_for_draw_data (vector<Point>::const_iterator i, 
                                           Sum const* sum)
{
    return i->y ? (sum_value(i, sum) - i->y) / i->y * 100 : 0;
}

void AuxPlot::draw(wxDC &dc, bool monochrome)
{
    int pos = AL->get_active_ds_position();
    Data const* data = AL->get_data(pos);
    Sum const* sum = AL->get_sum(pos);
    if (auto_zoom_y || fit_y_once) {
        fit_y_zoom(data, sum);
        fit_y_once = false;
    }
    set_scale();
    if (monochrome) {
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxBLACK_BRUSH);
    }
    else
        dc.SetPen(wxPen(xAxisCol));

    if (mark_peak_ctrs) {
        int ymax = GetClientSize().GetHeight();
        for (vector<wxPoint>::const_iterator i = shared.peaktops.begin(); 
             i != shared.peaktops.end(); i++) 
            dc.DrawLine(i->x, 0, i->x, ymax);
    }

    if (kind == apk_empty || data->is_empty()) 
        return;

    if (x_axis_visible) {
        dc.DrawLine (0, y2Y(0), GetClientSize().GetWidth(), y2Y(0));
        if (kind == apk_diff) 
            draw_zoom_text(dc, !monochrome);
    }
    if (y_axis_visible) {
        dc.DrawLine (x2X(0), 0, x2X(0), GetClientSize().GetHeight());
    }
    if (ytics_visible) {
        View v(0, 0, Y2y(GetClientSize().GetHeight()), Y2y(0));
        draw_ytics(dc, v, !monochrome);
    }

    fp (*f)(vector<Point>::const_iterator, Sum const*) = 0;
    bool cummulative = false;
    if (kind == apk_diff) 
        f = reversed_diff ? rdiff_of_data_for_draw_data 
                          : diff_of_data_for_draw_data;
    else if (kind == apk_diff_stddev)
        f = reversed_diff ? rdiff_stddev_of_data_for_draw_data 
                          : diff_stddev_of_data_for_draw_data;
    else if (kind == apk_diff_y_perc)
        f = reversed_diff ? rdiff_y_perc_of_data_for_draw_data
                          : diff_y_perc_of_data_for_draw_data;
    else if (kind == apk_cum_chi2) {
        f = diff_chi2_of_data_for_draw_data;
        cummulative = true;
    }
    wxColour col = monochrome ? dc.GetPen().GetColour() : wxNullColour;
    draw_data (dc, f, data, sum, col, col, 0, cummulative);
}

void AuxPlot::draw_zoom_text(wxDC& dc, bool set_pen)
{
    if (set_pen)
        dc.SetTextForeground(xAxisCol);
    if (shared.plot_y_scale) {
        dc.SetFont(*wxNORMAL_FONT);  
        string s = "x" + S(y_zoom);  
        wxCoord w, h;
        dc.GetTextExtent (s2wx(s), &w, &h); 
        dc.DrawText (s2wx(s), GetClientSize().GetWidth() - w - 2, 2);
    }
}

void AuxPlot::OnMouseMove(wxMouseEvent &event)
{
    int X = event.GetX();
    vert_line_following_cursor(mat_move, X);
    frame->set_status_coord_info(X2x(X), Y2y(event.GetY()), true);
    int new_cursor;
    if (X < move_plot_margin_width)
        new_cursor = wxCURSOR_POINT_LEFT;
    else if (X > GetClientSize().GetWidth() - move_plot_margin_width)
        new_cursor = wxCURSOR_POINT_RIGHT;
    else {
        frame->draw_crosshair(X, -1);
        new_cursor = wxCURSOR_CROSS;
    }
    if (new_cursor != cursor_id) {
        cursor_id = new_cursor;
        SetCursor(wxCursor(new_cursor));
    }
}

void AuxPlot::OnLeaveWindow (wxMouseEvent& WXUNUSED(event))
{
    frame->set_status_text("", sbf_coord);
    frame->draw_crosshair(-1, -1);
}

bool AuxPlot::is_zoomable()
{
    return kind == apk_diff || kind == apk_diff_stddev 
           || kind == apk_diff_y_perc || kind == apk_cum_chi2;
}

void AuxPlot::set_scale()
{
    frame->set_shared_scale();
    int h = GetClientSize().GetHeight();
    if (kind == apk_cum_chi2) {
        yUserScale = -1. * y_zoom_base * y_zoom;
        yLogicalOrigin = - h / yUserScale;
        return;
    }
    switch (kind) {
        case apk_empty:
            yUserScale = 1.; //y scale doesn't matter
            break; 
        case apk_diff: 
            if (shared.plot_y_scale)
                yUserScale = shared.plot_y_scale * y_zoom;
            else 
                yUserScale = y_zoom;
            break;
        case apk_diff_stddev:
        case apk_diff_y_perc:
            yUserScale = -1. * y_zoom_base * y_zoom;
            break;
        default:
            assert(0);
    }
    yLogicalOrigin = - h / 2. / yUserScale;
}
 
void AuxPlot::read_settings(wxConfigBase *cf)
{
    wxString path = wxT("/AuxPlot_") + name;
    cf->SetPath(path);
    kind = static_cast <Aux_plot_kind_enum> (cf->Read (wxT("kind"), apk_diff));
    mark_peak_ctrs = cfg_read_bool (cf, wxT("markCtr"), false);  
    reversed_diff = cfg_read_bool (cf, wxT("reversedDiff"), false);  
    auto_zoom_y = false;
    line_between_points = cfg_read_bool(cf, wxT("line_between_points"), true);
    point_radius = cf->Read (wxT("point_radius"), 1);
    y_max_tics = cf->Read(wxT("yMaxTics"), 5);
    y_tic_size = cf->Read(wxT("yTicSize"), 4);
    cf->SetPath(wxT("Visible"));
    // nothing here now
    cf->SetPath(wxT("../Colors"));
    backgroundCol = cfg_read_color (cf, wxT("bg"), wxColour(50, 50, 50));
    activeDataCol = cfg_read_color (cf, wxT("active_data"),
                                                       wxColour (wxT("GREEN")));
    inactiveDataCol = cfg_read_color(cf,wxT("inactive_data"),
                                                      wxColour (128, 128, 128));
    cf->SetPath(wxT(".."));
    FPlot::read_settings(cf);
    refresh();
}

void AuxPlot::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/AuxPlot_") + name);
    cf->Write (wxT("kind"), kind); 
    cf->Write (wxT("markCtr"), mark_peak_ctrs);
    cf->Write (wxT("reversedDiff"), reversed_diff);
    cf->Write (wxT("line_between_points"), line_between_points);
    cf->Write (wxT("point_radius"), point_radius);
    cf->Write(wxT("yMaxTics"), y_max_tics);
    cf->Write(wxT("yTicSize"), y_tic_size);

    cf->SetPath(wxT("Visible"));
    // nothing here now

    cf->SetPath(wxT("../Colors"));
    cfg_write_color(cf, wxT("bg"), backgroundCol); 
    cfg_write_color(cf, wxT("active_data"), activeDataCol);
    cfg_write_color(cf, wxT("inactive_data"),inactiveDataCol);
    cf->SetPath(wxT(".."));
    FPlot::save_settings(cf);
}

void AuxPlot::OnLeftDown (wxMouseEvent &event)
{
    cancel_mouse_left_press();
    if (event.ShiftDown()) { // the same as OnMiddleDown()
        frame->GViewAll();
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
        SetCursor(wxCursor(wxCURSOR_SIZEWE));  
        frame->set_status_text("Select x range and release button to zoom..."); 
        CaptureMouse();
    }
}

bool AuxPlot::cancel_mouse_left_press()
{
    if (mouse_press_X != INT_MIN) {
        vert_line_following_cursor(mat_stop);
        ReleaseMouse();
        mouse_press_X = INT_MIN;
        cursor_id = wxCURSOR_CROSS;
        SetCursor(wxCursor(wxCURSOR_CROSS));  
        frame->set_status_text(""); 
        return true;
    }
    else
        return false;
}

void AuxPlot::OnLeftUp (wxMouseEvent &event)
{
    if (mouse_press_X == INT_MIN)
        return;
    if (abs(event.GetX() - mouse_press_X) < 5) { //cancel
        cancel_mouse_left_press();
        return;
    }
    fp x1 = X2x(event.GetX());
    fp x2 = X2x(mouse_press_X);
    cancel_mouse_left_press();
    frame->change_zoom("[" + S(min(x1,x2)) + " : " + S(max(x1,x2)) + "]");
}

//popup-menu
void AuxPlot::OnRightDown (wxMouseEvent &event)
{
    if (cancel_mouse_left_press())
        return;

    wxMenu popup_menu (wxT("aux. plot menu"));
    //wxMenu *kind_menu = new wxMenu;
    popup_menu.AppendRadioItem(ID_aux_plot0+0, wxT("&empty"), wxT("nothing"));
    popup_menu.AppendRadioItem(ID_aux_plot0+1, wxT("&diff"), wxT("y_d - y_s"));
    popup_menu.AppendRadioItem(ID_aux_plot0+2, wxT("&weighted diff"), 
                               wxT("(y_d - y_s) / sigma"));
    popup_menu.AppendRadioItem(ID_aux_plot0+3, wxT("&proc diff"), 
                               wxT("(y_d - y_s) / y_d [%]"));
    popup_menu.AppendRadioItem(ID_aux_plot0+4, wxT("cumul. &chi2"), 
                               wxT("cumulative chi square"));
    popup_menu.Check(ID_aux_plot0+kind, true);
    popup_menu.AppendSeparator();
    popup_menu.AppendCheckItem(ID_aux_revd, wxT("reversed diff"), 
                               wxT(""));
    popup_menu.Check(ID_aux_revd, reversed_diff);
    popup_menu.AppendSeparator();
    popup_menu.AppendCheckItem(ID_aux_plot_ctr, wxT("show peak po&sitions"), 
                               wxT("mark centers of peaks"));
    popup_menu.Check(ID_aux_plot_ctr, mark_peak_ctrs);
    popup_menu.AppendSeparator();
    popup_menu.Append (ID_aux_yz_fit, wxT("&Fit to window"));
    popup_menu.Enable(ID_aux_yz_fit, is_zoomable());
    popup_menu.Append (ID_aux_yz_change, wxT("Change &y scale"));
    popup_menu.Enable(ID_aux_yz_change, is_zoomable());
    popup_menu.AppendCheckItem (ID_aux_yz_auto, wxT("&Auto-fit"));
    popup_menu.Check (ID_aux_yz_auto, auto_zoom_y);
    popup_menu.Enable(ID_aux_yz_auto, is_zoomable());
    popup_menu.AppendSeparator();
    wxMenu *color_menu = new wxMenu;
    color_menu->Append (ID_aux_c_background, wxT("&Background"));
    color_menu->Append (ID_aux_c_active_data, wxT("&Active Data"));
    color_menu->Append (ID_aux_c_inactive_data, wxT("&Inactive Data"));
    color_menu->Append (ID_aux_c_axis, wxT("&X Axis"));
    popup_menu.Append (ID_aux_color, wxT("&Color"), color_menu);
    wxMenu *misc_menu = new wxMenu;
    misc_menu->Append (ID_aux_m_tfont, wxT("&Tics font"));
    popup_menu.Append (wxNewId(), wxT("&Miscellaneous"), misc_menu);
    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void AuxPlot::OnMiddleDown (wxMouseEvent& WXUNUSED(event))
{
    if (cancel_mouse_left_press())
        return;
    frame->GViewAll();
}

void AuxPlot::OnKeyDown (wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_ESCAPE) {
        cancel_mouse_left_press();
    }
    else if (should_focus_input(event.GetKeyCode())) {
        cancel_mouse_left_press();
        frame->focus_input(event);
    }
    else
        event.Skip();
}

void AuxPlot::OnPopupPlot (wxCommandEvent& event)
{
    kind = static_cast<Aux_plot_kind_enum>(event.GetId()-ID_aux_plot0);
    //fit_y_zoom();
    fit_y_once = true;
    refresh(false);
}

void AuxPlot::OnPopupPlotCtr (wxCommandEvent& event)
{
    mark_peak_ctrs = event.IsChecked();
    refresh(false);
}

void AuxPlot::OnPopupReversedDiff (wxCommandEvent& event)
{
    reversed_diff = event.IsChecked();
    refresh(false);
}

void AuxPlot::OnPopupColor (wxCommandEvent& event)
{
    wxColour *color = 0;
    int n = event.GetId();
    if (n == ID_aux_c_background)
        color = &backgroundCol;
    else if (n == ID_aux_c_active_data) {
        color = &activeDataCol;
    }
    else if (n == ID_aux_c_inactive_data) {
        color = &inactiveDataCol;
    }
    else if (n == ID_aux_c_axis)
        color = &xAxisCol;
    else 
        return;
    if (change_color_dlg(*color)) {
        refresh();
    }
}

void AuxPlot::OnPopupYZoom (wxCommandEvent& WXUNUSED(event))
{
    int r = wxGetNumberFromUser(wxT("Set zoom in y direction [%]"), 
                                wxT(""), wxT(""), 
                                static_cast<int>(y_zoom * 100 + 0.5), 
                                1, 10000000);
    if (r > 0)
        y_zoom = r / 100.;
    refresh(false);
}

void AuxPlot::OnPopupYZoomFit (wxCommandEvent& WXUNUSED(event))
{
    //fit_y_zoom();
    fit_y_once = true;
    refresh(false);
}

void AuxPlot::fit_y_zoom(Data const* data, Sum const* sum)
{
    if (!is_zoomable())
        return;
    fp y = 0.;
    vector<Point>::const_iterator first = data->get_point_at(AL->view.left),
                                  last = data->get_point_at(AL->view.right);
    if (data->is_empty() || last==first)
        return;
    switch (kind) { // setting y_zoom
        case apk_diff: 
            {
            y = get_max_abs_y(diff_of_data_for_draw_data, first, last, sum);
            y_zoom = fabs (GetClientSize().GetHeight() / (2 * y 
                           * (shared.plot_y_scale ? shared.plot_y_scale : 1.)));
            fp order = pow (10, floor (log10(y_zoom)));
            y_zoom = floor(y_zoom / order) * order;
            }
            break;
        case apk_diff_stddev:
            y = get_max_abs_y(diff_stddev_of_data_for_draw_data, 
                              first, last, sum);
            y_zoom_base = GetClientSize().GetHeight() / (2. * y);
            y_zoom = 0.9;
            break;
        case apk_diff_y_perc:
            y = get_max_abs_y(diff_y_perc_of_data_for_draw_data, 
                              first, last, sum);
            y_zoom_base = GetClientSize().GetHeight() / (2. * y);
            y_zoom = 0.9;
            break;
        case apk_cum_chi2:
            y = 0.;
            for (vector<Point>::const_iterator i = first; i < last; i++) 
                y += diff_chi2_of_data_for_draw_data(i, sum);
            y_zoom_base = GetClientSize().GetHeight() / y;
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
        //fit_y_zoom() is called from draw
        refresh(false);
    }
}

