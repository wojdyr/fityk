// Purpose: input line with history (wxTextCtrl+wxSpinButton)
// Copyright: (c) 2007 Marcin Wojdyr 
// Licence: wxWidgets licence 
// $Id$

/// Input line with history (wxTextCtrl+wxSpinButton). 
/// Supported keybindings:
///      up / Ctrl-p   Move `back' through the history list, 
///                     fetching the previous command.
///    down / Ctrl-n   Move `forward' through the history list, 
///                     fetching the next command.
///      page up       Move to the first line in the history.
///      page down     Move to the end of the input history, 
///                     i.e., the line currently being entered.
///           Ctrl-a   Move to the start of the line.
///           Ctrl-e   Move to the end of the line.
///           Ctrl-k   Cut the text from the current cursor position 
///                     to the end of the line.
///           Ctrl-u   Cut backward from the cursor to the beginning 
///                     of the current line.
///           Ctrl-y   Yank, the same as Ctrl-V
/// This control was originally written for Fityk (http://fityk.sf.net)

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "inputline.h"



class InputLineText : public wxTextCtrl
{
public:
    InputLineText(InputLine *parent, 
                  wxWindowID winid = wxID_ANY,
                  const wxString& value = wxEmptyString,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = 0)
    : wxTextCtrl(parent, winid, value, pos, size, style),
      par(parent) {} 
protected:
    void OnKeyDown (wxKeyEvent& event);
    InputLine *par;

    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------

class InputLineButton : public wxSpinButton
{
public:
    InputLineButton(wxWindow *parent, InputLine *kbd)
        : wxSpinButton(parent, wxID_ANY, 
                       wxDefaultPosition, wxDefaultSize,
                       wxSP_VERTICAL|wxSP_ARROW_KEYS|wxNO_BORDER),
          m_kbd(kbd) {}
protected:
    InputLine *m_kbd;
    void OnKeyDown (wxKeyEvent& event);
    DECLARE_EVENT_TABLE()
};


//-----------------------------------------------------------------------

BEGIN_EVENT_TABLE(InputLineText, wxTextCtrl)
    EVT_KEY_DOWN (InputLineText::OnKeyDown)
END_EVENT_TABLE()

void InputLineText::OnKeyDown (wxKeyEvent& event)
{
    if (event.ControlDown()) {
        switch (event.m_keyCode) {
            case 'A':
                SetInsertionPoint(0);
                return;
            case 'E':
                SetInsertionPointEnd();
                return;
            case 'P':
                par->HistoryMove(-1, true);
                return;
            case 'N':
                par->HistoryMove(+1, true);
                return;
            case 'K':
                SetSelection(GetInsertionPoint(), GetLastPosition());
                Cut();
                return;
            case 'U':
                SetSelection(0, GetInsertionPoint());
                Cut();
                return;
            case 'Y':
                Paste();
                return;
        }
    }
    switch (event.m_keyCode) {
        case WXK_RETURN:
        case WXK_NUMPAD_ENTER:
            {
            wxString s = GetValue().Trim();
            if (!s.IsEmpty())
                par->OnInputLine(s);
            Clear();
            }
            break;
        // to process WXK_TAB, you need to uncomment wxTE_PROCESS_TAB
        //case WXK_TAB:
        //    Navigate();
        //    break;
        case WXK_UP:
        case WXK_NUMPAD_UP:
            par->HistoryMove(-1, true);
            break;
        case WXK_DOWN:
        case WXK_NUMPAD_DOWN:
            par->HistoryMove(+1, true);
            break;
        case WXK_PAGEUP:
        case WXK_NUMPAD_PAGEUP:
            par->HistoryMove(0, false);
            break;
        case WXK_PAGEDOWN:
        case WXK_NUMPAD_PAGEDOWN:
            par->HistoryMove(-1, false);
            break;
        default:
            event.Skip();
    }
}

//-----------------------------------------------------------------------

BEGIN_EVENT_TABLE(InputLineButton, wxSpinButton)
    EVT_KEY_DOWN (InputLineButton::OnKeyDown)
END_EVENT_TABLE()

// redirect key-down event to wxTextCtrl
void InputLineButton::OnKeyDown(wxKeyEvent& event)
{
    m_kbd->RedirectKeyPress(event);
    event.Skip();
}

//-----------------------------------------------------------------------

BEGIN_EVENT_TABLE(InputLine, wxPanel)
    EVT_SPIN(-1, InputLine::OnSpinButton)
END_EVENT_TABLE()

InputLine::InputLine(wxWindow *parent, wxWindowID id, 
                     V1Callback<wxString const&> const& receiver)
    : wxPanel(parent, id), m_hpos(0), m_receiver(receiver)
{
    m_text = new InputLineText(this, wxID_ANY, wxT(""),
                       wxDefaultPosition, wxDefaultSize, 
                       wxWANTS_CHARS|wxTE_PROCESS_ENTER /*|wxTE_PROCESS_TAB*/);
    m_button = new InputLineButton(this, this);
    m_button->SetRange(0, 0); 
    m_button->SetValue(0); 
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_text, 1, wxEXPAND);
    sizer->Add(m_button, 0, wxEXPAND);
    SetSizerAndFit(sizer);
    m_history.Add(wxT(""));
}

void InputLine::RedirectKeyPress(wxKeyEvent& event)
{
    m_text->SetFocus();
    m_text->SetInsertionPointEnd();
    m_text->EmulateKeyPress(event);
}

void InputLine::HistoryMove(int n, bool relative)
{
    int new_pos = relative ? m_hpos + n : n;
    if (!relative && n < 0)
        new_pos += m_history.GetCount();
    if (new_pos == m_hpos || new_pos<0 || new_pos >= (int)m_history.GetCount()) 
        return;

    // save line editing
    m_history[m_hpos] = m_text->GetValue();

    m_hpos = new_pos;
    m_text->SetValue(m_history[new_pos]);
    m_button->SetValue(m_history.GetCount() - 1 - new_pos);
    m_text->SetFocus();
    m_text->SetInsertionPointEnd();
}

void InputLine::OnInputLine(const wxString& line) 
{ 
    m_history.Last() = line;
    m_history.Add(wxT(""));
    if (m_history.GetCount() > 1024+128)
        m_history.RemoveAt(0, 128);
    m_hpos = m_history.GetCount() - 1;
    m_button->SetRange(0, m_hpos);
    m_button->SetValue(0);
    m_receiver(line); 
    m_text->SetFocus();
}


