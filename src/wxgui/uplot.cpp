// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

/// In this file: utilities for making plot (BufferedPanel, scale_tics_step())

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "uplot.h"

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

void BufferedPanel::buffered_draw()
{
    cout << "BufferedPanel::buffered_draw()" << endl;
    wxPaintDC dc(this);
    // check size
    wxCoord w, h;
    dc.GetSize(&w, &h);
    if (!buffer_.Ok() || w != buffer_.GetWidth() || h != buffer_.GetHeight()) {
        memory_dc_.SelectObject(wxNullBitmap);
        buffer_ = wxBitmap(w, h);
        memory_dc_.SelectObject(buffer_);
        memory_dc_.SetBackground(wxBrush(bg_color_));
        dirty_ = true;
    }
    // replot if it's necessary
    if (dirty_) {
        dirty_ = false;
        memory_dc_.SetLogicalFunction(wxCOPY);
        memory_dc_.Clear();
        draw(memory_dc_);
    }

    // copy bitmap to window
    dc.Blit(0, 0, buffer_.GetWidth(), buffer_.GetHeight(), &memory_dc_, 0, 0);
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
        for (double t = s * ceil(beg / s); t < end; t += s) {
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

