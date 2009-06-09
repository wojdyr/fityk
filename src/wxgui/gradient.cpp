// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

/// In this file:
///  Gradient Dialog (GradientDlg) and helpers

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/spinctrl.h>

#include "gradient.h"
#include "cmn.h" //SpinCtrl, change_color_dlg, add_apply_close_buttons, iround
#include "../common.h" //iround()

using namespace std;

enum {
    ID_CGD_RADIO               = 27900
};

BEGIN_EVENT_TABLE(ColorSpinSelector, wxPanel)
    EVT_BUTTON (-1, ColorSpinSelector::OnSelector)
END_EVENT_TABLE()

ColorSpinSelector::ColorSpinSelector(wxWindow *parent, wxString const& title,
                                     wxColour const& col)
    : wxPanel(parent, -1)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, title);
    sizer->Add(new wxStaticText(this, -1, wxT("R")),
                    0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    r = new SpinCtrl(this, -1, col.Red(), 0, 255);
    sizer->Add(r, 0, wxALL, 5);
    sizer->Add(new wxStaticText(this, -1, wxT("G")),
                    0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    g = new SpinCtrl(this, -1, col.Green(), 0, 255);
    sizer->Add(g, 0, wxALL, 5);
    sizer->Add(new wxStaticText(this, -1, wxT("B")),
                    0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    b = new SpinCtrl(this, -1, col.Blue(), 0, 255);
    sizer->Add(b, 0, wxALL, 5);
    sizer->Add(new wxButton(this, -1, wxT("Selector...")), 0, wxALL, 5);
    top_sizer->Add(sizer, 1, wxEXPAND);
    SetSizerAndFit(top_sizer);
}

void ColorSpinSelector::OnSelector(wxCommandEvent &)
{
    wxColour c(r->GetValue(), g->GetValue(), b->GetValue());
    if (change_color_dlg(c)) {
        r->SetValue(c.Red());
        g->SetValue(c.Green());
        b->SetValue(c.Blue());
    }
    // parent is notified about changes by handling all wxSpinCtrl events
    wxSpinEvent event(wxEVT_COMMAND_SPINCTRL_UPDATED);
    wxPostEvent(this, event);
}


BEGIN_EVENT_TABLE(GradientDlg, wxDialog)
    EVT_SPINCTRL (-1, GradientDlg::OnSpinEvent)
    EVT_RADIOBOX (ID_CGD_RADIO, GradientDlg::OnRadioChanged)
END_EVENT_TABLE()

GradientDlg::GradientDlg(wxWindow *parent, wxWindowID id,
                         wxColour const& first_col, wxColour const& last_col)
    : wxDialog(parent, id, wxT("Select color gradient"),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE/*|wxRESIZE_BORDER*/)
{
    wxBoxSizer* top_sizer = new wxBoxSizer(wxVERTICAL);

    from = new ColorSpinSelector(this, wxT("from"), first_col);
    top_sizer->Add(from, 0, wxALL, 5);

    to = new ColorSpinSelector(this, wxT("to"), last_col);
    top_sizer->Add(to, 0, wxALL, 5);

    wxArrayString choices;
    choices.Add(wxT("HSV gradient, clockwise hue"));
    choices.Add(wxT("HSV gradient, counter-clockwise"));
    choices.Add(wxT("RGB gradient"));
    choices.Add(wxT("one color"));
    kind_rb = new wxRadioBox(this, ID_CGD_RADIO, wxT("how to extrapolate..."),
                             wxDefaultPosition, wxDefaultSize, choices,
                             1, wxRA_SPECIFY_COLS);
    top_sizer->Add(kind_rb, 0, wxALL|wxEXPAND, 5);
    display = new ColorGradientDisplay<GradientDlg>(this,
                                 this, &GradientDlg::update_gradient_display);
    display->SetMinSize(wxSize(-1, 15));
    top_sizer->Add(display, 0, wxALL|wxEXPAND, 5);

    add_apply_close_buttons(this, top_sizer);
    SetSizerAndFit(top_sizer);
    update_gradient_display();
}

void GradientDlg::update_gradient_display()
{
    int display_width = display->GetClientSize().GetWidth();
    display->data.resize(display_width);
    for (int i = 0; i < display_width; ++i)
        display->data[i] = get_value(i / (display_width-1.0));
    display->Refresh();
}

static void rgb2hsv(unsigned char r, unsigned char g, unsigned char b,
                    unsigned char &h, unsigned char &s, unsigned char &v)
{
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

    float hx = h / 42.501;      // 0 <= hx <= 5.
    int i = (int) floor(hx);
    float f = hx - i;
    float sx = s / 255.;
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


wxColour GradientDlg::get_value(float x)
{
    wxColour c;
    if (x < 0)
        x = 0;
    if (x > 1)
        x = 1;
    int kind = kind_rb->GetSelection();
    if (kind == 0 || kind == 1) { //hsv
        unsigned char h1, s1, v1, h2, s2, v2;
        rgb2hsv(from->r->GetValue(), from->g->GetValue(), from->b->GetValue(),
                h1, s1, v1);
        rgb2hsv(to->r->GetValue(), to->g->GetValue(), to->b->GetValue(),
                h2, s2, v2);
        int corr = 0;
        if (kind == 0 && h1 > h2)
            corr = 256;
        else if (kind == 1 && h1 < h2)
            corr = -256;
        c = hsv2wxColour(iround(h1 * (1-x) + (h2 + corr) * x),
                         iround(s1 * (1-x) + s2 * x),
                         iround(v1 * (1-x) + v2 * x));
    }
    else if (kind == 2) { //rgb
        c = wxColour(
                iround(from->r->GetValue() * (1-x) + to->r->GetValue() * x),
                iround(from->g->GetValue() * (1-x) + to->g->GetValue() * x),
                iround(from->b->GetValue() * (1-x) + to->b->GetValue() * x));
    }
    else { //one color
        c = wxColour(from->r->GetValue(), from->g->GetValue(),
                        from->b->GetValue());
    }
    return c;
}

