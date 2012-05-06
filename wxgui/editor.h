// This file is part of fityk program. Copyright Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_EDITOR_H_
#define FITYK_WX_EDITOR_H_

class FitykEditor;
class wxStyledTextEvent;

class EditorDlg : public wxDialog
{
public:
    EditorDlg(wxWindow* parent);
    void open_file(const wxString& path);
    void new_file_with_content(const wxString& content);

private:
    wxToolBarBase *tb_;
    FitykEditor *ed_;
    wxString path_;
    bool lua_file_;

    void do_save_file(const wxString& save_path);
    void update_title();
    void on_save();
    void on_save_as();
    std::string get_editor_line(int n);
    int exec_fityk_line(int n);
    int exec_lua_line(int n);

    void OnSave(wxCommandEvent&) { on_save(); }
    void OnSaveAs(wxCommandEvent&) { on_save_as(); }
    void OnExec(wxCommandEvent&);
    void OnStep(wxCommandEvent&);
    void OnButtonClose(wxCommandEvent&) { Close(); }
    void OnCloseDlg(wxCloseEvent& event);
    void OnTextChange(wxStyledTextEvent&);
    DECLARE_EVENT_TABLE()
};

#endif
