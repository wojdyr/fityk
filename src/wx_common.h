// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$
#ifndef WX_COMMON__H__
#define WX_COMMON__H__

enum Mouse_mode_enum { mmd_zoom, mmd_bg, mmd_add, mmd_range, mmd_peak };
enum Output_style_enum  { os_ty_normal, os_ty_warn, os_ty_quot, os_ty_input };


class Plot_shared
{
public:
    fp xUserScale, xLogicalOrigin; 
    bool buffer_enabled;
    std::vector<std::vector<fp> > buf;
    fp plot_y_scale;

    Plot_shared() : xUserScale(1.), xLogicalOrigin(0.), plot_y_scale(1e3) {}
    int x2X (fp x) {return static_cast<int>((x - xLogicalOrigin) * xUserScale);}
    fp X2x (int X) { return X / xUserScale + xLogicalOrigin; }
    int dx2dX (fp dx) { return static_cast<int>(dx * xUserScale); }
    fp dX2dx (int dX) { return dX / xUserScale; }
};

//dummy events -- useful when calling event handler functions
extern wxMouseEvent dummy_mouse_event;
extern wxCommandEvent dummy_cmd_event;

#endif // WX_COMMON__H__
