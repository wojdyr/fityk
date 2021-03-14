// Purpose: input line with history (wxTextCtrl+wxSpinButton)
// Copyright: (c) 2007 Marcin Wojdyr
// Licence: wxWidgets licence, or (at your option) GPL ver. 2+

#ifndef FITYK_WX_INPUTLINE_H_
#define FITYK_WX_INPUTLINE_H_

#include <wx/spinctrl.h>

class InputLineObserver
{
public:
    virtual ~InputLineObserver() {}
    virtual void ProcessInputLine(const wxString& line) = 0;
};

class InputLine : public wxPanel
{
public:
    /// receiver will be called when Enter is pressed
    InputLine(wxWindow *parent, wxWindowID winid,
              InputLineObserver* observer, wxString const& hist_file_);
    ~InputLine();
    /// intended for use in EVT_KEY_DOWN handlers of other controls,
    /// to change focus and redirect keyboard input to text input
    void RedirectKeyPress(wxKeyEvent& event);
    void SetValue(const wxString& value) { m_text->SetValue(value); }
protected:
    wxTextCtrl *m_text;
#ifndef __WXGTK3__
    wxSpinButton *m_button;
#endif
    wxArrayString m_history;
    int m_hpos; // current item in m_history
    InputLineObserver* m_observer;
    wxString const hist_file;

    void OnSpinButton(wxSpinEvent &event)
        { HistoryMove(m_history.GetCount() - 1 - event.GetPosition(), false); }
    void OnInputLine(const wxString& line);
    void HistoryMove(int n, bool relative);
    void GoToHistoryEnd();
    void OnKeyDownAtText (wxKeyEvent& event);
    void OnKeyDownAtSpinButton (wxKeyEvent& event);
    wxSize DoGetBestSize() const;
};

#endif // FITYK_WX_INPUTLINE_H_
