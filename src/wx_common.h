// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$
#ifndef FITYK__WX_COMMON__H__
#define FITYK__WX_COMMON__H__

#include "common.h"
#include <wx/splitter.h>

enum MouseModeEnum { mmd_zoom, mmd_bg, mmd_add, mmd_range, mmd_peak };

struct PlotShared
{
    PlotShared() : xUserScale(1.), xLogicalOrigin(0.), plot_y_scale(1e3) {}
    int x2X (fp x) {return static_cast<int>((x - xLogicalOrigin) * xUserScale);}
    fp X2x (int X) { return X / xUserScale + xLogicalOrigin; }
    int dx2dX (fp dx) { return static_cast<int>(dx * xUserScale); }
    fp dX2dx (int dX) { return dX / xUserScale; }

    fp xUserScale, xLogicalOrigin; 
    std::vector<std::vector<fp> > buf;
    fp plot_y_scale;
    std::vector<wxPoint> peaktops;
};


// only wxString and long types can be read conveniently from wxConfig
// these functions are defined in plot.cpp

class wxConfigBase;
class wxString;

bool read_bool_from_config(wxConfigBase* cf, wxString const& key, bool def_val);
double read_double_from_config(wxConfigBase* cf, wxString const& key, 
                               double def_val);

wxColour read_color_from_config(wxConfigBase const* config, wxString const& key,
                                wxColour const& default_value);
void write_color_to_config(wxConfigBase* config, const wxString& key,
                           const wxColour& value);

wxFont read_font_from_config(wxConfigBase const* config, wxString const& key,
                             wxFont const& default_value);
void write_font_to_config(wxConfigBase* config, wxString const& key,
                          wxFont const& value);

inline bool should_focus_input(int key)
{
    return key == ' ' || key == WXK_TAB || (key >= 'A' && key <= 'Z')
        || key == '%' || key == '$' || key == '@' || key == '#';
}

bool change_color_dlg(wxColour& col);

//dummy events -- useful when calling event handler functions
extern wxMouseEvent dummy_mouse_event;
extern wxCommandEvent dummy_cmd_event;


// version 2.4 compatibility
#if !wxCHECK_VERSION(2,5,3)
enum {
    wxID_REVERT_TO_SAVED = 15000,
    wxID_ADD,
    wxID_REMOVE,
    wxID_UP,
    wxID_DOWN
};
#endif


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

#endif 
