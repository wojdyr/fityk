// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <wx/wx.h>

#include "pplot.h"
#include "plot.h"
#include "mplot.h"
#include "aplot.h"
#include "../logic.h" //ftk->view

extern Ftk *ftk; // frame.cpp

using namespace std;


PlotPane::PlotPane(wxWindow *parent, wxWindowID id)
    : ProportionalSplitter(parent, id),
      crosshair_cursor(false)
{
    plot = new MainPlot(this);
    aux_split = new ProportionalSplitter(this, -1, 0.5);
    SplitHorizontally(plot, aux_split);

    aux_plot[0] = new AuxPlot(aux_split, plot, wxT("0"));
    aux_plot[1] = new AuxPlot(aux_split, plot, wxT("1"));
    aux_plot[1]->Show(false);
    aux_split->Initialize(aux_plot[0]);
}

void PlotPane::zoom_forward()
{
    const int max_length_of_zoom_history = 10;
    zoom_hist.push_back(ftk->view.str());
    if (size(zoom_hist) > max_length_of_zoom_history)
        zoom_hist.erase(zoom_hist.begin());
}

string PlotPane::zoom_backward(int n)
{
    if (n < 1 || zoom_hist.empty())
        return "";
    int pos = zoom_hist.size() - n;
    if (pos < 0) pos = 0;
    string val = zoom_hist[pos];
    zoom_hist.erase(zoom_hist.begin() + pos, zoom_hist.end());
    return val;
}

void PlotPane::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/PlotPane"));
    cf->Write(wxT("PlotPaneProportion"), GetProportion());
    cf->Write(wxT("AuxPlotsProportion"), aux_split->GetProportion());
    cf->Write(wxT("ShowAuxPane0"), aux_visible(0));
    cf->Write(wxT("ShowAuxPane1"), aux_visible(1));
    plot->save_settings(cf);
    for (int i = 0; i < 2; ++i)
        aux_plot[i]->save_settings(cf);
}

void PlotPane::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/PlotPane"));
    SetProportion(cfg_read_double(cf, wxT("PlotPaneProportion"), 0.80));
    aux_split->SetProportion(
                        cfg_read_double(cf, wxT("AuxPlotsProportion"), 0.5));
    show_aux(0, cfg_read_bool(cf, wxT("ShowAuxPane0"), true));
    show_aux(1, cfg_read_bool(cf, wxT("ShowAuxPane1"), false));
    plot->read_settings(cf);
    for (int i = 0; i < 2; ++i)
        aux_plot[i]->read_settings(cf);
}

void PlotPane::refresh_plots(bool now, WhichPlot which_plot)
{
    if (now)
        plot->redraw_now();
    else
        plot->refresh();
    if (which_plot == kAllPlots) {
        vector<FPlot*> vp = get_visible_plots();
        for (vector<FPlot*>::const_iterator i = vp.begin(); i != vp.end(); ++i)
            if (now)
                (*i)->redraw_now();
            else
                (*i)->refresh();
    }
}

void PlotPane::set_mouse_mode(MouseModeEnum m)
{
    plot->set_mouse_mode(m);
}

bool PlotPane::is_background_white()
{
    //have all visible plots white background?
    vector<FPlot*> vp = get_visible_plots();
    for (vector<FPlot*>::const_iterator i = vp.begin(); i != vp.end(); ++i)
        if ((*i)->get_bg_color() != *wxWHITE)
            return false;
    return true;
}

BgManager* PlotPane::get_bg_manager() { return &plot->bgm; }

const std::vector<FPlot*> PlotPane::get_visible_plots() const
{
    vector<FPlot*> visible;
    visible.push_back(plot);
    for (int i = 0; i < 2; ++i)
        if (aux_visible(i))
            visible.push_back(aux_plot[i]);
    return visible;
}

bool PlotPane::aux_visible(int n) const
{
    return IsSplit() && (aux_split->GetWindow1() == aux_plot[n]
                         || aux_split->GetWindow2() == aux_plot[n]);
}

void PlotPane::show_aux(int n, bool show)
{
    if (aux_visible(n) == show) return;

    if (show) {
        if (!IsSplit()) { //both where invisible
            SplitHorizontally(plot, aux_split);
            aux_split->Show(true);
            assert(!aux_split->IsSplit());
            if (aux_split->GetWindow1() == aux_plot[n])
                ;
            else {
                aux_split->SplitHorizontally(aux_plot[0], aux_plot[1]);
                aux_plot[n]->Show(true);
                aux_split->Unsplit(aux_plot[n==0 ? 1 : 0]);
            }
        }
        else {//one was invisible
            aux_split->SplitHorizontally(aux_plot[0], aux_plot[1]);
            aux_plot[n]->Show(true);
        }
    }
    else { //hide
        if (aux_split->IsSplit()) //both where visible
            aux_split->Unsplit(aux_plot[n]);
        else // only one was visible
            Unsplit(); //hide whole aux_split
    }
}


/// draw "crosshair cursor" -> erase old and draw new;
/// can be used to draw vertical line, by calling with Y=-1
void PlotPane::update_crosshair(int X, int Y)
{
    static bool drawn = false;
    static int oldX = 0, oldY = 0;
    if (drawn) {
        do_draw_crosshair(oldX, oldY);
        drawn = false;
    }
    // there is a 
    if ((crosshair_cursor || Y == -1) && X >= 0) {
        do_draw_crosshair(X, Y);
        oldX = X;
        oldY = Y;
        drawn = true;
    }
}

void PlotPane::do_draw_crosshair(int X, int Y)
{
    plot->draw_crosshair(X, Y);
    for (int i = 0; i < 2; ++i)
        if (aux_visible(i))
            aux_plot[i]->draw_crosshair(X, -1);
}

