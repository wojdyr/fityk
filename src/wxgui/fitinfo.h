// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_FITINFO_H_
#define FITYK_WX_FITINFO_H_

#include <vector>
#include <string>
#include <wx/config.h>

#include "cmn.h" //SpinCtrl
#include "../common.h" //realt

/// Control for changing numeric precision and format.
/// Sends wxEVT_COMMAND_CHOICE_SELECTED when the precision changes.
class NumericFormatPanel: public wxPanel
{
public:
    NumericFormatPanel(wxWindow* parent);
    std::string fmt(realt d) { return format1<realt, 64>(format_.c_str(), d); }
    const std::string& format() const { return format_; }

private:
    std::string format_;
    wxSpinCtrl *prec_sc;
    wxChoice *fmt_c;

    void update_format();
    void OnPrecisionSpin(wxSpinEvent&) { update_format(); }
    void OnFormatChanged(wxCommandEvent&) { update_format(); }
};

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
    NumericFormatPanel *nf;

    void update_left_tc();
    void update_right_tc();
    void OnChoice(wxCommandEvent&) { update_left_tc(); update_right_tc(); }
    void OnCopy(wxCommandEvent&);
};

#endif // FITYK_WX_FITINFO_H_
