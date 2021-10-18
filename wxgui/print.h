// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_PRINT_H_
#define FITYK_WX_PRINT_H_

#include <wx/config.h>
#include <wx/print.h>

class PlotPane;
class wxSpinCtrl;

// it can be confusing how the code is split into classes here,
// but let's leave it as it is

class PrintManager
{
public:
    bool colors;
    bool plot_aux[2], plot_borders;
    int margin_left, margin_right, margin_top, margin_bottom;
    PlotPane* plot_pane;
    wxPrintData print_data;

    PrintManager(PlotPane* pane);
    ~PrintManager();
    void print();
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
};


class PageSetupDialog: public wxDialog
{
public:
    PageSetupDialog(wxWindow *parent, PrintManager *print_mgr);
    void OnApply(wxCommandEvent& event);
private:
    PrintManager *pm;
    wxRadioBox *orientation_rb, *colors_rb;
    wxCheckBox *plot_aux_cb[2], *plot_borders_cb;
    wxSpinCtrl *left_margin_sc, *right_margin_sc,
               *top_margin_sc, *bottom_margin_sc;
    DECLARE_EVENT_TABLE()
};


class FPrintout: public wxPrintout
{
public:
    FPrintout(PrintManager const* pm) : wxPrintout("fityk"), pm_(pm) {}
    bool HasPage(int page) { return (page == 1); }
    bool OnPrintPage(int page);
    void GetPageInfo(int *minPage,int *maxPage,int *selPageFrom,int *selPageTo)
        { *minPage = *maxPage = *selPageFrom = *selPageTo = 1; }
private:
    PrintManager const* pm_;
};

#endif
