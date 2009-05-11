// Purpose: input line with history (wxTextCtrl+wxSpinButton)
// Copyright: (c) 2007 Marcin Wojdyr
// Licence: wxWidgets licence, or (at your option) GPL
// $Id$
//

#ifndef FITYK__WX_INPUTLINE__H__
#define FITYK__WX_INPUTLINE__H__

#include <wx/spinctrl.h>
#include "callback.h"

class InputLine : public wxPanel
{
public:
    /// receiver will be called when Enter is pressed
    InputLine(wxWindow *parent, wxWindowID winid,
              V1Callback<wxString const&> const& receiver,
              wxString const& hist_file_);
    ~InputLine();
    /// intended for use in EVT_KEY_DOWN handlers of other controls,
    /// to change focus and redirect keyboard input to text input
    void RedirectKeyPress(wxKeyEvent& event);
    void SetValue(const wxString& value) { m_text->SetValue(value); }
protected:
    wxTextCtrl *m_text;
    wxSpinButton *m_button;
    wxArrayString m_history;
    int m_hpos; // current item in m_history
    V1Callback<wxString const&> m_receiver;
    wxString const hist_file;

    void OnSpinButton(wxSpinEvent &event)
        { HistoryMove(m_history.GetCount() - 1 - event.GetPosition(), false); }
    void OnInputLine(const wxString& line);
    void HistoryMove(int n, bool relative);
    void GoToHistoryEnd();
    void OnKeyDownAtText (wxKeyEvent& event);
    void OnKeyDownAtSpinButton (wxKeyEvent& event);
    wxSize DoGetBestSize() const;

    DECLARE_EVENT_TABLE()
};

#endif
