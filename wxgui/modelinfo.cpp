// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <wx/wx.h>
//#include <wx/statline.h>
//#include <wx/tooltip.h>
#include <wx/clipbrd.h>

#include "modelinfo.h"
#include "frame.h"
#include "fityk/logic.h"
#include "fityk/model.h"
#include "fityk/info.h"

using namespace std;


ModelInfoDlg::ModelInfoDlg(wxWindow* parent, wxWindowID id)
  : wxDialog(parent, id, wxString(wxT("Model as Formula or Script")),
             wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
}

bool ModelInfoDlg::Initialize()
{
    //  +------------------------------------------------------+
    //  |  _____           [] extra breaks     num fmt: [6][g] |
    //  | [o form] [                                         ] |
    //  | [o gnup] [ mathematical formula (normal or gnuplot ] |
    //  | [o scri] [ variant) or fityk script                ] |
    //  |  -----   [                                         ] |
    //  | [] simp  [                                         ] |
    //  |          [                                         ] |
    //  +------------------------------------------------------+

    extra_space_cb = new wxCheckBox(this, -1, "extra breaks");
    nf = new NumericFormatPanel(this);
    wxArrayString choices;
    choices.Add("formula");
    choices.Add("formula (gnuplot)");
    choices.Add("script (all models)");
    rb = new wxRadioBox(this, -1, "content", wxDefaultPosition, wxDefaultSize,
                        choices, 1, wxRA_SPECIFY_COLS);
    simplify_cb = new wxCheckBox(this, -1, "simplify");
    main_tc = new wxTextCtrl(this, -1, wxEmptyString,
                             wxDefaultPosition, wxDefaultSize,
                             wxTE_MULTILINE|wxTE_RICH|wxTE_READONLY);

    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *format_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *main_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *left_sizer = new wxBoxSizer(wxVERTICAL);
    format_sizer->Add(extra_space_cb, wxSizerFlags().Center().Border());
    format_sizer->Add(nf, wxSizerFlags().Border());
    left_sizer->Add(rb, wxSizerFlags().Expand().Border());
    left_sizer->Add(simplify_cb, wxSizerFlags().Border());
    main_sizer->Add(left_sizer, wxSizerFlags());
    main_sizer->Add(main_tc, wxSizerFlags(1).Expand());
    top_sizer->Add(format_sizer, wxSizerFlags().Right());
    top_sizer->Add(main_sizer, wxSizerFlags(1).Expand());

    wxBoxSizer *btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->Add(new wxButton(this, wxID_COPY),
                   wxSizerFlags().Border());
    btn_sizer->AddStretchSpacer();
    btn_sizer->Add(new wxButton(this, wxID_SAVE),
                   wxSizerFlags().Border());
    btn_sizer->Add(new wxButton(this, wxID_CLOSE),
                   wxSizerFlags().Right().Border());
    top_sizer->Add(btn_sizer, wxSizerFlags().Expand());
    SetSizerAndFit(top_sizer);
    SetSize(wxSize(640, 440));

    SetEscapeId(wxID_CLOSE);

    try {
        update_text();
    } catch (fityk::ExecuteError &e) {
        ftk->ui()->warn(string("Error: ") + e.what());
        return false;
    }

    Connect(-1, wxEVT_COMMAND_RADIOBOX_SELECTED,
            wxCommandEventHandler(ModelInfoDlg::OnRadio));
    Connect(-1, wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(ModelInfoDlg::OnFormatChange));
    Connect(-1, wxEVT_COMMAND_CHOICE_SELECTED,
            wxCommandEventHandler(ModelInfoDlg::OnFormatChange));
    Connect(wxID_COPY, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(ModelInfoDlg::OnCopy));
    Connect(wxID_SAVE, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(ModelInfoDlg::OnSave));

    return true;
}


void ModelInfoDlg::update_text()
{
    string s;
    int sel = rb->GetSelection();
    bool simplify = simplify_cb->GetValue();
    bool extra_breaks = extra_space_cb->GetValue();
    const char* fmt = nf->format().c_str();
    vector<fityk::Data*> datas = frame->get_selected_datas();
    v_foreach (fityk::Data*, i, datas) {
        fityk::Model *model = (*i)->model();
        if (sel == 0 || sel == 1) { // formula or gnuplot formula
            string formula = model->get_formula(simplify, fmt, extra_breaks);
            if (sel == 1)
                formula = fityk::gnuplotize_formula(formula);
            if (i != datas.begin())
                s += "\n";
            s += formula;
        }
        else { // if (sel == 2) // script
            models_as_script(ftk, s, false);
            // currently all models are exported at once
            break;
        }

        if (extra_breaks)
            s += "\n";
    }
    main_tc->SetValue(s2wx(s));

}

void ModelInfoDlg::OnRadio(wxCommandEvent& event)
{
    simplify_cb->Enable(event.GetSelection() <= 1);
    update_text();
}

void ModelInfoDlg::OnCopy(wxCommandEvent&)
{
    wxString sel = main_tc->GetStringSelection();
    if (sel.empty())
        sel = main_tc->GetValue();
    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(sel));
        wxTheClipboard->Close();
    }
}

void ModelInfoDlg::OnSave(wxCommandEvent&)
{
    EndModal(wxID_OK);
}

const string ModelInfoDlg::get_info_cmd() const
{
    string ret;
    int sel = rb->GetSelection();
    bool simplify = simplify_cb->GetValue();
    if (nf->format() != ftk->get_settings()->numeric_format)
        ret = "with numeric_format='" + nf->format() + "' ";
    ret += "info ";
    if (sel == 0)
        ret += (simplify ? "simplified_formula" : "formula");
    else if (sel == 1)
        ret += (simplify ? "simplified_gnuplot_formula" : "gnuplot_formula");
    else // sel == 2
        ret += "models";
    return ret;
}

