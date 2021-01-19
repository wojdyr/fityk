// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

///  printing

#include <vector>
#include <wx/wx.h>
#include <wx/statline.h>

#include "print.h"
#include "cmn.h"
#include "plotpane.h"
#include "plot.h"
#include "mplot.h"
#include "aplot.h"

using namespace std;

//======================================================================
//                         PageSetupDialog
//======================================================================
BEGIN_EVENT_TABLE(PageSetupDialog, wxDialog)
    EVT_BUTTON(wxID_PRINT, PageSetupDialog::OnApply)
    EVT_BUTTON(wxID_APPLY, PageSetupDialog::OnApply)
END_EVENT_TABLE()

PageSetupDialog::PageSetupDialog(wxWindow *parent, PrintManager *print_mgr)
    : wxDialog(parent, -1, wxT("Page Setup"),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
      pm(print_mgr)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    // orientation
    wxString orient_choices[] = { wxT("Portrait"), wxT("Landscape") };
    orientation_rb = new wxRadioBox(this, -1, wxT("Orientation"),
                                    wxDefaultPosition, wxDefaultSize,
                                    2, orient_choices,
                                    2, wxRA_SPECIFY_COLS);
    top_sizer->Add(orientation_rb, 0, wxALL|wxEXPAND, 5);

    wxStaticBoxSizer *hsizer = new wxStaticBoxSizer(wxVERTICAL, this,
                                                 wxT("Margins (mm)"));
    wxBoxSizer *m1sizer = new wxBoxSizer(wxHORIZONTAL);
    m1sizer->Add(new wxStaticText(this, -1, "Top"),
                 0, wxALIGN_CENTER_VERTICAL);
    top_margin_sc = make_wxspinctrl(this, -1, 0, -100, 500);
    m1sizer->Add(top_margin_sc, 0, wxALL, 5);
    hsizer->Add(m1sizer, 0, wxALIGN_CENTER|wxRIGHT, 15);
    wxBoxSizer *m2sizer = new wxBoxSizer(wxHORIZONTAL);
    m2sizer->Add(new wxStaticText(this, -1, "Left"),
                 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    left_margin_sc = make_wxspinctrl(this, -1, 0, -100, 500);
    m2sizer->Add(left_margin_sc, 0, wxLEFT, 5);
    m2sizer->AddStretchSpacer();
    m2sizer->Add(new wxStaticText(this, -1, "Right"),
                 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
    right_margin_sc = make_wxspinctrl(this, -1, 0, -100, 500);
    m2sizer->Add(right_margin_sc, 0, wxLEFT|wxRIGHT|wxALIGN_RIGHT, 5);
    hsizer->Add(m2sizer, 0, wxEXPAND);
    wxBoxSizer *m3sizer = new wxBoxSizer(wxHORIZONTAL);
    m3sizer->Add(new wxStaticText(this, -1, "Bottom"),
                 0, wxALIGN_CENTER_VERTICAL);
    bottom_margin_sc = make_wxspinctrl(this, -1, 0, -100, 500);
    m3sizer->Add(bottom_margin_sc, 0, wxALL, 5);
    hsizer->Add(m3sizer, 0, wxALIGN_CENTER|wxRIGHT, 30);
    top_sizer->Add(hsizer, 0, wxALL|wxEXPAND, 5);

    wxString color_choices[] = { wxT("black lines on white background"),
                                 wxT("colors from plots on white background") };
    colors_rb = new wxRadioBox(this, -1, wxT("Colors"),
                               wxDefaultPosition, wxDefaultSize,
                               2, color_choices,
                               1, wxRA_SPECIFY_COLS);
    top_sizer->Add(colors_rb, 0, wxALL|wxEXPAND, 5);
    wxStaticBoxSizer *boxsizer = new wxStaticBoxSizer(wxVERTICAL, this,
                                                      wxT("Optional elements"));
    for (int i = 0; i < 2; ++i) {
        plot_aux_cb[i] = new wxCheckBox(this, -1,
                                wxString::Format(wxT("auxiliary plot %i"), i));
        boxsizer->Add(plot_aux_cb[i], 0, wxLEFT|wxTOP, 5);
    }
    plot_borders_cb = new wxCheckBox(this, -1, wxT("line between plots"));
    boxsizer->Add(plot_borders_cb, 0, wxALL, 5);
    top_sizer->Add(boxsizer, 0, wxALL|wxEXPAND, 5);
    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *print_btn = new wxButton(this, wxID_PRINT);
    print_btn->SetToolTip("Apply & Print");
    button_sizer->Add(print_btn, 0, wxALL, 5);
    button_sizer->AddStretchSpacer();
    wxButton *apply_btn = new wxButton(this, wxID_APPLY);
    apply_btn->SetToolTip("Apply & Close");
    button_sizer->Add(apply_btn, 0, wxALL, 5);
    button_sizer->Add(new wxButton(this, wxID_CANCEL), 0, wxTOP|wxRIGHT, 5);
    top_sizer->Add(button_sizer, 0, wxALL|wxEXPAND, 5);
    SetSizerAndFit(top_sizer);
    bool landscape = pm->print_data.GetOrientation() == wxLANDSCAPE;
    orientation_rb->SetSelection(landscape ? 1 : 0);
    colors_rb->SetSelection(pm->colors ? 1 : 0);
    for (int i = 0; i < 2; ++i) {
        if (pm->plot_pane->aux_visible(i)) {
            plot_aux_cb[i]->SetValue(pm->plot_aux[i]);
        } else {
            plot_aux_cb[i]->SetValue(false);
            plot_aux_cb[i]->Enable(false);
        }
    }
    plot_borders_cb->SetValue(pm->plot_borders);

    left_margin_sc->SetValue(pm->margin_left);
    right_margin_sc->SetValue(pm->margin_right);
    top_margin_sc->SetValue(pm->margin_top);
    bottom_margin_sc->SetValue(pm->margin_bottom);
}

void PageSetupDialog::OnApply(wxCommandEvent& event)
{
    bool landscape = (orientation_rb->GetSelection() == 1);
    pm->print_data.SetOrientation(landscape ? wxLANDSCAPE : wxPORTRAIT);
    pm->colors = colors_rb->GetSelection() == 1;
    for (int i = 0; i < 2; ++i)
        pm->plot_aux[i] = plot_aux_cb[i]->GetValue();
    pm->plot_borders = plot_borders_cb->GetValue();
    pm->margin_left = left_margin_sc->GetValue();
    pm->margin_right = right_margin_sc->GetValue();
    pm->margin_top = top_margin_sc->GetValue();
    pm->margin_bottom = bottom_margin_sc->GetValue();

    EndModal(event.GetId());
}


//===============================================================
//                    Printing utilities
//===============================================================

bool FPrintout::OnPrintPage(int page)
{
    if (page != 1)
        return false;
    wxDC *dc = GetDC();
    if (!dc)
        return false;

    // respect margins
    wxCoord wmm, hmm;
    GetPageSizeMM(&wmm, &hmm);
    wxCoord wp, hp;
    GetPageSizePixels(&wp, &hp);
    float mmToDeviceX = float(wp) / wmm;
    float mmToDeviceY = float(hp) / hmm;
    dc->SetDeviceOrigin(pm_->margin_left * mmToDeviceX,
                        pm_->margin_top * mmToDeviceY);
    dc->SetUserScale(float(wmm - pm_->margin_left - pm_->margin_right) / wmm,
                     float(hmm - pm_->margin_top - pm_->margin_bottom) / hmm);

    const int space = 10; // vertical space between plots (in screen units)



    // `pp' is a list of plots that are to be printed.
    // PlotPane::get_visible_plots() can't be used, because aux plots can be
    // disabled in print setup.
    vector<FPlot*> pp;
    pp.push_back(pm_->plot_pane->get_plot());
    for (int i = 0; i < 2; ++i)
        if (pm_->plot_pane->aux_visible(i) && pm_->plot_aux[i])
            pp.push_back(pm_->plot_pane->get_aux_plot(i));

    // width is the same for all plots
    int W = pm_->plot_pane->GetClientSize().GetWidth();

    // height is a sum of all heights + (N-1)*space
    int H = (pp.size() - 1) * space;
    for (vector<FPlot*>::const_iterator i = pp.begin(); i != pp.end(); ++i)
        H += (*i)->GetClientSize().GetHeight();

    int w, h; // size in DC units
    dc->GetSize(&w, &h);
    double y_scale = (float) h / H;
    // drawing plots, every at different posY
    for (vector<FPlot*>::const_iterator i = pp.begin(); i != pp.end(); ++i) {
        if (pm_->plot_borders && i != pp.begin()) {
            int Y = iround(-0.5 * space * y_scale);
            dc->SetPen(*wxLIGHT_GREY_PEN);
            dc->DrawLine(0, Y, w, Y);
        }
        int plot_height = iround((*i)->GetClientSize().GetHeight() * y_scale);
        dc->SetClippingRegion(0, 0, w, plot_height);
        //dc->SetBrush(*wxGREEN_BRUSH); // for debugging
        //dc->DrawEllipse(0, 0, w, plot_height);  // for debugging
        (*i)->set_magnification(w / W + 1); // +1 for no special reason
        (*i)->draw(*dc, !pm_->colors); // <- 99% of plotting is done here
        (*i)->set_magnification(1);
        dc->DestroyClippingRegion();
        OffsetLogicalOrigin(0, plot_height + space * y_scale);
    }

    return true;
}

//===============================================================

PrintManager::PrintManager(PlotPane* pane)
    :  plot_pane(pane)
{
    read_settings(wxConfig::Get());
}

PrintManager::~PrintManager()
{
    save_settings(wxConfig::Get());
}

void PrintManager::save_settings(wxConfigBase *cf) const
{
    cf->Write(wxT("/print/colors"), colors);
    cf->Write(wxT("/print/plotBorders"), plot_borders);
    for (int i = 0; i < 2; ++i)
        cf->Write(wxString::Format(wxT("/print/plotAux%i"), i), plot_aux[i]);
    bool landscape = print_data.GetOrientation() == wxLANDSCAPE;
    cf->Write(wxT("/print/landscape"), landscape);
}

void PrintManager::read_settings(wxConfigBase *cf)
{
    colors = cfg_read_bool(cf, wxT("/print/colors"), false);
    plot_borders = cfg_read_bool(cf, wxT("/print/plotBorders"), true);
    for (int i = 0; i < 2; ++i)
        plot_aux[i] = cfg_read_bool(cf,
                          wxString::Format(wxT("/print/plotAux%i"), i), true);
#if defined(__WXGTK__) && !wxCHECK_VERSION(2, 9, 5)
    // gtk-printing landscape mode has been fixed in r72646
    bool landscape = false;
#else
    bool landscape = cfg_read_bool(cf, wxT("/print/landscape"), true);
#endif
    print_data.SetOrientation(landscape ? wxLANDSCAPE : wxPORTRAIT);
    margin_left = margin_right = margin_top = margin_bottom = 20;
}

void PrintManager::print()
{
    wxPrintDialogData print_dialog_data(print_data);
    print_dialog_data.SetFromPage(1);
    print_dialog_data.SetToPage(1);
    print_dialog_data.EnablePageNumbers(false);
    print_dialog_data.EnableSelection(false);
    wxPrinter printer (&print_dialog_data);
    FPrintout printout(this);
    bool r = printer.Print(plot_pane, &printout, true);
    if (r) {
        print_data = printer.GetPrintDialogData().GetPrintData();
    } else if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
        wxMessageBox("Printer Error.", "Printing", wxICON_ERROR|wxCANCEL);
}

