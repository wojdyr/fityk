// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
///  FPlot, the base class for MainPlot and AuxPlot

#include <wx/wx.h>
#include <wx/dcgraph.h>
#include <wx/confbase.h>

#include "plot.h"
#include "cmn.h"
#include "frame.h" //ftk
#include "../data.h"
#include "../logic.h"

using namespace std;

void Scale::set(double m, double M, int pixels)
{
    double h = 0;
    if (logarithm) {
        M = log(max(M, epsilon));
        m = log(max(m, epsilon));
    }
    h = M - m;
    origin = reversed ? M : m;
    if (h == 0)
        h = 0.1;
    scale = pixels / (reversed ? -h : h);
    // small (subpixel) shift that aligns axis with pixel grid
    if (!logarithm)
        origin = iround(origin * scale) / scale;
}

double Scale::valr(int px) const
{
    if (logarithm) {
        double val = exp(px / scale + origin);
        double delta = fabs(val - exp((px-1) / scale + origin));
        double t = pow(10, floor(log10(delta)));
        return floor(val / t + 0.5) * t;
    }
    else {
        double val = px / scale + origin;
        double delta = fabs(0.5 / scale);
        double t = pow(10, floor(log10(delta)));
        return floor(val / t + 0.5) * t;
    }
}

void Overlay::bg_color_updated(const wxColor& bg)
{
    if (0.299*bg.Red() + 0.587*bg.Green() + 0.114*bg.Blue() < 128)
        color_.Set(192, 192, 192);
    else
        color_.Set(64, 64, 64);
}

void Overlay::draw_overlay()
{
    if (mode_ == kFunction)
        // function is drawn by calling draw_lines()
        return;

    wxClientDC dc(panel_) ;
    panel_->blit(dc);

    dc.SetPen(wxPen(color_, 1, wxPENSTYLE_SHORT_DASH));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    switch (mode_) {
        case kRect:
            if (x1_ != x2_ || y1_ != y2_) {
                int width = abs(x1_ - x2_);
                int height = abs(y1_ - y2_);
                dc.DrawRectangle(min(x1_, x2_), min(y1_, y2_), width, height);
            }
            break;
        case kPeakDraft: {
            int ctr = x1_;
            int hwhm = abs(x1_ - x2_);
            int ymid = (y2_ + y1_) / 2;
            dc.DrawLine(ctr, y1_, ctr, y2_); // vertical line
            dc.DrawLine(ctr - hwhm, ymid, ctr + hwhm, ymid); // horizontal
            dc.DrawLine(ctr, y2_, ctr - 2 * hwhm, y1_); // left slope
            dc.DrawLine(ctr, y2_, ctr + 2 * hwhm, y1_); // right slope
            break;
        }
        case kLinearDraft: {
            int width = dc.GetSize().GetWidth();
            int height = dc.GetSize().GetHeight();
            int dy = y2_ - y1_;
            int dx = x2_ - x1_;
            if (abs(dy) < abs(dx)) {
                double m = (double) dy / dx;
                dc.DrawLine(0, y1_ - m * x1_,
                            width, y1_ + m * (width - x1_));
            }
            else {
                double im = (double) dx / dy;
                dc.DrawLine(x1_ - y1_ * im, 0,
                            x1_ + (height - y1_) * im, height);
            }
            break;
        }
        case kCrossHair:
            dc.CrossHair(x2_, y2_);
            break;
        case kVLine:
            dc.DrawLine(x2_, 0, x2_, dc.GetSize().GetHeight());
            break;
        case kHLine:
            dc.DrawLine(0, y2_, dc.GetSize().GetWidth(), y2_);
            break;
        case kVRange:
            dc.DrawLine(x1_, 0, x1_, dc.GetSize().GetHeight());
            dc.DrawLine(x2_, 0, x2_, dc.GetSize().GetHeight());
            break;
        case kHRange:
            dc.DrawLine(0, y1_, dc.GetSize().GetWidth(), y1_);
            dc.DrawLine(0, y2_, dc.GetSize().GetWidth(), y2_);
            break;
        case kFunction:
            assert(0);
            break;
        case kNone:
            break;
    }
}

void Overlay::draw_lines(int n, wxPoint points[])
{
    wxClientDC dc(panel_) ;
    panel_->blit(dc);
    if (n <= 0)
        return;
    dc.SetPen(wxPen(color_, 1, wxPENSTYLE_SHORT_DASH));
    dc.DrawLines(n, points);
}


//===============================================================
//                FPlot (plot with data and fitted curves)
//===============================================================

FPlot::FPlot(wxWindow *parent)
   : BufferedPanel(parent),
     overlay(this),
     pen_width(1),
     draw_sigma(false),
     downX(INT_MIN), downY(INT_MIN),
     esc_source_(NULL)
{
}

FPlot::~FPlot()
{
}

void FPlot::set_font(wxDC &dc, wxFont const& font)
{
    if (pen_width > 1) {
        wxFont f = font;
        f.SetPointSize(f.GetPointSize() * pen_width);
        dc.SetFont(f);
    }
    else
        dc.SetFont(font);
}

void draw_line_with_style(wxDC& dc, wxPenStyle style,
                          wxCoord X1, wxCoord Y1, wxCoord X2, wxCoord Y2)
{
    wxPen pen = dc.GetPen();
    wxPenStyle old_style = pen.GetStyle();
    pen.SetStyle(style);
    dc.SetPen(pen);
    dc.DrawLine (X1, Y1, X2, Y2);
    pen.SetStyle(old_style);
    dc.SetPen(pen);
}

void FPlot::draw_vertical_lines_on_overlay(int X1, int X2)
{
    wxPoint pp[4] = {
        wxPoint(X1, 0),
        wxPoint(X1, GetClientSize().GetHeight() + 1),
        wxPoint(X2, GetClientSize().GetHeight() + 1),
        wxPoint(X2, 0)
    };
    if (X1 < 0)
        overlay.draw_lines(0, pp);
    else if (X2 < 0)
        overlay.draw_lines(2, pp);
    else
        overlay.draw_lines(4, pp);
}

void FPlot::set_bg_color(wxColour const& c)
{
    BufferedPanel::set_bg_color(c);
    overlay.bg_color_updated(c);
}

/// draw x axis tics
void FPlot::draw_xtics (wxDC& dc, Rect const &v, bool set_pen)
{
    if (set_pen) {
        dc.SetPen(wxPen(xAxisCol, pen_width));
        dc.SetTextForeground(xAxisCol);
    }
    set_font(dc, ticsFont);
    // get tics text height
    wxCoord h;
    dc.GetTextExtent(wxT("1234567890"), 0, &h);

    vector<double> minors;
    vector<double> x_tics = scale_tics_step(v.left(), v.right(), x_max_tics,
                                            minors, xs.logarithm);

    //if x axis is visible tics are drawed at the axis,
    //otherwise tics are drawed at the bottom edge of the plot
    const int pixel_height = get_pixel_height(dc);
    int Y = pixel_height - h;
    if (x_axis_visible && !ys.logarithm && ys.px(0) >= 0 && ys.px(0) < Y)
        Y = ys.px(0);
    for (vector<double>::const_iterator i = x_tics.begin();
                                                    i != x_tics.end(); ++i) {
        int X = xs.px(*i);
        dc.DrawLine (X, Y, X, Y - x_tic_size);
        wxString label = format_label(*i, v.right() - v.left());
        wxCoord w;
        dc.GetTextExtent (label, &w, 0);
        dc.DrawText (label, X - w/2, Y + 1);
        if (x_grid) {
            wxPen pen = dc.GetPen();
            pen.SetStyle(wxPENSTYLE_DOT);
            wxDCPenChanger pen_changer(dc, pen);
            dc.DrawLine(X, 0, X, Y);
            dc.DrawLine(X, Y+1+h, X, pixel_height);
            /*
            draw_line_with_style(dc, wxPENSTYLE_DOT, X, 0, X, Y);
            draw_line_with_style(dc, wxPENSTYLE_DOT, X, Y+1+h, X, pixel_height);
            */
        }
    }
    //draw minor tics
    if (xminor_tics_visible)
        for (vector<double>::const_iterator i = minors.begin();
                                                    i != minors.end(); ++i) {
            int X = xs.px(*i);
            dc.DrawLine (X, Y, X, Y - x_tic_size);
        }
}

/// draw y axis tics
void FPlot::draw_ytics (wxDC& dc, Rect const &v, bool set_pen)
{
    if (set_pen) {
        dc.SetPen(wxPen(xAxisCol, pen_width));
        dc.SetTextForeground(xAxisCol);
    }
    set_font(dc, ticsFont);
    const int pixel_width = get_pixel_width(dc);


    //if y axis is visible, tics are drawed at the axis,
    //otherwise tics are drawed at the left hand edge of the plot
    int X = 0;
    if (y_axis_visible && xs.px(0) > 0
            && xs.px(0) < pixel_width - 10)
        X = xs.px(0);
    vector<double> minors;
    vector<double> y_tics = scale_tics_step(v.bottom(), v.top(), y_max_tics,
                                            minors, ys.logarithm);
    for (vector<double>::const_iterator i = y_tics.begin();
                                                    i != y_tics.end(); ++i) {
        int Y = ys.px(*i);
        dc.DrawLine (X, Y, X + y_tic_size, Y);
        wxString label = format_label(*i, v.top() - v.bottom());
        if (x_axis_visible && label == wxT("0"))
            continue;
        wxCoord w, h;
        dc.GetTextExtent (label, &w, &h);
        dc.DrawText (label, X + y_tic_size + 1, Y - h/2);
        if (y_grid) {
            draw_line_with_style(dc, wxPENSTYLE_DOT, 0,Y, X,Y);
            draw_line_with_style(dc, wxPENSTYLE_DOT, X+y_tic_size+1+w+1, Y,
                                                     pixel_width, Y);
        }
    }
    //draw minor tics
    if (yminor_tics_visible)
        for (vector<double>::const_iterator i = minors.begin();
                                                    i != minors.end(); ++i) {
            int Y = ys.px(*i);
            dc.DrawLine (X, Y, X + y_tic_size, Y);
        }
}

double FPlot::get_max_abs_y (double (*compute_y)(vector<Point>::const_iterator,
                                                 Model const*),
                         vector<Point>::const_iterator first,
                         vector<Point>::const_iterator last,
                         Model const* model)
{
    double max_abs_y = 0;
    for (vector<Point>::const_iterator i = first; i < last; ++i) {
        if (i->is_active) {
            double y = fabs(((*compute_y)(i, model)));
            if (y > max_abs_y) max_abs_y = y;
        }
    }
    return max_abs_y;
}


static
void stroke_lines(wxDC& dc, wxGraphicsContext *gc, int n, wxPoint2DDouble *pp)
{
    if (n < 2)
        return;
    if (gc) {
        gc->StrokeLines(n, pp);
    }
    else {
        wxPoint *points = new wxPoint[n];
        for (int i = 0; i < n; ++i) {
            points[i].x = iround(pp[i].m_x);
            points[i].y = iround(pp[i].m_y);
        }
        dc.DrawLines(n, points);
        delete [] points;
    }
}

void FPlot::draw_data_by_activity(wxDC& dc, wxPoint2DDouble *pp,
                                  const vector<bool>& aa, bool state)
{
    int len = aa.size();
    int count_state = count(aa.begin(), aa.end(), state);
    if (count_state == 0)
        return;
    wxGCDC* gdc = wxDynamicCast(&dc, wxGCDC);
    wxGraphicsContext *gc = gdc ? gdc->GetGraphicsContext() : NULL;
    if (line_between_points) {
        int start = (aa[0] == state ? 0 : -1);
        for (int i = 1; i != len; ++i) {
            if (aa[i] == state && start == -1)
                start = i;
            else if (aa[i] != state && start != -1) {
                wxPoint2DDouble start_bak = pp[start];
                wxPoint2DDouble i_bak = pp[i];
                if (start > 0) {
                    // draw half of the line between points start-1 and start
                    --start;
                    start_bak = pp[start];
                    pp[start] = (pp[start] + pp[start+1]) / 2;
                }
                // draw half of the line between points i-1 and i
                pp[i] = (pp[i-1] + pp[i]) / 2;
                stroke_lines(dc, gc, i - start + 1, pp + start);
                pp[i] = i_bak;
                pp[start] = start_bak;
                start = -1;
            }
        }
        if (start != -1) {
            wxPoint2DDouble start_bak = pp[start];
            if (start > 0) {
                // draw half of the line between points start-1 and start
                --start;
                start_bak = pp[start];
                pp[start] = (pp[start] + pp[start+1]) / 2;
            }
            stroke_lines(dc, gc, len - start, pp + start);
            pp[start] = start_bak;
        }
    }

    if (point_radius > 1) {
        int r = (point_radius - 1) * pen_width;
        if (gc) {
            for (int i = 0; i != len; ++i)
                if (aa[i] == state)
                    gc->DrawEllipse(pp[i].m_x - r/2., pp[i].m_y - r/2., r, r);
        }
        else
            for (int i = 0; i != len; ++i)
                if (aa[i] == state)
                    dc.DrawEllipse(pp[i].m_x - r/2, pp[i].m_y - r/2, r, r);
    }
    else if (!line_between_points) { // if we are here, point_radius == 1
        if (gc) {
            wxGraphicsPath path = gc->CreatePath();
            for (int i = 0; i != len; ++i)
                if (aa[i] == state) {
                    double rx = iround(pp[i].m_x);
                    double ry = iround(pp[i].m_y);
                    path.MoveToPoint(rx, ry - 0.1);
                    path.AddLineToPoint(rx, ry + 0.1);
                }
            gc->SetAntialiasMode(wxANTIALIAS_NONE);
            gc->StrokePath(path);
            gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        }
        else
            for (int i = 0; i != len; ++i)
                if (aa[i] == state)
                    dc.DrawPoint(iround(pp[i].m_x), iround(pp[i].m_y));
    }
}

void FPlot::draw_data (wxDC& dc,
                       double (*compute_y)(vector<Point>::const_iterator,
                                           Model const*),
                       Data const* data,
                       Model const* model,
                       wxColour const& color, wxColour const& inactive_color,
                       int Y_offset,
                       bool cumulative)
{
    if (data->is_empty())
        return;
    vector<Point>::const_iterator first = data->get_point_at(ftk->view.left()),
                                  last = data->get_point_at(ftk->view.right());
    if (first > data->points().begin())
        --first;
    if (last < data->points().end())
        ++last;
    // prepare coordinates
    int len = last - first;
    if (len <= 0)
        return;
    wxPoint2DDouble *pp = new wxPoint2DDouble[len];
    vector<bool> aa(len);
    vector<double> yy(len);
    Y_offset *= (get_pixel_height(dc) / 100);
    for (int i = 0; i != len; ++i) {
        const Point& p = *(first + i);
        pp[i].m_x = xs.px_d(p.x);
        yy[i] = (*compute_y)((first+i), model);
        if (cumulative && i > 0)
            yy[i] += yy[i-1];
        pp[i].m_y = ys.px_d(yy[i]) - Y_offset;
        aa[i] = p.is_active;
    }

    // draw inactive
    wxColour icol = inactive_color.Ok() ? inactive_color : inactiveDataCol;
    dc.SetPen(wxPen(icol, pen_width));
    dc.SetBrush(wxBrush(icol, wxSOLID));
    draw_data_by_activity(dc, pp, aa, false);
    if (draw_sigma)
        for (int i = 0; i != len; ++i)
            if (!aa[i]) {
                double sigma = (first + i)->sigma;
                dc.DrawLine(iround(pp[i].m_x), ys.px(yy[i] - sigma) - Y_offset,
                            iround(pp[i].m_x), ys.px(yy[i] + sigma) - Y_offset);
            }

    // draw active
    wxColour acol = color.Ok() ? color : activeDataCol;
    dc.SetPen(wxPen(acol, pen_width));
    dc.SetBrush(wxBrush(acol, wxSOLID));
    draw_data_by_activity(dc, pp, aa, true);
    if (draw_sigma)
        for (int i = 0; i != len; ++i)
            if (aa[i]) {
                double sigma = (first + i)->sigma;
                dc.DrawLine(iround(pp[i].m_x), ys.px(yy[i] - sigma) - Y_offset,
                            iround(pp[i].m_x), ys.px(yy[i] + sigma) - Y_offset);
            }

    delete [] pp;
}

void FPlot::set_scale(int pixel_width, int pixel_height)
{
    Rect const &v = ftk->view;
    if (pixel_width > 0)
	xs.set(v.left(), v.right(), pixel_width);
    if (pixel_height > 0)
	ys.set(v.top(), v.bottom(), pixel_height);
}

int FPlot::get_special_point_at_pointer(wxMouseEvent& event)
{
    // searching the closest peak-top and distance from it, d = dx + dy < 10
    int nearest = -1;
    int min_dist = 10;
    for (vector<wxPoint>::const_iterator i = special_points.begin();
                                             i != special_points.end(); ++i) {
        int d = abs(event.GetX() - i->x) + abs(event.GetY() - i->y);
        if (d < min_dist) {
            min_dist = d;
            nearest = i - special_points.begin();
        }
    }
    return nearest;
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
    ticsFont = cfg_read_font(cf, wxT("ticsFont"), wxFont(
#ifdef __WXMAC__
                                                         10,
#else
                                                         8,
#endif
                                                         wxFONTFAMILY_DEFAULT,
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


void FPlot::OnKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_ESCAPE)
        cancel_action();
    else
        event.Skip();
}


void FPlot::connect_esc_to_cancel(bool connect)
{
    if (esc_source_ != NULL) {
        esc_source_->Disconnect(wxID_ANY, wxEVT_KEY_DOWN,
                                wxKeyEventHandler(FPlot::OnKeyDown), 0, this);
        esc_source_ = NULL;
    }

    if (connect) {
        wxWindow *fw = wxWindow::FindFocus();
        if (fw == NULL || fw == this)
            return;
        esc_source_ = fw;
        fw->Connect(wxID_ANY, wxEVT_KEY_DOWN,
                    wxKeyEventHandler(FPlot::OnKeyDown), 0, this);
    }
}

