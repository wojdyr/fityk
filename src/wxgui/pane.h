// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_PANE_H_
#define FITYK_WX_PANE_H_

#include "../ui.h" // UserInterface::Style
#include "inputline.h" // InputLineObserver

class wxConfigBase;
class InputLine;

class OutputWin : public wxTextCtrl
{
    friend class OutputWinConfDlg;
public:
    OutputWin(wxWindow *parent, wxWindowID id);
    void append_text (UserInterface::Style style, const wxString& str);
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
    void show_preferences_dialog();

private:
    wxColour text_color_[4];
    wxColour bg_color_;
    wxString selection_; // string passed to OnEditLine()

    void show_fancy_dashes();
    void set_bg_color(wxColour const &color);
    void OnRightDown (wxMouseEvent& event);
    void OnEditLine(wxCommandEvent&);
    void OnClear(wxCommandEvent&);
    void OnKeyDown (wxKeyEvent& event);
    void OnConfigure(wxCommandEvent&) { show_preferences_dialog(); }

    DECLARE_EVENT_TABLE()
};


/// A pane containing input line and output window.
class IOPane : public wxPanel, public InputLineObserver
{
public:
    IOPane(wxWindow *parent, wxWindowID id=-1);
    void edit_in_input(std::string const& s);

    // implementation of InputLineObserver
    virtual void ProcessInputLine(wxString const& s);

    OutputWin *output_win;
    InputLine *input_field;
};

#endif // FITYK_WX_PANE_H_

