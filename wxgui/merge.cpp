// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

#include <wx/wx.h>

#include "merge.h"
#include "cmn.h"
#include "frame.h"
#include "fityk/logic.h"
#include "fityk/data.h"

using namespace std;

BEGIN_EVENT_TABLE (MergePointsDlg, wxDialog)
    EVT_CHECKBOX(-1, MergePointsDlg::OnCheckBox)
END_EVENT_TABLE()

MergePointsDlg::MergePointsDlg(wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, wxT("Merge data points"),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    inf = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize,
                         wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE|wxNO_BORDER);
#ifdef __WXGTK__
    wxColour bg_col = wxStaticText::GetClassDefaultAttributes().colBg;
#else
    wxColour bg_col = GetBackgroundColour();
#endif
    inf->SetBackgroundColour(bg_col);
    inf->SetDefaultStyle(wxTextAttr(wxNullColour, bg_col));
    update_info();
    top_sizer->Add(inf, 1, wxEXPAND|wxTOP|wxLEFT|wxRIGHT, 10);
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    dx_cb = new wxCheckBox(this, -1, wxT("merge points with |x1-x2|<"));
    dx_cb->SetValue(true);
    hsizer->Add(dx_cb, 0, wxALIGN_CENTER_VERTICAL);
    dx_val = new RealNumberCtrl(this, -1, ftk->get_settings()->epsilon);
    hsizer->Add(dx_val, 0);
    top_sizer->Add(hsizer, 0, wxALL, 5);
    y_rb = new wxRadioBox(this, -1, wxT("y = "),
                          wxDefaultPosition, wxDefaultSize,
                          ArrayString(wxT("sum"), wxT("avg")),
                          1, wxRA_SPECIFY_ROWS);
    top_sizer->Add(y_rb, 0, wxEXPAND|wxALL, 5);
    focused_data = frame->get_focused_data_index();
    wxString fdstr = wxString::Format(wxT("dataset @%d"), focused_data);
    output_rb = new wxRadioBox(this, -1, wxT("output to ..."),
                               wxDefaultPosition, wxDefaultSize,
                               ArrayString(fdstr, wxT("new dataset")),
                               1, wxRA_SPECIFY_ROWS);
    top_sizer->Add(output_rb, 0, wxEXPAND|wxALL, 5);
    top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL),
                   0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);
}

void MergePointsDlg::update_info()
{
    vector<int> dd = frame->get_selected_data_indices();
    const fityk::Data* data = ftk->dk.data(dd[0]);
    double x_min = data->get_x_min();
    double x_max = data->get_x_max();
    int n = data->points().size();
    wxString dstr = wxString::Format(wxT("@%d"), dd[0]);
    for (size_t i = 1; i < dd.size(); ++i) {
        data = ftk->dk.data(i);
        if (data->get_x_min() < x_min)
            x_min = data->get_x_min();
        if (data->get_x_max() > x_max)
            x_max = data->get_x_max();
        n += data->points().size();
        dstr += wxString::Format(wxT(" @%d"), (int) i);
    }
    wxString s = wxString::Format(wxT("%i data points from: "), n) + dstr;
    s += wxString::Format(wxT("\nx in range (%g, %g)"), x_min, x_max);
    if (dd.size() != 1 && data->get_x_step() != 0.)
        s += wxString::Format(wxT("\nfixed step: %g"), data->get_x_step());
    else
        s += wxString::Format(wxT("\naverage step: %g"), (x_max-x_min) / (n-1));
    inf->SetValue(s);
}

string MergePointsDlg::get_command()
{
    string s;
    if (dx_cb->GetValue()) {
        string eps = wx2s(dx_val->GetValue().Trim());
        if (eps != eS(ftk->get_settings()->epsilon))
            s += "with epsilon=" + eps + " ";
    }
    string dat = output_rb->GetSelection() == 0 ? S(focused_data) : S("+");
    s += "@" + dat + " = ";
    if (dx_cb->GetValue())
        s += y_rb->GetSelection() == 0 ? "sum_same_x" : "avg_same_x";
    s += "(@" + join_vector(frame->get_selected_data_indices(), " and @") + ")";
    return s;
}



