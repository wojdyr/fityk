// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

// wxwindows headers, see wxwindows samples for description
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/fontdlg.h>

#include "common.h"
#include "wx_plot.h"
#include "wx_gui.h"
#include "data.h"
#include "sum.h"
#include "logic.h" 

using namespace std;

enum {
    ID_aux_popup_plot_0            = 25310,
    ID_aux_popup_c_background      = 25340,
    ID_aux_popup_c_active_data            ,
    ID_aux_popup_c_inactive_data          ,
    ID_aux_popup_c_axis                   ,
    ID_aux_popup_color                    ,
    ID_aux_popup_m_tfont                  ,
    ID_aux_popup_yz_fit                   ,
    ID_aux_popup_yz_change                ,
    ID_aux_popup_yz_auto                  
};

//===============================================================
//                FPlot (plot with data and fitted curves) 
//===============================================================

fp scale_tics_step (fp beg, fp end, int max_tics);

void FPlot::draw_dashed_vert_lines (int x1)
{
    if (x1 != INVALID) {
        wxClientDC dc(this);
        dc.SetLogicalFunction (wxINVERT);
        dc.SetPen(*wxBLACK_DASHED_PEN);
        int h = GetClientSize().GetHeight();
        dc.DrawLine (x1, 0, x1, h);
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
    if (ma == mat_start) {
        draw_dashed_vert_lines(x0);
        vlfc_prev_x0 = x0;
    }
    else {
        if (vlfc_prev_x == INVALID) 
            return false;
        draw_dashed_vert_lines(vlfc_prev_x); //clear (or draw again) old line
    }
    if (ma == mat_move || ma == mat_start) {
        draw_dashed_vert_lines(x);
        vlfc_prev_x = x;
    }
    else if (ma == mat_redraw) {
        draw_dashed_vert_lines(vlfc_prev_x0);
    }
    else { // mat_stop, mat_cancel:
        draw_dashed_vert_lines(vlfc_prev_x0); //clear
        vlfc_prev_x = vlfc_prev_x0 = INVALID;
    }
    return true;
}

void FPlot::draw_tics (wxDC& dc, View const &v, 
                       int const x_max_tics, int const y_max_tics, 
                       int const x_tic_size, int const y_tic_size)
{
    dc.SetPen (xAxisPen);
    dc.SetFont(ticsFont);
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

fp FPlot::get_max_abs_y (fp (*compute_y)(vector<Point>::const_iterator, 
                                         Sum const*),
                         vector<Point>::const_iterator first,
                         vector<Point>::const_iterator last,
                         Sum const* sum)
{
    fp max_abs_y = 0;
    for (vector<Point>::const_iterator i = first; i < last; i++) {
        if (i->is_active) {
            fp y = fabs(((*compute_y)(i, sum)));
            if (y > max_abs_y) max_abs_y = y;
        }
    }
    return max_abs_y;
}

void FPlot::draw_data (wxDC& dc, 
                       fp (*compute_y)(vector<Point>::const_iterator, 
                                       Sum const*),
                       Data const* data, 
                       Sum const* sum,
                       wxColour const& color,
                       int Y_offset)
{
    Y_offset *= (GetClientSize().GetHeight() / 100);
    if (color.Ok())
        activeDataPen.SetColour(color);
    wxPen const& activePen = activeDataPen;
    wxPen const& inactivePen = inactiveDataPen;
    wxBrush const activeBrush(activePen.GetColour(), wxSOLID);
    wxBrush const inactiveBrush(inactivePen.GetColour(), wxSOLID);
    if (data->is_empty()) return;
    vector<Point>::const_iterator first = data->get_point_at(AL->view.left),
                                  last = data->get_point_at(AL->view.right);
    //if (last - first < 0) return;
    bool active = !first->is_active;//causes pens to be initialized in main loop
    int X_ = 0, Y_ = 0;
    // first line segment -- lines should be drawed towards points 
    //                                                 that are outside of plot 
    if (line_between_points) {
        dc.SetPen(first->is_active ? activePen : inactivePen);
        if (first > data->points().begin()) {
            X_ = x2X (AL->view.left);
            int Y_l = y2Y ((*compute_y)(first - 1, sum));
            int Y_r = y2Y ((*compute_y)(first, sum));
            int X_l = x2X ((first - 1)->x);
            int X_r = x2X (first->x);
            if (X_r == X_l)
                Y_ = Y_r;
            else
                Y_ = Y_l + (Y_r - Y_l) * (X_ - X_l) / (X_r - X_l);
        }
        else {
            X_ = x2X(first->x);
            Y_ = y2Y ((*compute_y)(first, sum));
        }
    }
    Y_ -= Y_offset;

    //drawing all points (and lines); main loop
    for (vector<Point>::const_iterator i = first; i < last; i++) {
        int X = x2X(i->x);
        int Y = y2Y ((*compute_y)(i, sum)) - Y_offset;
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
        int X = x2X (AL->view.right);
        int Y_l = y2Y ((*compute_y)(last - 1, sum));
        int Y_r = y2Y ((*compute_y)(last, sum));
        int X_l = x2X ((last - 1)->x);
        int X_r = x2X (last->x);
        if (X_r != X_l) {
            int Y = Y_l + (Y_r - Y_l) * (X_ - X_l) / (X_r - X_l) - Y_offset;
            dc.DrawLine (X_, Y_, X, Y);
        }
    }
}

void FPlot::change_tics_font()
{
    wxFontData data;
    data.SetInitialFont(ticsFont);
    data.SetColour(xAxisPen.GetColour());

    wxFontDialog dialog(frame, &data);
    if (dialog.ShowModal() == wxID_OK)
    {
        wxFontData retData = dialog.GetFontData();
        ticsFont = retData.GetChosenFont();
        xAxisPen.SetColour(retData.GetColour());
        Refresh(false);
    }
}

void FPlot::read_settings(wxConfigBase *cf)
{
    cf->SetPath("Visible");
    x_axis_visible = read_bool_from_config (cf, "xAxis", true);  
    tics_visible = read_bool_from_config (cf, "tics", true);
    cf->SetPath("../Colors");
    xAxisPen.SetColour (read_color_from_config(cf, "xAxis", wxColour("WHITE")));
    cf->SetPath("..");
    ticsFont = read_font_from_config(cf, "ticsFont", *wxSMALL_FONT);
}

void FPlot::save_settings(wxConfigBase *cf) const
{
    cf->SetPath("Visible");
    cf->Write ("xAxis", x_axis_visible);
    cf->Write ("tics", tics_visible);
    cf->SetPath("../Colors");
    write_color_to_config (cf, "xAxis", xAxisPen.GetColour());
    cf->SetPath("..");
    write_font_to_config (cf, "ticsFont", ticsFont);
}


BEGIN_EVENT_TABLE(FPlot, wxPanel)
END_EVENT_TABLE()



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
    EVT_MENU (ID_aux_popup_m_tfont, AuxPlot::OnTicsFont)
    EVT_MENU (ID_aux_popup_yz_change, AuxPlot::OnPopupYZoom)
    EVT_MENU (ID_aux_popup_yz_fit, AuxPlot::OnPopupYZoomFit)
    EVT_MENU (ID_aux_popup_yz_auto, AuxPlot::OnPopupYZoomAuto)
END_EVENT_TABLE()

void AuxPlot::OnPaint(wxPaintEvent &WXUNUSED(event))
{
    frame->draw_crosshair(-1, -1); 
    wxPaintDC dc(this);
    dc.SetLogicalFunction (wxCOPY);
    if (backgroundBrush.Ok())
        dc.SetBackground (backgroundBrush);
    dc.Clear();
    Draw(dc);
    vert_line_following_cursor(mat_redraw);//draw, if necessary, vertical lines
}

inline fp sum_value(vector<Point>::const_iterator pt, Sum const* sum)
{
    return sum->value(pt->x);
}

fp diff_of_data_for_draw_data (vector<Point>::const_iterator i, Sum const* sum)
{
    return i->y - sum_value(i, sum);
}

fp diff_stddev_of_data_for_draw_data (vector<Point>::const_iterator i, 
                                      Sum const* sum)
{
    return (i->y - sum_value(i, sum)) / i->sigma;
}

fp diff_y_proc_of_data_for_draw_data (vector<Point>::const_iterator i, 
                                      Sum const* sum)
{
    return i->y ? (i->y - sum_value(i, sum)) / i->y * 100 : 0;
}

void AuxPlot::Draw(wxDC &dc)
{
    int pos = AL->get_active_ds_position();
    Data const* data = AL->get_data(pos);
    Sum const* sum = AL->get_sum(pos);
    if (auto_zoom_y || fit_y_once) {
        fit_y_zoom(data, sum);
        fit_y_once = false;
    }
    set_scale();
    if (kind == apk_empty || data->is_empty()) 
        return;
    dc.SetPen (xAxisPen);

    if (kind == apk_peak_pos) {
        int ymax = GetClientSize().GetHeight();
        for (vector<wxPoint>::const_iterator i = shared.peaktops.begin(); 
             i != shared.peaktops.end(); i++) 
            dc.DrawLine(i->x, 0, i->x, ymax);
        return;
    }

    //don't draw y axis and y tics
    if (x_axis_visible) {
        dc.DrawLine (0, y2Y(0), GetClientSize().GetWidth(), y2Y(0));
        if (kind == apk_diff) draw_zoom_text(dc);
    }
    if (tics_visible) {
        View v(0, 0, Y2y(GetClientSize().GetHeight()), Y2y(0));
        draw_tics(dc, v, 0, 5, 0, 4);
    }

    if (kind == apk_diff)
        draw_data (dc, diff_of_data_for_draw_data, data, sum);
    else if (kind == apk_diff_stddev)
        draw_data (dc, diff_stddev_of_data_for_draw_data, data, sum);
    else if (kind == apk_diff_y_proc)
        draw_data (dc, diff_y_proc_of_data_for_draw_data, data, sum);
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
    vert_line_following_cursor(mat_move, X);
    wxString str;
    str.Printf ("%.3f  [%d]", x, static_cast<int>(y + 0.5));
    frame->set_status_text(str, sbf_coord);
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
    frame->set_status_text("", sbf_coord);
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
    cf->SetPath("Visible");
    // nothing here now
    cf->SetPath("../Colors");
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
    cf->SetPath("..");
    FPlot::read_settings(cf);
    Refresh();
}

void AuxPlot::save_settings(wxConfigBase *cf) const
{
    cf->SetPath("/AuxPlot_" + name);
    cf->Write ("kind", kind); 
    cf->Write ("line_between_points", line_between_points);
    cf->Write ("point_radius", point_radius);

    cf->SetPath("Visible");
    // nothing here now

    cf->SetPath("../Colors");
    write_color_to_config (cf, "bg", backgroundBrush.GetColour()); 
    write_color_to_config (cf, "active_data", activeDataPen.GetColour());
    write_color_to_config (cf, "inactive_data", inactiveDataPen.GetColour());
    cf->SetPath("..");
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
        SetCursor(wxCURSOR_SIZEWE);  
        frame->set_status_text("Select x range and release button to zoom..."); 
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
        frame->set_status_text(""); 
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
    wxMenu *misc_menu = new wxMenu;
    misc_menu->Append (ID_aux_popup_m_tfont, "&Tics font");
    popup_menu.Append (wxNewId(), "&Miscellaneous", misc_menu);
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
        frame->focus_input(event.GetKeyCode());
    }
    else
        event.Skip();
}

void AuxPlot::OnPopupPlot (wxCommandEvent& event)
{
    kind = static_cast<Aux_plot_kind_enum>(event.GetId()-ID_aux_popup_plot_0);
    //fit_y_zoom();
    fit_y_once = true;
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
    wxColour col = brush ? brush->GetColour() : pen->GetColour();
    if (change_color_dlg(col)) {
        if (brush) 
            brush->SetColour(col);
        if (pen) 
            pen->SetColour(col);
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
    //fit_y_zoom();
    fit_y_once = true;
    Refresh(false);
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
            y_zoom = fabs (GetClientSize().GetHeight() / 
                                            (2 * y * shared.plot_y_scale));
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
        case apk_diff_y_proc:
            y = get_max_abs_y(diff_y_proc_of_data_for_draw_data, 
                              first, last, sum);
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
        //fit_y_zoom(); -- it is called from Draw
        Refresh(false);
    }
}

//===============================================================
//                             utilities
//===============================================================

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

