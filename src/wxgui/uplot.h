// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id: plot.h 391 2008-02-15 02:59:57Z wojdyr $

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


/// returns positions of major and minor tics, for normal or logarithmic scale
std::vector<double> scale_tics_step(double beg, double end, int max_tics, 
                                    std::vector<double> &minors, 
                                    bool logarithm=false);

#endif //FITYK__WX_UPLOT__H__
