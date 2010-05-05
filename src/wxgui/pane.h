// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__WX_PANE__H__
#define FITYK__WX_PANE__H__

#include <list>
#include <vector>
#include <assert.h>
#include <wx/config.h>
#include <wx/spinctrl.h>
#include <wx/clrpicker.h>
#include <wx/fontpicker.h>

#include "../ui.h" // UserInterface::Style
#include "inputline.h" // InputLineObserver

class IOPane;
class MainPlot;
class AuxPlot;
class FPlot;
class BgManager;
class InputLine;


class OutputWin : public wxTextCtrl
{
    friend class OutputWinConfDlg;
public:
    OutputWin (wxWindow *parent, wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize);
    void append_text (UserInterface::Style style, const wxString& str);
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
    void show_fancy_dashes();
    void set_bg_color(wxColour const &color);
    void OnRightDown (wxMouseEvent& event);
    void OnConfigure(wxCommandEvent&);
    void OnClear(wxCommandEvent&);
    void OnKeyDown (wxKeyEvent& event);

private:
    wxColour text_color[4];
    wxColour bg_color;

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

class OutputWinConfDlg : public wxDialog
{
public:
    OutputWinConfDlg(wxWindow* parent, wxWindowID id, OutputWin* ow_);

private:
    OutputWin *ow;
    wxStaticText *font_label;
    wxColourPickerCtrl *cp_bg, *cp_input, *cp_output, *cp_quote, *cp_warning;
    wxFontPickerCtrl *font_picker;
    wxTextCtrl *preview;

    void show_preview();
    void OnSystemFontCheckbox(wxCommandEvent& event);
    void OnFontChange(wxFontPickerEvent& event);
    void OnColor(wxColourPickerEvent& event);
};

#endif

