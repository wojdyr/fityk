// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__WX_PLOT__H__
#define FITYK__WX_PLOT__H__

#include <limits.h>
#include <vector>
#include <wx/config.h>
#include "../data.h" //Point
#include "uplot.h" //BufferedPanel
#include "cmn.h" // compatibility with wx2.8 (defined wxPenStyle, etc.)

// convention: lowercase coordinates of point are real values,
// and uppercase ones are coordinates of point on screen (integers).

// INT_MIN, given as coordinate, is invalid value, means "cancel drawing"

class Model;
class Data;
class Rect;

enum MouseActEnum  { mat_start, mat_stop, mat_move, mat_redraw };

enum LineOrientation { kVerticalLine, kHorizontalLine };

void draw_line_with_style(wxDC& dc, wxPenStyle style,
                          wxCoord X1, wxCoord Y1, wxCoord X2, wxCoord Y2);


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

    // Returns the same value as val(), but rounded in a smart way,
    // so when the number is formatted with "%.12g", it is not too long.
    double valr(int px) const;

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
            return inf_px(-scale);
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
    FPlot(wxWindow *parent)
       : BufferedPanel(parent),
         pen_width(1),
         draw_sigma(false),
         mouse_press_X(INT_MIN), mouse_press_Y(INT_MIN),
         lfc_prev_X(INT_MIN), lfc_prev_X0(INT_MIN), lfc_orient(kVerticalLine),
         esc_seed_(NULL) {}

    virtual ~FPlot() {}

    // in wxGTK 2.9 it seems that changing this to true doesn't make
    // the window accept focus
    virtual bool AcceptsFocus() { return false; }

    void set_font(wxDC &dc, wxFont const& font);
    void draw_crosshair(int X, int Y);
    void set_scale(int pixel_width, int pixel_height);
    int get_special_point_at_pointer(wxMouseEvent& event);
    Scale const& get_x_scale() const { return xs; }
    Scale const& get_y_scale() const { return ys; }
    virtual void save_settings(wxConfigBase *cf) const;
    virtual void read_settings(wxConfigBase *cf);
    void set_magnification(int m) { pen_width = m > 0 ? m : 1; }

protected:
    Scale xs, ys;
    int pen_width;
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
    int lfc_prev_X, lfc_prev_X0; // lines following cursor (either vert. or h.)
    LineOrientation lfc_orient;
    std::vector<wxPoint> special_points; //used to mark positions of peak tops
    wxWindow *esc_seed_; // temporary source of OnKeyDown() events

    void draw_inverted_line(int X, wxPenStyle style, LineOrientation orient);
    void start_line_following_cursor(int X0, LineOrientation orient);
    bool line_following_cursor(MouseActEnum ma, int X=0);
    void draw_xtics (wxDC& dc, Rect const& v, bool set_pen=true);
    void draw_ytics (wxDC& dc, Rect const &v, bool set_pen=true);
    double get_max_abs_y(double (*compute_y)(std::vector<Point>::const_iterator,
                                             Model const*),
                         std::vector<Point>::const_iterator first,
                         std::vector<Point>::const_iterator last,
                         Model const* model);
    void draw_data (wxDC& dc,
                    double (*compute_y)(std::vector<Point>::const_iterator,
                                        Model const*),
                    Data const* data,
                    Model const* model,
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


    // if connect is true: connect, otherwise disconnect;
    // connect to the currently focused window and handle wxEVT_KEY_DOWN 
    // event: if Esc is pressed call cancel_action().
    void connect_esc_to_cancel(bool connect);

    virtual void cancel_action() {}
    // handler used by connect_esc_to_cancel()
    void OnKeyDown(wxKeyEvent& event);
};

#endif

