// Purpose: input line with history (wxTextCtrl+wxSpinButton)
// Copyright: (c) 2007 Marcin Wojdyr
// Licence: wxWidgets licence, or (at your option) GPL ver. 2+

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

#include <wx/wx.h>
#include <wx/ffile.h>
#include <stdio.h>  // fgets()

#include "inputline.h"


InputLine::InputLine(wxWindow *parent, wxWindowID id,
                     InputLineObserver* observer, wxString const& hist_file_)
    : wxPanel(parent, id), m_hpos(0), m_observer(observer),
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
#if !wxCHECK_VERSION(2, 9, 0)
# define wxEVT_SPIN wxEVT_SCROLL_THUMBTRACK
#endif
    m_button->Connect(wxEVT_SPIN, wxSpinEventHandler(InputLine::OnSpinButton),
                      NULL, this);
    m_text->Connect(wxEVT_KEY_DOWN,
                    wxKeyEventHandler(InputLine::OnKeyDownAtText),
                    NULL, this);
    m_button->Connect(wxEVT_KEY_DOWN,
                      wxKeyEventHandler(InputLine::OnKeyDownAtSpinButton),
                      NULL, this);
    // read history
    if (!hist_file.IsEmpty()) {
        wxFFile f(hist_file, "r");
        if (f.IsOpened()) {
            char line[512];
            while (fgets(line, 512, f.fp())) {
                wxString s = wxString(line, wxConvUTF8).Trim();
                if (!s.empty())
                    m_history.Add(s);
            }
        }
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
    wxFFile f(hist_file, "w");
    if (!f.IsOpened())
        return;
    for (size_t i = 0; i < m_history.GetCount(); ++i)
        f.Write(m_history[i] + "\n", wxConvUTF8);
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
    m_observer->ProcessInputLine(line);
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

