// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// (It is also part of xyconvert and can be distributed under LGPL2.1)

#ifndef FITYK_WX_UPLOT_H_
#define FITYK_WX_UPLOT_H_

#include <vector>

/// wxPanel with associated bitmap buffer, used for drawing plots
/// refresh() must be called when data is changed
/// BufferedPanel checks if size of the plot was changed and refreshes
/// plot automatically.
class BufferedPanel : public wxPanel
{
public:
    static wxString format_label(double x, double range)
    {
        return wxString::Format(range < 1e6 ? wxT("%.12g") : wxT("%g"), x);
    }

    BufferedPanel(wxWindow *parent);
    virtual ~BufferedPanel() {}

    /// mark panel as dirty and needing replotting,
    /// to be called when the content of panel is changed
    void refresh() { dirty_ = true; }
    // force redrawing panel now
    void redraw_now();
    /// called from wxPaint event handler
    /// updates buffer only if the window size is changed or if dirty_
    void update_buffer_and_blit();
    /// copy bitmap to window
    void blit(wxDC& dc);
    /// wraps dc in wxGCDC (if needed) and calls draw()
    void gc_draw(wxMemoryDC &dc);
    /// create bitmap w x h with given depth (-1 = screen) and draw plot on it
    wxBitmap draw_on_bitmap(int w, int h, int depth=-1);
    /// plotting function called to refresh buffer
    virtual void draw(wxDC &dc, bool monochrome=false) = 0;
    /// get bitmap buffer
    wxBitmap const& get_bitmap() const { return buffer_; }

    /// set background color
    virtual void set_bg_color(wxColour const& c);
    /// get background color
    wxColour const& get_bg_color() const { return bg_color_; }

protected:
    bool support_antialiasing_;
private:
    bool dirty_;
    wxMemoryDC memory_dc_;
    wxBitmap buffer_;
    wxColour bg_color_;

    bool resize_buffer(wxDC &dc);

    void OnIdle(wxIdleEvent&);
};

class PlotWithTics : public BufferedPanel
{
public:
    PlotWithTics(wxWindow* parent);
    void OnPaint(wxPaintEvent &) { update_buffer_and_blit(); }
    void draw_tics(wxDC &dc, double x_min, double x_max,
                   double y_min, double y_max);
    void draw_axis_labels(wxDC& dc, std::string const& x_name,
                          std::string const& y_name);
    void draw_point(wxDC& dc, double x, double y)
                                        { dc.DrawPoint(getX(x), getY(y)); }

protected:
    double xScale, yScale;
    double xOffset, yOffset;
    int getX(double x) { return iround(x * xScale + xOffset); }
    int getY(double y) { return iround(y * yScale + yOffset); }
    // Round real to integer. Defined here to avoid dependency on ../common.h.
    static int iround(double d) { return static_cast<int>(floor(d+0.5)); }
};


/// returns positions of major and minor tics, for normal or logarithmic scale
std::vector<double> scale_tics_step(double beg, double end, int max_tics,
                                    std::vector<double> &minors,
                                    bool logarithm=false);

#endif //FITYK_WX_UPLOT_H_
