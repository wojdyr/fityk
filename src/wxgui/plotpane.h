// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_PLOTPANE_H_
#define FITYK_WX_PLOTPANE_H_

#include "cmn.h"  //for MouseModeEnum, ProportionalSplitter

class MainPlot;
class AuxPlot;
class FPlot;

class ZoomHistory
{
public:
    ZoomHistory(const std::string& s) : items_(1, s), pos_(0) {}
    void push(const std::string& s);
    void move(int n) { set_pos(n >= -(int)pos_ ? pos_ + n : 0); }
    void set_pos(size_t p);
    size_t pos() const { return pos_; }
    const std::vector<std::string>& items() const { return items_; }

private:
    std::vector<std::string> items_;
    size_t pos_;
};

/// A pane containing main and auxiliary plots and a proxy for some
/// of the methods of plots.
class PlotPane : public ProportionalSplitter
{
    friend class FPrintout;
public:
    PlotPane(wxWindow *parent, wxWindowID id=-1);
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
    void refresh_plots(bool now, WhichPlot which_plot);
    bool is_background_white();
    const MainPlot* get_plot() const { return plot_; }
    MainPlot* get_plot() { return plot_; }
    AuxPlot* get_aux_plot(int n) const
                     { assert(n>=0 && n<2); return aux_plot_[n]; }
    void show_aux(int n, bool show);
    bool aux_visible(int n) const;
    void draw_vertical_lines(int X1, int X2, FPlot* skip);
    wxBitmap prepare_bitmap_for_export(int W, int H, bool include_aux);

private:
    MainPlot *plot_;
    ProportionalSplitter *aux_split_;
    AuxPlot *aux_plot_[2];
};

#endif // FITYK_WX_PLOTPANE_H_
