// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__WX_DEFMGR__H__
#define FITYK__WX_DEFMGR__H__

#include <string>
#include <vector>
#include "../func.h"
#include "../cparser.h"


class DefinitionMgrDlg : public wxDialog
{
public:
    DefinitionMgrDlg(wxWindow* parent);
    void OnFunctionChanged(wxCommandEvent &) { select_function(); }
    void OnDefChanged(wxCommandEvent &) { parse_definition(); }
    void OnAddButton(wxCommandEvent &);
    void OnRemoveButton(wxCommandEvent &);
    void OnOk(wxCommandEvent &event);
    std::vector<std::string> get_commands();

private:
    int selected_;
    std::vector<Tplate> modified_;
    Parser parser_;
    wxListBox *lb;
    wxTextCtrl *def_tc, *desc_tc;
    wxStaticText *def_label_st;
    wxButton *remove_btn, *ok_btn;

    void select_function();
    void parse_definition();
    void update_desc(const Tplate& tp);
};

#endif
