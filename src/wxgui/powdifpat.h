// Author: Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $
//
// A tool for creating models for powder diffraction patterns (Pawley method)

#ifndef FITYK_WX_POWDIFPAT_H_
#define FITYK_WX_POWDIFPAT_H_

#include <map>
#include <vector>
#include <string>
#include <wx/listbook.h>

#include "ceria.h"

#if STANDALONE_POWDIFPAT
#  include <wx/filepicker.h>
#  include <xylib/xylib.h>
#else
   class Data;
#endif

class LockableRealCtrl;
class PhasePanel;
class RealNumberCtrl;


class PowderBook : public wxListbook
{
    friend class PlotWithLines;
#if STANDALONE_POWDIFPAT
    friend class App;
#endif
public:
    static const int max_wavelengths = 5;
    std::map<std::string, CelFile> quick_phase_list;

    PowderBook(wxWindow* parent, wxWindowID id);
    void OnAnodeSelected(wxCommandEvent& event);
    void OnQuickPhaseSelected(wxCommandEvent& event);
    void OnQuickListRemove(wxCommandEvent&);
    void OnQuickListImport(wxCommandEvent&);
    void OnLambdaChange(wxCommandEvent& event);
    void OnPageChanged(wxListbookEvent& event);
    void OnPeakRadio(wxCommandEvent& event);
    void OnWidthRadio(wxCommandEvent& event);
    void OnShapeRadio(wxCommandEvent& event);
    PhasePanel *get_phase_panel(int n);
    PhasePanel *get_current_phase_panel();
    void deselect_phase_quick_list();
    double get_lambda(int n) const;
    wxListBox *get_quick_phase_lb() { return quick_phase_lb; }
    double get_x_min() const { return x_min; }
    double get_x_max() const { return x_max; }
#if STANDALONE_POWDIFPAT
    void OnFilePicked(wxFileDirPickerEvent& event);
    void set_file(wxString const& path);
#endif

private:
    wxListBox *anode_lb, *quick_phase_lb;
    std::vector<LockableRealCtrl*> lambda_ctrl, intensity_ctrl, corr_ctrl;
    wxNotebook *sample_nb;
    double x_min, x_max, y_max;
#if STANDALONE_POWDIFPAT
    wxFilePickerCtrl *file_picker;
    RealNumberCtrl *range_from, *range_to;
    const xylib::DataSet* data;
#else
    const Data* data;
#endif
    wxRadioBox *peak_rb, *width_rb, *shape_rb;
    wxTextCtrl *peak_txt;
    LockableRealCtrl *par_u, *par_v, *par_w, *par_z, *par_a, *par_b, *par_c;

    wxPanel* PrepareIntroPanel();
    wxPanel* PrepareInstrumentPanel();
    wxPanel* PrepareSamplePanel();
    wxPanel* PreparePeakPanel();
    wxPanel* PrepareActionPanel();
    wxPanel* PrepareSizeStrainPanel();
    void initialize_quick_phase_list();
    void update_peak_parameters();
};

#if !STANDALONE_POWDIFPAT
class PowderDiffractionDlg : public wxDialog
{
public:
    PowderDiffractionDlg(wxWindow* parent, wxWindowID id);
};
#endif // STANDALONE_POWDIFPAT

#endif // FITYK_WX_POWDIFPAT_H_

