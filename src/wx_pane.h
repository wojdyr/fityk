// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_PANE__H__
#define FITYK__WX_PANE__H__

#include <wx/print.h>
#include <wx/config.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <list>
#include <utility>
#include "wx_common.h"  //for MouseModeEnum, OutputStyle

class PlotPane;
class IOPane;
class MainPlot;
class AuxPlot;
class FPlot;
class PlotCore;
class BgManager;
class FancyRealCtrl;
class Variable;
class Function;


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
    void edit_in_input(std::string const& s);
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

class ListWithColors : public wxListView
{
public:
    ListWithColors(wxWindow *parent, wxWindowID id, 
                   std::vector<std::pair<std::string,bool> > const& columns_);
    void populate(std::vector<std::string> const& data, 
                  wxImageList* image_list = 0,
                  int active = -2);
    void OnColumnMenu(wxListEvent &event);
    void OnRightDown(wxMouseEvent &event);
    void OnShowColumn(wxCommandEvent &event);
    void OnFitColumnWidths(wxCommandEvent &event);
    void OnSelectAll(wxCommandEvent &event);
    DECLARE_EVENT_TABLE()
private:
    std::vector<std::pair<std::string,bool> > columns;
    std::vector<std::string> list_data;
};

class ListPlusText : public ProportionalSplitter
{
public:
    ListWithColors *list;
    wxTextCtrl* inf;

    ListPlusText(wxWindow *parent, wxWindowID id, wxWindowID list_id,
                 std::vector<std::pair<std::string,bool> > const& columns_);

    void OnSwitchInfo(wxCommandEvent &event);
    DECLARE_EVENT_TABLE()
};


class SideBar : public ProportionalSplitter
{
public:
    SideBar(wxWindow *parent, wxWindowID id=-1);
    void OnDataButtonNew (wxCommandEvent& event);
    void OnDataButtonDup (wxCommandEvent& event);
    void OnDataButtonRen (wxCommandEvent& event);
    void OnDataButtonDel (wxCommandEvent& event);
    void OnDataButtonCopyF (wxCommandEvent& event);
    void OnDataButtonCol (wxCommandEvent& event);
    void OnDataLookChanged (wxCommandEvent& event);
    void OnDataShiftUpChanged (wxSpinEvent& event);
    void OnFuncButtonNew (wxCommandEvent& event);
    void OnFuncButtonDel (wxCommandEvent& event);
    void OnFuncButtonEdit (wxCommandEvent& event);
    void OnFuncButtonChType (wxCommandEvent& event);
    void OnFuncButtonCol (wxCommandEvent& event);
    void OnVarButtonNew (wxCommandEvent& event);
    void OnVarButtonDel (wxCommandEvent& event);
    void OnVarButtonEdit (wxCommandEvent& event);
    void OnFuncFilterChanged (wxCommandEvent& event);
    void OnDataFocusChanged(wxListEvent &event);
    void OnFuncFocusChanged(wxListEvent &event);
    void OnVarFocusChanged(wxListEvent &event);
    void update_lists(bool nondata_changed=true);
    /// get active dataset number -- if none is focused, return first one (0)
    int get_focused_data() const
                     { int n=d->list->GetFocusedItem(); return n==-1 ? 0 : n; }
    int get_focused_func() const; 
    int get_focused_var() const;
    bool is_func_selected(int n) const { return f->list->IsSelected(n) 
                                           || f->list->GetFocusedItem() == n; }
    int set_selection(int page) { return nb->SetSelection(page); }
    void activate_function(int n);
    std::vector<std::string> get_selected_data() const;
    bool howto_plot_dataset(int n, bool& shadowed, int& offset) const;
    std::vector<std::string> get_selected_func() const;
    std::vector<std::string> get_selected_vars() const;
    void update_data_inf();
    void update_func_inf();
    void update_var_inf();
    void update_bottom_panel();
private:
    wxNotebook *nb;
    wxPanel *data_page, *func_page, *var_page, *bottom_panel;
    wxFlexGridSizer* bp_sizer;
    wxStaticText *bp_label;
    std::vector<FancyRealCtrl*> bp_frc;
    std::vector<wxStaticText*> bp_statict;
    std::vector<bool> bp_sig;
    ListPlusText *d, *f, *v;
    wxChoice *data_look, *filter_ch;
    wxSpinCtrl *shiftup_sc;

    void update_data_list(bool nondata_changed);
    void update_func_list(bool nondata_changed);
    void update_var_list();
    void add_variable_to_bottom_panel(Variable const* var, 
                                      std::string const& tv_name);
    void clear_bottom_panel();
    std::vector<bool> make_bottom_panel_sig(Function const* func);

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

