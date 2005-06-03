// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$
#ifndef WX_COMMON__H__
#define WX_COMMON__H__

#include "common.h"

enum Mouse_mode_enum { mmd_zoom, mmd_bg, mmd_add, mmd_range, mmd_peak };

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

bool read_bool_from_config(wxConfigBase *cf, const wxString& key, bool def_val);
double read_double_from_config(wxConfigBase *cf, const wxString& key, 
                               double def_val);

wxColour read_color_from_config(const wxConfigBase *config, const wxString& key,
                                const wxColour& default_value);
void write_color_to_config (wxConfigBase *config, const wxString& key,
                            const wxColour& value);

wxFont read_font_from_config (const wxConfigBase *config, const wxString& key,
                              const wxFont& default_value);
void write_font_to_config (wxConfigBase *config, const wxString& key,
                           const wxFont& value);

//dummy events -- useful when calling event handler functions
extern wxMouseEvent dummy_mouse_event;
extern wxCommandEvent dummy_cmd_event;

#endif // WX_COMMON__H__
