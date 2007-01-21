// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/fontdlg.h>
#include <wx/numdlg.h>
#include <wx/confbase.h>

#include "common.h"
#include "wx_plot.h"
#include "wx_gui.h"
#include "data.h"
#include "sum.h"
#include "logic.h" 

using namespace std;

//===============================================================
//                       BufferedPanel
//===============================================================

void BufferedPanel::refresh(bool now)
{
    clear_and_draw();
    Refresh(false);
    if (now) 
        Update();
}

bool BufferedPanel::resize_buffer(wxDC &dc)
{
    wxCoord w, h;
    dc.GetSize(&w, &h);
    if (!buffer.Ok()
            || w != buffer.GetWidth() || h != buffer.GetHeight()) {
        memory_dc.SelectObject(wxNullBitmap);
        buffer = wxBitmap(w, h);
        memory_dc.SelectObject(buffer);
        return true;
    }
    return false;
}

void BufferedPanel::clear_and_draw()
{
    if (!buffer.Ok())
        return;
    memory_dc.SetLogicalFunction(wxCOPY);
    memory_dc.SetBackground(wxBrush(backgroundCol));
    memory_dc.Clear();
    draw(memory_dc);
}

/// called from wxPaint event handler
void BufferedPanel::buffered_draw()
{
    wxPaintDC dc(this);
    if (resize_buffer(dc)) 
        clear_and_draw();
    dc.Blit(0, 0, buffer.GetWidth(), buffer.GetHeight(), &memory_dc, 0, 0);
}

//===============================================================
//                FPlot (plot with data and fitted curves) 
//===============================================================

void FPlot::draw_dashed_vert_line(int X, int style)
{
    if (X != INT_MIN) {
        wxClientDC dc(this);
        dc.SetLogicalFunction (wxINVERT);
        if (style == wxSHORT_DASH)
            dc.SetPen(*wxBLACK_DASHED_PEN);
        else {
            wxPen pen = *wxBLACK_DASHED_PEN;
            pen.SetStyle(style);
            dc.SetPen(pen);
        }
        int h = GetClientSize().GetHeight();
        dc.DrawLine (X, 0, X, h);
    }
}

void FPlot::draw_crosshair(int X, int Y)
{
    wxClientDC dc(this);
    dc.SetLogicalFunction (wxINVERT);
    dc.SetPen(*wxBLACK_DASHED_PEN);
    dc.CrossHair(X, Y);
}

bool FPlot::vert_line_following_cursor (MouseActEnum ma, int x, int x0)
{
    if (ma == mat_start) {
        draw_dashed_vert_line(x0);
        vlfc_prev_x0 = x0;
    }
    else {
        if (vlfc_prev_x == INT_MIN) 
            return false;
        draw_dashed_vert_line(vlfc_prev_x); //clear (or draw again) old line
    }
    if (ma == mat_move || ma == mat_start) {
        draw_dashed_vert_line(x);
        vlfc_prev_x = x;
    }
    else if (ma == mat_redraw) {
        draw_dashed_vert_line(vlfc_prev_x0);
    }
    else { // mat_stop
        draw_dashed_vert_line(vlfc_prev_x0); //clear
        vlfc_prev_x = vlfc_prev_x0 = INT_MIN;
    }
    return true;
}

void draw_line_with_style(wxDC& dc, int style, 
                          wxCoord X1, wxCoord Y1, wxCoord X2, wxCoord Y2)
{
    wxPen pen = dc.GetPen();
    int old_style = pen.GetStyle();
    pen.SetStyle(style);
    dc.SetPen(pen);
    dc.DrawLine (X1, Y1, X2, Y2);
    pen.SetStyle(old_style);
    dc.SetPen(pen);
}

/// draw x axis tics
void FPlot::draw_xtics (wxDC& dc, View const &v, bool set_pen)
{
    if (set_pen) {
        dc.SetPen(wxPen(xAxisCol));
        dc.SetTextForeground(xAxisCol);
    }
    dc.SetFont(ticsFont);

    vector<fp> minors;
    vector<fp> x_tics = scale_tics_step(v.left, v.right, x_max_tics, minors);
    int Y = y2Y(y_logarithm ? 1 : 0);
    for (vector<fp>::const_iterator i = x_tics.begin(); i != x_tics.end(); ++i){
        int X = x2X(*i);
        dc.DrawLine (X, Y, X, Y - x_tic_size);
        if (x_grid) 
            draw_line_with_style(dc, wxDOT, X,0, X,GetClientSize().GetHeight());
        wxString label = s2wx(S(*i));
        if (label == wxT("-0")) 
            label = wxT("0");
        wxCoord w, h;
        dc.GetTextExtent (label, &w, &h);
        dc.DrawText (label, X - w/2, Y + 1);
    }
    //draw minor tics
    if (xminor_tics_visible)
        for (vector<fp>::const_iterator i = minors.begin(); 
                                                    i != minors.end(); ++i) {
            int X = x2X(*i);
            dc.DrawLine (X, Y, X, Y - x_tic_size);
        }
}

/// draw y axis tics
void FPlot::draw_ytics (wxDC& dc, View const &v, bool set_pen)
{
    if (set_pen) {
        dc.SetPen(wxPen(xAxisCol));
        dc.SetTextForeground(xAxisCol);
    }
    dc.SetFont(ticsFont);


    //if y axis is visible, tics are drawed at the axis, 
    //otherwise tics are drawed at the left hand edge of the plot
    int X = 0;
    if (y_axis_visible && x2X(0) > 0 && x2X(0) < GetClientSize().GetWidth()-10)
        X = x2X(0);
    vector<fp> minors;
    vector<fp> y_tics = scale_tics_step(v.bottom, v.top, y_max_tics, 
                                        minors, y_logarithm);
    for (vector<fp>::const_iterator i = y_tics.begin(); i != y_tics.end(); ++i){
        int Y = y2Y(*i);
        dc.DrawLine (X, Y, X + y_tic_size, Y);
        if (y_grid) 
            draw_line_with_style(dc, wxDOT, 0,Y, GetClientSize().GetWidth(),Y);
        wxString label = s2wx(S(*i));
        if (label == wxT("-0"))
            label = wxT("0");
        if (x_axis_visible && label == wxT("0")) 
            continue;
        wxCoord w, h;
        dc.GetTextExtent (label, &w, &h);
        dc.DrawText (label, X + y_tic_size + 1, Y - h/2);
    }
    //draw minor tics
    if (yminor_tics_visible)
        for (vector<fp>::const_iterator i = minors.begin(); 
                                                    i != minors.end(); ++i) {
            int Y = y2Y(*i);
            dc.DrawLine (X, Y, X + y_tic_size, Y);
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
                       wxColour const& color, wxColour const& inactive_color, 
                       int Y_offset,
                       bool cumulative)
{
    Y_offset *= (GetClientSize().GetHeight() / 100);
    wxPen const activePen(color.Ok() ? color : activeDataCol);
    wxPen const inactivePen(inactive_color.Ok() ? inactive_color 
                                                : inactiveDataCol);
    wxBrush const activeBrush(activePen.GetColour(), wxSOLID);
    wxBrush const inactiveBrush(inactivePen.GetColour(), wxSOLID);
    if (data->is_empty()) 
        return;
    vector<Point>::const_iterator first = data->get_point_at(AL->view.left),
                                  last = data->get_point_at(AL->view.right);
    //if (last - first < 0) return;
    bool active = first->is_active;
    dc.SetPen (active ? activePen : inactivePen);
    dc.SetBrush (active ? activeBrush : inactiveBrush);
    int X_ = INT_MIN, Y_ = INT_MIN;
    // first line segment -- lines should be drawed towards points 
    //                                                 that are outside of plot 
    if (line_between_points && first > data->points().begin() && !cumulative) {
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
    Y_ -= Y_offset;
    fp y = 0;

    //drawing all points (and lines); main loop
    for (vector<Point>::const_iterator i = first; i < last; i++) {
        int X = x2X(i->x);
        if (cumulative)
            y += (*compute_y)(i, sum);
        else
            y = (*compute_y)(i, sum);
        int Y = y2Y(y) - Y_offset;
        if (X == X_ && Y == Y_) 
            continue;

        if (i->is_active != active) {
            //half of line between points should be active and half not.
            //draw first half here and change X_, Y_; the rest will be drawed
            //as usually.
            if (line_between_points) {
                int X_mid = (X_ + X) / 2, Y_mid = (Y_ + Y) / 2;
                dc.DrawLine (X_, Y_, X_mid, Y_mid);
                X_ = X_mid, Y_ = Y_mid;
            }
            active = i->is_active;
            if (active) {
                dc.SetPen (activePen);
                dc.SetBrush (activeBrush);
            }
            else {
                dc.SetPen (inactivePen);
                dc.SetBrush (inactiveBrush);
            }
        }

        if (point_radius > 1) 
            dc.DrawCircle (X, Y, point_radius - 1);
        if (line_between_points) {
            if (X_ != INT_MIN)
                dc.DrawLine (X_, Y_, X, Y);
            X_ = X, Y_ = Y;
        }
        else {//no line_between_points
            if (point_radius == 1)
                dc.DrawPoint (X, Y);
        }
    }

    //the last line segment, toward next point
    if (line_between_points && last < data->points().end() && !cumulative) {
        int X = x2X (AL->view.right);
        int Y_l = y2Y ((*compute_y)(last - 1, sum));
        int Y_r = y2Y ((*compute_y)(last, sum));
        int X_l = x2X ((last - 1)->x);
        int X_r = x2X (last->x);
        if (X_r != X_l) {
            int Y = Y_l + (Y_r - Y_l) * (X - X_l) / (X_r - X_l) - Y_offset;
            dc.DrawLine (X_, Y_, X, Y);
        }
    }
}

void FPlot::change_tics_font()
{
    wxFontData data;
    data.SetInitialFont(ticsFont);
    data.SetColour(xAxisCol);

    wxFontDialog dialog(frame, data);
    if (dialog.ShowModal() == wxID_OK)
    {
        wxFontData retData = dialog.GetFontData();
        ticsFont = retData.GetChosenFont();
        xAxisCol = retData.GetColour();
        refresh(false);
    }
}

void FPlot::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("Visible"));
    x_axis_visible = cfg_read_bool (cf, wxT("xAxis"), true);  
    y_axis_visible = cfg_read_bool (cf, wxT("yAxis"), false);  
    xtics_visible = cfg_read_bool (cf, wxT("xtics"), true);
    ytics_visible = cfg_read_bool (cf, wxT("ytics"), true);
    xminor_tics_visible = cfg_read_bool (cf, wxT("xMinorTics"), true);
    yminor_tics_visible = cfg_read_bool (cf, wxT("yMinorTics"), false);
    x_grid = cfg_read_bool (cf, wxT("xgrid"), false);
    y_grid = cfg_read_bool (cf, wxT("ygrid"), false);
    cf->SetPath(wxT("../Colors"));
    xAxisCol = cfg_read_color(cf, wxT("xAxis"), wxColour(wxT("WHITE")));
    cf->SetPath(wxT(".."));
    ticsFont = cfg_read_font(cf, wxT("ticsFont"), 
                             wxFont(8, wxFONTFAMILY_DEFAULT, 
                                    wxFONTSTYLE_NORMAL, 
                                    wxFONTWEIGHT_NORMAL));
}

void FPlot::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("Visible"));
    cf->Write (wxT("xAxis"), x_axis_visible);
    cf->Write (wxT("yAxis"), y_axis_visible);
    cf->Write (wxT("xtics"), xtics_visible);
    cf->Write (wxT("ytics"), ytics_visible);
    cf->Write (wxT("xMinorTics"), xminor_tics_visible);
    cf->Write (wxT("yMinorTics"), yminor_tics_visible);
    cf->Write (wxT("xgrid"), x_grid);
    cf->Write (wxT("ygrid"), y_grid);
    cf->SetPath(wxT("../Colors"));
    cfg_write_color (cf, wxT("xAxis"), xAxisCol);
    cf->SetPath(wxT(".."));
    cfg_write_font (cf, wxT("ticsFont"), ticsFont);
}


BEGIN_EVENT_TABLE(FPlot, wxPanel)
END_EVENT_TABLE()


//===============================================================
//                             utilities
//===============================================================

/// returns major and minor tics positions
vector<fp> scale_tics_step (fp beg, fp end, int max_tics, 
                            vector<fp> &minors, bool log)
{
    vector<fp> result;
    minors.clear();
    if (beg >= end || max_tics <= 0)
        return result;
    if (log) {
        if (beg <= 0)
            beg = 1e-1;
        if (end <= beg)
            end = 2*beg;
        fp min_logstep = (log10(end/beg)) / max_tics;
        bool with_2_5 = (min_logstep < log10(2.));
        fp logstep = ceil(min_logstep);
        fp step0 = pow(10, logstep * ceil(log10(beg) / logstep));
        for (int i = 2; i <= 9; ++i) {
            fp v = step0/10. * i;
            if (v > beg && v < end) {
                if (with_2_5 && (i == 2 || i == 5))
                    result.push_back(v);
                else
                    minors.push_back(v);
            }
        }
        for (fp t = step0; t < end; t *= pow(10,logstep)) {
            result.push_back(t);
            for (int i = 2; i <= 9; ++i) {
                fp v = t * i;
                if (v > beg && v < end)
                    if (with_2_5 && (i == 2 || i == 5))
                        result.push_back(v);
                    else
                        minors.push_back(v);
            }
        }
    }
    else {
        fp min_step = (end - beg) / max_tics;
        fp s = pow(10, floor (log10 (min_step)));
        int minor_div = 5; //ratio of major-step to minor-step
        // now s <= min_step
        if (s >= min_step)
            ;
        else if (s * 2 >= min_step) {
            s *= 2;
            minor_div = 2;
        }
        else if (s * 2.5 >= min_step) 
            s *= 2.5;
        else if (s * 5 >=  min_step) 
            s *= 5;
        else
            s *= 10;
        fp t = s * ceil(beg / s);
        for (int i = 1; i < minor_div; ++i) {
            fp v = t - s * i / minor_div;
            if (v > beg && v < end)
                minors.push_back(v);
        }
        for (; t < end; t += s) {
            result.push_back(t);
            for (int i = 1; i < minor_div; ++i) {
                fp v = t + s * i / minor_div;
                if (v < end)
                    minors.push_back(v);
            }
        }
    }
    return result;
}

