// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_MERGE_H_
#define FITYK_WX_MERGE_H_

class RealNumberCtrl;

class MergePointsDlg : public wxDialog
{
public:
    MergePointsDlg(wxWindow* parent, wxWindowID id=wxID_ANY);
    std::string get_command();
    void update_info();
    void OnCheckBox(wxCommandEvent&) { y_rb->Enable(dx_cb->GetValue()); }

private:
    int focused_data;
    wxRadioBox *y_rb, *output_rb;
    wxCheckBox *dx_cb;
    RealNumberCtrl *dx_val;
    wxTextCtrl *inf;
    DECLARE_EVENT_TABLE()
};

#endif

