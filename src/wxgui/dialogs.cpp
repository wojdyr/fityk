// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

/// In this file:
///  small dialogs:  SumHistoryDlg, FitRunDlg, DataExportDlg, AboutDlg,
///                  MergePointsDlg

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/statline.h>
#include <boost/spirit/version.hpp> //SPIRIT_VERSION for AboutDlg

#include "dialogs.h"
#include "frame.h"
#include "../fit.h"
#include "../ui.h"
#include "../settings.h"
#include "../datatrans.h" 
#include "../logic.h"
#include "../data.h"

#include "img/up_arrow.xpm"
#include "img/down_arrow.xpm"
#include "img/fityk.xpm"

using namespace std;


enum {
    ID_SHIST_LC             = 26100,
    ID_SHIST_UP                    ,
    ID_SHIST_DOWN                  ,
    ID_SHIST_CWSSR                 ,
    ID_SHIST_V                     , // and next 3
    ID_SHIST_V_   = ID_SHIST_V + 10,

    ID_DED_RADIO                   ,
    ID_DED_INACT_CB                ,
    ID_DED_TEXT                   
};


//=====================   data->history dialog  ==================

BEGIN_EVENT_TABLE(SumHistoryDlg, wxDialog)
    EVT_BUTTON      (ID_SHIST_UP,     SumHistoryDlg::OnUpButton)
    EVT_BUTTON      (ID_SHIST_DOWN,   SumHistoryDlg::OnDownButton)
    EVT_BUTTON      (ID_SHIST_CWSSR,  SumHistoryDlg::OnComputeWssrButton)
    EVT_LIST_ITEM_SELECTED  (ID_SHIST_LC, SumHistoryDlg::OnSelectedItem)
    EVT_LIST_ITEM_ACTIVATED (ID_SHIST_LC, SumHistoryDlg::OnActivatedItem)
    EVT_SPINCTRL    (ID_SHIST_V+0,    SumHistoryDlg::OnViewSpinCtrlUpdate)
    EVT_SPINCTRL    (ID_SHIST_V+1,    SumHistoryDlg::OnViewSpinCtrlUpdate)
    EVT_SPINCTRL    (ID_SHIST_V+2,    SumHistoryDlg::OnViewSpinCtrlUpdate)
    EVT_SPINCTRL    (ID_SHIST_V+3,    SumHistoryDlg::OnViewSpinCtrlUpdate)
    EVT_BUTTON      (wxID_CLOSE,      SumHistoryDlg::OnClose)
END_EVENT_TABLE()

SumHistoryDlg::SumHistoryDlg (wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, wxT("Parameters History"), 
               wxDefaultPosition, wxDefaultSize, 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER), 
      lc(0)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    initialize_lc(); //wxListCtrl
    hsizer->Add (lc, 1, wxEXPAND);

    wxBoxSizer *arrows_sizer = new wxBoxSizer(wxVERTICAL);
    up_arrow = new wxBitmapButton (this, ID_SHIST_UP, wxBitmap (up_arrow_xpm));
    arrows_sizer->Add (up_arrow, 0);
    arrows_sizer->Add (10, 10, 1);
    down_arrow = new wxBitmapButton (this, ID_SHIST_DOWN, 
                                     wxBitmap (down_arrow_xpm));
    arrows_sizer->Add (down_arrow, 0);
    hsizer->Add (arrows_sizer, 0, wxALIGN_CENTER);
    top_sizer->Add (hsizer, 1, wxEXPAND);

    wxBoxSizer *buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    compute_wssr_button = new wxButton (this, ID_SHIST_CWSSR, 
                                        wxT("Compute WSSRs"));
    buttons_sizer->Add (compute_wssr_button, 0, wxALL, 5);
    buttons_sizer->Add (10, 10, 1);
    buttons_sizer->Add (new wxStaticText(this, -1, wxT("View parameters:")), 
                        0, wxALL|wxALIGN_CENTER, 5);
    for (int i = 0; i < 4; i++)
        buttons_sizer->Add (new SpinCtrl(this, ID_SHIST_V + i, view[i],
                                         0, view_max, 40),
                            0, wxALL, 5);
    buttons_sizer->Add (10, 10, 1);
    buttons_sizer->Add (new wxButton (this, wxID_CLOSE, wxT("&Close")), 
                        0, wxALL, 5);
    top_sizer->Add (buttons_sizer, 0, wxALIGN_CENTER);

    SetSizer (top_sizer);
    top_sizer->SetSizeHints (this);

    update_selection();
}

void SumHistoryDlg::initialize_lc()
{
    assert (lc == 0);
    view_max = ftk->get_parameters().size() - 1;
    assert (view_max != -1);
    for (int i = 0; i < 4; i++)
        view[i] = min (i, view_max);
    lc = new wxListCtrl (this, ID_SHIST_LC, 
                         wxDefaultPosition, wxSize(450, 250), 
                         wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_HRULES|wxLC_VRULES
                             |wxSIMPLE_BORDER);
    lc->InsertColumn(0, wxT("No."));
    lc->InsertColumn(1, wxT("parameters"));
    lc->InsertColumn(2, wxT("WSSR"));
    for (int i = 0; i < 4; i++)
        lc->InsertColumn(3 + i, wxString::Format(wxT("par. %i"), view[i])); 

    FitMethodsContainer const* fmc = ftk->get_fit_container();
    for (int i = 0; i != fmc->get_param_history_size(); ++i) {
        add_item_to_lc(i, fmc->get_item(i));
    }
    for (int i = 0; i < 3+4; i++)
        lc->SetColumnWidth(i, wxLIST_AUTOSIZE);
}

void SumHistoryDlg::add_item_to_lc(int pos, vector<double> const& item)
{
    lc->InsertItem(pos, wxString::Format(wxT("  %i  "), pos));
    lc->SetItem(pos, 1, wxString::Format(wxT("%i"), (int) item.size()));
    lc->SetItem(pos, 2, wxT("      ?      "));
    for (int j = 0; j < 4; j++) {
        int n = view[j];
        if (n < size(item))
            lc->SetItem(pos, 3 + j, wxString::Format(wxT("%g"), item[n]));
    }
}

void SumHistoryDlg::update_selection()
{
    FitMethodsContainer const* fmc = ftk->get_fit_container();
    int index = fmc->get_active_nr();
    lc->SetItemState (index, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, 
                             wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
    up_arrow->Enable (index != 0);
    down_arrow->Enable (index != fmc->get_param_history_size() - 1);
}

void SumHistoryDlg::OnUpButton (wxCommandEvent&)
{
    ftk->exec("fit undo");

    // undo can cause adding new item to the history
    FitMethodsContainer const* fmc = ftk->get_fit_container();
    int ps = fmc->get_param_history_size();
    if (lc->GetItemCount() == ps - 1)
        add_item_to_lc(ps - 1, fmc->get_item(ps - 1));
    assert(lc->GetItemCount() == ps);

    update_selection();
}

void SumHistoryDlg::OnDownButton (wxCommandEvent&)
{
    ftk->exec("fit redo");
    update_selection();
}

void SumHistoryDlg::OnComputeWssrButton (wxCommandEvent&)
{
    FitMethodsContainer const* fmc = ftk->get_fit_container();
    vector<double> const orig = ftk->get_parameters();
    vector<DataWithSum*> dsds = frame->get_selected_ds();

    for (int i = 0; i != fmc->get_param_history_size(); ++i) {
        vector<double> const& item = fmc->get_item(i);
        if (item.size() == orig.size()) {
            double wssr = ftk->get_fit()->do_compute_wssr(item, dsds, true);
            lc->SetItem(i, 2, wxString::Format(wxT("%g"), wssr));
        }
    }
    lc->SetColumnWidth(2, wxLIST_AUTOSIZE);
}

void SumHistoryDlg::OnSelectedItem (wxListEvent&)
{
    update_selection();
}

void SumHistoryDlg::OnActivatedItem (wxListEvent& event)
{
    int n = event.GetIndex();
    ftk->exec("fit history " + S(n));
    update_selection();
}

void SumHistoryDlg::OnViewSpinCtrlUpdate (wxSpinEvent& event) 
{
    int v = event.GetId() - ID_SHIST_V;
    assert (0 <= v && v < 4);
    int n = event.GetPosition();
    assert (0 <= n && n <= view_max);
    view[v] = n;
    //update header in wxListCtrl
    wxListItem li;
    li.SetMask (wxLIST_MASK_TEXT);
    li.SetText(wxString::Format(wxT("par. %i"), n));
    lc->SetColumn(3 + v, li); 
    //update data in wxListCtrl
    FitMethodsContainer const* fmc = ftk->get_fit_container();
    for (int i = 0; i != fmc->get_param_history_size(); ++i) {
        vector<double> const& item = fmc->get_item(i);
        wxString s = n < size(item) ? wxString::Format(wxT("%g"), item[n]) 
                                    : wxString();
        lc->SetItem(i, 3 + v, s);
    }
}


//=====================    fit->run  dialog    ==================

BEGIN_EVENT_TABLE(FitRunDlg, wxDialog)
    EVT_BUTTON (wxID_OK, FitRunDlg::OnOK)
    EVT_SPINCTRL (-1, FitRunDlg::OnSpinEvent)
    EVT_CHOICE (-1, FitRunDlg::OnChangeDsOrMethod)
    EVT_RADIOBOX (-1, FitRunDlg::OnChangeDsOrMethod)
END_EVENT_TABLE()

FitRunDlg::FitRunDlg(wxWindow* parent, wxWindowID id, bool initialize)
    : wxDialog(parent, id, wxT("fit functions to data"),
               wxDefaultPosition, wxDefaultSize, 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER) 
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxArrayString ds_choices;
    sel = frame->get_selected_ds_indices();
    string a = S(sel.size()) + " selected dataset";
    if (sel.size() == 1)
        a += ": @" + S(sel[0]);
    else
        a += "s";
    ds_choices.Add(s2wx(a)); 
    ds_choices.Add(wxT("all datasets"));
    ds_rb = new wxRadioBox(this, -1, wxT("fit..."), 
                           wxDefaultPosition, wxDefaultSize,
                           ds_choices, 1, wxRA_SPECIFY_COLS);
    if (ftk->get_ds_count() == 1)
        ds_rb->Enable(1, false);
    top_sizer->Add(ds_rb, 0, wxALL|wxEXPAND, 5);
    wxBoxSizer *method_sizer = new wxBoxSizer(wxHORIZONTAL);
    method_sizer->Add(new wxStaticText(this, -1, wxT("method:")), 
                      0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    wxArrayString m_choices;
    m_choices.Add(wxT("Levenberg-Marquardt")); 
    m_choices.Add(wxT("Nelder-Mead simplex"));
    m_choices.Add(wxT("Genetic Algorithm")); 
    method_c = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize,
                            m_choices);
    int method_nr = ftk->get_settings()->get_e("fitting-method");
    method_c->SetSelection(method_nr);
    method_sizer->Add(method_c, 0, wxALL, 5);
    top_sizer->Add(method_sizer, 0);

    wxFlexGridSizer *max_sizer = new wxFlexGridSizer(2, 3, 0, 0);
    max_sizer->Add(new wxStaticText(this, -1, wxT("max. iterations")),
                   0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5);
    maxiter_sc = new SpinCtrl(this, -1, 0, 0, 999999, 70);
    max_sizer->Add(maxiter_sc, 0, wxALL, 5);
    nomaxiter_st = new wxStaticText(this, -1, wxT("(unlimited)"));
    max_sizer->Add(nomaxiter_st, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
    max_sizer->Add(new wxStaticText(this, -1, wxT("max. WSSR evaluations")),
                   0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5);
    int default_max_eval = ftk->get_settings()->get_i("max-wssr-evaluations"); 
    maxeval_sc = new SpinCtrl(this, -1, default_max_eval, 0, 999999, 70);
    max_sizer->Add(maxeval_sc, 0, wxALL, 5);
    nomaxeval_st = new wxStaticText(this, -1, wxT("(unlimited)"));
    max_sizer->Add(nomaxeval_st, 0, wxALIGN_CENTER_VERTICAL, 0);
    top_sizer->Add(max_sizer, 0);
    
    initialize_cb = new wxCheckBox(this, -1, wxT("initialize method"));
    initialize_cb->SetValue(initialize);
    top_sizer->Add(initialize_cb, 0, wxALL, 5);

    bool autop = (ftk->get_settings()->getp("autoplot") == "on-fit-iteration");
    autoplot_cb = new wxCheckBox(this, -1, 
                                 wxT("refresh plot after each iteration"));
    autoplot_cb->SetValue(autop); 
    top_sizer->Add(autoplot_cb, 0, wxALL, 5);


    top_sizer->Add(new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 10);
    top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 
                   0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);
    update_allow_continue();
    update_unlimited();
}

void FitRunDlg::update_allow_continue()
{
    initialize_cb->SetValue(true);
    bool is_initialized;
    int m_sel = method_c->GetSelection();
    //use "::Fit", because Fit is also a method of wxDialog
    ::Fit const* f = ftk->get_fit_container()->get_method(m_sel);
    if (ds_rb->GetSelection() == 0) {
        vector<DataWithSum*> dsds(sel.size());
        for (size_t i = 0; i < sel.size(); ++i)
            dsds[i] = ftk->get_ds(sel[i]);
        is_initialized = f->is_initialized(dsds);
    }
    else {
        is_initialized = f->is_initialized(ftk->get_dsds());
    }
    initialize_cb->Enable(is_initialized);
}

void FitRunDlg::update_unlimited()
{
    nomaxeval_st->Show(maxeval_sc->GetValue() == 0);
    nomaxiter_st->Show(maxiter_sc->GetValue() == 0);
}

void FitRunDlg::OnOK(wxCommandEvent&)
{
    string cmd;
    int m = method_c->GetSelection();
    if (m != ftk->get_settings()->get_e("fitting-method"))
        cmd += "with fitting-method=" 
            + ftk->get_fit_container()->get_method(m)->name + " ";

    bool autop = (ftk->get_settings()->getp("autoplot") == "on-fit-iteration");
    if (autoplot_cb->GetValue() != autop) {
        cmd += string(cmd.empty() ? "with" : ",") + " autoplot=";
        cmd += (autoplot_cb->GetValue() ? "on-fit-iteration " 
                                        : "on-plot-change ");
    }

    int max_eval = maxeval_sc->GetValue();
    if (max_eval != ftk->get_settings()->get_i("max-wssr-evaluations")) 
        cmd += (cmd.empty() ? "with" : ",") 
                + string(" max-wssr-evaluations=") + S(max_eval) + " ";

    bool ini = initialize_cb->GetValue();
    cmd +=  ini ? "fit " : "fit+ ";

    int max_iter = maxiter_sc->GetValue();
    if (max_iter > 0)
        cmd += S(max_iter);

    if (ini) {
        if (ds_rb->GetSelection() == 0)
            cmd += frame->get_in_datasets();
        else
            cmd += " in @*";
    }

    Show(false);
    ftk->exec(cmd);
    close_it(this, wxID_OK);
}



/// show "Export data" dialog
bool export_data_dlg(wxWindow *parent, bool load_exported)
{
    static wxString dir = wxConfig::Get()->Read(wxT("/exportDir"));
    string columns = "";
    vector<int> sel = frame->get_selected_ds_indices();
    if (sel.size() != 1)
        return false;
    string ds = "@" + S(sel[0]);
    if (!load_exported) {
        DataExportDlg ded(parent, -1, ds);
        if (ded.ShowModal() != wxID_OK)
            return false;
        columns = " (" + ded.get_columns() + ")";
    }
    wxFileDialog fdlg (parent, wxT("Export data to file"), dir, wxT(""),
                       wxT("x y data (*.dat, *.xy)|*.dat;*.DAT;*.xy;*.XY"),
                       wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    dir = fdlg.GetDirectory();
    if (fdlg.ShowModal() == wxID_OK) {
        string path = wx2s(fdlg.GetPath());
        ftk->exec(ds + columns + " > '" + path + "'");
        if (load_exported)
            ftk->exec(ds + " < '" + path + "'");
        return true;
    }
    else
        return false;
}
//======================================================================
//                         DataExportDlg
//======================================================================
BEGIN_EVENT_TABLE(DataExportDlg, wxDialog)
    EVT_BUTTON(wxID_OK, DataExportDlg::OnOk)
    EVT_RADIOBOX(ID_DED_RADIO, DataExportDlg::OnRadioChanged)
    EVT_CHECKBOX(ID_DED_INACT_CB, DataExportDlg::OnInactiveChanged)
    EVT_TEXT(ID_DED_TEXT, DataExportDlg::OnTextChanged)
END_EVENT_TABLE()

DataExportDlg::DataExportDlg(wxWindow* parent, wxWindowID id, 
                             std::string const& ds)
    : wxDialog(parent, id, wxT("Export data/functions as points"), 
               wxDefaultPosition, wxSize(600, 500), 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *st1 = new wxStaticText(this, -1, 
                                         wxT("       Step 1: Select columns"));
    top_sizer->Add(st1, 0, wxTOP|wxLEFT|wxRIGHT, 5);
    wxStaticText *st2 = new wxStaticText(this, -1, 
                                         wxT("       Step 2: Choose a file"));
    st2->Enable(false);
    top_sizer->Add(st2, 0, wxALL, 5);
    wxArrayString choices;
    choices.Add(wxT("x, y"));
    cv.Add(wxT("x, y"));
    choices.Add(wxT("x, y, sigma"));
    cv.Add(wxT("x, y, s"));
    choices.Add(wxT("x, sum"));
    cv.Add(wxT("x, ") + s2wx(ds) + wxT(".F(x)"));
    choices.Add(wxT("x, sum, zero shift"));
    cv.Add(wxT("x, ") + s2wx(ds) + wxT(".F(x), ") + s2wx(ds) + wxT(".Z(x)"));
    choices.Add(wxT("x, y, sigma, sum"));
    cv.Add(wxT("x, y, s, ") + s2wx(ds) + wxT(".F(x)"));
    choices.Add(wxT("x, y, sigma, sum, all functions..."));
    //TODO:
    //vector<string> const& ff_names = AL->get_sum(tmp_int)->get_ff_names();
    cv.Add(wxT("x, y, s, ") + s2wx(ds) + wxT(".F(x), *F(x)"));

    choices.Add(wxT("x, y, sigma, sum, residual"));
    cv.Add(wxT("x, y, s, ") + s2wx(ds) + wxT(".F(x), y-") + s2wx(ds) 
            + wxT(".F(x)"));
    choices.Add(wxT("x, y, sigma, weighted residual"));
    cv.Add(wxT("x, y, s, (y-") + s2wx(ds) + wxT(".F(x))/s"));
    choices.Add(wxT("custom"));
    rb = new wxRadioBox(this, ID_DED_RADIO, wxT("exported columns"),
                        wxDefaultPosition, wxDefaultSize, choices,
                        2, wxRA_SPECIFY_COLS);
    top_sizer->Add(rb, 0, wxALL|wxEXPAND, 5);
    inactive_cb = new wxCheckBox(this, ID_DED_INACT_CB, 
                                 wxT("export also inactive points"));
    top_sizer->Add(inactive_cb, 0, wxALL, 5);
    text = new wxTextCtrl(this, ID_DED_TEXT, wxT(""));
    top_sizer->Add(text, 0, wxEXPAND|wxALL, 5);
    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 
                   0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);

    //read settings
    wxString t = wxConfig::Get()->Read(wxT("/exportDataCols"), wxT("x, y, s"));
    for (size_t i = 0; i < cv.GetCount(); ++i) {
        if (t == cv[i]) {
            rb->SetSelection(i);
            inactive_cb->SetValue(false);
            on_widget_change();
            return;
        }
        else if (t == cv[i]+wxT(", a")) {
            rb->SetSelection(i);
            inactive_cb->SetValue(true);
            on_widget_change();
            return;
        }
    }
    rb->SetSelection(cv.GetCount());
    text->SetValue(t);
    text->MarkDirty();
}

void DataExportDlg::on_widget_change()
{
    int n = rb->GetSelection();
    bool is_custom = (n == (int) cv.GetCount());
    if (!is_custom) {
        text->SetValue(cv[n] + (inactive_cb->GetValue() ? wxT(", a"):wxT("")));
        FindWindow(wxID_OK)->Enable(true);
    }
    text->Enable(is_custom);
    inactive_cb->Enable(!is_custom);
}

void DataExportDlg::OnTextChanged(wxCommandEvent&) 
{ 
    if (!text->IsModified())
        return;
    vector<string> cols = split_string(wx2s(text->GetValue()), ",");
    bool has_a = false;
    bool parsable = true;
    for (vector<string>::const_iterator i = cols.begin(); i != cols.end(); ++i){
        string t = strip_string(*i);
        if (t == "a")
            has_a = true;

        if (startswith(t, "*F(") && *(t.end()-1) == ')') 
            t = t.substr(3, t.size()-4);
        if (!compile_data_expression(t))
            parsable = false;
    }
    FindWindow(wxID_OK)->Enable(parsable);
    inactive_cb->SetValue(has_a);
}

void DataExportDlg::OnOk(wxCommandEvent& event) 
{
    wxConfig::Get()->Write(wxT("/exportDataCols"), text->GetValue());
    event.Skip();
}


//===============================================================
//                         AboutDlg   
//===============================================================

BEGIN_EVENT_TABLE (AboutDlg, wxDialog)
    EVT_TEXT_URL (wxID_ANY, AboutDlg::OnTextURL)
END_EVENT_TABLE()

AboutDlg::AboutDlg(wxWindow* parent)
    : wxDialog(parent, -1, wxT("About Fityk"), wxDefaultPosition, 
               wxSize(350,400), wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticBitmap(this, -1, wxBitmap(fityk_xpm)),
               0, wxALIGN_CENTER|wxALL, 5);
    wxStaticText *name = new wxStaticText(this, -1, 
                                          wxT("fityk ") + pchar2wx(VERSION));
    name->SetFont(wxFont(18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                         wxFONTWEIGHT_BOLD));
    sizer->Add(name, 0, wxALIGN_CENTER|wxALL, 5); 
    txt = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxSize(350,250), 
                         wxTE_MULTILINE|wxTE_RICH2|wxNO_BORDER
                             |wxTE_READONLY|wxTE_AUTO_URL);
    txt->SetBackgroundColour(GetBackgroundColour());
    txt->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour, wxNullFont,
                                    wxTEXT_ALIGNMENT_CENTRE));
    
    txt->AppendText(wxT("A curve fitting and data analysis program\n\n"));
    txt->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour, 
                                    *wxITALIC_FONT));
    txt->AppendText(wxT("powered by ") wxVERSION_STRING wxT("\n"));
    txt->AppendText(wxString::Format(wxT("powered by Boost.Spirit %d.%d.%d\n"), 
                                       SPIRIT_VERSION / 0x1000,
                                       SPIRIT_VERSION % 0x1000 / 0x0100,
                                       SPIRIT_VERSION % 0x0100));
    txt->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour, 
                                    *wxNORMAL_FONT));
    txt->AppendText(wxT("\nCopyright (C) 2001 - 2008 Marcin Wojdyr\n\n"));
    txt->SetDefaultStyle(wxTextAttr(*wxBLUE));
    txt->AppendText(wxT("http://www.unipress.waw.pl/fityk/\n\n"));
    txt->SetDefaultStyle(wxTextAttr(*wxBLACK));
    txt->AppendText(wxT("This program is free software; ")
      wxT("you can redistribute it ")
      wxT("and/or modify it under the terms of the GNU General Public ")
      wxT("License, version 2, as published by the Free Software Foundation"));
    sizer->Add (txt, 1, wxALL|wxEXPAND|wxFIXED_MINSIZE, 5);
    //sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 10);
    wxButton *bu_ok = new wxButton (this, wxID_OK, wxT("OK"));
    bu_ok->SetDefault();
    sizer->Add (bu_ok, 0, wxALL|wxEXPAND, 10);
    SetSizerAndFit(sizer);
}

void AboutDlg::OnTextURL(wxTextUrlEvent& event) 
{
    if (!event.GetMouseEvent().LeftDown()) {
        event.Skip();
        return;
    }
    long start = event.GetURLStart(),
         end = event.GetURLEnd();
    wxString url = txt->GetValue().Mid(start, end - start);
    wxLaunchDefaultBrowser(url);
}


//===============================================================
//                         MergePointsDlg
//===============================================================

BEGIN_EVENT_TABLE (MergePointsDlg, wxDialog)
    EVT_CHECKBOX(-1, MergePointsDlg::OnCheckBox)
END_EVENT_TABLE()

MergePointsDlg::MergePointsDlg(wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, wxT("Merge data points"),
               wxDefaultPosition, wxDefaultSize, 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER) 
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    inf = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize,
                         wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE);
    update_info();
    top_sizer->Add(inf, 1, wxEXPAND|wxALL, 1);
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL); 
    dx_cb = new wxCheckBox(this, -1, wxT("merge points with |x1-x2|<"));
    dx_cb->SetValue(true);
    hsizer->Add(dx_cb, 0, wxALIGN_CENTER_VERTICAL);
    dx_val = new RealNumberCtrl(this, -1, ftk->get_settings()->getp("epsilon"));
    hsizer->Add(dx_val, 0);
    top_sizer->Add(hsizer, 0, wxALL, 5);
    y_rb = new wxRadioBox(this, -1, wxT("set y as ..."), 
                          wxDefaultPosition, wxDefaultSize, 
                          make_wxArrStr(wxT("sum"), wxT("avg")),
                          1, wxRA_SPECIFY_ROWS);
    top_sizer->Add(y_rb, 0, wxEXPAND|wxALL, 5);
    focused_data = frame->get_focused_ds_index();
    wxString this_ds = wxString::Format(wxT("dataset @%d"), focused_data);
    output_rb = new wxRadioBox(this, -1, wxT("output to ..."), 
                               wxDefaultPosition, wxDefaultSize, 
                               make_wxArrStr(this_ds, wxT("new dataset")),
                               1, wxRA_SPECIFY_ROWS);
    top_sizer->Add(output_rb, 0, wxEXPAND|wxALL, 5);
    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 
                   0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);
}

void MergePointsDlg::update_info()
{
    vector<int> dd = frame->get_selected_ds_indices();
    Data const* data = ftk->get_data(dd[0]);
    fp x_min = data->get_x_min();
    fp x_max = data->get_x_max();
    int n = data->points().size();
    wxString dstr = wxString::Format(wxT("@%d"), dd[0]);
    for (size_t i = 1; i < dd.size(); ++i) {
        data = ftk->get_data(i);
        if (data->get_x_min() < x_min)
            x_min = data->get_x_min();
        if (data->get_x_max() > x_max)
            x_max = data->get_x_max();
        n += data->points().size();
        dstr += wxString::Format(wxT(" @%d"), (int) i);
    }
    wxString s = wxString::Format(wxT("%i data points from: "), n) + dstr;
    s += wxString::Format(wxT("\nx in range (%g, %g)"), x_min, x_max);
    if (dd.size() != 1 && data->get_x_step() != 0.)
        s += wxString::Format(wxT("\nfixed step: %g"), data->get_x_step());
    else
        s += wxString::Format(wxT("\naverage step: %g"), (x_max-x_min) / (n-1));
    inf->SetValue(s);
}

string MergePointsDlg::get_command()
{
    string s;
    if (dx_cb->GetValue())
        s += "with epsilon=" + wx2s(dx_val->GetValue()) + " ";
    string dat = output_rb->GetSelection() == 0 ? S(focused_data) : S("+");
    s += "@" + dat + " = ";
    if (dx_cb->GetValue())
        s += y_rb->GetSelection() == 0 ? "sum_same_x " : "avg_same_x ";
    s += "@" + join_vector(frame->get_selected_ds_indices(), " + @");
    return s;
}


