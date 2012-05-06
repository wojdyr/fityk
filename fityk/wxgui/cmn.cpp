// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <ctype.h>
#include <wx/wx.h>
#include <wx/config.h>
#include <wx/colordlg.h>
#include <wx/statline.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>

#include "cmn.h"

using namespace std;

namespace {
/// Round real to integer. Defined here to avoid dependency on ../common.h.
int iround(double d) { return static_cast<int>(floor(d+0.5)); }
}

bool cfg_read_bool(wxConfigBase *cf, const wxString& key, bool def_val)
{
    bool b;
    cf->Read(key, &b, def_val);
    return b;
}

double cfg_read_double(wxConfigBase *cf, const wxString& key, double def_val)
{
    double d;
    cf->Read(key, &d, def_val);
    return d;
}

// the storing of color values was changed in 0.9.7
//   before:
//     config->Write(key + wxT("/Red"), (int) value.Red());
//     config->Write(key + wxT("/Green"), (int) value.Green());
//     config->Write(key + wxT("/Blue"), (int) value.Blue());
//   after:
//     config->Write(key + wxT("_col"), value.GetAsString(wxC2S_CSS_SYNTAX));

wxColour cfg_read_color(const wxConfigBase *config, const wxString& key,
                        const wxColour& default_value)
{
    wxString val = config->Read(key + wxT("_col"), wxEmptyString);
    if (val.empty() && config->Exists(key + wxT("/Red")))
        return wxColour(
              config->Read (key + wxT("/Red"), (int) default_value.Red()),
              config->Read (key + wxT("/Green"), (int) default_value.Green()),
              config->Read (key + wxT("/Blue"), (int) default_value.Blue()));
    return val.empty() ? default_value : wxColour(val);
}

void cfg_write_color(wxConfigBase *config, const wxString& key,
                           const wxColour& value)
{
    config->Write(key + wxT("_col"), value.GetAsString(wxC2S_CSS_SYNTAX));
    if (config->Exists(key + wxT("/Red")))
        config->DeleteGroup(key);
}

wxFont cfg_read_font(wxConfigBase const *config, wxString const& key,
                             wxFont const &default_value)
{
    wxString str = config->Read(key, wxEmptyString);
    if (str.empty())
        return default_value;
    wxFont font;
    bool ok = font.SetNativeFontInfo(str);
    return ok ? font : default_value;
}

void cfg_write_font (wxConfigBase *config, const wxString& key,
                           const wxFont& value)
{
    config->Write(key, value.IsOk() ? value.GetNativeFontInfoDesc()
                                    : wxString());
}

bool change_color_dlg(wxColour& col)
{
    wxColourData col_data;
    col_data.SetCustomColour(0, col);
    col_data.SetColour(col);
    wxColourDialog dialog(0, &col_data);
    bool ok = (dialog.ShowModal() == wxID_OK);
    if (ok)
        col = dialog.GetColourData().GetColour();
    return ok;
}

void add_apply_close_buttons(wxWindow *parent, wxSizer *top_sizer)
{
    top_sizer->Add(new wxStaticLine(parent, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    wxBoxSizer *s = new wxBoxSizer(wxHORIZONTAL);
    s->Add(new wxButton(parent, wxID_APPLY, wxT("&Apply")), 0, wxALL, 5);
    s->Add(new wxButton(parent, wxID_CLOSE, wxT("&Close")), 0, wxALL, 5);
    top_sizer->Add(s, 0, wxALL|wxALIGN_CENTER, 0);
}

//===============================================================
//                            ProportionalSplitter
//===============================================================

ProportionalSplitter::ProportionalSplitter(wxWindow* parent, wxWindowID id,
                                           float proportion, const wxSize& size,
                                           long style)
    : wxSplitterWindow(parent, id, wxDefaultPosition, size, style),
      m_proportion(proportion), m_firstpaint(true)
{
    //wxASSERT(m_proportion >= 0. && m_proportion <= 1.);
    SetMinimumPaneSize(20);
    ResetSash();
    Connect(GetId(), wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED,
                (wxObjectEventFunction) &ProportionalSplitter::OnSashChanged);
    Connect(GetId(), wxEVT_SIZE,
                     (wxObjectEventFunction) &ProportionalSplitter::OnReSize);
    //hack to set sizes on first paint event
    Connect(GetId(), wxEVT_PAINT,
                      (wxObjectEventFunction) &ProportionalSplitter::OnPaint);
}

bool ProportionalSplitter::SplitHorizontally(wxWindow* win1, wxWindow* win2,
                                             float proportion)
{
    if (proportion >= 0. && proportion <= 1.)
        m_proportion = proportion;
    int height = GetClientSize().GetHeight();
    int h = iround(height * m_proportion);
    //sometimes there is a strange problem without it (why?)
    if (h < GetMinimumPaneSize() || h > height-GetMinimumPaneSize())
        h = 0;
    return wxSplitterWindow::SplitHorizontally(win1, win2, h);
}

bool ProportionalSplitter::SplitVertically(wxWindow* win1, wxWindow* win2,
                                           float proportion)
{
    if (proportion >= 0. && proportion <= 1.)
        m_proportion = proportion;
    int width = GetClientSize().GetWidth();
    int w = iround(width * m_proportion);
    if (w < GetMinimumPaneSize() || w > width-GetMinimumPaneSize())
        w = 0;
    return wxSplitterWindow::SplitVertically(win1, win2, w);
}

int ProportionalSplitter::GetExpectedSashPosition()
{
    return iround(GetWindowSize() * m_proportion);
}

void ProportionalSplitter::SetSashPosition(int position)
{
    if (position < 0 || position > GetWindowSize())
        return;
    m_proportion = float(position) / GetWindowSize();
    wxSplitterWindow::SetSashPosition(position);
}

void ProportionalSplitter::ResetSash()
{
    wxSplitterWindow::SetSashPosition(GetExpectedSashPosition());
}

void ProportionalSplitter::OnReSize(wxSizeEvent& event)
{
    // We may need to adjust the sash based on m_proportion.
    ResetSash();
    event.Skip();
}

void ProportionalSplitter::OnSashChanged(wxSplitterEvent &event)
{
    // This event is triggered on start-up in some cases,
    // eg. on wxGTK, in a setup with h-splitter containing two v-splitters,
    // if the inner splitters are split first the lower v-splitter gets it.
    if (!m_firstpaint) {
        event.Skip();
        return;
    }

    // We'll change m_proportion now based on where user dragged the sash.
    const wxSize& s = GetSize();
    int t = GetSplitMode() == wxSPLIT_HORIZONTAL ? s.GetHeight() : s.GetWidth();
    float prop = float(event.GetSashPosition()) / t;
    if (prop > 0. && prop < 1.)
        m_proportion = prop;
    event.Skip();
}

void ProportionalSplitter::OnPaint(wxPaintEvent &event)
{
    if (m_firstpaint) {
        if (GetSashPosition() != GetExpectedSashPosition())
            ResetSash();
        m_firstpaint = false;
    }
    event.Skip();
}



//                            KFTextCtrl
void KFTextCtrl::OnKillFocus(wxFocusEvent& event)
{
    wxCommandEvent ev(wxEVT_COMMAND_TEXT_ENTER, GetId());
    ev.SetEventObject(this);
    ProcessEvent(ev);
    event.Skip();
}

BEGIN_EVENT_TABLE(KFTextCtrl, wxTextCtrl)
    EVT_KILL_FOCUS(KFTextCtrl::OnKillFocus)
END_EVENT_TABLE()


/// All configuration is stored in files in directory:
///    Unix: ~/.fityk
/// Windows: C:\Documents and Settings\username\Application Data\fityk
///     Mac: ~/Library/Application Support/fityk
wxString get_conf_file(string const& filename)
{
    return wxStandardPaths::Get().GetUserDataDir()
                                   + wxFILE_SEP_PATH + s2wx(filename);
}


bool should_focus_input(wxKeyEvent& event)
{
    if (event.AltDown() || event.ControlDown() || event.CmdDown())
        return false;
    int c = event.GetKeyCode();
    return c < 128 && (c == ' ' || isalnum(c) || ispunct(c));
}


void updateControlWithItems(wxControlWithItems *cwi, vector<string> const& v)
{
    if (v.size() != (size_t) cwi->GetCount()) {
        cwi->Clear();
        for (size_t i = 0; i < v.size(); ++i)
            cwi->Append(s2wx(v[i]));
    }
    else
        for (size_t i = 0; i < v.size(); ++i)
            if (cwi->GetString(i) != s2wx(v[i]))
                cwi->SetString(i, s2wx(v[i]));
}


SmallStaticText::SmallStaticText(wxWindow* parent, const wxString& label)
    : wxStaticText(parent, wxID_ANY, label)
{
    wxFont font = GetFont();
#ifdef __WXMSW__
    font.SetPointSize(font.GetPointSize() - 1);
#else
    font.SetPointSize(font.GetPointSize() - 2);
#endif
    SetFont(font);
}


DialogCloser* DialogCloser::instance_ = NULL;

DialogCloser* DialogCloser::instance()
{
    if (instance_ == NULL)
        instance_ = new DialogCloser();
    return instance_;
}
void DialogCloser::OnClose(wxCommandEvent& event)
{
    wxWindow* w = (wxWindow*) event.GetEventObject();
    while (!w->IsTopLevel())
        w = w->GetParent();
    wxDialog *dialog = wxDynamicCast(w, wxDialog);
    assert(dialog->IsModal());
    dialog->EndModal(wxID_CANCEL);
}

