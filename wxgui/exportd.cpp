// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// export_data_dlg() and export_peak_parameters()
/// which are called from the GUI menu.

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/statline.h>

#include "exportd.h"
#include "cmn.h"
#include "frame.h" //ftk

#include "fityk/logic.h"
#include "fityk/ui.h"
#include "fityk/model.h" // get_ff()

using namespace std;


ExtraCheckBox::ExtraCheckBox(wxWindow* parent, const wxString& label,
                             bool value)
    : wxPanel(parent, -1)
{
    cb = new wxCheckBox(this, -1, label);
    cb->SetValue(value);
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
#ifdef __WXMSW__
    sizer->AddSpacer(100);
    sizer->Add(cb, 0, wxLEFT|wxBOTTOM, 5);
#else
    sizer->Add(cb);
#endif
    SetSizerAndFit(sizer);
}

// helper for export_data_dlg()
class DataExportDlg : public wxDialog
{
public:
    DataExportDlg(wxWindow* parent, wxWindowID id, int f_count);
    std::string get_text() { return wx2s(text->GetValue()); }

private:
    void OnCheckChanged(wxCommandEvent&) { on_widget_change(); }
    void OnInactiveChanged(wxCommandEvent&) { on_widget_change(); }
    void OnTextChanged(wxCommandEvent&);
    void OnOk(wxCommandEvent& event);
    void on_widget_change();

    wxCheckListBox *list;
    wxCheckBox *only_a_cb;
    wxTextCtrl *text;
    wxArrayString cv;
};

DataExportDlg::DataExportDlg(wxWindow* parent, wxWindowID id, int f_count)
    : wxDialog(parent, id, wxT("Export data/functions as points"),
               wxDefaultPosition, wxSize(600, 500),
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *st1 = new wxStaticText(this, -1,
                                         wxT("       Step 1: Select columns"));
    top_sizer->Add(st1, 0, wxTOP|wxLEFT|wxRIGHT, 5);
    wxStaticText *st2 = new wxStaticText(this, -1,
                                         wxT("       Step 2: Choose a file"));
    st2->Enable(false);
    top_sizer->Add(st2, 0, wxALL, 5);
    wxArrayString choices;
    choices.Add(wxT("x"));
    cv.Add(wxT("x"));
    choices.Add(wxT("y"));
    cv.Add(wxT("y"));
    choices.Add(wxT("\u03C3 (std. dev. of y)"));
    cv.Add(wxT("s"));
    choices.Add(wxT("active (0/1)"));
    cv.Add(wxT("a"));

    choices.Add(wxT("all component functions"));
    wxString all_func;
    for (int i = 0; i < f_count; ++i) {
        if (!all_func.empty())
            all_func += wxT(", ");
        all_func +=  wxString::Format(wxT("F[%d](x)"), i);
    }
    cv.Add(all_func);

    choices.Add(wxT("model (sum)"));
    cv.Add(wxT("F(x)"));
    choices.Add(wxT("x-correction"));
    cv.Add(wxT("Z(x)"));

    choices.Add(wxT("residuals"));
    cv.Add(wxT("y-F(x)"));
    choices.Add(wxT("absolute residuals"));
    cv.Add(wxT("abs(y-F(x))"));
    choices.Add(wxT("weighted residuals"));
    cv.Add(wxT("(y-F(x))/s"));

    list = new wxCheckListBox(this, -1, wxDefaultPosition, wxDefaultSize,
                              choices);
    top_sizer->Add(list, 0, wxALL|wxEXPAND, 5);
    only_a_cb = new wxCheckBox(this, -1, wxT("only active points"));
    top_sizer->Add(only_a_cb, 0, wxALL, 5);
    text = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxSize(300,-1));
    top_sizer->Add(text, 0, wxEXPAND|wxALL, 5);
    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL),
                   0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);

    Connect(wxID_OK, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(DataExportDlg::OnOk));
    Connect(list->GetId(), wxEVT_COMMAND_CHECKLISTBOX_TOGGLED,
            wxCommandEventHandler(DataExportDlg::OnCheckChanged));
    Connect(only_a_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(DataExportDlg::OnInactiveChanged));
    Connect(text->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            wxCommandEventHandler(DataExportDlg::OnTextChanged));

    wxString t = wxConfig::Get()->Read(wxT("/exportPoints"),
                                       wxT("all: x, y, F(x)"));
    text->SetValue(t);
}


void DataExportDlg::on_widget_change()
{
    wxString s = (only_a_cb->GetValue() ? wxT("if a: ") : wxT("all: "));
    for (size_t i = 0; i < list->GetCount(); ++i) {
        if (list->IsChecked(i)) {
            if (!s.EndsWith(wxT(": ")) && !cv[i].empty())
                    s += wxT(", ");
            s += cv[i];
        }
    }
    text->ChangeValue(s);
    FindWindow(wxID_OK)->Enable(true);
}

void DataExportDlg::OnTextChanged(wxCommandEvent&)
{
    //if (!text->IsModified())
    //    return;
    string s = wx2s(text->GetValue());
    size_t colon = s.find(':');
    bool parsable = (ftk->check_syntax("print " + s) && colon != string::npos);
    FindWindow(wxID_OK)->Enable(parsable);
    if (parsable) {
        only_a_cb->SetValue(s.substr(0, colon) == "if a");
        vector<string> v = split_string(s.substr(colon + 1), ',');
        vm_foreach (string, i, v)
            *i = strip_string(*i);
        for (size_t i = 0; i < list->GetCount(); ++i)
            list->Check(i, contains_element(v, wx2s(cv[i])));
    }
}

void DataExportDlg::OnOk(wxCommandEvent& event)
{
    wxConfig::Get()->Write(wxT("/exportPoints"), text->GetValue());
    event.Skip();
}


void exec_redirected_command(const vector<int>& sel,
                             const string& cmd, const wxString& path)
{
    if (sel.size() == 1) {
        exec("@" + S(sel[0]) + ": " + cmd + " > '" + wx2s(path) + "'");
        return;
    }
    string datasets;
    if (ftk->dk.count() == (int) sel.size())
        datasets = "@*";
    else
        datasets = "@" + join_vector(sel, " @");
    if (wxFileExists(path))
        exec("delete file '" + wx2s(path) + "'");
    exec(datasets + ": " + cmd + " >> '" + wx2s(path) + "'");
}

static
wxString get_default_name(const vector<int>& sel, const char* ext)
{
    if (sel.size() == 1) {
        const string& filename = ftk->dk.data(sel[0])->get_filename();
        if (!filename.empty())
            return wxFileName(s2wx(filename)).GetName() + ext;
    }
    return "";
}

/// show "Export data" dialog
void export_data_dlg(const vector<int>& sel, wxWindow *parent,
                     wxString *export_dir)
{
    int f_count = 0;
    if (sel.size() == 1) {
        f_count = ftk->dk.get_model(sel[0])->get_ff().names.size();
    }

    DataExportDlg ded(parent, -1, f_count);
    if (ded.ShowModal() != wxID_OK)
        return;

    wxFileDialog fdlg(parent, "Export data to file", *export_dir,
                      get_default_name(sel, ".dat"),
                      "x y data (*.dat, *.xy)|*.dat;*.DAT;*.xy;*.XY",
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fdlg.ShowModal() == wxID_OK)
        exec_redirected_command(sel, "print " + ded.get_text(), fdlg.GetPath());
    *export_dir = fdlg.GetDirectory();
}

static
wxWindow* peakparam_extra(wxWindow* parent)
{
    return new ExtraCheckBox(parent, "include standard errors", false);
}

void export_peak_parameters(const vector<int>& sel, wxWindow *parent,
                            wxString *export_dir)
{
    wxFileDialog fdlg(parent, "Export parameters to file", *export_dir,
                      get_default_name(sel, ".peaks"),
                      "parameters of functions (*.peaks)|*.peaks|all files|*",
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    // on wxOSX/Carbon SetExtraControlCreator() is unreliable
#ifndef __WXMAC__
    fdlg.SetExtraControlCreator(&peakparam_extra);
#endif
    if (fdlg.ShowModal() == wxID_OK) {
        wxWindow *extra = fdlg.GetExtraControl();
        if (extra != NULL && wxDynamicCast(extra,ExtraCheckBox)->is_checked())
            exec_redirected_command(sel, "info peaks_err", fdlg.GetPath());
        else
            exec_redirected_command(sel, "info peaks", fdlg.GetPath());
    }
    *export_dir = fdlg.GetDirectory();
}

