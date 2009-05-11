// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

#ifndef FITYK__WX_DEFMGR__H__
#define FITYK__WX_DEFMGR__H__

#include <string>
#include <vector>
#include <wx/grid.h>
//#include "wx_common.h"


class DefinitionMgrDlg : public wxDialog
{
public:
    struct FunctionDefinitonElems
    {
        std::string name;
        std::vector<std::string> parameters;
        std::vector<std::string> defvalues;
        std::string rhs;
        int builtin;

        std::string get_full_definition() const;
    };

    DefinitionMgrDlg(wxWindow* parent);
    void OnFunctionChanged(wxCommandEvent &) { select_function(); }
    void OnEndCellEdit(wxGridEvent &event);
    void OnNameChanged(wxCommandEvent &);
    void OnDefChanged(wxCommandEvent &);
    void OnAddButton(wxCommandEvent &);
    void OnRemoveButton(wxCommandEvent &);
    void OnOk(wxCommandEvent &event);
    std::string get_command();

private:
    int selected;
    wxListBox *lb;
    wxTextCtrl *name_tc, *def_tc;
    wxStaticText *name_comment_st, *guess_label_st, *def_label_st;
    wxGrid *par_g;
    wxButton *add_btn, *remove_btn;
    std::vector<FunctionDefinitonElems> orig, modified;

    void select_function(bool init=false);
    void fill_function_list();
    bool check_definition();
    void update_guess_comment();
    bool is_name_in_modified(std::string const& name);
    bool save_changes();

    DECLARE_EVENT_TABLE()
};

#endif
