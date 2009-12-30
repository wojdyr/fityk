// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

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
    wxString s; // string for the left panel
    try {
        vector<DataAndModel*> dms = frame->get_selected_dms();
        vector<fp> const &pp = ftk->get_parameters();
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
            s.Printf(wxT("dataset %d: %s\n"),
                     frame->get_selected_data_indices()[0],
                     dms[0]->data()->get_title().c_str());
        else
            s.Printf(wxT("%d datasets\n"), (int) dms.size());
        s += wxString::Format(
                wxT("points: %d\nDoF: %d\nWSSR: %g\nSSR: %g\nWSSR/DoF: %g\n")
                wxT("R-squared: %g\n"),
                points, dof, wssr, ssr, wssr_over_dof, r2);
    } catch (ExecuteError &e) {
        ftk->warn(string("Error: ") + e.what());
        return false;
    }

    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxSplitterWindow *hsplit = new ProportionalSplitter(this, -1, 0.25);
    left_tc = new wxTextCtrl(hsplit, -1, wxT(""),
                             wxDefaultPosition, wxDefaultSize,
                             wxTE_MULTILINE|wxTE_RICH|wxTE_READONLY);
    left_tc->SetValue(s);
    wxPanel *right_panel = new wxPanel(hsplit);
    wxSizer *rsizer = new wxBoxSizer(wxVERTICAL);
    rsizer->Add(new wxStaticText(right_panel, -1, wxT("Symmetric errors")),
                wxSizerFlags().Border().Center());
    scale_cb = new wxCheckBox(right_panel, -1,
                              wxT("multiply errors by sqrt(WSSR/DoF)"));
    rsizer->Add(scale_cb, wxSizerFlags().Border(wxRIGHT).Right());
    right_tc = new wxTextCtrl(right_panel, -1, wxT(""),
                              wxDefaultPosition, wxDefaultSize,
                              wxTE_MULTILINE|wxTE_RICH|wxTE_READONLY);
    wxFont font(10, wxFONTFAMILY_TELETYPE,
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

    Connect(scale_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(FitInfoDlg::OnDivideCheckbox));

    return true;
}

void FitInfoDlg::update_right_tc()
{
    ::Fit *fit = ftk->get_fit();
    fp scale = 1.;
    if (scale_cb->GetValue())
        scale = sqrt(wssr_over_dof);
    vector<DataAndModel*> dms = frame->get_selected_dms();
    vector<fp> alpha = fit->get_covariance_matrix(dms);
    vector<fp> const &pp = ftk->get_parameters();
    wxString s;
    for (size_t i = 0; i < pp.size(); ++i) {
        if (fit->is_param_used(i)) {
            fp err = sqrt(alpha[i*pp.size() + i]);
            string const& xname = ftk->find_variable_handling_param(i)->xname;
            // \u00B1 == +/-
            s += wxString::Format(wxT("\n%10s = %10g \u00B1 "),
                                  xname.c_str(), pp[i]);
            if (err == 0.)
                s += wxT("??");
            else
                s += wxString::Format(wxT("%g"), err * scale);
        }
    }
    // On wxMSW 2.9.0 wxTextCtrl::ChangeValue() ignores default styles
    //right_tc->ChangeValue(s);
    right_tc->Clear();
    right_tc->AppendText(s);
}

