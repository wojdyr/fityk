// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

///  Script Editor and Debugger (ScriptDebugDlg)

#include <wx/wx.h>

#include "sdebug.h"
#include "frame.h" //ftk
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
    ID_SE_STEP                  ,
    ID_SE_SAVE                  ,
    ID_SE_SAVE_AS               ,
    ID_SE_CLOSE                 ,
    ID_SE_TXT
};


BEGIN_EVENT_TABLE(ScriptDebugDlg, wxDialog)
    EVT_TOOL(ID_SE_OPEN, ScriptDebugDlg::OnOpenFile)
    EVT_TOOL(ID_SE_EXECSEL, ScriptDebugDlg::OnExecSelected)
    EVT_TOOL(ID_SE_STEP, ScriptDebugDlg::OnStep)
    EVT_TOOL(ID_SE_SAVE, ScriptDebugDlg::OnSave)
    EVT_TOOL(ID_SE_SAVE_AS, ScriptDebugDlg::OnSaveAs)
    EVT_TOOL(ID_SE_CLOSE, ScriptDebugDlg::OnClose)
    EVT_TEXT(ID_SE_TXT, ScriptDebugDlg::OnTextChange)
    EVT_CLOSE(ScriptDebugDlg::OnCloseDlg)
END_EVENT_TABLE()

ScriptDebugDlg::ScriptDebugDlg(wxWindow* parent)
    : wxDialog(parent, -1, wxT(""),
               wxDefaultPosition, wxSize(600, 500),
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    tb = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize,
                       wxTB_TEXT | wxTB_HORIZONTAL | wxNO_BORDER);
    tb->SetToolBitmapSize(wxSize(24, 24));
    tb->AddTool(ID_SE_OPEN, wxT("Open"), wxBitmap(open_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Open File"), wxT("Open script file"));
    tb->AddSeparator();
    tb->AddTool(ID_SE_EXECSEL, wxT("Execute"),
                wxBitmap(exec_selected_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Execute line/selection"),
                wxT("Execute selected lines"));
    tb->AddTool(ID_SE_STEP, wxT("Step"),
                wxBitmap(exec_down_xpm), wxNullBitmap,
                wxITEM_NORMAL,
                wxT("Execute line(s) and go to the next line"),
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
    txt = new wxTextCtrl(this, ID_SE_TXT, wxT(""),
                         wxDefaultPosition, wxDefaultSize,
                         wxTE_MULTILINE|wxTE_RICH);

    top_sizer->Add(txt, 1, wxALL|wxEXPAND, 0);
    SetSizerAndFit(top_sizer);
    SetSize(600, 500);
    set_title();
}

void ScriptDebugDlg::open_file(wxWindow *parent)
{
    wxFileDialog dialog(parent, wxT("open script file"), dir, wxT(""),
                        wxT("fityk scripts (*.fit)|*.fit|all files|*"),
                        wxFD_OPEN /*| wxFD_FILE_MUST_EXIST*/);
    if (dialog.ShowModal() == wxID_OK)
        do_open_file(dialog.GetPath());
    dir = dialog.GetDirectory();
}

// can be used to "open" a new file (not existing)
void ScriptDebugDlg::do_open_file(const wxString& path)
{
    if (wxFileExists(path))
        txt->LoadFile(path);
    else {
        txt->Clear();
        txt->MarkDirty(); // new file -> modified
    }
    path_ = path;
    script_dir = get_directory(wx2s(path_));
    set_title();
}

void ScriptDebugDlg::OnSave(wxCommandEvent& event)
{
    if (!path_.empty())
        save_file(path_);
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

void ScriptDebugDlg::save_file(const wxString& save_path)
{
    bool r = txt->SaveFile(save_path);
    if (r) {
        path_ = save_path;
        script_dir = get_directory(wx2s(path_));
        txt->DiscardEdits();
        set_title();
    }
}

void ScriptDebugDlg::exec_line(int n)
{
    wxString line;
    line = txt->GetLineText(n);
    // there was a bug in wxTextCtrl::GetLineText(),
    // empty line gives "\n"+next line
    if (!line.empty() && line[0] == '\n') // wx bug
        line = wxT("");
    string s = wx2s(line);
    replace_all(s, "_EXECUTED_SCRIPT_DIR_/", script_dir);
    ftk->exec(s);
    /*
    UserInterface::Status r = ftk->exec(s);
    if (r == UserInterface::kStatusOk) {
    }
    else { // error
    }
    */
}

int ScriptDebugDlg::exec_selected()
{
    long from, to;
    txt->GetSelection(&from, &to);
    if (from == to) { //no selection
        long x, y;
        txt->PositionToXY(txt->GetInsertionPoint(), &x, &y);
        if (y >= 0)
            exec_line(y);
        return y;
    }
    else { //selection, exec all lines (not only selection)
        long x, y1, y2;
        txt->PositionToXY(from, &x, &y1);
        txt->PositionToXY(to, &x, &y2);
        for (int i = y1; i <= y2; ++i) {
            if (i >= 0)
                exec_line(i);
        }
        return y2;
    }
}

void ScriptDebugDlg::OnStep(wxCommandEvent&)
{
    long last = exec_selected();
    if (last == -1)
        return;
    ++last;
    txt->SetInsertionPoint(txt->XYToPosition(0, last));
}

void ScriptDebugDlg::OnTextChange(wxCommandEvent&)
{
    if (GetTitle().EndsWith(wxT(" *")) != txt->IsModified())
        set_title();
#if 0
    tb->EnableTool(wxID_UNDO, txt->CanUndo());
    tb->EnableTool(wxID_REDO, txt->CanRedo());
#endif
}

void ScriptDebugDlg::set_title()
{
    wxString p = path_.empty() ? wxString(wxT("unnamed")) : path_;
    if (txt->IsModified())
        p += wxT(" *");
    SetTitle(p);
    tb->EnableTool(ID_SE_SAVE, !path_.empty() && txt->IsModified());
}


