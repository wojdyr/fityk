// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// class SideBar

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/imaglist.h>
#include <wx/tglbtn.h>

#include "sidebar.h"
#include "fancyrc.h"
#include "listptxt.h"
#include "gradient.h"
#include "cmn.h" //SpinCtrl, ProportionalSplitter, change_color_dlg, ...
#include "frame.h" //frame
#include "plotpane.h"
#include "mplot.h"
#include "../common.h" //vector4, join_vector, S(), ...
#include "../ui.h"
#include "../logic.h"
#include "../data.h"
#include "../model.h"
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
//#include "img/convert.xpm"
#include "img/copyfunc.xpm"
#include "img/unused.xpm"
#include "img/zshift.xpm"
#include "img/eq_fwhm.h"
#include "img/eq_shape.h"


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
    ID_FP_HWHM                 ,
    ID_FP_SHAPE                ,
    ID_VP_LIST                 ,
    ID_VP_NEW                  ,
    ID_VP_DEL                  ,
    ID_VP_EDIT
};

//===============================================================
//                           SideBar
//===============================================================

void add_bitmap_button(wxWindow* parent, wxWindowID id, const char** xpm,
                       wxString const& tip, wxSizer* sizer)
{
    wxBitmapButton *btn = new wxBitmapButton(parent, id, wxBitmap(xpm));
    btn->SetToolTip(tip);
    sizer->Add(btn);
}

// wxToggleBitmapButton was added in 2.9. We use wxToggleButton instead
void add_toggle_bitmap_button(wxWindow* parent, wxWindowID id,
                              wxString const& /*label*/, const wxBitmap& bmp,
                              wxString const& tip, wxSizer* sizer)
{
    /*
    wxToggleButton *btn = new wxToggleButton(parent, id, label,
                                             wxDefaultPosition, wxDefaultSize,
                                             wxBU_EXACTFIT);
    */
    wxBitmapToggleButton *btn = new wxBitmapToggleButton(parent, id, bmp);
    btn->SetToolTip(tip);
    sizer->Add(btn, wxSizerFlags().Expand());
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
    EVT_LIST_ITEM_SELECTED(ID_DP_LIST, SideBar::OnDataSelectionChanged)
    EVT_LIST_ITEM_DESELECTED(ID_DP_LIST, SideBar::OnDataSelectionChanged)
    EVT_LIST_ITEM_FOCUSED(ID_DP_LIST, SideBar::OnDataFocusChanged)
    EVT_CHOICE (ID_FP_FILTER, SideBar::OnFuncFilterChanged)
    EVT_BUTTON (ID_FP_NEW, SideBar::OnFuncButtonNew)
    EVT_BUTTON (ID_FP_DEL, SideBar::OnFuncButtonDel)
    EVT_BUTTON (ID_FP_EDIT, SideBar::OnFuncButtonEdit)
    EVT_BUTTON (ID_FP_CHTYPE, SideBar::OnFuncButtonChType)
    EVT_BUTTON (ID_FP_COL, SideBar::OnFuncButtonCol)
    EVT_TOGGLEBUTTON (ID_FP_HWHM, SideBar::OnFuncButtonHwhm)
    EVT_TOGGLEBUTTON (ID_FP_SHAPE, SideBar::OnFuncButtonShape)
    EVT_LIST_ITEM_FOCUSED(ID_FP_LIST, SideBar::OnFuncFocusChanged)
    EVT_LIST_ITEM_SELECTED(ID_FP_LIST, SideBar::OnFuncSelectionChanged)
    EVT_LIST_ITEM_DESELECTED(ID_FP_LIST, SideBar::OnFuncSelectionChanged)
    EVT_BUTTON (ID_VP_NEW, SideBar::OnVarButtonNew)
    EVT_BUTTON (ID_VP_DEL, SideBar::OnVarButtonDel)
    EVT_BUTTON (ID_VP_EDIT, SideBar::OnVarButtonEdit)
    EVT_LIST_ITEM_FOCUSED(ID_VP_LIST, SideBar::OnVarFocusChanged)
    EVT_LIST_ITEM_SELECTED(ID_VP_LIST, SideBar::OnVarSelectionChanged)
    EVT_LIST_ITEM_DESELECTED(ID_VP_LIST, SideBar::OnVarSelectionChanged)
END_EVENT_TABLE()

SideBar::SideBar(wxWindow *parent, wxWindowID id)
    : ProportionalSplitter(parent, id, 0.75),
      pp_func(NULL), active_function(-1),
      skipOnFuncFocusChanged_(false)
{
    //wxPanel *upper = new wxPanel(this, -1);
    //wxBoxSizer *upper_sizer = new wxBoxSizer(wxVERTICAL);
    nb = new wxNotebook(this, -1);
    //upper_sizer->Add(nb, 1, wxEXPAND);
    //upper->SetSizerAndFit(upper_sizer);
    param_panel = new ParameterPanel(this, -1, this);
    param_panel->set_key_sink(frame, wxKeyEventHandler(FFrame::focus_input));
    SplitHorizontally(nb, param_panel);

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
    data_look_sizer->Add(data_look, 1, wxEXPAND);
    data_sizer->Add(data_look_sizer, 0, wxEXPAND);

    wxBoxSizer *data_spin_sizer = new wxBoxSizer(wxHORIZONTAL);
    // point-size spin button
    data_spin_sizer->Add(new wxStaticBitmap(data_page, -1,
                                            wxBitmap(dpsize_xpm)),
                         0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    dpsize_sc = new SpinCtrl(data_page, ID_DP_PSIZE, 1, 1, 9, 40);
    dpsize_sc->SetToolTip(wxT("data point size"));
    data_spin_sizer->Add(dpsize_sc, 0);
    // line between points
    dpline_cb = new wxCheckBox(data_page, ID_DP_PLINE, wxT("line"));
    dpline_cb->SetToolTip(wxT("line between data points"));
    data_spin_sizer->Add(dpline_cb, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    // line between points
    dpsigma_cb = new wxCheckBox(data_page, ID_DP_PS, wxT("\u03C3")); // sigma
    dpsigma_cb->SetToolTip(wxT("show std. dev. of y"));
    data_spin_sizer->Add(dpsigma_cb, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    // shift-up spin button
    data_spin_sizer->AddStretchSpacer();
    data_spin_sizer->Add(new wxStaticBitmap(data_page, -1,
                                            wxBitmap(shiftup_xpm)),
                         0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    shiftup_sc = new SpinCtrl(data_page, ID_DP_SHIFTUP, 0, 0, 80, 50);
    shiftup_sc->SetToolTip(wxT("shift up (in % of plot height)"));
    data_spin_sizer->Add(shiftup_sc, 0);
    data_sizer->Add(data_spin_sizer, 0, wxEXPAND);

    wxBoxSizer *data_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_bitmap_button(data_page, ID_DP_COL, colorsel_xpm,
                      wxT("change color"), data_buttons_sizer);
    //add_bitmap_button(data_page, ID_DP_NEW, add_xpm,
    //                  wxT("new data"), data_buttons_sizer);
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
    add_toggle_bitmap_button(func_page, ID_FP_HWHM, wxT("=W"), GET_BMP(eq_fwhm),
                             wxT("same HWHM for all functions"),
                             func_buttons_sizer);
    add_toggle_bitmap_button(func_page, ID_FP_SHAPE, wxT("=S"),
                             GET_BMP(eq_shape),
                             wxT("same shape for all functions"),
                             func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_NEW, add_xpm,
                      wxT("new function"), func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_EDIT, editf_xpm,
                      wxT("edit function"), func_buttons_sizer);
    //add_bitmap_button(func_page, ID_FP_CHTYPE, convert_xpm,
    //                  wxT("change type of function"), func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_COL, colorsel_xpm,
                      wxT("change color"), func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_DEL, close_xpm,
                      wxT("delete"), func_buttons_sizer);
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
}

void SideBar::OnDataButtonNew (wxCommandEvent&)
{
    ftk->exec("@+ = 0");
}

void SideBar::OnDataButtonDup (wxCommandEvent&)
{
    ftk->exec("@+ = " + join_vector(d->get_selected_data(), " + "));
}

void SideBar::OnDataButtonRen (wxCommandEvent&)
{
    int n = get_focused_data();
    Data *data = ftk->get_data(n);
    wxString old_title = s2wx(data->get_title());

    wxString s = wxGetTextFromUser(
                         wxString::Format(wxT("New name for dataset @%i"), n),
                         wxT("Rename dataset"),
                         old_title);
    if (!s.IsEmpty() && s != old_title)
        ftk->exec("@" + S(n) + ".title = '" + wx2s(s) + "'");
}

void SideBar::delete_selected_items()
{
    int n = nb->GetSelection();
    if (n < 0)
        return;
    string txt = wx2s(nb->GetPageText(n));
    vector<string> elems;
    if (txt == "data") {
        elems = d->get_selected_data();
    }
    else if (txt == "functions") {
        elems = get_selected_func();
        vm_foreach (string, i, elems)
            *i = "%" + *i;
    }
    else if (txt == "variables") {
        elems = get_selected_vars();
        vm_foreach (string, i, elems)
            *i = "$" + *i;
    }
    else
        assert(0);
    ftk->exec("delete " + join_vector(elems, ", "));
}

void SideBar::OnDataButtonCopyF (wxCommandEvent&)
{
    int n = get_focused_data();
    if (n+1 >= ftk->get_dm_count())
        return;
    d->list->Select(n, false);
    d->list->Select(n+1, true);
    d->list->Focus(n+1);
    string cmd = "@" + S(n+1) + ".F=copy(@" + S(n) + ".F)";
    if (!ftk->get_model(n)->get_zz().names.empty()
            || !ftk->get_model(n+1)->get_zz().names.empty())
        cmd += "; @" + S(n+1) + ".Z=copy(@" + S(n) + ".Z)";
    if (ftk->find_function_nr("bg" + S(n)) != -1)
        cmd += "; %bg" + S(n+1) + "=copy(%bg" + S(n) + ")";
    update_data_buttons();
    ftk->exec(cmd);
}

void SideBar::OnDataButtonCol (wxCommandEvent&)
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
            frame->plot_pane()->refresh_plots(false, kMainPlot);
        }
    }
    else {//sel_size > 1
        int first_sel = d->list->GetFirstSelected();
        int last_sel = 0;
        for (int i = first_sel; i != -1; i = d->list->GetNextSelected(i))
            last_sel = i;
        wxColour first_col = frame->get_main_plot()->get_data_color(first_sel);
        wxColour last_col = frame->get_main_plot()->get_data_color(last_sel);
        GradientDlg gd(this, -1, first_col, last_col);
        gd.FindWindow(wxID_APPLY)->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
                        wxCommandEventHandler(SideBar::OnDataColorsChanged),
                        NULL, this);
        gd.ShowModal();
    }
}

void SideBar::OnDataColorsChanged(wxCommandEvent& event)
{
    wxWindow* object = wxDynamicCast(event.GetEventObject(), wxWindow);
    if (object == NULL || object->GetId() != wxID_APPLY)
        return;
    GradientDlg *gd = static_cast<GradientDlg*>(object->GetParent());

    vector<int> selected;
    for (int i = d->list->GetFirstSelected(), c = 0; i != -1;
                                        i = d->list->GetNextSelected(i), ++c) {
        selected.push_back(i);
        wxColour col = gd->get_value(c / (d->list->GetSelectedItemCount()-1.));
        frame->get_main_plot()->set_data_color(i, col);
    }
    update_lists();
    frame->plot_pane()->refresh_plots(false, kMainPlot);
    for (int i = 0; i != d->list->GetItemCount(); ++i)
        d->list->Select(i, contains_element(selected, i));
}


void SideBar::OnDataLookChanged (wxCommandEvent&)
{
    frame->plot_pane()->refresh_plots(false, kMainPlot);
}

void SideBar::OnDataShiftUpChanged (wxSpinEvent&)
{
    frame->plot_pane()->refresh_plots(false, kMainPlot);
}

void SideBar::OnDataPSizeChanged (wxSpinEvent& event)
{
    //for (int i = d->list->GetFirstSelected(); i != -1;
    //                                       i = d->list->GetNextSelected(i))
    //    frame->get_main_plot()->set_data_point_size(i, event.GetPosition());
    // now it is set globally
    frame->get_main_plot()->set_data_point_size(0, event.GetPosition());
    frame->plot_pane()->refresh_plots(false, kMainPlot);
}

void SideBar::OnDataPLineChanged (wxCommandEvent& event)
{
    frame->get_main_plot()->set_data_with_line(0, event.IsChecked());
    frame->plot_pane()->refresh_plots(false, kMainPlot);
}

void SideBar::OnDataPSChanged (wxCommandEvent& event)
{
    frame->get_main_plot()->set_data_with_sigma(0, event.IsChecked());
    frame->plot_pane()->refresh_plots(false, kMainPlot);
}

void SideBar::OnFuncFilterChanged (wxCommandEvent&)
{
    update_lists(false);
}

void SideBar::OnFuncButtonNew (wxCommandEvent&)
{
    string peak_type = frame->get_peak_type();
    const Tplate *tp = ftk->get_tpm()->get_tp(peak_type);
    string t = "%put_name_here = " + peak_type + "("
                                      + join_vector(tp->fargs, "= , ") + "= )";
    frame->edit_in_input(t);
}

void SideBar::OnFuncButtonEdit (wxCommandEvent&)
{
    if (!pp_func)
        return;
    string t = pp_func->get_current_assignment(ftk->variables(),
                                               ftk->parameters());
    frame->edit_in_input(t);
}

void SideBar::OnFuncButtonChType (wxCommandEvent&)
{
    ftk->warn("Sorry. Changing type of function is not implemented yet.");
}

void SideBar::OnFuncButtonCol (wxCommandEvent&)
{
    vector<int> const& ffi
        = ftk->get_model(frame->get_focused_data_index())->get_ff().idx;
    vector<int>::const_iterator in_ff = find(ffi.begin(), ffi.end(),
                                                         active_function);
    if (in_ff == ffi.end())
        return;
    int color_id = in_ff - ffi.begin();
    wxColour col = frame->get_main_plot()->get_func_color(color_id);
    if (change_color_dlg(col)) {
        frame->get_main_plot()->set_func_color(color_id, col);
        update_lists();
        frame->plot_pane()->refresh_plots(false, kMainPlot);
    }
}

void SideBar::OnVarButtonNew (wxCommandEvent&)
{
    frame->edit_in_input("$put_name_here = ");
}

void SideBar::OnVarButtonEdit (wxCommandEvent&)
{
    int n = get_focused_var();
    if (n < 0 || n >= size(ftk->variables()))
        return;
    Variable const* var = ftk->get_variable(n);
    string t = "$" + var->name + " = "+ var->get_formula(ftk->parameters());
    frame->edit_in_input(t);
}

void SideBar::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/SideBar"));
    d->split(cfg_read_double(cf, wxT("dataProportion"), 0.75));
    f->split(cfg_read_double(cf, wxT("funcProportion"), 0.75));
    v->split(cfg_read_double(cf, wxT("varProportion"), 0.75));
    data_look->Select(cf->Read(wxT("dataLook"), 1L));
}

void SideBar::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/SideBar"));
    cf->Write(wxT("dataProportion"), d->GetProportion());
    cf->Write(wxT("funcProportion"), f->GetProportion());
    cf->Write(wxT("varProportion"), v->GetProportion());
    cf->Write(wxT("dataLook"), data_look->GetSelection());
}

void SideBar::update_lists(bool nondata_changed)
{
    Freeze();
    d->update_data_list(nondata_changed);
    update_func_list(nondata_changed);
    update_var_list();

    //-- enable/disable buttons
    update_data_buttons();
    update_func_buttons();
    update_var_buttons();

    int n = get_focused_data();
    dpline_cb->SetValue(frame->get_main_plot()->get_data_with_line(n));
    dpsize_sc->SetValue(frame->get_main_plot()->get_data_point_size(n));

    update_data_inf();
    update_func_inf();
    update_var_inf();
    update_param_panel();
    Thaw();
}


void SideBar::update_func_list(bool nondata_changed)
{
    MainPlot const* mplot = frame->get_main_plot();
    wxColour const& bg_col = mplot->get_bg_color();

    //functions filter
    while ((int) filter_ch->GetCount() > ftk->get_dm_count() + 1)
        filter_ch->Delete(filter_ch->GetCount()-1);
    for (int i = filter_ch->GetCount() - 1; i < ftk->get_dm_count(); ++i)
        filter_ch->Append(wxString::Format(wxT("only functions from @%i"), i));

    //functions
    static vector<int> func_col_id;
    static int old_func_size;
    vector<string> func_data;
    vector<int> new_func_col_id;
    Model const* model = ftk->get_model(frame->get_focused_data_index());
    // if filter is selected, only functions in `filter_model' are listed
    Model const* filter_model = NULL;
    if (filter_ch->GetSelection() > 0)
        filter_model = ftk->get_model(filter_ch->GetSelection()-1);
    int func_size = ftk->functions().size();
    if (active_function == -1)
        active_function = func_size - 1;
    else {
        if (active_function >= func_size ||
                ftk->get_function(active_function) != pp_func)
            active_function = ftk->find_function_nr(active_function_name);
        if (active_function == -1 || func_size == old_func_size+1)
            active_function = func_size - 1;
    }
    if (active_function != -1)
        active_function_name = ftk->get_function(active_function)->name;
    else
        active_function_name = "";

    int pos = -1;
    for (int i = 0; i < func_size; ++i) {
        if (filter_model && !contains_element(filter_model->get_ff().idx, i)
                           && !contains_element(filter_model->get_zz().idx, i))
            continue;
        if (i == active_function)
            pos = new_func_col_id.size();
        Function const* f = ftk->get_function(i);
        func_data.push_back(f->name);
        func_data.push_back(f->tp()->name);
        realt a;
        func_data.push_back(f->get_center(&a) ? S(a) : S("-"));
        func_data.push_back(f->get_area(&a)   ? S(a) : S("-"));
        func_data.push_back(f->get_height(&a) ? S(a) : S("-"));
        func_data.push_back(f->get_fwhm(&a)   ? S(a) : S("-"));
        vector<int> const& ffi = model->get_ff().idx;
        vector<int> const& zzi = model->get_zz().idx;
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
        v_foreach (int, i, func_col_id) {
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
    skipOnFuncFocusChanged_ = true;
    f->list->populate(func_data, func_images, pos);
    skipOnFuncFocusChanged_ = false;
}

void SideBar::update_var_list()
{
    vector<string> var_data;
    //  count references first
    vector<Variable*> const& variables = ftk->variables();
    vector<int> var_vrefs(variables.size(), 0), var_frefs(variables.size(), 0);
    v_foreach (Variable*, i, variables) {
        for (int j = 0; j != (*i)->get_vars_count(); ++j)
            var_vrefs[(*i)->get_var_idx(j)]++;
    }
    v_foreach (Function*, i, ftk->functions()) {
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
        var_data.push_back(v->get_formula(ftk->parameters()));  //formula
    }
    v->list->populate(var_data);
}

int SideBar::get_focused_data() const
{
    wxListView *lv = d->list;
    int focused = lv->GetFocusedItem();
    if (focused >= ftk->get_dm_count()) {
        d->update_data_list(false);
        focused = ftk->get_dm_count() - 1;
        lv->Focus(focused);
    }
    else if (focused < 0) {
        focused = 0;
        if (lv->GetItemCount() != 0)
            lv->Focus(focused);
    }
    return focused;
}

int SideBar::get_focused_var() const
{
    if (ftk->variables().empty())
        return -1;
    else {
        int n = v->list->GetFocusedItem();
        return n > 0 ? n : 0;
    }
}

vector<int> SideBar::get_selected_data_indices()
{
    vector<int> sel;
    if (!frame) // app not fully initialized yet
        return sel;
    wxListView *lv = d->list;
    if (ftk->get_dm_count() != lv->GetItemCount())
        d->update_data_list(false);
    for (int i = lv->GetFirstSelected(); i != -1; i = lv->GetNextSelected(i))
        sel.push_back(i);
    if (sel.empty()) {
        int n = get_focused_data();
        if (lv->GetItemCount() > n)
            lv->Select(n, true);
        sel.push_back(n);
    }
    return sel;
}

// in plotting order
vector<int> SideBar::get_ordered_dataset_numbers()
{
    wxListView *lv = d->list;
    if (ftk->get_dm_count() != lv->GetItemCount())
        d->update_data_list(false);
    vector<int> ordered;
    int count = lv->GetItemCount();
    if (count == 0)
        return ordered;
    vector<int> selected;
    int focused = lv->GetFocusedItem();
    // focused but not selected is not important
    if (focused >= 0 && !lv->IsSelected(focused))
        focused = -1;
    for (int i = 0; i < count; ++i) {
        if (lv->IsSelected(i)) {
            if (i != focused)
                selected.push_back(i);
        }
        else
            ordered.push_back(i);
    }
    ordered.insert(ordered.end(), selected.begin(), selected.end());
    if (focused >= 0)
        ordered.push_back(focused);
    assert (size(ordered) == count);
    return ordered;
}

string SideBar::get_sel_datasets_as_string()
{
    if (ftk->get_dm_count() == 1)
        return "";
    if (data_look->GetSelection() == 0) // all datasets
        return " @*";
    string s;
    for (int i = d->list->GetFirstSelected(); i != -1;
                                             i = d->list->GetNextSelected(i))
        s += " @" + S(i);
    if (s.empty())
        s = " @" + S(get_focused_data());
    return s;
}

//bool SideBar::is_func_selected(int n) const
//{
//    return f->list->IsSelected(n) || f->list->GetFocusedItem() == n;
//}
//

void SideBar::activate_function(int n)
{
    active_function = n;
    do_activate_function();

    int pos;
    if (filter_ch->GetSelection() > 0)
        pos = f->list->FindItem(-1, s2wx(active_function_name));
    else if (n < f->list->GetItemCount())
        pos = n;
    else
        pos = -1;

    if (pos != -1) {
        skipOnFuncFocusChanged_ = true;
        f->list->Focus(pos);
        skipOnFuncFocusChanged_ = false;
        for (int i = 0; i != f->list->GetItemCount(); ++i)
            f->list->Select(i, i == pos);
    }
}

void SideBar::do_activate_function()
{
    if (active_function != -1)
        active_function_name = ftk->get_function(active_function)->name;
    else
        active_function_name = "";
    frame->plot_pane()->refresh_plots(false, kMainPlot);
    update_func_inf();
    update_param_panel();
}

void SideBar::select_datasets(const vector<int>& datasets)
{
    if (datasets.empty())
        return;
    wxListView *lv = d->list;
    int n = lv->GetItemCount();
    for (int i = 0; i != n; ++i)
        lv->Select(i, contains_element(datasets, i));
    if (datasets[0] < n)
        lv->Focus(datasets[0]);
}

// Focus is _not_ used for data-related operations, only for function-related
// ones. Sum and functions are shown only for focused dataset.
void SideBar::OnDataFocusChanged(wxListEvent &)
{
    int n = d->list->GetFocusedItem();
    //ftk->msg("[F] id:" + S(event.GetIndex()) + " focused:"
    //         + S(d->list->GetFocusedItem()));
    if (n < 0)
        return;
    int length = ftk->get_dm_count();
    if (length > 1)
        frame->plot_pane()->refresh_plots(false, kAllPlots);
    update_data_inf();
}

void SideBar::OnDataSelectionChanged(wxListEvent &/*event*/)
{
    if (data_look->GetSelection() != 0) // ! all datasets
        frame->plot_pane()->refresh_plots(false, kAllPlots);
    //bool selected= (event.GetEventType() == wxEVT_COMMAND_LIST_ITEM_SELECTED);
    //assert (selected
    //        || event.GetEventType() == wxEVT_COMMAND_LIST_ITEM_DESELECTED);
    //long index = event.GetIndex();
    //ftk->msg("id:" + S(index) + " e.sel:" + S(selected)
    //         + " issel:" + S(d->list->IsSelected(index))
    //         + " foc:" + S(d->list->GetFocusedItem()));
    update_data_buttons();
}

void SideBar::update_data_buttons()
{
    bool not_the_last = get_focused_data()+1 < ftk->get_dm_count();
    int sel_d = d->list->GetSelectedItemCount();
    data_page->FindWindow(ID_DP_REN)->Enable(sel_d == 1);
    //data_page->FindWindow(ID_DP_DEL)->Enable(sel_d > 0);
    //data_page->FindWindow(ID_DP_COL)->Enable(sel_d > 0);
    data_page->FindWindow(ID_DP_CPF)->Enable(sel_d == 1 && not_the_last);
}

void SideBar::update_func_buttons()
{
    int sel_f = f->list->GetSelectedItemCount();
    func_page->FindWindow(ID_FP_DEL)->Enable(sel_f > 0);
    func_page->FindWindow(ID_FP_EDIT)->Enable(sel_f == 1);
    //func_page->FindWindow(ID_FP_CHTYPE)->Enable(sel_f > 0);
    func_page->FindWindow(ID_FP_COL)->Enable(sel_f > 0);

    bool has_hwhm = false, has_shape = false;
    v_foreach (Function*, i, ftk->functions()) {
        if (contains_element((*i)->tp()->fargs, "hwhm"))
            has_hwhm = true;
        if (contains_element((*i)->tp()->fargs, "shape"))
            has_shape = true;
    }
    func_page->FindWindow(ID_FP_HWHM)->Enable(has_hwhm);
    func_page->FindWindow(ID_FP_SHAPE)->Enable(has_shape);
}

void SideBar::update_var_buttons()
{
    int sel_v = v->list->GetSelectedItemCount();
    var_page->FindWindow(ID_VP_DEL)->Enable(sel_v > 0);
    var_page->FindWindow(ID_VP_EDIT)->Enable(sel_v == 1);
}

string SideBar::get_datasets_for_plot()
{
    int first = frame->get_focused_data_index();
    string s = "@" + S(first);
    if (data_look->GetSelection() == 0) // all datasets
        return s + ", @*";
    else { // only selected datasets
        for (int i = d->list->GetFirstSelected(); i != -1;
                                              i = d->list->GetNextSelected(i))
            if (i != first)
                s += ", @" + S(i);
        return s;
    }
}

void SideBar::update_data_inf()
{
    int n = get_focused_data();
    if (n < 0)
        return;
    wxTextCtrl* inf = d->inf;
    inf->Clear();
    inf->AppendText(wxString::Format(wxT("@%d: "), n));
    inf->AppendText(s2wx(ftk->get_data(n)->get_info()));
    wxFileName fn(s2wx(ftk->get_data(n)->get_filename()));
    if (fn.IsOk() && !fn.IsAbsolute()) {
        fn.MakeAbsolute();
        inf->AppendText(wxT("\nPath: ") + fn.GetFullPath());
    }
    inf->ShowPosition(0);

    // update also data filename in app window title
    frame->update_app_title();
}

void SideBar::update_func_inf()
{
    wxTextCtrl* inf = f->inf;
    inf->Clear();
    if (active_function < 0)
        return;
    Function const* func = ftk->get_function(active_function);
    realt a;
    if (func->get_center(&a))
        inf->AppendText(wxT("Center: ")
                   + s2wx(format1<realt, 30>("%.10"REALT_LENGTH_MOD"g", a)));
    if (func->get_area(&a))
        inf->AppendText(wxT("\nArea: ") + s2wx(S(a)));
    if (func->get_height(&a))
        inf->AppendText(wxT("\nHeight: ") + s2wx(S(a)));
    if (func->get_fwhm(&a))
        inf->AppendText(wxT("\nFWHM: ") + s2wx(S(a)));
    if (func->get_iwidth(&a))
        inf->AppendText(wxT("\nInt. Width: ") + s2wx(S(a)));
    v_foreach (string, i, func->get_other_prop_names())
        inf->AppendText(s2wx("\n" + *i + ": " + S(func->get_other_prop(*i))));

    vector<string> in;
    for (int i = 0; i < ftk->get_dm_count(); ++i) {
        if (contains_element(ftk->get_model(i)->get_ff().idx, active_function))
            in.push_back("@" + S(i) + ".F");
        if (contains_element(ftk->get_model(i)->get_zz().idx, active_function))
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
    Variable const* var = ftk->get_variable(n);
    string t = "$" + var->name + " = " + var->get_formula(ftk->parameters());
    inf->AppendText(s2wx(t));
    vector<string> in = ftk->get_variable_references(var->name);
    if (!in.empty())
        inf->AppendText(s2wx("\nIn: " + join_vector(in, ", ")));
    inf->ShowPosition(0);
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
        dd.push_back(wx2s(f->list->GetItemText(i)));
    //if (dd.empty() && f->list->GetItemCount() > 0) {
    //    int n = f->list->GetFocusedItem();
    //    dd.push_back(ftk->get_function(n == -1 ? 0 : n)->xname);
    //}
    return dd;
}

vector<string> SideBar::get_selected_vars() const
{
    vector<string> dd;
    for (int i = v->list->GetFirstSelected(); i != -1;
                                             i = v->list->GetNextSelected(i))
        dd.push_back(ftk->get_variable(i)->name);
    return dd;
}


void SideBar::OnFuncFocusChanged(wxListEvent&)
{
    if (skipOnFuncFocusChanged_)
        return;
    int n = f->list->GetFocusedItem();
    if (n == -1)
        active_function = -1;
    else {
        string name = wx2s(f->list->GetItemText(n));
        active_function = ftk->find_function_nr(name);
    }
    do_activate_function();
}

void SideBar::OnVarFocusChanged(wxListEvent&)
{
    update_var_inf();
}

bool SideBar::find_value_of_param(string const& p, double* value)
{
    if (active_function != -1) {
        Function const* f = ftk->get_function(active_function);
        int idx = index_of_element(f->tp()->fargs, p);
        if (idx != -1) {
            *value = f->av()[idx];
            return true;
        }
    }

    v_foreach (Function*, i, ftk->functions()) {
        int idx = index_of_element((*i)->tp()->fargs, p);
        if (idx != -1) {
            *value = (*i)->av()[idx];
            return true;
        }
    }
    return false;
}

void SideBar::make_same_func_par(string const& p, bool checked)
{
    string varname = "_" + p;
    string val_str;
    if (checked) {
        double value = 0.;
        bool found = find_value_of_param(p, &value);
        if (!found)
            return;

        ftk->exec("$" + varname + " = ~" + S(value));
        val_str = "$" + varname;
    }
    else {
        int nr = ftk->find_variable_nr(varname);
        if (nr == -1)
            return;
        val_str = ftk->get_variable(nr)->get_formula(ftk->parameters());
        // varname (_hwhm or _shape) will be auto-deleted
    }
    string cmd;
    for (int i = 0; i < ftk->get_dm_count(); ++i)
        if (ftk->get_model(i)->get_ff().names.size() > 0) {
            if (!cmd.empty())
                cmd += "; ";
            cmd += "@" + S(i) + ".F[*]." + p + " = " + val_str;
        }
    ftk->exec(cmd);
}

void SideBar::on_parameter_changing(const std::vector<realt>& values)
{
    frame->get_main_plot()->draw_overlay_func(pp_func, values);
}

void SideBar::on_parameter_changed(int n)
{
    string vname = wx2s(param_panel->get_label2(n));
    ftk->exec(vname + " = ~" + eS(param_panel->get_value(n)));
}

void SideBar::on_parameter_lock_clicked(int n, int state)
{
    string vname = wx2s(param_panel->get_label2(n));
    if (state == 0)
        ftk->exec(vname + " = ~{" + vname + "}");
    else if (state == 1)
        ftk->exec(vname + " = {" + vname + "}");
    else { // state == 2
        nb->SetSelection(2); // "variables" page
        wxString vname_no_prefix = s2wx(vname.substr(1));
        for (int i = 0; i != v->list->GetItemCount(); ++i) {
            if (v->list->GetItemText(i) == vname_no_prefix) {
                v->list->Select(i, true);
                v->list->EnsureVisible(i);
                v->list->Focus(i);
            }
            else
                v->list->Select(i, false);
        }
    }
}

void SideBar::change_parameter_value(int idx, double value)
{
    if (idx < param_panel->get_count())
        param_panel->set_value(idx, value);
}

void SideBar::update_param_panel()
{
    int old_count = param_panel->get_count();
    if (active_function < 0) {
        param_panel->delete_row_range(0, old_count);
        pp_func = NULL;
        return;
    }

    pp_func = ftk->get_function(active_function);

    wxString new_label = s2wx("%" + pp_func->name +" : "+ pp_func->tp()->name);
    if (param_panel->get_title() != new_label)
        param_panel->set_title(new_label);

    int new_count = min(pp_func->nv(), 8);
    if (new_count < old_count)
        param_panel->delete_row_range(new_count, old_count);

    for (int i = 0; i < new_count; ++i) {
        Variable const* var = ftk->get_variable(pp_func->get_var_idx(i));
        wxString label = s2wx(pp_func->get_param(i));
        if (var->is_simple() || var->is_constant()) {
            bool locked = var->is_constant();
            param_panel->set_normal_parameter(i, label, var->get_value(),
                                              locked, s2wx("$"+var->name));
        }
        else
            param_panel->set_disabled_parameter(i, label, var->get_value(),
                                                s2wx("$"+var->name));
    }

    // Layout() is needed only when the layout has changed (e.g. when label2
    // is shown for the first time). We call it always, just in case.
    param_panel->Layout();
    if (new_count != old_count) {
        int sash_pos = GetClientSize().GetHeight() - 3
                         - param_panel->GetSizer()->GetMinSize().GetHeight();
        if (sash_pos < GetSashPosition())
            SetSashPosition(max(50, sash_pos));
    }
}



