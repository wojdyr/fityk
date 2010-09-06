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
    void OnXAxisSelected(wxCommandEvent& event);
    void OnAnodeSelected(wxCommandEvent& event);
    void OnQuickPhaseSelected(wxCommandEvent& event);
    void OnQuickListRemove(wxCommandEvent&);
    void OnQuickListImport(wxCommandEvent&);
    void OnLambdaChange(wxCommandEvent& event);
    void OnPageChanged(wxListbookEvent& event);
    void OnPeakRadio(wxCommandEvent& event);
    void OnPeakSplit(wxCommandEvent& event);
    void OnWidthRadio(wxCommandEvent& event);
    void OnShapeRadio(wxCommandEvent& event);
    void OnDelButton(wxCommandEvent&);
    void OnOk(wxCommandEvent&);
    PhasePanel *get_phase_panel(int n);
    PhasePanel *get_current_phase_panel();
    void deselect_phase_quick_list();
    RadiationType get_radiation_type() const;
    double get_lambda(int n) const;
    double get_min_d() const;
    double d2x(double d) const;
    bool is_d_active(double d) const;
    wxListBox *get_saved_phase_lb() { return saved_phase_lb; }
    void update_phase_labels(PhasePanel* p);
    double get_x_min() const { return x_min; }
    double get_x_max() const { return x_max; }
#if STANDALONE_POWDIFPAT
    void OnFilePicked(wxFileDirPickerEvent& event);
    void set_file(wxString const& path);
#endif

private:
    // used to store extra phase data (i.e. data not stored in variables)
    struct PhasePanelExtraData
    {
        wxString name;
        wxString sg;
        wxString atoms;
    };
    static int xaxis_sel;
    static std::vector<PhasePanelExtraData> phase_desc;

    wxPanel *wave_panel;
    wxListBox *anode_lb, *saved_phase_lb;
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
    wxRadioBox *radiation_rb, *xaxis_rb;
    wxRadioBox *peak_rb, *center_rb, *width_rb, *shape_rb;
    wxCheckBox *split_cb;
    wxTextCtrl *peak_txt;
    LockableRealCtrl *par_u, *par_v, *par_w, *par_z, *par_a, *par_b, *par_c;
    wxTextCtrl *action_del_txt, *action_txt;

    wxPanel* PrepareIntroPanel();
    wxPanel* PrepareInstrumentPanel();
    wxPanel* PrepareSamplePanel();
    wxPanel* PreparePeakPanel();
    wxPanel* PrepareActionPanel();
    wxPanel* PrepareSizeStrainPanel();
    void initialize_quick_phase_list();
    void update_peak_parameters();
    wxString prepare_commands();
    void fill_forms();
    wxString get_peak_name() const;
    void save_phase_desc();
};

#if !STANDALONE_POWDIFPAT
class PowderDiffractionDlg : public wxDialog
{
public:
    PowderDiffractionDlg(wxWindow* parent, wxWindowID id);
};
#endif // STANDALONE_POWDIFPAT

#endif // FITYK_WX_POWDIFPAT_H_

