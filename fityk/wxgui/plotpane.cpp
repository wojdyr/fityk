// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <wx/wx.h>

#include "plotpane.h"
#include "plot.h"
#include "mplot.h"
#include "aplot.h"
#include "frame.h" // exec()

using namespace std;


void ZoomHistory::push(const string& s)
{
    if (pos_ + 1 < items_.size())
        items_.erase(items_.begin() + pos_ + 1, items_.end());
    items_.push_back(s);
    ++pos_;
    if (pos_ > 50) {
        items_.erase(items_.begin(), items_.begin() + 10);
        pos_ -= 10;
    }
}

void ZoomHistory::set_pos(size_t p)
{
    size_t old_pos = pos_;
    pos_ = std::min(p,  items_.size() - 1);
    if (pos_ != old_pos)
        exec("plot " + items_[pos_]);
}

PlotPane::PlotPane(wxWindow *parent, wxWindowID id)
    : ProportionalSplitter(parent, id)
{
    plot_ = new MainPlot(this);
    aux_split_ = new ProportionalSplitter(this, -1, 0.5);
    SplitHorizontally(plot_, aux_split_);

    aux_plot_[0] = new AuxPlot(aux_split_, plot_, wxT("0"));
    aux_plot_[1] = new AuxPlot(aux_split_, plot_, wxT("1"));
    aux_plot_[1]->Show(false);
    aux_split_->Initialize(aux_plot_[0]);
}

void PlotPane::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/PlotPane"));
    cf->Write(wxT("PlotPaneProportion"), GetProportion());
    cf->Write(wxT("AuxPlotsProportion"), aux_split_->GetProportion());
    cf->Write(wxT("ShowAuxPane0"), aux_visible(0));
    cf->Write(wxT("ShowAuxPane1"), aux_visible(1));
    plot_->save_settings(cf);
    for (int i = 0; i < 2; ++i)
        aux_plot_[i]->save_settings(cf);
}

void PlotPane::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/PlotPane"));
    SetProportion(cfg_read_double(cf, wxT("PlotPaneProportion"), 0.80));
    aux_split_->SetProportion(
                        cfg_read_double(cf, wxT("AuxPlotsProportion"), 0.5));
    show_aux(0, cfg_read_bool(cf, wxT("ShowAuxPane0"), true));
    show_aux(1, cfg_read_bool(cf, wxT("ShowAuxPane1"), false));
    plot_->read_settings(cf);
    for (int i = 0; i < 2; ++i)
        aux_plot_[i]->read_settings(cf);
}

void PlotPane::refresh_plots(bool now, WhichPlot which_plot)
{
    if (now)
        plot_->redraw_now();
    else
        plot_->refresh();
    if (which_plot == kAllPlots) {
        for (int i = 0; i < 2; ++i)
            if (aux_visible(i)) {
                if (now)
                    aux_plot_[i]->redraw_now();
                else
                    aux_plot_[i]->refresh();
            }
    }
}

bool PlotPane::is_background_white()
{
    //have all visible plots white background?
    if (plot_->get_bg_color() != *wxWHITE)
        return false;
    for (int i = 0; i < 2; ++i)
        if (aux_visible(i) && aux_plot_[i]->get_bg_color() != *wxWHITE)
            return false;
    return true;
}

bool PlotPane::aux_visible(int n) const
{
    return IsSplit() && (aux_split_->GetWindow1() == aux_plot_[n]
                         || aux_split_->GetWindow2() == aux_plot_[n]);
}

void PlotPane::show_aux(int n, bool show)
{
    if (aux_visible(n) == show)
        return;

    if (show) {
        if (!IsSplit()) { //both where invisible
            SplitHorizontally(plot_, aux_split_);
            aux_split_->Show(true);
            assert(!aux_split_->IsSplit());
            if (aux_split_->GetWindow1() == aux_plot_[n])
                ;
            else {
                aux_split_->SplitHorizontally(aux_plot_[0], aux_plot_[1]);
                aux_plot_[n]->Show(true);
                aux_split_->Unsplit(aux_plot_[n==0 ? 1 : 0]);
            }
        }
        else {//one was invisible
            aux_split_->SplitHorizontally(aux_plot_[0], aux_plot_[1]);
            aux_plot_[n]->Show(true);
        }
    }
    else { //hide
        if (aux_split_->IsSplit()) //both where visible
            aux_split_->Unsplit(aux_plot_[n]);
        else // only one was visible
            Unsplit(); //hide whole aux_split_
    }
}

void PlotPane::draw_vertical_lines(int X1, int X2, FPlot* skip)
{
    if (plot_ != skip)
        plot_->draw_vertical_lines_on_overlay(X1, X2);
    for (int i = 0; i < 2; ++i)
        if (aux_visible(i) && aux_plot_[i] != skip)
            aux_plot_[i]->draw_vertical_lines_on_overlay(X1, X2);
}

wxBitmap PlotPane::prepare_bitmap_for_export(int W, int H, bool include_aux)
{
    int my = get_plot()->get_bitmap().GetSize().y;
    int th = H;
    int ah[2] = { 0, 0 };
    for (int i = 0; i != 2; ++i)
        if (include_aux && aux_visible(i)) {
            int ay = get_aux_plot(i)->GetClientSize().y;
            ah[i] = iround(ay * H / my);
            th += 5 + ah[i];
        }

    wxBitmap bmp(W, th);
    wxMemoryDC memory_dc(bmp);
    MainPlot *plot = get_plot();
    MouseModeEnum old_mode = plot->get_mouse_mode();
    // We change the mode to avoid drawing peaktops.
    // In the case of mmd_bg peaktops are not drawn anyway,
    // and changing the mode would prevent drawing the baseline.
    if (plot->get_mouse_mode() != mmd_bg)
        plot->set_mode(mmd_readonly);
    memory_dc.DrawBitmap(plot->draw_on_bitmap(W, H), 0, 0);
    plot->set_mode(old_mode);

    int y = H + 5;
    for (int i = 0; i != 2; ++i)
        if (include_aux && aux_visible(i)) {
            AuxPlot *aplot = get_aux_plot(i);
            memory_dc.DrawBitmap(aplot->draw_on_bitmap(W, ah[i]), 0, y);
            y += ah[i] + 5;
        }
    return bmp;
}

