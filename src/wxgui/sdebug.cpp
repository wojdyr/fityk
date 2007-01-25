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
    ID_SE_CLOSE                 ,
    ID_SE_NB              
};


BEGIN_EVENT_TABLE(ScriptDebugDlg, wxDialog)
    EVT_TOOL(ID_SE_OPEN, ScriptDebugDlg::OnOpenFile)
    EVT_TOOL(ID_SE_EXECSEL, ScriptDebugDlg::OnExecSelected)
    EVT_TOOL(ID_SE_EXECDOWN, ScriptDebugDlg::OnExecDown)
    EVT_TOOL(ID_SE_SAVE, ScriptDebugDlg::OnSave)
    EVT_TOOL(ID_SE_SAVE_AS, ScriptDebugDlg::OnSaveAs)
    EVT_TOOL(ID_SE_CLOSE, ScriptDebugDlg::OnClose)
    EVT_NOTEBOOK_PAGE_CHANGED(ID_SE_NB, ScriptDebugDlg::OnPageChange)
END_EVENT_TABLE()

ScriptDebugDlg::ScriptDebugDlg(wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, wxT("unnamed"), 
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
    tb->AddTool(ID_SE_CLOSE, wxT("Close"), wxBitmap(close_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Exit debugger"), wxT("Close debugger"));
    tb->Realize();
    top_sizer->Add(tb, 0, wxEXPAND); 
    wxNotebook *nb = new wxNotebook(this, ID_SE_NB);

    list = new wxListView(nb, -1, 
                          wxDefaultPosition, wxDefaultSize,
                          wxLC_REPORT|wxLC_HRULES|wxLC_VRULES);
    list->InsertColumn(0, wxT("No"));
    list->InsertColumn(1, wxT("Text"));
    list->InsertColumn(2, wxT("Exec. time"));
    list->SetColumnWidth(0, 40);
    list->SetColumnWidth(1, 430);
    list->SetColumnWidth(2, 100);

    txt = new wxTextCtrl(nb, -1, wxT(""), wxDefaultPosition, wxDefaultSize,
                         wxTE_MULTILINE|wxTE_RICH);

    nb->AddPage(list, wxT("view"));
    nb->AddPage(txt, wxT("edit"));
    top_sizer->Add(nb, 1, wxALL|wxEXPAND, 0);
    SetSizerAndFit(top_sizer);
    SetSize(600, 500);
}

void ScriptDebugDlg::OpenFile()
{
    wxFileDialog dialog(this, wxT("open script file"), dir, wxT(""), 
                        wxT("fityk scripts (*.fit)|*.fit|all files|*"), 
                        wxOPEN | wxFILE_MUST_EXIST);
    if (dialog.ShowModal() == wxID_OK) {
        bool r = txt->LoadFile(dialog.GetPath());
        if (r) {
            path = wx2s(dialog.GetPath());
            SetTitle(path);
            make_list_from_txt();
        }
    }
    dir = dialog.GetDirectory();
}

void ScriptDebugDlg::make_list_from_txt()
{
    list->DeleteAllItems();
    for (int i = 0; i != txt->GetNumberOfLines(); ++i) {
        string line = wx2s(txt->GetLineText(i));
        // there is a bug in wxTextCtrl::GetLineText(),
        // empty line gives "\n"+next line
        if (!line.empty() && line[0] == '\n') // wx bug 
            line = wxT("");
        add_line(i+1, wx2s(line));
    }
}

void ScriptDebugDlg::OnSaveAs(wxCommandEvent&)
{
    wxFileDialog dialog(this, wxT("save script as..."), dir, wxT(""), 
                        wxT("fityk scripts (*.fit)|*.fit|all files|*"), 
                        wxSAVE | wxOVERWRITE_PROMPT);
    if (dialog.ShowModal() == wxID_OK) 
        save_file(wx2s(dialog.GetPath()));
    dir = dialog.GetDirectory();
}

void ScriptDebugDlg::save_file(string const& save_path)
{
    //TODO
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
    //TODO if edit/view
    for (long i=list->GetFirstSelected(); i!=-1; i=list->GetNextSelected(i)) {
        exec_line(i);
    }
}

void ScriptDebugDlg::OnExecDown(wxCommandEvent&)
{
    //TODO if edit/view
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

void ScriptDebugDlg::OnPageChange(wxNotebookEvent& event)
{
    int old_sel = event.GetOldSelection();
    if (old_sel == 0) {
        long line = list->GetFirstSelected();
        if (line >= 0)
            txt->SetInsertionPoint(txt->XYToPosition(0, line));
    }
    else if (old_sel == 1){
        long x, y;
        txt->PositionToXY(txt->GetInsertionPoint(), &x, &y);
        make_list_from_txt();
        list->Select(y);
        list->Focus(y);
        list->SetFocus();
    }
}

