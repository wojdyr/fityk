// Purpose: input line with history (wxTextCtrl+wxSpinButton)
// Copyright: (c) 2007 Marcin Wojdyr 
// Licence: wxWidgets licence 
// $Id:$
//

#ifndef FITYK__WX_INPUTLINE__H__
#define FITYK__WX_INPUTLINE__H__

#include <wx/spinctrl.h>
#include "callback.h"

class InputLineText;
class InputLineButton;

class InputLineCallee
{
public:
    virtual ~InputLineCallee() {} //virtual dtor to avoid compiler warnings
    virtual void OnInputLine(wxString const& line) = 0;
};

class InputLine : public wxPanel
{
public:
    InputLine(wxWindow *parent, wxWindowID winid, 
              V1Callback<wxString const&> const& receiver);
    void SetValue(const wxString& value);
    void OnInputLine(const wxString& line);
    void HistoryMove(int n, bool relative);
protected:
    InputLineText *m_text;
    InputLineButton *m_button;
    wxArrayString m_history;
    int m_hpos; // current item in m_history
    V1Callback<wxString const&> m_receiver;

    void OnSpinButtonUp(wxSpinEvent &) { HistoryMove(-1, true); }
    void OnSpinButtonDown(wxSpinEvent &) { HistoryMove(+1, true); }

    DECLARE_EVENT_TABLE()
};

#endif
