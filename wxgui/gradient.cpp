// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
///  Gradient Dialog (GradientDlg) and helpers

#include <wx/wx.h>
#include <wx/spinctrl.h>

#include "gradient.h"
#include "cmn.h" // change_color_dlg, add_apply_close_buttons
#include "fityk/common.h" //iround()

using namespace std;

/// displays colors from data member from left to right (one pixel - one color)
class ColorGradientDisplay : public wxPanel
{
public:
    ColorGradientDisplay(GradientDlg *parent)
        : wxPanel(parent, -1), gradient_dlg_(parent)
    {
        Connect(wxEVT_PAINT,
                wxPaintEventHandler(ColorGradientDisplay::OnPaint));
    }

    void OnPaint(wxPaintEvent&)
    {
        wxPaintDC dc(this);
        wxSize size = GetClientSize();
        wxPen pen;
        for (int i = 0; i < size.GetWidth(); ++i) {
            double d = i / (size.GetWidth() - 1.0);
            pen.SetColour(gradient_dlg_->get_value(d));
            dc.SetPen(pen);
            dc.DrawLine(i, 0, i, size.GetHeight());
        }
    }

private:
    GradientDlg *gradient_dlg_;
};



GradientDlg::GradientDlg(wxWindow *parent, wxWindowID id,
                         const wxColour& first_col, const wxColour& last_col)
    : wxDialog(parent, id, wxT("Select color gradient"),
               wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    wxBoxSizer* top_sizer = new wxBoxSizer(wxVERTICAL);

    from_cp_ = new wxColourPickerCtrl(this, -1, first_col,
                                      wxDefaultPosition, wxDefaultSize,
                                      wxCLRP_USE_TEXTCTRL);
    top_sizer->Add(from_cp_, 0, wxALL|wxEXPAND, 5);

    to_cp_ = new wxColourPickerCtrl(this, -1, last_col,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxCLRP_USE_TEXTCTRL);
    top_sizer->Add(to_cp_, 0, wxALL|wxEXPAND, 5);

    rb1_ = new wxRadioButton(this, -1, wxT("HSV gradient, clockwise hue"),
                             wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    rb2_ = new wxRadioButton(this, -1, wxT("HSV gradient, counter-clockwise"));
    rb3_ = new wxRadioButton(this, -1, wxT("RGB gradient"));
    rb4_ = new wxRadioButton(this, -1, wxT("one color"));
    top_sizer->Add(rb1_, 0, wxLEFT|wxRIGHT|wxTOP, 5);
    top_sizer->Add(rb2_, 0, wxLEFT|wxRIGHT|wxTOP, 5);
    top_sizer->Add(rb3_, 0, wxLEFT|wxRIGHT|wxTOP, 5);
    top_sizer->Add(rb4_, 0, wxALL, 5);
    display_ = new ColorGradientDisplay(this);
    display_->SetMinSize(wxSize(-1, 15));
    top_sizer->Add(display_, 0, wxALL|wxEXPAND, 5);

    add_apply_close_buttons(this, top_sizer);
    SetSizerAndFit(top_sizer);
    SetEscapeId(wxID_CLOSE);

    Connect(-1, wxEVT_COMMAND_COLOURPICKER_CHANGED,
            wxColourPickerEventHandler(GradientDlg::OnColor));
    Connect(-1, wxEVT_COMMAND_RADIOBUTTON_SELECTED,
            wxCommandEventHandler(GradientDlg::OnRadioChanged));
}

static void wxColour2hsv(const wxColour& color,
                         unsigned char &h, unsigned char &s, unsigned char &v)
{
    int r = color.Red();
    int g = color.Green();
    int b = color.Blue();
    v = max(max(r, g), b);
    h = s = 0;
    if (v == 0)
        return;
    unsigned char delta = v - min(min(r, g), b);
    s = 255 * delta / v;
    if (s == 0)
        return;
    if (v == r)
        h = 43 * (g - b) / delta;
    else if (v == g)
        h = 85 + 43 * (b - r) / delta;
    else  //  v == b
        h = 171 + 43 * (r - g) / delta;
}

static wxColour hsv2wxColour(unsigned char h, unsigned char s, unsigned char v)
{
    if (s == 0)
        return wxColour(v, v, v);

    double hx = h / 42.501;      // 0 <= hx <= 5.
    int i = (int) floor(hx);
    double f = hx - i;
    double sx = s / 255.;
    unsigned char p = iround(v * (1 - sx));
    unsigned char q = iround(v * (1 - sx * f));
    unsigned char t = iround(v * (1 - sx * (1 - f)));

    switch(i) {
        case 0:
            return wxColour(v, t, p);
        case 1:
            return wxColour(q, v, p);
        case 2:
            return wxColour(p, v, t);
        case 3:
            return wxColour(p, q, v);
        case 4:
            return wxColour(t, p, v);
        case 5:
        default:
            return wxColour(v, p, q);
    }
}


wxColour GradientDlg::get_value(double x)
{
    wxColour c;
    if (x < 0)
        x = 0;
    if (x > 1)
        x = 1;
    if (rb1_->GetValue() || rb2_->GetValue()) { // hsv
        unsigned char h1, s1, v1, h2, s2, v2;
        wxColour2hsv(from_cp_->GetColour(), h1, s1, v1);
        wxColour2hsv(to_cp_->GetColour(), h2, s2, v2);
        int corr = 0;
        if (rb1_->GetValue() && h1 > h2)
            corr = 256;
        else if (rb2_->GetValue() && h1 < h2)
            corr = -256;
        c = hsv2wxColour(iround(h1 * (1-x) + (h2 + corr) * x),
                         iround(s1 * (1-x) + s2 * x),
                         iround(v1 * (1-x) + v2 * x));
    } else if (rb3_->GetValue()) { // rgb
        wxColour c1 = from_cp_->GetColour();
        wxColour c2 = to_cp_->GetColour();
        c = wxColour(iround(c1.Red() * (1-x) + c2.Red() * x),
                     iround(c1.Green() * (1-x) + c2.Green() * x),
                     iround(c1.Blue() * (1-x) + c2.Blue() * x));
    } else { // one color
        c = from_cp_->GetColour();
    }
    return c;
}


const wxColour MultiColorCombo::palette[21] =
{
    wxColour(0x00, 0xCC, 0x00), // green
    wxColour(0x00, 0xBB, 0xFF), // light blue
    wxColour(0xFF, 0xBB, 0x00), // orange
    wxColour(0xEE, 0xEE, 0xBB), // beige
    wxColour(0x55, 0x88, 0x00), // dark green
    wxColour(0x00, 0x44, 0x99), // dark blue
    wxColour(0xDD, 0x00, 0x00), // red
    wxColour(0xFF, 0x00, 0xFF), // magenta
    wxColour(0xEE, 0xEE, 0x00), // yellow
    wxColour(0x00, 0x80, 0x80), // dark cyan
    wxColour(0xC0, 0xDC, 0xC0), // money green
    wxColour(0xFF, 0xFB, 0xF0), // cream
    wxColour(0x80, 0x00, 0x00), // dark red
    wxColour(0x00, 0x80, 0x00), // dark green
    wxColour(0x80, 0x80, 0x00), // dark yellow
    wxColour(0x00, 0x00, 0x80), // dark blue
    wxColour(0x80, 0x00, 0x80), // dark magenta
    wxColour(0x00, 0x00, 0xFF), // blue
    wxColour(0x00, 0xFF, 0xFF), // cyan
    wxColour(0xA6, 0xCA, 0xF0), // sky blue
    wxColour(0x00, 0xFF, 0x00), // green
};


MultiColorCombo::MultiColorCombo(wxWindow* parent, const wxColour* bg_color,
                                 vector<wxColour>& colors)
    : bg_color_(bg_color), colors_(colors)
{
    wxString choices[8] = {
        wxT(""), // current colors
        wxT(""), // palette
        wxT(""), // this and the next empty items are predefined color maps
        wxT(""),
        wxT(""),
        wxT(""),
        wxT(""),
        wxT("single color..."),
    };
    wxOwnerDrawnComboBox::Create(parent, -1, wxEmptyString,
                                 wxDefaultPosition, wxDefaultSize,
                                 8, choices, wxCB_READONLY);
    Connect(wxEVT_COMMAND_COMBOBOX_SELECTED,
            wxCommandEventHandler(MultiColorCombo::OnSelection));
}

static wxColour jet_color(int t)
{
    if (t < 32) // blue
        return wxColour(0, 0, 127 + 4 * t);
    else if (t < 3*32) // cyan
        return wxColour(0, 4 * (t - 32), 255);
    else if (t < 5*32) // yellow
        return wxColour(4 * (t - 3*32), 255, 255 - 4 * (t - 3*32));
    else if (t < 7*32) // orange
        return wxColour(255, 255 - 4 * (t - 5*32), 0);
    else // red
        return wxColour(255 - 4 * (t - 7*32), 0,  0);
}

// t - position in gradient/palette (0 <= t <= 255)
wxColour MultiColorCombo::get_color(int selection, int i) const
{
    int t = 255 * i / (colors_.size() - 1);
    // some color maps here are copied from AtomEye (public domain code)
    switch (selection) {
        case 0: // current colors
            return colors_[i];
        case 1: // palette
            return palette[i < 21 ? i : 0];
        case 2: // from cyan to magenta
            return wxColour(t, 255-t, 255);
        case 3: // from green to blue
            return wxColour(0, 255-t, (255+t)/2);
        case 4: // from copper orange to black
            return wxColour(min(iround(1.25*(255-t)), 255),
                            iround(0.7812*(255-t)),
                            iround(0.4975*(255-t)));
        case 5: // hsv from blue to red
            return jet_color(t);
        case 6: // hsv from green to green
            return hsv2wxColour((t+64) % 255, 255, 255);
        default:
            assert(0);
            return wxColour();
    }
}

void MultiColorCombo::OnSelection(wxCommandEvent& event)
{
    int n = event.GetSelection();
    if (n == 0) // current colors, nothing changes
        return;
    if (n == 7) { // single color...
        if (change_color_dlg(colors_[0]))
            for (size_t i = 1; i < colors_.size(); ++i)
                colors_[i] = colors_[0];
    } else
        for (size_t i = 0; i < colors_.size(); ++i)
            colors_[i] = get_color(n, i);
    SetSelection(0);
    event.Skip();
}


void MultiColorCombo::OnDrawItem(wxDC& dc, const wxRect& rect,
                                 int item, int /*flags*/) const
{
    if (item == wxNOT_FOUND)
        return;
    if (item >= 7) {
        dc.DrawText(GetString(item), rect.x + 3,
                    rect.y + (rect.height - dc.GetCharHeight()) / 2);
        return;
    }
    wxRect bg_rect(rect);
    bg_rect.Deflate(2);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(*bg_color_));
    //dc.SetPen(bg_pen);
    dc.DrawRectangle(bg_rect.x, bg_rect.y, bg_rect.width, bg_rect.height);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    int y1 = bg_rect.y + 3;
    int y2 = bg_rect.y + bg_rect.height - 3;
    for (size_t i = 0; i < colors_.size(); ++i) {
        dc.SetPen(get_color(item, i));
        int x = bg_rect.x + 3 + 2*i;
        dc.DrawLine(x, y1, x, y2);
        ++x;
        dc.DrawLine(x, y1, x, y2);
    }
}

// if the text is empty, wxComboCtrlBase::DoGetBestSize() sets text width
// 150px. Let's decrease it to 142px.
wxSize MultiColorCombo::DoGetBestSize() const
{
    wxSize s = wxComboCtrlBase::DoGetBestSize();
    s.x -= 8;
    CacheBestSize(s);
    return s;
}

