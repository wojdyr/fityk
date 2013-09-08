// Author: Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

//TODO:
//
// deconvolution of instrumental profile
// buffer the plot with data
// import .cif files

#include <cmath>
#include <cstring>
#include <algorithm>
#include <iostream>

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/cmdline.h>
#include <wx/listctrl.h>
#include <wx/stdpaths.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/generic/statbmpg.h>

#include "fityk/common.h"
#include "fityk/udf.h"
#include "powdifpat.h"
#include "sgchooser.h"
#include "atomtables.h"
#include "cmn.h"
#include "uplot.h" // BufferedPanel, scale_tics_step()
#include "parpan.h" // LockableRealCtrl 
#include "ceria.h"

#if !STANDALONE_POWDIFPAT
#include "frame.h"
#include "fityk/logic.h"
#include "fityk/data.h"
#endif

// icons and images
#include "img/correction.h"
#include "img/info32.h"
#include "img/peak32.h"
#include "img/radiation32.h"
#include "img/rubik32.h"
#include "img/run32.h"
#include "img/sizes32.h"

using namespace std;

class PlotWithLines : public PlotWithTics
{
public:
    PlotWithLines(wxWindow* parent, PhasePanel *phase_panel,
                  PowderBook *powder_book);
    virtual void draw(wxDC &dc, bool);

private:
    void draw_active_data(wxDC& dc);

    PhasePanel *phase_panel_;
    PowderBook *powder_book_;
    DISALLOW_COPY_AND_ASSIGN(PlotWithLines);
};

class PhasePanel : public wxPanel
{
    friend class PowderBook;
public:
    PhasePanel(wxNotebook *parent, PowderBook *powder_book_);
    void OnSpaceGroupButton(wxCommandEvent& event);
    void OnClearButton(wxCommandEvent& event);
    void OnAddToQLButton(wxCommandEvent& event);
    void OnNameChanged(wxCommandEvent& event);
    void OnSpaceGroupChanged(wxCommandEvent&);
    void OnSpaceGroupChanging(wxCommandEvent&)
                               { powder_book->deselect_phase_quick_list(); }
    void OnParameterChanging(wxCommandEvent&);
    void OnParameterChanged(wxCommandEvent&);
    void OnLineToggled(wxCommandEvent& event);
    void OnLineSelected(wxCommandEvent& event);
    void OnAtomsFocus(wxFocusEvent&);
    void OnAtomsUnfocus(wxFocusEvent&);
    void OnAtomsChanged(wxCommandEvent& event);
    void set_phase(string const& name, CelFile const& cel);
    const Crystal& get_crystal() const { return cr_; }
    Crystal& get_crystal() { return cr_; }
    bool editing_atoms() const { return editing_atoms_; }
    int get_selected_hkl() const { return hkl_list->GetSelection(); }

private:
    static const wxString default_atom_string;
    PowderBook *powder_book;

    wxTextCtrl *name_tc, *sg_tc;
    LockableRealCtrl *par_a, *par_b, *par_c, *par_alpha, *par_beta, *par_gamma;
    wxButton *save_btn;
    wxCheckListBox *hkl_list;
    wxTextCtrl *atoms_tc, *info_tc;
    wxStaticText *sg_nr_st;
    bool atoms_show_help_;
    bool editing_atoms_;
    PlotWithLines *sample_plot_;
    // line number with the first syntax error atoms_tc; -1 if correct
    int line_with_error_;

    Crystal cr_;

    void enable_parameter_fields();
    void update_disabled_parameters();
    void update_miller_indices(bool sg_changed);
    void set_ortho_angles();
    void update_space_group_ui();

    DISALLOW_COPY_AND_ASSIGN(PhasePanel);
};

namespace {

LockableRealCtrl *addMaybeRealCtrl(wxWindow *parent, wxString const& label,
                                   wxSizer *sizer, wxSizerFlags const& flags,
                                   bool locked=true)
{
    wxStaticText *st = new wxStaticText(parent, -1, label);
    LockableRealCtrl *ctrl = new LockableRealCtrl(parent);
    ctrl->set_lock(locked);
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(st, 0, wxALIGN_CENTER_VERTICAL);
    hsizer->Add(ctrl, 0);
    sizer->Add(hsizer, flags);
    return ctrl;
}

} // anonymous namespace

#ifndef wxTBK_BUTTONBAR // for wx < 2.9
#define wxTBK_BUTTONBAR wxBK_BUTTONBAR
#endif

vector<PowderBook::PhasePanelExtraData> PowderBook::phase_desc;
int PowderBook::xaxis_sel = 0;

PowderBook::PowderBook(wxWindow* parent, wxWindowID id)
    : wxToolbook(parent, id, wxDefaultPosition, wxDefaultSize,
#ifdef __WXMAC__
                 wxTBK_BUTTONBAR
#else
                 wxBK_LEFT
#endif
                 ),
      x_min(10), x_max(150), y_max(1000), data(NULL)
{
#if !STANDALONE_POWDIFPAT
    int data_nr = frame->get_focused_data_index();
    if (data_nr >= 0) {
        data = ftk->dk.data(data_nr);
        x_min = data->get_x_min();
        x_max = data->get_x_max();
        y_max = 0;
        v_foreach(fityk::Point, p, data->points())
            if (p->is_active && p->y > y_max)
                y_max = p->y;
    }
#endif
    initialize_quick_phase_list();

    wxImageList *image_list = new wxImageList(32, 32);
    image_list->Add(GET_BMP(info32));
    image_list->Add(GET_BMP(radiation32));
    image_list->Add(GET_BMP(rubik32));
    image_list->Add(GET_BMP(peak32));
    image_list->Add(GET_BMP(run32));
    image_list->Add(GET_BMP(sizes32));
    AssignImageList(image_list);

    AddPage(PrepareIntroPanel(), wxT("intro"), false, 0);
    AddPage(PrepareInstrumentPanel(), wxT("instrument"), false, 1);
    AddPage(PrepareSamplePanel(), wxT("sample"), false, 2);
    AddPage(PreparePeakPanel(), wxT("peak"), false, 3);
    AddPage(PrepareActionPanel(), wxT("action"), false, 4);
    AddPage(PrepareSizeStrainPanel(), wxT("size-strain"), false, 5);
#if !STANDALONE_POWDIFPAT
    fill_forms();
#endif

    wxColour c = GetToolBar()->GetBackgroundColour();
    GetToolBar()->SetBackgroundColour(wxColour(max(0, c.Red() - 50),
                                               max(0, c.Green() - 50),
                                               c.Blue(), c.Alpha()));

    Connect(GetId(), wxEVT_COMMAND_TOOLBOOK_PAGE_CHANGED,
            (wxObjectEventFunction) &PowderBook::OnPageChanged);
}

static
wxString get_cel_files_dir()
{
    wxString fityk_dir = wxStandardPaths::Get().GetUserDataDir();
#if STANDALONE_POWDIFPAT
    fityk_dir.Replace(wxT("powdifpat"), wxT("fityk"));
#endif
    wxString cel_dir = fityk_dir + wxFILE_SEP_PATH
                       + wxT("cel_files") + wxFILE_SEP_PATH;
    return cel_dir;
}

void PowderBook::initialize_quick_phase_list()
{
    wxString cel_dir = get_cel_files_dir();
    if (!wxDirExists(cel_dir)) {
        wxMkdir(cel_dir);
        write_default_cel_files((const char*) cel_dir.mb_str());
    }

    wxDir dir(cel_dir);
    wxString filename;
    bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
    while (cont)
    {
        FILE *f = wxFopen(cel_dir + filename, wxT("r"));
        if (f) {
            CelFile cel = read_cel_file(f);
            fclose(f);
            if (cel.sgs != NULL) {
                wxString name = wxFileName(filename).GetName();
                quick_phase_list[wx2s(name)] = cel;
            }
        }
        cont = dir.GetNext(&filename);
    }
}


wxPanel* PowderBook::PrepareIntroPanel()
{
    static const char* intro_str =
#if STANDALONE_POWDIFPAT
    "This simple program can generate diffraction pattern using information\n"
    "about symmetry group, unit cell and radiation wavelength.\n"
    "At this moment the program is experimental and the old PowderCell\n"
    "is definitely more reliable.\n"
    "This is a stand-alone version of a dialog in fityk program.\n";
#else
    "Before you start:\n"
    " - load a powder diffraction pattern,\n"
    " - if you want to analyze only a part of the pattern, deactivate the rest"
    "\n"
    " - either remove the background manually\n"
    "   or add a polynomial (or other function) to model it.\n"
    "\n"
    "This tool constructs a model for powder diffraction data. "
    "The model has constrained positions of peaks "
    "and unconstrained intensities. "
    "Fitting the model to the data optimizes all parameters at the same time. "
    "This type of refinement is known as the Pawley method.\n"
    "\n"
    "Space group data have been generated using the CCTBX library.\n"
    "\n"
    "The size-strain analysis is not implemented yet.\n";
#endif

    wxPanel *panel = new wxPanel(this);
    wxSizer *intro_sizer = new wxBoxSizer(wxVERTICAL);
    wxTextCtrl *intro_tc = new wxTextCtrl(panel, -1, pchar2wx(intro_str),
                                          wxDefaultPosition, wxDefaultSize,
                                          wxTE_READONLY|wxTE_MULTILINE
                                          |wxNO_BORDER);
    intro_tc->SetBackgroundColour(GetBackgroundColour());
    intro_sizer->Add(intro_tc, wxSizerFlags(1).Expand().Border(wxALL, 20));
#if STANDALONE_POWDIFPAT
    wxStaticText *file_picker_desc = new wxStaticText(panel, -1,
            wxT("You can use a data file as a background image"));
    intro_sizer->Add(file_picker_desc, wxSizerFlags().Expand().Border());
    file_picker = new wxFilePickerCtrl(panel, -1);
    intro_sizer->Add(file_picker, wxSizerFlags().Expand().Border());
    wxBoxSizer *range_sizer = new wxBoxSizer(wxHORIZONTAL);
    range_sizer->Add(new wxStaticText(panel, -1, wxT("Generate pattern from")),
                     wxSizerFlags().Center().Border());
    range_from = new RealNumberCtrl(panel, -1, wxT("20"));
    range_sizer->Add(range_from, wxSizerFlags().Center().Border());
    range_sizer->Add(new wxStaticText(panel, -1, wxT("to")),
                     wxSizerFlags().Center().Border());
    range_to = new RealNumberCtrl(panel, -1, wxT("150"));
    range_sizer->Add(range_to, wxSizerFlags().Center().Border());
    intro_sizer->Add(range_sizer);
    Connect(file_picker->GetId(), wxEVT_COMMAND_FILEPICKER_CHANGED,
            (wxObjectEventFunction) &PowderBook::OnFilePicked);
#endif

    wxStaticBoxSizer *legend_box = new wxStaticBoxSizer(wxHORIZONTAL,
                                                        panel, wxT("Legend"));
    wxFlexGridSizer *legend = new wxFlexGridSizer(2, 5, 5);
    legend->Add(new wxStaticBitmap(panel, -1, wxBitmap(get_lock_xpm())),
                wxSizerFlags().Center().Right());
    legend->Add(new wxStaticText(panel, -1, wxT("parameter is known")),
                wxSizerFlags().Center().Left());
    legend->Add(new wxStaticBitmap(panel, -1, wxBitmap(get_lock_open_xpm())),
                wxSizerFlags().Center().Right());
    legend->Add(new wxStaticText(panel, -1,
                                 wxT("parameter should be optimized")),
                wxSizerFlags().Center().Left());
    legend_box->Add(legend, wxSizerFlags(1).Expand().Border());
    intro_sizer->Add(legend_box);
    intro_sizer->AddSpacer(20);

    panel->SetSizerAndFit(intro_sizer);
    return panel;
}


wxPanel* PowderBook::PrepareInstrumentPanel()
{
    wxPanel *panel = new wxPanel(this);
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    wxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);

    radiation_rb = new wxRadioBox(panel, -1, wxT("radiation"),
                                  wxDefaultPosition, wxDefaultSize,
                                  ArrayString(wxT("x-ray"), wxT("neutron")),
                                  1, wxRA_SPECIFY_ROWS);
    radiation_rb->SetToolTip(wxT("used only to set initial intensities"));
    hsizer->AddSpacer(50);
    hsizer->Add(radiation_rb, wxSizerFlags().Border());
    hsizer->AddStretchSpacer();

    wxArrayString xaxis_choices;
    xaxis_choices.Add(wxT("2\u03B8")); //\u03B8 = theta
    xaxis_choices.Add(wxT("Q"));
    xaxis_choices.Add(wxT("d"));
    xaxis_rb = new wxRadioBox(panel, -1, wxT("x axis"),
                                          wxDefaultPosition, wxDefaultSize,
                                          xaxis_choices, 3);
    // \u03C0 = pi, \u03BB = lambda
    wxString desc = wxT("Q = 4 \u03C0 sin(\u03B8) / \u03BB\n")
                    wxT("d = \u03BB / (2 sin \u03B8)");
    xaxis_rb->SetToolTip(desc);
    xaxis_rb->SetSelection(xaxis_sel);
    hsizer->Add(xaxis_rb, wxSizerFlags().Border());
    hsizer->AddSpacer(50);
    sizer->Add(hsizer, wxSizerFlags().Expand());

    // wavelength panel
    wave_panel = new wxPanel(panel, -1);
    wxSizer *wave_sizer = new wxStaticBoxSizer(wxHORIZONTAL, wave_panel,
                                               wxT("wavelengths"));
    anode_lb = new wxListBox(wave_panel, -1);
    for (Anode const* i = anodes; i->name; ++i) {
        anode_lb->Append(pchar2wx(i->name));
        anode_lb->Append(pchar2wx(i->name) + wxT(" A1"));
        anode_lb->Append(pchar2wx(i->name) + wxT(" A12"));
    }
#ifdef __WXMAC__
    anode_lb->EnsureVisible(0);
#endif
    wave_sizer->Add(anode_lb, wxSizerFlags(1).Border().Expand());

    wxSizer *lambda_sizer = new wxFlexGridSizer(2, 5, 5);
    lambda_sizer->Add(new wxStaticText(wave_panel, -1, wxT("wavelength")),
                  wxSizerFlags().Center());
    lambda_sizer->Add(new wxStaticText(wave_panel, -1, wxT("intensity")),
                  wxSizerFlags().Center());
    for (int i = 0; i < max_wavelengths; ++i) {
        LockableRealCtrl *lambda = new LockableRealCtrl(wave_panel);
        lambda_sizer->Add(lambda);
        lambda_ctrl.push_back(lambda);

        LockableRealCtrl *intens = new LockableRealCtrl(wave_panel, true);
        lambda_sizer->Add(intens);
        intensity_ctrl.push_back(intens);

        if (i == 0) {
            intens->set_value(100);
            intens->Enable(false);
            //intens->SetEditable(false);
        }
    }
    lambda_sizer->AddSpacer(20);
    lambda_sizer->AddSpacer(1);

    wave_sizer->Add(lambda_sizer, wxSizerFlags(1).Border());
    wave_panel->SetSizerAndFit(wave_sizer);
    if (xaxis_sel != 0)
        wave_panel->Enable(false);
    sizer->Add(wave_panel, wxSizerFlags().Expand());

    // x corrections
    wxSizer *corr_sizer = new wxStaticBoxSizer(wxVERTICAL, panel,
                                           wxT("corrections (use with care)"));
    corr_sizer->Add(new wxGenericStaticBitmap(panel, -1, GET_BMP(correction)),
                    wxSizerFlags().Border());

    wxSizer *g_sizer = new wxGridSizer(2, 5, 5);
    for (int i = 1; i <= 6; ++i) {
        wxSizer *px_sizer = new wxBoxSizer(wxHORIZONTAL);
        wxString label = wxString::Format(wxT("p%d = "), i);
        px_sizer->Add(new wxStaticText(panel, -1, label),
                     wxSizerFlags().Center());
        LockableRealCtrl *cor = new LockableRealCtrl(panel);
        px_sizer->Add(cor);
        g_sizer->Add(px_sizer, wxSizerFlags());
        corr_ctrl.push_back(cor);
    }
    corr_sizer->Add(g_sizer, wxSizerFlags().Expand().Border());
    sizer->Add(corr_sizer, wxSizerFlags().Expand());

    panel->SetSizerAndFit(sizer);

    for (size_t i = 0; i < lambda_ctrl.size(); ++i)
        lambda_ctrl[i]->Connect(wxID_ANY, wxEVT_COMMAND_TEXT_UPDATED,
                           wxCommandEventHandler(PowderBook::OnLambdaChange),
                           NULL, this);
    for (size_t i = 1; i < intensity_ctrl.size(); ++i)
        intensity_ctrl[i]->Connect(wxID_ANY, wxEVT_COMMAND_TEXT_UPDATED,
                           wxCommandEventHandler(PowderBook::OnLambdaChange),
                           NULL, this);
    Connect(anode_lb->GetId(), wxEVT_COMMAND_LISTBOX_SELECTED,
            wxCommandEventHandler(PowderBook::OnAnodeSelected));
    Connect(xaxis_rb->GetId(), wxEVT_COMMAND_RADIOBOX_SELECTED,
            wxCommandEventHandler(PowderBook::OnXAxisSelected));

    return panel;
}

PlotWithLines::PlotWithLines(wxWindow* parent, PhasePanel* phase_panel,
                             PowderBook* powder_book)
    : PlotWithTics(parent),
      phase_panel_(phase_panel),
      powder_book_(powder_book)
{
    set_bg_color(wxColour(64, 64, 64));
}

static
double get_max_intensity(const vector<PlanesWithSameD>& bp)
{
    double max_intensity = 0.;
    v_foreach (PlanesWithSameD, i, bp)
        if (i->intensity > max_intensity)
            max_intensity = i->intensity;
    return max_intensity;
}

void PlotWithLines::draw(wxDC &dc, bool)
{
    draw_tics(dc, powder_book_->x_min, powder_book_->x_max,
                  0, powder_book_->y_max);
    // draw data
    dc.SetPen(*wxGREEN_PEN);
    draw_active_data(dc);
    //TODO buffer the plot with tics and data

    // draw lines
    int Y0 = dc.GetSize().GetHeight();
    int Yx = getY(0); // Y of x axis

    wxPen peak_pen(*wxRED, 1, wxPENSTYLE_DOT);
    //wxYELLOW_PEN and wxYELLOW were added in 2.9
    wxPen sel_peak_pen(wxColour(255, 255, 0));
    wxPen mark_pen = *wxRED_PEN;

    dc.SetPen(peak_pen);
    const vector<PlanesWithSameD>& bp = phase_panel_->get_crystal().bp;
    int selected = phase_panel_->get_selected_hkl();
    double max_intensity = get_max_intensity(bp);;
    double h_mult = 0;
    if (max_intensity > 0)
        h_mult = powder_book_->y_max / max_intensity;
    v_foreach (PlanesWithSameD, i, bp) {
        if (!i->enabled)
            continue;
        bool is_selected = (i - bp.begin() == selected);
        dc.SetPen(is_selected ? sel_peak_pen : mark_pen);
        double x = powder_book_->d2x(i->d);
        int X = getX(x);
        // draw short line at the bottom to mark position of the peak
        dc.DrawLine(X, Y0, X, (Yx+Y0)/2);
        if (i->intensity && !phase_panel_->editing_atoms()) {
            int Y1 = getY(h_mult * i->intensity);
            dc.SetPen(is_selected ? sel_peak_pen : peak_pen);
            // draw line that has height proportional to peak intensity
            dc.DrawLine(X, Yx, X, Y1);
        }
    }
}

void PlotWithLines::draw_active_data(wxDC& dc)
{
#if STANDALONE_POWDIFPAT
    if (!powder_book_->data)
        return;
    xylib::Block const *block = powder_book_->data->get_block(0);
    xylib::Column const& xcol = block->get_column(1);
    xylib::Column const& ycol = block->get_column(2);
    int n = block->get_point_count();
    //for (int i = 0; i != n; ++i)
    //    draw_point(dc, xcol.get_value(i), ycol.get_value(i));
    wxPoint *points = new wxPoint[n];
    for (int i = 0; i != n; ++i) {
        points[i].x = getX(xcol.get_value(i));
        points[i].y = getY(ycol.get_value(i));
    }
#else // STANDALONE_POWDIFPAT
    if (!powder_book_->data)
        return;
    int n = powder_book_->data->get_n();
    if (n == 0)
        return;
    wxPoint *points = new wxPoint[n];
    for (int i = 0; i != n; ++i) {
        points[i].x = getX(powder_book_->data->get_x(i));
        points[i].y = getY(powder_book_->data->get_y(i));
    }
#endif // STANDALONE_POWDIFPAT
    dc.DrawLines(n, points);
    delete [] points;
}

const wxString PhasePanel::default_atom_string =
    wxT("Optional: atom positions, e.g.\n")
    wxT("Si 0 0 0\n")
    wxT("C 0.25 0.25 0.25");

PhasePanel::PhasePanel(wxNotebook *parent, PowderBook *powder_book_)
        : wxPanel(parent), powder_book(powder_book_),
          line_with_error_(-1)
{
    wxSizer *vsizer = new wxBoxSizer(wxVERTICAL);

    wxSizer *h0sizer = new wxBoxSizer(wxHORIZONTAL);
    h0sizer->Add(new wxStaticText(this, -1, wxT("Name:")),
                 wxSizerFlags().Center().Border());
    name_tc = new KFTextCtrl(this, -1, wxEmptyString, 150);
    h0sizer->Add(name_tc, wxSizerFlags().Center().Border());

    wxBoxSizer *h2sizer = new wxBoxSizer(wxHORIZONTAL);
    save_btn = new wxButton(this, wxID_SAVE);
    h2sizer->Add(save_btn, wxSizerFlags().Border());
    h2sizer->AddStretchSpacer();
    wxButton *s_clear_btn = new wxButton(this, wxID_CLEAR);
    h2sizer->Add(s_clear_btn, wxSizerFlags().Border());
    h0sizer->Add(h2sizer, wxSizerFlags().Expand());

    vsizer->Add(h0sizer, wxSizerFlags().Expand());

    wxSizer *h1sizer = new wxBoxSizer(wxHORIZONTAL);
    h1sizer->Add(new wxStaticText(this, -1, wxT("Space group:")),
                 wxSizerFlags().Center().Border());
    sg_tc = new KFTextCtrl(this, -1, wxT("P 1"), 120);
    h1sizer->Add(sg_tc, wxSizerFlags().Center());
    wxButton *sg_btn = new wxButton(this, -1, wxT("..."),
                                    wxDefaultPosition, wxDefaultSize,
                                    wxBU_EXACTFIT);
    h1sizer->Add(sg_btn, wxSizerFlags().Center().Border());
    sg_nr_st = new wxStaticText(this, -1, wxEmptyString);
    h1sizer->Add(sg_nr_st, wxSizerFlags().Center().Border());

    vsizer->Add(h1sizer, wxSizerFlags().Expand());

    wxSizer *stp_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this,
                                              wxT("lattice parameters"));
    wxSizer *par_sizer = new wxGridSizer(3, 2, 2);
    wxSizerFlags flags = wxSizerFlags().Right();
    par_a = addMaybeRealCtrl(this, wxT("a ="),  par_sizer, flags);
    par_b = addMaybeRealCtrl(this, wxT("b ="),  par_sizer, flags);
    par_c = addMaybeRealCtrl(this, wxT("c ="),  par_sizer, flags);
    // greek letters: alpha \u03B1, beta \u03B2, gamma \u03B3
    par_alpha = addMaybeRealCtrl(this, wxT("\u03B1 ="), par_sizer, flags);
    par_beta = addMaybeRealCtrl(this, wxT("\u03B2 ="),  par_sizer, flags);
    par_gamma = addMaybeRealCtrl(this, wxT("\u03B3 ="),  par_sizer, flags);
    stp_sizer->Add(par_sizer, wxSizerFlags(1).Expand());
    vsizer->Add(stp_sizer, wxSizerFlags().Border().Expand());

    ProportionalSplitter *vsplit = new ProportionalSplitter(this, -1, 0.75);

    ProportionalSplitter *hkl_split = new ProportionalSplitter(vsplit,-1, 0.4);
    hkl_list = new wxCheckListBox(hkl_split, -1);
    sample_plot_ = new PlotWithLines(hkl_split, this, powder_book);

    ProportionalSplitter *atom_split = new ProportionalSplitter(vsplit,-1, 0.6);
    atoms_tc = new wxTextCtrl(atom_split, -1, wxEmptyString,
                          wxDefaultPosition, wxDefaultSize,
                          wxTE_RICH|wxTE_MULTILINE);
    atoms_tc->ChangeValue(default_atom_string);
    atoms_show_help_ = true;
    atoms_tc->SetSelection(-1, -1);
    info_tc = new wxTextCtrl(atom_split, -1, wxEmptyString,
                             wxDefaultPosition, wxDefaultSize,
                             wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE);
    info_tc->SetBackgroundColour(powder_book->GetBackgroundColour());

    vsplit->SplitHorizProp(hkl_split, atom_split);
    hkl_split->SplitVertProp(hkl_list, sample_plot_);
    atom_split->SplitVertProp(atoms_tc, info_tc);

    vsizer->Add(vsplit, wxSizerFlags(1).Expand().Border());
    SetSizerAndFit(vsizer);

    Connect(sg_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &PhasePanel::OnSpaceGroupButton);
    Connect(save_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &PhasePanel::OnAddToQLButton);
    Connect(s_clear_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &PhasePanel::OnClearButton);
    Connect(name_tc->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            (wxObjectEventFunction) &PhasePanel::OnNameChanged);
    Connect(sg_tc->GetId(), wxEVT_COMMAND_TEXT_ENTER,
            (wxObjectEventFunction) &PhasePanel::OnSpaceGroupChanged);
    Connect(sg_tc->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            (wxObjectEventFunction) &PhasePanel::OnSpaceGroupChanging);

    const LockableRealCtrl* param_controls[] =
            { par_a, par_b, par_c, par_alpha, par_beta, par_gamma };
    for (int i = 0; i != sizeof(param_controls) / sizeof(par_a); ++i) {
        wxWindowID id = param_controls[i]->get_text_ctrl()->GetId();
        Connect(id, wxEVT_COMMAND_TEXT_UPDATED,
                (wxObjectEventFunction) &PhasePanel::OnParameterChanging);
        Connect(id, wxEVT_COMMAND_TEXT_ENTER,
                (wxObjectEventFunction) &PhasePanel::OnParameterChanged);
    }

    Connect(hkl_list->GetId(), wxEVT_COMMAND_CHECKLISTBOX_TOGGLED,
            (wxObjectEventFunction) &PhasePanel::OnLineToggled);
    Connect(hkl_list->GetId(), wxEVT_COMMAND_LISTBOX_SELECTED,
            (wxObjectEventFunction) &PhasePanel::OnLineSelected);

    atoms_tc->Connect(wxEVT_SET_FOCUS,
                      (wxObjectEventFunction) &PhasePanel::OnAtomsFocus,
                      NULL, this);
    atoms_tc->Connect(wxEVT_KILL_FOCUS,
                      (wxObjectEventFunction) &PhasePanel::OnAtomsUnfocus,
                      NULL, this);
    atoms_tc->Connect(wxEVT_COMMAND_TEXT_UPDATED,
                      (wxObjectEventFunction) &PhasePanel::OnAtomsChanged,
                      NULL, this);
}

void PhasePanel::OnSpaceGroupButton(wxCommandEvent& event)
{
    SpaceGroupChooser dlg(this, sg_tc->GetValue());
    if (dlg.ShowModal() == wxID_OK)
        sg_tc->SetValue(dlg.get_value());
    OnSpaceGroupChanged(event);
}

void PhasePanel::OnAddToQLButton(wxCommandEvent&)
{
    wxString name = name_tc->GetValue();
    if (name.find(wxT("|")) != string::npos) {
        wxMessageBox(wxT("The pipe character '|' is not allowed in name."),
                     wxT("Error"), wxOK|wxICON_ERROR);
        return;
    }
    wxListBox *saved_phase_lb = powder_book->get_saved_phase_lb();
    if (saved_phase_lb->FindString(name) != wxNOT_FOUND) {
        int answer = wxMessageBox(wxT("Name `") + name
                                        + wxT("' already exists.\nOverwrite?"),
                                  wxT("Overwrite?"), wxYES_NO|wxICON_QUESTION);
        if (answer != wxYES)
            return;
    } else
        saved_phase_lb->Append(name);

    CelFile cel;
    cel.sgs = cr_.sgs;
    cel.a = par_a->get_value();
    cel.b = par_b->get_value();
    cel.c = par_c->get_value();
    cel.alpha = par_alpha->get_value();
    cel.beta = par_beta->get_value();
    cel.gamma = par_gamma->get_value();
    v_foreach (Atom, i, cr_.atoms) {
        t_pse const* pse = find_in_pse(i->symbol);
        assert (pse != NULL);
        AtomInCell aic;
        aic.Z = pse->Z;
        aic.x = i->pos[0].x;
        aic.y = i->pos[0].y;
        aic.z = i->pos[0].z;
        cel.atoms.push_back(aic);
    }

    saved_phase_lb->SetStringSelection(name);
    powder_book->quick_phase_list[wx2s(name)] = cel;
    FILE *f = wxFopen(get_cel_files_dir() + name + wxT(".cel"), wxT("w"));
    if (f) {
        write_cel_file(cel, f);
        fclose(f);
    }
}

void PhasePanel::OnClearButton(wxCommandEvent& event)
{
    name_tc->Clear();
    sg_tc->Clear();
    par_a->Clear();
    par_b->Clear();
    par_c->Clear();
    par_alpha->Clear();
    par_beta->Clear();
    par_gamma->Clear();
    OnSpaceGroupChanged(event);
    powder_book->update_phase_labels(this);
}

void PhasePanel::OnNameChanged(wxCommandEvent&)
{
    powder_book->deselect_phase_quick_list();
    powder_book->update_phase_labels(this);
}

void PhasePanel::OnSpaceGroupChanged(wxCommandEvent&)
{
    string text = wx2s(sg_tc->GetValue());
    const SpaceGroupSetting *sgs = parse_any_sg_symbol(text.c_str());
    cr_.set_space_group(sgs);
    update_space_group_ui();
    update_miller_indices(true);
    powder_book->deselect_phase_quick_list();
    sample_plot_->refresh();
}

void PhasePanel::update_space_group_ui()
{
    if (cr_.sgs == NULL) {
        sg_tc->Clear();
        hkl_list->Clear();
        sg_nr_st->SetLabel(wxEmptyString);
        return;
    }

    sg_tc->ChangeValue(s2wx(fullHM(cr_.sgs)));
    const char* system = get_crystal_system_name(cr_.xs());
    sg_nr_st->SetLabel(wxString::Format(wxT("%d, %s, order %d"),
                                        cr_.sgs->sgnumber,
                                        pchar2wx(system).c_str(),
                                        get_sg_order(cr_.sg_ops)));
    enable_parameter_fields();
    update_disabled_parameters();
}

void PhasePanel::enable_parameter_fields()
{
    switch (cr_.xs()) {
        case TriclinicSystem:
            par_b->Enable(true);
            par_c->Enable(true);
            par_alpha->Enable(true);
            par_beta->Enable(true);
            par_gamma->Enable(true);
            break;
        case MonoclinicSystem:
            par_b->Enable(true);
            par_c->Enable(true);
            par_beta->Enable(true);
            par_alpha->Enable(false);
            par_alpha->set_value(90.);
            par_gamma->Enable(false);
            par_gamma->set_value(90.);
            break;
        case OrthorhombicSystem:
            par_b->Enable(true);
            par_c->Enable(true);
            set_ortho_angles();
            break;
        case TetragonalSystem:
            par_b->Enable(false);
            par_c->Enable(true);
            set_ortho_angles();
            break;
            /*
        Crystals in the trigonal crystal system are either in the rhombohedral
        lattice system or in the hexagonal lattice system.
        Those in the rhombohedral space groups are described either using
        the hexagonal basis or the rhombohedral basis.
        The latter is not supported yet.
        case RhombohedralSystem:
            par_b->Enable(false);
            par_c->Enable(false);
            par_alpha->Enable(true);
            par_beta->Enable(false);
            par_gamma->Enable(false);
            break;
            */
        case TrigonalSystem:
        case HexagonalSystem:
            par_b->Enable(false);
            par_c->Enable(true);
            par_alpha->set_value(90);
            par_beta->set_value(90);
            par_gamma->set_value(120);
            break;
        case CubicSystem:
            par_b->Enable(false);
            par_c->Enable(false);
            set_ortho_angles();
            break;
        case UndefinedSystem:
            break;
    }
}

void PhasePanel::set_ortho_angles()
{
    par_alpha->Enable(false);
    par_alpha->set_value(90.);
    par_beta->Enable(false);
    par_beta->set_value(90.);
    par_gamma->Enable(false);
    par_gamma->set_value(90.);
}

void PhasePanel::OnParameterChanging(wxCommandEvent&)
{
    update_disabled_parameters();
    powder_book->deselect_phase_quick_list();
}

void PhasePanel::OnParameterChanged(wxCommandEvent&)
{
    update_miller_indices(false);
    sample_plot_->refresh();
}

void PhasePanel::OnLineToggled(wxCommandEvent& event)
{
    int n = event.GetSelection();
    PlanesWithSameD& bp = cr_.bp[n];
    // event.IsChecked() is not set (wxGTK 2.9)
    bp.enabled = hkl_list->IsChecked(n);

    sample_plot_->refresh();
}

static
wxString make_info_string_for_line(const PlanesWithSameD& bp,
                                   PowderBook *powder_book)
{
    wxString info = wxT("line ");
    wxString mult_str = wxT("multiplicity: ");
    wxString sfac_str = wxT("|F(hkl)|: ");
    v_foreach (Plane, i, bp.planes) {
        if (i != bp.planes.begin()) {
            info += wxT(", ");
            mult_str += wxT(", ");
            sfac_str += wxT(", ");
        }
        info += wxString::Format(wxT("(%d,%d,%d)"), i->h, i->k, i->l);
        mult_str += wxString::Format(wxT("%d"), i->multiplicity);
        sfac_str += wxString::Format(wxT("%g"), sqrt(i->F2));
    }
    info += wxString::Format(wxT("\nd=%g\n"), bp.d);
    info += mult_str + wxT("\n") + sfac_str;
    info += wxString::Format(wxT("\nLorentz-polarization: %g"), bp.lpf);
    info += wxString::Format(wxT("\ntotal intensity: %g"), bp.intensity);
    for (int i = 0; ; ++i) {
        double lambda = powder_book->get_lambda(i);
        if (lambda == 0.)
            break;
        wxString format = (i == 0 ? wxT("\n2\u03B8: %g") : wxT(", %g"));
        info += wxString::Format(format, lambda);
    }
    return info;
}

static
wxString make_info_string_for_atoms(const vector<Atom>& atoms, int error_line)
{
    wxString info = wxT("In unit cell:");
    v_foreach (Atom, i, atoms) {
        info += wxString::Format(wxT(" %d %s "), (int) i->pos.size(),
                                                 pchar2wx(i->symbol).c_str());
    }
    if (error_line != -1)
        info += wxString::Format(wxT("\nError in line %d."), error_line);
    return info;
}


void PhasePanel::OnLineSelected(wxCommandEvent& event)
{
    if (event.IsSelection()) {
        int n = event.GetSelection();
        PlanesWithSameD const& bp = cr_.bp[n];
        info_tc->ChangeValue(make_info_string_for_line(bp, powder_book));
    }
    sample_plot_->refresh();
}

void PhasePanel::OnAtomsFocus(wxFocusEvent&)
{
    if (atoms_show_help_) {
        atoms_tc->Clear();
        atoms_show_help_ = false;
    }
    wxString info = make_info_string_for_atoms(cr_.atoms, line_with_error_);
    info_tc->ChangeValue(info);
    editing_atoms_ = true;
    sample_plot_->refresh();
}

void PhasePanel::OnAtomsUnfocus(wxFocusEvent&)
{
    double lambda = powder_book->get_lambda(0);
    cr_.update_intensities(powder_book->get_radiation_type(), lambda);
    int n = hkl_list->GetSelection();
    if (n >= 0) {
        PlanesWithSameD const& bp = cr_.bp[n];
        info_tc->SetValue(make_info_string_for_line(bp, powder_book));
    }
    editing_atoms_ = false;
    sample_plot_->refresh();
}

void PhasePanel::OnAtomsChanged(wxCommandEvent&)
{
    string atoms_str = wx2s(atoms_tc->GetValue());
    if (atoms_str.size() < 3) {
        info_tc->Clear();
        return;
    }
    line_with_error_ = parse_atoms(atoms_str.c_str(), cr_);

    powder_book->deselect_phase_quick_list();
    wxString info = make_info_string_for_atoms(cr_.atoms, line_with_error_);
    info_tc->SetValue(info);
}

void PhasePanel::update_disabled_parameters()
{
    if (!par_b->IsEnabled())
        par_b->set_string(par_a->get_string());

    if (!par_c->IsEnabled())
        par_c->set_string(par_a->get_string());

    /*
    if (cr_.xs() == RhombohedralSystem) {
        par_beta->set_string(par_alpha->get_string());
        par_gamma->set_string(par_alpha->get_string());
    }
    */
}

void PhasePanel::update_miller_indices(bool sg_changed)
{
    double a = par_a->get_value();
    double b = par_b->get_value();
    double c = par_c->get_value();
    double alpha = par_alpha->get_value() * M_PI / 180.;
    double beta = par_beta->get_value() * M_PI / 180.;
    double gamma = par_gamma->get_value() * M_PI / 180.;

    double lambda = powder_book->get_lambda(0);
    double min_d = powder_book->get_min_d();
    RadiationType radiation = powder_book->get_radiation_type();

    if (a <= 0 || b <= 0 || c <= 0 || alpha <= 0 || beta <= 0 || gamma <= 0
            || min_d <= 0) {
        hkl_list->Clear();
        return;
    }

    if (sg_changed || cr_.uc == NULL ||
            cr_.uc->a != a || cr_.uc->b != b || cr_.uc->c != c ||
            cr_.uc->alpha != alpha || cr_.uc->beta != beta ||
            cr_.uc->gamma != gamma || cr_.old_min_d != min_d) {
        cr_.set_unit_cell(new UnitCell(a, b, c, alpha, beta, gamma));
        cr_.generate_reflections(min_d);

        hkl_list->Clear();
        vm_foreach (PlanesWithSameD, i, cr_.bp) {
            i->enabled = powder_book->is_d_active(i->d);
            char ac = (i->planes.size() == 1 ? ' ' : '*');
            Miller const& m = i->planes[0];
            hkl_list->Append(wxString::Format(wxT("(%d,%d,%d)%c  d=%g"),
                                              m.h, m.k, m.l, ac, i->d));
            hkl_list->Check(hkl_list->GetCount() - 1, i->enabled);
        }
#ifdef __WXMAC__
        hkl_list->EnsureVisible(0);
#endif
    }

    cr_.update_intensities(radiation, lambda);
}

void PhasePanel::set_phase(string const& name, CelFile const& cel)
{
    name_tc->ChangeValue(s2wx(name));
    powder_book->update_phase_labels(this);
    cr_.set_space_group(cel.sgs);
    par_a->set_string(wxString::Format(wxT("%g"), (cel.a)));
    par_b->set_string(wxString::Format(wxT("%g"), (cel.b)));
    par_c->set_string(wxString::Format(wxT("%g"), (cel.c)));
    par_alpha->set_string(wxString::Format(wxT("%g"), (cel.alpha)));
    par_beta->set_string(wxString::Format(wxT("%g"), (cel.beta)));
    par_gamma->set_string(wxString::Format(wxT("%g"), (cel.gamma)));
    wxString atoms_str;
    v_foreach (AtomInCell, i, cel.atoms) {
        t_pse const* pse = find_Z_in_pse(i->Z);
        if (pse == NULL)
            continue;
        atoms_str += wxString::Format(wxT("%s %g %g %g\n"),
                                      pchar2wx(pse->symbol).c_str(),
                                      i->x, i->y, i->z);
    }
    atoms_tc->ChangeValue(atoms_str);
    update_space_group_ui();

    cr_.atoms.clear();
    line_with_error_ = parse_atoms((const char*) atoms_str.mb_str(), cr_);
    wxString info = make_info_string_for_atoms(cr_.atoms, line_with_error_);
    info_tc->SetValue(info);
    atoms_show_help_ = false;
    update_miller_indices(true);
}

wxPanel* PowderBook::PrepareSamplePanel()
{
    wxPanel *panel = new wxPanel(this);
    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    wxSizer *left_sizer = new wxStaticBoxSizer(wxVERTICAL, panel,
                                               wxT("saved structures"));
    saved_phase_lb = new wxListBox(panel, -1, wxDefaultPosition, wxDefaultSize,
                                   0, NULL, wxLB_SINGLE|wxLB_SORT);

    for (map<string, CelFile>::const_iterator i = quick_phase_list.begin();
                                            i != quick_phase_list.end(); ++i)
        saved_phase_lb->Append(s2wx(i->first));

#ifdef __WXMAC__
    saved_phase_lb->EnsureVisible(0);
#endif
    left_sizer->Add(saved_phase_lb, wxSizerFlags(1).Border().Expand());
    wxButton *s_remove_btn = new wxButton(panel, wxID_REMOVE);
    left_sizer->Add(s_remove_btn,
                    wxSizerFlags().Center().Border(wxLEFT|wxRIGHT));
    wxButton *s_import_btn = new wxButton(panel, wxID_OPEN, wxT("Import"));
    left_sizer->Add(s_import_btn, wxSizerFlags().Center().Border());

    sizer->Add(left_sizer, wxSizerFlags().Expand());

    sample_nb = new wxNotebook(panel, -1, wxDefaultPosition, wxDefaultSize,
                               wxNB_BOTTOM);
    PhasePanel *page = new PhasePanel(sample_nb, this);
    sample_nb->AddPage(page, wxT("+"));

    sizer->Add(sample_nb, wxSizerFlags(1).Expand());

    panel->SetSizerAndFit(sizer);

    Connect(saved_phase_lb->GetId(), wxEVT_COMMAND_LISTBOX_SELECTED,
            wxCommandEventHandler(PowderBook::OnQuickPhaseSelected));
    Connect(s_remove_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(PowderBook::OnQuickListRemove));
    Connect(s_import_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(PowderBook::OnQuickListImport));

    return panel;
}

wxPanel* PowderBook::PreparePeakPanel()
{
    wxPanel *panel = new wxPanel(this);
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    wxArrayString peak_choices;
    peak_choices.Add(wxT("Gaussian"));
    peak_choices.Add(wxT("Lorentzian"));
    peak_choices.Add(wxT("Pearson VII"));
    peak_choices.Add(wxT("Pseudo-Voigt"));
    peak_choices.Add(wxT("Voigt"));
    peak_rb = new wxRadioBox(panel, -1, wxT("peak function"),
                             wxDefaultPosition, wxDefaultSize, peak_choices, 5);
    peak_rb->SetSelection(3 /*Pseudo-Voigt*/);
    sizer->Add(peak_rb, wxSizerFlags().Expand().Border());

    split_cb = new wxCheckBox(panel, -1, wxT("split (different widths and shapes for both sides)"));
    sizer->Add(split_cb, wxSizerFlags().Border());

    wxArrayString center_choices;
    center_choices.Add(wxT("independent for each peak"));
    center_choices.Add(wxT("constrained by wavelength and lattice parameters"));
    center_rb = new wxRadioBox(panel, -1, wxT("peak position"),
                         wxDefaultPosition, wxDefaultSize, center_choices, 1);
    center_rb->SetSelection(1);
    center_rb->Enable(0, false);
    sizer->Add(center_rb, wxSizerFlags().Expand().Border());

    wxArrayString peak_widths;
    peak_widths.Add(wxT("independent for each hkl"));
    peak_widths.Add(wxT("H\u00B2=U tan\u00B2\u03B8 + V tan\u03B8 + W + ")
                    wxT("Z/cos\u00B2\u03B8"));
    width_rb = new wxRadioBox(panel, -1, wxT("peak width"),
                              wxDefaultPosition, wxDefaultSize, peak_widths, 1);
    sizer->Add(width_rb, wxSizerFlags().Expand().Border());

    wxArrayString peak_shapes;
    peak_shapes.Add(wxT("independent for each hkl (initial value: A)"));
    peak_shapes.Add(wxT("A + B (2\u03B8) + C (2\u03B8)\u00B2"));
    peak_shapes.Add(wxT("A + B / (2\u03B8) + C / (2\u03B8)\u00B2"));
    shape_rb = new wxRadioBox(panel, -1, wxT("peak shape"),
                              wxDefaultPosition, wxDefaultSize, peak_shapes, 1);
    sizer->Add(shape_rb, wxSizerFlags().Expand().Border());

    wxSizer *stp_sizer = new wxStaticBoxSizer(wxHORIZONTAL, panel,
                                              wxT("initial parameters"));
    wxSizer *par_sizer = new wxGridSizer(4, 10, 5);
    wxSizerFlags flags = wxSizerFlags().Right();
    par_u = addMaybeRealCtrl(panel, wxT("U ="),  par_sizer, flags, false);
    par_v = addMaybeRealCtrl(panel, wxT("V ="),  par_sizer, flags, false);
    par_w = addMaybeRealCtrl(panel, wxT("W ="),  par_sizer, flags, false);
    par_w->set_value(0.1);
    par_z = addMaybeRealCtrl(panel, wxT("Z ="),  par_sizer, flags, false);
    par_a = addMaybeRealCtrl(panel, wxT("A ="),  par_sizer, flags, false);
    par_a->set_value(0.8); // valid value for both Voigt and Pearson7
    par_b = addMaybeRealCtrl(panel, wxT("B ="),  par_sizer, flags, false);
    par_c = addMaybeRealCtrl(panel, wxT("C ="),  par_sizer, flags, false);
    stp_sizer->Add(par_sizer, wxSizerFlags(1).Expand());
    sizer->Add(stp_sizer, wxSizerFlags().Border().Expand());

    update_peak_parameters();
    panel->SetSizerAndFit(sizer);

    Connect(peak_rb->GetId(), wxEVT_COMMAND_RADIOBOX_SELECTED,
            wxCommandEventHandler(PowderBook::OnPeakRadio));
    Connect(split_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(PowderBook::OnPeakSplit));
    Connect(width_rb->GetId(), wxEVT_COMMAND_RADIOBOX_SELECTED,
            wxCommandEventHandler(PowderBook::OnWidthRadio));
    Connect(shape_rb->GetId(), wxEVT_COMMAND_RADIOBOX_SELECTED,
            wxCommandEventHandler(PowderBook::OnShapeRadio));

    return panel;
}

wxString PowderBook::get_peak_name() const
{
    wxString s;
    switch (peak_rb->GetSelection()) {
        case 0: s = wxT("Gaussian"); break;
        case 1: s = wxT("Lorentzian"); break;
        case 2: s = wxT("Pearson7"); break;
        case 3: s = wxT("PseudoVoigt"); break;
        case 4: s = wxT("Voigt"); break;
    }
    return split_cb->GetValue() ? wxT("Split") + s : s;
}

void PowderBook::set_peak_name(const string& name)
{
    string basename;
    if (startswith(name, "Split")) {
        split_cb->SetValue(true);
        basename = name.substr(5);
    } else
        basename = name;

    int sel = 0;
    if (basename == "Gaussian")
        sel = 0;
    else if (basename == "Lorentzian")
        sel = 1;
    else if (basename == "Pearson7")
        sel = 2;
    else if (basename == "PseudoVoigt")
        sel = 3;
    else if (basename == "Voigt")
        sel = 4;
    peak_rb->SetSelection(sel);
}

#if !STANDALONE_POWDIFPAT
static
bool has_old_variables()
{
    v_foreach (fityk::Variable*, i, ftk->mgr.variables())
        if (startswith((*i)->name, "pd"))
            return true;
    return false;
}
#endif

wxPanel* PowderBook::PrepareActionPanel()
{
    wxPanel *panel = new wxPanel(this);
#if !STANDALONE_POWDIFPAT
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(new wxStaticText(panel, -1, wxT("Script that prepares model:")),
               wxSizerFlags().Border());
    action_txt = new wxTextCtrl(panel, -1, wxEmptyString,
                                wxDefaultPosition, wxSize(-1, 200),
                                wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE);
    sizer->Add(action_txt, wxSizerFlags(1).Expand().Border(wxLEFT|wxRIGHT));

    // apparently Mac/Carbon does not like multi-line static texts
    wxStaticText *text = new wxStaticText(panel, -1,
     wxT("Press OK to execute the script above and close this window.")
#ifndef __WXMAC__
     wxT("\nIf the initial model is good, fit it to the data.")
     wxT("\nOtherwise, reopen this window and correct the model.")
#endif
     );
    wxFont font = text->GetFont();
    font.SetPointSize(font.GetPointSize() - 1);
    text->SetFont(font);
    sizer->Add(text, wxSizerFlags().Border());

    wxBoxSizer *btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *del_btn = new wxButton(panel, -1, wxT("Delete old model"));
    del_btn->Enable(has_old_variables());
    del_btn->SetToolTip(wxT("delete %pd*, $pd*"));
    btn_sizer->Add(del_btn, wxSizerFlags().Border());
    btn_sizer->AddStretchSpacer();
    wxButton *cancel_btn = new wxButton(panel, wxID_CANCEL);
    btn_sizer->Add(cancel_btn, wxSizerFlags().Border());
    wxButton *ok_btn = new wxButton(panel, wxID_OK);
    btn_sizer->Add(ok_btn, wxSizerFlags().Border());
    sizer->Add(btn_sizer, wxSizerFlags().Expand());

    panel->SetSizerAndFit(sizer);

    Connect(del_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(PowderBook::OnDelButton));
    Connect(ok_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(PowderBook::OnOk));

#endif //!STANDALONE_POWDIFPAT
    return panel;
}

static
wxString get_var(LockableRealCtrl *ctrl, double mult = 1.)
{
    return wxString::Format(ctrl->is_locked() ? wxT("%.9g") : wxT("~%.9g"),
                            ctrl->get_value() * mult);
}

static
wxString hkl2wxstr(const Miller& hkl)
{
    wxString s;
    bool separate = !(hkl.h > -10 && hkl.h < 10 &&
                      hkl.k > -10 && hkl.k < 10 &&
                      hkl.l > -10 && hkl.l < 10);
    if (hkl.h < 0)
        s += wxT("m");
    s += s2wx(S(abs(hkl.h)));
    if (separate)
        s += wxT("_");
    if (hkl.k < 0)
        s += wxT("m");
    s += s2wx(S(abs(hkl.k)));
    if (separate)
        s += wxT("_");
    if (hkl.l < 0)
        s += wxT("m");
    s += s2wx(S(abs(hkl.l)));
    return s;
}

#if !STANDALONE_POWDIFPAT
// All variable and function names have "pd" prefix.
// Each name is composed from a few parts.
// One example: "$pd1b_c201":
//  "pd" - prefix
//  "1" - first phase (usually there is only one phase)
//  "b" - first wavelength
//  "_" - separator
//  "c" - center
//  "201" - hkl=(201)
// The variable $pd1a_c201 contains the center (position) of the (201)
// reflection of the first phase, in the radiation of the first wavelenth.
wxString PowderBook::prepare_commands()
{
    wxString s;
    if (has_old_variables())
        s = wxT("delete %pd*, $pd* # delete old model\n");

    wxString ds_pref;
    if (ftk->dk.count() > 1) {
        int data_nr = frame->get_focused_data_index();
        ds_pref.Printf(wxT("@%d."), data_nr);
    }

    int xaxis_val = xaxis_rb->GetSelection();

    //wavelength
    char lambda_symbol = 'a';
    if (xaxis_val == 0) {
        for (size_t i = 0; i != lambda_ctrl.size(); ++i) {
            if (!lambda_ctrl[i]->is_nonzero()
                    || !intensity_ctrl[i]->is_nonzero())
                continue;
            s += wxString::Format(wxT("$pd_lambda_%c = %s\n"), lambda_symbol,
                                              get_var(lambda_ctrl[i]).c_str());
            if (i != 0) {
                s += wxString::Format(wxT("$pd_intens_%c = %s\n"),
                                      lambda_symbol,
                                      get_var(intensity_ctrl[i], 0.01).c_str());
            }
            ++lambda_symbol;
        }
    } else
        ++lambda_symbol;

    // corrections
    if (ftk->get_tpm()->get_tp("PdXcorr") != NULL)
        s += wxT("undefine PdXcorr\n");
    wxString xc_args, xc_vargs, xc_def;
    wxString xc_formulas[] = { wxT("p1/tan(x*pi/180)"),
                               wxT("p2/sin(x*pi/180)"),
                               wxT("p3/tan(x/2*pi/180)"),
                               wxT("p4*sin(x*pi/180)"),
                               wxT("p5*cos(x/2*pi/180)"),
                               wxT("p6") };
    for (int i = 0; i != (int) corr_ctrl.size(); ++i) {
        if (corr_ctrl[i]->is_nonzero()) {
            if (!xc_args.empty()) {
                xc_args += wxT(", ");
                xc_vargs += wxT(", ");
                xc_def += wxT(" + ");
            }
            xc_args += wxString::Format(wxT("p%d"), i+1);
            xc_vargs += wxString::Format(wxT("$pd_p%d"), i+1);
            xc_def += xc_formulas[i];
            s += wxString::Format(wxT("$pd_p%d = %s\n"), i+1,
                                  get_var(corr_ctrl[i]).c_str());
        }
    }
    if (!xc_args.empty()) {
        if (xc_args == wxT("p6")) { // special case, we don't need "define"
            s += wxT("%pd_xcorr = Constant($pd_p6)\n");
        } else {
            s += wxT("define PdXcorr(") + xc_args + wxT(") = ") + xc_def
                + wxT("\n");
            s += wxT("%pd_xcorr = PdXcorr(") + xc_vargs + wxT(")\n");
        }
        s += ds_pref + wxT("Z = %pd_xcorr\n");
    }

    bool has_u = par_u->is_nonzero();
    bool has_v = par_v->is_nonzero();
    bool has_w = par_w->is_nonzero();
    bool has_z = par_z->is_nonzero();
    double u_val = par_u->get_value();
    double v_val = par_v->get_value();
    double w_val = par_w->get_value();
    double z_val = par_z->get_value();
    s += wxT("\n");

    int width_sel = width_rb->GetSelection();
    if (width_sel == 1 /*H2=Utan2T...*/) {
        if (has_u)
            s += wxT("$pd_u = ") + get_var(par_u) + wxT("\n");
        if (has_v)
            s += wxT("$pd_v = ") + get_var(par_v) + wxT("\n");
        if (has_w)
            s += wxT("$pd_w = ") + get_var(par_w) + wxT("\n");
        if (has_z)
            s += wxT("$pd_z = ") + get_var(par_z) + wxT("\n");
    }

    bool has_a = par_a->IsEnabled() && par_a->is_nonzero();
    bool has_b = par_b->IsEnabled() && par_b->is_nonzero();
    bool has_c = par_c->IsEnabled() && par_c->is_nonzero();
    double a_val = par_a->get_value();
    if (has_a)
        s += wxT("$pd_a = ") + get_var(par_a) + wxT("\n");
    if (has_b)
        s += wxT("$pd_b = ") + get_var(par_b) + wxT("\n");
    if (has_c)
        s += wxT("$pd_c = ") + get_var(par_c) + wxT("\n");

    for (int i = 0; i < (int) sample_nb->GetPageCount() - 1; ++i) {
        const PhasePanel* p = get_phase_panel(i);
        const Crystal& cr = p->get_crystal();

        double max_intensity = get_max_intensity(cr.bp);
        double h_mult = y_max / (max_intensity > 0 ? max_intensity : 2);

        s += wxT("\n# ") + p->name_tc->GetValue() + wxT("\n");
        wxString pre = wxString::Format(wxT("$pd%d_"), i);
        s += pre + wxT("a = ") + get_var(p->par_a) + wxT("\n");
        if (p->par_b->IsEnabled())
            s += pre + wxT("b = ") + get_var(p->par_b) + wxT("\n");
        if (p->par_c->IsEnabled())
            s += pre + wxT("c = ") + get_var(p->par_c) + wxT("\n");
        double d2r = M_PI/180.;
        if (p->par_alpha->IsEnabled())
            s += pre + wxT("alpha = ") + get_var(p->par_alpha, d2r) + wxT("\n");
        if (p->par_beta->IsEnabled())
            s += pre + wxT("beta = ") + get_var(p->par_beta, d2r) + wxT("\n");
        if (p->par_gamma->IsEnabled())
            s += pre + wxT("gamma = ") + get_var(p->par_gamma, d2r) + wxT("\n");

        if (cr.xs() == TriclinicSystem) {
            wxString v_str = wxT(":a*:b*:c *")
                 wxT(" sqrt(1 - cos(:alpha)^2 - cos(:beta)^2 - cos(:gamma)^2")
                 wxT(" + 2*cos(:alpha)*cos(:beta)*cos(:gamma))\n");
            v_str.Replace(wxT(":"), pre);
            s += pre + wxT("volume = ") + v_str;
        }

        bool has_shape = peak_rb->GetSelection() >= 2;
        int shape_sel = shape_rb->GetSelection();

        v_foreach (PlanesWithSameD, j, cr.bp) {
            if (!j->enabled)
                continue;
            const Miller& hkl = j->planes[0];
            wxString hkl_str = hkl2wxstr(hkl);
            wxString hvar = pre + wxT("h") + hkl_str;
            double h = h_mult * max(j->intensity, 1.);
            double lambda1 = get_lambda(0);
            double ctr = 180 / M_PI * 2 * asin(lambda1 / (2 * j->d));
            s += wxT("\n") + hvar + wxString::Format(wxT(" = ~%.5g\n"), h);

            // we need to pre-define $pdXa_cHKL if wvar or svar depend on it
            wxString cvar_a = wxString::Format(wxT("$pd%da_c%s"),
                                               i, hkl_str.c_str());
            wxString theta_rad = cvar_a + wxT("*pi/360");
            if (width_sel == 1 || (has_shape && shape_sel >= 1)) {
                s += cvar_a + wxT(" = 0 # will be changed\n");
            }

            wxString wvar = pre + wxT("w") + hkl_str;
            if (width_sel == 0 /*independent*/) {
                double t = tan(ctr/2);
                double c = cos(ctr/2);
                double hwhm = sqrt(u_val*t*t + v_val*t + w_val + z_val/(c*c))/2;
                s += wvar + wxString::Format(wxT(" = ~%.5g\n"), hwhm);
            } else if (width_sel == 1 /*H2=Utan2T...*/) {
                s +=  wvar + wxT(" = sqrt(");
                if (has_u)
                    s += wxT("$pd_u*tan(") + theta_rad + wxT(")^2");
                if (has_v) {
                    if (has_u)
                        s += wxT(" + ");
                    s += wxT("$pd_v*tan(") + theta_rad + wxT(")");
                }
                if (has_w) {
                    if (has_u || has_v)
                        s += wxT(" + ");
                    s += wxT("$pd_w");
                }
                if (has_z) {
                    if (has_u || has_v || has_w)
                        s += wxT(" + ");
                    s += wxT("$pd_z/cos(") + theta_rad + wxT(")^2");
                }
                s += wxT(")\n");
            }

            wxString svar = pre + wxT("s") + hkl_str;
            if (shape_sel == 0 /*independent*/) {
                s += svar + wxString::Format(wxT(" = ~%.5g\n"), a_val);
            } else if (shape_sel == 1 || shape_sel == 2) {
                s += svar + wxT(" = ");
                if (has_a)
                    s += wxT("$pd_a");
                if (has_b) {
                    if (has_a)
                        s += wxT(" + ");
                    s += (shape_sel == 1 ? wxT("$pd_b*") : wxT("$pd_b/"))
                         + cvar_a;
                }
                if (has_c) {
                    if (has_a || has_b)
                        s += wxT(" + ");
                    s += (shape_sel == 1 ? wxT("$pd_c*") : wxT("$pd_c/("))
                         + cvar_a + wxT("*") + cvar_a +
                         (shape_sel == 1 ? wxT("") : wxT(")"));
                }
                s += wxT("\n");
            }

            for (char wave = 'a'; wave != lambda_symbol; ++wave) {
                wxString cvar = wxString::Format(wxT("$pd%d%c_c%s"),
                                                 i, wave, hkl_str.c_str());
                wxString rd_str; // d^-1
                switch (cr.xs()) {
                    case CubicSystem: // use only a
                        rd_str.Printf(wxT("sqrt(%d)/:a"),
                                hkl.h*hkl.h + hkl.k*hkl.k + hkl.l*hkl.l);
                        break;
                    case TetragonalSystem: // use a and c
                        rd_str.Printf(wxT("sqrt(%d/:a^2 + %d/:c^2)"),
                                  hkl.h*hkl.h + hkl.k*hkl.k, hkl.l*hkl.l);
                        break;
                    //TODO rhombohedral basis are not supported
                    case TrigonalSystem:
                    case HexagonalSystem:
                        rd_str.Printf(wxT("sqrt(%g/:a^2 + %d/:c^2)"),
                          4./3.*(hkl.h*hkl.h + hkl.h*hkl.k + hkl.k*hkl.k),
                          hkl.l*hkl.l);
                        break;
                    case OrthorhombicSystem: // use a, b, c
                        rd_str.Printf(
                       wxT("sqrt(%d/:a^2 + %d/:b^2 + %d/:c^2)"),
                            hkl.h*hkl.h, hkl.k*hkl.k, hkl.l*hkl.l);
                        break;
                    case MonoclinicSystem:
                        rd_str.Printf(
                       wxT("(1/sin(:beta) * ")
                       wxT("sqrt(%d/:a^2 + %d*sin(:beta)^2/:b^2 + %d/:c^2 ")
                           wxT("- %d*cos(:beta)/(:a*:c)))"),
                            hkl.h*hkl.h, hkl.k*hkl.k, hkl.l*hkl.l,
                            2*hkl.h*hkl.l);
                        break;
                    case TriclinicSystem:
                        rd_str.Printf(
                wxT("(sqrt(")
                wxT(" (%d*:b*:c*sin(:alpha))^2 + ")
                wxT(" (%d*:a*:c*sin(:beta))^2 + ")
                wxT(" (%d*:a*:b*sin(:gamma))^2 + ")
                wxT("%d*:a*:b*:c^2*(cos(:alpha)*cos(:beta) - cos(:gamma)) + ")
                wxT("%d*:a^2*:b*:c*(cos(:beta)*cos(:gamma) - cos(:alpha)) + ")
                wxT("%d*:a*:b^2*:c*(cos(:alpha)*cos(:gamma) - cos(:beta)) ")
                wxT(") / :volume)"),
                            hkl.h, hkl.k, hkl.l,
                            2*hkl.h*hkl.k, 2*hkl.k*hkl.l, 2*hkl.h*hkl.l);
                        break;
                    case UndefinedSystem:
                        rd_str = wxT(" # undefined system # ");
                        break;
                }
                rd_str.Replace(wxT(":"), wxString::Format(wxT("$pd%d_"), i));
                s += cvar;
                if (xaxis_val == 0) // 2T
                    s += wxString::Format(
                               wxT(" = 360/pi * asin($pd_lambda_%c/2 * %s)\n"),
                               wave, rd_str.c_str());
                else if (xaxis_val == 1) // Q
                    s += wxString::Format(wxT(" = 2*pi*%s\n"), rd_str.c_str());
                else if (xaxis_val == 2) { // d
                    if (cr.xs() == CubicSystem) // we can simplify this one
                        s += wxString::Format(wxT(" = $pd%d_a/sqrt(%d)\n"),
                                   i, hkl.h*hkl.h + hkl.k*hkl.k + hkl.l*hkl.l);
                    else
                        s += wxString::Format(wxT(" = 1/%s\n"), rd_str.c_str());
                }
                wxString fname = wxString::Format(wxT("%%pd%d%c_%s"),
                                                  i, wave, hkl_str.c_str());
                s += fname + wxT(" = ") + get_peak_name();
                // all functions have height and center
                wxString height = hvar;
                if (wave != 'a')
                    height += wxString::Format(wxT("*$pd_intens_%c"), wave);
                s += wxString::Format(wxT("(%s, %s, "),
                                      height.c_str(), cvar.c_str());
                if (split_cb->GetValue()) { // split function
                    s += wvar + wxT(", ") + wvar;
                    if (has_shape)
                        s += wxT(", ") + svar + wxT(", ") + svar;
                } else { // normal (not split) function
                    s += wvar;
                    if (has_shape)
                        s += wxT(", ") + svar;
                }
                s += wxT(")\n");
                s += ds_pref + wxT("F += ") + fname + wxT("\n");
            }
        }
    }

    return s;
}

static
void var2lockctrl(const string& varname, LockableRealCtrl* ctrl, double mult=1.)
{
    int k = ftk->mgr.find_variable_nr(varname);
    if (k == -1)
        return;
    const fityk::Variable* v = ftk->mgr.get_variable(k);
    ctrl->set_value(v->value() * mult);
    ctrl->set_lock(v->is_constant());
}

// this function does the opposite to prepare_commands():
// fills the form using assigned previously $pd* and %pd functions 
void PowderBook::fill_forms()
{
    // instrument page
    for (size_t i = 0; i != lambda_ctrl.size(); ++i) {
        char wave = 'a' + i;
        var2lockctrl("pd_lambda_" + S(wave), lambda_ctrl[i]);
        var2lockctrl("pd_intens_" + S(wave), intensity_ctrl[i], 100);
    }
    // corrections
    for (size_t i = 0; i != corr_ctrl.size(); ++i) {
        var2lockctrl("pd_" + S(i+1), corr_ctrl[i]);
    }

    int first_func = -1;
    // sample page
    for (size_t i = 0; ; ++i) {
        string pre = "pd" + S(i) + "_";
        int k = ftk->mgr.find_variable_nr(pre + "a");
        if (k == -1)
            break;
        assert(i == sample_nb->GetPageCount() - 1);
        PhasePanel* p = get_phase_panel(i);
        var2lockctrl(pre+"a", p->par_a);
        var2lockctrl(pre+"b", p->par_b);
        var2lockctrl(pre+"c", p->par_c);
        var2lockctrl(pre+"alpha", p->par_alpha);
        var2lockctrl(pre+"beta", p->par_beta);
        var2lockctrl(pre+"gamma", p->par_gamma);

        if (i < phase_desc.size()) {
            p->name_tc->ChangeValue(phase_desc[i].name);
            update_phase_labels(p, i);
            p->sg_tc->ChangeValue(phase_desc[i].sg);
            wxCommandEvent dummy;
            p->OnSpaceGroupChanged(dummy);
            p->atoms_tc->SetValue(phase_desc[i].atoms);
        }
        Crystal& cr = p->get_crystal();
        assert(cr.bp.size() == p->hkl_list->GetCount());
        int n = 0;
        vm_foreach (PlanesWithSameD, j, cr.bp) {
            wxString fname = wxString::Format(wxT("pd%da_"), (int) i)
                             + hkl2wxstr(j->planes[0]);
            int nr = ftk->mgr.find_function_nr(wx2s(fname));
            j->enabled = (nr != -1);
            p->hkl_list->Check(n, j->enabled);
            ++n;
            if (j->enabled && first_func == -1)
                first_func = nr;
        }
    }

    // peak page
    if (first_func != -1) {
        const fityk::Function *f = ftk->mgr.get_function(first_func);
        set_peak_name(f->tp()->name);
        int w = index_of_element(f->tp()->fargs, "hwhm");
        if (w == -1)
            w = index_of_element(f->tp()->fargs, "hwhm1");
        if (w != -1) {
            int idx = f->used_vars().get_idx(w);
            const fityk::Variable* hwhm = ftk->mgr.get_variable(idx);
            if (!hwhm->is_simple() || hwhm->name == "pd_w")
                width_rb->SetSelection(1);
        }
        int sh = index_of_element(f->tp()->fargs, "shape");
        if (sh == -1)
            sh = index_of_element(f->tp()->fargs, "shape1");
        if (sh != -1) {
            int idx = f->used_vars().get_idx(sh);
            const fityk::Variable* shape = ftk->mgr.get_variable(idx);
            if (!shape->is_simple() || shape->name == "pd_a") {
                string formula = shape->get_formula(ftk->mgr.parameters());
                bool has_div = contains_element(formula, '/');
                shape_rb->SetSelection(has_div ? 2 : 1);
            }
        }
    }
    var2lockctrl("pd_u", par_u);
    var2lockctrl("pd_v", par_v);
    var2lockctrl("pd_w", par_w);
    var2lockctrl("pd_z", par_z);
    var2lockctrl("pd_a", par_a);
    var2lockctrl("pd_b", par_b);
    var2lockctrl("pd_c", par_c);
}
#endif //!STANDALONE_POWDIFPAT

wxPanel* PowderBook::PrepareSizeStrainPanel()
{
    wxPanel *panel = new wxPanel(this);
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    wxTextCtrl *txt = new wxTextCtrl(panel, -1, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE);
    txt->SetBackgroundColour(GetBackgroundColour());
    txt->ChangeValue(wxT("It should be easy to put here simple size-strain analysis, like Williamson-Hall. But first we'd like to handle somehow instrumental broadening."));
    sizer->Add(txt, wxSizerFlags(1).Expand().Border());

    panel->SetSizerAndFit(sizer);

    return panel;
}

#if STANDALONE_POWDIFPAT
void PowderBook::OnFilePicked(wxFileDirPickerEvent& event)
{
    set_file(event.GetPath());
}

void PowderBook::set_file(wxString const& path)
{
    try {
        delete data;
        data = xylib::load_file(wx2s(path));
        xylib::Block const *block = data->get_block(0);
        xylib::Column const& xcol = block->get_column(1);
        xylib::Column const& ycol = block->get_column(2);
        x_min = xcol.get_min();
        x_max = xcol.get_max(block->get_point_count());
        y_max = ycol.get_max(block->get_point_count());
        range_from->set(x_min);
        range_to->set(x_max);
        for (size_t i = 0; i < sample_nb->GetPageCount(); ++i)
            get_phase_panel(i)->sample_plot_->refresh();
    } catch (runtime_error const& e) {
        data = NULL;
        wxMessageBox(wxT("Can not load file:\n") + s2wx(e.what()),
                     wxT("Error"), wxICON_ERROR);
    }
}
#endif

void PowderBook::OnXAxisSelected(wxCommandEvent&)
{
    bool use_wavelength = (xaxis_rb->GetSelection() == 0);
    wave_panel->Enable(use_wavelength);
}

void PowderBook::OnAnodeSelected(wxCommandEvent& event)
{
    int n = event.GetSelection();
    if (n < 0)
        return;

    // clear controls
    for (size_t i = 0; i < lambda_ctrl.size(); ++i)
        lambda_ctrl[i]->Clear();
    for (size_t i = 1; i < intensity_ctrl.size(); ++i)
        intensity_ctrl[i]->Clear();

    // set new value
    wxString s = anode_lb->GetString(n);
    for (Anode const* i = anodes; i->name; ++i) {
        if (s.StartsWith(pchar2wx(i->name))) {
            double a1 = 0, a2 = 0;
            if (s.EndsWith(wxT("A12"))) {
                a1 = i->alpha1;
                a2 = i->alpha2;
            } else if (s.EndsWith(wxT("A1"))) {
                a1 = i->alpha1;
            } else
                a1 = (2 * i->alpha1 + i->alpha2) / 3.;
            lambda_ctrl[0]->set_value(a1);
            if (a2 != 0) {
                lambda_ctrl[1]->set_value(a2);
                intensity_ctrl[1]->set_value(50);
            }
        }
    }
}

void PowderBook::OnQuickPhaseSelected(wxCommandEvent& event)
{
    int n = event.GetSelection();
    if (n < 0)
        return;
    string name = wx2s(saved_phase_lb->GetString(n));
    CelFile const& cel = quick_phase_list[name];
    PhasePanel *panel = get_current_phase_panel();
    assert(panel->GetParent() == sample_nb);
    panel->set_phase(name, cel);
    panel->save_btn->Enable(false);
    panel->sample_plot_->refresh();
}

void PowderBook::OnQuickListRemove(wxCommandEvent&)
{
    int n = saved_phase_lb->GetSelection();
    if (n < 0)
        return;

    string name = wx2s(saved_phase_lb->GetString(n));
    assert(saved_phase_lb->GetCount() == quick_phase_list.size());

    saved_phase_lb->SetSelection(wxNOT_FOUND);
    saved_phase_lb->Delete(n);
    quick_phase_list.erase(name);
}

void PowderBook::OnQuickListImport(wxCommandEvent&)
{
    wxFileDialog dlg (this, wxT("Import CIF or CEL files"), wxT(""), wxT(""),
                      wxT("all supported files|*.cif;*.CIF;*.cel;*.CEL|")
                      wxT("CIF files (*.cif)|*.cif;*.CIF|")
                      wxT("PowderCell files (*.cel)|*.cel;*.CEL"),
                      wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK)
        return;
    wxArrayString paths;
    dlg.GetPaths(paths);
    for (size_t i = 0; i != paths.GetCount(); ++i) {
        wxFileName fn(paths[i]);
        wxString name = fn.GetName();
        wxString ext = fn.GetExt();
        if (saved_phase_lb->FindString(name) != wxNOT_FOUND) {
            int answer = wxMessageBox(wxT("Name `") + name
                    + wxT("' already exists.\nOverwrite?"),
                    wxT("Overwrite?"), wxYES_NO|wxICON_QUESTION);
            if (answer != wxYES)
                continue;;
        }
        if (ext == wxT("cif") || ext == wxT("CIF")) {
            wxMessageBox(wxT("Support for CIF files is not ready yet."),
                         wxT("Sorry."), wxOK|wxICON_ERROR);
            continue;
        } else {
            FILE *f = wxFopen(paths[i], wxT("r"));
            if (f) {
                CelFile cel = read_cel_file(f);
                fclose(f);
                if (cel.sgs != NULL)
                    quick_phase_list[wx2s(name)] = cel;
            }
        }
        saved_phase_lb->Append(name);
    }
}

void PowderBook::OnLambdaChange(wxCommandEvent&)
{
    anode_lb->SetSelection(wxNOT_FOUND);
}

void PowderBook::OnPageChanged(wxBookCtrlEvent& event)
{
    if (event.GetSelection() == 2) { // sample
        for (size_t i = 0; i != sample_nb->GetPageCount(); ++i) {
            PhasePanel* p = get_phase_panel(i);
            p->update_miller_indices(false);
            p->sample_plot_->refresh();
        }
    } else if (event.GetSelection() == 4) { // action
#if !STANDALONE_POWDIFPAT
        action_txt->SetValue(prepare_commands());
#endif
    }
}

PhasePanel *PowderBook::get_phase_panel(int n)
{
    return static_cast<PhasePanel*>(sample_nb->GetPage(n));
}

PhasePanel *PowderBook::get_current_phase_panel()
{
    return static_cast<PhasePanel*>(sample_nb->GetCurrentPage());
}

void PowderBook::deselect_phase_quick_list()
{
    saved_phase_lb->SetSelection(wxNOT_FOUND);
    PhasePanel *panel = get_current_phase_panel();
    bool valid = !panel->name_tc->GetValue().empty();
    panel->save_btn->Enable(valid);
}

RadiationType PowderBook::get_radiation_type() const
{
    if (radiation_rb->GetSelection() == 0)
        return kXRay;
    else
        return kNeutron;
}

double PowderBook::get_lambda(int n) const
{
    if (n < 0 || n >= (int) lambda_ctrl.size())
        return 0;
    if (xaxis_rb->GetSelection() != 0)
        return 0.;
    return lambda_ctrl[n]->get_value();
}

double PowderBook::d2x(double d) const
{
    int xaxis_val = xaxis_rb->GetSelection();
    if (xaxis_val == 0) { // 2T
        double lambda0 = lambda_ctrl[0]->get_value();
        return 180 / M_PI * 2 * asin(lambda0 / (2 * d));
    } else if (xaxis_val == 1) // Q
        return 2 * M_PI / d;
    else if (xaxis_val == 2) // d
        return d;
    return 0.;
}

bool PowderBook::is_d_active(double d) const
{
#if !STANDALONE_POWDIFPAT
    double x = d2x(d);
    vector<fityk::Point>::const_iterator point = data->get_point_at(x);
    return point != data->points().end() && point->is_active;
#else
    return true;
#endif
}

double PowderBook::get_min_d() const
{
    int xaxis_val = xaxis_rb->GetSelection();
    if (xaxis_val == 0) // 2T
        return get_lambda(0) / (2 * sin(x_max * M_PI / 180 / 2.));
    else if (xaxis_val == 1) // Q
        return 2 * M_PI / x_max;
    else if (xaxis_val == 2) // d
        return x_min;
    return 0;
}

void PowderBook::OnPeakRadio(wxCommandEvent& event)
{
    // enable/disable peak and shape radiobuttons
    int sel = event.GetSelection();
    bool has_shape = (sel > 1); // not Gaussian/Lorentzian
    shape_rb->Enable(has_shape);
    update_peak_parameters();
}

void PowderBook::OnPeakSplit(wxCommandEvent& event)
{
    // Split-* functions don't have width/shape set as f(2T) now.
    // This could be implemented (two sets of parameters - for left and right).
    bool is_split = event.IsChecked();
    if (is_split && width_rb->GetSelection() == 1 /*f(2T)*/)
        width_rb->SetSelection(0);
    width_rb->Enable(1, !is_split);
    if (shape_rb->IsEnabled()) {
        if (is_split && shape_rb->GetSelection() >= 1 /*f(2T)*/)
            shape_rb->SetSelection(0);
        shape_rb->Enable(1, !is_split);
        shape_rb->Enable(2, !is_split);
    }
}

void PowderBook::OnWidthRadio(wxCommandEvent&)
{
    update_peak_parameters();
}

void PowderBook::OnShapeRadio(wxCommandEvent&)
{
    update_peak_parameters();
}

void PowderBook::update_peak_parameters()
{
    bool has_shape = shape_rb->IsEnabled();
    par_a->Enable(has_shape);
    par_b->Enable(has_shape);
    par_c->Enable(has_shape);
}

// update wxNoteBook page labels: set the current page name to be the same
// as the name in wxTextCtrl, make sure there is always one empty page.
void PowderBook::update_phase_labels(PhasePanel* p, int active)
{
    wxString name = p->name_tc->GetValue();
    bool valid = !name.empty();
    int last = sample_nb->GetPageCount() - 1;
    if (active == -1)
        active = sample_nb->GetSelection();
    wxString empty_label = wxT("+");
    bool last_empty = (sample_nb->GetPageText(last) == empty_label);

    sample_nb->SetPageText(active, valid ? name : empty_label);

    if (valid) {
        bool has_empty = false;
        for (size_t i = 0; i < sample_nb->GetPageCount(); ++i)
            if (sample_nb->GetPageText(last) == empty_label) {
                has_empty = true;
                break;
            }
        if (!has_empty) {
            PhasePanel *page = new PhasePanel(sample_nb, this);
            sample_nb->AddPage(page, wxT("+"));
        }
    } else if (active != last && last_empty) {
        sample_nb->DeletePage(last);
    }
}

#if !STANDALONE_POWDIFPAT
void PowderBook::OnDelButton(wxCommandEvent&)
{
    exec("delete %pd*, $pd*");
}

void PowderBook::OnOk(wxCommandEvent&)
{
    wxString script = action_txt->GetValue();
    ftk->ui()->exec_string_as_script(script.mb_str());
    save_phase_desc();
    wxDialog* dialog = static_cast<wxDialog*>(GetParent());
    dialog->EndModal(wxID_OK);
    // unlike exec(), exec_string_as_script() does not call after_cmd_updates()
    frame->after_cmd_updates();
}
#endif

void PowderBook::save_phase_desc()
{
    xaxis_sel = xaxis_rb->GetSelection();

    int n = (int) sample_nb->GetPageCount() - 1;
    phase_desc.resize(n);
    for (int i = 0; i < n; ++i) {
        const PhasePanel* p = get_phase_panel(i);
        phase_desc[i].name = p->name_tc->GetValue();
        phase_desc[i].sg = p->sg_tc->GetValue();
        phase_desc[i].atoms = p->atoms_tc->GetValue();
    }
}

#if STANDALONE_POWDIFPAT

#include <wx/aboutdlg.h>
#include "img/powdifpat16.xpm"
#include "img/powdifpat48.xpm"

class App : public wxApp
{
public:
    wxString version;

    bool OnInit();
    void OnAbout(wxCommandEvent&);
    void OnClose(wxCommandEvent&) { GetTopWindow()->Close(); }
};

IMPLEMENT_APP(App)


static const wxCmdLineEntryDesc cmdLineDesc[] = {
#if wxCHECK_VERSION(2, 9, 0)
    { wxCMD_LINE_SWITCH, "V", "version",
          "output version information and exit", wxCMD_LINE_VAL_NONE, 0 },
    { wxCMD_LINE_PARAM,  0, 0, "data file", wxCMD_LINE_VAL_STRING,
                                            wxCMD_LINE_PARAM_OPTIONAL },
#else
    { wxCMD_LINE_SWITCH, wxT("V"), wxT("version"),
          wxT("output version information and exit"), wxCMD_LINE_VAL_NONE, 0 },
    { wxCMD_LINE_PARAM,  0, 0, wxT("data file"), wxCMD_LINE_VAL_STRING,
                                            wxCMD_LINE_PARAM_OPTIONAL },
#endif
    { wxCMD_LINE_NONE, 0, 0, 0,  wxCMD_LINE_VAL_NONE, 0 }
};


bool App::OnInit()
{
    version = wxT("0.1.0");

    // write numbers in C locale
    setlocale(LC_NUMERIC, "C");

    SetAppName(wxT("powdifpat"));

    // parse command line parameters
    wxCmdLineParser cmdLineParser(cmdLineDesc, argc, argv);
    if (cmdLineParser.Parse(false) != 0) {
        cmdLineParser.Usage();
        return false;
    }
    if (cmdLineParser.Found(wxT("V"))) {
        wxMessageOutput::Get()->Printf(wxT("powdifpat ") + version + wxT("\n"));
        return false;
    }

    wxImage::AddHandler(new wxPNGHandler);

    wxFrame *frame = new wxFrame(NULL, wxID_ANY, GetAppName());

#ifdef __WXMSW__
    frame->SetIcon(wxIcon(wxT("powdifpat"))); // load from a resource
#else
    wxIconBundle ib;
    ib.AddIcon(wxIcon(powdifpat48_xpm));
    ib.AddIcon(wxIcon(powdifpat16_xpm));
    frame->SetIcons(ib);
#endif

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    PowderBook *pb = new PowderBook(frame, wxID_ANY);
    sizer->Add(pb, wxSizerFlags(1).Expand());

    wxBoxSizer *btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *about = new wxButton(frame, wxID_ABOUT);
    wxButton *close = new wxButton(frame, wxID_CLOSE);
    btn_sizer->Add(about, wxSizerFlags().Border());
    btn_sizer->AddStretchSpacer();
    btn_sizer->Add(close, wxSizerFlags().Border());
    sizer->Add(btn_sizer, wxSizerFlags().Expand().Border());

    frame->SetSizerAndFit(sizer);

    // wxMSW bug workaround
    frame->SetBackgroundColour(pb->GetBackgroundColour());

    if (cmdLineParser.GetParamCount() == 1) {
        wxString path = cmdLineParser.GetParam(0);
        pb->set_file(path);
        pb->file_picker->SetPath(path);
    }

    frame->Show();

    Connect(about->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(App::OnAbout));
    Connect(close->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(App::OnClose));
    return true;
}


void App::OnAbout(wxCommandEvent&)
{
    wxAboutDialogInfo adi;
    adi.SetVersion(version);
    wxString desc = wxT("Powder diffraction pattern generator.\n");
    adi.SetDescription(desc);
    adi.SetWebSite(wxT("http://www.unipress.waw.pl/fityk/powdifpat/"));
    adi.SetCopyright(wxT("(C) 2008 - 2011 Marcin Wojdyr <wojdyr@gmail.com>"));
    wxAboutBox(adi);
}

#else

PowderDiffractionDlg::PowderDiffractionDlg(wxWindow* parent, wxWindowID id)
     : wxDialog(parent, id, wxT("powder diffraction analysis"),
                wxDefaultPosition, wxDefaultSize,
                wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    PowderBook *pb = new PowderBook(this, wxID_ANY);
    sizer->Add(pb, wxSizerFlags(1).Expand());
    SetSizerAndFit(sizer);
    if (GetClientSize().GetHeight() < 440)
#ifdef __WXMAC__
        SetClientSize(-1, 520);
#else
        SetClientSize(-1, 440);
#endif
}

#endif

