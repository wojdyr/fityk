// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

///  Script Editor and Debugger (EditorDlg)

#include <wx/wx.h>

#include "editor.h"
#include "frame.h" //ftk
#include "../logic.h"

#include "img/exec_selected.xpm"
#include "img/exec_down.xpm"
#include "img/save.xpm"
#include "img/save_as.xpm"
#include "img/close.xpm"

#if __WXMAC__
#include <wx/generic/buttonbar.h>
#define wxToolBar wxButtonToolBar
#endif
#if wxUSE_STC
#include <wx/stc/stc.h>
#endif

using namespace std;


enum {
    ID_SE_EXECSEL        = 28300,
    ID_SE_STEP                  ,
    ID_SE_SAVE                  ,
    ID_SE_SAVE_AS               ,
    ID_SE_CLOSE                 ,
    ID_SE_TXT
};


BEGIN_EVENT_TABLE(EditorDlg, wxDialog)
    EVT_TOOL(ID_SE_EXECSEL, EditorDlg::OnExecSelected)
    EVT_TOOL(ID_SE_STEP, EditorDlg::OnStep)
    EVT_TOOL(ID_SE_SAVE, EditorDlg::OnSave)
    EVT_TOOL(ID_SE_SAVE_AS, EditorDlg::OnSaveAs)
    EVT_TOOL(ID_SE_CLOSE, EditorDlg::OnButtonClose)
#if wxUSE_STC
    EVT_TEXT(ID_SE_TXT, EditorDlg::OnTextChange)
#else
    EVT_STC_CHANGE(ID_SE_TXT, EditorDlg::OnTextChange)
#endif
    EVT_CLOSE(EditorDlg::OnCloseDlg)
END_EVENT_TABLE()

#if wxUSE_STC
class Editor : public wxStyledTextCtrl
{
public:
    Editor(wxWindow* parent, wxWindowID id)
        : wxStyledTextCtrl(parent, id)
    {
        SetMarginType(0, wxSTC_MARGIN_NUMBER);
        SetMarginWidth(0, 32);
        SetUseVerticalScrollBar(true);
        wxFont mono(11, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_NORMAL);
        StyleSetFont(wxSTC_STYLE_DEFAULT, mono);
    }

    void set_filetype(bool lua)
    {
        wxColour comment_col(10, 150, 10);
        if (lua) {
            SetLexer(wxSTC_LEX_LUA);
            StyleSetForeground(wxSTC_LUA_COMMENT, comment_col);
            StyleSetForeground(wxSTC_LUA_COMMENTLINE, comment_col);
        }
        else {
            // actually we set filetype to apache config, but since
            // we only customize colors of comments it works fine.
            SetLexer(wxSTC_LEX_CONF);
            StyleSetForeground(wxSTC_CONF_COMMENT, comment_col);
        }
    }
};
#else
class Editor : public wxTextCtrl
{
public:
    Editor(wxWindow* parent, wxWindowID id)
        : wxTextCtrl(parent, id, "", wxDefaultPosition, wxDefaultSize,
                     wxTE_MULTILINE|wxTE_RICH) {}
};
#endif


EditorDlg::EditorDlg(wxWindow* parent)
    : wxDialog(parent, -1, wxT(""),
               wxDefaultPosition, wxSize(600, 500),
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    tb_ = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize,
                       wxTB_TEXT | wxTB_HORIZONTAL | wxNO_BORDER);
    tb_->SetToolBitmapSize(wxSize(24, 24));
    tb_->AddTool(ID_SE_EXECSEL, wxT("Execute"),
                 wxBitmap(exec_selected_xpm), wxNullBitmap,
                 wxITEM_NORMAL, wxT("Execute line/selection"),
                 wxT("Execute selected lines"));
    tb_->AddTool(ID_SE_STEP, wxT("Step"),
                 wxBitmap(exec_down_xpm), wxNullBitmap,
                 wxITEM_NORMAL,
                 wxT("Execute line(s) and go to the next line"),
                 wxT("Execute selected lines"));
    tb_->AddSeparator();
    tb_->AddTool(ID_SE_SAVE, wxT("Save"), wxBitmap(save_xpm), wxNullBitmap,
                 wxITEM_NORMAL, wxT("Save"),
                 wxT("Save to file"));
    tb_->AddTool(ID_SE_SAVE_AS, wxT("Save as"),
                 wxBitmap(save_as_xpm), wxNullBitmap,
                 wxITEM_NORMAL, wxT("Save as..."),
                 wxT("Save a copy to file"));
#if 0
    tb_->AddSeparator();
    tb_->AddTool(wxID_UNDO, wxT("Undo"),
                 wxBitmap(undo_xpm), wxNullBitmap,
                 wxITEM_NORMAL, wxT("Undo"),
                 wxT("Undo the last edit"));
    tb_->AddTool(wxID_REDO, wxT("Redo"),
                 wxBitmap(redo_xpm), wxNullBitmap,
                 wxITEM_NORMAL, wxT("Redo"),
                 wxT("Redo the last undone edit"));
#endif
    tb_->AddSeparator();
    tb_->AddTool(ID_SE_CLOSE, wxT("Close"), wxBitmap(close_xpm), wxNullBitmap,
                 wxITEM_NORMAL, wxT("Exit debugger"), wxT("Close debugger"));
#if __WXMAC__
    for (size_t i = 0; i < tb_->GetToolsCount(); ++i) {
        const wxToolBarToolBase *tool = tb_->GetToolByPos(i);
        tb_->SetToolShortHelp(tool->GetId(), tool->GetLabel());
    }
#endif
    tb_->Realize();
    top_sizer->Add(tb_, 0, wxEXPAND);
    ed_ = new Editor(this, ID_SE_TXT);
    top_sizer->Add(ed_, 1, wxALL|wxEXPAND, 0);
    SetSizerAndFit(top_sizer);
    SetSize(600, 500);
}

void EditorDlg::open_file(const wxString& path)
{
    if (wxFileExists(path))
        ed_->LoadFile(path);
    else
        ed_->Clear();
    path_ = path;
    bool lua = path.Lower().EndsWith("lua");
    ed_->set_filetype(lua);
    ed_->DiscardEdits();
    update_title();
}

void EditorDlg::new_file_with_content(const wxString& content)
{
    ed_->ChangeValue(content);
    path_.clear();
    bool lua = content.StartsWith("--");
    ed_->set_filetype(lua);
#if wxUSE_STC
    // i don't know why, but in wxGTK 2.9.3 initially all text is selected
    ed_->ClearSelections();
    ed_->GotoLine(ed_->GetLineCount() - 1);
#else
    ed_->SetInsertionPointEnd();
#endif
    ed_->DiscardEdits();
    update_title();
}

void EditorDlg::OnSave(wxCommandEvent& event)
{
    if (!path_.empty())
        save_file(path_);
    else
        OnSaveAs(event);
}

void EditorDlg::OnSaveAs(wxCommandEvent&)
{
    wxFileDialog dialog(this, "save script as...", frame->script_dir(), "",
                        "fityk scripts (*.fit)|*.fit;*.FIT"
                        "|Lua scripts (*.lua)|*.lua;*.LUA"
                        "|all files|*",
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() == wxID_OK) {
        save_file(dialog.GetPath());
        frame->set_script_dir(dialog.GetDirectory());
    }
}

void EditorDlg::save_file(const wxString& save_path)
{
    bool r = ed_->SaveFile(save_path);
    if (r) {
        path_ = save_path;
        ed_->DiscardEdits();
        update_title();
    }
}

void EditorDlg::exec_line(int n)
{
    wxString line;
    line = ed_->GetLineText(n);
    // there was a bug in wxTextCtrl::GetLineText(),
    // empty line gives "\n"+next line
    if (!line.empty() && line[0] == '\n') // wx bug
        line = wxT("");
#if __WXMAC__
    if (line.EndsWith(wxT("\n")))
        line.resize(line.size() - 1);
#endif
    string s = wx2s(line);

    if (s.find("_EXECUTED_SCRIPT_DIR_/") != string::npos) {
        string dir = get_directory(wx2s(path_));
        replace_all(s, "_EXECUTED_SCRIPT_DIR_/", dir);
    }
    ftk->exec(s);
    /*
    UserInterface::Status r = ftk->exec(s);
    if (r == UserInterface::kStatusOk) {
    }
    else { // error
    }
    */
}

int EditorDlg::exec_selected()
{
    long from, to;
    ed_->GetSelection(&from, &to);
    if (from == to) { //no selection
        long x, y;
        ed_->PositionToXY(ed_->GetInsertionPoint(), &x, &y);
        if (y >= 0)
            exec_line(y);
        return y;
    }
    else { //selection, exec all lines (not only selection)
        long x, y1, y2;
        ed_->PositionToXY(from, &x, &y1);
        ed_->PositionToXY(to, &x, &y2);
        for (int i = y1; i <= y2; ++i) {
            if (i >= 0)
                exec_line(i);
        }
        return y2;
    }
}

void EditorDlg::OnStep(wxCommandEvent&)
{
    long last = exec_selected();
    if (last == -1)
        return;
    ++last;
    ed_->SetInsertionPoint(ed_->XYToPosition(0, last));
}

void EditorDlg::OnTextChange(wxCommandEvent&)
{
    if (GetTitle().EndsWith(wxT(" *")) != ed_->IsModified())
        update_title();
#if 0
    tb_->EnableTool(wxID_UNDO, ed_->CanUndo());
    tb_->EnableTool(wxID_REDO, ed_->CanRedo());
#endif
}

void EditorDlg::update_title()
{
    wxString p = path_.empty() ? wxString(wxT("unnamed")) : path_;
    if (ed_->IsModified())
        p += wxT(" *");
    SetTitle(p);
    tb_->EnableTool(ID_SE_SAVE, !path_.empty() && ed_->IsModified());
}

void EditorDlg::OnCloseDlg(wxCloseEvent&)
{
    if (ed_->IsModified()) {
    }
    Destroy();
}

