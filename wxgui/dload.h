// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__WX_DLOAD__H__
#define FITYK__WX_DLOAD__H__

#include "cmn.h" //ProportionalSplitter, KFTextCtrl, ...

namespace fityk { class Data; }
class XyFileBrowser;

class DLoadDlg : public wxDialog
{
public:
    DLoadDlg(wxWindow* parent, int data_idx, fityk::Data* data,
             const wxString& dir);

private:
    int data_idx_;
    XyFileBrowser* browser_;
    wxButton *open_here_btn_, *open_new_btn_;

    std::string get_command(std::string const& ds, int d_nr);
    void OnOpenHere(wxCommandEvent&) { exec_command(true); }
    void OnOpenNew(wxCommandEvent&) { exec_command(false); }
    void exec_command(bool replace);
};

#endif

