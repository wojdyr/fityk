// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_SDEBUG__H__
#define FITYK__WX_SDEBUG__H__

#include <wx/notebook.h>
#include "cmn.h" //close_it()

class ScriptDebugDlg : public wxDialog
{
public:
    ScriptDebugDlg(wxWindow* parent, wxWindowID id);
    void OpenFile(wxWindow *parent);
    void OnOpenFile(wxCommandEvent&) { OpenFile(this); }
    bool do_open_file(wxString const& path_);
    void OnSave(wxCommandEvent&);
    void OnSaveAs(wxCommandEvent&);
    void OnExecSelected(wxCommandEvent&) { ExecSelected(); }
    int ExecSelected();
    void OnExecDown(wxCommandEvent&);
    void OnClose(wxCommandEvent&) { close_it(this); }
    void OnPageChange(wxNotebookEvent& event);
    void OnTextChange(wxCommandEvent&);
    wxString get_list_item(int i);
    void exec_line(int n);
    void save_file(wxString const& path);
    wxString const& get_path() const { return path; }
protected:
    wxToolBar *tb;
    wxNotebook *nb;
    wxListView *list;
    wxTextCtrl *txt;
    wxString dir;
    wxString path;

    void add_line(int n, std::string const& line);
    void make_list_from_txt();
    void set_title();
    DECLARE_EVENT_TABLE()
};

#endif
