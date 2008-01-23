// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

/// In this file:
///  Data Editor (DataEditorDlg) and helpers

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/statline.h>

#include "dataedit.h"
#include "frame.h"
#include "dialogs.h" //export_data_dlg()
#include "../data.h" // Data, Point
#include "../logic.h"  
#include "../datatrans.h" //compile_data_transformation()

using namespace std;


enum {
    ID_DE_GRID              = 26200,
    ID_DE_RESET                    ,
    ID_DE_CODE                     ,
    ID_DE_EXAMPLES                 ,
    ID_DE_REZOOM                   ,
};


class DataTable: public wxGridTableBase
{
public:
    DataTable(Data const* data_, DataEditorDlg *ded_) : wxGridTableBase(), 
                                            data(data_), ded(ded_) {}
    int GetNumberRows() { return data->points().size(); }
    int GetNumberCols() { return 4; }
    bool IsEmptyCell(int /*row*/, int /*col*/) { return false; }

    wxString GetValue(int row, int col) 
    {  
        if (col == 0) 
            return GetValueAsBool(row,col) ? wxT("1") : wxT("0");
        else 
            return wxString::Format(wxT("%g"), GetValueAsDouble(row,col)); 
    }

    void SetValue(int, int, const wxString&) { assert(0); }

    wxString GetTypeName(int /*row*/, int col)
        { return col == 0 ? wxGRID_VALUE_BOOL : wxGRID_VALUE_FLOAT; }

    bool CanGetValueAs(int row, int col, const wxString& typeName)
        { return typeName == GetTypeName(row, col); }

    double GetValueAsDouble(int row, int col)
    { 
        const Point &p = data->points()[row]; 
        switch (col) {
            case 1: return p.x;      
            case 2: return p.y;      
            case 3: return p.sigma;  
            default: assert(0);
        }
    }

    bool GetValueAsBool(int row, int col)
        { assert(col==0); return data->points()[row].is_active; }

    void SetValueAsDouble(int row, int col, double value) 
    { 
        string t;
        switch (col) {
            case 1: t = "X";  break;
            case 2: t = "Y";  break;
            case 3: t = "S";  break;
            default: assert(0);
        }
        ftk->exec(t + "[" + S(row)+"]=" + S(value) + frame->get_in_datasets());
        if (col == 1) // order of items can be changed
            ded->grid->ForceRefresh();
        ded->rezoom_btn->Enable();
    }

    void SetValueAsBool(int row, int col, bool value) 
    { 
        assert(col==0); 
        ftk->exec("A[" + S(row)+"]=" + (value?"true":"false") 
                                                  + frame->get_in_datasets()); 
        ded->rezoom_btn->Enable();
    }

    wxString GetRowLabelValue(int row) {return wxString::Format(wxT("%i"),row);}

    wxString GetColLabelValue(int col) 
    { 
        switch (col) {
            case 0: return wxT("active"); 
            case 1: return wxT("x");      
            case 2: return wxT("y");      
            case 3: return wxT("sigma");  
            default: assert(0);
        }
    }

private:
    Data const* data;
    DataEditorDlg *ded;
};


// ';' will be replaced by line break
static const char *default_transforms = 

"accumulate|useful|Accumulate y of data and adjust std. dev."
"|Y[1...] = Y[n-1] + y[n];S = sqrt(max2(1,y))|Y\n"

"differentiate|useful|compute numerical derivative f'(x)"
"|Y[...-1] = (y[n+1]-y[n])/(x[n+1]-x[n]);X[...-1] = (x[n+1]+x[n])/2;"
"M=M-1;S = sqrt(max2(1,y))|Y\n"

"normalize area|useful|divide all Y (and std. dev.) values;"
"by the current data area; (it gives unit area)"
"|Y = y/darea(y), S = s / darea(y)" 
"|Y\n" 

"reduce 2x|useful|join every two adjacent points"
"|X[...-1] = (x[n]+x[n+1])/2;Y[...-1] = y[n]+y[n+1];"
"S[...-1] = sqrt(s[n]^2+s[n]^2); delete(n%2==1)|Y\n"

"equilibrate step|useful|make equal step, keep the number of points"
"|X = x[0] + n * (x[M-1]-x[0]) / (M-1), Y = y[x=X], S = s[x=X], A = a[x=X]"
"|Y\n"

"zero negative y|useful|zero the Y value; of points with negative Y"
"|Y=max2(y,0)|Y\n"

"clear inactive|useful|delete inactive points"
"|delete(not a)|Y\n"

"swap axes|example|Swap X and Y axes and adjust std. dev."
"|Y=x , X=y , S=sqrt(max2(1,Y))|N\n"

"generate sinusoid|example|replaces current data with sinusoid"
"|M=2000; x=n/100; y=sin(x); s=1|N\n"

"invert|example|inverts y value of points"
"|Y=-y|N\n"

"activate all|example|activate all data points"
"|a=true|N\n"

"Q -> 2theta(Cu)|example|rescale X axis;in powder diffraction pattern"
"|X = asin(x/(4*pi)*1.54051) * 2*180/pi|N\n"

"2theta(Cu) -> Q|example|rescale X axis;in powder diffraction pattern"
"|X = 4*pi * sin(x/2*pi/180) / 1.54051|N\n"
;

DataTransform::DataTransform(string line)
     : in_menu(false) 
{
    replace_all(line, ";", "\n");
    string::size_type pos=0;
    for (int cnt = 0; cnt <= 4; ++cnt) {
        string::size_type new_pos = line.find('|', pos);
        string sub = string(line, pos, new_pos-pos);
        if (cnt == 0) 
            name = sub;
        else if (cnt == 1)
            category = sub;
        else if (cnt == 2)
            description = sub;
        else if (cnt == 3)
            code = sub;
        else if (cnt == 4)
            in_menu = (sub == "Y");
        if (new_pos == string::npos)
            break;
        pos = new_pos + 1;
    }
}

string DataTransform::as_fileline() const
{
    string s = name + "|" + category + "|" + description + "|" + code 
               + "|" + (in_menu ? "Y" : "N");
    replace_all(s, "\n", ";");
    return s;
}


BEGIN_EVENT_TABLE(DataEditorDlg, wxDialog)
    EVT_BUTTON      (wxID_REVERT_TO_SAVED,  DataEditorDlg::OnRevert)
    EVT_BUTTON      (wxID_SAVEAS,           DataEditorDlg::OnSaveAs)
    EVT_BUTTON      (wxID_ADD,              DataEditorDlg::OnAdd)
    EVT_BUTTON      (wxID_REMOVE,           DataEditorDlg::OnRemove)
    EVT_BUTTON      (wxID_UP,               DataEditorDlg::OnUp)
    EVT_BUTTON      (wxID_DOWN,             DataEditorDlg::OnDown)
    EVT_BUTTON      (wxID_SAVE,             DataEditorDlg::OnSave)
    EVT_BUTTON      (ID_DE_RESET,           DataEditorDlg::OnReset)
    EVT_BUTTON      (wxID_APPLY,            DataEditorDlg::OnApply)
    EVT_BUTTON      (ID_DE_REZOOM,          DataEditorDlg::OnReZoom)
    EVT_BUTTON      (wxID_HELP,             DataEditorDlg::OnHelp)
    EVT_BUTTON      (wxID_CLOSE,            DataEditorDlg::OnClose)
    EVT_TEXT        (ID_DE_CODE,            DataEditorDlg::OnCodeText)
    EVT_LIST_ITEM_SELECTED(ID_DE_EXAMPLES,  DataEditorDlg::OnESelected)
    EVT_LIST_ITEM_ACTIVATED(ID_DE_EXAMPLES, DataEditorDlg::OnEActivated)
END_EVENT_TABLE()

DataEditorDlg::DataEditorDlg (wxWindow* parent, wxWindowID id, 
                              ndnd_type const& dd)
    : wxDialog(parent, id, wxT("Data Editor"), 
               wxDefaultPosition, wxSize(500, 500), 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    ProportionalSplitter *splitter = new ProportionalSplitter(this, -1, 0.5);

    // left side of the dialog
    wxPanel *left_panel = new wxPanel(splitter); 
    wxBoxSizer *left_sizer = new wxBoxSizer(wxVERTICAL);
    left_sizer->Add(new wxStaticText(left_panel, -1,wxT("Original filename:")));
    filename_label = new wxStaticText(left_panel, -1, wxT(""));
    left_sizer->Add(filename_label, 0);
    wxBoxSizer *two_btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    revert_btn = new wxButton(left_panel, wxID_REVERT_TO_SAVED, 
                              wxT("Revert to Saved"));
    two_btn_sizer->Add(revert_btn, 0, wxALL|wxALIGN_CENTER, 5);
    save_as_btn = new wxButton(left_panel, wxID_SAVEAS, 
                               wxT("Save &As..."));
    two_btn_sizer->Add(save_as_btn, 0, wxALL|wxALIGN_CENTER, 5);
    left_sizer->Add(two_btn_sizer, 0, wxALIGN_CENTER);
    left_sizer->Add(new wxStaticText(left_panel, -1, wxT("Data title: ")),
                    0, wxLEFT|wxRIGHT|wxTOP, 5);
    title_label = new wxStaticText(left_panel, -1, wxT(""));
    left_sizer->Add(title_label, 0, wxLEFT|wxRIGHT|wxBOTTOM, 5);
    grid = new wxGrid(left_panel, ID_DE_GRID, 
                      wxDefaultPosition, wxSize(-1, 350));
    left_sizer->Add(grid, 1, wxEXPAND);
    left_panel->SetSizerAndFit(left_sizer);

    // right side of the dialog
    wxPanel *right_panel = new wxPanel(splitter); 
    wxBoxSizer *right_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *trans_sizer = new wxBoxSizer(wxHORIZONTAL);
    trans_list = new wxListCtrl(right_panel, ID_DE_EXAMPLES, 
                                  wxDefaultPosition, wxDefaultSize,
                                  wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_HRULES);
    trans_list->InsertColumn(0, wxT("transformation"));
    trans_list->InsertColumn(1, wxT("in menu"));
    trans_sizer->Add(trans_list, 1, wxEXPAND|wxALL, 5);
    wxBoxSizer *trans_button_sizer = new wxBoxSizer(wxVERTICAL);
    add_btn = new wxButton(right_panel, wxID_ADD, wxT("Add"));
    trans_button_sizer->Add(add_btn, 0, wxALL, 5);
    remove_btn = new wxButton(right_panel, wxID_REMOVE, wxT("Remove"));
    trans_button_sizer->Add(remove_btn, 0, wxALL, 5);
    up_btn = new wxButton(right_panel, wxID_UP, wxT("&Up"));
    trans_button_sizer->Add(up_btn, 0, wxALL, 5);
    down_btn = new wxButton(right_panel, wxID_DOWN, wxT("&Down"));
    trans_button_sizer->Add(down_btn, 0, wxALL, 5);
    save_btn = new wxButton(right_panel, wxID_SAVE, wxT("&Save"));
    trans_button_sizer->Add(save_btn, 0, wxALL, 5);
    reset_btn = new wxButton(right_panel, ID_DE_RESET, wxT("&Reset"));
    trans_button_sizer->Add(reset_btn, 0, wxALL, 5);
    trans_sizer->Add(trans_button_sizer, 0);
    right_sizer->Add(trans_sizer, 0, wxEXPAND);
    description = new wxStaticText(right_panel, -1, wxT("\n\n\n\n"), 
                                   wxDefaultPosition, wxDefaultSize,
                                   wxALIGN_LEFT);
    right_sizer->Add(description, 0, wxEXPAND|wxALL, 5);
    code = new wxTextCtrl(right_panel, ID_DE_CODE, wxT(""), 
                          wxDefaultPosition, wxDefaultSize,
                          wxTE_MULTILINE|wxHSCROLL|wxVSCROLL);
    right_sizer->Add(code, 1, wxEXPAND|wxALL, 5);
    wxBoxSizer *apply_help_sizer = new wxBoxSizer(wxHORIZONTAL);
    apply_help_sizer->Add(1, 1, 1);
    apply_btn = new wxButton(right_panel, wxID_APPLY, wxT("&Apply"));
    apply_help_sizer->Add(apply_btn, 0, wxALIGN_CENTER|wxALL, 5);
    apply_help_sizer->Add(1, 1, 1);
    rezoom_btn = new wxButton(right_panel, ID_DE_REZOOM, wxT("&Fit Zoom"));
    apply_help_sizer->Add(rezoom_btn, 0, wxALIGN_CENTER|wxALL, 5);
    apply_help_sizer->Add(1, 1, 1);
    help_btn = new wxButton(right_panel, wxID_HELP, wxT("&Help"));
    apply_help_sizer->Add(help_btn, 0, wxALIGN_RIGHT|wxALL, 5);
    right_sizer->Add(apply_help_sizer, 0, wxEXPAND);
    right_panel->SetSizerAndFit(right_sizer);

    // setting column sizes and a bit of logic
    update_data(dd);
    grid->SetEditable(true);
    grid->SetColumnWidth(0, 40);
    grid->SetRowLabelSize(60);
    initialize_transforms();
    for (int i = 0; i < 2; i++)
        trans_list->SetColumnWidth(i, wxLIST_AUTOSIZE);
    apply_btn->Enable(false);

    // finishing layout
    splitter->SplitVertically(left_panel, right_panel);
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    top_sizer->Add(splitter, 1, wxEXPAND, 1);
    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 10);
    top_sizer->Add(new wxButton(this, wxID_CLOSE, wxT("&Close")), 
                   0, wxALIGN_CENTER|wxALL, 5);
    SetSizerAndFit(top_sizer);

    // workaround for wxMSW 2.5.3 strange problem -- very small dialog window
    if (GetClientSize().GetHeight() < 200)
        SetClientSize(500, 500); 
                             
    Centre();
}

std::vector<DataTransform> DataEditorDlg::transforms;

void DataEditorDlg::read_transforms(bool reset)
{
    transforms.clear();
    // this item should be always present
    transforms.push_back(DataTransform("custom", "builtin", 
                                        "Custom transformation.\n"
                                        "You can type eg. Y=log10(y).\n"
                                        "See Help for details.",
                                        "", false));
    //TODO add last transformation item
    wxString transform_path = get_user_conffile("transform");
    string t_line;
    if (wxFileExists(transform_path) && !reset) {
        ifstream f(wx2s(transform_path).c_str());
        while (getline(f, t_line))
            transforms.push_back(DataTransform(t_line));
    }
    else {
        istringstream f(default_transforms);
        while (getline(f, t_line))
            transforms.push_back(DataTransform(t_line));
    }
}

void DataEditorDlg::initialize_transforms(bool reset)
{
    if (reset)
        read_transforms(reset);
    trans_list->DeleteAllItems();
    for (int i = 0; i < size(transforms); ++i) 
        insert_trans_list_item(i);
    select_transform(0);
}

void DataEditorDlg::insert_trans_list_item(int n)
{
    const DataTransform& ex = transforms[n];
    trans_list->InsertItem(n, s2wx(ex.name));
    trans_list->SetItem(n, 1, (ex.in_menu ? wxT("Yes") : wxT("No")));
}

void DataEditorDlg::select_transform(int item)
{
    if (item >= trans_list->GetItemCount())
        return;
    trans_list->SetItemState (item, 
                                wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED,
                                wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
    // ESelected();
}

int DataEditorDlg::get_selected_item()
{
    return trans_list->GetNextItem(-1,wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}

void DataEditorDlg::update_data(ndnd_type const& dd)
{
    ndnd = dd;
    string filename;
    string title;
    if (dd.size() == 1) {
        Data const* data = ndnd[0].second;
        filename = data->get_filename();
        save_as_btn->Enable(true);
        title = data->get_title();
        title_label->Show(true);
        title_label->SetLabel(s2wx(title));
        grid->SetTable(new DataTable(data, this), true, 
                       wxGrid::wxGridSelectRows);
        refresh_grid();
    }
    else {
        for (ndnd_type::const_iterator i = ndnd.begin(); i != ndnd.end(); ++i) {
            string nr = "@" + S(i->first) + ": ";
            filename += nr + i->second->get_filename() + "\n";
            title += nr + i->second->get_title() + "\n";
        }
        save_as_btn->Enable(false);
        title_label->Show(false);
        grid->Show(false);
    }
    revert_btn->Enable(is_revertable());
    filename_label->SetLabel(s2wx(filename));
}

bool DataEditorDlg::is_revertable() const
{
    for (ndnd_type::const_iterator i = ndnd.begin(); i != ndnd.end(); ++i) 
        if (i->second->get_filename().empty())
            return false;
    return true;
}

void DataEditorDlg::refresh_grid()
{
    if (ndnd.size() != 1)
        return;
    if (grid->GetNumberRows() != grid->GetTable()->GetNumberRows()) {
        grid->SetTable(new DataTable(ndnd[0].second, this), true, 
                       wxGrid::wxGridSelectRows);
    }
    grid->ForceRefresh();
    grid->AdjustScrollbars();
}

void DataEditorDlg::OnRevert (wxCommandEvent&)
{
    string cmd;
    for (ndnd_type::const_iterator i = ndnd.begin(); i != ndnd.end(); ++i) {
        if (i != ndnd.begin())
            cmd += "; ";
        cmd += "@" + S(i->first) + ".revert";
    }
    ftk->exec(cmd);
    refresh_grid();
}

void DataEditorDlg::OnSaveAs (wxCommandEvent&)
{
    if (ndnd.size() != 1)
        return;
    bool ok = export_data_dlg(this, true);
    if (ok) {
        filename_label->SetLabel(s2wx(ndnd[0].second->get_filename()));
    }
}

void DataEditorDlg::OnAdd (wxCommandEvent&)
{
    DataTransform new_transform("new", "useful",
                                 "", wx2s(code->GetValue()));
    TransEditorDlg dlg(this, -1, new_transform, transforms, -1);
    if (dlg.ShowModal() == wxID_OK) {
        int pos = get_selected_item() + 1;
        transforms.insert(transforms.begin() + pos, new_transform);
        insert_trans_list_item(pos);
        select_transform(pos);

        FFrame *fframe = static_cast<FFrame *>(GetParent());
        fframe->update_menu_fast_tranforms();
    }
}

void DataEditorDlg::OnRemove (wxCommandEvent&)
{
    int item = get_selected_item();
    if (item == -1 || transforms[item].category == "builtin")
        return;
    transforms.erase(transforms.begin() + item);
    trans_list->DeleteItem(item);
    select_transform(item > 0 ? item-1 : 0);
}

void DataEditorDlg::OnUp (wxCommandEvent&)
{
    int item = get_selected_item();
    if (item == 0)
        return;
    // swap item-1 and item
    DataTransform ex = transforms[item-1];
    transforms.erase(transforms.begin() + item - 1);
    trans_list->DeleteItem(item-1);
    transforms.insert(transforms.begin() + item, ex);
    insert_trans_list_item(item);
    up_btn->Enable(item-1 > 0);
    down_btn->Enable(true);
}

void DataEditorDlg::OnDown (wxCommandEvent&)
{
    int item = get_selected_item();
    if (item >= size(transforms) - 1)
        return;
    // swap item+1 and item
    DataTransform ex = transforms[item+1];
    transforms.erase(transforms.begin() + item + 1);
    trans_list->DeleteItem(item+1);
    transforms.insert(transforms.begin() + item, ex);
    insert_trans_list_item(item);
    up_btn->Enable(true);
    down_btn->Enable(item+1 < trans_list->GetItemCount() - 1);
}

void DataEditorDlg::OnSave (wxCommandEvent&)
{
    wxString transform_path = get_user_conffile("transform");
    ofstream f(wx2s(transform_path).c_str());
    for (vector<DataTransform>::const_iterator i = transforms.begin();
            i != transforms.end(); ++i)
        if (i->category != "builtin")
            f << i->as_fileline() << endl;
}

void DataEditorDlg::OnReset (wxCommandEvent&)
{
    initialize_transforms(true);
}

void DataEditorDlg::OnApply (wxCommandEvent&)
{
    execute_tranform(wx2s(code->GetValue().Trim()));
    refresh_grid();
    rezoom_btn->Enable();
}

void DataEditorDlg::OnReZoom (wxCommandEvent&)
{
    frame->GViewAll();
    rezoom_btn->Enable(false);
}

void DataEditorDlg::execute_tranform(string code)
{
    replace_all(code, "\n", "; ");
    vector<string> cmds = split_string(code, ';');
    string t;
    for (vector<string>::const_iterator i = cmds.begin(); i != cmds.end(); ++i){
        if (!strip_string(*i).empty())
            t += *i + frame->get_in_datasets();
        if (i+1 != cmds.end())
            t += ";";
    }
    ftk->exec(t);
}

void DataEditorDlg::OnHelp (wxCommandEvent&)
{
    frame->display_help_section("Data transformations");
}

void DataEditorDlg::OnClose (wxCommandEvent&)
{
    close_it(this);
}

void DataEditorDlg::CodeText()
{
    bool check_syntax = true;
    wxString text = code->GetValue().Trim();
    if (check_syntax) {
        string text = wx2s(code->GetValue());
        replace_all(text, "\n", " , ");
        // 'text' is not identical with the final command (, instead of ;)
        apply_btn->Enable(compile_data_transformation(text));
    }
    else
        apply_btn->Enable(!code->GetValue().Trim().IsEmpty());
}

void DataEditorDlg::ESelected()
{
    int item = get_selected_item();
    if (item == -1) {
        item = 0;
        select_transform(0);
        return;
    }
    const DataTransform& ex = transforms[item];
    // to avoid frequent resizing, description should have >= 3 lines
    string desc = ex.description;
    for (int i = count(desc.begin(), desc.end(), '\n') + 1; i < 3; ++i)
        desc += "\n";
    description->SetLabel(s2wx(desc));
    Layout(); // to resize description
    code->SetValue(s2wx(ex.code));

    up_btn->Enable(item > 0);
    down_btn->Enable(item < trans_list->GetItemCount() - 1);
    remove_btn->Enable(ex.category != "builtin");
    CodeText();
}

void DataEditorDlg::OnEActivated (wxListEvent& event)
{
    int n = event.GetIndex();
    if (transforms[n].category == "builtin")
        return;
    TransEditorDlg dlg(this, -1, transforms[n], transforms, n);
    if (dlg.ShowModal() == wxID_OK) {
        trans_list->DeleteItem(n);
        insert_trans_list_item(n);
        select_transform(n);
    }
}


BEGIN_EVENT_TABLE(TransEditorDlg, wxDialog)
    EVT_BUTTON  (wxID_OK,    TransEditorDlg::OnOK)
END_EVENT_TABLE()

TransEditorDlg::TransEditorDlg(wxWindow* parent, wxWindowID id, 
                                   DataTransform& ex_,
                                   const vector<DataTransform>& transforms_,
                                   int pos_)
    : wxDialog(parent, id, wxT("Transformation Editor"), 
               wxDefaultPosition, wxDefaultSize, 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
      ex(ex_), transforms(transforms_), pos(pos_)
{
    name_tc = new wxTextCtrl(this, -1, s2wx(ex.name));
    description_tc = new wxTextCtrl(this, -1, s2wx(ex.description),
                                    wxDefaultPosition, wxSize(-1, 80),
                                    wxTE_MULTILINE|wxHSCROLL|wxVSCROLL);
    wxString choices[] = {wxT("useful"), wxT("example"), wxT("other")};
    category_c = new wxComboBox(this, -1, s2wx(ex.category),
                                wxDefaultPosition, wxDefaultSize,
                                3, choices,
                                wxCB_READONLY);
    code_tc = new wxTextCtrl(this, -1, s2wx(ex.code), 
                             wxDefaultPosition, wxSize(-1, 100),
                             wxTE_MULTILINE|wxHSCROLL|wxVSCROLL);
    inmenu_cb = new wxCheckBox(this, -1,wxT("show item in Data->Fast_DT menu"));
    inmenu_cb->SetValue(ex.in_menu);

    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer *flexsizer = new wxFlexGridSizer(2);
    flexsizer->Add(new wxStaticText(this, -1, wxT("name")), 0, wxALL, 5);
    flexsizer->Add(name_tc, 0, wxALL|wxEXPAND, 5);
    flexsizer->Add(new wxStaticText(this, -1, wxT("category")), 0, wxALL, 5);
    flexsizer->Add(category_c, 0, wxALL|wxEXPAND, 5);
    flexsizer->Add(new wxStaticText(this, -1, wxT("description")), 0, wxALL, 5);
    flexsizer->Add(description_tc, 0, wxALL|wxEXPAND, 5);
    flexsizer->Add(new wxStaticText(this, -1, wxT("code")), 0, wxALL, 5);
    flexsizer->Add(code_tc, 0, wxALL|wxEXPAND, 5);
    flexsizer->AddGrowableRow(2); // description
    flexsizer->AddGrowableRow(3); // code
    flexsizer->AddGrowableCol(1);
    top_sizer->Add(flexsizer, 0, wxEXPAND);
    top_sizer->Add(inmenu_cb, 0, wxALL, 5);
    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxALL, 5);
    SetSizerAndFit(top_sizer);
    Centre();
}

void TransEditorDlg::OnOK(wxCommandEvent &)
{
    string new_name = wx2s(name_tc->GetValue().Trim());
    for (int i = 0; i < size(transforms); ++i) 
            if (i != pos && transforms[i].name == new_name) {//name not unique
                name_tc->SetFocus();
                name_tc->SetSelection(-1, -1);
                return;
            }
    // we are here -- name is unique
    ex.name = new_name;
    ex.category = wx2s(category_c->GetValue());
    ex.description = wx2s(description_tc->GetValue().Trim());
    ex.code = wx2s(code_tc->GetValue().Trim());
    ex.in_menu = inmenu_cb->GetValue();
    close_it(this, wxID_OK);
}

