// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id:  $
//
// DataTableDlg: Data > Table dialog

#ifndef FITYK_WX_DATATABLE_H_
#define FITYK_WX_DATATABLE_H_

#include <wx/dialog.h>

class Data;
class GridTable;
class wxGrid;
class wxGridEvent;
class wxCheckBox;;

class DataTableDlg : public wxDialog
{
    friend class GridTable;
public:
    DataTableDlg(wxWindow* parent, wxWindowID id, int data_nr, Data* data);

private:
    GridTable *grid_table;
    wxGrid* grid;
    wxCheckBox *cb;
    void OnApply(wxCommandEvent& event);
    void OnUpdateCheckBox(wxCommandEvent& event);
    void OnCellChanged(wxGridEvent& event);
};

#endif // FITYK_WX_DATATABLE_H_
