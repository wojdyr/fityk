// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
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
#include "gui.h" //ftk
#include "../cmd.h" //check_command_syntax()
#include "../logic.h" 

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
    ID_SE_NB                    ,
    ID_SE_TXT
};


BEGIN_EVENT_TABLE(ScriptDebugDlg, wxDialog)
    EVT_TOOL(ID_SE_OPEN, ScriptDebugDlg::OnOpenFile)
    EVT_TOOL(ID_SE_EXECSEL, ScriptDebugDlg::OnExecSelected)
    EVT_TOOL(ID_SE_EXECDOWN, ScriptDebugDlg::OnExecDown)
    EVT_TOOL(ID_SE_SAVE, ScriptDebugDlg::OnSave)
    EVT_TOOL(ID_SE_SAVE_AS, ScriptDebugDlg::OnSaveAs)
    EVT_TOOL(ID_SE_CLOSE, ScriptDebugDlg::OnClose)
    EVT_NOTEBOOK_PAGE_CHANGED(ID_SE_NB, ScriptDebugDlg::OnPageChange)
    EVT_TEXT(ID_SE_TXT, ScriptDebugDlg::OnTextChange)
END_EVENT_TABLE()

ScriptDebugDlg::ScriptDebugDlg(wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, wxT(""), 
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
#if 0
    tb->AddSeparator();
    tb->AddTool(wxID_UNDO, wxT("Undo"), 
                wxBitmap(undo_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Undo"), 
                wxT("Undo the last edit"));
    tb->AddTool(wxID_REDO, wxT("Redo"), 
                wxBitmap(redo_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Redo"), 
                wxT("Redo the last undone edit"));
#endif
    tb->AddSeparator();
    tb->AddTool(ID_SE_CLOSE, wxT("Close"), wxBitmap(close_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Exit debugger"), wxT("Close debugger"));
    tb->Realize();
    top_sizer->Add(tb, 0, wxEXPAND); 
    nb = new wxNotebook(this, ID_SE_NB);

    list = new wxListView(nb, -1, 
                          wxDefaultPosition, wxDefaultSize,
                          wxLC_REPORT|wxLC_HRULES|wxLC_VRULES);
    list->InsertColumn(0, wxT("No"));
    list->InsertColumn(1, wxT("Text"));
    list->InsertColumn(2, wxT("Exec. time"));
    list->SetColumnWidth(0, 40);
    list->SetColumnWidth(1, 430);
    list->SetColumnWidth(2, 100);

    txt = new wxTextCtrl(nb, ID_SE_TXT, wxT(""), 
                         wxDefaultPosition, wxDefaultSize,
                         wxTE_MULTILINE|wxTE_RICH);

    nb->AddPage(list, wxT("view"));
    nb->AddPage(txt, wxT("edit"));
    top_sizer->Add(nb, 1, wxALL|wxEXPAND, 0);
    SetSizerAndFit(top_sizer);
    SetSize(600, 500);
    set_title();
}

void ScriptDebugDlg::OpenFile(wxWindow *parent)
{
    wxFileDialog dialog(parent, wxT("open script file"), dir, wxT(""), 
                        wxT("fityk scripts (*.fit)|*.fit|all files|*"), 
                        wxFD_OPEN /*| wxFD_FILE_MUST_EXIST*/);
    if (dialog.ShowModal() == wxID_OK) 
        do_open_file(dialog.GetPath());
    dir = dialog.GetDirectory();
}

bool ScriptDebugDlg::do_open_file(wxString const& path_)
{
    bool r = false;
    if (wxFileExists(path_)) 
        r = txt->LoadFile(path_);
    else
        txt->Clear();
    if (!r)
        txt->MarkDirty(); //new file -> modified
    path = path_;
    script_dir = get_directory(wx2s(path));
    set_title();
    make_list_from_txt();
    return r;
}

void ScriptDebugDlg::make_list_from_txt()
{
    list->DeleteAllItems();
    // there is at least one bug in wxTextCtrl::GetLineText()
    // (empty line gives "\n"+next line) so we don't use it
    // the bug was fixed on 2007-04-09
    vector<string> lines = split_string(wx2s(txt->GetValue()), "\n");
    for (size_t i = 0; i != lines.size(); ++i) 
        add_line(i+1, lines[i]);
}

void ScriptDebugDlg::OnSave(wxCommandEvent& event)
{ 
    if (!path.IsEmpty())
        save_file(path); 
    else
        OnSaveAs(event);
}

void ScriptDebugDlg::OnSaveAs(wxCommandEvent&)
{
    wxFileDialog dialog(this, wxT("save script as..."), dir, wxT(""), 
                        wxT("fityk scripts (*.fit)|*.fit|all files|*"), 
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() == wxID_OK) 
        save_file(dialog.GetPath());
    dir = dialog.GetDirectory();
}

void ScriptDebugDlg::save_file(wxString const& save_path)
{
    bool r = txt->SaveFile(save_path);
    if (r) {
        path = save_path;
        script_dir = get_directory(wx2s(path));
        txt->DiscardEdits();
        set_title();
    }
}

void ScriptDebugDlg::add_line(int n, string const& line)
{
    int pos = list->GetItemCount();
    list->InsertItem(pos, n == -1 ? wxString(wxT("...")) 
                                  : wxString::Format(wxT("%i"), n));
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
    string::size_type nonblank = head.find_first_not_of(" \r\n\t");
    bool is_comment = (nonblank == string::npos || head[nonblank] == '#');
    list->SetItemData(pos, is_comment ? 1 : 0); //to be skipped if comment
    if (is_comment) 
        list->SetItemTextColour(pos, *wxGREEN);
    else if (!check_command_syntax(head))
        list->SetItemBackgroundColour(pos, *wxRED);
    else if (head[nonblank] == 'i' || head[nonblank] == 'p') { //info or plot
        list->SetItemTextColour(pos, *wxBLUE);
    }
    if (!tail.empty())
        add_line(-1, tail);
}

void ScriptDebugDlg::exec_line(int n)
{
    wxStopWatch sw;
    wxString line;
    if (nb->GetSelection() == 0)  //view tab
        line = get_list_item(n);
    else if (nb->GetSelection() == 1) { //edit tab
        line = txt->GetLineText(n);
        // there was a bug in wxTextCtrl::GetLineText(),
        // empty line gives "\n"+next line
        if (!line.empty() && line[0] == '\n') // wx bug 
            line = wxT("");
    }
    string s = wx2s(line);
    replace_all(s, "_EXECUTED_SCRIPT_DIR_/", script_dir);
    Commands::Status r = ftk->exec(s);
    long millisec = sw.Time();
    if (nb->GetSelection() == 0) { //view tab
        if (r == Commands::status_ok) {
            list->SetItem(n, 2, wxString::Format(wxT("%.2f"), millisec/1000.));
        }
        else {
            list->SetItem(n, 2, wxT("ERROR"));
        }
    }
}

int ScriptDebugDlg::ExecSelected()
{
    long last = -1;
    if (nb->GetSelection() == 0) { //view tab
        for (long i = list->GetFirstSelected(); i != -1; 
                i = list->GetNextSelected(i)) {
            exec_line(i);
            last = i;
        }
    }
    else if (nb->GetSelection() == 1) { //edit tab
        long from, to;
        txt->GetSelection(&from, &to);
        if (from == to) { //no selection
            long x, y;
            txt->PositionToXY(txt->GetInsertionPoint(), &x, &y);
            if (y >= 0) 
                exec_line(y);
            last = y;
        }
        else { //selection, exec all lines (not only selection)
            long x, y, y2;
            txt->PositionToXY(from, &x, &y);
            txt->PositionToXY(to, &x, &y2);
            for (int i = y; i <= y2; ++i) {
                if (i >= 0) 
                    exec_line(i);
            }
            return y2;
        }
    }
    return last;
}

void ScriptDebugDlg::OnExecDown(wxCommandEvent&)
{
    long last = ExecSelected();
    if (last == -1)
        return;
    ++last;
    if (nb->GetSelection() == 0) { //view tab
        //skip comments
        while (last < list->GetItemCount() && list->GetItemData(last) == 1)
            ++last;
        for (long i = 0; i < list->GetItemCount(); ++i)
            list->Select(i, i == last);
        if (last < list->GetItemCount())
            list->Focus(last);
    }
    else { //edit tab
        txt->SetInsertionPoint(txt->XYToPosition(0, last));
    }
}

wxString ScriptDebugDlg::get_list_item(int i)
{
    wxListItem info;
    info.SetId(i);
    info.SetColumn(1);
    info.SetMask(wxLIST_MASK_TEXT);
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
#if 0
        tb->EnableTool(wxID_UNDO, txt->CanUndo());
        tb->EnableTool(wxID_REDO, txt->CanRedo());
#endif
    }
    else if (old_sel == 1) {
        long x, y;
        txt->PositionToXY(txt->GetInsertionPoint(), &x, &y);
        make_list_from_txt();
        list->Select(y);
        list->Focus(y);
        list->SetFocus();
#if 0
        tb->EnableTool(wxID_UNDO, false);
        tb->EnableTool(wxID_REDO, false);
#endif
    }
}

void ScriptDebugDlg::OnTextChange(wxCommandEvent&)
{
    //if (GetTitle().EndsWith(" *") != txt->IsModified())  //wx2.8
    if ((GetTitle().Right(2) == wxT(" *")) != txt->IsModified())
        set_title();
#if 0
    tb->EnableTool(wxID_UNDO, txt->CanUndo());
    tb->EnableTool(wxID_REDO, txt->CanRedo());
#endif
}

void ScriptDebugDlg::set_title()
{
    wxString p = path.IsEmpty() ? wxString(wxT("unnamed")) : path;
    if (txt->IsModified())
        p += wxT(" *");
    SetTitle(p);
}


