// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_PLOT__H__
#define FITYK__WX_PLOT__H__

#include <limits.h>
#include <vector>
#include <wx/config.h>
#include "cmn.h" // PlotShared

// INT_MIN, given as coordinate, is invalid value, means "cancel drawing"


struct Point;
class Sum;
class Data;
class View;

enum MouseActEnum  { mat_start, mat_stop, mat_move, mat_redraw };

void draw_line_with_style(wxDC& dc, int style, 
                          wxCoord X1, wxCoord Y1, wxCoord X2, wxCoord Y2);

// convention here: lowercase coordinates of point are real values,
// and uppercase ones are coordinates of point on screen (integers).
// eg. x2X() - convert point x coordinate to pixel position on screen 
//


class BufferedPanel : public wxPanel
{
public:
    BufferedPanel(wxWindow *parent)
       : wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize, 
                 wxNO_BORDER|wxFULL_REPAINT_ON_RESIZE) {}
    void refresh(bool now=false);
    void buffered_draw();
    virtual void draw(wxDC &dc, bool monochrome=false) = 0;

protected:
    wxColour backgroundCol;

private:
    wxMemoryDC memory_dc;
    wxBitmap buffer;

    bool resize_buffer(wxDC &dc);
    void clear_and_draw();
};


/// This class has no instances, MainPlot and AuxPlot are derived from it
class FPlot : public BufferedPanel //wxPanel
{
public:
    FPlot (wxWindow *parent, PlotShared &shar)
       : BufferedPanel(parent),
         draw_sigma(false), y_logarithm(false), 
         yUserScale(1.), yLogicalOrigin(0.), 
         shared(shar), mouse_press_X(INT_MIN), mouse_press_Y(INT_MIN),
         vlfc_prev_x(INT_MIN), vlfc_prev_x0(INT_MIN)   {}
         
    ~FPlot() {}
    wxColour const& get_bg_color() const { return backgroundCol; }
    void draw_crosshair(int X, int Y);
    virtual void save_settings(wxConfigBase *cf) const;
    virtual void read_settings(wxConfigBase *cf);

protected:
    wxColour activeDataCol, inactiveDataCol, xAxisCol;
    wxFont ticsFont;
    int point_radius;
    bool line_between_points;
    bool draw_sigma;
    bool x_axis_visible, y_axis_visible, xtics_visible, ytics_visible,
         xminor_tics_visible, yminor_tics_visible;
    bool y_logarithm;
    bool x_grid, y_grid;
    int x_max_tics, y_max_tics, x_tic_size, y_tic_size;
    double yUserScale, yLogicalOrigin; 
    PlotShared &shared;
    int mouse_press_X, mouse_press_Y;
    int vlfc_prev_x, vlfc_prev_x0; //vertical lines following cursor

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
    int y2Y (double y) {  
        if (y_logarithm) {
            if (y > 0)
                y = log(y);
            else
                return yUserScale > 0 ? SHRT_MIN : SHRT_MAX;
        }
        double t = (y - yLogicalOrigin) * yUserScale;
        return (fabs(t) < SHRT_MAX ? static_cast<int>(t) 
                                   : t > 0 ? SHRT_MAX : SHRT_MIN);
    }
    double Y2y (int Y) { 
        double y = Y / yUserScale + yLogicalOrigin;
        return y_logarithm ? exp(y) : y; 
    }
    int x2X (double x) { return shared.x2X(x); }
    double X2x (int X) { return shared.X2x(X); }

    DECLARE_EVENT_TABLE()
};

// utilities

inline wxColour invert_colour(const wxColour& col)
{ return wxColour(255 - col.Red(), 255 - col.Green(), 255 - col.Blue()); }

std::vector<double> scale_tics_step (double beg, double end, int max_tics, 
                                 std::vector<double> &minors, bool log=false);

#endif 

