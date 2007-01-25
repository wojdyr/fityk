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
    void OpenFile();
    void OnOpenFile(wxCommandEvent&) { OpenFile(); }
    void OnSave(wxCommandEvent&) { save_file(path); }
    void OnSaveAs(wxCommandEvent&);
    void OnExecSelected(wxCommandEvent&);
    void OnExecDown(wxCommandEvent&);
    void OnClose(wxCommandEvent&) { close_it(this); }
    void OnPageChange(wxNotebookEvent& event);
    wxString get_list_item(int i);
    void exec_line(int n);
    void save_file(std::string const& path);
protected:
    wxToolBar *tb;
    wxListView *list;
    wxTextCtrl *txt;
    wxString dir;
    std::string path;

    void add_line(int n, std::string const& line);
    void make_list_from_txt();
    DECLARE_EVENT_TABLE()
};

#endif
