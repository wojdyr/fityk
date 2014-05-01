// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

///  Script Editor and Debugger (EditorDlg)

#include <wx/wx.h>

#include "editor.h"
#include "frame.h" //ftk, exec()
#include "fityk/logic.h"
#include "fityk/luabridge.h"

#include "img/exec_selected.xpm"
#include "img/exec_down.xpm"
#include "img/save.xpm"
#include "img/save_as.xpm"
#include "img/close.xpm"

#include <wx/stc/stc.h>
#ifdef __WXOSX_CARBON__
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


// compare with
// http://sourceforge.net/p/scintilla/scite/ci/default/tree/src/lua.properties
// http://git.geany.org/geany/tree/data/filetypes.lua
static const char* kLuaMostOfKeywords =
"and break do else elseif end for function goto if in local not"
" or repeat return then until while";

static const char* kLuaValueKeywords = "false nil true";

// constants and functions
static const char* kLuaFunctions =
"_G _VERSION _ENV "
" assert collectgarbage dofile error getmetatable ipairs load loadfile"
" next pairs pcall print rawequal rawget rawlen rawset require"
" select setmetatable tonumber tostring type xpcall"
" bit32.arshift bit32.band bit32.bnot bit32.bor bit32.btest bit32.bxor"
" bit32.extract bit32.lrotate bit32.lshift bit32.replace bit32.rrotate"
" bit32.rshift"
" coroutine.create coroutine.resume coroutine.running"
" coroutine.status coroutine.wrap coroutine.yield"
" debug.debug debug.getfenv debug.gethook debug.getinfo debug.getlocal"
" debug.getmetatable debug.getregistry debug.getupvalue"
" debug.getuservalue debug.setfenv debug.sethook debug.setlocal"
" debug.setmetatable debug.setupvalue debug.setuservalue debug.traceback"
" debug.upvalueid debug.upvaluejoin "
" io.close io.flush io.input io.lines io.open io.output io.popen io.read"
" io.stderr io.stdin io.stdout io.tmpfile io.type io.write"
" math.abs math.acos math.asin math.atan math.atan2 math.ceil math.cos"
" math.cosh math.deg math.exp math.floor math.fmod math.frexp math.huge"
" math.ldexp math.log math.log10 math.max math.min math.modf math.pi"
" math.pow math.rad math.random math.randomseed math.sin math.sinh"
" math.sqrt math.tan math.tanh"
" os.clock os.date os.difftime os.execute os.exit os.getenv os.remove"
" os.rename os.setlocale os.time os.tmpname"
" package.config package.cpath package.loaded package.loadlib package.path"
" package.preload package.searchers package.searchpath"
" string.byte string.char string.dump string.find"
" string.format string.gmatch string.gsub string.len string.lower"
" string.match string.rep string.reverse string.sub string.upper"
" table.concat table.insert table.maxn table.pack table.remove"
" table.sort table.unpack";

static const char* kLuaMethods =
"close flush lines read seek setvbuf write"
" byte find format gmatch gsub len lower match rep reverse sub upper";

static const char* kLuaTableF =
"F:add_point F:all_functions F:all_parameters F:all_variables"
" F:calculate_expr F:execute F:executef F:get_components"
" F:get_covariance_matrix F:get_data F:get_dataset_count"
" F:get_default_dataset F:get_dof F:get_function F:get_info"
" F:get_model_value F:get_option_as_number F:get_option_as_string"
" F:get_parameter_count F:get_rsquared F:get_ssr F:get_variable"
" F:get_view_boundary F:get_wssr F:input F:load_data F:out"
" F:set_option_as_number F:set_option_as_string";

#if wxUSE_STC
class FitykEditor : public wxStyledTextCtrl
{
public:
    FitykEditor(wxWindow* parent, wxWindowID id)
        : wxStyledTextCtrl(parent, id)
    {
        SetMarginType(0, wxSTC_MARGIN_NUMBER);
        SetMarginWidth(0, 32);
        SetUseVerticalScrollBar(true);
#ifdef __WXMAC__
        const int font_size = 13;
#else
        const int font_size = 11;
#endif
        wxFont mono(wxFontInfo(font_size).Family(wxFONTFAMILY_TELETYPE));
        StyleSetFont(wxSTC_STYLE_DEFAULT, mono);
        SetIndent(2);
        SetUseTabs(false);
        SetTabIndents(true);
        SetBackSpaceUnIndents(true);
        AutoCompSetAutoHide(false);
        AutoCompSetCancelAtStart(false);
        SetIndentationGuides(3 /*SC_IV_LOOKBOTH*/);
        wxFont bold(StyleGetFont(wxSTC_STYLE_DEFAULT).Bold());
        StyleSetFont(wxSTC_STYLE_BRACELIGHT, bold);
        StyleSetForeground(wxSTC_STYLE_BRACELIGHT, wxColour(0, 0, 255));
        StyleSetFont(wxSTC_STYLE_BRACEBAD, bold);
        StyleSetForeground(wxSTC_STYLE_BRACEBAD, wxColour(144, 0, 0));
        Connect(wxID_ANY, wxEVT_STC_CHARADDED,
                wxStyledTextEventHandler(FitykEditor::OnCharAdded));
        Connect(wxID_ANY, wxEVT_STC_UPDATEUI,
                wxStyledTextEventHandler(FitykEditor::OnUpdateUI));
    }

    void set_filetype(bool lua)
    {
        wxColour comment_col(10, 150, 10);
        if (lua) {
            // use similar colors as in Lua wiki
            SetLexer(wxSTC_LEX_LUA);
            //SetProperty("fold", "1");
            StyleSetForeground(wxSTC_LUA_COMMENT, comment_col);
            StyleSetForeground(wxSTC_LUA_COMMENTLINE, comment_col);
            StyleSetForeground(wxSTC_LUA_COMMENTDOC, comment_col);
            wxColour string_col(0, 144, 144);
            StyleSetForeground(wxSTC_LUA_STRING, string_col);
            StyleSetForeground(wxSTC_LUA_CHARACTER, string_col);
            StyleSetForeground(wxSTC_LUA_LITERALSTRING, string_col);
            StyleSetForeground(wxSTC_LUA_NUMBER, wxColour(32, 80, 96));
            SetKeyWords(0, kLuaMostOfKeywords);
            wxFont bold(StyleGetFont(wxSTC_STYLE_DEFAULT).Bold());
            StyleSetFont(wxSTC_LUA_WORD, bold);
            StyleSetForeground(wxSTC_LUA_WORD, wxColour(0, 0, 128));
            SetKeyWords(1, kLuaValueKeywords);
            StyleSetForeground(wxSTC_LUA_WORD2, wxColour(0, 0, 128));
            SetKeyWords(2, kLuaFunctions);
            StyleSetForeground(wxSTC_LUA_WORD3, wxColour(144, 0, 144));
            SetKeyWords(3, kLuaMethods);
            StyleSetForeground(wxSTC_LUA_WORD4, wxColour(144, 0, 144));
            SetKeyWords(4, "F");
            StyleSetFont(wxSTC_LUA_WORD5, bold);
        } else {
            // actually we set filetype to apache config, but since
            // we only customize colors of comments it works fine.
            SetLexer(wxSTC_LEX_CONF);
            StyleSetForeground(wxSTC_CONF_COMMENT, comment_col);
        }
    }

    void OnCharAdded(wxStyledTextEvent& event)
    {
        // auto-indentation (the same as previous line)
        if (event.GetKey() == '\n') {
            int line = GetCurrentLine();
            if (line > 0) { // just in case
                int indent = GetLineIndentation(line-1);
                if (indent > 0) {
                    SetLineIndentation(line, indent);
                    LineEnd();
                }
            }
        }
        // auto-completion
        if (GetLexer() == wxSTC_LEX_LUA && !AutoCompActive())
            LuaAutocomp();
    }

    void LuaAutocomp()
    {
        int pos = GetCurrentPos() - 1; // GetCurrentPos() returns
        if (pos < 1)
            return;
        char key = GetCharAt(pos);
        if (key == ':' && GetStyleAt(pos-1) == wxSTC_LUA_WORD5) {
            // wxSTC_LUA_WORD5 is only "F", so other checks are redundant:
            //      GetCharAt(pos-1) == 'F' &&
            //      WordStartPosition(pos-1, true) == pos-1)
            AutoCompShow(2, kLuaTableF);
        }
        else if (key == '.' && GetStyleAt(pos-1) == wxSTC_LUA_IDENTIFIER) {
            int start = WordStartPosition(pos-1, true);
            wxString word = GetTextRange(start, pos);
            if (word == "bit32" || word == "coroutine" || word == "debug" ||
                    word == "io" || word == "math" || word == "os" ||
                    word == "package" || word == "string" || word == "table") {
                AutoCompShow(pos-start+1, kLuaFunctions);
            }
        }
    }

    void OnUpdateUI(wxStyledTextEvent&)
    {
        const char* braces = "(){}[]";
        int p = GetCurrentPos();
        int brace_pos = -1;
        if (p > 1 && strchr(braces, GetCharAt(p-1)))
            brace_pos = p - 1;
        else if (strchr(braces, GetCharAt(p)))
            brace_pos = p;
        if (brace_pos != -1) {
            int match_pos = BraceMatch(brace_pos);
            if (match_pos == wxSTC_INVALID_POSITION)
                BraceBadLight(brace_pos);
            else
                BraceHighlight(brace_pos, match_pos);
        } else {
            BraceBadLight(wxSTC_INVALID_POSITION); // remove brace light
        }
    }
};

#else
class FitykEditor : public wxTextCtrl
{
public:
    FitykEditor(wxWindow* parent, wxWindowID id)
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
#ifdef __WXOSX_CARBON__
    for (size_t i = 0; i < tb_->GetToolsCount(); ++i) {
        const wxToolBarToolBase *tool = tb_->GetToolByPos(i);
        tb_->SetToolShortHelp(tool->GetId(), tool->GetLabel());
    }
#endif
    tb_->Realize();
    top_sizer->Add(tb_, 0, wxEXPAND);
    ed_ = new FitykEditor(this, ID_SE_EDITOR);
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

    // the same replacement as in UserInterface::exec_script()
    if (s.find("_SCRIPT_DIR_/") != string::npos) {
        string dir = fityk::get_directory(wx2s(path_));
        fityk::replace_all(s, "_EXECUTED_SCRIPT_DIR_/", dir);// old magic string
        fityk::replace_all(s, "_SCRIPT_DIR_/", dir); // new magic string
    }

    exec(s);
    /*
    UserInterface::Status r = exec(s);
    if (r == UserInterface::kStatusOk) {
    } else { // error
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
    while (ftk->lua_bridge()->is_lua_line_incomplete(s.c_str())) {
        if (n+counter >= ed_->GetLineCount())
            break;
        s += "\n    " + get_editor_line(n+counter);
        ++counter;
    }

    exec("lua " + s);
    /*
    UserInterface::Status r = exec(s);
    if (r == UserInterface::kStatusOk) {
    } else { // error
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
    } else
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

