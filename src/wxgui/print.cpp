// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

/// In this file:
///  printing related utilities

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <vector>
#include <wx/paper.h>
#include <wx/statline.h>

#include <wx/generic/printps.h>
#include <wx/generic/prntdlgg.h>

#include "print.h"
#include "cmn.h" 
#include "pane.h" 
#include "plot.h" 
#include "mplot.h" 
#include "aplot.h" 

using namespace std;


//======================================================================
//                         PageSetupDialog
//======================================================================
BEGIN_EVENT_TABLE(PageSetupDialog, wxDialog)
    EVT_BUTTON(wxID_OK, PageSetupDialog::OnOk)
END_EVENT_TABLE()

PageSetupDialog::PageSetupDialog(wxWindow *parent, PrintManager *print_mgr)
    : wxDialog(parent, -1, wxT("Page Setup"), 
               wxDefaultPosition, wxDefaultSize, 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
      pm(print_mgr)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    //paper size
    wxStaticBoxSizer *sizerp = new wxStaticBoxSizer(wxVERTICAL, this, 
                                                    wxT("Paper Size"));
    wxArrayString paper_sizes;
    int paper_sel = 0;
    for (size_t i = 0; i < wxThePrintPaperDatabase->GetCount(); ++i) {
        wxPrintPaperType *papertype = wxThePrintPaperDatabase->Item(i);
        paper_sizes.Add(papertype->GetName());
        if (pm->get_print_data().GetPaperId() == papertype->GetId())
            paper_sel = i;
    }
    papers = new  wxComboBox(this, -1, _("Paper Size"), 
                             wxDefaultPosition, wxDefaultSize, 
                             paper_sizes, wxCB_READONLY);
    papers->SetSelection(paper_sel);
    sizerp->Add(papers, 1, wxALL|wxEXPAND, 5);
    top_sizer->Add(sizerp, 0, wxALL|wxEXPAND, 5);

    //orientation
    wxString orient_choices[] = { wxT("Portrait"), wxT("Landscape") };
    orientation = new wxRadioBox(this, -1, wxT("Orientation"),
                                 wxDefaultPosition, wxDefaultSize, 
                                 2, orient_choices, 
                                 2, wxRA_SPECIFY_COLS);
    top_sizer->Add(orientation, 0, wxALL|wxEXPAND, 5);

    wxStaticBox *margbox = new wxStaticBox(this, -1, 
                                           wxT("Margins (millimetres)"));
    wxStaticBoxSizer *hsizer = new wxStaticBoxSizer(margbox, wxHORIZONTAL);
    hsizer->Add(new wxStaticText(this, -1, wxT(" Left")),
                0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    left_margin = new SpinCtrl(this, -1, 0, -100, 500);
    hsizer->Add(left_margin, 0, wxALL, 5);
    hsizer->Add(new wxStaticText(this, -1, wxT(" Right")),
                0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    right_margin = new SpinCtrl(this, -1, 0, -100, 500);
    hsizer->Add(right_margin, 0, wxALL, 5);
    hsizer->Add(new wxStaticText(this, -1, wxT(" Top")),
                0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    top_margin = new SpinCtrl(this, -1, 0, -100, 500);
    hsizer->Add(top_margin, 0, wxALL, 5);
    hsizer->Add(new wxStaticText(this, -1, wxT(" Bottom")),
                0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    bottom_margin = new SpinCtrl(this, -1, 0, -100, 500);
    hsizer->Add(bottom_margin, 0, wxALL, 5);
    top_sizer->Add(hsizer, 0, wxALL|wxEXPAND, 5);

    /*
    wxBoxSizer *h2sizer = new wxBoxSizer(wxHORIZONTAL);
    h2sizer->Add(new wxStaticText(this, -1, wxT("Scale to")),
                 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    scale = new SpinCtrl(this, -1, 100, 1, 150);
    h2sizer->Add(scale, 0, wxTOP|wxBOTTOM, 5);
    h2sizer->Add(new wxStaticText(this, -1, wxT("% of page")),
                 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    top_sizer->Add(h2sizer);
    keep_ratio = new wxCheckBox(this, -1, wxT("keep width to height ratio"));
    top_sizer->Add(keep_ratio, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    */
    wxString color_choices[] = { wxT("black lines on white background"), 
                                 wxT("colors from plots on white background") };
    colors = new wxRadioBox(this, -1, wxT("Colors"),
                            wxDefaultPosition, wxDefaultSize, 
                            2, color_choices, 
                            1, wxRA_SPECIFY_COLS);
    top_sizer->Add(colors, 0, wxALL|wxEXPAND, 5);
    wxStaticBoxSizer *boxsizer = new wxStaticBoxSizer(wxVERTICAL, this, 
                                                      wxT("Optional elements"));
    for (int i = 0; i < 2; ++i) {
        plot_aux[i] = new wxCheckBox(this, -1, 
                                wxString::Format(wxT("auxiliary plot %i"), i));
        boxsizer->Add(plot_aux[i], 0, wxALL, 5);
    }
    plot_borders = new wxCheckBox(this, -1, wxT("borders around plots"));
    boxsizer->Add(plot_borders, 0, wxALL, 5);
    top_sizer->Add(boxsizer, 0, wxALL|wxEXPAND, 5);
    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 
                   0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);
    orientation->SetSelection(pm->landscape ? 1 : 0);
    colors->SetSelection(pm->colors ? 1 : 0);
    //keep_ratio->SetValue(pm->keep_ratio);
    //scale->SetValue(pm->scale);
    for (int i = 0; i < 2; ++i) {
        if (pm->plot_pane->aux_visible(i)) {
            plot_aux[i]->SetValue(pm->plot_aux[i]);
        }
        else {
            plot_aux[i]->SetValue(false);
            plot_aux[i]->Enable(false);
        }
    }
    plot_borders->SetValue(pm->plot_borders);
    
    left_margin->SetValue(pm->get_page_data().GetMarginTopLeft().x);
    right_margin->SetValue(pm->get_page_data().GetMarginBottomRight().x);
    top_margin->SetValue(pm->get_page_data().GetMarginTopLeft().y); 
    bottom_margin->SetValue(pm->get_page_data().GetMarginBottomRight().y);
}

void PageSetupDialog::OnOk(wxCommandEvent&) 
{
    if (papers->GetSelection() != -1) {
        wxPrintPaperType *ppt 
                      = wxThePrintPaperDatabase->Item(papers->GetSelection());
        if (ppt) {
            pm->get_page_data().SetPaperSize(wxSize(ppt->GetWidth()/10, 
                                             ppt->GetHeight()/10));
            pm->get_print_data().SetPaperId(ppt->GetId());
        }
    }

    pm->landscape = (orientation->GetSelection() == 1);
    pm->colors = colors->GetSelection() == 1;
    //pm->keep_ratio = keep_ratio->GetValue();
    //pm->scale = scale->GetValue();
    for (int i = 0; i < 2; ++i) 
        pm->plot_aux[i] = plot_aux[i]->GetValue();
    pm->plot_borders = plot_borders->GetValue();

    wxPoint top_left(left_margin->GetValue(), top_margin->GetValue());
    pm->get_page_data().SetMarginTopLeft(top_left);
    wxPoint bottom_right(right_margin->GetValue(), bottom_margin->GetValue());
    pm->get_page_data().SetMarginBottomRight(bottom_right);

    close_it(this, wxID_OK);
}


//===============================================================
//                    Printing utilities
//===============================================================

void do_print_plots(wxDC *dc, PrintManager const* pm)
{
    // Set the scale and origin
    const int space = 10; //vertical space between plots (in screen units)

    // `vp' is a list of plots that are to be printed.
    // PlotPane::get_visible_plots() can't be used, because aux plots can be
    // disabled in print setup.
    vector<FPlot*> vp; 
    vp.push_back(pm->plot_pane->get_plot());
    for (int i = 0; i < 2; ++i)
        if (pm->plot_pane->aux_visible(i) && pm->plot_aux[i])
            vp.push_back(pm->plot_pane->get_aux_plot(i));

    //width is the same for all plots
    int W = pm->plot_pane->GetClientSize().GetWidth(); 

    // height is a sum of all heights + (N-1)*space
    int H = (vp.size() - 1) * space;  
    for (vector<FPlot*>::const_iterator i = vp.begin(); i != vp.end(); ++i) 
        H += (*i)->GetClientSize().GetHeight();

    int w, h; // size in DC units
    dc->GetSize(&w, &h);
    fp y_scale = 1. * h / H;
    int space_dc = iround(space * y_scale);
    //fp scale = pm->scale/100.;
    //fp scaleX = scale * w / W;
    //fp scaleY = scale * h / H;
    //if (pm->keep_ratio) 
    //    scaleX = scaleY = min(scaleX, scaleY);
    //const int marginX = iround((w - W * scaleX) / 2.);
    //const int marginY = iround((h - H * scaleX) / 2.);

    const int posX = 0;
    int posY = 0; // changed for every plot
//
//    dc->SetUserScale (scaleX, scaleY);

    //drawing all visible plots, every at different posY
    for (vector<FPlot*>::const_iterator i = vp.begin(); i != vp.end(); ++i) {
        dc->SetDeviceOrigin (posX, posY);
        if (pm->plot_borders && i != vp.begin()) {
            int Y = -space_dc / 2;
            dc->DrawLine(0, Y, w, Y);
        }
        int plot_height = iround((*i)->GetClientSize().GetHeight() * y_scale);
        dc->SetClippingRegion(0, 0, w, plot_height);
        (*i)->draw(*dc, !pm->colors); // <- 99% of plotting is done here
        dc->DestroyClippingRegion();
        posY += plot_height + space_dc;
    }
}

//===============================================================

FPrintout::FPrintout(PrintManager const* print_manager) 
    : wxPrintout(wxT("fityk")), pm(print_manager) 
{}

bool FPrintout::OnPrintPage(int page)
{
    if (page != 1) 
        return false;
    wxDC *dc = GetDC();
    if (!dc) 
        return false;
    do_print_plots(dc, pm);
    return true;
}

//===============================================================

class FPreviewFrame : public wxPreviewFrame
{
public:
    FPreviewFrame(wxPrintPreview* preview, wxWindow* parent) 
        : wxPreviewFrame (preview, parent, wxT("Print Preview"), 
                          wxDefaultPosition, wxSize(600, 550)) {}
    void CreateControlBar() { 
        m_controlBar = new wxPreviewControlBar(m_printPreview, 
                                        wxPREVIEW_PRINT|wxPREVIEW_ZOOM, this);
        m_controlBar->CreateButtons();
        m_controlBar->SetZoomControl(110);
    }
};

//===============================================================

PrintManager::PrintManager(PlotPane* pane)
    :  plot_pane(pane), print_data(0), page_setup_data(0) 
{ 
    read_settings(wxConfig::Get());
}

PrintManager::~PrintManager() 
{
    save_settings(wxConfig::Get());
    if (print_data)
        delete print_data;
    if (page_setup_data)
        delete page_setup_data;
}

void PrintManager::save_settings(wxConfigBase *cf) const
{
    cf->Write(wxT("/print/colors"), colors);
    //cf->Write(wxT("/print/scale"), scale);
    //cf->Write(wxT("/print/keepRatio"), keep_ratio);
    cf->Write(wxT("/print/plotBorders"), plot_borders);
    for (int i = 0; i < 2; ++i)
        cf->Write(wxString::Format(wxT("/print/plotAux%i"), i), plot_aux[i]);
    cf->Write(wxT("/print/landscape"), landscape);
}

void PrintManager::read_settings(wxConfigBase *cf)
{
    colors = cfg_read_bool(cf, wxT("/print/colors"), false);
    //scale = cf->Read(wxT("/print/scale"), 100);
    //keep_ratio = cfg_read_bool(cf, wxT("/print/keepRatio"), false);
    plot_borders = cfg_read_bool(cf, wxT("/print/plotBorders"), true);
    for (int i = 0; i < 2; ++i)
        plot_aux[i] = cfg_read_bool(cf, 
                          wxString::Format(wxT("/print/plotAux%i"), i), true);
    landscape = cfg_read_bool(cf, wxT("/print/landscape"), true);
}

wxPrintData& PrintManager::get_print_data() 
{
    if (!print_data) {
        print_data = new wxPrintData;
        print_data->SetOrientation(landscape ? wxLANDSCAPE : wxPORTRAIT);
    }
    return *print_data;
}

wxPageSetupDialogData& PrintManager::get_page_data() 
{
    if (!page_setup_data)
        page_setup_data = new wxPageSetupDialogData;
    return *page_setup_data;
}

void PrintManager::printPreview()
{
    // Pass two printout objects: for preview, and possible printing.
    wxPrintDialogData print_dialog_data(get_print_data());
    wxPrintPreview *preview = new wxPrintPreview (new FPrintout(this),
                                                  new FPrintout(this), 
                                                  &print_dialog_data);
    if (!preview->Ok()) {
        delete preview;
        wxMessageBox(wxT("There was a problem previewing.\n")
                     wxT("Perhaps your current printer is not set correctly?"),
                     wxT("Previewing"), wxOK);
        return;
    }
    FPreviewFrame *preview_frame = new FPreviewFrame (preview, 0);
    preview_frame->Centre(wxBOTH);
    preview_frame->Initialize();
    preview_frame->Show(true);
}

void PrintManager::pageSetup()
{
    PageSetupDialog dlg(0, this);
    dlg.ShowModal();
    /*
     * Old standard wxWidgets "page setup" dlg worked in this way:
    if (!page_setup_data) 
        page_setup_data = new wxPageSetupDialogData(get_print_data());
    else
        (*page_setup_data) = get_print_data();

    wxPageSetupDialog page_setup_dialog(this, page_setup_data);
    page_setup_dialog.ShowModal();

    get_print_data()=page_setup_dialog.GetPageSetupData().GetPrintData();
    (*page_setup_data) = page_setup_dialog.GetPageSetupData();
    */
}

void PrintManager::print()
{
    wxPrintDialogData print_dialog_data(get_print_data());
    print_dialog_data.SetFromPage(1);
    print_dialog_data.SetToPage(1);
    print_dialog_data.EnablePageNumbers(false);
    print_dialog_data.EnableSelection(false);
    wxPrinter printer (&print_dialog_data);
    FPrintout printout(this);
    bool r = printer.Print(plot_pane, &printout, true);
    if (r) {
        get_print_data() = printer.GetPrintDialogData().GetPrintData();
        landscape = (get_print_data().GetOrientation() == wxLANDSCAPE);
    }
    else if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
        wxMessageBox(wxT("There was a problem printing.\nPerhaps your current ")
                     wxT("printer is not set correctly?"), 
                     wxT("Printing"), wxOK);
}


void PrintManager::print_to_psfile()
{
    wxFileDialog dialog(0, wxT("PostScript file"), wxT(""), wxT(""), 
                        wxT("*.ps"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK) 
        return;
    get_print_data().SetPrintMode(wxPRINT_MODE_FILE);
    get_print_data().SetFilename(dialog.GetPath());
    wxPrintDialogData print_dialog_data(get_print_data());
    print_dialog_data.SetFromPage(1);
    print_dialog_data.SetToPage(1);
    wxPostScriptPrinter printer (&print_dialog_data);
    FPrintout printout(this);
    bool r = printer.Print(0, &printout, false);
    if (!r)
        wxMessageBox(wxT("Can't save plots as a file:\n") + dialog.GetPath(),
                     wxT("Exporting to PostScript"), wxOK|wxICON_ERROR);
}


