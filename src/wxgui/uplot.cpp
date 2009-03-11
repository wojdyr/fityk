// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id: plot.cpp 391 2008-02-15 02:59:57Z wojdyr $

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

void BufferedPanel::clear()
{
    memory_dc.SetLogicalFunction(wxCOPY);
    memory_dc.SetBackground(wxBrush(backgroundCol));
    memory_dc.Clear();
}

void BufferedPanel::clear_and_draw()
{
    if (!buffer.Ok())
        return;
    clear();
    draw(memory_dc);
}

void BufferedPanel::buffered_draw()
{
    wxPaintDC dc(this);
    if (resize_buffer(dc)) 
        clear_and_draw();
    dc.Blit(0, 0, buffer.GetWidth(), buffer.GetHeight(), &memory_dc, 0, 0);
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

