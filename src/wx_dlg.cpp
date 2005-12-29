// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$


// wxwindows headers, see wxwindows samples for description
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif


#include <istream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <utility>
#include <map>
#include <wx/valtext.h>
#include <wx/bmpbuttn.h>
#include <wx/grid.h>
#include <wx/statline.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include "common.h"
#include "wx_dlg.h"
#include "wx_common.h"
#include "wx_pane.h"
#include "wx_gui.h"
#include "data.h"
#include "sum.h"
#include "fit.h"
#include "ui.h"
#include "settings.h"
#include "datatrans.h" 

#if 0
//bitmaps for buttons
#include "img/up_arrow.xpm"
#include "img/down_arrow.xpm"
#endif

using namespace std;


enum {
    ID_BRO_TREE             = 26100,
    /*
    ID_BRO_A_WHAT                  ,
    ID_BRO_A_TYPE                  ,
    ID_BRO_A_ADD                   ,
    ID_BRO_A_NRB            = 26110, // and next 10
    ID_BRO_A_TPC            = 26125, // and next 2
    ID_BRO_A_NTC            = 26130,
    ID_BRO_C_LL                    ,
    ID_BRO_C_L                     ,
    ID_BRO_C_R                     ,
    ID_BRO_C_RR                    ,
    ID_BRO_C_TXT                   ,
    ID_BRO_C_SETCB                 ,
    ID_BRO_C_CSETCB                ,
    ID_BRO_C_RB0                   ,
    ID_BRO_C_RB1                   ,
    ID_BRO_C_APPL                  ,
    ID_BRO_D_DEL                   ,
    ID_BRO_D_RCRM                  ,
    ID_BRO_F_FT                    ,
    ID_BRO_F_FA                    ,
    ID_BRO_F_TA                    ,
    ID_BRO_V_BTN                   ,
    ID_BRO_MENU_EXP                , 
    ID_BRO_MENU_COL                , 
    ID_BRO_MENU_BUT                , 
    ID_BRO_MENU_RST                , 
    */
    
    ID_DXLOAD_FTYPE_RB             ,
    ID_DXLOAD_STDDEV_CB            ,
  
    ID_SHIST_LC                    ,
    ID_SHIST_UP                    ,
    ID_SHIST_DOWN                  ,
    ID_SHIST_TSAV                  ,
    ID_SHIST_CWSSR                 ,
    ID_SHIST_V              = 26300, // and next 2
    ID_DE_GRID              = 26310,
    ID_DE_RESET                    ,
    ID_DE_CODE                     ,
    ID_DE_EXAMPLES                 ,
    ID_DE_REZOOM
};


//=====================   data->LoadFile(Custom) dialog  ==================

// first helper class: LoadDataDirCtrl
BEGIN_EVENT_TABLE(LoadDataDirCtrl, wxGenericDirCtrl)
    EVT_TREE_SEL_CHANGED(wxID_TREECTRL, LoadDataDirCtrl::OnPathSelectionChanged)
END_EVENT_TABLE()

LoadDataDirCtrl::LoadDataDirCtrl(FDXLoadDlg* parent)
    : wxGenericDirCtrl(parent, -1, wxDirDialogDefaultFolderStr,
                       wxDefaultPosition, wxSize(-1, 300),
                       wxDIRCTRL_SHOW_FILTERS,
                       // multiple wildcards, eg. 
                       // |*.dat;*.DAT;*.xy;*.XY;*.fio;*.FIO
                       // are not supported by wxGenericDirCtrl  
                       "all files (*)|*"
                       "|ASCII x y files (*)|*" 
                       "|rit files (*.rit)|*.rit"
                       "|cpi files (*.cpi)|*.cpi"
                       "|mca files (*.mca)|*.mca"
                       "|Siemens/Bruker (*.raw)|*.raw"),
      load_dlg(parent)
{}

void LoadDataDirCtrl::OnPathSelectionChanged(wxTreeEvent &WXUNUSED(event))
{
    load_dlg->on_path_change();
}

void LoadDataDirCtrl::SetFilterIndex(int n)
{
    wxGenericDirCtrl::SetFilterIndex(n);
    load_dlg->on_filter_change();
}


BEGIN_EVENT_TABLE(FDXLoadDlg, wxDialog)
    EVT_CHECKBOX    (ID_DXLOAD_STDDEV_CB, FDXLoadDlg::OnStdDevCheckBox)
END_EVENT_TABLE()

FDXLoadDlg::FDXLoadDlg (wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, "Data load (custom)", 
               wxDefaultPosition, wxSize(-1, 500), 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER) 
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    dir_ctrl = new LoadDataDirCtrl(this);
    string path = my_data->get_filename();
    //TODO it segfaults, don't know why
    //dir_ctrl->SetPath(path.c_str());
    top_sizer->Add(dir_ctrl, 1, wxALL|wxEXPAND, 5);

    filename_tc = new wxTextCtrl (this, -1, path.c_str(), 
                                  wxDefaultPosition, wxDefaultSize,
                                  wxTE_READONLY);
    top_sizer->Add (filename_tc, 0, wxALL|wxEXPAND, 5);

    //selecting columns
    columns_panel = new wxPanel (this, -1);
    wxStaticBox *cbox = new wxStaticBox (columns_panel, -1, "Select columns:");
    wxStaticBoxSizer *h2a_sizer = new wxStaticBoxSizer (cbox, wxHORIZONTAL);
    h2a_sizer->Add (new wxStaticText (columns_panel, -1, "x"), 
                    0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
    x_column = new wxSpinCtrl (columns_panel, -1, "1", 
                               wxDefaultPosition, wxSize(70, -1), 
                               wxSP_ARROW_KEYS, 1, 99, 1);
    h2a_sizer->Add (x_column, 0, wxALL|wxALIGN_LEFT, 5);
    h2a_sizer->Add (new wxStaticText (columns_panel, -1, "y"), 
                    0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
    y_column = new wxSpinCtrl (columns_panel, -1, "2",
                               wxDefaultPosition, wxSize(70, -1), 
                               wxSP_ARROW_KEYS, 1, 99, 2);
    h2a_sizer->Add (y_column, 0, wxALL|wxALIGN_LEFT, 5);
    std_dev_cb = new wxCheckBox(columns_panel, ID_DXLOAD_STDDEV_CB, "std.dev.");
    std_dev_cb->SetValue (false);
    h2a_sizer->Add (std_dev_cb, 0, wxALL|wxALIGN_LEFT, 5);
    s_column = new wxSpinCtrl (columns_panel, -1, "3",
                               wxDefaultPosition, wxSize(70, -1), 
                               wxSP_ARROW_KEYS, 1, 99, 3);
    h2a_sizer->Add (s_column, 0, wxALL|wxALIGN_LEFT, 5);
    columns_panel->SetSizerAndFit(h2a_sizer);
    top_sizer->Add (columns_panel, 0, wxALL|wxEXPAND, 5);
    OnStdDevCheckBox(dummy_cmd_event);

    append_cb = new wxCheckBox(this, -1, 
                               "Append data from file to already loaded data");
    append_cb->Enable(!my_data->is_empty());
    top_sizer->Add(append_cb, 0, wxALL, 5);

    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    top_sizer->Add (CreateButtonSizer (wxOK|wxCANCEL), 
                    0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);
    on_filter_change();
}

void FDXLoadDlg::OnStdDevCheckBox (wxCommandEvent& WXUNUSED(event))
{
    s_column->Enable(std_dev_cb->GetValue());
}

void FDXLoadDlg::on_filter_change()
{
    int idx = dir_ctrl->GetFilterIndex();
    if (idx == 0) // all files
        on_path_change();
    else
        columns_panel->Enable(idx == 1); // enable if ASCII 
}

void FDXLoadDlg::on_path_change()
{
    wxString path = dir_ctrl->GetFilePath();
    filename_tc->SetValue(path);
    if (dir_ctrl->GetFilterIndex() == 0) { // all files
        columns_panel->Enable(!path.IsEmpty() 
                              && Data::guess_file_type(path.c_str()) == 'd'); 
    }
    //TODO enable/disable OK button?
}

string FDXLoadDlg::get_filename()
{
    return filename_tc->GetValue().c_str();
}

string FDXLoadDlg::get_command_tail()
{
    string cols;
    if (columns_panel->IsEnabled()) { // a:b[:c]
        cols = " " + S(x_column->GetValue()) + ":" + S(y_column->GetValue());
        if (std_dev_cb->GetValue())
            cols += S(":") + S(s_column->GetValue());
    }
    return "'" + get_filename() + "'" + cols;
}

#if 0
//=====================   data->history dialog  ==================

BEGIN_EVENT_TABLE(SumHistoryDlg, wxDialog)
    EVT_BUTTON      (ID_SHIST_UP,     SumHistoryDlg::OnUpButton)
    EVT_BUTTON      (ID_SHIST_DOWN,   SumHistoryDlg::OnDownButton)
    EVT_BUTTON      (ID_SHIST_TSAV,   SumHistoryDlg::OnToggleSavedButton)
    EVT_BUTTON      (ID_SHIST_CWSSR,  SumHistoryDlg::OnComputeWssrButton)
    EVT_LIST_ITEM_SELECTED  (ID_SHIST_LC, SumHistoryDlg::OnSelectedItem)
    EVT_LIST_ITEM_ACTIVATED (ID_SHIST_LC, SumHistoryDlg::OnActivatedItem)
    EVT_SPINCTRL    (ID_SHIST_V+0,    SumHistoryDlg::OnViewSpinCtrlUpdate)
    EVT_SPINCTRL    (ID_SHIST_V+1,    SumHistoryDlg::OnViewSpinCtrlUpdate)
    EVT_SPINCTRL    (ID_SHIST_V+2,    SumHistoryDlg::OnViewSpinCtrlUpdate)
END_EVENT_TABLE()

SumHistoryDlg::SumHistoryDlg (wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, "Parameters History", 
               wxDefaultPosition, wxDefaultSize, 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER), 
      lc(0)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    initialize_lc(); //wxListCtrl
    hsizer->Add (lc, 1, wxEXPAND);

    wxBoxSizer *arrows_sizer = new wxBoxSizer(wxVERTICAL);
    up_arrow = new wxBitmapButton (this, ID_SHIST_UP, wxBitmap (up_arrow_xpm));
    arrows_sizer->Add (up_arrow, 0);
    arrows_sizer->Add (10, 10, 1);
    down_arrow = new wxBitmapButton (this, ID_SHIST_DOWN, 
                                     wxBitmap (down_arrow_xpm));
    arrows_sizer->Add (down_arrow, 0);
    hsizer->Add (arrows_sizer, 0, wxALIGN_CENTER);
    top_sizer->Add (hsizer, 1, wxEXPAND);

    wxBoxSizer *buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    buttons_sizer->Add (new wxButton (this, ID_SHIST_TSAV, "Toggle saved"), 
                        0, wxALL, 5);
    buttons_sizer->Add (10, 10, 1);
    compute_wssr_button = new wxButton (this, ID_SHIST_CWSSR,"Compute WSSRs");
    buttons_sizer->Add (compute_wssr_button, 0, wxALL, 5);
    buttons_sizer->Add (10, 10, 1);
    buttons_sizer->Add (new wxStaticText (this, -1, "View @:"), 
                        0, wxALL|wxALIGN_CENTER, 5);
    for (int i = 0; i < 3; i++)
        buttons_sizer->Add (new wxSpinCtrl (this, ID_SHIST_V + i, 
                                            S(view[i]).c_str(),
                                            wxDefaultPosition, wxSize(40, -1),
                                            wxSP_ARROW_KEYS, 0, view_max),
                            0, wxALL, 5);
    buttons_sizer->Add (10, 10, 1);
    buttons_sizer->Add (new wxButton (this, wxID_CANCEL, "Close"), 
                        0, wxALL, 5);
    top_sizer->Add (buttons_sizer, 0, wxALIGN_CENTER);

    SetSizer (top_sizer);
    top_sizer->SetSizeHints (this);

    update_selection();
}

void SumHistoryDlg::initialize_lc()
{
    assert (lc == 0);
    view_max = my_sum->pars()->count_a() - 1;
    assert (view_max != -1);
    for (int i = 0; i < 3; i++)
        view[i] = min (i, view_max);
    lc = new wxListCtrl (this, ID_SHIST_LC, 
                         wxDefaultPosition, wxSize(450, 250), 
                         wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_HRULES|wxLC_VRULES
                             |wxSIMPLE_BORDER);
    lc->InsertColumn(0, "Saved");
    lc->InsertColumn(1, "No.");
    lc->InsertColumn(2, "Changed by");
    lc->InsertColumn(3, "WSSR");
    for (int i = 0; i < 3; i++)
        lc->InsertColumn(4 + i, ("@" + S(view[i])).c_str()); 

    for (int i = 0; i != my_sum->pars()->history_size(); ++i) {
        const HistoryItem& item = my_sum->pars()->history_item(i);
        lc->InsertItem(i, item.saved ? "   *   " : "       ");
        lc->SetItem (i, 1, ("  " + S(i+1) + "  ").c_str());
        lc->SetItem (i, 2, item.comment.c_str());
        lc->SetItem (i, 3, "      ?      ");
        for (int j = 0; j < 3; j++)
            lc->SetItem (i, 4 + j, S(item.a[view[j]]).c_str());
    }
    for (int i = 0; i < 7; i++)
        lc->SetColumnWidth(i, wxLIST_AUTOSIZE);
}

void SumHistoryDlg::update_selection()
{
    int index = my_sum->pars()->history_position();
    lc->SetItemState (index, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, 
                             wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
    up_arrow->Enable (index != 0);
    down_arrow->Enable (index != my_sum->pars()->history_size() - 1);
}

void SumHistoryDlg::OnUpButton       (wxCommandEvent& WXUNUSED(event))
{
    exec_command ("s.history -1");
    update_selection();
}

void SumHistoryDlg::OnDownButton     (wxCommandEvent& WXUNUSED(event))
{
    exec_command ("s.history +1");
    update_selection();
}

void SumHistoryDlg::OnToggleSavedButton (wxCommandEvent& WXUNUSED(event))
{
    exec_command ("s.history *");
    int idx = my_sum->pars()->history_position();
    lc->SetItemText (idx, my_sum->pars()->history_item(idx).saved ? "   *   " 
                                                          : "       ");
}

void SumHistoryDlg::OnComputeWssrButton (wxCommandEvent& WXUNUSED(event))
{
    for (int i = 0; i != my_sum->pars()->history_size(); ++i) {
        const HistoryItem& item = my_sum->pars()->history_item(i);
        my_sum->use_param_a_for_value (item.a);
        fp wssr = Fit::compute_wssr_for_data (my_data, my_sum, true);
        lc->SetItem (i, 3, S(wssr).c_str());
    }
    lc->SetColumnWidth(3, wxLIST_AUTOSIZE);
    compute_wssr_button->Enable(false);
}
void SumHistoryDlg::OnSelectedItem (wxListEvent& WXUNUSED(event))
{
    update_selection();
}

void SumHistoryDlg::OnActivatedItem (wxListEvent& event)
{
    int n = event.GetIndex();
    exec_command ("s.history " + S(n+1));
    update_selection();
}

void SumHistoryDlg::OnViewSpinCtrlUpdate (wxSpinEvent& event) 
{
    int v = event.GetId() - ID_SHIST_V;
    assert (0 <= v && v < 3);
    int n = event.GetPosition();
    assert (0 <= n && n <= view_max);
    view[v] = n;
    //update header in wxListCtrl
    wxListItem li;
    li.SetMask (wxLIST_MASK_TEXT);
    li.SetText (("@" + S(view[v])).c_str());
    lc->SetColumn(4 + v, li); 
    //update data in wxListCtrl
    for (int i = 0; i != my_sum->pars()->history_size(); ++i) {
        const HistoryItem& item = my_sum->pars()->history_item(i);
        lc->SetItem (i, 4 + v, S(item.a[view[v]]).c_str());
    }
}
#endif


//=====================   data->editor dialog  ==================

class DataTable: public wxGridTableBase
{
public:
    DataTable(Data *data_, DataEditorDlg *ded_) : wxGridTableBase(), 
                                            data(data_), ded(ded_) {}
    int GetNumberRows() { return data->points().size(); }
    int GetNumberCols() { return 4; }
    bool IsEmptyCell(int WXUNUSED(row), int WXUNUSED(col)) { return false; }

    wxString GetValue(int row, int col) 
        {  return S(col == 0 ? double(GetValueAsBool(row,col))
                             : GetValueAsDouble(row,col)).c_str(); }

    void SetValue(int, int, const wxString&) { assert(0); }

    wxString GetTypeName(int WXUNUSED(row), int col)
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
        exec_command(t + "[" + S(row)+"]=" + S(value));
        if (col == 1) // order of items can be changed
            ded->grid->ForceRefresh();
        ded->rezoom_btn->Enable();
    }

    void SetValueAsBool(int row, int col, bool value) 
    { 
        assert(col==0); 
        exec_command("A[" + S(row)+"]=" + (value ?"true":"false")); 
        ded->rezoom_btn->Enable();
    }

    wxString GetRowLabelValue(int row) { return S(row).c_str(); }

    wxString GetColLabelValue(int col) 
    { 
        switch (col) {
            case 0: return "active"; 
            case 1: return "x";      
            case 2: return "y";      
            case 3: return "sigma";  
            default: assert(0);
        }
    }

private:
    Data *data;
    DataEditorDlg *ded;
};


// ';' will be replaced by line break
static const char *default_examples = 

"integrate|useful|Integrate data numerically and adjust std. dev.;"
"in other words it produces cumulative area"
"|Y[1...] = Y[n-1] + y[n];S = sqrt(max(1,y))|Y\n"

"differentiate|useful|compute numerical derivative f'(x)"
"|Y[...-1] = y[n+1]-y[n];X[...-1] = (x[n+1]+x[n])/2;"
"M=M-1;S = sqrt(max(1,y))|Y\n"

"normalize area|useful|divide all Y (and std. dev.) values;"
"by the current data area; it produces unit area"
"|S = s / sum(n > 0 ? (x[n] - x[n-1]) * (y[n-1] + y[n])/2 : 0);" 
"Y = y / sum(n > 0 ? (x[n] - x[n-1]) * (y[n-1] + y[n])/2 : 0)|Y\n" 

"zero negative y|useful|zero the Y value; of points with negative Y"
"|Y=max(y,0)|Y\n"

"clear inactive|useful|delete inactive points"
"|delete(not a)|Y\n"

"swap axes|example|Swap X and Y axes and adjust std. dev."
"|Y=x & X=y & S=sqrt(max(1,Y))|N\n"

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

DataTransExample::DataTransExample(string line)
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

string DataTransExample::as_fileline() const
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

DataEditorDlg::DataEditorDlg (wxWindow* parent, wxWindowID id, Data *data_)
    : wxDialog(parent, id, "Data Editor", 
               wxDefaultPosition, wxSize(500, 500), 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
      data(0)
{
    ProportionalSplitter *splitter = new ProportionalSplitter(this, -1, 0.5);

    // left side of the dialog
    wxPanel *left_panel = new wxPanel(splitter); 
    wxBoxSizer *left_sizer = new wxBoxSizer(wxVERTICAL);
    left_sizer->Add(new wxStaticText(left_panel, -1, "Original filename:"));
    filename_label = new wxStaticText(left_panel, -1, "");
    left_sizer->Add(filename_label, 0, wxADJUST_MINSIZE);
    wxBoxSizer *two_btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    revert_btn = new wxButton(left_panel, wxID_REVERT_TO_SAVED, 
                              "Revert to Saved");
    two_btn_sizer->Add(revert_btn, 0, wxALL|wxALIGN_CENTER, 5);
    save_as_btn = new wxButton(left_panel, wxID_SAVEAS, 
                               "Save &As...");
    two_btn_sizer->Add(save_as_btn, 0, wxALL|wxALIGN_CENTER, 5);
    left_sizer->Add(two_btn_sizer, 0, wxALIGN_CENTER);
    left_sizer->Add(new wxStaticText(left_panel, -1, "Data title: "),
                    0, wxLEFT|wxRIGHT|wxTOP, 5);
    title_label = new wxStaticText(left_panel, -1, "");
    left_sizer->Add(title_label, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE,5);
    grid = new wxGrid(left_panel, ID_DE_GRID, 
                      wxDefaultPosition, wxSize(-1, 350));
    left_sizer->Add(grid, 1, wxEXPAND);
    left_panel->SetSizerAndFit(left_sizer);

    // right side of the dialog
    wxPanel *right_panel = new wxPanel(splitter); 
    wxBoxSizer *right_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *example_sizer = new wxBoxSizer(wxHORIZONTAL);
    example_list = new wxListCtrl(right_panel, ID_DE_EXAMPLES, 
                                  wxDefaultPosition, wxDefaultSize,
                                  wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_HRULES);
    example_list->InsertColumn(0, "transformation");
    example_list->InsertColumn(1, "in menu");
    example_sizer->Add(example_list, 1, wxEXPAND|wxALL, 5);
    wxBoxSizer *example_button_sizer = new wxBoxSizer(wxVERTICAL);
    add_btn = new wxButton(right_panel, wxID_ADD, "Add");
    example_button_sizer->Add(add_btn, 0, wxALL, 5);
    remove_btn = new wxButton(right_panel, wxID_REMOVE, "Remove");
    example_button_sizer->Add(remove_btn, 0, wxALL, 5);
    up_btn = new wxButton(right_panel, wxID_UP, "&Up");
    example_button_sizer->Add(up_btn, 0, wxALL, 5);
    down_btn = new wxButton(right_panel, wxID_DOWN, "&Down");
    example_button_sizer->Add(down_btn, 0, wxALL, 5);
    save_btn = new wxButton(right_panel, wxID_SAVE, "&Save");
    example_button_sizer->Add(save_btn, 0, wxALL, 5);
    reset_btn = new wxButton(right_panel, ID_DE_RESET, "&Reset");
    example_button_sizer->Add(reset_btn, 0, wxALL, 5);
    example_sizer->Add(example_button_sizer, 0);
    right_sizer->Add(example_sizer, 0, wxEXPAND);
    description = new wxStaticText(right_panel, -1, "\n\n\n\n", 
                                   wxDefaultPosition, wxDefaultSize,
                                   wxALIGN_LEFT);
    right_sizer->Add(description, 0, wxEXPAND|wxALL|wxADJUST_MINSIZE, 5);
    code = new wxTextCtrl(right_panel, ID_DE_CODE, "", 
                          wxDefaultPosition, wxDefaultSize,
                          wxTE_MULTILINE|wxHSCROLL|wxVSCROLL);
    right_sizer->Add(code, 1, wxEXPAND|wxALL, 5);
    wxBoxSizer *apply_help_sizer = new wxBoxSizer(wxHORIZONTAL);
    apply_help_sizer->Add(1, 1, 1);
    apply_btn = new wxButton(right_panel, wxID_APPLY, "&Apply");
    apply_help_sizer->Add(apply_btn, 0, wxALIGN_CENTER|wxALL, 5);
    apply_help_sizer->Add(1, 1, 1);
    rezoom_btn = new wxButton(right_panel, ID_DE_REZOOM, "&Fit Zoom");
    apply_help_sizer->Add(rezoom_btn, 0, wxALIGN_CENTER|wxALL, 5);
    apply_help_sizer->Add(1, 1, 1);
    help_btn = new wxButton(right_panel, wxID_HELP, "&Help");
    apply_help_sizer->Add(help_btn, 0, wxALIGN_RIGHT|wxALL, 5);
    right_sizer->Add(apply_help_sizer, 0, wxEXPAND);
    right_panel->SetSizerAndFit(right_sizer);

    // setting column sizes and a bit of logic
    update_data(data_);
    grid->SetEditable(true);
    grid->SetColumnWidth(0, 40);
    grid->SetRowLabelSize(60);
    initialize_examples();
    for (int i = 0; i < 2; i++)
        example_list->SetColumnWidth(i, wxLIST_AUTOSIZE);
    apply_btn->Enable(false);

    // finishing layout
    splitter->SplitVertically(left_panel, right_panel);
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    top_sizer->Add(splitter, 1, wxEXPAND, 1);
    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 10);
    top_sizer->Add(new wxButton(this, wxID_CLOSE, "&Close"), 
                   0, wxALIGN_CENTER|wxALL, 5);
    SetSizerAndFit(top_sizer);

    // workaround for wxMSW 2.5.3 strange problem -- very small dialog window
    if (GetClientSize().GetHeight() < 200)
        SetClientSize(500, 500); 
                             
    Centre();
}

std::vector<DataTransExample> DataEditorDlg::examples;

void DataEditorDlg::read_examples(bool reset)
{
    examples.clear();
    // this item should be always present
    examples.push_back(DataTransExample("custom", "builtin", 
                                        "Custom transformation.\n"
                                        "You can type eg. Y=log10(y).\n"
                                        "See Help for details.",
                                        "", false));
    //TODO last transformation item
    wxString transform_path = get_user_conffile("transform");
    string t_line;
    if (wxFileExists(transform_path) && !reset) {
        ifstream f(transform_path.c_str());
        while (getline(f, t_line))
            examples.push_back(DataTransExample(t_line));
    }
    else {
        istringstream f(default_examples);
        while (getline(f, t_line))
            examples.push_back(DataTransExample(t_line));
    }
}

void DataEditorDlg::initialize_examples(bool reset)
{
    if (reset)
        read_examples(reset);
    example_list->DeleteAllItems();
    for (int i = 0; i < size(examples); ++i) 
        insert_example_list_item(i);
    select_example(0);
}

void DataEditorDlg::insert_example_list_item(int n)
{
    const DataTransExample& ex = examples[n];
    example_list->InsertItem(n, ex.name.c_str());
    example_list->SetItem(n, 1, (ex.in_menu ? "Yes" : "No"));
}

void DataEditorDlg::select_example(int item)
{
    if (item >= example_list->GetItemCount())
        return;
    example_list->SetItemState (item, 
                                wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED,
                                wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
    // ESelected();
}

int DataEditorDlg::get_selected_item()
{
    return example_list->GetNextItem(-1,wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}

void DataEditorDlg::update_data(Data *data_)
{
    data = data_;
    string const& filename = data->get_filename();
    filename_label->SetLabel(filename.c_str());
    revert_btn->Enable(!filename.empty());
    title_label->SetLabel(data->get_title().c_str());
    grid->SetTable(new DataTable(data, this), true, wxGrid::wxGridSelectRows);
    refresh_grid();
    Show();
}

void DataEditorDlg::refresh_grid()
{
    if (grid->GetNumberRows() != grid->GetTable()->GetNumberRows())
        grid->SetTable(new DataTable(data, this), true, 
                       wxGrid::wxGridSelectRows);
    grid->ForceRefresh();
    grid->AdjustScrollbars();
}

void DataEditorDlg::OnRevert (wxCommandEvent& WXUNUSED(event))
{
    exec_command("@ <'" + data->get_filename() + "'"); //TODO cols etc.
    refresh_grid();
}

void DataEditorDlg::OnSaveAs (wxCommandEvent& WXUNUSED(event))
{
    bool ok = export_data_dlg(this /*GetParent()*/, true);
    if (ok) {
        filename_label->SetLabel(data->get_filename().c_str());
    }
}

void DataEditorDlg::OnAdd (wxCommandEvent& WXUNUSED(event))
{
    DataTransExample new_example("new", "useful",
                                 "", code->GetValue().c_str());
    ExampleEditorDlg dlg(this, -1, new_example, examples, -1);
    if (dlg.ShowModal() == wxID_OK) {
        int pos = get_selected_item() + 1;
        examples.insert(examples.begin() + pos, new_example);
        insert_example_list_item(pos);
        select_example(pos);
    }
}

void DataEditorDlg::OnRemove (wxCommandEvent& WXUNUSED(event))
{
    int item = get_selected_item();
    if (item == -1 || examples[item].category == "builtin")
        return;
    examples.erase(examples.begin() + item);
    example_list->DeleteItem(item);
    select_example(item > 0 ? item-1 : 0);
}

void DataEditorDlg::OnUp (wxCommandEvent& WXUNUSED(event))
{
    int item = get_selected_item();
    if (item == 0)
        return;
    // swap item-1 and item
    DataTransExample ex = examples[item-1];
    examples.erase(examples.begin() + item - 1);
    example_list->DeleteItem(item-1);
    examples.insert(examples.begin() + item, ex);
    insert_example_list_item(item);
    up_btn->Enable(item-1 > 0);
    down_btn->Enable(true);
}

void DataEditorDlg::OnDown (wxCommandEvent& WXUNUSED(event))
{
    int item = get_selected_item();
    if (item >= size(examples) - 1)
        return;
    // swap item+1 and item
    DataTransExample ex = examples[item+1];
    examples.erase(examples.begin() + item + 1);
    example_list->DeleteItem(item+1);
    examples.insert(examples.begin() + item, ex);
    insert_example_list_item(item);
    up_btn->Enable(true);
    down_btn->Enable(item+1 < example_list->GetItemCount() - 1);
}

void DataEditorDlg::OnSave (wxCommandEvent& WXUNUSED(event))
{
    wxString transform_path = get_user_conffile("transform");
    ofstream f(transform_path.c_str());
    for (vector<DataTransExample>::const_iterator i = examples.begin();
            i != examples.end(); ++i)
        if (i->category != "builtin")
            f << i->as_fileline() << endl;
}

void DataEditorDlg::OnReset (wxCommandEvent& WXUNUSED(event))
{
    initialize_examples(true);
}

void DataEditorDlg::OnApply (wxCommandEvent& WXUNUSED(event))
{
    execute_tranform(code->GetValue().Trim().c_str());
    refresh_grid();
    rezoom_btn->Enable();
}

void DataEditorDlg::OnReZoom (wxCommandEvent& WXUNUSED(event))
{
    frame->GViewAll();
    rezoom_btn->Enable(false);
}

void DataEditorDlg::execute_tranform(string code)
{
    replace_all(code, "\n", "; ");
    exec_command(code);
}

void DataEditorDlg::OnHelp (wxCommandEvent& WXUNUSED(event))
{
    getUI()->displayHelpTopic("Data transformations");
}

void DataEditorDlg::OnClose (wxCommandEvent& event)
{
    OnCancel(event);
}

void DataEditorDlg::CodeText()
{
    bool check_syntax = true;
    wxString text = code->GetValue().Trim();
    if (check_syntax) {
        string text = code->GetValue().c_str();
        replace_all(text, "\n", " & ");
        apply_btn->Enable(validate_transformation(text));
    }
    else
        apply_btn->Enable(!code->GetValue().Trim().IsEmpty());
}

void DataEditorDlg::ESelected()
{
    int item = get_selected_item();
    if (item == -1) {
        item = 0;
        select_example(0);
        return;
    }
    const DataTransExample& ex = examples[item];
    // to avoid frequent resizing, description should have >= 3 lines
    string desc = ex.description;
    for (int i = count(desc.begin(), desc.end(), '\n') + 1; i < 3; ++i)
        desc += "\n";
    description->SetLabel(desc.c_str());
    Layout(); // to resize description
    code->SetValue(ex.code.c_str());

    up_btn->Enable(item > 0);
    down_btn->Enable(item < example_list->GetItemCount() - 1);
    remove_btn->Enable(ex.category != "builtin");
    CodeText();
}

void DataEditorDlg::OnEActivated (wxListEvent& event)
{
    int n = event.GetIndex();
    if (examples[n].category == "builtin")
        return;
    ExampleEditorDlg dlg(this, -1, examples[n], examples, n);
    if (dlg.ShowModal() == wxID_OK) {
        example_list->DeleteItem(n);
        insert_example_list_item(n);
        select_example(n);
    }
}


BEGIN_EVENT_TABLE(ExampleEditorDlg, wxDialog)
    EVT_BUTTON  (wxID_OK,    ExampleEditorDlg::OnOK)
END_EVENT_TABLE()

ExampleEditorDlg::ExampleEditorDlg(wxWindow* parent, wxWindowID id, 
                                   DataTransExample& ex_,
                                   const vector<DataTransExample>& examples_,
                                   int pos_)
    : wxDialog(parent, id, "Example Editor", 
               wxDefaultPosition, wxDefaultSize, 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
      ex(ex_), examples(examples_), pos(pos_)
{
    name_tc = new wxTextCtrl(this, -1, ex.name.c_str());
    description_tc = new wxTextCtrl(this, -1, ex.description.c_str(),
                                    wxDefaultPosition, wxSize(-1, 80),
                                    wxTE_MULTILINE|wxHSCROLL|wxVSCROLL);
    wxString choices[] = {"useful", "example", "other"};
    category_c = new wxComboBox(this, -1, ex.category.c_str(),
                                wxDefaultPosition, wxDefaultSize,
                                3, choices,
                                wxCB_READONLY);
    code_tc = new wxTextCtrl(this, -1, ex.code.c_str(), 
                             wxDefaultPosition, wxSize(-1, 100),
                             wxTE_MULTILINE|wxHSCROLL|wxVSCROLL);
    inmenu_cb = new wxCheckBox(this, -1, "show item in Data->Fast_DT menu");
    inmenu_cb->SetValue(ex.in_menu);

    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer *flexsizer = new wxFlexGridSizer(2);
    flexsizer->Add(new wxStaticText(this, -1, "name"), 0, wxALL, 5);
    flexsizer->Add(name_tc, 0, wxALL|wxEXPAND, 5);
    flexsizer->Add(new wxStaticText(this, -1, "category"), 0, wxALL, 5);
    flexsizer->Add(category_c, 0, wxALL|wxEXPAND, 5);
    flexsizer->Add(new wxStaticText(this, -1, "description"), 0, wxALL, 5);
    flexsizer->Add(description_tc, 0, wxALL|wxEXPAND, 5);
    flexsizer->Add(new wxStaticText(this, -1, "code"), 0, wxALL, 5);
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

void ExampleEditorDlg::OnOK(wxCommandEvent &event)
{
    string new_name = name_tc->GetValue().Trim().c_str();
    for (int i = 0; i < size(examples); ++i) 
            if (i != pos && examples[i].name == new_name) {//name is not unique
                name_tc->SetFocus();
                name_tc->SetSelection(-1, -1);
                return;
            }
    // we are here -- name is unique
    ex.name = new_name;
    ex.category = category_c->GetValue().c_str();
    ex.description = description_tc->GetValue().Trim().c_str();
    ex.code = code_tc->GetValue().Trim().c_str();
    ex.in_menu = inmenu_cb->GetValue();
    wxDialog::OnOK(event);
}

/// get path ~/.fityk/filename and create ~/.fityk/ dir if not exists
wxString get_user_conffile(const wxString &filename)
{
    wxString fityk_dir = wxGetHomeDir() + wxFILE_SEP_PATH + config_dirname;
    if (!wxDirExists(fityk_dir))
        wxMkdir(fityk_dir);
    return fityk_dir + wxFILE_SEP_PATH + filename;
}


/// show "Export data" dialog
bool export_data_dlg(wxWindow *parent, bool load_exported)
{
    static wxString dir = ".";
    wxFileDialog fdlg (parent, "Export data to file", dir, "",
                       "x y data (*.dat, *.xy)|*.dat;*.DAT;*.xy;*.XY",
                       wxSAVE | wxOVERWRITE_PROMPT);
    dir = fdlg.GetDirectory();
    if (fdlg.ShowModal() == wxID_OK) {
        string path = fdlg.GetPath().c_str();
        string ds = "@" + S(frame->get_focused_data());
        exec_command(ds + " > '" + path + "'");
        if (load_exported)
            exec_command(ds + " <'" + path + "'");
        return true;
    }
    else
        return false;
}


//=====================    setttings  dialog    ==================

//small utility
wxArrayString stl2wxArrayString(vector<string> const& vs)
{
    wxArrayString wxas; 
    for (vector<string>::const_iterator i = vs.begin(); i != vs.end(); ++i)
        wxas.Add(i->c_str());
    return wxas;
}

SettingsDlg::SettingsDlg(wxWindow* parent, const wxWindowID id)
    : wxDialog(parent, id, "Settings",
               wxDefaultPosition, wxDefaultSize, 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER) 
{
    wxNotebook *nb = new wxNotebook(this, -1);
    wxPanel *page_general = new wxPanel(nb, -1);
    nb->AddPage(page_general, "general");
    wxPanel *page_peakfind = new wxPanel(nb, -1);
    nb->AddPage(page_peakfind, "peak-finding");
    wxPanel *page_fitting = new wxPanel(nb, -1);
    nb->AddPage(page_fitting, "fitting");

    // page general
    autoplot_rb = new wxRadioBox(page_general, -1, "auto-refresh plot",
                                 wxDefaultPosition, wxDefaultSize, 
                                 stl2wxArrayString(
                                      getSettings()->expand_enum("autoplot")));
    autoplot_rb->SetStringSelection(getSettings()->getp("autoplot").c_str());

    wxStaticText *verbosity_st = new wxStaticText(page_general, -1, 
                            "verbosity (amount of messages in output pane)");
    verbosity_ch = new wxChoice(page_general, -1, 
                                wxDefaultPosition, wxDefaultSize,
                                stl2wxArrayString(
                                      getSettings()->expand_enum("verbosity")));
    verbosity_ch->SetStringSelection(getSettings()->getp("verbosity").c_str());
    exit_cb = new wxCheckBox(page_general, -1, 
                             "quit if error or warning was generated");
    exit_cb->SetValue(getSettings()->get_b("exit-on-warning"));

    wxBoxSizer *sizer_general = new wxBoxSizer(wxVERTICAL);
    sizer_general->Add(autoplot_rb, 0, wxEXPAND|wxALL, 5);
    sizer_general->Add(verbosity_st, 0, wxLEFT|wxRIGHT|wxTOP, 5);
    sizer_general->Add(verbosity_ch, 0, wxEXPAND|wxALL, 5);
    sizer_general->Add(exit_cb, 0, wxEXPAND|wxALL, 5);
    page_general->SetSizerAndFit(sizer_general);

    // page peak-finding
    wxStaticText *hc_st = new wxStaticText(page_peakfind, -1, 
                               "factor used to correct detected peak height"); 
    height_correction = new RealNumberCtrl(page_peakfind, -1, 
                                    getSettings()->getp("height-correction"));
    wxStaticText *wc_st = new wxStaticText(page_peakfind, -1, 
                               "factor used to correct detected peak width"); 
    width_correction = new RealNumberCtrl(page_peakfind, -1, 
                                    getSettings()->getp("width-correction"));
    cancel_poos = new wxCheckBox(page_peakfind, -1, 
                             "cancel peak searching, if the highest point"
                             "\nis near the boundary of the given range");
    cancel_poos->SetValue(getSettings()->get_b("cancel-peak-out-of-search"));
    wxBoxSizer *sizer_pf = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *sizer_pf_hc = new wxBoxSizer(wxHORIZONTAL);
    sizer_pf_hc->Add(hc_st, 0, wxALL, 5);
    sizer_pf_hc->Add(height_correction, 0, wxALL, 5);
    sizer_pf->Add(sizer_pf_hc, 0);
    wxBoxSizer *sizer_pf_wc = new wxBoxSizer(wxHORIZONTAL);
    sizer_pf_wc->Add(wc_st, 0, wxALL, 5);
    sizer_pf_wc->Add(width_correction, 0, wxALL, 5);
    sizer_pf->Add(sizer_pf_wc, 0);
    sizer_pf->Add(cancel_poos, 0, wxALL, 5);
    page_peakfind->SetSizerAndFit(sizer_pf);

    // TODO

    wxBoxSizer *top_sizer = new wxBoxSizer (wxVERTICAL);
    top_sizer->Add(nb, 1, wxALL|wxEXPAND, 10);
    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    top_sizer->Add (CreateButtonSizer (wxOK|wxCANCEL), 
                    0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);
}

SettingsDlg::pair_vec SettingsDlg::get_changed_items()
{
    pair_vec result;
    map<string, string> m;
    m["autoplot"] = autoplot_rb->GetStringSelection().c_str();
    m["verbosity"] = verbosity_ch->GetStringSelection().c_str();
    m["exit-on-warning"] = exit_cb->GetValue() ? "1" : "0";
    m["height-correction"] = height_correction->GetValue().c_str();
    m["width-correction"] = width_correction->GetValue().c_str();
    m["cancel-peak-out-of-search"] = cancel_poos->GetValue() ? "1" : "0";
    vector<string> kk = getSettings()->expanp();
    for (vector<string>::const_iterator i = kk.begin(); i != kk.end(); ++i)
        if (m.count(*i) && m[*i] != getSettings()->getp(*i))
            result.push_back(make_pair(*i, m[*i]));
    return result;
}

