// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_PLOTPANE_H_
#define FITYK_WX_PLOTPANE_H_

#include "cmn.h"  //for MouseModeEnum, ProportionalSplitter

class MainPlot;
class AuxPlot;
class BgManager;
class FPlot;

/// A pane containing main and auxiliary plots, zoom history,
/// proxy for some of the methods  of plots.
class PlotPane : public ProportionalSplitter
{
    friend class FPrintout;
public:
    PlotPane(wxWindow *parent, wxWindowID id=-1);
    void zoom_forward();
    std::string zoom_backward(int n=1);
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
    void refresh_plots(bool now, WhichPlot which_plot);
    bool is_background_white();
    std::vector<std::string> const& get_zoom_hist() const { return zoom_hist; }
    MainPlot const* get_plot() const { return plot; }
    MainPlot* get_plot() { return plot; }
    AuxPlot* get_aux_plot(int n) const
                     { assert(n>=0 && n<2); return aux_plot[n]; }
    void show_aux(int n, bool show);
    bool aux_visible(int n) const;
    void draw_vertical_lines(int X1, int X2, FPlot* skip);

private:
    MainPlot *plot;
    ProportionalSplitter *aux_split;
    AuxPlot *aux_plot[2];
    std::vector<std::string> zoom_hist;
};

#endif // FITYK_WX_PLOTPANE_H_
