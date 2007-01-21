// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

/// In this file:
///  Script Editor and Debugger (ScriptDebugDlg)

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
//#include <wx/spinctrl.h>
#include <fstream>

#include "sdebug.h"
#include "../cmd.h" //check_command_syntax()
#include "../ui.h" // Commands::Status

#include "img/open.xpm"
#include "img/exec_selected.xpm"
#include "img/exec_down.xpm"
#include "img/save.xpm"
#include "img/save_as.xpm"
#include "img/close.xpm"

using namespace std;

enum {
    ID_SE_OPEN           = 28300,
    ID_SE_EXECSEL               ,
    ID_SE_EXECDOWN              ,
    ID_SE_SAVE                  ,
    ID_SE_SAVE_AS               ,
    ID_SE_CLOSE                 
};


BEGIN_EVENT_TABLE(ScriptDebugDlg, wxDialog)
    EVT_TOOL(ID_SE_OPEN, ScriptDebugDlg::OnOpenFile)
    EVT_TOOL(ID_SE_EXECSEL, ScriptDebugDlg::OnExecSelected)
    EVT_TOOL(ID_SE_EXECDOWN, ScriptDebugDlg::OnExecDown)
    EVT_TOOL(ID_SE_CLOSE, ScriptDebugDlg::OnClose)
END_EVENT_TABLE()

ScriptDebugDlg::ScriptDebugDlg(wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, wxT("(not working yet) Script Editor and Debugger"), 
               wxDefaultPosition, wxSize(600, 500), 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    tb = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, 
                       wxTB_HORIZONTAL | wxNO_BORDER);
    tb->SetToolBitmapSize(wxSize(24, 24));
    tb->AddTool(ID_SE_OPEN, wxT("Open"), wxBitmap(open_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Open File"), wxT("Open script file"));
    tb->AddSeparator();
    tb->AddTool(ID_SE_EXECSEL, wxT("Execute"), 
                wxBitmap(exec_selected_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Execute selected"), 
                wxT("Execute selected lines"));
    tb->AddTool(ID_SE_EXECDOWN, wxT("Execute"), 
                wxBitmap(exec_down_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Execute selected and select next"), 
                wxT("Execute selected lines"));
    tb->AddSeparator();
    tb->AddTool(ID_SE_SAVE, wxT("Save"), wxBitmap(save_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Save"), 
                wxT("Save to file"));
    tb->AddTool(ID_SE_SAVE_AS, wxT("Save as"), 
                wxBitmap(save_as_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Save as..."), 
                wxT("Save a copy to file"));
    tb->AddSeparator();
    //TODO Edit, Move up, Move down
    tb->AddTool(ID_SE_CLOSE, wxT("Close"), wxBitmap(close_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Exit debugger"), wxT("Close debugger"));
    tb->Realize();
    top_sizer->Add(tb, 0, wxEXPAND); 
    list = new wxListView(this, -1, 
                          wxDefaultPosition, wxDefaultSize,
                          wxLC_REPORT|wxLC_HRULES|wxLC_VRULES);
    list->InsertColumn(0, wxT("No"));
    list->InsertColumn(1, wxT("Text"));
    list->InsertColumn(2, wxT("Exec. time"));
    list->SetColumnWidth(0, 40);
    list->SetColumnWidth(1, 430);
    list->SetColumnWidth(2, 100);

    //TODO last execution status column (ok, warning, error)
    top_sizer->Add(list, 1, wxALL|wxEXPAND, 0);
    //TODO editor
    SetSizerAndFit(top_sizer);
    SetSize(600, 500);
}

void ScriptDebugDlg::OpenFile()
{
    wxFileDialog dialog(this, wxT("open script file"), wxT(""), wxT(""), 
                        wxT("fityk scripts (*.fit)|*.fit|all files|*"), 
                        wxOPEN | wxFILE_MUST_EXIST);
    if (dialog.ShowModal() != wxID_OK) 
        return;
    string path = wx2s(dialog.GetPath());
    ifstream f(path.c_str());
    if (!f) 
        return;
    list->DeleteAllItems();
    string line;
    int n = 0;
    while (getline(f, line)) {
        add_line(++n, line);
    }
    list->InsertItem(list->GetItemCount(), wxT("+"));
}

void ScriptDebugDlg::add_line(int n, string const& line)
{
    int pos = list->GetItemCount();
    list->InsertItem(pos, n == -1 ? wxString(wxT("...")) 
                                  : wxString::Format("%i", n));
    string::size_type sep = line.find(';');
    string::size_type hash = line.find('#');
    string head, tail;
    if (sep != string::npos && (hash == string::npos || sep < hash)) {
        head = string(line, 0, sep+1);
        tail = string(line, sep+1);
    }
    else
        head = line;
    
    list->SetItem(pos, 1, s2wx(head));
    //TODO categories: comment, syntax error, info, plot, ok
    string::size_type nb = head.find_first_not_of(" \r\n\t");
    bool is_comment = (nb == string::npos || head[nb] == '#');
    if (is_comment) 
        list->SetItemTextColour(pos, *wxLIGHT_GREY);
    else if (!check_command_syntax(head))
        list->SetItemBackgroundColour(pos, *wxRED);
    if (!tail.empty())
        add_line(-1, tail);
}

void ScriptDebugDlg::exec_line(int n)
{
    wxStopWatch sw;
    Commands::Status r = exec_command(wx2s(get_list_item(n)));
    long millisec = sw.Time();
    if (r == Commands::status_ok) {
        list->SetItem(n, 2, wxString::Format(wxT("%.2f"), millisec/1000.));
    }
    else {
        list->SetItem(n, 2, wxT("ERROR"));
    }
}

void ScriptDebugDlg::OnExecSelected(wxCommandEvent&)
{
    for (long i=list->GetFirstSelected(); i!=-1; i=list->GetNextSelected(i)) {
        exec_line(i);
    }
}

void ScriptDebugDlg::OnExecDown(wxCommandEvent&)
{
    long last = -1;
    for (long i=list->GetFirstSelected(); i!=-1; i=list->GetNextSelected(i)) {
        exec_line(i);
        last = i;
    }
    if (last == -1)
        return;
    ++last;
    for (long i = 0; i < list->GetItemCount(); ++i)
        list->Select(i, i == last);
    if (last < list->GetItemCount())
        list->Focus(last);
}

wxString ScriptDebugDlg::get_list_item(int i)
{
    wxListItem info;
    info.SetId(i);
    info.SetColumn(1);
    list->GetItem(info);
    return info.GetText();
}

