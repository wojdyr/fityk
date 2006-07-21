// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$
#ifndef FITYK__WX_COMMON__H__
#define FITYK__WX_COMMON__H__

#include "common.h"
#include <wx/splitter.h>
#include <wx/arrstr.h>
#include <wx/spinctrl.h>

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

inline wxString pchar2wx(char const* pc)
{
    return wxString(pc, wxConvUTF8);
}

inline wxString s2wx(std::string const& s) { return pchar2wx(s.c_str()); }


inline std::string wx2s(wxString const& w) 
{ 
    return std::string((const char*) w.mb_str(wxConvUTF8)); 
}

inline wxArrayString stl2wxArrayString(std::vector<std::string> const& vs)
{
    wxArrayString wxas; 
    for (std::vector<std::string>::const_iterator i = vs.begin(); 
                                                        i != vs.end(); ++i)
        wxas.Add(s2wx(*i));
    return wxas;
}


// only wxString and long types can be read conveniently from wxConfig
// these functions are defined in plot.cpp

class wxConfigBase;
class wxString;

bool cfg_read_bool(wxConfigBase* cf, wxString const& key, bool def_val);
double cfg_read_double(wxConfigBase* cf, wxString const& key, 
                               double def_val);

wxColour cfg_read_color(wxConfigBase const* config, wxString const& key,
                        wxColour const& default_value);
void cfg_write_color(wxConfigBase* config, const wxString& key,
                           const wxColour& value);

wxFont cfg_read_font(wxConfigBase const* config, wxString const& key,
                             wxFont const& default_value);
void cfg_write_font(wxConfigBase* config, wxString const& key,
                          wxFont const& value);

inline bool should_focus_input(int key)
{
    return key == ' ' || key == WXK_TAB || (key >= 'A' && key <= 'Z')
        || key == '%' || key == '$' || key == '@' || key == '#';
}

bool change_color_dlg(wxColour& col);
void add_apply_close_buttons(wxWindow *parent, wxSizer *top_sizer);

//dummy events -- useful when calling event handler functions
extern wxMouseEvent dummy_mouse_event;
extern wxCommandEvent dummy_cmd_event;


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
                         long style=wxSP_NOBORDER|wxSP_FULLSASH|wxSP_3DSASH);
    bool SplitHorizontally(wxWindow* win1, wxWindow* win2, float proportion=-1);
    bool SplitVertically(wxWindow* win1, wxWindow* win2, float proportion=-1);
    float GetProportion() const { return m_proportion; }
    void SetProportion(float proportion) {m_proportion=proportion; ResetSash();}
    void SetSashPosition(int position);

protected:
    float m_proportion; //0-1
    bool m_firstpaint;

    void ResetSash();
    int GetExpectedSashPosition();
    void OnReSize(wxSizeEvent& event);
    void OnSashChanged(wxSplitterEvent &event);
    void OnPaint(wxPaintEvent &event);
};

class SpinCtrl: public wxSpinCtrl
{
public:
    SpinCtrl(wxWindow* parent, wxWindowID id, int val, 
             int min, int max, int width=50)
        : wxSpinCtrl (parent, id, wxString::Format(wxT("%i"), val),
                      wxDefaultPosition, wxSize(width, -1), 
                      wxSP_ARROW_KEYS, min, max, val) 
    {}
};

#endif 
