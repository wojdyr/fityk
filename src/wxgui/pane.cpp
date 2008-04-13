// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/fontdlg.h>
#include <wx/treectrl.h>
#include <wx/textdlg.h>
#include <wx/image.h>

#include "pane.h" 
#include "frame.h" 
#include "plot.h" 
#include "mplot.h" 
#include "aplot.h" 
#include "inputline.h" 
#include "../common.h"
#include "../logic.h" 
#include "../ui.h" 

using namespace std;

enum { 
    ID_OUTPUT_C_BG      = 27001,
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
      crosshair_cursor(false)
{
    plot = new MainPlot(this);
    aux_split = new ProportionalSplitter(this, -1, 0.5);
    SplitHorizontally(plot, aux_split);

    aux_plot[0] = new AuxPlot(aux_split, plot, wxT("0"));
    aux_plot[1] = new AuxPlot(aux_split, plot, wxT("1"));
    aux_plot[1]->Show(false);
    aux_split->Initialize(aux_plot[0]);
}

void PlotPane::zoom_forward()
{
    const int max_length_of_zoom_history = 10;
    zoom_hist.push_back(ftk->view.str());
    if (size(zoom_hist) > max_length_of_zoom_history)
        zoom_hist.erase(zoom_hist.begin());
}

string PlotPane::zoom_backward(int n)
{
    if (n < 1 || zoom_hist.empty()) 
        return "";
    int pos = zoom_hist.size() - n;
    if (pos < 0) pos = 0;
    string val = zoom_hist[pos];
    zoom_hist.erase(zoom_hist.begin() + pos, zoom_hist.end());
    return val;
}

void PlotPane::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/PlotPane"));
    cf->Write(wxT("PlotPaneProportion"), GetProportion());
    cf->Write(wxT("AuxPlotsProportion"), aux_split->GetProportion());
    cf->Write(wxT("ShowAuxPane0"), aux_visible(0));
    cf->Write(wxT("ShowAuxPane1"), aux_visible(1));
    plot->save_settings(cf);
    for (int i = 0; i < 2; ++i)
        aux_plot[i]->save_settings(cf);
}

void PlotPane::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/PlotPane"));
    SetProportion(cfg_read_double(cf, wxT("PlotPaneProportion"), 0.75));
    aux_split->SetProportion(cfg_read_double(cf, wxT("AuxPlotsProportion"), 
                                                         0.5));
    show_aux(0, cfg_read_bool(cf, wxT("ShowAuxPane0"), true));
    show_aux(1, cfg_read_bool(cf, wxT("ShowAuxPane1"), false));
    plot->read_settings(cf);
    for (int i = 0; i < 2; ++i)
        aux_plot[i]->read_settings(cf);
}

void PlotPane::refresh_plots(bool now, bool only_main)
{
    if (only_main) {
        plot->refresh(now);
        return;
    }
    // not only main
    vector<FPlot*> vp = get_visible_plots();
    for (vector<FPlot*>::const_iterator i = vp.begin(); i != vp.end(); ++i) 
        (*i)->refresh(now);
}

void PlotPane::set_mouse_mode(MouseModeEnum m) 
{ 
    plot->set_mouse_mode(m); 
}

void PlotPane::update_mouse_hints() 
{ 
    plot->update_mouse_hints();
}

bool PlotPane::is_background_white() 
{ 
    //have all visible plots white background?
    vector<FPlot*> vp = get_visible_plots();
    for (vector<FPlot*>::const_iterator i = vp.begin(); i != vp.end(); ++i) 
        if ((*i)->get_bg_color() != *wxWHITE)
            return false;
    return true;
}

BgManager* PlotPane::get_bg_manager() { return &plot->bgm; }

const std::vector<FPlot*> PlotPane::get_visible_plots() const
{
    vector<FPlot*> visible;
    visible.push_back(plot);
    for (int i = 0; i < 2; ++i)
        if (aux_visible(i))
            visible.push_back(aux_plot[i]);
    return visible;
}

bool PlotPane::aux_visible(int n) const
{
    return IsSplit() && (aux_split->GetWindow1() == aux_plot[n]
                         || aux_split->GetWindow2() == aux_plot[n]);
}

void PlotPane::show_aux(int n, bool show)
{
    if (aux_visible(n) == show) return;

    if (show) {
        if (!IsSplit()) { //both where invisible
            SplitHorizontally(plot, aux_split);
            aux_split->Show(true);
            assert(!aux_split->IsSplit());
            if (aux_split->GetWindow1() == aux_plot[n])
                ;
            else {
                aux_split->SplitHorizontally(aux_plot[0], aux_plot[1]);
                aux_plot[n]->Show(true);
                aux_split->Unsplit(aux_plot[n==0 ? 1 : 0]);
            }
        }
        else {//one was invisible
            aux_split->SplitHorizontally(aux_plot[0], aux_plot[1]);
            aux_plot[n]->Show(true);
        }
    }
    else { //hide
        if (aux_split->IsSplit()) //both where visible
            aux_split->Unsplit(aux_plot[n]);
        else // only one was visible
            Unsplit(); //hide whole aux_split
    }
}


/// draw "crosshair cursor" -> erase old and draw new
void PlotPane::draw_crosshair(int X, int Y)
{
    static bool drawn = false;
    static int oldX = 0, oldY = 0;
    if (drawn) {
        do_draw_crosshair(oldX, oldY);
        drawn = false;
    }
    if (crosshair_cursor && X >= 0) {
        do_draw_crosshair(X, Y);
        oldX = X;
        oldY = Y;
        drawn = true;
    }
}

void PlotPane::do_draw_crosshair(int X, int Y)
{
    plot->draw_crosshair(X, Y);
    for (int i = 0; i < 2; ++i)
        if (aux_visible(i))
            aux_plot[i]->draw_crosshair(X, -1);
}

//===============================================================
//                            IOPane
//===============================================================

IOPane::IOPane(wxWindow *parent, wxWindowID id)
    : wxPanel(parent, id)
{
    wxBoxSizer *io_sizer = new wxBoxSizer (wxVERTICAL);

    // on Linux platform GUI and CLI history is stored in the same file
    wxString hist_file = get_user_conffile("history");
    input_field = new InputLine(this, -1, 
              make_callback<wxString const&>().V1(this, &IOPane::OnInputLine),
              hist_file);
    output_win = new OutputWin (this, -1);
    io_sizer->Add (output_win, 1, wxEXPAND);
    io_sizer->Add (input_field, 0, wxEXPAND);

    SetSizer(io_sizer);
    io_sizer->SetSizeHints(this);
}

void IOPane::edit_in_input(string const& s) 
{
    input_field->SetValue(s2wx(s));
    input_field->SetFocus(); 
}

void IOPane::OnInputLine(wxString const& s)
{
    frame->set_status_text(wx2s(s));
    ftk->exec(wx2s(s)); //displaying and executing command
}

//===============================================================
//                            OutputWin
//===============================================================

BEGIN_EVENT_TABLE(OutputWin, wxTextCtrl)
    EVT_RIGHT_DOWN (                      OutputWin::OnRightDown)
    EVT_MENU_RANGE (ID_OUTPUT_C_BG, ID_OUTPUT_C_WR, OutputWin::OnPopupColor)
    EVT_MENU       (ID_OUTPUT_P_FONT    , OutputWin::OnPopupFont)
    EVT_MENU       (ID_OUTPUT_P_CLEAR   , OutputWin::OnPopupClear)
    EVT_KEY_DOWN   (                      OutputWin::OnKeyDown)
END_EVENT_TABLE()

OutputWin::OutputWin (wxWindow *parent, wxWindowID id, 
                      const wxPoint& pos, const wxSize& size)
    : wxTextCtrl(parent, id, wxT(""), pos, size,
                 wxTE_MULTILINE|wxTE_RICH|wxNO_BORDER|wxTE_READONLY)
{}

void OutputWin::show_fancy_dashes() {
    for (int i = 0; i < 16; i++) {
        SetDefaultStyle (wxTextAttr (wxColour(i * 16, i * 16, i * 16)));
        AppendText (wxT("-"));
    }
    AppendText (wxT("\n"));
}

void OutputWin::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/OutputWin/Colors"));
    text_color[os_normal] = cfg_read_color(cf, wxT("normal"), 
                                                      wxColour(150, 150, 150));
    text_color[os_warn] = cfg_read_color(cf, wxT("warn"), 
                                                    wxColour(200, 0, 0));
    text_color[os_quot] = cfg_read_color(cf, wxT("quot"), 
                                                    wxColour(50, 50, 255));
    text_color[os_input] = cfg_read_color(cf, wxT("input"), 
                                                     wxColour(0, 200, 0));
    bg_color = cfg_read_color(cf, wxT("bg"), wxColour(20, 20, 20));
    cf->SetPath(wxT("/OutputWin"));
    wxFont font = cfg_read_font(cf, wxT("font"), wxNullFont);
    SetDefaultStyle (wxTextAttr(text_color[os_quot], bg_color, font));

    // this "if" is needed on GTK 1.2 (I don't know why)
    // if it is called before window is shown, it doesn't work 
    // and it is impossible to change the background later.
    if (frame->IsShown())  
        SetBackgroundColour(bg_color); 
    Refresh(); 
} 

void OutputWin::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/OutputWin/Colors"));
    cfg_write_color (cf, wxT("normal"), text_color[os_normal]);  
    cfg_write_color (cf, wxT("warn"), text_color[os_warn]); 
    cfg_write_color (cf, wxT("quot"), text_color[os_quot]); 
    cfg_write_color (cf, wxT("input"), text_color[os_input]); 
    cfg_write_color (cf, wxT("bg"), bg_color); 
    cf->SetPath(wxT("/OutputWin"));
    cfg_write_font (cf, wxT("font"), GetDefaultStyle().GetFont());
}

void OutputWin::append_text (OutputStyle style, const wxString& str)
{
    const int max_len = 1048576;
    const int delta = 262144;
    if (GetLastPosition() > max_len)
        Remove(0, delta);

    SetDefaultStyle (wxTextAttr (text_color[style]));
    AppendText (str);
    //ShowPosition(GetLastPosition());
}

void OutputWin::OnPopupColor (wxCommandEvent& event)
{
    int n = event.GetId();
    wxColour *col;
    if (n == ID_OUTPUT_C_BG)
        col = &bg_color;
    else if (n == ID_OUTPUT_C_OU)
        col = &text_color[os_normal];
    else if (n == ID_OUTPUT_C_QT)
        col = &text_color[os_quot];
    else if (n == ID_OUTPUT_C_WR)
        col = &text_color[os_warn];
    else if (n == ID_OUTPUT_C_IN)
        col = &text_color[os_input];
    else 
        return;
    if (change_color_dlg(*col)) {
        SetBackgroundColour (bg_color);
        SetDefaultStyle (wxTextAttr(wxNullColour, bg_color));
        Refresh();
    }
}

void OutputWin::OnPopupFont (wxCommandEvent&)
{
    wxFontData data; 
    data.SetInitialFont (GetDefaultStyle().GetFont());
    wxFontDialog dlg (frame, data);
    int r = dlg.ShowModal();
    if (r == wxID_OK) {
        wxFont f = dlg.GetFontData().GetChosenFont();
        SetDefaultStyle (wxTextAttr (wxNullColour, wxNullColour, f));
        Refresh();
    }
}

void OutputWin::OnPopupClear (wxCommandEvent&)
{
    Clear();
    show_fancy_dashes();
}

    
void OutputWin::OnRightDown (wxMouseEvent& event)
{
    wxMenu popup_menu (wxT("output text menu"));

    wxMenu *color_menu = new wxMenu;
    color_menu->Append (ID_OUTPUT_C_BG, wxT("&Background"));
    color_menu->Append (ID_OUTPUT_C_IN, wxT("&Input"));
    color_menu->Append (ID_OUTPUT_C_OU, wxT("&Output"));
    color_menu->Append (ID_OUTPUT_C_QT, wxT("&Quotation"));
    color_menu->Append (ID_OUTPUT_C_WR, wxT("&Warning"));
    popup_menu.Append  (ID_OUTPUT_C   , wxT("&Color"), color_menu);

    popup_menu.Append  (ID_OUTPUT_P_FONT, wxT("&Font"));
    popup_menu.Append  (ID_OUTPUT_P_CLEAR, wxT("Clea&r"));

    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void OutputWin::OnKeyDown (wxKeyEvent& event)
{
    frame->focus_input(event);
}

