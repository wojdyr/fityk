// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

#include <wx/wx.h>
#include <wx/statline.h>

#include "fitrun.h"
#include "frame.h"
#include "fityk/fit.h"
#include "fityk/logic.h"

using namespace std;
using fityk::FitManager;

BEGIN_EVENT_TABLE(FitRunDlg, wxDialog)
    EVT_SPINCTRL (-1, FitRunDlg::OnSpinEvent)
    EVT_CHOICE (-1, FitRunDlg::OnChangeDsOrMethod)
    EVT_RADIOBOX (-1, FitRunDlg::OnChangeDsOrMethod)
END_EVENT_TABLE()

FitRunDlg::FitRunDlg(wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, wxT("fit functions to data"),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxArrayString data_choices;
    vector<int> sel = frame->get_selected_data_indices();
    string a = S(sel.size()) + " selected dataset";
    if (sel.size() == 1)
        a += ": @" + S(sel[0]);
    else
        a += "s";
    data_choices.Add(s2wx(a));
    data_choices.Add(wxT("all datasets"));
    data_rb = new wxRadioBox(this, -1, wxT("fit..."),
                             wxDefaultPosition, wxDefaultSize,
                             data_choices, 1, wxRA_SPECIFY_COLS);
    separately_cb = new wxCheckBox(this, -1, "fit each dataset separately");
    if (ftk->dk.count() == 1) {
        data_rb->Enable(1, false);
        separately_cb->Enable(false);
    } else {
        bool independent = ftk->are_independent(ftk->dk.datas());
        separately_cb->SetValue(independent);
    }
    top_sizer->Add(data_rb, 0, wxALL|wxEXPAND, 5);
    top_sizer->Add(separately_cb, 0, wxALL|wxEXPAND, 5);
    wxBoxSizer *method_sizer = new wxBoxSizer(wxHORIZONTAL);
    method_sizer->Add(new wxStaticText(this, -1, wxT("method:")),
                      0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    wxArrayString m_choices;
    for (int i = 0; FitManager::method_list[i][0] != NULL; ++i)
        m_choices.Add(FitManager::method_list[i][1]);
    method_c = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize,
                            m_choices);
    int method_nr = ftk->settings_mgr()->get_enum_index("fitting_method");
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
    int default_max_eval = ftk->get_settings()->max_wssr_evaluations;
    maxeval_sc = new SpinCtrl(this, -1, default_max_eval, 0, 999999, 70);
    max_sizer->Add(maxeval_sc, 0, wxALL, 5);
    nomaxeval_st = new wxStaticText(this, -1, wxT("(unlimited)"));
    max_sizer->Add(nomaxeval_st, 0, wxALIGN_CENTER_VERTICAL, 0);
    top_sizer->Add(max_sizer, 0);

    autoplot_cb = new wxCheckBox(this, -1,
                                 wxT("refresh plot after each iteration"));
    autoplot_cb->SetValue(ftk->get_settings()->fit_replot);
    top_sizer->Add(autoplot_cb, 0, wxALL, 5);


    top_sizer->Add(new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 10);
    top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL),
                   0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);
    update_unlimited();
}

void FitRunDlg::update_unlimited()
{
    nomaxeval_st->Show(maxeval_sc->GetValue() == 0);
    nomaxiter_st->Show(maxiter_sc->GetValue() == 0);
}

string FitRunDlg::get_cmd() const
{
    string cmd;
    int sel = method_c->GetSelection();
    string m = ftk->settings_mgr()->get_allowed_values("fitting_method")[sel];
    if (m != ftk->get_settings()->fitting_method)
        cmd += "with fitting_method=" + m + " ";

    if (autoplot_cb->GetValue() != ftk->get_settings()->fit_replot)
        cmd += string(cmd.empty() ? "with" : ",") + " fit_replot="
                + (autoplot_cb->GetValue() ? "1 " : "0 ");

    int max_eval = maxeval_sc->GetValue();
    if (max_eval != ftk->get_settings()->max_wssr_evaluations)
        cmd += string(cmd.empty() ? "with" : ",")
                + " max_wssr_evaluations=" + S(max_eval) + " ";

    cmd += "fit";

    int max_iter = maxiter_sc->GetValue();
    if (max_iter > 0)
        cmd += " " + S(max_iter);

    bool only_selected = (data_rb->GetSelection() == 0);
    string ds = only_selected ? frame->get_datasets() : "@*: ";
    if (separately_cb->GetValue())
        cmd = ds + cmd; // e.g. @*: fit
    else
        cmd += " " + ds.substr(0, ds.size() - 2); // e.g. fit @*
    return cmd;
}


