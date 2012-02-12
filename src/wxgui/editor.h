// This file is part of fityk program. Copyright Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_EDITOR_H_
#define FITYK_WX_EDITOR_H_

class Editor;

class EditorDlg : public wxDialog
{
public:
    EditorDlg(wxWindow* parent);
    void open_file(const wxString& path);
    void new_file_with_content(const wxString& content);

private:
    wxToolBarBase *tb_;
    Editor *ed_;
    wxString path_;

    int exec_selected();
    void exec_line(int n);
    void save_file(const wxString& save_path);
    void update_title();

    void OnSave(wxCommandEvent&);
    void OnSaveAs(wxCommandEvent&);
    void OnExecSelected(wxCommandEvent&) { exec_selected(); }
    void OnStep(wxCommandEvent&);
    void OnButtonClose(wxCommandEvent&) { Close(); }
    void OnCloseDlg(wxCloseEvent&);
    void OnTextChange(wxCommandEvent&);
    DECLARE_EVENT_TABLE()
};

#endif
