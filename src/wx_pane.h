// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef WX_PANE__H__
#define WX_PANE__H__

#include <wx/print.h>
#include <wx/config.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>
#include "wx_common.h"  //for Mouse_mode_enum, OutputStyle

class PlotPane;
class IOPane;
class MainPlot;
class AuxPlot;
class FPlot;
class PlotCore;


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
                         //IMHO default wxSP_3D doesn't look good (on wxGTK).
                         long style=wxSP_NOBORDER|wxSP_FULLSASH,
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


class FCombo : public wxComboBox
{
public:
    FCombo(wxWindow *parent, wxWindowID id,
                const wxString& value = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                int n = 0, const wxString choices[] = NULL,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxComboBoxNameStr)
    : wxComboBox(parent, id, value, pos, size, n, choices, style,
                    validator, name) { }
protected:
    void OnKeyDown (wxKeyEvent& event);

    DECLARE_EVENT_TABLE()
};


class Output_win : public wxTextCtrl
{
public:
    Output_win (wxWindow *parent, wxWindowID id,
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

private:
    wxColour text_color[4]; 
    wxColour bg_color;

    void fancy_dashes();

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
    void focus_input() { input_combo->SetFocus(); }
    void focus_output() { output_win->SetFocus(); }
private:
    Output_win *output_win;
    FCombo   *input_combo;

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
    void refresh_plots(bool update);
    void set_mouse_mode(Mouse_mode_enum m);
    void update_mouse_hints();
    bool is_background_white();
    const std::vector<std::string>& get_zoom_hist() const { return zoom_hist; }
    const MainPlot* get_plot() const { return plot; }
    const std::vector<FPlot*> get_visible_plots() const;
    void show_aux(int n, bool show); 
    bool aux_visible(int n) const;
private:
    Plot_shared plot_shared;
    MainPlot *plot;
    ProportionalSplitter *aux_split;
    AuxPlot *aux_plot[2];
    std::vector<std::string> zoom_hist;

    DECLARE_EVENT_TABLE()
};


class DataPaneTree : public wxTreeCtrl
{
public:
    DataPaneTree(wxWindow *parent, wxWindowID id);

    void OnIdle(wxIdleEvent &event);
    void OnSelChanging(wxTreeEvent &event);
    void OnSelChanged(wxTreeEvent &event);
    void OnPopupMenu(wxMouseEvent &event);
    void OnMenuItem(wxCommandEvent &event);
    void OnKeyDown(wxKeyEvent& event);
private:
    int pmenu_p, pmenu_d;// communication between OnPopupMenu and OnMenuItem

    void update_tree_datalabels(const PlotCore *pcore, 
                                const wxTreeItemId &plot_item);
    int count_previous_siblings(const wxTreeItemId &id);

    DECLARE_EVENT_TABLE()
};

class DataPane : public wxPanel
{
public:
    DataPane(wxWindow *parent, wxWindowID id=-1);
private:
    DataPaneTree *tree;

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

//------------------------------------------------------------------

wxColour read_color_from_config(const wxConfigBase *config, const wxString& key,
                                const wxColour& default_value);

void write_color_to_config (wxConfigBase *config, const wxString& key,
                            const wxColour& value);
inline bool read_bool_from_config (const wxConfigBase *config, 
                                   const wxString& key, bool def_val)
                    { bool b; config->Read(key, &b, def_val); return b; }



#endif //WX_PANE__H__

