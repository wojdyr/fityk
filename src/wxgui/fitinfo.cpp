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
    wxString r; // string for the right panel
    try {
        vector<int> sel = frame->get_selected_data_indices();
        vector<fp> const &pp = ftk->get_parameters();
        ::Fit *fit = ftk->get_fit();
        vector<DataAndModel*> dms(sel.size());
        for (size_t i = 0; i < sel.size(); ++i)
            dms[i] = ftk->get_dm(sel[i]);
        int dof = fit->get_dof(dms);
        fp wssr = fit->do_compute_wssr(pp, dms, true);
        fp ssr = fit->do_compute_wssr(pp, dms, false);
        fp r2 = fit->compute_r_squared(pp, dms);
        int points = 0;
        for (vector<DataAndModel*>::const_iterator i = dms.begin();
                                                        i != dms.end(); ++i)
            points += (*i)->data()->get_n();

        if (sel.size() == 1)
            s.Printf(wxT("dataset %d: %s\n"), sel[0],
                                  ftk->get_data(sel[0])->get_title().c_str());
        else
            s.Printf(wxT("%d datasets\n"), (int) sel.size());
        s += wxString::Format(
                wxT("points: %d\nDoF: %d\nWSSR: %g\nSSR: %g\nWSSR/DoF: %g\n")
                wxT("R-squared: %g\n"),
                points, dof, wssr, ssr, wssr/dof, r2);
        r = s2wx(fit->get_error_info(dms, false));
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
    right_tc = new wxTextCtrl(hsplit, -1, wxT(""),
                              wxDefaultPosition, wxDefaultSize,
                              wxTE_MULTILINE|wxTE_RICH|wxTE_READONLY);
    right_tc->SetValue(r);
    hsplit->SplitVertically(left_tc, right_tc);
    top_sizer->Add(hsplit, wxSizerFlags(1).Expand().Border());
    top_sizer->Add(new wxButton(this, wxID_CLOSE),
                   wxSizerFlags().Right().Border());
    SetSizerAndFit(top_sizer);
    SetSize(wxSize(640, 440));

    SetEscapeId(wxID_CLOSE);

    return true;
}


