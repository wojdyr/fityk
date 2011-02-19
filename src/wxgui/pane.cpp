// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/fontdlg.h>
#include <wx/treectrl.h>
#include <wx/textdlg.h>
#include <wx/image.h>

#include "pane.h"
#include "frame.h"
#include "inputline.h"
#include "../common.h"
#include "../logic.h"
#include "../ui.h"

using namespace std;

enum {
    ID_OUTPUT_CLEAR     = 27001,
    ID_OUTPUT_CONFIGURE,
    ID_OUTPUT_EDITLINE
};


//===============================================================
//                            IOPane
//===============================================================

IOPane::IOPane(wxWindow *parent, wxWindowID id)
    : wxPanel(parent, id)
{
    wxBoxSizer *io_sizer = new wxBoxSizer (wxVERTICAL);

    // on Linux platform GUI and CLI history is stored in the same file
    wxString hist_file = get_conf_file("history");
    input_field = new InputLine(this, -1, this, hist_file);
    output_win = new OutputWin (this, -1);
    io_sizer->Add (output_win, 1, wxEXPAND);
    io_sizer->Add (input_field, 0, wxEXPAND);

    SetSizer(io_sizer);
    io_sizer->SetSizeHints(this);
}

void IOPane::edit_in_input(string const& s)
{
    input_field->SetValue(s2wx(s));
    input_field->SetFocus();
}

void IOPane::ProcessInputLine(wxString const& s)
{
    frame->set_status_text(wx2s(s));
    ftk->exec(wx2s(s)); //displaying and executing command
}

//===============================================================
//                            OutputWin
//===============================================================

BEGIN_EVENT_TABLE(OutputWin, wxTextCtrl)
    EVT_RIGHT_DOWN (                      OutputWin::OnRightDown)
    EVT_MENU       (ID_OUTPUT_CLEAR,      OutputWin::OnClear)
    EVT_MENU       (ID_OUTPUT_CONFIGURE,  OutputWin::OnConfigure)
    EVT_MENU       (ID_OUTPUT_EDITLINE,   OutputWin::OnEditLine)
    EVT_KEY_DOWN   (                      OutputWin::OnKeyDown)
END_EVENT_TABLE()

OutputWin::OutputWin (wxWindow *parent, wxWindowID id)
    : wxTextCtrl(parent, id, wxT(""), wxDefaultPosition, wxDefaultSize,
                 wxTE_MULTILINE|wxTE_RICH|wxNO_BORDER|wxTE_READONLY)
{}

void OutputWin::show_fancy_dashes() {
    for (int i = 0; i < 16; i++) {
        SetDefaultStyle (wxTextAttr (wxColour(i * 16, i * 16, i * 16)));
        AppendText (wxT("-"));
    }
    AppendText (wxT("\n"));
}

void OutputWin::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/OutputWin/Colors"));
    bg_color = cfg_read_color(cf, wxT("bg"), wxColour(28, 28, 28));
    text_color[UserInterface::kNormal] =
        cfg_read_color(cf, wxT("normal"), wxColour(160, 160, 160));
    text_color[UserInterface::kInput] =
        cfg_read_color(cf, wxT("input"), wxColour(64, 133, 73));
    text_color[UserInterface::kQuoted] =
        cfg_read_color(cf, wxT("quot"), wxColour(122, 142, 220));
    text_color[UserInterface::kWarning] =
        cfg_read_color(cf, wxT("warn"), wxColour(192, 64, 64));

    cf->SetPath(wxT("/OutputWin"));
    wxFont font = cfg_read_font(cf, wxT("font"), wxNullFont);
    SetDefaultStyle(wxTextAttr(bg_color, bg_color, font));
    SetBackgroundColour(bg_color);
    if (IsEmpty())
        show_fancy_dashes();
    Refresh();
}

void OutputWin::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/OutputWin/Colors"));
    cfg_write_color (cf, wxT("normal"), text_color[UserInterface::kNormal]);
    cfg_write_color (cf, wxT("warn"), text_color[UserInterface::kWarning]);
    cfg_write_color (cf, wxT("quot"), text_color[UserInterface::kQuoted]);
    cfg_write_color (cf, wxT("input"), text_color[UserInterface::kInput]);
    cfg_write_color (cf, wxT("bg"), bg_color);
    cf->SetPath(wxT("/OutputWin"));
    cfg_write_font (cf, wxT("font"), GetDefaultStyle().GetFont());
}

void OutputWin::append_text (UserInterface::Style style, const wxString& str)
{
    const int max_len = 1048576;
    const int delta = 262144;
    if (GetLastPosition() > max_len)
        Remove(0, delta);

    SetDefaultStyle (wxTextAttr (text_color[style]));
    AppendText (str);

    // in wxMSW 2.9.1(trunk) there is a scrolling-related bug:
    // when the control is auto-scrolled after appending text, 
    // the page gets blank. Clicking on scrollbar shows the text again.
    // Below is a workaround.
#ifdef __WXMSW__
    ScrollLines(-1);
    ScrollLines(1);
#endif
}

void OutputWin::OnConfigure (wxCommandEvent&)
{
    OutputWinConfDlg dlg(NULL, -1, this);
    dlg.ShowModal();
}

void OutputWin::OnClear (wxCommandEvent&)
{
    Clear();
    show_fancy_dashes();
}

void OutputWin::OnEditLine (wxCommandEvent&)
{
    InputLine *input_field = static_cast<IOPane*>(GetParent())->input_field;
    input_field->SetValue(selection);
    input_field->SetFocus();
}


void OutputWin::OnRightDown (wxMouseEvent& event)
{
    wxMenu popup_menu;
    popup_menu.Append (ID_OUTPUT_EDITLINE, wxT("&Edit Line/Selection"));
    selection = GetStringSelection();
    if (selection.empty()) {
        wxTextCoord col, row;
        if (HitTest(event.GetPosition(), &col, &row) != wxTE_HT_UNKNOWN)
            selection = GetLineText(row);
    }
    if (selection.StartsWith(wxT("=-> ")))
        selection = selection.substr(4);
    if (selection.empty())
        popup_menu.Enable(ID_OUTPUT_EDITLINE, false);
    popup_menu.Append (ID_OUTPUT_CLEAR, wxT("Clea&r"));
    popup_menu.Append (ID_OUTPUT_CONFIGURE, wxT("&Configure"));
    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void OutputWin::OnKeyDown (wxKeyEvent& event)
{
    frame->focus_input(event);
}

void OutputWin::set_bg_color(wxColour const &color)
{
    bg_color = color;
    SetBackgroundColour (color);
    SetDefaultStyle (wxTextAttr(wxNullColour, color));
}


OutputWinConfDlg::OutputWinConfDlg(wxWindow* parent, wxWindowID id,
                                   OutputWin* ow_)
  : wxDialog(parent, id, wxString(wxT("Configure Status Bar")),
             wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
    ow(ow_)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    wxCheckBox *system_font_cb = new wxCheckBox(this, -1,
                                    wxT("use the regular system font"));
    top_sizer->Add(system_font_cb, wxSizerFlags().Border());

    wxBoxSizer *fsizer = new wxBoxSizer(wxHORIZONTAL);
    fsizer->AddSpacer(16);
    font_label = new wxStaticText(this, -1, wxT("font:"));
    fsizer->Add(font_label, wxSizerFlags().Center().Border());
    font_picker = new wxFontPickerCtrl(this, -1,
                                             ow->GetDefaultStyle().GetFont());
    fsizer->Add(font_picker, wxSizerFlags(1).Center().Border());
    top_sizer->Add(fsizer, wxSizerFlags().Expand());

    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);

    wxGridSizer *gsizer = new wxGridSizer(2, 5, 5);
    wxSizerFlags cl = wxSizerFlags().Align(wxALIGN_CENTRE_VERTICAL),
             cr = wxSizerFlags().Align(wxALIGN_CENTRE_VERTICAL|wxALIGN_RIGHT);

    gsizer->Add(new wxStaticText(this, -1, wxT("background color")), cr);
    cp_bg = new wxColourPickerCtrl(this, -1, ow->bg_color);
    gsizer->Add(cp_bg, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("input color")), cr);
    cp_input = new wxColourPickerCtrl(this, -1,
                                      ow->text_color[UserInterface::kInput]);
    gsizer->Add(cp_input, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("output color")), cr);
    cp_output = new wxColourPickerCtrl(this, -1,
                                      ow->text_color[UserInterface::kNormal]);
    gsizer->Add(cp_output, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("quotation color")), cr);
    cp_quote = new wxColourPickerCtrl(this, -1,
                                      ow->text_color[UserInterface::kQuoted]);
    gsizer->Add(cp_quote, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("warning color")), cr);
    cp_warning = new wxColourPickerCtrl(this, -1,
                                      ow->text_color[UserInterface::kWarning]);
    gsizer->Add(cp_warning, cl);

    hsizer->Add(gsizer, wxSizerFlags());

    preview = new wxTextCtrl(this, -1, wxT(""),
                             wxDefaultPosition, wxDefaultSize,
                             wxTE_MULTILINE|wxTE_RICH|wxTE_READONLY);
    preview->SetMinSize(wxSize(140, -1));
    hsizer->Add(preview, wxSizerFlags(1).Expand());

    top_sizer->Add(hsizer, wxSizerFlags(1).Expand().Border());

    top_sizer->Add(persistance_note_sizer(this),
                   wxSizerFlags().Expand().Border());

    top_sizer->Add(new wxButton(this, wxID_CLOSE),
                   wxSizerFlags().Right().Border());

    SetSizerAndFit(top_sizer);

    if (ow->GetDefaultStyle().GetFont() == *wxNORMAL_FONT) {
        system_font_cb->SetValue(true);
        font_label->Enable(false);
        font_picker->Enable(false);
    }

    show_preview();

    SetEscapeId(wxID_CLOSE);

    Connect(system_font_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnSystemFontCheckbox);
    Connect(font_picker->GetId(), wxEVT_COMMAND_FONTPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnFontChange);
    Connect(cp_bg->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColor);
    Connect(cp_input->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColor);
    Connect(cp_output->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColor);
    Connect(cp_quote->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColor);
    Connect(cp_warning->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColor);
}

void OutputWinConfDlg::OnSystemFontCheckbox(wxCommandEvent& event)
{
    bool checked = event.IsChecked();
    font_label->Enable(!checked);
    font_picker->Enable(!checked);
    wxFont const& font = checked ? *wxNORMAL_FONT
                                 : font_picker->GetSelectedFont();
    ow->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour, font));
    show_preview();
}

void OutputWinConfDlg::OnFontChange(wxFontPickerEvent& event)
{
    ow->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour,
                                   event.GetFont()));
    show_preview();
}

void OutputWinConfDlg::OnColor(wxColourPickerEvent& event)
{
    int id = event.GetId();
    if (id == cp_bg->GetId())
        ow->set_bg_color(event.GetColour());
    else if (id == cp_input->GetId())
        ow->text_color[UserInterface::kInput] = event.GetColour();
    else if (id == cp_output->GetId())
        ow->text_color[UserInterface::kNormal] = event.GetColour();
    else if (id == cp_quote->GetId())
        ow->text_color[UserInterface::kQuoted] = event.GetColour();
    else if (id == cp_warning->GetId())
        ow->text_color[UserInterface::kWarning] = event.GetColour();
    show_preview();
}

void OutputWinConfDlg::show_preview()
{
    const wxColour& output = ow->text_color[UserInterface::kNormal];
    const wxColour& input = ow->text_color[UserInterface::kInput];
    const wxColour& quote = ow->text_color[UserInterface::kQuoted];
    const wxColour& warning = ow->text_color[UserInterface::kWarning];

    preview->Clear();
    preview->SetBackgroundColour(ow->bg_color);
    preview->SetDefaultStyle(ow->GetDefaultStyle());
    preview->SetDefaultStyle(wxTextAttr(output));
    preview->AppendText(wxT("\nsettings preview\n\n"));
    preview->SetDefaultStyle(wxTextAttr(input));
    preview->AppendText(wxT("=-> i pi\n"));
    preview->SetDefaultStyle(wxTextAttr(output));
    preview->AppendText(wxT("3.14159\n"));
    preview->SetDefaultStyle(wxTextAttr(input));
    preview->AppendText(wxT("=-> c < file.fit\n"));
    preview->SetDefaultStyle(wxTextAttr(quote));
    preview->AppendText(wxT("1> bleh in file\n"));
    preview->SetDefaultStyle(wxTextAttr(warning));
    preview->AppendText(wxT("Syntax error.\n"));
}


