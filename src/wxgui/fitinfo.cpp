// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/tooltip.h>

#include "fitinfo.h"
#include "frame.h" //frame
#include "cmn.h" //ProportionalSplitter
#include "../logic.h"
#include "../fit.h"
#include "../data.h"
#include "../var.h"

using namespace std;

NumericFormatPanel::NumericFormatPanel(wxWindow* parent)
    : wxPanel(parent, -1)
{
    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(new wxStaticText(this, -1, wxT("precision:")),
               wxSizerFlags().Center());
    prec_sc = new SpinCtrl(this, -1, 6, 0, 30);
    sizer->Add(prec_sc, wxSizerFlags().Center());
    wxArrayString fmt_choices;
    fmt_choices.Add(wxT("g"));
    fmt_choices.Add(wxT("e"));
    fmt_choices.Add(wxT("E"));
    fmt_choices.Add(wxT("f"));
    fmt_c = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize,
                         fmt_choices);
    fmt_c->SetSelection(0);
    fmt_c->SetToolTip(wxT("g: mixed format\n")
                      wxT("e: 1.234e+02\n")
                      wxT("E: 1.234E+02\n")
                      wxT("f: 123.400"));
    sizer->Add(fmt_c, wxSizerFlags(1).Center());
    SetSizerAndFit(sizer);

    update_format();
    Connect(fmt_c->GetId(), wxEVT_COMMAND_CHOICE_SELECTED,
            wxCommandEventHandler(NumericFormatPanel::OnFormatChanged));
    Connect(prec_sc->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED,
            wxSpinEventHandler(NumericFormatPanel::OnPrecisionSpin));
}

void NumericFormatPanel::update_format()
{
    format_= "%." + S(prec_sc->GetValue()) + REALT_LENGTH_MOD
             + wx2s(fmt_c->GetStringSelection());

    wxCommandEvent event(wxEVT_COMMAND_CHOICE_SELECTED, GetId());
    AddPendingEvent(event);
}


FitInfoDlg::FitInfoDlg(wxWindow* parent, wxWindowID id)
  : wxDialog(parent, id, wxString(wxT("Fit Info")),
             wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
}

bool FitInfoDlg::Initialize()
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxSplitterWindow *hsplit = new ProportionalSplitter(this, -1, 0.25);
    wxPanel *left_panel = new wxPanel(hsplit);
    wxSizer *lsizer = new wxBoxSizer(wxVERTICAL);
    nf = new NumericFormatPanel(left_panel);
    lsizer->Add(nf, wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM));
    left_tc = new wxTextCtrl(left_panel, -1, wxEmptyString,
                             wxDefaultPosition, wxDefaultSize,
                             wxTE_MULTILINE|wxTE_RICH|wxTE_READONLY);
    lsizer->Add(left_tc, wxSizerFlags(1).Expand());
    left_panel->SetSizerAndFit(lsizer);
    wxPanel *right_panel = new wxPanel(hsplit);
    wxSizer *rsizer = new wxBoxSizer(wxVERTICAL);
    wxArrayString choices;
    // \u00B1 == +/-
    choices.Add(wxT("\u00B1 standard errors: sqrt(WSSR/DoF COV_kk)"));
    choices.Add(wxT("\u00B1 sqrt(COV_kk)"));
    choices.Add(wxT("covariance matrix"));
    right_c = new wxChoice(right_panel, -1, wxDefaultPosition, wxDefaultSize,
                           choices);
    right_c->SetSelection(0);
    rsizer->Add(right_c, wxSizerFlags().Expand().Border());
    right_tc = new wxTextCtrl(right_panel, -1, wxT(""),
                              wxDefaultPosition, wxDefaultSize,
                              wxTE_MULTILINE|wxTE_RICH|wxTE_READONLY|
                              wxTE_DONTWRAP);
    int default_font_size = right_tc->GetFont().GetPointSize();
    wxFont font(default_font_size - 1, wxFONTFAMILY_TELETYPE,
                            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    wxTextAttr attr;
    attr.SetFont(font);
    right_tc->SetDefaultStyle(attr);
    rsizer->Add(right_tc, wxSizerFlags(1).Expand());
    right_panel->SetSizerAndFit(rsizer);
    hsplit->SplitVertically(left_panel, right_panel);
    top_sizer->Add(hsplit, wxSizerFlags(1).Expand().Border());
    top_sizer->Add(new wxButton(this, wxID_CLOSE),
                   wxSizerFlags().Right().Border());
    SetSizerAndFit(top_sizer);
    SetSize(wxSize(640, 440));

    SetEscapeId(wxID_CLOSE);

    try {
        update_left_tc();
        update_right_tc();
    } catch (ExecuteError &e) {
        ftk->warn(string("Error: ") + e.what());
        return false;
    }

    // connect both right_c and nf (NumericFormatPanel)
    Connect(-1, wxEVT_COMMAND_CHOICE_SELECTED,
            wxCommandEventHandler(FitInfoDlg::OnChoice));

    return true;
}

void FitInfoDlg::update_left_tc()
{
    string s;
    vector<DataAndModel*> dms = frame->get_selected_dms();
    const vector<realt> &pp = ftk->parameters();
    ::Fit *fit = ftk->get_fit();
    int dof = fit->get_dof(dms);
    double wssr = fit->do_compute_wssr(pp, dms, true);
    wssr_over_dof = wssr / dof;
    double ssr = fit->do_compute_wssr(pp, dms, false);
    double r2 = fit->compute_r_squared(pp, dms);
    int points = 0;
    for (vector<DataAndModel*>::const_iterator i = dms.begin();
                                                    i != dms.end(); ++i)
        points += (*i)->data()->get_n();

    if (dms.size() == 1)
        s = "dataset " + S(frame->get_selected_data_indices()[0])
            + ": " + dms[0]->data()->get_title() + "\n";
    else
        s = S(dms.size()) + " datasets\n";
    s += "points: " + S(points) +
         "\n\nDoF: " + S(dof) +
         "\nWSSR: " + nf->fmt(wssr) +
         "\nSSR: " + nf->fmt(ssr) +
         "\nWSSR/DoF: " + nf->fmt(wssr_over_dof) +
         "\nR-squared: " + nf->fmt(r2) + "\n";
    left_tc->SetValue(s2wx(s));
}


void FitInfoDlg::update_right_tc()
{
    ::Fit *fit = ftk->get_fit();
    int choice = right_c->GetSelection();
    vector<DataAndModel*> dms = frame->get_selected_dms();
    vector<realt> const &pp = ftk->parameters();
    int na = pp.size();
    wxString s;
    if (choice == 0 || choice == 1) {
        double scale = (choice == 0 ? 1. : 1. / sqrt(wssr_over_dof));
        vector<realt> errors;
        try {
            errors = fit->get_standard_errors(dms);
        }
        catch (ExecuteError&) {
            errors.resize(na, 0.);
        }
        for (int i = 0; i < na; ++i) {
            if (fit->is_param_used(i)) {
                const Variable *var = ftk->find_variable_handling_param(i);
                vector<string> in = ftk->get_variable_references(var->name);
                wxString name = wxT("$") + s2wx(var->name);
                if (in.size() == 1 && in[0][0] == '%')
                    name += wxT(" = ") + s2wx(in[0]);
                else if (in.size() == 1)
                    name += wxT(" (in ") + s2wx(in[0]) + wxT(")");
                else
                    name += wxT(" (") + s2wx(S(in.size())) + wxT(" refs)");
                string val = nf->fmt(pp[i]);
                // \u00B1 == +/-
                s += wxString::Format(wxT("\n%20s = %10s \u00B1 "),
                                      name.c_str(), val.c_str());
                if (errors[i] == 0.)
                    s += wxT("??");
                else
                    s += s2wx(nf->fmt(scale * errors[i]));
            }
        }
    }
    else {
        s = wxT("          ");
        vector<realt> alpha;
        try {
            alpha = fit->get_covariance_matrix(dms);
        }
        catch (ExecuteError&) {
            alpha.resize(na*na, 0.);
        }
        for (int i = 0; i < na; ++i)
            if (fit->is_param_used(i)) {
                wxString t=s2wx("$"+ftk->find_variable_handling_param(i)->name);
                s += wxString::Format(wxT("%10s"), t.c_str());
            }
        for (int i = 0; i < na; ++i) {
            if (fit->is_param_used(i)) {
                wxString t=s2wx("$"+ftk->find_variable_handling_param(i)->name);
                s += wxString::Format(wxT("\n%10s"), t.c_str());
                for (int j = 0; j < na; ++j) {
                    if (fit->is_param_used(j)) {
                        double val = alpha[na*i + j];
                        if (fabs(val) < 1e-99)
                            val = 0.;
                        s += wxString::Format(wxT(" %9s"),nf->fmt(val).c_str());
                    }
                }
            }
        }
    }
    // On wxMSW 2.9.0 wxTextCtrl::ChangeValue() ignores default styles
    //right_tc->ChangeValue(s);
    right_tc->Clear();
    right_tc->AppendText(s);
}

