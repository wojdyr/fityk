// Author: Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

//TODO:
// phases quick-list:
//  - read/save from/to file,
//  - the built-in list should also have atoms
// ceria+atomtables+sglite=libceria
// replace UTF8 chars with hex
// peaks: peak formula, widths, shapes
// make a (preview) release
//
// enable x-ray / neutron switching (different LPF and scattering factors)
// buffer plot with data
// import .cel and .cif files
// action: info about old and new models, button to add/update the model

#include <cmath>
#include <cstring>
#include <algorithm>
#include <iostream>

#include <wx/wx.h>

#include <wx/imaglist.h>
#include <wx/cmdline.h>
#include <wx/listctrl.h>
#include <wx/stdpaths.h>


#include "powdifpat.h"
#include "atomtables.h"
#include "../common.h"
#include "cmn.h"
#include "uplot.h" // BufferedPanel, scale_tics_step()
#include "fancyrc.h" // LockableRealCtrl 
#include "ceria.h"

#if !STANDALONE_POWDIFPAT
#include "frame.h"
#include "../logic.h"
#include "../data.h"
#endif

using namespace std;


class StaticBitmap : public wxControl
{
public:
    StaticBitmap(wxWindow *parent, wxBitmap const& bitmap_)
        : wxControl(parent, -1, wxDefaultPosition, wxDefaultSize,
                    wxBORDER_NONE),
          bitmap(bitmap_)
    {
        InheritAttributes();
        SetInitialSize(wxSize(bitmap.GetWidth(), bitmap.GetHeight()));
        Connect(GetId(), wxEVT_ERASE_BACKGROUND,
                (wxObjectEventFunction) &StaticBitmap::OnEraseBackground);
        Connect(GetId(), wxEVT_PAINT,
                (wxObjectEventFunction) &StaticBitmap::OnPaint);
    }

    bool ShouldInheritColours() { return true; }

    bool AcceptsFocus() { return false; }

    wxSize DoGetBestSize() const
           { return wxSize(bitmap.GetWidth(), bitmap.GetHeight()); }

    void OnEraseBackground(wxEraseEvent&) {}

    void OnPaint(wxPaintEvent&)
    {
        wxPaintDC dc(this);
        //DoPrepareDC(dc);
        PrepareDC(dc);
        dc.DrawBitmap(bitmap, 0, 0, true);
    }

private:
    wxBitmap bitmap;
};



class SpaceGroupChooser : public wxDialog
{
public:
    SpaceGroupChooser(wxWindow* parent);
    void OnCheckBox(wxCommandEvent&) { regenerate_list(); }
    void OnSystemChoice(wxCommandEvent&) { regenerate_list(); }
    void OnListItemActivated(wxCommandEvent&) { EndModal(wxID_OK); }
    void regenerate_list();
    wxString get_value() const;
private:
    wxChoice *system_c;
    wxListView *list;
    wxCheckBox *centering_cb[7];
    DISALLOW_COPY_AND_ASSIGN(SpaceGroupChooser);
};

class PlotWithLines : public PlotWithTics
{
public:
    PlotWithLines(wxWindow* parent, PhasePanel *phase_panel,
                  PowderBook *powder_book);
    void draw(wxDC &dc, bool);

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
    void OnSpaceGroupChanged(wxCommandEvent& event);
    void OnSpaceGroupChanging(wxCommandEvent&)
                               { powder_book->deselect_phase_quick_list(); }
    void OnParameterChanged(wxCommandEvent& event);
    void OnLineToggled(wxCommandEvent& event);
    void OnLineSelected(wxCommandEvent& event);
    void OnAtomsFocus(wxFocusEvent&);
    void OnAtomsUnfocus(wxFocusEvent&);
    void OnAtomsChanged(wxCommandEvent& event);
    void set_phase(string const& name, CelFile const& cel);
    const Crystal& get_crystal() { return cr_; }
    bool editing_atoms() const { return editing_atoms_; }
    int get_selected_hkl() const { return hkl_list->GetSelection(); }

private:
    wxNotebook *nb_parent;
    PowderBook *powder_book;

    wxTextCtrl *name_tc, *sg_tc;
    LockableRealCtrl *par_a, *par_b, *par_c, *par_alpha, *par_beta, *par_gamma;
    wxButton *s_qadd_btn;
    wxCheckListBox *hkl_list;
    wxTextCtrl *atoms_tc, *info_tc;
    wxStaticText *sg_nr_st;
    bool atoms_show_help_;
    bool editing_atoms_;
    PlotWithLines *sample_plot;
    // line number with the first syntax error atoms_tc; -1 if correct
    int line_with_error_;

    Crystal cr_;

    void enable_parameter_fields();
    void update_disabled_parameters();
    void update_miller_indices();
    void set_ortho_angles();
    void change_space_group(string const& text);

    DISALLOW_COPY_AND_ASSIGN(PhasePanel);
};

namespace {

LockableRealCtrl *addMaybeRealCtrl(wxWindow *parent, wxString const& label,
                                   wxSizer *sizer, wxSizerFlags const& flags)
{
    wxStaticText *st = new wxStaticText(parent, -1, label);
    LockableRealCtrl *ctrl = new LockableRealCtrl(parent);
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(st, 0, wxALIGN_CENTER_VERTICAL);
    hsizer->Add(ctrl, 0);
    sizer->Add(hsizer, flags);
    return ctrl;
}

} // anonymous namespace


PowderBook::PowderBook(wxWindow* parent, wxWindowID id)
    : wxListbook(parent, id), x_min(10), x_max(150), y_max(1000), data(NULL)
{
#if !STANDALONE_POWDIFPAT
    int data_nr = frame->get_focused_data_index();
    if (data_nr >= 0) {
        data = ftk->get_data(data_nr);
        x_min = data->get_x_min();
        x_max = data->get_x_max();
        y_max = data->get_y_max();
    }
#endif
    initialize_quick_phase_list();

    wxImageList *image_list = new wxImageList(32, 32);
    image_list->Add(wxBitmap(wxImage("img/info32.png")));
    image_list->Add(wxBitmap(wxImage("img/radiation32.png")));
    image_list->Add(wxBitmap(wxImage("img/rubik32.png")));
    image_list->Add(wxBitmap(wxImage("img/peak32.png")));
    image_list->Add(wxBitmap(wxImage("img/run32.png")));
    AssignImageList(image_list);

    AddPage(PrepareIntroPanel(), wxT("intro"), false, 0);
    AddPage(PrepareInstrumentPanel(), wxT("instrument"), false, 1);
    AddPage(PrepareSamplePanel(), wxT("sample"), false, 2);
    AddPage(PreparePeakPanel(), wxT("peak"), false, 3);
    //AddPage(PrepareActionPanel(), wxT("action"), false, 4);
    AddPage(PrepareActionPanel(), wxEmptyString, false, 4);

    Connect(GetId(), wxEVT_COMMAND_LISTBOOK_PAGE_CHANGED,
            (wxObjectEventFunction) &PowderBook::OnPageChanged);
}

void PowderBook::initialize_quick_phase_list()
{
    wxString fityk_dir = wxStandardPaths::Get().GetUserDataDir();
#if STANDALONE_POWDIFPAT
    fityk_dir.Replace(wxT("powdifpat"), wxT("fityk"));
#endif
    wxString cel_dir = fityk_dir + wxFILE_SEP_PATH
                       + wxT("cel_files") + wxFILE_SEP_PATH;
    if (!wxDirExists(cel_dir)) {
        wxMkdir(cel_dir);
        write_default_cel_files(wx2s(cel_dir).c_str());
    }

    // TODO
    //for () {
    //    CelFile cel = read_cel_file();
    //    if (cel.sgs != NULL)
    //        quick_phase_list[name] = cel;
    //}
}


wxPanel* PowderBook::PrepareIntroPanel()
{
    static const char* intro_str =
#if STANDALONE_POWDIFPAT
    "This simple program can generate diffraction pattern using information\n"
    "about symmetry group, unit cell and radiation wavelength.\n"
    "At this moment the program is experimental and the old PowderCell\n"
    "is definitely more reliable.\n"
    "Actually this is stand-alone version of fityk's tool that prepares to\n"
    "Pawley method analysis of powder diffraction data.\n";
#else
    "Before you start, you should have:\n"
    "  - data (powder diffraction pattern) loaded,\n"
    "  - only interesting data range active,\n"
    "  - and baseline (background) either removed manually,\n"
    "    or modeled with e.g. polynomial.\n"
    "\n"
    "This window will help you to build a model for your data. "
    "The model has constrained position of peaks and "
    "not constrained intensities. Fitting this model (all variables "
    "at the same time) is "
    "known as Pawley's method. LeBail's method is similar, "
    "but the fitting procedure is more complex.\n"
    "\n"
    "This window will not analyze the final result for you. "
    "But there is another window that can help with size-strain analysis.\n"
    "\n"
    "Good luck!\n";
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
    legend->Add(new wxStaticBitmap(panel, -1, get_lock_xpm()),
                wxSizerFlags().Center().Right());
    legend->Add(new wxStaticText(panel, -1, wxT("constant parameter")),
                wxSizerFlags().Center().Left());
    legend->Add(new wxStaticBitmap(panel, -1, get_lock_open_xpm()),
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

    wxRadioBox *radiation_rb = new wxRadioBox(panel, -1, wxT("radiation"),
                                  wxDefaultPosition, wxDefaultSize,
                                  ArrayString(wxT("x-ray"), wxT("neutron")),
                                  1, wxRA_SPECIFY_ROWS);
    radiation_rb->Enable(1, false);
    hsizer->AddSpacer(50);
    hsizer->Add(radiation_rb, wxSizerFlags().Border());
    hsizer->AddStretchSpacer();

    wxArrayString xaxis_choices;
    xaxis_choices.Add(wxT("2θ")); //03B8
    xaxis_choices.Add(wxT("Q"));
    wxRadioBox *xaxis_rb = new wxRadioBox(panel, -1, wxT("data x axis"),
                                          wxDefaultPosition, wxDefaultSize,
                                          xaxis_choices, 3);
    xaxis_rb->Enable(1, false);
    hsizer->Add(xaxis_rb, wxSizerFlags().Border());
    hsizer->AddSpacer(50);
    sizer->Add(hsizer, wxSizerFlags().Expand());

    wxSizer *wave_sizer = new wxStaticBoxSizer(wxHORIZONTAL, panel,
                                               wxT("wavelengths"));
    anode_lb = new wxListBox(panel, -1);
    for (Anode const* i = anodes; i->name; ++i) {
        anode_lb->Append(pchar2wx(i->name));
        anode_lb->Append(pchar2wx(i->name) + wxT(" A1"));
        anode_lb->Append(pchar2wx(i->name) + wxT(" A12"));
    }
    wave_sizer->Add(anode_lb, wxSizerFlags(1).Border().Expand());

    wxSizer *lambda_sizer = new wxFlexGridSizer(2, 5, 5);
    lambda_sizer->Add(new wxStaticText(panel, -1, wxT("wavelength")),
                  wxSizerFlags().Center());
    lambda_sizer->Add(new wxStaticText(panel, -1, wxT("intensity")),
                  wxSizerFlags().Center());
    for (int i = 0; i < max_wavelengths; ++i) {
        LockableRealCtrl *lambda = new LockableRealCtrl(panel);
        lambda_sizer->Add(lambda);
        lambda_ctrl.push_back(lambda);

        LockableRealCtrl *intens = new LockableRealCtrl(panel, true);
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
    sizer->Add(wave_sizer, wxSizerFlags().Expand());

    wxSizer *corr_sizer = new wxStaticBoxSizer(wxVERTICAL, panel,
                                           wxT("corrections (use with care)"));
    wxBitmap formula(wxT("img/correction.png"), wxBITMAP_TYPE_PNG);
    corr_sizer->Add(new StaticBitmap(panel, formula), wxSizerFlags().Border());

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
                           (wxObjectEventFunction) &PowderBook::OnLambdaChange,
                           NULL, this);
    for (size_t i = 1; i < intensity_ctrl.size(); ++i)
        intensity_ctrl[i]->Connect(wxID_ANY, wxEVT_COMMAND_TEXT_UPDATED,
                           (wxObjectEventFunction) &PowderBook::OnLambdaChange,
                           NULL, this);
    Connect(anode_lb->GetId(), wxEVT_COMMAND_LISTBOX_SELECTED,
            (wxObjectEventFunction) &PowderBook::OnAnodeSelected);

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
    dc.SetPen(*wxRED_PEN);
    double lambda1 = powder_book_->get_lambda(0);
    if (lambda1 == 0.)
        return;
    const vector<PlanesWithSameD>& bp = phase_panel_->get_crystal().bp;
    int selected = phase_panel_->get_selected_hkl();
    double max_intensity = 0.;
    for (vector<PlanesWithSameD>::const_iterator i = bp.begin();
                                                        i != bp.end(); ++i)
        if (i->intensity > max_intensity)
            max_intensity = i->intensity;
    double y_scaling = 0;
    if (max_intensity > 0)
        y_scaling = powder_book_->y_max / max_intensity;
    for (vector<PlanesWithSameD>::const_iterator i = bp.begin();
                                                        i != bp.end(); ++i) {
        if (!i->enabled)
            continue;
        bool is_selected = (i - bp.begin() == selected);
        if (is_selected)
            //wxYELLOW_PEN and wxYELLOW were added in 2.9
            dc.SetPen(wxPen(wxColour(255, 255, 0)));
        double two_theta = 180 / M_PI * 2 * asin(lambda1 / (2 * i->d));
        int X = getX(two_theta);
        // draw short line at the bottom to mark position of the peak
        dc.DrawLine(X, Y0, X, (Yx+Y0)/2);
        if (i->intensity && !phase_panel_->editing_atoms()) {
            int Y1 = getY(y_scaling * i->intensity);
            // draw line that has height proportional to peak intensity
            dc.DrawLine(X, Yx, X, Y1);
        }
        // set it back to normal color
        if (is_selected)
            dc.SetPen(*wxRED_PEN);
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
    wxPoint *points = new wxPoint[n];
    for (int i = 0; i != n; ++i) {
        points[i].x = getX(powder_book_->data->get_x(i));
        points[i].y = getY(powder_book_->data->get_y(i));
    }
#endif // STANDALONE_POWDIFPAT
    dc.DrawLines(n, points);
    delete [] points;
}

PhasePanel::PhasePanel(wxNotebook *parent, PowderBook *powder_book_)
        : wxPanel(parent), nb_parent(parent), powder_book(powder_book_),
          line_with_error_(-1)
{
    wxSizer *vsizer = new wxBoxSizer(wxVERTICAL);

    wxSizer *h0sizer = new wxBoxSizer(wxHORIZONTAL);
    h0sizer->Add(new wxStaticText(this, -1, wxT("Name:")),
                 wxSizerFlags().Center().Border());
    name_tc = new KFTextCtrl(this, -1, wxEmptyString, 150);
    h0sizer->Add(name_tc, wxSizerFlags().Center().Border());

    wxBoxSizer *h2sizer = new wxBoxSizer(wxHORIZONTAL);
    s_qadd_btn = new wxButton(this, wxID_ADD, "Add to quick list");
    h2sizer->Add(s_qadd_btn, wxSizerFlags().Border());
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
    par_alpha = addMaybeRealCtrl(this, wxT("α ="),  par_sizer, flags);//03B1
    par_beta = addMaybeRealCtrl(this, wxT("β ="),  par_sizer, flags); //03B2
    par_gamma = addMaybeRealCtrl(this, wxT("γ ="),  par_sizer, flags);//03B3
    stp_sizer->Add(par_sizer, wxSizerFlags(1).Expand());
    vsizer->Add(stp_sizer, wxSizerFlags().Border().Expand());

    ProportionalSplitter *vsplit = new ProportionalSplitter(this, -1, 0.75);

    ProportionalSplitter *hkl_split = new ProportionalSplitter(vsplit,-1, 0.4);
    hkl_list = new wxCheckListBox(hkl_split, -1);
    sample_plot = new PlotWithLines(hkl_split, this, powder_book);

    ProportionalSplitter *atom_split = new ProportionalSplitter(vsplit,-1, 0.6);
    atoms_tc = new wxTextCtrl(atom_split, -1, wxEmptyString,
                          wxDefaultPosition, wxDefaultSize,
                          wxTE_RICH|wxTE_MULTILINE);
    atoms_tc->SetValue(wxT("Optional: atom positions, e.g.\n")
                    wxT("Si 0 0 0\n")
                    wxT("C 0.25 0.25 0.25"));
    atoms_show_help_ = true;
    atoms_tc->SetSelection(-1, -1);
    info_tc = new wxTextCtrl(atom_split, -1, wxEmptyString,
                             wxDefaultPosition, wxDefaultSize,
                             wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE);
    info_tc->SetBackgroundColour(powder_book->GetBackgroundColour());

    vsplit->SplitHorizontally(hkl_split, atom_split);
    hkl_split->SplitVertically(hkl_list, sample_plot);
    atom_split->SplitVertically(atoms_tc, info_tc);

    vsizer->Add(vsplit, wxSizerFlags(1).Expand().Border());
    SetSizerAndFit(vsizer);

    Connect(sg_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &PhasePanel::OnSpaceGroupButton);
    Connect(s_qadd_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &PhasePanel::OnAddToQLButton);
    Connect(s_clear_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &PhasePanel::OnClearButton);
    Connect(name_tc->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            (wxObjectEventFunction) &PhasePanel::OnNameChanged);
    Connect(sg_tc->GetId(), wxEVT_COMMAND_TEXT_ENTER,
            (wxObjectEventFunction) &PhasePanel::OnSpaceGroupChanged);
    Connect(sg_tc->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            (wxObjectEventFunction) &PhasePanel::OnSpaceGroupChanging);

    Connect(par_a->get_text_ctrl()->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            (wxObjectEventFunction) &PhasePanel::OnParameterChanged);
    Connect(par_b->get_text_ctrl()->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            (wxObjectEventFunction) &PhasePanel::OnParameterChanged);
    Connect(par_c->get_text_ctrl()->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            (wxObjectEventFunction) &PhasePanel::OnParameterChanged);
    Connect(par_alpha->get_text_ctrl()->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            (wxObjectEventFunction) &PhasePanel::OnParameterChanged);
    Connect(par_beta->get_text_ctrl()->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            (wxObjectEventFunction) &PhasePanel::OnParameterChanged);
    Connect(par_gamma->get_text_ctrl()->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            (wxObjectEventFunction) &PhasePanel::OnParameterChanged);

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
    SpaceGroupChooser dlg(this);
    if (dlg.ShowModal() == wxID_OK)
        sg_tc->SetValue(dlg.get_value());
    OnSpaceGroupChanged(event);
}

void PhasePanel::OnAddToQLButton(wxCommandEvent&)
{
    wxString name = name_tc->GetValue();
    if (name.find("|") != string::npos) {
        wxMessageBox(wxT("The pipe character '|' is not allowed in name."),
                     wxT("Error"), wxOK|wxICON_ERROR);
        return;
    }
    wxListBox *quick_phase_lb = powder_book->get_quick_phase_lb();
    if (quick_phase_lb->FindString(name) != wxNOT_FOUND) {
        int answer = wxMessageBox(wxT("Name `") + name
                                        + wxT("' already exists.\nOverwrite?"),
                                  wxT("Overwrite?"), wxYES_NO|wxICON_QUESTION);
        if (answer != wxYES)
            return;
    }
    else
        quick_phase_lb->Append(name);

    CelFile cel;
    cel.sgs = find_first_sg_with_number(cr_.sg_number); // TODO ...
    cel.a = par_a->get_value();
    cel.b = par_b->get_value();
    cel.c = par_c->get_value();
    cel.alpha = par_alpha->get_value();
    cel.beta = par_beta->get_value();
    cel.gamma = par_gamma->get_value();

    quick_phase_lb->SetStringSelection(name);
    powder_book->quick_phase_list[wx2s(name)] = cel;
}

void PhasePanel::OnClearButton(wxCommandEvent& event)
{
    size_t sel = nb_parent->GetSelection();
    if (sel < nb_parent->GetPageCount() - 1) {
        nb_parent->SetSelection(nb_parent->GetPageCount() - 1);
        nb_parent->DeletePage(sel);
    }
    else {
        name_tc->Clear();
        sg_tc->Clear();
        par_a->Clear();
        par_b->Clear();
        par_c->Clear();
        par_alpha->Clear();
        par_beta->Clear();
        par_gamma->Clear();
        OnSpaceGroupChanged(event);
    }
}

void PhasePanel::OnNameChanged(wxCommandEvent&)
{
    wxString name = name_tc->GetValue();
    bool valid = !name.empty();
    int last = nb_parent->GetPageCount() - 1;
    int active = nb_parent->GetSelection();
    wxString empty_label = wxT("+");
    bool last_empty = (nb_parent->GetPageText(last) == empty_label);

    s_qadd_btn->Enable(valid);
    nb_parent->SetPageText(active, valid ? name : empty_label);

    if (valid) {
        bool has_empty = false;
        for (size_t i = 0; i < nb_parent->GetPageCount(); ++i)
            if (nb_parent->GetPageText(last) == empty_label) {
                has_empty = true;
                break;
            }
        if (!has_empty) {
            PhasePanel *page = new PhasePanel(nb_parent, powder_book);
            nb_parent->AddPage(page, wxT("+"));
        }
    }
    else if (!valid && active != last && last_empty) {
        nb_parent->DeletePage(last);
    }
}

void PhasePanel::OnSpaceGroupChanged(wxCommandEvent&)
{
    string text = wx2s(sg_tc->GetValue());
    change_space_group(text);
    powder_book->deselect_phase_quick_list();
    sample_plot->refresh();
}

void PhasePanel::change_space_group(string const& text)
{
    cr_.set_space_group(text.c_str());
    if (cr_.sg_number <= 0) {
        sg_tc->Clear();
        hkl_list->Clear();
        sg_nr_st->SetLabel(wxEmptyString);
        return;
    }

    sg_tc->ChangeValue(pchar2wx(cr_.sg_hm));
    sg_nr_st->SetLabel(wxString::Format(wxT("no. %d, %s, order %d"),
                                        cr_.sg_number,
                                        get_crystal_system_name(cr_.sg_xs),
                                        get_sg_order(cr_.sg_ops)));
    enable_parameter_fields();
    update_disabled_parameters();
    update_miller_indices();
}

void PhasePanel::enable_parameter_fields()
{
    switch (cr_.sg_xs) {
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
            par_alpha->Enable(true);
            par_beta->Enable(false);
            par_beta->set_value(90.);
            par_gamma->Enable(false);
            par_gamma->set_value(90.);
            break;
        case OrthorhombicSystem:
            par_b->Enable(false);
            par_c->Enable(true);
            set_ortho_angles();
            break;
        case TetragonalSystem:
            par_b->Enable(true);
            par_c->Enable(true);
            set_ortho_angles();
            break;
        case TrigonalSystem:
            par_b->Enable(false);
            par_c->Enable(false);
            par_alpha->Enable(true);
            par_beta->Enable(false);
            par_gamma->Enable(false);
            break;
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
        default:
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

void PhasePanel::OnParameterChanged(wxCommandEvent&)
{
    update_disabled_parameters();
    powder_book->deselect_phase_quick_list();
    sample_plot->refresh();
}

void PhasePanel::OnLineToggled(wxCommandEvent& event)
{
    int n = event.GetSelection();
    PlanesWithSameD& bp = cr_.bp[n];
    // event.IsChecked() is not set (wxGTK 2.9)
    bp.enabled = hkl_list->IsChecked(n);

    sample_plot->refresh();
}

wxString make_info_string_for_line(const PlanesWithSameD& bp,
                                   PowderBook *powder_book)
{
    wxString info = wxT("line ");
    wxString mult_str = wxT("multiplicity: ");
    wxString sfac_str = wxT("|F(hkl)|: ");
    for (vector<Plane>::const_iterator i = bp.planes.begin();
                                              i != bp.planes.end(); ++i) {
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
        wxString format = (i == 0 ? wxT("\npeak center: %g") : wxT(", %g"));
        info += wxString::Format(format, lambda);
    }
    return info;
}

wxString make_info_string_for_atoms(const vector<Atom>& atoms, int error_line)
{
    wxString info = wxT("In unit cell:");
    for (vector<Atom>::const_iterator i = atoms.begin(); i != atoms.end(); ++i){
        info += wxString::Format(wxT(" %d %s "),
                                 (int)i->positions.size(), i->symbol);
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
        info_tc->SetValue(make_info_string_for_line(bp, powder_book));
    }
    sample_plot->refresh();
}

void PhasePanel::OnAtomsFocus(wxFocusEvent&)
{
    if (atoms_show_help_) {
        atoms_tc->Clear();
        atoms_show_help_ = false;
    }
    wxString info = make_info_string_for_atoms(cr_.atoms, line_with_error_);
    info_tc->SetValue(info);
    editing_atoms_ = true;
    sample_plot->refresh();
}

void PhasePanel::OnAtomsUnfocus(wxFocusEvent&)
{
    double lambda = powder_book->get_lambda(0);
    if (lambda > 0)
        cr_.calculate_intensities(lambda);
    int n = hkl_list->GetSelection();
    if (n >= 0) {
        PlanesWithSameD const& bp = cr_.bp[n];
        info_tc->SetValue(make_info_string_for_line(bp, powder_book));
    }
    editing_atoms_ = false;
    sample_plot->refresh();
}

bool isspace(const char* s)
{
    for (const char* i = s; *s != '\0'; ++i)
        if (!isspace(s))
            return false;
    return true;
}

void PhasePanel::OnAtomsChanged(wxCommandEvent&)
{
    string atoms_str = wx2s(atoms_tc->GetValue());
    if (atoms_str.size() < 3) {
        info_tc->Clear();
        return;
    }
    const char* s = atoms_str.c_str();
    line_with_error_ = -1;
    int atom_count = 0;
    // don't use cr_.atoms.clear(), to make it faster
    for (int line_nr = 1; ; ++line_nr) {
        // skip whitespace
        while(isspace(*s) && *s != '\n')
            ++s;
        if (*s == '\n')
            continue;
        if (*s == '\0')
            break;

        // usually the atom is not changed, so we first parse data
        // into a new struct Atom, and if it is different we copy it
        // and do calculations
        Atom a;

        // parse symbol
        const char* word_end = s;
        while (isalnum(*word_end))
            ++word_end;
        int symbol_len = word_end - s;
        if (symbol_len == 0 || symbol_len >= 8) {
            line_with_error_ = line_nr;
            break;
        }
        memcpy(a.symbol, s, symbol_len);
        a.symbol[symbol_len] = '\0';
        s = word_end;

        // parse x, y, z
        char *endptr;
        a.positions[0].x = strtod(s, &endptr);
        s = endptr;
        a.positions[0].y = strtod(s, &endptr);
        s = endptr;
        a.positions[0].z = strtod(s, &endptr);

        // check if the numbers were parsed
        if (endptr == s || // one or more numbers were not parsed
            (!isspace(*endptr) && *endptr != '\0')) // e.g. "Si 0 0 0foo"
        {
            line_with_error_ = line_nr;
            break;
        }

        s = endptr;

        // if there is more than 4 words in the line, ignore the extra words
        while (*s != '\n' && *s != '\0')
            ++s;
        if (*s == '\n')
            ++s;

        // check if the atom needs to be copied, and copy it if necessary
        if (atom_count == (int) cr_.atoms.size()) {
            a.xray_sf = find_in_it92(a.symbol);
            a.neutron_sf = find_in_nn92(a.symbol);
            cr_.atoms.push_back(a);
            add_symmetric_images(cr_.atoms[atom_count], cr_.sg_ops);
        }
        else {
            Atom &b = cr_.atoms[atom_count];
            if (strcmp(a.symbol, b.symbol) != 0) {
                memcpy(b.symbol, a.symbol, 8);
                b.xray_sf = find_in_it92(b.symbol);
                b.neutron_sf = find_in_nn92(b.symbol);
            }
            if (a.positions[0].x != b.positions[0].x
                    || a.positions[0].y != b.positions[0].y
                    || a.positions[0].z != b.positions[0].z) {
                b.positions.resize(1);
                b.positions[0] = a.positions[0];
                add_symmetric_images(cr_.atoms[atom_count], cr_.sg_ops);
            }
        }
        ++atom_count;
    }
    // vector cr_.atoms may be longer than necessary
    cr_.atoms.resize(atom_count);

    wxString info = make_info_string_for_atoms(cr_.atoms, line_with_error_);
    info_tc->SetValue(info);
}

void PhasePanel::update_disabled_parameters()
{
    if (!par_b->IsEnabled())
        par_b->set_string(par_a->get_string());

    if (!par_c->IsEnabled())
        par_c->set_string(par_a->get_string());

    if (cr_.sg_xs == TrigonalSystem) {
        par_beta->set_string(par_alpha->get_string());
        par_gamma->set_string(par_alpha->get_string());
    }
}

void PhasePanel::update_miller_indices()
{
    double a = par_a->get_value();
    double b = par_b->get_value();
    double c = par_c->get_value();
    double alpha = par_alpha->get_value() * M_PI / 180.;
    double beta = par_beta->get_value() * M_PI / 180.;
    double gamma = par_gamma->get_value() * M_PI / 180.;

    double lambda = powder_book->get_lambda(0);

    double max_2theta = powder_book->get_x_max() * M_PI / 180;

    hkl_list->Clear();
    if (a <= 0 || b <= 0 || c <= 0 || alpha <= 0 || beta <= 0 || gamma <= 0
            || lambda <= 0 || max_2theta <= 0) {
        return;
    }
    cr_.set_unit_cell(a, b, c, alpha, beta, gamma);

    double min_d = lambda / (2 * sin(max_2theta / 2.));
    cr_.generate_reflections(min_d);
    cr_.calculate_intensities(lambda);

    for (vector<PlanesWithSameD>::const_iterator i = cr_.bp.begin();
                                                i != cr_.bp.end(); ++i) {
        char a = (i->planes.size() == 1 ? ' ' : '*');
        Miller const& m = i->planes[0];
        hkl_list->Append(wxString::Format(wxT("(%d,%d,%d)%c  d=%g"),
                                          m.h, m.k, m.l, a, i->d));
    }
    for (size_t i = 0; i < hkl_list->GetCount(); ++i)
        hkl_list->Check(i, true);
}

void PhasePanel::set_phase(string const& name, CelFile const& cel)
{
    name_tc->SetValue(s2wx(name));
    change_space_group(cel.sgs->HM); // TODO ...
    par_a->set_string(wxString::Format(wxT("%g"), (cel.a)));
    par_b->set_string(wxString::Format(wxT("%g"), (cel.b)));
    par_c->set_string(wxString::Format(wxT("%g"), (cel.c)));
    par_alpha->set_string(wxString::Format(wxT("%g"), (cel.alpha)));
    par_beta->set_string(wxString::Format(wxT("%g"), (cel.beta)));
    par_gamma->set_string(wxString::Format(wxT("%g"), (cel.gamma)));
    update_miller_indices();
}

wxPanel* PowderBook::PrepareSamplePanel()
{
    wxPanel *panel = new wxPanel(this);
    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    wxSizer *quick_sizer = new wxStaticBoxSizer(wxVERTICAL, panel,
                                                wxT("quick list"));
    quick_phase_lb = new wxListBox(panel, -1, wxDefaultPosition, wxDefaultSize,
                                   0, NULL, wxLB_SINGLE|wxLB_SORT);

    for (map<string, CelFile>::const_iterator i
              = quick_phase_list.begin(); i != quick_phase_list.end(); ++i)
        quick_phase_lb->Append(s2wx(i->first));

    quick_sizer->Add(quick_phase_lb, wxSizerFlags(1).Border().Expand());
    wxButton *s_qremove_btn = new wxButton(panel, wxID_REMOVE);
    quick_sizer->Add(s_qremove_btn,
                     wxSizerFlags().Center().Border(wxLEFT|wxRIGHT));
    wxButton *s_qimport_btn = new wxButton(panel, wxID_OPEN, wxT("Import"));
    quick_sizer->Add(s_qimport_btn, wxSizerFlags().Center().Border());

    sizer->Add(quick_sizer, wxSizerFlags().Expand());

    sample_nb = new wxNotebook(panel, -1, wxDefaultPosition, wxDefaultSize,
                               wxNB_BOTTOM);
    PhasePanel *page = new PhasePanel(sample_nb, this);
    sample_nb->AddPage(page, wxT("+"));

    sizer->Add(sample_nb, wxSizerFlags(1).Expand());

    panel->SetSizerAndFit(sizer);

    Connect(quick_phase_lb->GetId(), wxEVT_COMMAND_LISTBOX_SELECTED,
            (wxObjectEventFunction) &PowderBook::OnQuickPhaseSelected);
    Connect(s_qremove_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &PowderBook::OnQuickListRemove);
    Connect(s_qimport_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &PowderBook::OnQuickListImport);

    return panel;
}

wxPanel* PowderBook::PreparePeakPanel()
{
    wxPanel *panel = new wxPanel(this);
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    /*  Npr=0, 1, 5 - Pseudo-Voigt,  shape - see (3.21)
     *  Npr=4 - 3 * Pseudo-Voigt
     *  Npr=2, 3, 6 - Pearson VII - shape - see (3.23)
     *  FWHM - see (3.20)
     */

    panel->SetSizerAndFit(sizer);

    return panel;
}

wxPanel* PowderBook::PrepareActionPanel()
{
    wxPanel *panel = new wxPanel(this);
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

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
            get_phase_panel(i)->sample_plot->refresh();
    } catch (runtime_error const& e) {
        data = NULL;
        wxMessageBox(wxT("Can not load file:\n") + s2wx(e.what()),
                     wxT("Error"), wxICON_ERROR);
    }
}
#endif

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
        if (s.StartsWith(i->name)) {
            double a1 = 0, a2 = 0;
            if (s.EndsWith("A12")) {
                a1 = i->alpha1;
                a2 = i->alpha2;
            }
            else if (s.EndsWith("A1")) {
                a1 = i->alpha1;
            }
            else
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
    string name = wx2s(quick_phase_lb->GetString(n));
    CelFile const& cel = quick_phase_list[name];
    PhasePanel *panel = get_current_phase_panel();
    assert(panel->GetParent() == sample_nb);
    panel->set_phase(name, cel);
    panel->s_qadd_btn->Enable(false);
    panel->sample_plot->refresh();
}

void PowderBook::OnQuickListRemove(wxCommandEvent&)
{
    int n = quick_phase_lb->GetSelection();
    if (n < 0)
        return;

    string name = wx2s(quick_phase_lb->GetString(n));
    assert(quick_phase_lb->GetCount() == quick_phase_list.size());

    quick_phase_lb->SetSelection(wxNOT_FOUND);
    quick_phase_lb->Delete(n);
    quick_phase_list.erase(name);
}

void PowderBook::OnQuickListImport(wxCommandEvent&)
{
    wxFileDialog dlg (this, wxT("Import CIF or CEL files"), wxT(""), wxT(""),
                      wxT("all supported files|*.cif;*.CIF;*.cel;*.CEL|")
                      wxT("CIF files (*.cif)|*.cif;*.CIF|")
                      wxT("PowderCell files (*.cel)|*.cel;*.CEL"),
                      wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK) {
    }
}

void PowderBook::OnLambdaChange(wxCommandEvent&)
{
    anode_lb->SetSelection(wxNOT_FOUND);
}

void PowderBook::OnPageChanged(wxListbookEvent& event)
{
    if (event.GetSelection() == 2) { // sample
        for (size_t i = 0; i != sample_nb->GetPageCount(); ++i) {
            PhasePanel* p = get_phase_panel(i);
            p->update_miller_indices();
            p->sample_plot->refresh();
        }
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
    quick_phase_lb->SetSelection(wxNOT_FOUND);
    PhasePanel *panel = get_current_phase_panel();
    if (!panel->name_tc->GetValue().empty())
        panel->s_qadd_btn->Enable(true);
}

double PowderBook::get_lambda(int n) const
{
    if (n < 0 || n >= (int) lambda_ctrl.size())
        return 0;
    return lambda_ctrl[n]->get_value();
}


SpaceGroupChooser::SpaceGroupChooser(wxWindow* parent)
        : wxDialog(parent, -1, wxT("Choose space group"),
                   wxDefaultPosition, wxDefaultSize,
                   wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    wxSizer *ssizer = new wxBoxSizer(wxHORIZONTAL);
    ssizer->Add(new wxStaticText(this, -1, wxT("system:")),
                wxSizerFlags().Border().Center());
    wxArrayString s_choices;
    s_choices.Add(wxT("All lattice systems"));
    for (int i = 2; i < 9; ++i)
        s_choices.Add(pchar2wx(CrystalSystemNames[i]));
    system_c = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize,
                            s_choices);
    system_c->SetSelection(0);
    ssizer->Add(system_c, wxSizerFlags(1).Border());
    sizer->Add(ssizer, wxSizerFlags().Expand());

    wxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(new wxStaticText(this, -1, wxT("centering:")),
                wxSizerFlags().Border().Center());
    wxString letters = wxT("PABCIRF");
    for (size_t i = 0; i < sizeof(centering_cb)/sizeof(centering_cb[0]); ++i) {
        centering_cb[i] = new wxCheckBox(this, -1, letters[i]);
        centering_cb[i]->SetValue(true);
        hsizer->Add(centering_cb[i], wxSizerFlags().Border());
    }
    sizer->Add(hsizer);

    list = new wxListView(this, -1,
                          wxDefaultPosition, wxSize(300, 500),
                          wxLC_REPORT|wxLC_HRULES|wxLC_VRULES);
    list->InsertColumn(0, wxT("No"));
    list->InsertColumn(1, wxT("H-M symbol"));
    list->InsertColumn(2, wxT("Schoenflies symbol"));
    list->InsertColumn(3, wxT("Hall symbol"));

    regenerate_list();

    // set widths of columns
    int col_widths[4];
    int total_width = 0;
    for (int i = 0; i < 4; i++) {
        list->SetColumnWidth(i, wxLIST_AUTOSIZE);
        col_widths[i] = list->GetColumnWidth(i);
        total_width += col_widths[i];
    }
    // leave margin of 50 px for scrollbar
    int empty_width = GetClientSize().GetWidth() - total_width - 50;
    if (empty_width > 0)
        for (int i = 0; i < 4; i++)
            list->SetColumnWidth(i, col_widths[i] + empty_width / 4);

    sizer->Add(list, wxSizerFlags(1).Expand().Border());
    sizer->Add(CreateButtonSizer(wxOK|wxCANCEL),
               wxSizerFlags().Expand());
    SetSizerAndFit(sizer);

    for (size_t i = 0; i < sizeof(centering_cb)/sizeof(centering_cb[0]); ++i)
        Connect(centering_cb[i]->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
                (wxObjectEventFunction) &SpaceGroupChooser::OnCheckBox);

    Connect(system_c->GetId(), wxEVT_COMMAND_CHOICE_SELECTED,
            (wxObjectEventFunction) &SpaceGroupChooser::OnSystemChoice);
    Connect(system_c->GetId(), wxEVT_COMMAND_LIST_ITEM_FOCUSED,
            (wxObjectEventFunction) &SpaceGroupChooser::OnListItemActivated);
}

void SpaceGroupChooser::regenerate_list()
{
    list->DeleteAllItems();
    int sel = system_c->GetSelection();
    //for (const T_Main_HM_Dict* i = Main_HM_Dict; i->HM != NULL; ++i) {
    for (const SpaceGroupSetting* i = space_group_settings; i->sgnumber; ++i) {
        if (sel != 0 && get_crystal_system(i->sgnumber) != sel+1)
            continue;
        switch (i->HM[0]) {
            case 'P': if (!centering_cb[0]->IsChecked()) continue; break;
            case 'A': if (!centering_cb[1]->IsChecked()) continue; break;
            case 'B': if (!centering_cb[2]->IsChecked()) continue; break;
            case 'C': if (!centering_cb[3]->IsChecked()) continue; break;
            case 'I': if (!centering_cb[4]->IsChecked()) continue; break;
            case 'R': if (!centering_cb[5]->IsChecked()) continue; break;
            case 'F': if (!centering_cb[6]->IsChecked()) continue; break;
            default: assert(0);
        }
        int n = list->GetItemCount();
        list->InsertItem(n, wxString::Format(wxT("%d"), i->sgnumber));
        list->SetItem(n, 1, pchar2wx(i->HM));
        list->SetItem(n, 2, pchar2wx(SchoenfliesSymbols[i->sgnumber-1]));
        list->SetItem(n, 3, pchar2wx(i->Hall));
    }
}

wxString SpaceGroupChooser::get_value() const
{
    int n = list->GetFirstSelected();
    if (n < 0)
        return wxEmptyString;
    wxListItem info;
    info.SetId(n);
    info.SetColumn(1);
    info.SetMask(wxLIST_MASK_TEXT);
    list->GetItem(info);
    return info.GetText();
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
    { wxCMD_LINE_SWITCH, "V", "version",
          "output version information and exit", wxCMD_LINE_VAL_NONE, 0 },
    { wxCMD_LINE_PARAM,  0, 0, "data file", wxCMD_LINE_VAL_STRING,
                                            wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_NONE, 0, 0, 0,  wxCMD_LINE_VAL_NONE, 0 }
};


bool App::OnInit()
{
    version = "0.1.0";

    // write numbers in C locale
    setlocale(LC_NUMERIC, "C");

    SetAppName("powdifpat");

    // parse command line parameters
    wxCmdLineParser cmdLineParser(cmdLineDesc, argc, argv);
    if (cmdLineParser.Parse(false) != 0) {
        cmdLineParser.Usage();
        return false;
    }
    if (cmdLineParser.Found(wxT("V"))) {
        wxMessageOutput::Get()->Printf("powdifpat " + version + "\n");
        return false;
    }

    wxImage::AddHandler(new wxPNGHandler);

    wxFrame *frame = new wxFrame(NULL, wxID_ANY, GetAppName());

#ifdef __WXMSW__
    frame->SetIcon(wxIcon("powdifpat")); // load from a resource
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
            (wxObjectEventFunction) &App::OnAbout);
    Connect(close->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &App::OnClose);
    return true;
}


void App::OnAbout(wxCommandEvent&)
{
    wxAboutDialogInfo adi;
    adi.SetVersion(version);
    wxString desc = "Powder diffraction pattern generator.\n";
    adi.SetDescription(desc);
    adi.SetWebSite("http://www.unipress.waw.pl/fityk/powdifpat/");
    wxString copyright = "(C) 2008 - 2009 Marcin Wojdyr <wojdyr@gmail.com>";
#ifdef __WXGTK__
    copyright.Replace("(C)", "\xc2\xa9");
#endif
    adi.SetCopyright(copyright);
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
}

#endif

