// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_PANE__H__
#define FITYK__WX_PANE__H__

#include <wx/config.h>
#include <wx/spinctrl.h>
#include <list>
#include <vector>
#include <assert.h>
#include "cmn.h"  //for MouseModeEnum, OutputStyle, PlotShared, 
                  //    ProportionalSplitter
#include "../common.h" // OutputStyle

class PlotPane;
class IOPane;
class MainPlot;
class AuxPlot;
class FPlot;
class BgManager;
class InputLine;


class OutputWin : public wxTextCtrl
{
public:
    OutputWin (wxWindow *parent, wxWindowID id,
                const wxPoint& pos = wxDefaultPosition, 
                const wxSize& size = wxDefaultSize);
    void append_text (OutputStyle style, const wxString& str);
    void OnRightDown (wxMouseEvent& event);
    void OnPopupColor  (wxCommandEvent& event);       
    void OnPopupFont   (wxCommandEvent& event);  
    void OnPopupClear  (wxCommandEvent& event); 
    void OnKeyDown     (wxKeyEvent& event);
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
    void fancy_dashes();

private:
    wxColour text_color[4]; 
    wxColour bg_color;

    DECLARE_EVENT_TABLE()
};


class IOPane : public wxPanel
{
public:
    IOPane(wxWindow *parent, wxWindowID id=-1);
    void append_text (OutputStyle style, const wxString& str) 
                                      { output_win->append_text(style, str); }
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
    void focus_input(int key);
    void show_fancy_dashes() { output_win->fancy_dashes(); }
    void show_popup_menu(wxMouseEvent& ev) { output_win->OnRightDown(ev); }
    void edit_in_input(std::string const& s);
    void OnInputLine(wxString const& s);
private:
    OutputWin *output_win;
    InputLine *input_field;
};


class PlotPane : public ProportionalSplitter
{
    friend class FPrintout;
public:
    PlotPane(wxWindow *parent, wxWindowID id=-1);
    void zoom_forward();
    std::string zoom_backward(int n=1);
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);
    void refresh_plots(bool update, bool only_main=false);
    void set_mouse_mode(MouseModeEnum m);
    void update_mouse_hints();
    bool is_background_white();
    std::vector<std::string> const& get_zoom_hist() const { return zoom_hist; }
    MainPlot const* get_plot() const { return plot; }
    MainPlot* get_plot() { return plot; }
    BgManager* get_bg_manager(); 
    std::vector<FPlot*> const get_visible_plots() const;
    AuxPlot* get_aux_plot(int n) const 
                     { assert(n>=0 && n<2); return aux_plot[n]; }
    void show_aux(int n, bool show); 
    bool aux_visible(int n) const;
    void draw_crosshair(int X, int Y);
    void set_shared_scale();

    bool crosshair_cursor;
private:
    PlotShared plot_shared;
    MainPlot *plot;
    ProportionalSplitter *aux_split;
    AuxPlot *aux_plot[2];
    std::vector<std::string> zoom_hist;

    void do_draw_crosshair(int X, int Y);

    DECLARE_EVENT_TABLE()
};


#endif 

