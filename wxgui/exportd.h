// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_EXPORTD_H_
#define FITYK_WX_EXPORTD_H_

#include <string>
#include <vector>

void export_data_dlg(const std::vector<int>& sel, wxWindow *parent,
                     wxString *export_dir);
void export_peak_parameters(const std::vector<int>& sel, wxWindow *parent,
                            wxString *export_dir);

// helper used also in frame.cpp
void exec_redirected_command(const std::vector<int>& sel,
                             const std::string& cmd, const wxString& path);

class ExtraCheckBox: public wxPanel
{
public:
    ExtraCheckBox(wxWindow* parent, const wxString& label, bool value);
    bool is_checked() const { return cb->GetValue(); }
private:
    wxCheckBox *cb;
};


#endif
