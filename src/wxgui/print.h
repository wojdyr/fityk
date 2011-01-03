// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__WX_PRINT__H__
#define FITYK__WX_PRINT__H__

#include <wx/config.h>
#include <wx/print.h>

class PlotPane;
class SpinCtrl;

class PrintManager
{
public:
    bool landscape;
    bool colors;
    //int scale;
    //bool keep_ratio;
    bool plot_aux[2], plot_borders;
    PlotPane* plot_pane;

    PrintManager(PlotPane* pane);
    ~PrintManager();
    wxPrintData& get_print_data();
    wxPageSetupDialogData& get_page_data();
    void print();
    void print_to_psfile();
    void printPreview();
    void pageSetup();
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
private:
    wxPrintData *print_data;
    wxPageSetupDialogData *page_setup_data;
};


class PageSetupDialog: public wxDialog
{
public:
    PageSetupDialog(wxWindow *parent, PrintManager *print_mgr);
    void OnOk(wxCommandEvent& event);
protected:
    PrintManager *pm;
    wxRadioBox *orientation, *colors;
    wxComboBox *papers;
    //wxCheckBox *keep_ratio;
    wxCheckBox *plot_aux[2], *plot_borders;
    SpinCtrl *left_margin, *right_margin, *top_margin, *bottom_margin;
    //SpinCtrl *scale;
    DECLARE_EVENT_TABLE()
};


class FPrintout: public wxPrintout
{
public:
    FPrintout(PrintManager const* print_manager);
    bool HasPage(int page) { return (page == 1); }
    bool OnPrintPage(int page);
    void GetPageInfo(int *minPage,int *maxPage,int *selPageFrom,int *selPageTo)
        { *minPage = *maxPage = *selPageFrom = *selPageTo = 1; }
private:
    PrintManager const* pm;
};

void do_print_plots(wxDC *dc, PrintManager const* pm);


#endif
