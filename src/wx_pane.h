// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_PANE__H__
#define FITYK__WX_PANE__H__

#include <wx/print.h>
#include <wx/config.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <list>
#include "wx_common.h"  //for MouseModeEnum, OutputStyle

class PlotPane;
class IOPane;
class MainPlot;
class AuxPlot;
class FPlot;
class PlotCore;
class BgManager;


/// based on http://wiki.wxpython.org/index.cgi/ProportionalSplitterWindow
/// it is like wxSplitterWindow, but when resized, both windows are resized
/// proporionally
class ProportionalSplitter: public wxSplitterWindow
{
public:
    ProportionalSplitter(wxWindow* parent, 
                         wxWindowID id=-1, 
                         float proportion=0.66, // 0. - 1.
                         const wxSize& size = wxDefaultSize,
                         long style=wxSP_NOBORDER|wxSP_FULLSASH|wxSP_3DSASH,
                         const wxString& name = "proportionalSplitterWindow");
    bool SplitHorizontally(wxWindow* win1, wxWindow* win2, float proportion=-1);
    bool SplitVertically(wxWindow* win1, wxWindow* win2, float proportion=-1);
    int GetExpectedSashPosition();
    void ResetSash();
    float GetProportion() const { return m_proportion; }
    void SetProportion(float proportion) {m_proportion=proportion; ResetSash();}

protected:
    float m_proportion; //0-1
    bool m_firstpaint;

    void OnReSize(wxSizeEvent& event);
    void OnSashChanged(wxSplitterEvent &event);
    void OnPaint(wxPaintEvent &event);
};


class InputField : public wxTextCtrl
{
public:
    InputField(wxWindow *parent, wxWindowID id,
               const wxString& value = wxEmptyString,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0)
    : wxTextCtrl(parent, id, value, pos, size, style), 
      history(1), h_pos(history.begin()) {} 
protected:
    void OnKeyDown (wxKeyEvent& event);

    std::list<wxString> history;
    std::list<wxString>::iterator h_pos;

    DECLARE_EVENT_TABLE()
};


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
    void focus_output() { output_win->SetFocus(); }
    void show_fancy_dashes() { output_win->fancy_dashes(); }
private:
    OutputWin *output_win;
    InputField *input_field;

    DECLARE_EVENT_TABLE()
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
    void refresh_plots(bool refresh, bool update, bool only_main=false);
    void set_mouse_mode(MouseModeEnum m);
    void update_mouse_hints();
    bool is_background_white();
    std::vector<std::string> const& get_zoom_hist() const { return zoom_hist; }
    MainPlot const* get_plot() const { return plot; }
    MainPlot* get_plot() { return plot; }
    BgManager* get_bg_manager(); 
    std::vector<FPlot*> const get_visible_plots() const;
    void show_aux(int n, bool show); 
    bool aux_visible(int n) const;
    void draw_crosshair(int X, int Y);

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


class DataList : public wxListView
{
public:
    DataList(wxWindow *parent, wxWindowID id=-1);
    void populate();
    std::vector<std::string> get_selected_data();
    void OnFocusChanged(wxListEvent &event);
    DECLARE_EVENT_TABLE()
};

class DataPane : public ProportionalSplitter
{
public:
    DataPane(wxWindow *parent, wxWindowID id=-1);
    void OnDataButtonNew (wxCommandEvent& event);
    void OnDataButtonDup (wxCommandEvent& event);
    void OnDataButtonRen (wxCommandEvent& event);
    void OnDataButtonDel (wxCommandEvent& event);
    void OnDataButtonCol (wxCommandEvent& event);
    void OnDataLookChanged (wxCommandEvent& event);
    void update();
    /// get active dataset number -- if none is focused, return first one (0)
    int get_focused_data() { int n=dl->GetFocusedItem(); return n==-1 ? 0 : n; }
    int set_selection(int page) { return nb->SetSelection(page); }
private:
    wxNotebook *nb;
    DataList *dl;

    DECLARE_EVENT_TABLE()
};


class FPrintout: public wxPrintout
{
public:
    FPrintout(const PlotPane *p_pane);
    bool HasPage(int page) { return (page == 1); }
    bool OnPrintPage(int page);
    void GetPageInfo(int *minPage,int *maxPage,int *selPageFrom,int *selPageTo)
        { *minPage = *maxPage = *selPageFrom = *selPageTo = 1; }
private:
    const PlotPane *pane;
};


#endif 

