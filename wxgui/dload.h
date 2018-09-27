// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_DLOAD_H_
#define FITYK_WX_DLOAD_H_

namespace fityk { class Data; }
class XyFileBrowser;

std::string make_lua_load(int data_idx, const wxString& path, int b,
                          int x, int y, int sig,
                          const std::string& fmt, bool comma);

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

