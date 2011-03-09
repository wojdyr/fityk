// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/hyperlink.h>
#include <fstream>

#include "dataedit.h"

#include "frame.h"
#include "app.h" // get_full_path_of_help_file()
#include "dialogs.h" //export_data_dlg()
#include "../data.h" // Data, Point
#include "../logic.h"

using namespace std;


// ';' will be replaced by line break
static const char *default_transforms[] = {

"std.dev.=1"
"||"
"|s=1"
"|Y",

"std.dev.=sqrt(y)"
"||std.dev. = sqrt(y) (or 1 if y<1)"
"|s=sqrt(max2(1,y))"
"|Y",

"integrate"
"||"
"|Y = Y[n-1] + (x[n] - x[n-1]) * (y[n-1] + y[n]) / 2"
"|Y",

"differentiate"
"||compute numerical derivative f'(x)"
"|Y = (y[n+1]-y[n])/(x[n+1]-x[n])"
";X = (x[n+1]+x[n])/2"
";M=M-1"
";S = sqrt(max2(1,y))"
"|Y",

"accumulate"
"||Accumulate y of data and adjust std. dev."
"|Y = Y[n-1] + y[n]"
";S = sqrt(max2(1,y))"
"|Y",

"normalize area"
"||"
"divide all Y (and std. dev.) values by the current data area"
" (it gives unit area)"
"|Y = y/darea(y), S = s / darea(y)"
"|Y",

"reduce 2x"
"||join every two adjacent points"
"|X = (x[n]+x[n+1])/2"
";Y = y[n]+y[n+1]"
";S = s[n]+s[n+1]"
";delete(mod(n,2)==1)"
"|Y",

"equilibrate step"
"||make equal step, keep the number of points"
"|X = x[0] + n * (x[M-1]-x[0]) / (M-1), Y = y[index(X)], S = s[index(X)], A = a[index(X)]"
"|Y",

"zero negative y"
"||zero the Y value; of points with negative Y"
"|Y=max2(y,0)"
"|Y",

"clear inactive"
"||delete inactive points"
"|delete(not a)"
"|Y",

"swap axes"
"||Swap X and Y axes and adjust std. dev."
"|Y=x , X=y , S=sqrt(max2(1,Y))"
"|N",

"generate sinusoid"
"||replaces current data with sinusoid"
"|M=2000"
";x=n/100"
";y=sin(x)"
";s=1"
"|N",

"invert"
"||inverts y value of points"
"|Y=-y"
"|N",

"activate all"
"||activate all data points"
"|a=true"
"|N",

"Q -> 2theta(Cu)"
"||rescale X axis (for powder diffraction patterns)"
"|X = asin(x/(4*pi)*1.54051) * 2*180/pi"
"|N",

"2theta(Cu) -> Q"
"||rescale X axis (for powder diffraction patterns)"
"|X = 4*pi * sin(x/2*pi/180) / 1.54051"
"|N"
};

DataTransform::DataTransform(const string& line)
     : in_menu(false), is_changed(false)
{
    vector<string> tokens = split_string(line, '|');
    if (tokens.size() < 4)
        return;
    for (vector<string>::iterator i = tokens.begin(); i != tokens.end(); ++i)
        replace(i->begin(), i->end(), ';', '\n');
    name = s2wx(tokens[0]);
    //category = tokens[1]; // category field has been removed
    description = s2wx(tokens[2]);
    code = s2wx(tokens[3]);
    in_menu = (tokens[4] == "Y");
}

// as a side effect, | is changed to / in all string members
string DataTransform::as_fileline()
{
    name.Replace(wxT("|"), wxT("/"));
    description.Replace(wxT("|"), wxT("/"));
    code.Replace(wxT("|"), wxT("/"));
    // The second field is reserved. It used to be "category".
    wxString s = name + wxT("||") + description + wxT("|") + code
               + wxT("|") + (in_menu ? wxT("Y") : wxT("N"));
    s.Replace(wxT("\n"), wxT(";"));
    return wx2s(s);
}


std::vector<DataTransform> EditTransDlg::transforms;

EditTransDlg::EditTransDlg (wxWindow* parent, wxWindowID id,
                              ndnd_type const& dd)
    : wxDialog(parent, id, wxT("Data Transformations"),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
      ndnd(dd)
{
    init();
}

void EditTransDlg::init()
{
    ProportionalSplitter *splitter = new ProportionalSplitter(this, -1, 0.5);

    // left side of the dialog
    wxPanel *left_panel = new wxPanel(splitter);
    wxBoxSizer *left_sizer = new wxBoxSizer(wxVERTICAL);
    left_sizer->Add(new wxStaticText(left_panel, -1,
                                     wxT("Transformations in menu:")),
                    wxSizerFlags().Border(wxLEFT|wxRIGHT|wxTOP));
    wxBoxSizer *lh_sizer = new wxBoxSizer(wxHORIZONTAL);
    trans_list = new wxCheckListBox(left_panel, -1);
    trans_list->SetMinSize(wxSize(200, -1));
    lh_sizer->Add(trans_list, wxSizerFlags(1).Expand().Border());
    // buttons on the right of the list
    wxBoxSizer *lbutton_sizer = new wxBoxSizer(wxVERTICAL);
    add_btn = new wxButton(left_panel, wxID_ADD);
    lbutton_sizer->Add(add_btn, wxSizerFlags().Border());
    remove_btn = new wxButton(left_panel, wxID_REMOVE);
    lbutton_sizer->Add(remove_btn, wxSizerFlags().Border());
    up_btn = new wxButton(left_panel, wxID_UP);
    lbutton_sizer->Add(up_btn, wxSizerFlags().Border());
    down_btn = new wxButton(left_panel, wxID_DOWN);
    lbutton_sizer->Add(down_btn, wxSizerFlags().Border());
    lh_sizer->Add(lbutton_sizer, wxSizerFlags().Center());
    left_sizer->Add(lh_sizer, wxSizerFlags(1).Expand());
    left_panel->SetSizerAndFit(left_sizer);

    // right side of the dialog
    wxPanel *right_panel = new wxPanel(splitter);
    wxBoxSizer *right_sizer = new wxBoxSizer(wxVERTICAL);
    right_sizer->AddSpacer(10);
    wxBoxSizer *name_sizer = new wxBoxSizer(wxHORIZONTAL);
    name_sizer->Add(new wxStaticText(right_panel, -1, wxT("Name:")),
                    wxSizerFlags().Border(wxLEFT).Centre());
    name_tc = new wxTextCtrl(right_panel, -1, wxEmptyString);
    name_sizer->Add(name_tc, wxSizerFlags(1).Border());
    right_sizer->Add(name_sizer, wxSizerFlags().Expand());
    right_sizer->Add(new wxStaticText(right_panel, -1,
                                      wxT("Description (optional):")),
                     wxSizerFlags().Border(wxLEFT|wxRIGHT|wxTOP));
    description_tc = new wxTextCtrl(right_panel, -1, wxEmptyString,
                                    wxDefaultPosition, wxSize(-1, 50),
                                    wxTE_MULTILINE);
    right_sizer->Add(description_tc, wxSizerFlags().Expand().Border());
    wxBoxSizer *cl_sizer = new wxBoxSizer(wxHORIZONTAL);
    cl_sizer->Add(new wxStaticText(right_panel, -1, wxT("Code:")),
                  wxSizerFlags().Border().Center());
    wxString help_url = get_help_url(wxT("data.html"))
#ifndef __WXMSW__
          // wxMSW converts URI back to filename, and it can't handle #fragment
          + wxT("#data-point-transformations")
#endif
          ;
    wxHyperlinkCtrl *help_ctrl = new wxHyperlinkCtrl(right_panel, -1,
                                                  wxT("syntax reference"),
                                                  help_url);
    cl_sizer->AddStretchSpacer();
    cl_sizer->Add(help_ctrl, wxSizerFlags().Center().Border(wxRIGHT));
    right_sizer->Add(cl_sizer, wxSizerFlags().Expand());
    code_tc = new wxTextCtrl(right_panel, -1, wxEmptyString,
                             wxDefaultPosition, wxSize(-1, 100),
                             wxTE_MULTILINE|wxHSCROLL|wxVSCROLL);
    right_sizer->Add(code_tc,
                     wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT|wxBOTTOM));

    wxString t = (ndnd.size() == 1
                    ? wxString::Format(wxT("@%d"), ndnd[0].first)
                    : wxString::Format(wxT("%d datasets"), (int) ndnd.size()));
    wxSizer *apply_sizer = new wxStaticBoxSizer(wxHORIZONTAL, right_panel,
                                                wxT("Apply to ") + t);
    apply_btn = new wxButton(right_panel, wxID_APPLY);
    rezoom_btn = new wxButton(right_panel, wxID_ZOOM_FIT);
    undo_btn = new wxButton(right_panel, wxID_UNDO);
    undo_btn->SetToolTip(wxT("Read the dataset from file again (if possible)"));
    apply_sizer->Add(apply_btn, wxSizerFlags().Border());
    apply_sizer->Add(rezoom_btn, wxSizerFlags().Border());
    apply_sizer->Add(undo_btn, wxSizerFlags().Border());
    right_sizer->Add(apply_sizer, wxSizerFlags().Border().Expand());
    right_panel->SetSizerAndFit(right_sizer);

    /*
    help_btn = new wxButton(left_panel, wxID_HELP);
    */

    splitter->SplitVertically(left_panel, right_panel);
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    top_sizer->Add(splitter, wxSizerFlags(1).Expand());
    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 10);

    // buttons at the bottom of the dialog
    wxBoxSizer *bb_sizer = new wxBoxSizer(wxHORIZONTAL);
    save_btn = new wxButton(this, wxID_SAVE);
    bb_sizer->Add(save_btn, wxSizerFlags().Border());
    revert_btn = new wxButton(this, wxID_REVERT_TO_SAVED);
    bb_sizer->Add(revert_btn, wxSizerFlags().Border());
    todefault_btn = new wxButton(this, -1, wxT("Restore &to Default"));
    todefault_btn->SetToolTip(wxT("Load default transformations."));
    bb_sizer->Add(todefault_btn, wxSizerFlags().Border());
    bb_sizer->AddStretchSpacer();
    bb_sizer->Add(new wxButton(this, wxID_CLOSE, wxT("&Close")),
                  wxSizerFlags().Border().Right());
    top_sizer->Add(bb_sizer, wxSizerFlags().Expand());
    SetSizerAndFit(top_sizer);
    SetEscapeId(wxID_CLOSE);

    // workaround for wxMSW 2.5.3 strange problem -- very small dialog window
    if (GetClientSize().GetHeight() < 200)
        SetClientSize(500, 500);

    Centre();

    initialize_checklist();
    update_right_side();
    update_apply_button();

    Connect(wxID_ADD, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(EditTransDlg::OnAdd));
    Connect(wxID_REMOVE, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(EditTransDlg::OnRemove));
    Connect(wxID_UP, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(EditTransDlg::OnUp));
    Connect(wxID_DOWN, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(EditTransDlg::OnDown));
    Connect(wxID_SAVE, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(EditTransDlg::OnSave));
    Connect(wxID_REVERT_TO_SAVED, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(EditTransDlg::OnRevert));
    Connect(todefault_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(EditTransDlg::OnToDefault));
    Connect(wxID_APPLY, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(EditTransDlg::OnApply));
    Connect(wxID_ZOOM_FIT, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(EditTransDlg::OnReZoom));
    Connect(wxID_UNDO, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(EditTransDlg::OnUndo));
    Connect(name_tc->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            wxCommandEventHandler(EditTransDlg::OnNameText));
    Connect(description_tc->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            wxCommandEventHandler(EditTransDlg::OnDescText));
    Connect(code_tc->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            wxCommandEventHandler(EditTransDlg::OnCodeText));
    Connect(trans_list->GetId(), wxEVT_COMMAND_LISTBOX_SELECTED,
            wxCommandEventHandler(EditTransDlg::OnListItemSelected));
    Connect(trans_list->GetId(), wxEVT_COMMAND_CHECKLISTBOX_TOGGLED,
            wxCommandEventHandler(EditTransDlg::OnListItemToggled));
}

void EditTransDlg::read_transforms(bool skip_file)
{
    transforms.clear();
    wxString transform_path = get_conf_file("transform");
    if (wxFileExists(transform_path) && !skip_file) {
        ifstream f((const char*) transform_path.mb_str());
        string t_line;
        while (getline(f, t_line))
            transforms.push_back(DataTransform(t_line));
    }
    else {
        int n = sizeof(default_transforms) / sizeof(default_transforms[0]);
        for (int i = 0; i != n; ++i)
            transforms.push_back(DataTransform(default_transforms[i]));
    }
}

void EditTransDlg::initialize_checklist()
{
    wxArrayString array;
    array.Alloc(transforms.size());
    for (vector<DataTransform>::const_iterator i = transforms.begin();
                                                i != transforms.end(); ++i)
        array.Add(i->name);
    trans_list->Set(array);
    for (size_t i = 0; i != transforms.size(); ++i)
        trans_list->Check(i, transforms[i].in_menu);
}


void EditTransDlg::OnAdd(wxCommandEvent&)
{
    wxString name;
    for (int i = 1; ; ++i) {
        name.Printf(wxT("transform %d"), i);
        bool uniq = true;
        for (vector<DataTransform>::const_iterator i = transforms.begin();
                                                i != transforms.end(); ++i)
            if (i->name == name) {
                uniq = false;
                break;
            }
        if (uniq)
            break;
    }
    DataTransform new_transform(name, wxEmptyString, wxEmptyString);
    int pos = trans_list->GetSelection() + 1;
    transforms.insert(transforms.begin() + pos, new_transform);
    trans_list->Insert(name, pos);
    trans_list->SetSelection(pos);
    update_right_side();
    apply_btn->Enable(false);
    name_tc->SetFocus();
    name_tc->SetSelection(-1, -1);
}

void EditTransDlg::OnRemove(wxCommandEvent&)
{
    int item = trans_list->GetSelection();
    if (item == wxNOT_FOUND)
        return;
    transforms.erase(transforms.begin() + item);
    trans_list->Delete(item);
    trans_list->SetSelection(item > 0 ? item-1 : 0);
    update_right_side();
}

void EditTransDlg::OnUp(wxCommandEvent&)
{
    int item = trans_list->GetSelection();
    if (item == wxNOT_FOUND || item == 0)
        return;
    swap(transforms[item-1], transforms[item]);
    trans_list->SetString(item-1, transforms[item-1].name);
    trans_list->Check(item-1, transforms[item-1].in_menu);
    trans_list->SetString(item, transforms[item].name);
    trans_list->Check(item, transforms[item].in_menu);
    trans_list->SetSelection(item-1);
    up_btn->Enable(item-1 > 0);
    down_btn->Enable(true);
}

void EditTransDlg::OnDown(wxCommandEvent&)
{
    int item = trans_list->GetSelection();
    if (item == wxNOT_FOUND || item >= size(transforms) - 1)
        return;
    swap(transforms[item], transforms[item+1]);
    trans_list->SetString(item, transforms[item].name);
    trans_list->Check(item, transforms[item].in_menu);
    trans_list->SetString(item+1, transforms[item+1].name);
    trans_list->Check(item+1, transforms[item+1].in_menu);
    trans_list->SetSelection(item+1);
    up_btn->Enable(true);
    down_btn->Enable(item+1 < (int) trans_list->GetCount() - 1);
}

void EditTransDlg::OnSave(wxCommandEvent&)
{
    wxString transform_path = get_conf_file("transform");
    ofstream f((const char*) transform_path.mb_str());
    for (vector<DataTransform>::iterator i = transforms.begin();
                                            i != transforms.end(); ++i) {
        f << i->as_fileline() << endl;
        i->is_changed = false;
    }
    for (size_t i = 0; i != transforms.size(); ++i) {
        trans_list->SetString(i, transforms[i].name);
        trans_list->Check(i, transforms[i].in_menu);
    }
}

void EditTransDlg::OnRevert(wxCommandEvent&)
{
    read_transforms(false);
    initialize_checklist();
    update_right_side();
}

void EditTransDlg::OnToDefault(wxCommandEvent&)
{
    read_transforms(true);
    initialize_checklist();
    update_right_side();
}

void EditTransDlg::OnApply(wxCommandEvent&)
{
    string code = wx2s(code_tc->GetValue());
    execute_tranform(code);
}

void EditTransDlg::OnReZoom(wxCommandEvent&)
{
    frame->GViewAll();
}

void EditTransDlg::OnUndo(wxCommandEvent& event)
{
    frame->OnDataRevert(event);
    // TODO: Real undo. This would require additional commands, e.g. 
    // @n.mark, @n.rollback
}

void EditTransDlg::OnNameText(wxCommandEvent&)
{
    int item = trans_list->GetSelection();
    if (item == wxNOT_FOUND)
        return;
    transforms[item].name = name_tc->GetValue();
    trans_list->SetString(item, transforms[item].get_display_name());
    trans_list->Check(item, transforms[item].in_menu);
}

void EditTransDlg::OnDescText(wxCommandEvent&)
{
    int item = trans_list->GetSelection();
    if (item == wxNOT_FOUND)
        return;
    transforms[item].description = description_tc->GetValue();
    if (!transforms[item].is_changed) {
        transforms[item].is_changed = true;
        trans_list->SetString(item, transforms[item].get_display_name());
        trans_list->Check(item, transforms[item].in_menu);
    }
}

void EditTransDlg::OnCodeText(wxCommandEvent&)
{
    int item = trans_list->GetSelection();
    if (item == wxNOT_FOUND)
        return;
    update_apply_button();
    transforms[item].code = code_tc->GetValue();
    if (!transforms[item].is_changed) {
        transforms[item].is_changed = true;
        trans_list->SetString(item, transforms[item].get_display_name());
        trans_list->Check(item, transforms[item].in_menu);
    }
}

void EditTransDlg::OnListItemToggled(wxCommandEvent& event)
{
    int item = event.GetInt();
    transforms[item].in_menu = trans_list->IsChecked(item);
}

void EditTransDlg::execute_tranform(string const& code)
{
    string t = conv_code_to_one_line(code);
    ftk->exec(frame->get_datasets() + t);
}

bool EditTransDlg::update_apply_button()
{
    string code = wx2s(code_tc->GetValue());
    string text = conv_code_to_one_line(code);
    bool ok = ftk->get_ui()->check_syntax(text);
    apply_btn->Enable(ok);
    return ok;
}

string EditTransDlg::conv_code_to_one_line(string const& code)
{
    string cmd;
    const char* p = code.c_str();
    for (;;) {
        while(isspace(*p))
            ++p;
        while (*p == '#') {
            while(*p != '\0' && *p != '\n')
                ++p;
            while(isspace(*p))
                ++p;
        }
        if (*p == '\0')
            break;
        const char* start = p;
        while (*p != '\0' && *p != '#' && *p != '\n')
            ++p;
        if (!cmd.empty())
            cmd += "; ";
        cmd += string(start, p);
    }
    return cmd;
}

void EditTransDlg::update_right_side()
{
    int item = trans_list->GetSelection();
    if (item == wxNOT_FOUND) {
        item = 0;
        trans_list->SetSelection(0);
    }
    const DataTransform& dt = transforms[item];
    name_tc->ChangeValue(dt.name);
    description_tc->ChangeValue(dt.description);
    code_tc->ChangeValue(dt.code);

    up_btn->Enable(item > 0);
    down_btn->Enable(item < (int) trans_list->GetCount() - 1);
    //remove_btn->Enable(item >= 0);
    apply_btn->Enable(true);
}

