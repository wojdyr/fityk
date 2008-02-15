// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__WX_PLOT__H__
#define FITYK__WX_PLOT__H__

#include <limits.h>
#include <vector>
#include <wx/config.h>
#include "../data.h" //Point

// INT_MIN, given as coordinate, is invalid value, means "cancel drawing"

class Sum;
class Data;
class View;

enum MouseActEnum  { mat_start, mat_stop, mat_move, mat_redraw };

void draw_line_with_style(wxDC& dc, int style, 
                          wxCoord X1, wxCoord Y1, wxCoord X2, wxCoord Y2);

// convention here: lowercase coordinates of point are real values,
// and uppercase ones are coordinates of point on screen (integers).


#if 0
class Canvas
{
public:
    wxDC &w;
    bool monochrome;

    fDC(wxDC &w_) : w(w_), monochrome(false) {}
    int get_pixel_width() const;
    int get_pixel_height() const;
    void use_subcanvas(int X, int Y, int width, int height);
        //dc->SetClippingRegion(X, Y, width, height);
        //dc->SetDeviceOrigin(X, Y);
        //p_width = width;
        //p_height = height;
    void use_all();
        //dc->DestroyClippingRegion();
        //dc->SetDeviceOrigin(0, 0);
        //p_width = 0;
        //p_height = 0;
private:
    int p_width;
    int p_height;
};
#endif

/// wxPanel with associated bitmap buffer, used for drawing plots
/// refresh() must be called when data is changed
/// BufferedPanel checks if size of the plot was changed and refreshes
/// plot automatically.
class BufferedPanel : public wxPanel
{
public:
    BufferedPanel(wxWindow *parent)
       : wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize, 
                 wxNO_BORDER|wxFULL_REPAINT_ON_RESIZE) {}
    /// to be called when content of the panel is changed
    void refresh(bool now=false);
    /// called from wxPaint event handler
    void buffered_draw();
    /// no need to call it explicitely
    void clear();
    /// plotting function called to refresh buffer
    virtual void draw(wxDC &dc, bool monochrome=false) = 0;

protected:
    wxColour backgroundCol;

private:
    wxMemoryDC memory_dc;
    wxBitmap buffer;

    bool resize_buffer(wxDC &dc);
    void clear_and_draw();
};


/// convertion between pixels and logical values
class Scale
{
public:
    double scale, origin; 
    bool logarithm, reversed;

    Scale() : scale(1.), origin(0.), logarithm(false), reversed(false) {}

    /// value -> pixel
    inline int px(double val) const;
    /// pixel -> value
    inline double val(int px) const;

    // set scale using minimum and maximum logical values and width/height 
    // of the screen in pixels.
    // In case of y scale, where pixel=0 is at the top, m and M are switched 
    void set(fp m, fp M, int pixels);

private:
    static int inf_px(float t) { return t > 0 ? SHRT_MAX : SHRT_MIN; }
};


int Scale::px(double val) const
{
    if (logarithm) {
        if (val <= 0)
            return inf_px(scale);
        val = log(val);
    }
    double t = (val - origin) * scale;
    return fabs(t) < SHRT_MAX ? static_cast<int>(t) : inf_px(t);
}

double Scale::val(int px) const
{ 
    fp a = px / scale + origin; 
    return logarithm ? exp(a) : a; 
}


/// This class has no instances, MainPlot and AuxPlot are derived from it
/// It knows how to draw on wxDC. Note that wxDC:SetClippingRegion() should be
/// used together with wxDC::SetDeviceOrigin(). Clipping box is used only in
/// get_pixel_width() and get_pixel_height() functions.
/// When plotting a curve, values in each x from 0 to get_pixel_width() is 
/// calculated.

class FPlot : public BufferedPanel 
{
public:
    FPlot (wxWindow *parent)
       : BufferedPanel(parent),
         draw_sigma(false), 
         mouse_press_X(INT_MIN), mouse_press_Y(INT_MIN),
         vlfc_prev_x(INT_MIN), vlfc_prev_x0(INT_MIN)   {}
         
    ~FPlot() {}
    wxColour const& get_bg_color() const { return backgroundCol; }
    void draw_crosshair(int X, int Y);
    void set_scale(int pixel_width, int pixel_height);
    int get_special_point_at_pointer(wxMouseEvent& event);
    std::vector<wxPoint> const& get_special_points() const
                                                { return special_points; }
    Scale const& get_x_scale() const { return xs; }
    Scale const& get_y_scale() const { return ys; }
    virtual void save_settings(wxConfigBase *cf) const;
    virtual void read_settings(wxConfigBase *cf);

protected:
    Scale xs, ys;
    wxColour activeDataCol, inactiveDataCol, xAxisCol;
    wxFont ticsFont;
    int point_radius;
    bool line_between_points;
    bool draw_sigma;
    bool x_axis_visible, y_axis_visible, xtics_visible, ytics_visible,
         xminor_tics_visible, yminor_tics_visible;
    bool x_grid, y_grid;
    int x_max_tics, y_max_tics, x_tic_size, y_tic_size;
    int mouse_press_X, mouse_press_Y;
    int vlfc_prev_x, vlfc_prev_x0; //vertical lines following cursor
    std::vector<wxPoint> special_points; //used to mark positions of peak tops

    void draw_dashed_vert_line(int X, int style=wxSHORT_DASH);
    bool vert_line_following_cursor(MouseActEnum ma, int x=0, int x0=INT_MIN);
    void draw_xtics (wxDC& dc, View const& v, bool set_pen=true);
    void draw_ytics (wxDC& dc, View const &v, bool set_pen=true);
    double get_max_abs_y(double (*compute_y)(std::vector<Point>::const_iterator,
                                             Sum const*),
                         std::vector<Point>::const_iterator first,
                         std::vector<Point>::const_iterator last,
                         Sum const* sum);
    void draw_data (wxDC& dc, 
                    double (*compute_y)(std::vector<Point>::const_iterator, 
                                        Sum const*),
                    Data const* data, 
                    Sum const* sum, 
                    wxColour const& color = wxNullColour,
                    wxColour const& inactive_color = wxNullColour,
                    int Y_offset = 0,
                    bool cumulative=false);
    void change_tics_font();

    int get_pixel_width(wxDC const& dc) const 
      //{ return dc.GetSize().GetWidth(); }
    { 
          int w;
          dc.GetClippingBox(NULL, NULL, &w, NULL);
          return w != 0 ? w : dc.GetSize().GetWidth(); 
    }
    int get_pixel_height(wxDC const& dc) const 
    //  { return dc.GetSize().GetHeight(); }
    { 
          int h;
          dc.GetClippingBox(NULL, NULL, NULL, &h);
          return h != 0 ? h : dc.GetSize().GetHeight(); 
    }

    DECLARE_EVENT_TABLE()
};

// utilities

inline wxColour invert_colour(const wxColour& col)
{ return wxColour(255 - col.Red(), 255 - col.Green(), 255 - col.Blue()); }

std::vector<double> scale_tics_step (double beg, double end, int max_tics, 
                                 std::vector<double> &minors, bool log=false);

#endif 

