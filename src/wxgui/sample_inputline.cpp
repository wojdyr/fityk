// InputLine sample, by Marcin Wojdyr, public domain

#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "inputline.h"
#include "callback.h"

class Frame : public wxFrame
{
public:
    Frame();
    void AddLine(wxString const& s) { m_output->AppendText(s + wxT("\n")); }
private:
    InputLine* m_input;
    wxTextCtrl* m_output;
};

class App : public wxApp
{
public:
    bool OnInit()
    {
        wxApp::OnInit();
        Frame *frame = new Frame;
        frame->Show(true);
        return true;
    }
};

IMPLEMENT_APP(App)

Frame::Frame() : wxFrame(0, wxID_ANY, wxT("InputLine sample"))
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    m_input = new InputLine(this, wxID_ANY, 
                 make_callback<wxString const&>().V1(this, &Frame::AddLine));
    m_output = new wxTextCtrl(this, wxID_ANY, wxT(""), 
                              wxDefaultPosition, wxDefaultSize,
                              wxTE_MULTILINE|wxTE_READONLY);
    sizer->Add(m_input, 0, wxEXPAND);
    sizer->Add(m_output, 1, wxEXPAND);
    SetSizerAndFit(sizer);
    m_input->SetFocus();
}

