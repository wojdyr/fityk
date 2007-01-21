// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_SDEBUG__H__
#define FITYK__WX_SDEBUG__H__

#include <cmn.h> //close_it()

class ScriptDebugDlg : public wxDialog
{
public:
    ScriptDebugDlg(wxWindow* parent, wxWindowID id);
    void OpenFile();
    void OnOpenFile(wxCommandEvent&) { OpenFile(); }
    void OnExecSelected(wxCommandEvent&);
    void OnExecDown(wxCommandEvent&);
    void OnClose(wxCommandEvent&) { close_it(this); }
    wxString get_list_item(int i);
    void exec_line(int n);
protected:
    wxToolBar *tb;
    wxListView *list;

    void add_line(int n, std::string const& line);
    DECLARE_EVENT_TABLE()
};

#endif
