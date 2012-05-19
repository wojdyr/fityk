// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
/// DataTableDlg: Data > Table dialog

#ifndef FITYK_WX_DATATABLE_H_
#define FITYK_WX_DATATABLE_H_

#include <wx/dialog.h>

namespace fityk { class Data; }
class GridTable;
class wxGrid;
class wxGridEvent;
class wxCheckBox;;

class DataTableDlg : public wxDialog
{
    friend class GridTable;
public:
    DataTableDlg(wxWindow* parent, wxWindowID id, int data_nr,
                 fityk::Data* data);

private:
    GridTable *grid_table;
    wxGrid* grid;
    wxCheckBox *cb;
    void OnApply(wxCommandEvent& event);
    void OnUpdateCheckBox(wxCommandEvent& event);
    void OnCellChanged(wxGridEvent& event);
    void OnCellRightClick(wxGridEvent& event);
    void OnCopy(wxCommandEvent&) { OnCopy(); }
    void OnCopy();
    void OnKeyDown(wxKeyEvent& event);
};

#endif // FITYK_WX_DATATABLE_H_
