// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_MODELINFO_H_
#define FITYK_WX_MODELINFO_H_

#include <vector>
#include <string>
#include "fitinfo.h" // NumericFormatPanel

/// Status bar configuration dialog
class ModelInfoDlg: public wxDialog
{
public:
    ModelInfoDlg(wxWindow* parent, wxWindowID id);
    bool Initialize();
    const std::string get_info_cmd() const;

private:
    wxTextCtrl *main_tc;
    wxRadioBox *rb;
    wxCheckBox *simplify_cb, *extra_space_cb;
    NumericFormatPanel *nf;

    void update_text();
    void OnRadio(wxCommandEvent&);
    void OnFormatChange(wxCommandEvent&) { update_text(); }
    void OnCopy(wxCommandEvent&);
    void OnSave(wxCommandEvent&);
};

#endif // FITYK_WX_MODELINFO_H_
