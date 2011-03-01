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


FitInfoDlg::FitInfoDlg(wxWindow* parent, wxWindowID id)
  : wxDialog(parent, id, wxString(wxT("Fit Info")),
             wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
}

bool FitInfoDlg::Initialize()
{
    string s; // string for the left panel
    try {
        vector<DataAndModel*> dms = frame->get_selected_dms();
        vector<fp> const &pp = ftk->parameters();
        ::Fit *fit = ftk->get_fit();
        int dof = fit->get_dof(dms);
        fp wssr = fit->do_compute_wssr(pp, dms, true);
        wssr_over_dof = wssr / dof;
        fp ssr = fit->do_compute_wssr(pp, dms, false);
        fp r2 = fit->compute_r_squared(pp, dms);
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
             "\nWSSR: " + S(wssr) +
             "\nSSR: " + S(ssr) +
             "\nWSSR/DoF: " + S(wssr_over_dof) +
             "\nR-squared: " + S(r2) + "\n";
    } catch (ExecuteError &e) {
        ftk->warn(string("Error: ") + e.what());
        return false;
    }

    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxSplitterWindow *hsplit = new ProportionalSplitter(this, -1, 0.25);
    left_tc = new wxTextCtrl(hsplit, -1, s2wx(s),
                             wxDefaultPosition, wxDefaultSize,
                             wxTE_MULTILINE|wxTE_RICH|wxTE_READONLY);
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
    hsplit->SplitVertically(left_tc, right_panel);
    top_sizer->Add(hsplit, wxSizerFlags(1).Expand().Border());
    top_sizer->Add(new wxButton(this, wxID_CLOSE),
                   wxSizerFlags().Right().Border());
    SetSizerAndFit(top_sizer);
    SetSize(wxSize(640, 440));

    SetEscapeId(wxID_CLOSE);

    update_right_tc();

    Connect(right_c->GetId(), wxEVT_COMMAND_CHOICE_SELECTED,
            wxCommandEventHandler(FitInfoDlg::OnChoice));

    return true;
}

void FitInfoDlg::update_right_tc()
{
    ::Fit *fit = ftk->get_fit();
    int choice = right_c->GetSelection();
    vector<DataAndModel*> dms = frame->get_selected_dms();
    vector<fp> const &pp = ftk->parameters();
    int na = pp.size();
    wxString s;
    if (choice == 0 || choice == 1) {
        fp scale = (choice == 0 ? 1. : 1. / sqrt(wssr_over_dof));
        vector<fp> errors;
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
                // \u00B1 == +/-
                s += wxString::Format(wxT("\n%20s = %10g \u00B1 "),
                                      name.c_str(), pp[i]);
                if (errors[i] == 0.)
                    s += wxT("??");
                else
                    s += wxString::Format(wxT("%g"), scale * errors[i]);
            }
        }
    }
    else {
        s = wxT("          ");
        vector<fp> alpha;
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
                        fp val = alpha[na*i + j];
                        if (fabs(val) < 1e-99)
                            s += wxT("         0");
                        else
                            s += wxString::Format(wxT(" %9.2e"), val);
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

