// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

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
    ID_OUTPUT_CONFIGURE
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
    input_field = new InputLine(this, -1,
              make_callback<wxString const&>().V1(this, &IOPane::OnInputLine),
              hist_file);
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

void IOPane::OnInputLine(wxString const& s)
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
    EVT_KEY_DOWN   (                      OutputWin::OnKeyDown)
END_EVENT_TABLE()

OutputWin::OutputWin (wxWindow *parent, wxWindowID id,
                      const wxPoint& pos, const wxSize& size)
    : wxTextCtrl(parent, id, wxT(""), pos, size,
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
    text_color[os_normal] = cfg_read_color(cf, wxT("normal"),
                                                      wxColour(150, 150, 150));
    text_color[os_warn] = cfg_read_color(cf, wxT("warn"),
                                                    wxColour(200, 0, 0));
    text_color[os_quot] = cfg_read_color(cf, wxT("quot"),
                                                    wxColour(50, 50, 255));
    text_color[os_input] = cfg_read_color(cf, wxT("input"),
                                                     wxColour(0, 200, 0));
    bg_color = cfg_read_color(cf, wxT("bg"), wxColour(20, 20, 20));
    cf->SetPath(wxT("/OutputWin"));
    wxFont font = cfg_read_font(cf, wxT("font"), wxNullFont);
    SetDefaultStyle (wxTextAttr(text_color[os_quot], bg_color, font));
    SetBackgroundColour(bg_color);
    if (IsEmpty())
        show_fancy_dashes();
    Refresh();
}

void OutputWin::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/OutputWin/Colors"));
    cfg_write_color (cf, wxT("normal"), text_color[os_normal]);
    cfg_write_color (cf, wxT("warn"), text_color[os_warn]);
    cfg_write_color (cf, wxT("quot"), text_color[os_quot]);
    cfg_write_color (cf, wxT("input"), text_color[os_input]);
    cfg_write_color (cf, wxT("bg"), bg_color);
    cf->SetPath(wxT("/OutputWin"));
    cfg_write_font (cf, wxT("font"), GetDefaultStyle().GetFont());
}

void OutputWin::append_text (OutputStyle style, const wxString& str)
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


void OutputWin::OnRightDown (wxMouseEvent& event)
{
    wxMenu popup_menu;
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
    bg_cpicker = new wxColourPickerCtrl(this, -1, ow->bg_color);
    gsizer->Add(bg_cpicker, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("input color")), cr);
    t0_cpicker = new wxColourPickerCtrl(this, -1, ow->text_color[os_input]);
    gsizer->Add(t0_cpicker, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("output color")), cr);
    t1_cpicker = new wxColourPickerCtrl(this, -1, ow->text_color[os_normal]);
    gsizer->Add(t1_cpicker, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("quotation color")), cr);
    t2_cpicker = new wxColourPickerCtrl(this, -1, ow->text_color[os_quot]);
    gsizer->Add(t2_cpicker, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("warning color")), cr);
    t3_cpicker = new wxColourPickerCtrl(this, -1, ow->text_color[os_warn]);
    gsizer->Add(t3_cpicker, cl);

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
    Connect(bg_cpicker->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColorBg);
    Connect(t0_cpicker->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColorT0);
    Connect(t1_cpicker->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColorT1);
    Connect(t2_cpicker->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColorT2);
    Connect(t3_cpicker->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColorT3);
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

void OutputWinConfDlg::show_preview()
{
    preview->Clear();
    preview->SetBackgroundColour(ow->bg_color);
    preview->SetDefaultStyle(ow->GetDefaultStyle());
    preview->SetDefaultStyle (wxTextAttr (ow->text_color[os_normal]));
    preview->AppendText(wxT("\nsettings preview\n\n"));
    preview->SetDefaultStyle (wxTextAttr (ow->text_color[os_input]));
    preview->AppendText(wxT("=-> i pi\n"));
    preview->SetDefaultStyle (wxTextAttr (ow->text_color[os_normal]));
    preview->AppendText(wxT("3.14159\n"));
    preview->SetDefaultStyle (wxTextAttr (ow->text_color[os_input]));
    preview->AppendText(wxT("=-> c < file.fit\n"));
    preview->SetDefaultStyle (wxTextAttr (ow->text_color[os_quot]));
    preview->AppendText(wxT("1> bleh in file\n"));
    preview->SetDefaultStyle (wxTextAttr (ow->text_color[os_warn]));
    preview->AppendText(wxT("Syntax error.\n"));
}


