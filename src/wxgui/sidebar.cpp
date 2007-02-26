// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

/// In this file:
///  SideBar class, the right hand sidebar in GUI 

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/imaglist.h>

#include "sidebar.h"
#include "fancyrc.h"
#include "listptxt.h"
#include "gradient.h"
#include "cmn.h" //SpinCtrl, ProportionalSplitter, change_color_dlg, ...
#include "gui.h" //frame
#include "mplot.h" 
#include "../common.h" //vector4, join_vector, S(), ...
#include "../ui.h" 
#include "../logic.h"
#include "../data.h"
#include "../sum.h"
#include "../func.h"

#include "img/add.xpm"
#include "img/sum.xpm"
#include "img/rename.xpm"
#include "img/close.xpm"
#include "img/colorsel.xpm"
#include "img/editf.xpm"
#include "img/filter.xpm"
#include "img/shiftup.xpm"
#include "img/dpsize.xpm"
#include "img/convert.xpm"
#include "img/copyfunc.xpm"
#include "img/unused.xpm"
#include "img/zshift.xpm"


using namespace std;

enum {
    ID_DP_LIST       = 27500   ,
    ID_DP_LOOK                 ,
    ID_DP_PSIZE                ,
    ID_DP_PLINE                ,
    ID_DP_PS                   ,
    ID_DP_SHIFTUP              ,
    ID_DP_NEW                  ,
    ID_DP_DUP                  ,
    ID_DP_REN                  ,
    ID_DP_DEL                  ,
    ID_DP_CPF                  ,
    ID_DP_COL                  ,
    ID_FP_FILTER               ,
    ID_FP_LIST                 ,
    ID_FP_NEW                  ,
    ID_FP_DEL                  ,
    ID_FP_EDIT                 ,
    ID_FP_CHTYPE               ,
    ID_FP_COL                  ,
    ID_VP_LIST                 ,
    ID_VP_NEW                  ,
    ID_VP_DEL                  ,
    ID_VP_EDIT                 
};

//===============================================================
//                           SideBar
//===============================================================

void add_bitmap_button(wxWindow* parent, wxWindowID id, char** xpm,
                       wxString const& tip, wxSizer* sizer)
{
    wxBitmapButton *btn = new wxBitmapButton(parent, id, wxBitmap(xpm));
    btn->SetToolTip(tip);
    sizer->Add(btn);
}

BEGIN_EVENT_TABLE(SideBar, ProportionalSplitter)
    EVT_BUTTON (ID_DP_NEW, SideBar::OnDataButtonNew)
    EVT_BUTTON (ID_DP_DUP, SideBar::OnDataButtonDup)
    EVT_BUTTON (ID_DP_REN, SideBar::OnDataButtonRen)
    EVT_BUTTON (ID_DP_DEL, SideBar::OnDataButtonDel)
    EVT_BUTTON (ID_DP_CPF, SideBar::OnDataButtonCopyF)
    EVT_BUTTON (ID_DP_COL, SideBar::OnDataButtonCol)
    EVT_CHOICE (ID_DP_LOOK, SideBar::OnDataLookChanged)
    EVT_SPINCTRL (ID_DP_PSIZE, SideBar::OnDataPSizeChanged)
    EVT_CHECKBOX (ID_DP_PLINE, SideBar::OnDataPLineChanged)
    EVT_CHECKBOX (ID_DP_PS,    SideBar::OnDataPSChanged)
    EVT_SPINCTRL (ID_DP_SHIFTUP, SideBar::OnDataShiftUpChanged)
    EVT_LIST_ITEM_SELECTED(ID_DP_LIST, SideBar::OnDataFocusChanged)
    EVT_LIST_ITEM_DESELECTED(ID_DP_LIST, SideBar::OnDataFocusChanged)
    EVT_LIST_ITEM_FOCUSED(ID_DP_LIST, SideBar::OnDataFocusChanged)
    EVT_CHOICE (ID_FP_FILTER, SideBar::OnFuncFilterChanged)
    EVT_BUTTON (ID_FP_NEW, SideBar::OnFuncButtonNew)
    EVT_BUTTON (ID_FP_DEL, SideBar::OnFuncButtonDel)
    EVT_BUTTON (ID_FP_EDIT, SideBar::OnFuncButtonEdit)
    EVT_BUTTON (ID_FP_CHTYPE, SideBar::OnFuncButtonChType)
    EVT_BUTTON (ID_FP_COL, SideBar::OnFuncButtonCol)
    EVT_LIST_ITEM_FOCUSED(ID_FP_LIST, SideBar::OnFuncFocusChanged)
    EVT_BUTTON (ID_VP_NEW, SideBar::OnVarButtonNew)
    EVT_BUTTON (ID_VP_DEL, SideBar::OnVarButtonDel)
    EVT_BUTTON (ID_VP_EDIT, SideBar::OnVarButtonEdit)
    EVT_LIST_ITEM_FOCUSED(ID_VP_LIST, SideBar::OnVarFocusChanged)
END_EVENT_TABLE()

SideBar::SideBar(wxWindow *parent, wxWindowID id)
: ProportionalSplitter(parent, id, 0.75), bp_func(0), active_function(-1)
{
    //wxPanel *upper = new wxPanel(this, -1);
    //wxBoxSizer *upper_sizer = new wxBoxSizer(wxVERTICAL);
    nb = new wxNotebook(this, -1);
    //upper_sizer->Add(nb, 1, wxEXPAND);
    //upper->SetSizerAndFit(upper_sizer);
    bottom_panel = new wxPanel(this, -1);
    SplitHorizontally(nb, bottom_panel);

    //-----  data page  -----
    data_page = new wxPanel(nb, -1);
    wxBoxSizer *data_sizer = new wxBoxSizer(wxVERTICAL);
    d = new DataListPlusText(data_page, -1, ID_DP_LIST, 
                                vector4(pair<string,int>("No", 43),
                                        pair<string,int>("#F+#Z", 43),
                                        pair<string,int>("Name", 108),
                                        pair<string,int>("File", 0)));
    d->list->set_side_bar(this);
    data_sizer->Add(d, 1, wxEXPAND|wxALL, 1);

    wxBoxSizer *data_look_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxArrayString choices;
    choices.Add(wxT("show all datasets"));
    choices.Add(wxT("show only selected"));
    choices.Add(wxT("shadow unselected"));
    choices.Add(wxT("hide all"));
    data_look = new wxChoice(data_page, ID_DP_LOOK,
                             wxDefaultPosition, wxDefaultSize, choices);
    data_look->Select(0);
    data_look_sizer->Add(data_look, 1, wxEXPAND);
    data_sizer->Add(data_look_sizer, 0, wxEXPAND);

    wxBoxSizer *data_spin_sizer = new wxBoxSizer(wxHORIZONTAL);
    // point-size spin button
    data_spin_sizer->Add(new wxStaticBitmap(data_page, -1, 
                                            wxBitmap(dpsize_xpm)), 
                         0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    dpsize_sc = new SpinCtrl(data_page, ID_DP_PSIZE, 1, 1, 8, 40);
    dpsize_sc->SetToolTip(wxT("data point size"));
    data_spin_sizer->Add(dpsize_sc, 0);
    // line between points
    dpline_cb = new wxCheckBox(data_page, ID_DP_PLINE, wxT("line"));
    dpline_cb->SetToolTip(wxT("line between data points"));
    data_spin_sizer->Add(dpline_cb, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    // line between points
    dpsigma_cb = new wxCheckBox(data_page, ID_DP_PS, wxT("s"));
    dpsigma_cb->SetToolTip(wxT("show std. dev. of y (sigma)"));
    data_spin_sizer->Add(dpsigma_cb, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    // shift-up spin button
    data_spin_sizer->AddStretchSpacer();
    data_spin_sizer->Add(new wxStaticBitmap(data_page, -1, 
                                            wxBitmap(shiftup_xpm)), 
                         0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    shiftup_sc = new SpinCtrl(data_page, ID_DP_SHIFTUP, 0, 0, 80, 40);
    shiftup_sc->SetToolTip(wxT("shift up in \% of plot Y size"));
    data_spin_sizer->Add(shiftup_sc, 0);
    data_sizer->Add(data_spin_sizer, 0, wxEXPAND);

    wxBoxSizer *data_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_bitmap_button(data_page, ID_DP_COL, colorsel_xpm, 
                      wxT("change color"), data_buttons_sizer);
    add_bitmap_button(data_page, ID_DP_NEW, add_xpm, 
                      wxT("new data"), data_buttons_sizer);
    add_bitmap_button(data_page, ID_DP_DUP, sum_xpm, 
                      wxT("duplicate/sum"), data_buttons_sizer);
    add_bitmap_button(data_page, ID_DP_CPF, copyfunc_xpm, 
                      wxT("copy F to next dataset"), data_buttons_sizer);
    add_bitmap_button(data_page, ID_DP_REN, rename_xpm, 
                      wxT("rename"), data_buttons_sizer);
    add_bitmap_button(data_page, ID_DP_DEL, close_xpm, 
                      wxT("delete"), data_buttons_sizer);
    data_sizer->Add(data_buttons_sizer, 0, wxEXPAND);
    data_page->SetSizerAndFit(data_sizer);
    nb->AddPage(data_page, wxT("data"));

    //-----  functions page  -----
    func_page = new wxPanel(nb, -1);
    wxBoxSizer *func_sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *func_filter_sizer = new wxBoxSizer(wxHORIZONTAL);
    func_filter_sizer->Add(new wxStaticBitmap(func_page, -1, 
                                              wxBitmap(filter_xpm)),
                           0, wxALIGN_CENTER_VERTICAL);
    filter_ch = new wxChoice(func_page, ID_FP_FILTER,
                             wxDefaultPosition, wxDefaultSize, 0, 0);
    filter_ch->Append(wxT("list all functions"));
    filter_ch->Select(0);
    func_filter_sizer->Add(filter_ch, 1, wxEXPAND);
    func_sizer->Add(func_filter_sizer, 0, wxEXPAND);

    vector<pair<string,int> > fdata;
    fdata.push_back( pair<string,int>("Name", 54) );
    fdata.push_back( pair<string,int>("Type", 80) );
    fdata.push_back( pair<string,int>("Center", 60) );
    fdata.push_back( pair<string,int>("Area", 0) );
    fdata.push_back( pair<string,int>("Height", 0) );
    fdata.push_back( pair<string,int>("FWHM", 0) );
    f = new ListPlusText(func_page, -1, ID_FP_LIST, fdata);
    f->list->set_side_bar(this);
    func_sizer->Add(f, 1, wxEXPAND|wxALL, 1);
    wxBoxSizer *func_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_bitmap_button(func_page, ID_FP_NEW, add_xpm, 
                      wxT("new function"), func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_DEL, close_xpm, 
                      wxT("delete"), func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_EDIT, editf_xpm, 
                      wxT("edit function"), func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_CHTYPE, convert_xpm, 
                      wxT("change type of function"), func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_COL, colorsel_xpm, 
                      wxT("change color"), func_buttons_sizer);
    func_sizer->Add(func_buttons_sizer, 0, wxEXPAND);
    func_page->SetSizerAndFit(func_sizer);
    nb->AddPage(func_page, wxT("functions"));

    //-----  variables page  -----
    var_page = new wxPanel(nb, -1);
    wxBoxSizer *var_sizer = new wxBoxSizer(wxVERTICAL);
    vector<pair<string,int> > vdata = vector4(
                                     pair<string,int>("Name", 52),
                                     pair<string,int>("#/#", 72), 
                                     pair<string,int>("value", 70),
                                     pair<string,int>("formula", 0) );
    v = new ListPlusText(var_page, -1, ID_VP_LIST, vdata);
    v->list->set_side_bar(this);
    var_sizer->Add(v, 1, wxEXPAND|wxALL, 1);
    wxBoxSizer *var_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_bitmap_button(var_page, ID_VP_NEW, add_xpm, 
                      wxT("new variable"), var_buttons_sizer);
    add_bitmap_button(var_page, ID_VP_DEL, close_xpm, 
                      wxT("delete"), var_buttons_sizer);
    add_bitmap_button(var_page, ID_VP_EDIT, editf_xpm, 
                      wxT("edit variable"), var_buttons_sizer);
    var_sizer->Add(var_buttons_sizer, 0, wxEXPAND);
    var_page->SetSizerAndFit(var_sizer);
    nb->AddPage(var_page, wxT("variables"));

    //-----
    wxBoxSizer* bp_topsizer = new wxBoxSizer(wxVERTICAL);
    bp_label = new wxStaticText(bottom_panel, -1, wxT(""), 
                                wxDefaultPosition, wxDefaultSize, 
                                wxST_NO_AUTORESIZE|wxALIGN_CENTRE);
    bp_topsizer->Add(bp_label, 0, wxEXPAND|wxALL, 5);
    bp_sizer = new wxFlexGridSizer(2, 0, 0);
    bp_sizer->AddGrowableCol(1);
    bp_topsizer->Add(bp_sizer, 1, wxEXPAND);
    bottom_panel->SetSizer(bp_topsizer);
    bottom_panel->SetAutoLayout(true);
}

void SideBar::OnDataButtonNew (wxCommandEvent& WXUNUSED(event))
{
    exec_command("@+");
}

void SideBar::OnDataButtonDup (wxCommandEvent& WXUNUSED(event))
{
    exec_command("@+ = " + join_vector(d->get_selected_data(), " + "));
}

void SideBar::OnDataButtonRen (wxCommandEvent& WXUNUSED(event))
{
    int n = get_focused_data();
    Data *data = AL->get_data(n);
    wxString old_title = s2wx(data->get_title());

    wxString s = wxGetTextFromUser(
                         wxString::Format(wxT("New name for dataset @%i"), n), 
                         wxT("Rename dataset"),
                         old_title);
    if (!s.IsEmpty() && s != old_title)
        exec_command("@" + S(n) + ".title = '" + wx2s(s) + "'");
}

void SideBar::delete_selected_items()
{
    int n = nb->GetSelection();
    if (n < 0)
        return;
    string txt = wx2s(nb->GetPageText(n));
    if (txt == "data")
        exec_command("delete " + join_vector(d->get_selected_data(), ", "));
    else if (txt == "functions")
        exec_command("delete " + join_vector(get_selected_func(), ", "));
    else if (txt == "variables")
        exec_command("delete " + join_vector(get_selected_vars(), ", "));
    else
        assert(0);
}

void SideBar::OnDataButtonCopyF (wxCommandEvent& WXUNUSED(event))
{
    int n = get_focused_data();
    if (n+1 >= AL->get_ds_count())
        return;
    d->list->Select(n, false);
    d->list->Select(n+1, true);
    d->list->Focus(n+1);
    DataFocusChanged();
    string cmd = "@" + S(n+1) + ".F=copy(@" + S(n) + ".F)";
    if (!AL->get_sum(n)->get_zz_names().empty() 
            || !AL->get_sum(n+1)->get_zz_names().empty())
        cmd += "; @" + S(n+1) + ".Z=copy(@" + S(n) + ".Z)";
    exec_command(cmd);
}

void SideBar::OnDataButtonCol (wxCommandEvent& WXUNUSED(event))
{
    int sel_size = d->list->GetSelectedItemCount();
    if (sel_size == 0)
        return;
    else if (sel_size == 1) {
        int n = d->list->GetFirstSelected();
        wxColour col = frame->get_main_plot()->get_data_color(n);
        if (change_color_dlg(col)) {
            frame->get_main_plot()->set_data_color(n, col);
            update_lists();
            frame->refresh_plots(false, true);
        }
    }
    else {//sel_size > 1
        vector<string> sel = d->get_selected_data();
        int first_sel = d->list->GetFirstSelected();
        int last_sel = 0;
        for (int i = first_sel; i != -1; i = d->list->GetNextSelected(i))
            last_sel = i;
        wxColour first_col = frame->get_main_plot()->get_data_color(first_sel);
        wxColour last_col = frame->get_main_plot()->get_data_color(last_sel);
        GradientDlgWithApply<SideBar> gd(this, -1, first_col, last_col, 
                                         this, &SideBar::OnDataColorsChanged);
        gd.ShowModal();
    }
}

void SideBar::OnDataColorsChanged(GradientDlg *gd)
{
    vector<int> selected;
    for (int i = d->list->GetFirstSelected(), c = 0; i != -1; 
                                        i = d->list->GetNextSelected(i), ++c) {
        selected.push_back(i);
        wxColour col = gd->get_value(c / (d->list->GetSelectedItemCount()-1.));
        frame->get_main_plot()->set_data_color(i, col);
    }
    update_lists();
    frame->refresh_plots(false, true);
    for (vector<int>::const_iterator i = selected.begin(); 
            i != selected.end(); ++i) 
    for (int i = 0; i != d->list->GetItemCount(); ++i)
        d->list->Select(i, contains_element(selected, i));
}


void SideBar::OnDataLookChanged (wxCommandEvent& WXUNUSED(event))
{
    frame->refresh_plots(false, true);
}

void SideBar::OnDataShiftUpChanged (wxSpinEvent& WXUNUSED(event))
{
    frame->refresh_plots(false, true);
}

void SideBar::OnDataPSizeChanged (wxSpinEvent& event)
{
    //for (int i = d->list->GetFirstSelected(); i != -1; 
    //                                       i = d->list->GetNextSelected(i))
    //    frame->get_main_plot()->set_data_point_size(i, event.GetPosition());
    // now it is set globally
    frame->get_main_plot()->set_data_point_size(0, event.GetPosition());
    frame->refresh_plots(false, true);
}

void SideBar::OnDataPLineChanged (wxCommandEvent& event)
{
    frame->get_main_plot()->set_data_with_line(0, event.IsChecked());
    frame->refresh_plots(false, true);
}

void SideBar::OnDataPSChanged (wxCommandEvent& event)
{
    frame->get_main_plot()->set_data_with_sigma(0, event.IsChecked());
    frame->refresh_plots(false, true);
}

void SideBar::OnFuncFilterChanged (wxCommandEvent& WXUNUSED(event))
{
    update_lists(false);
}

void SideBar::OnFuncButtonNew (wxCommandEvent& WXUNUSED(event))
{
    string peak_type = frame->get_peak_type();
    string formula = Function::get_formula(peak_type);
    vector<string> varnames = Function::get_varnames_from_formula(formula);
    string t = "%put_name_here = " + peak_type + "(" 
                                      + join_vector(varnames, "= , ") + "= )";
    frame->edit_in_input(t);
}

void SideBar::OnFuncButtonEdit (wxCommandEvent& WXUNUSED(event))
{
    if (!bp_func)
        return;
    string t = bp_func->get_current_assignment(AL->get_variables(), 
                                               AL->get_parameters());
    frame->edit_in_input(t);
}

void SideBar::OnFuncButtonChType (wxCommandEvent& WXUNUSED(event))
{
    warn("Sorry. Changing type of function is not implemented yet.");
}

void SideBar::OnFuncButtonCol (wxCommandEvent& WXUNUSED(event))
{
    vector<int> const& ffi 
        = AL->get_sum(AL->get_active_ds_position())->get_ff_idx();
    vector<int>::const_iterator in_ff = find(ffi.begin(), ffi.end(), 
                                                         active_function);
    if (in_ff == ffi.end())
        return;
    int color_id = in_ff - ffi.begin();
    wxColour col = frame->get_main_plot()->get_func_color(color_id);
    if (change_color_dlg(col)) {
        frame->get_main_plot()->set_func_color(color_id, col);
        update_lists();
        frame->refresh_plots(false, true);
    }
}

void SideBar::OnVarButtonNew (wxCommandEvent& WXUNUSED(event))
{
    frame->edit_in_input("$put_name_here = ");
}

void SideBar::OnVarButtonEdit (wxCommandEvent& WXUNUSED(event))
{
    int n = get_focused_var();
    if (n < 0 || n >= size(AL->get_variables()))
        return;
    Variable const* var = AL->get_variable(n);
    string t = var->xname + " = "+ var->get_formula(AL->get_parameters());
    frame->edit_in_input(t);
}

void SideBar::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/SideBar"));
    d->split(cfg_read_double(cf, wxT("dataProportion"), 0.75));
    f->split(cfg_read_double(cf, wxT("funcProportion"), 0.75));
    v->split(cfg_read_double(cf, wxT("varProportion"), 0.75));
}

void SideBar::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/SideBar"));
}

void SideBar::update_lists(bool nondata_changed)
{
    Freeze();
    d->update_data_list(nondata_changed, true);
    update_func_list(nondata_changed);
    update_var_list();
    
    //-- enable/disable buttons
    bool not_the_last = get_focused_data()+1 < AL->get_ds_count();
    data_page->FindWindow(ID_DP_CPF)->Enable(not_the_last);
    bool has_any_funcs = (f->list->GetItemCount() > 0);
    func_page->FindWindow(ID_FP_DEL)->Enable(has_any_funcs);
    func_page->FindWindow(ID_FP_EDIT)->Enable(has_any_funcs);
    func_page->FindWindow(ID_FP_CHTYPE)->Enable(has_any_funcs);
    func_page->FindWindow(ID_FP_COL)->Enable(has_any_funcs);
    bool has_any_vars = (v->list->GetItemCount() > 0);
    var_page->FindWindow(ID_VP_DEL)->Enable(has_any_vars);
    var_page->FindWindow(ID_VP_EDIT)->Enable(has_any_vars);

    int n = get_focused_data();
    dpline_cb->SetValue(frame->get_main_plot()->get_data_with_line(n));
    dpsize_sc->SetValue(frame->get_main_plot()->get_data_point_size(n));

    update_data_inf();
    update_func_inf();
    update_var_inf();
    update_bottom_panel();
    Thaw();
}


void SideBar::update_func_list(bool nondata_changed)
{
    MainPlot const* mplot = frame->get_main_plot();
    wxColour const& bg_col = mplot->get_bg_color();

    //functions filter
    while ((int) filter_ch->GetCount() > AL->get_ds_count() + 1)
        filter_ch->Delete(filter_ch->GetCount()-1);
    for (int i = filter_ch->GetCount() - 1; i < AL->get_ds_count(); ++i)
        filter_ch->Append(wxString::Format(wxT("only functions from @%i"), i));

    //functions
    static vector<int> func_col_id;
    static int old_func_size;
    vector<string> func_data;
    vector<int> new_func_col_id;
    int active_ds_pos = AL->get_active_ds_position();
    Sum const* sum = AL->get_sum(active_ds_pos);
    Sum const* filter_sum = 0;
    if (filter_ch->GetSelection() > 0)
        filter_sum = AL->get_sum(filter_ch->GetSelection()-1);
    int func_size = AL->get_functions().size();
    if (active_function == -1)
        active_function = func_size - 1;
    else {
        if (active_function >= func_size || 
                AL->get_function(active_function) != bp_func)
            active_function = AL->find_function_nr(active_function_name);
        if (active_function == -1 || func_size == old_func_size+1)
            active_function = func_size - 1;
    }
    if (active_function != -1)
        active_function_name = AL->get_function(active_function)->name;
    else
        active_function_name = "";

    int pos = -1;
    for (int i = 0; i < func_size; ++i) {
        if (filter_sum && !contains_element(filter_sum->get_ff_idx(), i)
                           && !contains_element(filter_sum->get_zz_idx(), i))
            continue;
        if (i == active_function)
            pos = new_func_col_id.size();
        Function const* f = AL->get_function(i);
        func_data.push_back(f->name);
        func_data.push_back(f->type_name);
        func_data.push_back(f->has_center() ? S(f->center()).c_str() : "-");
        func_data.push_back(f->has_area() ? S(f->area()).c_str() : "-");
        func_data.push_back(f->has_height() ? S(f->height()).c_str() : "-");
        func_data.push_back(f->has_fwhm() ? S(f->fwhm()).c_str() : "-");
        vector<int> const& ffi = sum->get_ff_idx();
        vector<int> const& zzi = sum->get_zz_idx();
        vector<int>::const_iterator in_ff = find(ffi.begin(), ffi.end(), i);
        int color_id = -2;
        if (in_ff != ffi.end())
            color_id = in_ff - ffi.begin();
        else if (find(zzi.begin(), zzi.end(), i) != zzi.end())
            color_id = -1;
        new_func_col_id.push_back(color_id);
    }
    wxImageList* func_images = 0;
    if (nondata_changed || func_col_id != new_func_col_id) {
        func_col_id = new_func_col_id;
        func_images = new wxImageList(16, 16);
        for (vector<int>::const_iterator i = func_col_id.begin(); 
                                                i != func_col_id.end(); ++i) {
            if (*i == -2)
                func_images->Add(wxBitmap(unused_xpm));
            else if (*i == -1)
                func_images->Add(wxBitmap(zshift_xpm));
            else 
                func_images->Add(make_color_bitmap16(mplot->get_func_color(*i), 
                                                     bg_col));
        }
    }
    old_func_size = func_size;
    f->list->populate(func_data, func_images, pos);
}

void SideBar::update_var_list()
{
    vector<string> var_data;
    //  count references first
    vector<Variable*> const& variables = AL->get_variables();
    vector<int> var_vrefs(variables.size(), 0), var_frefs(variables.size(), 0);
    for (vector<Variable*>::const_iterator i = variables.begin();
                                                i != variables.end(); ++i) {
        for (int j = 0; j != (*i)->get_vars_count(); ++j)
            var_vrefs[(*i)->get_var_idx(j)]++;
    }
    for (vector<Function*>::const_iterator i = AL->get_functions().begin();
                                         i != AL->get_functions().end(); ++i) {
        for (int j = 0; j != (*i)->get_vars_count(); ++j)
            var_frefs[(*i)->get_var_idx(j)]++;
    }

    for (int i = 0; i < size(variables); ++i) {
        Variable const* v = variables[i];
        var_data.push_back(v->name);           //name
        string refs = S(var_frefs[i]) + "+" + S(var_vrefs[i]) + " / " 
                      + S(v->get_vars_count());
        var_data.push_back(refs); //refs
        var_data.push_back(S(v->get_value())); //value
        var_data.push_back(v->get_formula(AL->get_parameters()));  //formula
    }
    v->list->populate(var_data);
}

int SideBar::get_focused_data() const
{ 
    int n = d->list->GetFocusedItem(); 
    return n == -1 ? 0 : n; 
}

int SideBar::get_focused_var() const 
{ 
    if (AL->get_variables().empty())
        return -1;
    else {
        int n = v->list->GetFocusedItem(); 
        return n > 0 ? n : 0;
    }
}

bool SideBar::is_func_selected(int n) const 
{ 
    return f->list->IsSelected(n) || f->list->GetFocusedItem() == n; 
}

void SideBar::activate_function(int n)
{
    active_function = n;
    do_activate_function();
    int pos = n;
    if (filter_ch->GetSelection() > 0)
        pos = f->list->FindItem(-1, s2wx(active_function_name));
    f->list->Focus(pos);
    for (int i = 0; i != f->list->GetItemCount(); ++i)
        f->list->Select(i, i==pos);
}

void SideBar::do_activate_function()
{
    if (active_function != -1)
        active_function_name = AL->get_function(active_function)->name;
    else
        active_function_name = "";
    frame->refresh_plots(false, true);
    update_func_inf();
    update_bottom_panel();
}

void SideBar::DataFocusChanged()
{
    int n = d->list->GetFocusedItem();
    if (n < 0)
        return;
    int length = AL->get_ds_count();
    if (length > 1 && AL->get_active_ds_position() != n) 
        exec_command("plot .. in @" + S(n));
    data_page->FindWindow(ID_DP_CPF)->Enable(n+1<length);
}

string SideBar::get_datasets_str()
{
    if (data_look->GetSelection() == 0)
        return "@*";
    else
        return frame->get_active_data_str();
}

void SideBar::update_data_inf()
{
    int n = get_focused_data();
    if (n < 0)
        return;
    wxTextCtrl* inf = d->inf;
    inf->Clear();
    wxTextAttr defattr = inf->GetDefaultStyle();
    wxFont font = defattr.GetFont();
    wxTextAttr boldattr = defattr;
    font.SetWeight(wxFONTWEIGHT_BOLD);
    boldattr.SetFont(font);

    inf->SetDefaultStyle(boldattr);
    inf->AppendText(s2wx("@" + S(n) + "\n"));
    inf->SetDefaultStyle(defattr);
    inf->AppendText(s2wx(AL->get_data(n)->getInfo()));
    inf->ShowPosition(0);
}

void SideBar::update_func_inf()
{
    wxTextCtrl* inf = f->inf;
    inf->Clear();
    if (active_function < 0)
        return;
    wxTextAttr defattr = inf->GetDefaultStyle();
    wxFont font = defattr.GetFont();
    wxTextAttr boldattr = defattr;
    font.SetWeight(wxFONTWEIGHT_BOLD);
    boldattr.SetFont(font);

    Function const* func = AL->get_function(active_function);
    inf->SetDefaultStyle(boldattr);
    inf->AppendText(s2wx(func->xname));
    inf->SetDefaultStyle(defattr);
    if (func->has_center()) 
        inf->AppendText(wxT("\nCenter: ") + s2wx(S(func->center())));
    if (func->has_area()) 
        inf->AppendText(wxT("\nArea: ") + s2wx(S(func->area())));
    if (func->has_height()) 
        inf->AppendText(wxT("\nHeight: ") + s2wx(S(func->height())));
    if (func->has_fwhm()) 
        inf->AppendText(wxT("\nFWHM: ") + s2wx(S(func->fwhm())));
    if (func->has_iwidth()) 
        inf->AppendText(wxT("\nInt. Width: ") + s2wx(S(func->iwidth())));
    vector<string> in;
    for (int i = 0; i < AL->get_ds_count(); ++i) {
        if (contains_element(AL->get_sum(i)->get_ff_idx(), active_function))
            in.push_back("@" + S(i) + ".F");
        if (contains_element(AL->get_sum(i)->get_zz_idx(), active_function))
            in.push_back("@" + S(i) + ".Z");
    }
    if (!in.empty())
        inf->AppendText(s2wx("\nIn: " + join_vector(in, ", ")));
    inf->ShowPosition(0);
}

void SideBar::update_var_inf()
{
    wxTextCtrl* inf = v->inf;
    inf->Clear();
    int n = get_focused_var();
    if (n < 0)
        return;
    wxTextAttr defattr = inf->GetDefaultStyle();
    wxFont font = defattr.GetFont();
    wxTextAttr boldattr = defattr;
    font.SetWeight(wxFONTWEIGHT_BOLD);
    boldattr.SetFont(font);

    Variable const* var = AL->get_variable(n);
    inf->SetDefaultStyle(boldattr);
    string t = var->xname + " = " + var->get_formula(AL->get_parameters());
    inf->AppendText(s2wx(t));
    inf->SetDefaultStyle(defattr);
    vector<string> in = AL->get_variable_references(var->name);
    if (!in.empty())
        inf->AppendText(s2wx("\nIn:\n    " + join_vector(in, "\n    ")));
    inf->ShowPosition(0);
}

void SideBar::add_variable_to_bottom_panel(Variable const* var, 
                                           string const& tv_name)
{
    wxStaticText* name_st = new wxStaticText(bottom_panel, -1, s2wx(tv_name));
    bp_sizer->Add(name_st, 0, wxALL|wxALIGN_CENTER_VERTICAL, 1);
    bp_statict.push_back(name_st);
    if (var->is_simple() || var->is_constant()) {
        FancyRealCtrl *frc = new FancyRealCtrl(bottom_panel, -1,
                                               var->get_value(), var->xname,
                                               !var->is_simple(), this);
        frc->connect_to_onkeydown(wxKeyEventHandler(FFrame::focus_input), frame);
        bp_sizer->Add(frc, 1, wxALL|wxEXPAND, 1);
        bp_frc.push_back(frc);
    }
    else {
        string t = var->xname + " = " + var->get_formula(AL->get_parameters());
        wxStaticText *var_st = new wxStaticText(bottom_panel, -1, s2wx(t));
        bp_sizer->Add(var_st, 1, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 1);
        bp_statict.push_back(var_st);
        bp_frc.push_back(0);
    }
}

void SideBar::clear_bottom_panel()
{
    for (vector<wxStaticText*>::iterator i = bp_statict.begin(); 
                                                i != bp_statict.end(); ++i)
        (*i)->Destroy();
    bp_statict.clear();
    for (vector<FancyRealCtrl*>::iterator i = bp_frc.begin(); 
                                                      i != bp_frc.end(); ++i)
        if (*i)
            (*i)->Destroy();
    bp_frc.clear();
    bp_label->SetLabel(wxT(""));
    bp_sig.clear();
}

vector<bool> SideBar::make_bottom_panel_sig(Function const* func)
{
    vector<bool> sig;
    for (int i = 0; i < size(func->type_var_names); ++i) {
        Variable const* var = AL->get_variable(func->get_var_idx(i));
        sig.push_back(var->is_simple() || var->is_constant());
    }
    return sig;
}

void SideBar::draw_function_draft(FancyRealCtrl const* frc) const
{
    vector<fp> p_values(bp_func->nv);
    for (int i = 0; i < bp_func->nv; ++i) {
        string xname = "$" + bp_func->get_var_name(i);
        p_values[i] = frc->get_name() != xname ? bp_func->get_var_value(i) 
                                               : frc->get_value();
    }
    frame->get_main_plot()->draw_xor_peak(bp_func, p_values);
}

void SideBar::change_bp_parameter_value(int idx, double value)
{
    if (idx < (int)bp_frc.size()) 
        bp_frc[idx]->set_temporary_value(value); 
}

void SideBar::update_bottom_panel()
{
    if (active_function < 0) {
        clear_bottom_panel();
        bp_func = 0;
        return;
    }
    bottom_panel->Freeze();
    bp_func = AL->get_function(active_function);
    wxString new_label = s2wx(bp_func->xname + " : " + bp_func->type_name);
    if (bp_label->GetLabel() != new_label)
        bp_label->SetLabel(new_label);
    vector<bool> sig = make_bottom_panel_sig(bp_func);
    if (sig != bp_sig) {
        clear_bottom_panel();
        bp_sig = sig;
        for (int i = 0; i < bp_func->nv; ++i) {
            Variable const* var = AL->get_variable(bp_func->get_var_idx(i));
            add_variable_to_bottom_panel(var, bp_func->type_var_names[i]);
        }
        int sash_pos = GetClientSize().GetHeight() - 3
                         - bottom_panel->GetSizer()->GetMinSize().GetHeight();
        if (sash_pos < GetSashPosition())
            SetSashPosition(max(50, sash_pos));
    }
    else {
        vector<wxStaticText*>::iterator st = bp_statict.begin();
        for (int i = 0; i < bp_func->nv; ++i) {
            string const& t = bp_func->type_var_names[i];
            (*st)->SetLabel(s2wx(t));
            ++st;
            Variable const* var = AL->get_variable(bp_func->get_var_idx(i));
            if (var->is_simple() || var->is_constant()) {
                bp_frc[i]->set(var->get_value(), var->xname);
                if (bp_frc[i]->get_locked() != !var->is_simple())
                    bp_frc[i]->toggle_lock(false);
            }
            else {
                assert (bp_frc[i] == 0);
                string f = var->xname + " = " 
                                   + var->get_formula(AL->get_parameters());
                (*st)->SetLabel(s2wx(f));
                ++st;
            }
        }
    }
    bottom_panel->Layout();
    bottom_panel->Thaw();
}

bool SideBar::howto_plot_dataset(int n, bool& shadowed, int& offset) const
{
    // choice_idx: 0: "show all datasets" 
    //             1: "show only selected" 
    //             2: "shadow unselected" 
    //             3: "hide all"
    int choice_idx = data_look->GetSelection();
    bool sel = d->list->IsSelected(n) || d->list->GetFocusedItem() == n;
    if ((choice_idx == 1 && !sel) || choice_idx == 3)
        return false;
    shadowed = (choice_idx == 2 && !sel);
    offset = n * shiftup_sc->GetValue();
    return true;
}

vector<string> SideBar::get_selected_func() const
{
    vector<string> dd;
    for (int i = f->list->GetFirstSelected(); i != -1; 
                                            i = f->list->GetNextSelected(i))
        dd.push_back(AL->get_function(i)->xname);
    if (dd.empty() && f->list->GetItemCount() > 0) {
        int n = f->list->GetFocusedItem();
        dd.push_back(AL->get_function(n == -1 ? 0 : n)->xname);
    }
    return dd;
}

vector<string> SideBar::get_selected_vars() const
{
    vector<string> dd;
    for (int i = v->list->GetFirstSelected(); i != -1; 
                                             i = v->list->GetNextSelected(i))
        dd.push_back(AL->get_variable(i)->xname);
    if (dd.empty() && v->list->GetItemCount() > 0) {
        int n = v->list->GetFocusedItem();
        dd.push_back(AL->get_function(n == -1 ? 0 : n)->xname);
    }
    return dd;
}


void SideBar::OnFuncFocusChanged(wxListEvent& WXUNUSED(event))
{
    int n = f->list->GetFocusedItem(); 
    if (n == -1)
        active_function = -1;
    else {
        string name = wx2s(f->list->GetItemText(n));
        active_function = AL->find_function_nr(name);
    }
    do_activate_function();
}

void SideBar::OnVarFocusChanged(wxListEvent& WXUNUSED(event))
{
    update_var_inf();
}


