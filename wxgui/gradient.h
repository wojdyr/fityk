// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_GRADIENT_H_
#define FITYK_WX_GRADIENT_H_

#include <vector>
#include <wx/odcombo.h>
#include <wx/clrpicker.h>

class GradientDlg : public wxDialog
{
public:
    GradientDlg(wxWindow *parent, wxWindowID id,
                const wxColour& first_col, const wxColour& last_col);
    void OnColor(wxColourPickerEvent&) { display_->Refresh(); }
    void OnRadioChanged(wxCommandEvent &) { display_->Refresh(); }
    wxColour get_value(float x);
private:
    wxRadioButton *rb1_, *rb2_, *rb3_, *rb4_;
    wxColourPickerCtrl *from_cp_, *to_cp_;
    wxPanel *display_;
};


class MultiColorCombo : public wxOwnerDrawnComboBox
{
public:
    MultiColorCombo(wxWindow* parent, const wxColour* bg_color,
                    std::vector<wxColour>& colors);
    virtual void OnDrawItem(wxDC& dc, const wxRect& rect,
                            int item, int flags) const;
    virtual wxCoord OnMeasureItem(size_t) const { return 24; }
    virtual wxCoord OnMeasureItemWidth(size_t) const
        { return colors_.size() * 2 + 8; }
    virtual wxSize DoGetBestSize() const;

private:
    static const wxColour palette[21];
    const wxColour* bg_color_;
    // this class does not resize colors_, only changes values
    std::vector<wxColour>& colors_;

    wxColour get_color(int selection, int i) const;
    void OnSelection(wxCommandEvent& event);
};

#endif
