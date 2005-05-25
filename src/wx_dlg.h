// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef WX_DLG__H__
#define WX_DLG__H__

#include <wx/treectrl.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/listctrl.h> 
#include "pag.h"

class FuncTree;
struct z_names_type;
class wxGrid;

struct par_descr_type
{
    wxRadioButton *radio;
    std::string name;
    char option;
    std::string from_tree, new_value;
};

class FuncBrowserDlg : public wxDialog
{
public:
    FuncBrowserDlg (wxWindow* parent, wxWindowID id, int tab);
    void OnSelChanged           (wxTreeEvent& event);
    void OnAddWhatChoice (wxCommandEvent& WXUNUSED(event)) 
                                                    {set_list_of_fzg_types();}
    void OnAddTypeChoice (wxCommandEvent& WXUNUSED(event)) {type_was_chosen();}
    void OnParNumChosen (wxCommandEvent& WXUNUSED(event)) 
                                            { parameter_number_was_chosen(); }
    void OnAddTNCRadio       (wxCommandEvent& event); 
    void OnAddValText        (wxCommandEvent& event);
    void OnAddAddButton      (wxCommandEvent& event);
    void OnChangeButton      (wxCommandEvent& event);
    void OnArrowButton       (wxCommandEvent& event);
    void OnSetDomCheckBox    (wxCommandEvent& event);
    void OnSetDomCtrCheckBox (wxCommandEvent& event);
    void OnDeleteButton      (wxCommandEvent& event);
    void OnRRCheckBox        (wxCommandEvent& event);
    void OnFreezeButton      (wxCommandEvent& event);
    void OnFreezeAllButton   (wxCommandEvent& event);
    void OnValueButton       (wxCommandEvent& event);
    void show_expanded (int item, int subitem=-1);
protected:
    FuncTree *func_tree;
    wxTextCtrl *info_text, *frozen_tc, *value_at_tc, *value_output_tc;
    wxStaticText *ch_label, *del_label, *ch_dom_label[4], *value_label;
    wxButton *del_button, *freeze_button;
    wxPanel *p_change, *p_add;
    wxListCtrl *slc;
    wxCheckBox *dom_set_cb, *dom_ctr_set_cb;
    std::string sel_fun;
    wxChoice *fzg_choice, *type_choice;
    wxTextCtrl *add_preview_tc, *add_p_val_tc;
    wxTextCtrl *ch_edit, *ch_ctr, *ch_sigma, *ch_left_b, *ch_right_b, 
               *ch_def_dom_w;
    wxRadioButton *tpc_rb[3], *ch_dom_rb[2];
    wxStaticBox *add_box;
    wxButton *add_add_button;
    wxSizer *ah3_sizer;
    char add_what;
    std::vector<par_descr_type> par_descr;
    int current_add_p_number;
    std::vector<const z_names_type*> all_t;
    bool initialized;

    void set_list_of_fzg_types();
    void type_was_chosen();
    void parameter_number_was_chosen();
    void change_tpc_radio (int nradio);
    void update_add_preview();

    void update_frozen_tc();
    void update_freeze_button_label();
    void set_change_initials(int n);
    void change_domain_enable();
    DECLARE_EVENT_TABLE()
};

class FuncTree : public wxTreeCtrl
{
public:
    FuncTree (wxWindow *parent, const wxWindowID id);
    void OnRightDown          (wxMouseEvent&   event);
    void OnPopupExpandAll     (wxCommandEvent& event);
    void OnPopupCollapseAll   (wxCommandEvent& event);
    void OnPopupToggleButton  (wxCommandEvent& event);
    void OnPopupReset         (wxCommandEvent& event);
    int update_labels (const std::string& beginning);
    void reset_funcs_in_root();
    void ExpandAll (const wxTreeItemId& item);
protected:
    std::vector<std::vector<wxTreeItemId> > a_ids, g_ids, f_ids, z_ids;
    void add_pags_to_tree (wxTreeItemId item_id, const std::vector<Pag>& pags);
    void add_fzg_to_tree (wxTreeItemId p_id, One_of_fzg fzg, int n);
    wxTreeItemId next_item (const wxTreeItemId& item);
    wxTreeItemId next_item_but_not_child (const wxTreeItemId& item);
    DECLARE_EVENT_TABLE()
};

class FDXLoadDlg : public wxDialog
{
public:
    std::string filename;
    FDXLoadDlg (wxWindow* parent, wxWindowID id);
    std::string get_command();
    void OnChangeButton (wxCommandEvent& event);
    void set_filename (const std::string &path);

protected:
    wxTextCtrl *file_txt_ctrl;
    wxSpinCtrl *x_column, *y_column, *s_column, 
               *from_range, *to_range, *from_every, 
               *to_every, *of_every, *merge_number; 
    wxPanel *columns_panel, *other_types_panel;
    wxRadioBox *rb_filetype, *yrbox;
    wxCheckBox *merge_cb, *std_dev_cb;
    wxListBox *lb_filetypes;
    void OnFTypeRadioBoxSelection (wxCommandEvent& event);
    void OnSelRadioBoxSelection (wxCommandEvent& event);
    void OnMergeCheckBox (wxCommandEvent& event);
    void OnStdDevCheckBox (wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
};


class SumHistoryDlg : public wxDialog
{
public:
    SumHistoryDlg (wxWindow* parent, wxWindowID id);
    void OnUpButton           (wxCommandEvent& event);
    void OnDownButton         (wxCommandEvent& event);
    void OnToggleSavedButton  (wxCommandEvent& event);
    void OnComputeWssrButton  (wxCommandEvent& event);
    void OnSelectedItem       (wxListEvent&    event);
    void OnActivatedItem      (wxListEvent&    event); 
    void OnViewSpinCtrlUpdate (wxSpinEvent&    event); 
protected:
    int view[3], view_max;
    wxListCtrl *lc;
    wxBitmapButton *up_arrow, *down_arrow;
    wxButton *compute_wssr_button;

    void initialize_lc();
    void update_selection();
    DECLARE_EVENT_TABLE()
};


class DataEditorDlg : public wxDialog
{
public:
    DataEditorDlg (wxWindow* parent, wxWindowID id, Data *data_);
    void OnRevert (wxCommandEvent& event);
    void update_data(Data *data_);
protected:
    wxGrid *grid;
    Data *data;
    wxStaticText *filename_label, *title_label, *description;
    wxTextCtrl *code;
    wxButton *apply_button;
    DECLARE_EVENT_TABLE()
};

#endif

