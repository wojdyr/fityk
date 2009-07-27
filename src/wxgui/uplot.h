// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__WX_UPLOT__H__
#define FITYK__WX_UPLOT__H__

#include <vector>

/// wxPanel with associated bitmap buffer, used for drawing plots
/// refresh() must be called when data is changed
/// BufferedPanel checks if size of the plot was changed and refreshes
/// plot automatically.
class BufferedPanel : public wxPanel
{
public:
    BufferedPanel(wxWindow *parent);

    /// mark panel as dirty and needing replotting,
    /// to be called when the content of panel is changed
    void refresh() { dirty_ = true; }
    // force redrawing panel now
    void redraw_now();
    /// called from wxPaint event handler
    void buffered_draw();
    /// plotting function called to refresh buffer
    virtual void draw(wxDC &dc, bool monochrome=false) = 0;
    /// get bitmap buffer
    wxBitmap const& get_bitmap() const { return buffer_; }

    /// set background color
    void set_bg_color(wxColour const& c);
    /// get background color
    wxColour const& get_bg_color() const { return bg_color_; }

protected:

private:
    wxMemoryDC memory_dc_;
    wxBitmap buffer_;
    bool dirty_;
    wxColour bg_color_;

    bool resize_buffer(wxDC &dc);

    void OnIdle(wxIdleEvent&);
};


/// returns positions of major and minor tics, for normal or logarithmic scale
std::vector<double> scale_tics_step(double beg, double end, int max_tics,
                                    std::vector<double> &minors,
                                    bool logarithm=false);

#endif //FITYK__WX_UPLOT__H__
