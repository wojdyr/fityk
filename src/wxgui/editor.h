// This file is part of fityk program. Copyright Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_EDITOR_H_
#define FITYK_WX_EDITOR_H_

class Editor;
class wxStyledTextEvent;

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
    void do_save_file(const wxString& save_path);
    void update_title();
    void on_save();
    void on_save_as();

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
