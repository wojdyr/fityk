// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <wx/wx.h>

#include <wx/treectrl.h>
#include <wx/textdlg.h>
#include <wx/image.h>
#include <wx/config.h>
#include <wx/clrpicker.h>
#include <wx/fontpicker.h>

#include "textpane.h"
#include "frame.h"
#include "inputline.h"

using namespace std;

enum {
    ID_OUTPUT_CLEAR     = 27001,
    ID_OUTPUT_CONFIGURE,
    ID_OUTPUT_EDITLINE
};

class OutputWinConfDlg : public wxDialog
{
public:
    OutputWinConfDlg(wxWindow* parent, wxWindowID id, OutputWin* ow);

private:
    OutputWin *ow_;
    wxStaticText *font_label_;
    wxColourPickerCtrl *cp_bg_, *cp_input_, *cp_output_, *cp_quote_,
                       *cp_warning_;
    wxFontPickerCtrl *font_picker_;
    wxTextCtrl *preview_;

    void show_preview();
    void OnSystemFontCheckbox(wxCommandEvent& event);
    void OnFontChange(wxFontPickerEvent& event);
    void OnColor(wxColourPickerEvent& event);
};


//===============================================================
//                            TextPane
//===============================================================

TextPane::TextPane(wxWindow *parent)
    : wxPanel(parent, -1)
{
    wxBoxSizer *io_sizer = new wxBoxSizer(wxVERTICAL);

    // on Linux platform GUI and CLI history is stored in the same file
    wxString hist_file = get_conf_file("history");
    // readline and editline (libedit) use different history format,
    // we like to use the same file file format as CLI, but it's not easy
    // with libedit. If it's libedit history we use a different file.
    FILE *f = fopen(hist_file.mb_str(), "r");
    if (f) {
        char buf[10];
        fgets(buf, 10, f);
        fclose(f);
        if (strncmp(buf, "_HiStOrY_", 9) == 0) // libedit weirdness
            hist_file += "_gui";
    }
    input_field = new InputLine(this, -1, this, hist_file);
    output_win = new OutputWin (this, -1);
    io_sizer->Add(output_win, 1, wxEXPAND);
    io_sizer->Add(input_field, 0, wxEXPAND);

    SetSizer(io_sizer);
    io_sizer->SetSizeHints(this);
}

void TextPane::edit_in_input(string const& s)
{
    input_field->SetValue(s2wx(s));
    input_field->SetFocus();
}

void TextPane::ProcessInputLine(wxString const& s)
{
    frame->set_status_text(wx2s(s));
    exec(wx2s(s)); //displaying and executing command
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
    bg_color_ = cfg_read_color(cf, wxT("bg"), wxColour(28, 28, 28));
    text_color_[UserInterface::kNormal] =
        cfg_read_color(cf, wxT("normal"), wxColour(160, 160, 160));
    text_color_[UserInterface::kInput] =
        cfg_read_color(cf, wxT("input"), wxColour(64, 133, 73));
    text_color_[UserInterface::kQuoted] =
        cfg_read_color(cf, wxT("quot"), wxColour(122, 142, 220));
    text_color_[UserInterface::kWarning] =
        cfg_read_color(cf, wxT("warn"), wxColour(192, 64, 64));

    cf->SetPath(wxT("/OutputWin"));
    wxFont font = cfg_read_font(cf, wxT("font"), wxNullFont);
    SetDefaultStyle(wxTextAttr(bg_color_, bg_color_, font));
    SetBackgroundColour(bg_color_);
    if (IsEmpty())
        show_fancy_dashes();
    Refresh();
}

void OutputWin::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/OutputWin/Colors"));
    cfg_write_color (cf, wxT("normal"), text_color_[UserInterface::kNormal]);
    cfg_write_color (cf, wxT("warn"), text_color_[UserInterface::kWarning]);
    cfg_write_color (cf, wxT("quot"), text_color_[UserInterface::kQuoted]);
    cfg_write_color (cf, wxT("input"), text_color_[UserInterface::kInput]);
    cfg_write_color (cf, wxT("bg"), bg_color_);
    cf->SetPath(wxT("/OutputWin"));
    cfg_write_font (cf, wxT("font"), GetDefaultStyle().GetFont());
}

void OutputWin::append_text (UserInterface::Style style, const wxString& str)
{
    const int max_len = 1048576;
    const int delta = 262144;
    if (GetLastPosition() > max_len)
        Remove(0, delta);

    SetDefaultStyle (wxTextAttr (text_color_[style]));
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

void OutputWin::show_preferences_dialog()
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
    InputLine *input_field = static_cast<TextPane*>(GetParent())->input_field;
    input_field->SetValue(selection_);
    input_field->SetFocus();
}


void OutputWin::OnRightDown (wxMouseEvent& event)
{
    wxMenu popup_menu;
    popup_menu.Append (ID_OUTPUT_EDITLINE, wxT("&Edit Line/Selection"));
    selection_ = GetStringSelection();
    if (selection_.empty()) {
        wxTextCoord col, row;
        if (HitTest(event.GetPosition(), &col, &row) != wxTE_HT_UNKNOWN) {
            wxString line = GetLineText(row);
            if (line.StartsWith(wxT("=-> ")))
                selection_ = line.substr(4);
        }
    }
    else if (selection_.StartsWith(wxT("=-> ")))
        selection_ = selection_.substr(4);
    if (selection_.empty())
        popup_menu.Enable(ID_OUTPUT_EDITLINE, false);
    popup_menu.Append (ID_OUTPUT_CLEAR, wxT("Clea&r"));
    popup_menu.Append (ID_OUTPUT_CONFIGURE, wxT("&Configure..."));
    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void OutputWin::OnKeyDown (wxKeyEvent& event)
{
    frame->focus_input(event);
}

void OutputWin::set_bg_color(wxColour const &color)
{
    bg_color_ = color;
    SetBackgroundColour (color);
    SetDefaultStyle (wxTextAttr(wxNullColour, color));
}


OutputWinConfDlg::OutputWinConfDlg(wxWindow* parent, wxWindowID id,
                                   OutputWin* ow)
  : wxDialog(parent, id, wxString(wxT("Configure Output Window")),
             wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
    ow_(ow)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    wxCheckBox *system_font_cb = new wxCheckBox(this, -1,
                                    wxT("use the regular system font"));
    top_sizer->Add(system_font_cb, wxSizerFlags().Border());

    wxBoxSizer *fsizer = new wxBoxSizer(wxHORIZONTAL);
    fsizer->AddSpacer(16);
    font_label_ = new wxStaticText(this, -1, wxT("font:"));
    fsizer->Add(font_label_, wxSizerFlags().Center().Border());
    font_picker_ = new wxFontPickerCtrl(this, -1,
                                             ow_->GetDefaultStyle().GetFont());
    fsizer->Add(font_picker_, wxSizerFlags(1).Center().Border());
    top_sizer->Add(fsizer, wxSizerFlags().Expand());

    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);

    wxGridSizer *gsizer = new wxGridSizer(2, 5, 5);
    wxSizerFlags cl = wxSizerFlags().Align(wxALIGN_CENTRE_VERTICAL),
             cr = wxSizerFlags().Align(wxALIGN_CENTRE_VERTICAL|wxALIGN_RIGHT);

    gsizer->Add(new wxStaticText(this, -1, wxT("background color")), cr);
    cp_bg_ = new wxColourPickerCtrl(this, -1, ow_->bg_color_);
    gsizer->Add(cp_bg_, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("input color")), cr);
    cp_input_ = new wxColourPickerCtrl(this, -1,
                                      ow_->text_color_[UserInterface::kInput]);
    gsizer->Add(cp_input_, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("output color")), cr);
    cp_output_ = new wxColourPickerCtrl(this, -1,
                                      ow_->text_color_[UserInterface::kNormal]);
    gsizer->Add(cp_output_, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("quotation color")), cr);
    cp_quote_ = new wxColourPickerCtrl(this, -1,
                                      ow_->text_color_[UserInterface::kQuoted]);
    gsizer->Add(cp_quote_, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("warning color")), cr);
    cp_warning_ = new wxColourPickerCtrl(this, -1,
                                    ow_->text_color_[UserInterface::kWarning]);
    gsizer->Add(cp_warning_, cl);

    hsizer->Add(gsizer, wxSizerFlags());

    preview_ = new wxTextCtrl(this, -1, wxT(""),
                             wxDefaultPosition, wxDefaultSize,
                             wxTE_MULTILINE|wxTE_RICH|wxTE_READONLY);
    preview_->SetMinSize(wxSize(140, -1));
    hsizer->Add(preview_, wxSizerFlags(1).Expand());

    top_sizer->Add(hsizer, wxSizerFlags(1).Expand().Border());

    top_sizer->Add(persistance_note(this), wxSizerFlags().Border());

    top_sizer->Add(new wxButton(this, wxID_CLOSE),
                   wxSizerFlags().Right().Border());

    SetSizerAndFit(top_sizer);

    if (ow_->GetDefaultStyle().GetFont() == *wxNORMAL_FONT) {
        system_font_cb->SetValue(true);
        font_label_->Enable(false);
        font_picker_->Enable(false);
    }

    show_preview();

    SetEscapeId(wxID_CLOSE);

    Connect(system_font_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnSystemFontCheckbox);
    Connect(font_picker_->GetId(), wxEVT_COMMAND_FONTPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnFontChange);
    Connect(cp_bg_->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColor);
    Connect(cp_input_->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColor);
    Connect(cp_output_->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColor);
    Connect(cp_quote_->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColor);
    Connect(cp_warning_->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            (wxObjectEventFunction) &OutputWinConfDlg::OnColor);
}

void OutputWinConfDlg::OnSystemFontCheckbox(wxCommandEvent& event)
{
    bool checked = event.IsChecked();
    font_label_->Enable(!checked);
    font_picker_->Enable(!checked);
    wxFont const& font = checked ? *wxNORMAL_FONT
                                 : font_picker_->GetSelectedFont();
    ow_->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour, font));
    show_preview();
}

void OutputWinConfDlg::OnFontChange(wxFontPickerEvent& event)
{
    ow_->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour,
                                    event.GetFont()));
    show_preview();
}

void OutputWinConfDlg::OnColor(wxColourPickerEvent& event)
{
    int id = event.GetId();
    if (id == cp_bg_->GetId())
        ow_->set_bg_color(event.GetColour());
    else if (id == cp_input_->GetId())
        ow_->text_color_[UserInterface::kInput] = event.GetColour();
    else if (id == cp_output_->GetId())
        ow_->text_color_[UserInterface::kNormal] = event.GetColour();
    else if (id == cp_quote_->GetId())
        ow_->text_color_[UserInterface::kQuoted] = event.GetColour();
    else if (id == cp_warning_->GetId())
        ow_->text_color_[UserInterface::kWarning] = event.GetColour();
    show_preview();
}

void OutputWinConfDlg::show_preview()
{
    const wxColour& output = ow_->text_color_[UserInterface::kNormal];
    const wxColour& input = ow_->text_color_[UserInterface::kInput];
    const wxColour& quote = ow_->text_color_[UserInterface::kQuoted];
    const wxColour& warning = ow_->text_color_[UserInterface::kWarning];

    preview_->Clear();
    preview_->SetBackgroundColour(ow_->bg_color_);
    preview_->SetDefaultStyle(ow_->GetDefaultStyle());
    preview_->SetDefaultStyle(wxTextAttr(output));
    preview_->AppendText(wxT("\nsettings preview\n\n"));
    preview_->SetDefaultStyle(wxTextAttr(input));
    preview_->AppendText(wxT("=-> i pi\n"));
    preview_->SetDefaultStyle(wxTextAttr(output));
    preview_->AppendText(wxT("3.14159\n"));
    preview_->SetDefaultStyle(wxTextAttr(input));
    preview_->AppendText(wxT("=-> c < file.fit\n"));
    preview_->SetDefaultStyle(wxTextAttr(quote));
    preview_->AppendText(wxT("1> bleh in file\n"));
    preview_->SetDefaultStyle(wxTextAttr(warning));
    preview_->AppendText(wxT("Syntax error.\n"));
}


