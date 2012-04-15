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

#include <wx/stc/stc.h>
#if __WXMAC__
#include <wx/generic/buttonbar.h>
#define wxToolBar wxButtonToolBar
#endif

using namespace std;


enum {
    ID_SE_EXEC           = 28300,
    ID_SE_STEP                  ,
    ID_SE_SAVE                  ,
    ID_SE_SAVE_AS               ,
    ID_SE_CLOSE                 ,
    ID_SE_EDITOR
};


BEGIN_EVENT_TABLE(EditorDlg, wxDialog)
    EVT_TOOL(ID_SE_EXEC, EditorDlg::OnExec)
    EVT_TOOL(ID_SE_STEP, EditorDlg::OnStep)
    EVT_TOOL(ID_SE_SAVE, EditorDlg::OnSave)
    EVT_TOOL(ID_SE_SAVE_AS, EditorDlg::OnSaveAs)
    EVT_TOOL(ID_SE_CLOSE, EditorDlg::OnButtonClose)
#if wxUSE_STC
    EVT_STC_CHANGE(ID_SE_EDITOR, EditorDlg::OnTextChange)
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
    void set_filetype(bool) {}
    int GetCurrentLine() const
            { long x, y; PositionToXY(GetInsertionPoint(), &x, &y); return y; }
    void GotoLine(int line) { SetInsertionPoint(ed_->XYToPosition(0, line)); }
};
#endif


EditorDlg::EditorDlg(wxWindow* parent)
    : wxDialog(parent, -1, wxT(""),
               wxDefaultPosition, wxSize(600, 500),
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
      lua_file_(false)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    tb_ = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize,
                       wxTB_TEXT | wxTB_HORIZONTAL | wxNO_BORDER);
    tb_->SetToolBitmapSize(wxSize(24, 24));
    tb_->AddTool(ID_SE_EXEC, wxT("Execute"),
                 wxBitmap(exec_selected_xpm), wxNullBitmap,
                 wxITEM_NORMAL,
                 wxT("Execute all or selected lines"));
    tb_->AddTool(ID_SE_STEP, wxT("Step"),
                 wxBitmap(exec_down_xpm), wxNullBitmap,
                 wxITEM_NORMAL,
                 wxT("Execute line and go to the next line"));
    tb_->AddSeparator();
    tb_->AddTool(ID_SE_SAVE, wxT("Save"), wxBitmap(save_xpm), wxNullBitmap,
                 wxITEM_NORMAL,
                 wxT("Save to file"));
    tb_->AddTool(ID_SE_SAVE_AS, wxT("Save as"),
                 wxBitmap(save_as_xpm), wxNullBitmap,
                 wxITEM_NORMAL,
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
    ed_ = new Editor(this, ID_SE_EDITOR);
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
    lua_file_ = path.Lower().EndsWith("lua");
    ed_->set_filetype(lua_file_);
#if wxUSE_STC
    // i don't know why, but in wxGTK 2.9.3 initially all text is selected
    ed_->ClearSelections();
#endif
    ed_->DiscardEdits();
    update_title();
}

void EditorDlg::new_file_with_content(const wxString& content)
{
    ed_->ChangeValue(content);
    path_.clear();
    lua_file_ = content.StartsWith("--");
    ed_->set_filetype(lua_file_);
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

void EditorDlg::on_save()
{
    if (!path_.empty())
        do_save_file(path_);
    else
        on_save_as();
}

void EditorDlg::on_save_as()
{
    wxFileDialog dialog(this, "save script as...", frame->script_dir(), "",
                        "Fityk scripts (*.fit)|*.fit;*.FIT"
                        "|Lua scripts (*.lua)|*.lua;*.LUA"
                        "|all files|*",
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (lua_file_)
        dialog.SetFilterIndex(1);
    if (dialog.ShowModal() == wxID_OK) {
        do_save_file(dialog.GetPath());
        frame->set_script_dir(dialog.GetDirectory());
    }
}

void EditorDlg::do_save_file(const wxString& save_path)
{
    bool r = ed_->SaveFile(save_path);
    if (r) {
        path_ = save_path;
        ed_->DiscardEdits();
        update_title();
    }
}

string EditorDlg::get_editor_line(int n)
{
    string line = wx2s(ed_->GetLineText(n));
    if (!line.empty() && *(line.end()-1) == '\n') {
        line.resize(line.size() - 1);
        if (!line.empty() && *(line.end()-1) == '\r')
            line.resize(line.size() - 1);
    }
    return line;
}

int EditorDlg::exec_fityk_line(int n)
{
    if (n < 0)
        return 1;
    string s = get_editor_line(n);
    int counter = 1;
    while (!s.empty() && *(s.end()-1) == '\\') {
        s.resize(s.size() - 1);
        s += get_editor_line(n+counter);
        ++counter;
    }

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
    return counter;
}

int EditorDlg::exec_lua_line(int n)
{
    if (n < 0)
        return 1;
    string s = get_editor_line(n);
    int counter = 1;
    if (s.empty())
        return counter;
    while (ftk->get_ui()->is_lua_line_incomplete(s.c_str())) {
        s += "\n    " + get_editor_line(n+counter);
        ++counter;
    }

    ftk->exec("lua " + s);
    /*
    UserInterface::Status r = ftk->exec(s);
    if (r == UserInterface::kStatusOk) {
    }
    else { // error
    }
    */
    return counter;
}

void EditorDlg::OnExec(wxCommandEvent&)
{
    long p1, p2;
    ed_->GetSelection(&p1, &p2);
    long y1=0, y2=0;
    if (p1 != p2) { //selection, exec whole lines
        long x;
        ed_->PositionToXY(p1, &x, &y1);
        ed_->PositionToXY(p2, &x, &y2);
    }
    else
        y2 = ed_->GetNumberOfLines();

    for (int i = y1; i <= y2; )
        i += lua_file_ ? exec_lua_line(i) : exec_fityk_line(i);
}

void EditorDlg::OnStep(wxCommandEvent&)
{
    int y  = ed_->GetCurrentLine();
    int n = lua_file_ ? exec_lua_line(y) : exec_fityk_line(y);
    ed_->GotoLine(y+n);
}

void EditorDlg::OnTextChange(wxStyledTextEvent&)
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

void EditorDlg::OnCloseDlg(wxCloseEvent& event)
{
    if (ed_->IsModified()) {
        int r = wxMessageBox("Save before closing?", "Save?",
                             wxYES_NO | wxCANCEL | wxICON_QUESTION);
        if (r == wxCANCEL) {
            event.Veto();
            return;
        }
        if (r == wxYES)
            on_save();
    }
    Destroy();
}

