// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_SIDEBAR__H__
#define FITYK__WX_SIDEBAR__H__

#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/config.h>
#include <vector>
#include <string>

#include "cmn.h" //ProportionalSplitter

class GradientDlg;
class FancyRealCtrl;
class ListPlusText;
class Function;
class Variable;


class SideBar : public ProportionalSplitter
{
public:
    SideBar(wxWindow *parent, wxWindowID id=-1);
    void OnDataButtonNew (wxCommandEvent& event);
    void OnDataButtonDup (wxCommandEvent& event);
    void OnDataButtonRen (wxCommandEvent& event);
    void OnDataButtonDel (wxCommandEvent&) { delete_selected_items(); }
    void OnDataButtonCopyF (wxCommandEvent& event);
    void OnDataButtonCol (wxCommandEvent& event);
    void OnDataColorsChanged(GradientDlg *gd);
    void OnDataLookChanged (wxCommandEvent& event);
    void OnDataPSizeChanged (wxSpinEvent& event);
    void OnDataPLineChanged (wxCommandEvent& event);
    void OnDataShiftUpChanged (wxSpinEvent& event);
    void OnFuncButtonNew (wxCommandEvent& event);
    void OnFuncButtonDel (wxCommandEvent&) { delete_selected_items(); }
    void OnFuncButtonEdit (wxCommandEvent& event);
    void OnFuncButtonChType (wxCommandEvent& event);
    void OnFuncButtonCol (wxCommandEvent& event);
    void OnVarButtonNew (wxCommandEvent& event);
    void OnVarButtonDel (wxCommandEvent&) { delete_selected_items(); }
    void OnVarButtonEdit (wxCommandEvent& event);
    void OnFuncFilterChanged (wxCommandEvent& event);
    void OnDataFocusChanged(wxListEvent &) { DataFocusChanged(); }
    void DataFocusChanged();
    void OnFuncFocusChanged(wxListEvent &event);
    void OnVarFocusChanged(wxListEvent &event);
    void read_settings(wxConfigBase *cf);
    void save_settings(wxConfigBase *cf) const;
    void update_lists(bool nondata_changed=true);
    /// get active dataset number -- if none is focused, return first one (0)
    int get_focused_data() const;
    int get_active_function() const { return active_function; }
    int get_focused_var() const;
    bool is_func_selected(int n) const;
    int set_selection(int page) { return nb->SetSelection(page); }
    void activate_function(int n);
    std::vector<std::string> get_selected_data() const;
    bool howto_plot_dataset(int n, bool& shadowed, int& offset) const;
    std::vector<std::string> get_selected_func() const;
    std::vector<std::string> get_selected_vars() const;
    void update_data_inf();
    void update_func_inf();
    void update_var_inf();
    void update_bottom_panel();
    void delete_selected_items();
    void draw_function_draft(FancyRealCtrl const* frc) const;
    void change_bp_parameter_value(int idx, double value);
private:
    wxNotebook *nb;
    wxPanel *data_page, *func_page, *var_page, *bottom_panel;
    wxFlexGridSizer* bp_sizer;
    wxStaticText *bp_label;
    std::vector<FancyRealCtrl*> bp_frc;
    std::vector<wxStaticText*> bp_statict;
    std::vector<bool> bp_sig; /// bottom panel "signature" (widget order)
    Function const* bp_func; ///bottom panel function
    ListPlusText *d, *f, *v;
    wxChoice *data_look, *filter_ch;
    wxSpinCtrl *shiftup_sc, *dpsize_sc;
    wxCheckBox *dpline_cb;
    int active_function;
    std::string active_function_name;

    void update_data_list(bool nondata_changed);
    void update_func_list(bool nondata_changed);
    void update_var_list();
    void add_variable_to_bottom_panel(Variable const* var, 
                                      std::string const& tv_name);
    void clear_bottom_panel();
    std::vector<bool> make_bottom_panel_sig(Function const* func);
    void do_activate_function();

    DECLARE_EVENT_TABLE()
};

#endif
