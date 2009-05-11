// Purpose: input line with history (wxTextCtrl+wxSpinButton)
// Copyright: (c) 2007 Marcin Wojdyr
// Licence: wxWidgets licence, or (at your option) GPL
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

#include <fstream>

#include "inputline.h"


BEGIN_EVENT_TABLE(InputLine, wxPanel)
    EVT_SPIN(-1, InputLine::OnSpinButton)
    //KEY_DOWN events from wxTextCtrl and wxSpinButtonare Connect()-ed
END_EVENT_TABLE()

InputLine::InputLine(wxWindow *parent, wxWindowID id,
                     V1Callback<wxString const&> const& receiver,
                     wxString const& hist_file_)
    : wxPanel(parent, id), m_hpos(0), m_receiver(receiver),
      hist_file(hist_file_)
{
    m_text = new wxTextCtrl(this, wxID_ANY, wxT(""),
                       wxDefaultPosition, wxDefaultSize,
                       wxWANTS_CHARS|wxTE_PROCESS_ENTER /*|wxTE_PROCESS_TAB*/);
    m_button = new wxSpinButton(this, wxID_ANY,
                                wxDefaultPosition, wxDefaultSize,
                                wxSP_VERTICAL|wxSP_ARROW_KEYS|wxNO_BORDER),
    m_button->SetRange(0, 0);
    m_button->SetValue(0);
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_text, 1, wxEXPAND);
    sizer->Add(m_button, 0, wxEXPAND);
    SetSizer(sizer);
    SetMinSize(wxSize(-1, m_text->GetBestSize().y));
    m_text->Connect(wxID_ANY, wxEVT_KEY_DOWN,
                    wxKeyEventHandler(InputLine::OnKeyDownAtText),
                    0, this);
    m_button->Connect(wxID_ANY, wxEVT_KEY_DOWN,
                      wxKeyEventHandler(InputLine::OnKeyDownAtSpinButton),
                      0, this);
    // read history
    if (!hist_file.IsEmpty()) {
        std::ifstream f(hist_file.fn_str());
        char line[512];
        while (f.getline(line, 512))
            m_history.Add(wxString(line, wxConvUTF8));
    }
    // add empty line that will be displayed initially
    m_history.Add(wxT(""));
    GoToHistoryEnd();
}

InputLine::~InputLine()
{
    // write history
    if (hist_file.IsEmpty())
        return;
    std::ofstream f(hist_file.fn_str());
    if (!f)
        return;
    for (size_t i = 0; i < m_history.GetCount(); ++i) {
        if (i > 0)
            f << std::endl;
        f << m_history[i].fn_str();
    }
}

wxSize InputLine::DoGetBestSize() const
{
    return wxSize(wxPanel::DoGetBestSize().x, m_text->GetMinSize().y);
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
    GoToHistoryEnd();
    m_receiver(line);
    m_text->SetFocus();
}

void InputLine::GoToHistoryEnd()
{
    const int hist_size = 1024;
    const int hist_chunk = 128;
    if (m_history.GetCount() > hist_size + hist_chunk)
        m_history.RemoveAt(0, m_history.GetCount() - hist_size);
    m_hpos = m_history.GetCount() - 1;
    m_button->SetRange(0, m_hpos);
    m_button->SetValue(0);
}

void InputLine::OnKeyDownAtText (wxKeyEvent& event)
{
    if (event.ControlDown()) {
        switch (event.m_keyCode) {
            case 'A':
                m_text->SetInsertionPoint(0);
                return;
            case 'E':
                m_text->SetInsertionPointEnd();
                return;
            case 'P':
                HistoryMove(-1, true);
                return;
            case 'N':
                HistoryMove(+1, true);
                return;
            case 'K':
                m_text->SetSelection(m_text->GetInsertionPoint(),
                                     m_text->GetLastPosition());
                m_text->Cut();
                return;
            case 'U':
                m_text->SetSelection(0, m_text->GetInsertionPoint());
                m_text->Cut();
                return;
            case 'Y':
                m_text->Paste();
                return;
        }
    }
    switch (event.m_keyCode) {
        case WXK_RETURN:
        case WXK_NUMPAD_ENTER:
            {
            wxString s = m_text->GetValue().Trim();
            if (!s.IsEmpty())
                OnInputLine(s);
            m_text->Clear();
            }
            break;
        // to process WXK_TAB, you need to uncomment wxTE_PROCESS_TAB
        //case WXK_TAB:
        //    Navigate();
        //    break;
        case WXK_UP:
        case WXK_NUMPAD_UP:
            HistoryMove(-1, true);
            break;
        case WXK_DOWN:
        case WXK_NUMPAD_DOWN:
            HistoryMove(+1, true);
            break;
        case WXK_PAGEUP:
        case WXK_NUMPAD_PAGEUP:
            HistoryMove(0, false);
            break;
        case WXK_PAGEDOWN:
        case WXK_NUMPAD_PAGEDOWN:
            HistoryMove(-1, false);
            break;
        default:
            event.Skip();
    }
}

void InputLine::OnKeyDownAtSpinButton(wxKeyEvent& event)
{
    RedirectKeyPress(event);
    event.Skip();
}

