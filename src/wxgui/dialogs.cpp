// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
///  small dialogs:  FitRunDlg, MergePointsDlg

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/statline.h>

#include "dialogs.h"
#include "frame.h"
#include "../ui.h"
#include "../fit.h"
#include "../settings.h"
#include "../logic.h"
#include "../data.h"

using namespace std;


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
    wxArrayString data_choices;
    sel = frame->get_selected_data_indices();
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
    if (ftk->get_dm_count() == 1)
        data_rb->Enable(1, false);
    top_sizer->Add(data_rb, 0, wxALL|wxEXPAND, 5);
    wxBoxSizer *method_sizer = new wxBoxSizer(wxHORIZONTAL);
    method_sizer->Add(new wxStaticText(this, -1, wxT("method:")),
                      0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    wxArrayString m_choices;
    m_choices.Add(wxT("Levenberg-Marquardt"));
    m_choices.Add(wxT("Nelder-Mead simplex"));
    m_choices.Add(wxT("Genetic Algorithm"));
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

    initialize_cb = new wxCheckBox(this, -1, wxT("initialize method"));
    initialize_cb->SetValue(initialize);
    top_sizer->Add(initialize_cb, 0, wxALL, 5);

    autoplot_cb = new wxCheckBox(this, -1,
                                 wxT("refresh plot after each iteration"));
    autoplot_cb->SetValue(ftk->get_settings()->fit_replot);
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
    if (data_rb->GetSelection() == 0) {
        vector<DataAndModel*> dms(sel.size());
        for (size_t i = 0; i < sel.size(); ++i)
            dms[i] = ftk->get_dm(sel[i]);
        is_initialized = f->is_initialized(dms);
    }
    else {
        is_initialized = f->is_initialized(ftk->get_dms());
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

    bool ini = initialize_cb->GetValue();
    cmd +=  ini ? "fit " : "fit + ";

    int max_iter = maxiter_sc->GetValue();
    if (max_iter > 0)
        cmd += S(max_iter);

    if (ini) {
        if (data_rb->GetSelection() == 0) {
            string ds = frame->get_datasets();
            if (!ds.empty())
                // do not append ": "
                cmd += ds.substr(0, ds.size() - 2);
        }
        else
            cmd += " @*";
    }

    Show(false);
    ftk->exec(cmd);
    EndModal(wxID_OK);
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
                         wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE|wxNO_BORDER);
#ifdef __WXGTK__
    wxColour bg_col = wxStaticText::GetClassDefaultAttributes().colBg;
#else
    wxColour bg_col = GetBackgroundColour();
#endif
    inf->SetBackgroundColour(bg_col);
    inf->SetDefaultStyle(wxTextAttr(wxNullColour, bg_col));
    update_info();
    top_sizer->Add(inf, 1, wxEXPAND|wxTOP|wxLEFT|wxRIGHT, 10);
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    dx_cb = new wxCheckBox(this, -1, wxT("merge points with |x1-x2|<"));
    dx_cb->SetValue(true);
    hsizer->Add(dx_cb, 0, wxALIGN_CENTER_VERTICAL);
    dx_val = new RealNumberCtrl(this, -1, ftk->get_settings()->epsilon);
    hsizer->Add(dx_val, 0);
    top_sizer->Add(hsizer, 0, wxALL, 5);
    y_rb = new wxRadioBox(this, -1, wxT("y = "),
                          wxDefaultPosition, wxDefaultSize,
                          ArrayString(wxT("sum"), wxT("avg")),
                          1, wxRA_SPECIFY_ROWS);
    top_sizer->Add(y_rb, 0, wxEXPAND|wxALL, 5);
    focused_data = frame->get_focused_data_index();
    wxString fdstr = wxString::Format(wxT("dataset @%d"), focused_data);
    output_rb = new wxRadioBox(this, -1, wxT("output to ..."),
                               wxDefaultPosition, wxDefaultSize,
                               ArrayString(fdstr, wxT("new dataset")),
                               1, wxRA_SPECIFY_ROWS);
    top_sizer->Add(output_rb, 0, wxEXPAND|wxALL, 5);
    top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL),
                   0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);
}

void MergePointsDlg::update_info()
{
    vector<int> dd = frame->get_selected_data_indices();
    const Data* data = ftk->get_data(dd[0]);
    double x_min = data->get_x_min();
    double x_max = data->get_x_max();
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
    if (dx_cb->GetValue()) {
        string eps = wx2s(dx_val->GetValue().Trim());
        if (eps != eS(ftk->get_settings()->epsilon))
            s += "with epsilon=" + eps + " ";
    }
    string dat = output_rb->GetSelection() == 0 ? S(focused_data) : S("+");
    s += "@" + dat + " = ";
    if (dx_cb->GetValue())
        s += y_rb->GetSelection() == 0 ? "sum_same_x " : "avg_same_x ";
    s += "@" + join_vector(frame->get_selected_data_indices(), " + @");
    return s;
}



TextComboDlg::TextComboDlg(wxWindow *parent, const wxString& message,
                           const wxString& caption)
    : wxDialog(parent, -1, caption)
{
    wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(CreateTextSizer(message), wxSizerFlags().DoubleBorder());
    combo = new wxComboBox(this, -1);
    topsizer->Add(combo, wxSizerFlags().Expand().TripleBorder(wxLEFT|wxRIGHT));
    wxSizer *buttonSizer = CreateSeparatedButtonSizer(wxOK|wxCANCEL);
    if (buttonSizer)
        topsizer->Add(buttonSizer, wxSizerFlags().DoubleBorder().Expand());
    SetSizerAndFit(topsizer);
    combo->SetFocus();
}

