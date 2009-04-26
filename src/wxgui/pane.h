// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

#ifndef FITYK__WX_PANE__H__
#define FITYK__WX_PANE__H__

#include <wx/config.h>
#include <wx/spinctrl.h>
#include <list>
#include <vector>
#include <assert.h>
#include "../common.h" // OutputStyle

class IOPane;
class MainPlot;
class AuxPlot;
class FPlot;
class BgManager;
class InputLine;


class OutputWin : public wxTextCtrl
{
public:
    OutputWin (wxWindow *parent, wxWindowID id,
                const wxPoint& pos = wxDefaultPosition, 
                const wxSize& size = wxDefaultSize);
    void append_text (OutputStyle style, const wxString& str);
    void OnRightDown (wxMouseEvent& event);
    void OnPopupColor  (wxCommandEvent& event);       
    void OnPopupFont   (wxCommandEvent& event);  
    void OnPopupClear  (wxCommandEvent& event); 
    void OnKeyDown (wxKeyEvent& event);
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
    void show_fancy_dashes();

private:
    wxColour text_color[4]; 
    wxColour bg_color;

    DECLARE_EVENT_TABLE()
};


/// A pane containing input line and output window.
class IOPane : public wxPanel
{
public:
    IOPane(wxWindow *parent, wxWindowID id=-1);
    void edit_in_input(std::string const& s);
    void OnInputLine(wxString const& s);
    OutputWin *output_win;
    InputLine *input_field;
};


#endif 

