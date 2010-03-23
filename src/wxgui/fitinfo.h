// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

#ifndef FITYK_WX_FITINFO_H_
#define FITYK_WX_FITINFO_H_

#include <vector>
#include <string>
#include <wx/config.h>

#include "cmn.h" //SpinCtrl


/// Status bar configuration dialog
class FitInfoDlg: public wxDialog
{
public:
    FitInfoDlg(wxWindow* parent, wxWindowID id);
    bool Initialize();
private:
    wxTextCtrl *left_tc, *right_tc;
    wxChoice *right_c;
    double wssr_over_dof;
    void update_right_tc();
    void OnChoice(wxCommandEvent&) { update_right_tc(); }
};

#endif // FITYK_WX_FITINFO_H_
