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
    : ProportionalSplitter(parent, id)
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
        for (int i = 0; i < 2; ++i)
            if (aux_visible(i)) {
                if (now)
                    aux_plot[i]->redraw_now();
                else
                    aux_plot[i]->refresh();
            }
    }
}

bool PlotPane::is_background_white()
{
    //have all visible plots white background?
    if (plot->get_bg_color() != *wxWHITE)
        return false;
    for (int i = 0; i < 2; ++i)
        if (aux_visible(i) && aux_plot[i]->get_bg_color() != *wxWHITE)
            return false;
    return true;
}

bool PlotPane::aux_visible(int n) const
{
    return IsSplit() && (aux_split->GetWindow1() == aux_plot[n]
                         || aux_split->GetWindow2() == aux_plot[n]);
}

void PlotPane::show_aux(int n, bool show)
{
    if (aux_visible(n) == show)
        return;

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

void PlotPane::draw_vertical_lines(int X1, int X2, FPlot* skip)
{
    if (plot != skip)
        plot->draw_vertical_lines_on_overlay(X1, X2);
    for (int i = 0; i < 2; ++i)
        if (aux_visible(i) && aux_plot[i] != skip)
            aux_plot[i]->draw_vertical_lines_on_overlay(X1, X2);
}

