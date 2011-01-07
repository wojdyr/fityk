// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__WX_SDEBUG__H__
#define FITYK__WX_SDEBUG__H__

class ScriptDebugDlg : public wxDialog
{
public:
    ScriptDebugDlg(wxWindow* parent);
    void OnOpenFile(wxCommandEvent&) { open_file(this); }
    void OnSave(wxCommandEvent&);
    void OnSaveAs(wxCommandEvent&);
    void OnExecSelected(wxCommandEvent&) { exec_selected(); }
    void OnStep(wxCommandEvent&);
    void OnClose(wxCommandEvent&) { Close(); }
    void OnCloseDlg(wxCloseEvent&) { Destroy(); }
    void OnTextChange(wxCommandEvent&);
    void open_file(wxWindow *parent);
    void do_open_file(const wxString& path);
    int exec_selected();
    wxString get_list_item(int i);
    void exec_line(int n);
    void save_file(const wxString& save_path);
    const wxString& get_path() const { return path_; }
protected:
    wxToolBar *tb;
    wxTextCtrl *txt;
    wxString dir;
    wxString path_;
    std::string script_dir;

    void set_title();
    DECLARE_EVENT_TABLE()
};

#endif
