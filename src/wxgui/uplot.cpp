// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// utilities for making plot (BufferedPanel, scale_tics_step())

#include <wx/wx.h>
#include <wx/dcgraph.h>

#include "uplot.h"
#include "cmn.h"

#if !defined(XYCONVERT) && !defined(STANDALONE_POWDIFPAT)
#include "frame.h" // frame->antialias()
inline bool antialias() { return frame->antialias(); }
#else
inline bool antialias() { return false; }
#endif

using namespace std;

BufferedPanel::BufferedPanel(wxWindow *parent)
   : wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize,
             wxNO_BORDER|wxFULL_REPAINT_ON_RESIZE),
     dirty_(true)
{
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    Connect(wxEVT_IDLE, wxIdleEventHandler(BufferedPanel::OnIdle));
}

void BufferedPanel::redraw_now()
{
    dirty_ = true;
    Refresh(false);
    Update();
}

void BufferedPanel::update_buffer_and_blit()
{
    //cout << "BufferedPanel::update_buffer_and_blit()" << endl;
    wxPaintDC pdc(this);
    // check size
    wxCoord w, h;
    pdc.GetSize(&w, &h);
    if (!buffer_.Ok() || w != buffer_.GetWidth() || h != buffer_.GetHeight()) {
        memory_dc_.SelectObject(wxNullBitmap);
        buffer_ = wxBitmap(w, h);
        memory_dc_.SelectObject(buffer_);
        memory_dc_.SetBackground(wxBrush(bg_color_));
        dirty_ = true;
    }

    // update the buffer if necessary
    if (dirty_) {
        memory_dc_.SetLogicalFunction(wxCOPY);
        memory_dc_.Clear();
        if (antialias()) {
            wxGCDC gdc(memory_dc_);
            draw(gdc);
        }
        else
            draw(memory_dc_);
        // This condition is almost always true. It was added because on
        // wxGTK 2.8 with some window managers, after loading a data file
        // the next paint event had the update region set to the area
        // of the file dialog, and the rest of the plot remained not updated.
        if (GetUpdateRegion().GetBox().GetSize() == pdc.GetSize())
            dirty_ = false;
    }

    // copy bitmap to window
    pdc.Blit(0, 0, buffer_.GetWidth(), buffer_.GetHeight(), &memory_dc_, 0, 0);
}

void BufferedPanel::OnIdle(wxIdleEvent&)
{
    if (dirty_)
        Refresh(false);
}

void BufferedPanel::set_bg_color(wxColour const& c)
{
    bg_color_ = c;
    if (memory_dc_.IsOk())
        memory_dc_.SetBackground(wxBrush(c));
}

PlotWithTics::PlotWithTics(wxWindow* parent)
    : BufferedPanel(parent)
{
    Connect(GetId(), wxEVT_PAINT,
                  (wxObjectEventFunction) &PlotWithTics::OnPaint);
}

void PlotWithTics::draw_tics(wxDC &dc, double x_min, double x_max,
                             double y_min, double y_max)
{
    // prepare scaling
    double const margin = 0.1;
    double dx = x_max - x_min;
    double dy = y_max - y_min;
    int W = dc.GetSize().GetWidth();
    int H = dc.GetSize().GetHeight();
    xScale = (1 - 1.2 * margin) *  W / dx;
    yScale = - (1 - 1.2 * margin) * H / dy;
    xOffset = - x_min * xScale + margin * W;
    yOffset = H - y_min * yScale - margin * H;

    // draw scale
    dc.SetPen(*wxWHITE_PEN);
    dc.SetTextForeground(*wxWHITE);
    dc.SetFont(*wxSMALL_FONT);

    // ... horizontal
    vector<double> minors;
    vector<double> tics = scale_tics_step(x_min, x_max, 4, minors);
    for (vector<double>::const_iterator i = tics.begin(); i != tics.end(); ++i){
        int X = getX(*i);
        wxString label = format_label(*i, x_max - x_min);
        wxCoord tw, th;
        dc.GetTextExtent (label, &tw, &th);
        int Y = dc.DeviceToLogicalY(H - th - 2);
        dc.DrawText (label, X - tw/2, Y + 1);
        dc.DrawLine (X, Y, X, Y - 4);
    }

    // ... vertical
    tics = scale_tics_step(y_min, y_max, 4, minors);
    for (vector<double>::const_iterator i = tics.begin(); i != tics.end(); ++i){
        int Y = getY(*i);
        wxString label = format_label(*i, y_max - y_min);
        wxCoord tw, th;
        dc.GetTextExtent (label, &tw, &th);
        dc.DrawText (label, dc.DeviceToLogicalX(5), Y - th/2);
        dc.DrawLine (dc.DeviceToLogicalX(0), Y, dc.DeviceToLogicalX(4), Y);
    }
}

void PlotWithTics::draw_axis_labels(wxDC& dc, string const& x_name,
                                    string const& y_name)
{
    int W = dc.GetSize().GetWidth();
    int H = dc.GetSize().GetHeight();
    if (!x_name.empty()) {
        wxCoord tw, th;
        dc.GetTextExtent (s2wx(x_name), &tw, &th);
        dc.DrawText (s2wx(x_name), (W - tw)/2, 2);
    }
    if (!y_name.empty()) {
        wxCoord tw, th;
        dc.GetTextExtent (s2wx(y_name), &tw, &th);
        dc.DrawRotatedText (s2wx(y_name), W - 2, (H - tw)/2, 270);
    }
}


vector<double> scale_tics_step (double beg, double end, int max_tics,
                                vector<double> &minors, bool logarithm)
{
    vector<double> result;
    minors.clear();
    if (beg >= end || max_tics <= 0)
        return result;
    if (logarithm) {
        if (beg <= 0)
            beg = 1e-1;
        if (end <= beg)
            end = 2*beg;
        double min_logstep = (log10(end/beg)) / max_tics;
        bool with_2_5 = (min_logstep < log10(2.));
        double logstep = ceil(min_logstep);
        double step0 = pow(10, logstep * ceil(log10(beg) / logstep));
        for (int i = 2; i <= 9; ++i) {
            double v = step0/10. * i;
            if (v > beg && v < end) {
                if (with_2_5 && (i == 2 || i == 5))
                    result.push_back(v);
                else
                    minors.push_back(v);
            }
        }
        for (double t = step0; t < end; t *= pow(10,logstep)) {
            result.push_back(t);
            for (int i = 2; i <= 9; ++i) {
                double v = t * i;
                if (v > beg && v < end) {
                    if (with_2_5 && (i == 2 || i == 5))
                        result.push_back(v);
                    else
                        minors.push_back(v);
                }
            }
        }
    }
    else { // !logarithm
        double min_step = (end - beg) / max_tics;
        double s = pow(10, floor (log10 (min_step)));
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
        for (double t = s * floor(beg / s); t < end; t += s) {
            if (t > beg) {
                // make sure that 0 is not displayed as e.g. -2.7893e-17
                if (fabs(t) < 1e-6 * fabs(min_step))
                    t = 0.;
                result.push_back(t);
            }
            for (int i = 1; i < minor_div; ++i) {
                double v = t + s * i / minor_div;
                if (v > beg && v < end)
                    minors.push_back(v);
            }
        }
    }
    return result;
}

