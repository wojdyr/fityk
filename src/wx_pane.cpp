// This file is part of fityk program. Copyright (C) Marcin Wojdyr

// wxwindows headers, see wxwindows samples for description
#ifdef __GNUG__
#pragma implementation
#endif
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "common.h"
RCSID ("$Id$")

#include <math.h> //round
#include <wx/colordlg.h>
#include <wx/fontdlg.h>
#include "wx_pane.h" 
#include "wx_gui.h" 
#include "wx_plot.h" 
#include "data.h" 
#include "other.h" //view

using namespace std;

enum { 
    ID_COMBO            = 47001,
    ID_OUTPUT_TEXT             ,

    ID_OUTPUT_C_BG             ,
    ID_OUTPUT_C_IN             ,
    ID_OUTPUT_C_OU             ,
    ID_OUTPUT_C_QT             ,
    ID_OUTPUT_C_WR             ,
    ID_OUTPUT_C                ,
    ID_OUTPUT_P_FONT           ,
    ID_OUTPUT_P_CLEAR          
};


//===============================================================
//                            PlotPane
//===============================================================

BEGIN_EVENT_TABLE(PlotPane, ProportionalSplitter)
END_EVENT_TABLE()

PlotPane::PlotPane(wxWindow *parent, wxWindowID id)
    : ProportionalSplitter(parent, id, 0.75),
      plot_shared(), plot(0), diff_plot(0)
{
    plot = new MainPlot(this, plot_shared);
    diff_plot = new DiffPlot(this, plot_shared);
    SplitHorizontally(plot, diff_plot);
}

void PlotPane::zoom_forward()
{
    const int max_length_of_zoom_history = 10;
    zoom_hist.push_back(my_other->view.str());
    if (size(zoom_hist) > max_length_of_zoom_history)
        zoom_hist.erase(zoom_hist.begin());
}

string PlotPane::zoom_backward(int n)
{
    if (n < 1 || zoom_hist.empty()) return "";
    int pos = zoom_hist.size() - n;
    if (pos < 0) pos = 0;
    string val = zoom_hist[pos];
    zoom_hist.erase(zoom_hist.begin() + pos, zoom_hist.end());
    return val;
}

void PlotPane::save_settings(wxConfigBase *cf) const
{
    plot->save_settings(cf);
    diff_plot->save_settings(cf);
    //TODO height
}

void PlotPane::read_settings(wxConfigBase *cf)
{
    plot->read_settings(cf);
    diff_plot->read_settings(cf);
    //TODO height
}

void PlotPane::refresh_plots(bool update)
{
    plot->Refresh(false);
    if (update) plot->Update();
    diff_plot->Refresh(false);
    if (update) diff_plot->Update();
}

void PlotPane::set_mouse_mode(Mouse_mode_enum m) 
{ 
    plot->set_mouse_mode(m); 
}

void PlotPane::update_mouse_hints() 
{ 
    plot->update_mouse_hints();
}

bool PlotPane::is_background_white() 
{ 
    return plot->get_bg_color() == *wxWHITE 
        && diff_plot->get_bg_color() == *wxWHITE;
}


//===============================================================
//                            IOPane
//===============================================================

BEGIN_EVENT_TABLE(IOPane, wxPanel)
END_EVENT_TABLE()

IOPane::IOPane(wxWindow *parent, wxWindowID id)
    : wxPanel(parent, id), output_win(0), input_combo(0)
{
    wxBoxSizer *io_sizer = new wxBoxSizer (wxVERTICAL);

    // wxTextCtrl which displays output of commands
    output_win = new Output_win (this, ID_OUTPUT_TEXT);
    io_sizer->Add (output_win, 1, wxEXPAND);

    // FCombo - wxComboBox used for user keybord input
    //wxString input_choices[] = { /*"help"*/ };
    input_combo = new FCombo (this, ID_COMBO, "",
                               wxDefaultPosition, wxDefaultSize, 
                               0, 0,//input_choices, 
                               wxCB_DROPDOWN|wxWANTS_CHARS|
                               wxTE_PROCESS_ENTER|wxTE_PROCESS_TAB);
    io_sizer->Add (input_combo, 0, wxEXPAND);

    SetAutoLayout (true);
    SetSizer (io_sizer);
    io_sizer->Fit (this);
    io_sizer->SetSizeHints (this);
}

void IOPane::save_settings(wxConfigBase *cf) const
{
    output_win->save_settings(cf);
}

void IOPane::read_settings(wxConfigBase *cf)
{
    output_win->read_settings(cf);
}


//===============================================================
//                            DataPane
//===============================================================

BEGIN_EVENT_TABLE(DataPane, wxPanel)
END_EVENT_TABLE()

DataPane::DataPane(wxWindow *parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(*wxBLUE);
}


//===============================================================
//                            Output_win
//===============================================================

BEGIN_EVENT_TABLE(Output_win, wxTextCtrl)
    EVT_RIGHT_DOWN (                      Output_win::OnRightDown)
    EVT_MENU_RANGE (ID_OUTPUT_C_BG, ID_OUTPUT_C_WR, Output_win::OnPopupColor)
    EVT_MENU       (ID_OUTPUT_P_FONT    , Output_win::OnPopupFont)
    EVT_MENU       (ID_OUTPUT_P_CLEAR   , Output_win::OnPopupClear)
    EVT_KEY_DOWN   (                      Output_win::OnKeyDown)
END_EVENT_TABLE()

Output_win::Output_win (wxWindow *parent, wxWindowID id, 
                        const wxPoint& pos, const wxSize& size)
    : wxTextCtrl(parent, id, "", pos, size,
                 wxTE_MULTILINE|wxTE_RICH|wxNO_BORDER|wxTE_READONLY)
{
    //GetFont().SetFamily(wxMODERN);
    fancy_dashes();
    //SetScrollbar (wxVERTICAL, 0, 5, 50);
}

void Output_win::fancy_dashes() {
    for (int i = 0; i < 16; i++) {
        SetDefaultStyle (wxTextAttr (wxColour(i * 16, i * 16, i * 16)));
        AppendText ("-");
    }
    AppendText ("\n");
}

void Output_win::read_settings(wxConfigBase *cf)
{
    cf->SetPath("/OutputWin/Colors");
    text_color[os_ty_normal] = read_color_from_config(cf, "normal", 
                                                      wxColour(150, 150, 150));
    text_color[os_ty_warn] = read_color_from_config(cf, "warn", 
                                                    wxColour(200, 0, 0));
    text_color[os_ty_quot] = read_color_from_config(cf, "quot", 
                                                    wxColour(50, 50, 255));
    text_color[os_ty_input] = read_color_from_config(cf, "input", 
                                                     wxColour(0, 200, 0));
    bg_color = read_color_from_config(cf, "bg", wxColour(20, 20, 20));

    SetDefaultStyle (wxTextAttr(text_color[os_ty_quot], bg_color));
    if (frame->IsShown()) // this "if" is needed on GTK 1.2 (I don't know why)
        SetBackgroundColour (bg_color);
    Refresh();
}

void Output_win::save_settings(wxConfigBase *cf) const
{
    cf->SetPath("/OutputWin/Colors");
    write_color_to_config (cf, "normal", text_color[os_ty_normal]);  
    write_color_to_config (cf, "warn", text_color[os_ty_warn]); 
    write_color_to_config (cf, "quot", text_color[os_ty_quot]); 
    write_color_to_config (cf, "input", text_color[os_ty_input]); 
    write_color_to_config (cf, "bg", bg_color); 
}

void Output_win::append_text (const wxString& str, Output_style_enum style)
{
    SetDefaultStyle (wxTextAttr (text_color[style]));
    AppendText (str);
}

void Output_win::OnPopupColor (wxCommandEvent& event)
{
    int n = event.GetId();
    wxColour *col;
    if (n == ID_OUTPUT_C_BG)
        col = &bg_color;
    else if (n == ID_OUTPUT_C_OU)
        col = &text_color[os_ty_normal];
    else if (n == ID_OUTPUT_C_QT)
        col = &text_color[os_ty_quot];
    else if (n == ID_OUTPUT_C_WR)
        col = &text_color[os_ty_warn];
    else if (n == ID_OUTPUT_C_IN)
        col = &text_color[os_ty_input];
    else 
        return;
    wxColourData col_data;
    col_data.SetCustomColour (0, *col);
    col_data.SetColour (*col);
    wxColourDialog dialog (this, &col_data);
    if (dialog.ShowModal() == wxID_OK) {
        *col = dialog.GetColourData().GetColour();
        SetBackgroundColour (bg_color);
        SetDefaultStyle (wxTextAttr(wxNullColour, bg_color));
        Refresh();
    }
}

void Output_win::OnPopupFont (wxCommandEvent& WXUNUSED(event))
{
    wxFontData data; 
    data.SetInitialFont (GetDefaultStyle().GetFont());
    wxFontDialog dlg (this, &data);
    int r = dlg.ShowModal();
    if (r == wxID_OK) {
        wxFont f = dlg.GetFontData().GetChosenFont();
        SetDefaultStyle (wxTextAttr (wxNullColour, wxNullColour, f));
        Refresh();
    }
}

void Output_win::OnPopupClear (wxCommandEvent& WXUNUSED(event))
{
    Clear();
    fancy_dashes();
}

    
void Output_win::OnRightDown (wxMouseEvent& event)
{
    wxMenu popup_menu ("output text menu");

    wxMenu *color_menu = new wxMenu;
    color_menu->Append (ID_OUTPUT_C_BG, "&Background");
    color_menu->Append (ID_OUTPUT_C_IN, "&Input");
    color_menu->Append (ID_OUTPUT_C_OU, "&Output");
    color_menu->Append (ID_OUTPUT_C_QT, "&Quotation");
    color_menu->Append (ID_OUTPUT_C_WR, "&Warning");
    popup_menu.Append  (ID_OUTPUT_C   , "&Color", color_menu);

    popup_menu.Append  (ID_OUTPUT_P_FONT, "&Font");
    popup_menu.Append  (ID_OUTPUT_P_CLEAR, "Clea&r");

    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void Output_win::OnKeyDown (wxKeyEvent& event)
{
    if (event.GetKeyCode() == ' ' || event.GetKeyCode() == WXK_TAB) {
        IOPane *parent = static_cast<IOPane*>(GetParent()); //to not use RTTI
        parent->focus_input();
    }
    else
        event.Skip();
}

//===============================================================
//                            combo
//===============================================================

BEGIN_EVENT_TABLE(FCombo, wxComboBox)
    EVT_KEY_DOWN (FCombo::OnKeyDown)
END_EVENT_TABLE()

void FCombo::OnKeyDown (wxKeyEvent& event)
{
    const int n_list_items = 15;
    if (event.m_keyCode == WXK_RETURN || event.m_keyCode == WXK_NUMPAD_ENTER) {
        wxString s = GetValue().Trim();
        if (s.IsEmpty())
            return;
        frame->SetStatusText (s);
        // changing drop-down list
        vector<wxString> list;
        list.push_back(s);
        int n = std::min (n_list_items, GetCount() + 1);
        for (int i = 0; i < n - 1; i++)
            list.push_back(GetString(i));
        Clear();
        for (vector<wxString>::iterator i = list.begin(); i != list.end(); i++)
            Append (*i);
        SetValue("");

        //displaying and executing command
        exec_command (s);
    }
    else if (event.m_keyCode == WXK_TAB) {
        IOPane *parent = static_cast<IOPane*>(GetParent()); //to not use RTTI
        parent->focus_output();
    }
    else
        event.Skip();
}

//===============================================================
//                            FPrintout
//===============================================================

FPrintout::FPrintout(const PlotPane *p_pane) 
    : wxPrintout(my_data->get_filename().c_str()), pane(p_pane) 
{}

bool FPrintout::OnPrintPage(int page)
{
    if (page != 1) return false;
    if (my_data->is_empty()) return false;
    wxDC *dc = GetDC();
    if (!dc) return false;

    // Set the scale and origin
    int w, h;
    int space = 20;
    int maxX = pane->plot->GetClientSize().GetWidth();
    int maxY = pane->plot->GetClientSize().GetHeight();
    int maxYsum = maxY + space + pane->diff_plot->GetClientSize().GetHeight();
    int marginX = 50, marginY = 50;
    dc->GetSize(&w, &h);
    fp scaleX = static_cast<fp>(w) / (maxX + 2 * marginX);
    fp scaleY = static_cast<fp>(h) / (maxYsum + 2 * marginY);
    fp actualScale = min (scaleX, scaleY);
    int posX = static_cast<int>((w - maxX * actualScale) / 2. + 0.5);
    int posY = static_cast<int>((h - maxYsum * actualScale) / 2. + 0.5);
    dc->SetUserScale (actualScale, actualScale);
    dc->SetDeviceOrigin (posX, posY);
    
    pane->plot->Draw(*dc);//printing main plot  
    posY += static_cast<int>((maxY + space) * actualScale);   
    dc->SetDeviceOrigin (posX, posY);
    pane->diff_plot->Draw(*dc); //printing auxiliary plot  
    return true;
}


//===============================================================

wxColour read_color_from_config(const wxConfigBase *config, const wxString& key,
                                 const wxColour& default_value)
{
    return wxColour (config->Read (key + "/Red", default_value.Red()), 
                     config->Read (key + "/Green", default_value.Green()), 
                     config->Read (key + "/Blue", default_value.Blue()));
}

void write_color_to_config (wxConfigBase *config, const wxString& key,
                            const wxColour& value)
{
    config->Write (key + "/Red", value.Red());
    config->Write (key + "/Green", value.Green());
    config->Write (key + "/Blue", value.Blue());
}


//===============================================================
//                            ProportionalSplitter
//===============================================================

ProportionalSplitter::ProportionalSplitter(wxWindow* parent, wxWindowID id, 
                                           float proportion, const wxSize& size,
                                           long style, const wxString& name) 
    : wxSplitterWindow(parent, id, wxDefaultPosition, size, style, name),
      m_proportion(proportion), m_firstpaint(true)
{
    wxASSERT(m_proportion >= 0. && m_proportion <= 1.);
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
    int h = int(round(height * m_proportion));
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
    int w = int(round(width * m_proportion));
    if (w < GetMinimumPaneSize() || w > width-GetMinimumPaneSize())
        w = 0;
    return wxSplitterWindow::SplitVertically(win1, win2, w);
}

int ProportionalSplitter::GetExpectedSashPosition()
{
    return int(round(GetWindowSize() * m_proportion));
}

void ProportionalSplitter::ResetSash()
{
    SetSashPosition(GetExpectedSashPosition());
}

void ProportionalSplitter::OnReSize(wxSizeEvent& event)
{
    // We may need to adjust the sash based on m_proportion.
    ResetSash();
    event.Skip();
}

void ProportionalSplitter::OnSashChanged(wxSplitterEvent &event)
{
    // We'll change m_proportion now based on where user dragged the sash.
    const wxSize& s = GetSize();
    int t = GetSplitMode() == wxSPLIT_HORIZONTAL ? s.GetHeight() : s.GetWidth();
    m_proportion = float(GetSashPosition()) / t;
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

