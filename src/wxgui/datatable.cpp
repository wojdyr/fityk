// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

#include "datatable.h"

#include <wx/wx.h>
#include <wx/grid.h>

#include "frame.h"
#include "../data.h"
#include "../logic.h"

using namespace std;

class BetterGridCellFloatEditor : public wxGridCellFloatEditor
{
    void BeginEdit(int row, int col, wxGrid* grid)
    {
        // printf("screenposition1: %d\n", Text()->GetScreenPosition().y);
        wxGridCellTextEditor::BeginEdit(row, col, grid);
        // there is a bug in wxGTK: when edition is started, the previously
        // edited cell is made visible
    }
};

// helper needed by wxGrid
class GridTable: public wxGridTableBase
{
public:
    GridTable(int data_nr, Data const* data)
        : wxGridTableBase(), data_nr_(data_nr), data_(data),
          dataset_str_(" in @" + S(data_nr_)),
          instant_update_(true)
    {
        has_last_num[0] = has_last_num[1] = has_last_num[2] = false;
    }

    virtual int GetNumberRows() { return get_points().size() + 1; }
    virtual int GetNumberCols() { return 4; }

    // this function is used e.g. for control of text overflow,
    // returning always false should be fine
    virtual bool IsEmptyCell(int /*row*/, int /*col*/) { return false; }

    virtual wxString GetValue(int row, int col)
    {
        if (col == 0)
            return GetValueAsBool(row, col) ? wxT("1") : wxT("0");
        if (row == GetNumberRows() - 1 && !has_last_num[col-1])
            return wxEmptyString;
        return wxString::Format((col == 3 ? wxT("%g") : wxT("%.12g")),
                                GetValueAsDouble(row, col));
    }

    // this function is pure virtual in the base class, but is never used
    virtual void SetValue(int, int, const wxString&) { assert(0); }

    virtual wxString GetTypeName(int /*row*/, int col)
    {
        return col == 0 ? wxGRID_VALUE_BOOL : wxGRID_VALUE_FLOAT;
    }

    virtual bool CanGetValueAs(int /*row*/, int col, const wxString& typeName)
    {
        return (col == 0 && typeName == wxGRID_VALUE_BOOL) ||
               (col > 0 && typeName == wxGRID_VALUE_STRING);
    }

    virtual bool CanSetValueAs(int row, int col, const wxString& typeName)
    {
        return typeName == GetTypeName(row, col);
    }

    virtual double GetValueAsDouble(int row, int col)
    {
        const Point &p = get_point(row);
        switch (col) {
            case 1: return p.x;
            case 2: return p.y;
            case 3: return p.sigma;
            default: assert(0); return 0.;
        }
    }

    virtual bool GetValueAsBool(int row, int col)
    {
        assert(col == 0);
        return get_point(row).is_active;
    }

    virtual void SetValueAsDouble(int row, int col, double value)
    {
        if (row == GetNumberRows() - 1) {
            update_point(last_point_, col, value);
            has_last_num[col-1] = true;
            if (has_last_num[0] && has_last_num[1]) {
              char buffer[128];
              sprintf(buffer,
                      "M=M+1, X[%d]=%.12g, Y[%d]=%.12g, S[%d]=%.12g, A[%d]=%d",
                      row, last_point_.x, row, last_point_.y,
                      row, last_point_.sigma, row, last_point_.is_active);
              change_value(buffer);
              if (!instant_update_)
                  local_points_.push_back(last_point_);
              last_point_ = Point();
              has_last_num[0] = has_last_num[1] = has_last_num[2] = false;
              wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1);
              GetView()->ProcessTableMessage(msg);
            }
        }
        else {
            if (value == GetValueAsDouble(row, col))
                return;
            char buffer[32];
            switch (col) {
                case 1: buffer[0] = 'X';  break;
                case 2: buffer[0] = 'Y';  break;
                case 3: buffer[0] = 'S';  break;
                default: assert(0);
            }
            sprintf(buffer+1, "[%d]=%.12g", row, value);
            change_value(buffer);
            if (!instant_update_)
                update_point(local_points_[row], col, value);
        }
    }

    virtual void SetValueAsBool(int row, int col, bool value)
    {
        assert(col == 0);
        if (row == GetNumberRows() - 1) {
            last_point_.is_active = value;
            return;
        }
        char buffer[32];
        sprintf(buffer, "A[%d]=%s", row, (value ? "true" : "false"));
        change_value(buffer);
        if (!instant_update_)
            local_points_[row].is_active = value;
    }

    virtual wxString GetRowLabelValue(int row)
    {
        return wxString::Format(wxT("%i"), row);
    }

    virtual wxString GetColLabelValue(int col)
    {
        switch (col) {
            case 0: return wxT(" ");
            case 1: return wxT("x");
            case 2: return wxT("y");
            case 3: return wxT("\u03C3"); // sigma
            default: assert(0); return wxEmptyString;
        }
    }

    vector<Point> const& get_points()
    {
        return instant_update_ ? data_->points() : local_points_;
    }

    Point const& get_point(int row)
    {
        vector<Point> const& points = get_points();
        if (row == (int) points.size())
            return last_point_;
        assert(row < (int) points.size());
        return points[row];
    }

    void update_point(Point& point, int column, double value)
    {
        switch (column) {
            case 1: point.x = value;  break;
            case 2: point.y = value;  break;
            case 3: point.sigma = value;  break;
        }
    }

    void change_value(const char* buffer)
    {
        if (instant_update_)
            ftk->exec(buffer + dataset_str_);
        else {
            if (!pending_cmd_.empty())
                pending_cmd_ += ", ";
            pending_cmd_ += buffer;
        }
    }

    void apply_pending_cmd()
    {
        if (pending_cmd_.empty())
            return;
        ftk->exec(pending_cmd_ + dataset_str_);
        pending_cmd_.clear();
        // when Apply is pressed, the data may need to be sorted
        if (!instant_update_)
            local_points_ = data_->points();
    }

    void set_instant_update(bool instant)
    {
        if (instant)
            apply_pending_cmd();
        else
            local_points_ = data_->points();
        instant_update_ = instant;
    }

private:
    int data_nr_;
    Data const* data_;
    string dataset_str_;
    bool instant_update_;
    string pending_cmd_;
    vector<Point> local_points_;
    Point last_point_;
    bool has_last_num[3];
};


DataTableDlg::DataTableDlg(wxWindow* parent, wxWindowID id,
                           int data_nr, Data* data)
    : wxDialog(parent, id,
               wxString::Format(wxT("@%d "), data_nr) + s2wx(data->get_title()),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    grid = new wxGrid(this, -1, wxDefaultPosition, wxSize(-1, 400));
    grid_table = new GridTable(data_nr, data);
    grid->SetTable(grid_table, true, wxGrid::wxGridSelectRows);
    grid->SetEditable(true);
    grid->SetColumnWidth(0, 40);
    grid->SetRowLabelSize(60);
    // the default render uses %f format, we prefer %g
    grid->RegisterDataType(wxGRID_VALUE_FLOAT, new wxGridCellStringRenderer,
                                               new BetterGridCellFloatEditor);
    sizer->Add(grid, wxSizerFlags(1).Expand());
    cb = new wxCheckBox(this, -1, wxT("update and sort instantly"));
    sizer->Add(cb, wxSizerFlags().Border());
    add_apply_close_buttons(this, sizer);
    SetEscapeId(wxID_CLOSE);
    SetSizerAndFit(sizer);
    // apparently the initial width of wxGrid leaves no space for scrollbar
    SetClientSize(GetClientSize() + wxSize(20, 0));

    cb->SetValue(true);
    FindWindow(wxID_APPLY)->Enable(false);

    Connect(cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(DataTableDlg::OnUpdateCheckBox));
    Connect(wxID_APPLY, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(DataTableDlg::OnApply));
#if !wxCHECK_VERSION(2, 9, 0)
#  define wxEVT_GRID_CELL_CHANGED wxEVT_GRID_CELL_CHANGE
#endif
    Connect(grid->GetId(), wxEVT_GRID_CELL_CHANGED,
            wxGridEventHandler(DataTableDlg::OnCellChanged));
}

void DataTableDlg::OnApply(wxCommandEvent&)
{
    grid_table->apply_pending_cmd();
    grid->ForceRefresh();
}

void DataTableDlg::OnUpdateCheckBox(wxCommandEvent& event)
{
    bool checked = event.IsChecked();
    grid_table->set_instant_update(checked);
    FindWindow(wxID_APPLY)->Enable(!checked);
}

void DataTableDlg::OnCellChanged(wxGridEvent& event)
{
    if (event.GetCol() == 1 && cb->GetValue()) // order of items can be changed
        grid->ForceRefresh();
}

