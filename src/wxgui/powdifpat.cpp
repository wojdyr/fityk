// Author: Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

// TODO:
// double click in space group chooser should close the dialog
// systematic absences of peaks

#include <cmath>
#include <algorithm>
#include <iostream>

#include <wx/wx.h>

#include <wx/imaglist.h>
#include <wx/cmdline.h>
#include <wx/listctrl.h>

//#include <cctbx/sgtbx/symbols.h>
//#include <cctbx/error.h>
//#include <cctbx/miller/index_generator.h>
//
extern "C" {
#include <sglite/sglite.h>
#include <sglite/sgconst.h>
}
// SgLite doesn't provide API to this tables
// In 3rdparty/sglite this table was made not static
extern const T_Main_HM_Dict Main_HM_Dict[];


#include "powdifpat.h"
#include "../common.h"
#include "cmn.h"
#include "uplot.h" // BufferedPanel, scale_tics_step()
#include "fancyrc.h" // LockableRealCtrl 

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


class UnitCell
{
public:
    const double a, b, c, alpha, beta, gamma;
    const double V;
    double M[3][3];
    double M_1[3][3]; // M^-1

    UnitCell(double a_, double b_, double c_,
             double alpha_, double beta_, double gamma_)
        : a(a_), b(b_), c(c_), alpha(alpha_), beta(beta_), gamma(gamma_),
          V(calculate_V())
            { set_M(); }

    double calculate_V() const;

    // pre: V should be set
    void set_M();

    UnitCell get_reciprocal() const;

    // returns |v|, where v = M_1 * [h k l]; 
    double calculate_distance(double h, double k, double l) const;

    // calculate interplanar distance
    double calculate_d(int h, int k, int l) const;
};

struct Plane
{
    int h, k, l;
    double d;
    int multiplicity;
    double intensity;

    bool operator<(const Plane& p) const { return d > p.d; }
};


class SpaceGroupChooser : public wxDialog
{
public:
    SpaceGroupChooser(wxWindow* parent);
    void OnCheckBox(wxCommandEvent&) { regenerate_list(); }
    void regenerate_list();
    wxString get_value() const;
private:
    wxListView *list;
    wxCheckBox *centering_cb[7];
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
    void set_data(vector<string> const& tokens);
    vector<double> get_dists() const;

private:
    wxNotebook *nb_parent;
    PowderBook *powder_book;

    wxTextCtrl *name_tc, *sg_tc;
    LockableRealCtrl *par_a, *par_b, *par_c, *par_alpha, *par_beta, *par_gamma;
    wxButton *s_qadd_btn;
    wxCheckListBox *hkl_list;
    PlotWithLines *sample_plot;
    wxStaticText *sg_nr_st;

    //cctbx::sgtbx::space_group_symbols sgs;
    T_HM_as_Hall sg_names;
    T_SgOps sg_ops;
    int sg_xs;
    vector<Plane> hkls; // reflections

    void enable_parameter_fields();
    void update_disabled_parameters();
    void update_miller_indices();
    void set_ortho_angles();
    void set_space_group(string const& text);
};

namespace {

struct Anode
{
    const char *name;
    double alpha1, alpha2;
};

const Anode anodes[] = {
    { "Cu", 1.54056, 1.54439 },
    { "Cr", 2.28970, 2.29361 },
    { "Fe", 1.93604, 1.93998 },
    { "Mo", 0.70930, 0.71359 },
    { "Ag", 0.55941, 0.56380 },
    { "Co", 1.78901, 1.79290 },
    { NULL, 0, 0 }
};

const char* quick_list_ini =
"SiC 3C|F -4 3 m|4.36|4.36|4.36|90|90|90\n"
"SiC 6H|186|3.081|3.081|15.117|90|90|120\n"
"NaCl|F m -3 m|5.64009|5.64009|5.64009|90|90|90\n"
"CeO2|F m -3 m|5.41|5.41|5.41|90|90|90\n"
;

// helper to generate sequence 0, 1, -1, 2, -2, 3, ...
int inc_neg(int h) { return h > 0 ? -h : -h+1; }

static const double epsilon = 1e-9;

vector<Plane> generate_reflections(UnitCell const& uc, double min_d)
{
    vector<Plane> v;
    int max_h = 10; // TODO
    int max_k = 10; // TODO
    int max_l = 10; // TODO
    UnitCell reciprocal = uc.get_reciprocal();
    for (int h = 0; h != max_h; h = inc_neg(h))
        for (int k = 0; k != max_k; k = inc_neg(k))
            for (int l = (h==0 && k==0 ? 1 : 0); l != max_l; l = inc_neg(l)) {
                double d = 1 / reciprocal.calculate_distance(h, k, l);
                //double d = uc.calculate_d(h, k, l); // the same
                if (d < min_d)
                    continue;
                bool found = false;
                for (vector<Plane>::iterator i = v.begin(); i != v.end(); ++i) {
                    if (fabs(d - i->d) < epsilon) {
                        i->multiplicity++;
                        i->intensity += 0.;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    Plane p;
                    p.h = h;
                    p.k = k;
                    p.l = l;
                    p.d = d;
                    p.multiplicity = 1;
                    p.intensity = 0.;
                    v.push_back(p);
                }
            }
    sort(v.begin(), v.end());
    return v;
}

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

double UnitCell::calculate_V() const
{
    // Giacovazzo p.62
    double cosA = cos(alpha), cosB = cos(beta), cosG = cos(gamma);
    double t = 1 - cosA*cosA - cosB*cosB - cosG*cosG + 2*cosA*cosB*cosG;
    return a*b*c * sqrt(t);
}

double UnitCell::calculate_d(int h, int k, int l) const
{
    double sinA=sin(alpha), sinB=sin(beta), sinG=sin(gamma),
           cosA=cos(alpha), cosB=cos(beta), cosG=cos(gamma);
    return
      sqrt((1 - cosA*cosA - cosB*cosB - cosG*cosG + 2*cosA*cosB*cosG)
              / (  (h/a*sinA) * (h/a*sinA)
                 + (k/b*sinB) * (k/b*sinB)
                 + (l/c*sinG) * (l/c*sinG)
                 + 2*h*l/a/c*(cosA*cosG-cosB)
                 + 2*h*k/a/b*(cosA*cosB-cosG)
                 + 2*k*l/b/c*(cosB*cosG-cosA)
                )
          );
}

//     [       1/a               0        0   ]
// M = [  -cosG/(a sinG)    1/(b sinG)    0   ]
//     [     a*cosB*         b*cosA*      c*  ]
//
// where A is alpha, B is beta, G is gamma, * means reciprocal space.
// (Giacovazzo, p.68)
void UnitCell::set_M()
{
    double sinA=sin(alpha), sinB=sin(beta), sinG=sin(gamma),
           cosA=cos(alpha), cosB=cos(beta), cosG=cos(gamma);
    M[0][0] = 1/a;
    M[0][1] = 0.;
    M[0][2] = 0.;
    M[1][0] = -cosG/(a*sinG);
    M[1][1] = 1/(b*sinG);
    M[1][2] = 0.;
    M[2][0] = b * c * sinA / V // a*
              * (cosA*cosG-cosB) / (sinA*sinG); //cosB*
    M[2][1] = a * c * sinB / V // b*
              * (cosB*cosG-cosA) / (sinB*sinG); //cosA*
    M[2][2] = a * b * sinG / V; // c*

    M_1[0][0] = a;
    M_1[0][1] = 0;
    M_1[0][2] = 0;
    M_1[1][0] = b * cosG;
    M_1[1][1] = b * sinG;
    M_1[1][2] = 0;
    M_1[2][0] = c * cosB;
    M_1[2][1] = -c * sinB * (cosB*cosG-cosA) / (sinB*sinG);
    M_1[2][2] = 1 / M[2][2];
}

// returns UnitCell reciprocal to this, i.e. that has parameters a*, b*, ...
// (Giacovazzo, p. 64)
UnitCell UnitCell::get_reciprocal() const
{
    double ar = b * c * sin(alpha) / V;
    double br = a * c * sin(beta) / V;
    double cr = a * b * sin(gamma) / V;
    double cosAr = (cos(beta)*cos(gamma)-cos(alpha)) / (sin(beta)*sin(gamma));
    double cosBr = (cos(alpha)*cos(gamma)-cos(beta)) / (sin(alpha)*sin(gamma));
    double cosGr = (cos(alpha)*cos(beta)-cos(gamma)) / (sin(alpha)*sin(beta));
    return UnitCell(ar, br, cr, acos(cosAr), acos(cosBr), acos(cosGr));
}

// returns 1/|v|, where v = M * [h k l]; 
double UnitCell::calculate_distance(double h, double k, double l) const
{
    double v2 = 0.;
    for (int i = 0; i != 3; ++i) {
        double t = h * M_1[0][i] + k * M_1[1][i] + l * M_1[2][i];
        v2 += t*t;
    }
    return sqrt(v2);
}


PowderBook::PowderBook(wxWindow* parent, wxWindowID id)
    : wxListbook(parent, id), x_min(20), x_max(150), y_max(1000)
#if STANDALONE_POWDIFPAT
      , data(NULL)
#endif
{
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
}

void PowderBook::initialize_quick_phase_list()
{
    string s = quick_list_ini;

    const char *end = "\r\n";

    string::size_type a = s.find_first_not_of(end);
    while (a != string::npos) {
        string::size_type b = s.find_first_of(end, a);
        vector<string> tokens = split_string(s.substr(a, b-a), "|");

        if (tokens.size() >= 8)
            quick_phase_list[tokens[0]] = tokens;

        a = s.find_first_not_of(end, b);
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

    wxArrayString xaxis_choices;
    xaxis_choices.Add(wxT("2theta"));
    xaxis_choices.Add(wxT("Q"));
    xaxis_choices.Add(wxT("energy"));
    wxRadioBox *xaxis_rb = new wxRadioBox(panel, -1, wxT("data x axis"),
                                          wxDefaultPosition, wxDefaultSize,
                                          xaxis_choices, 3);
    xaxis_rb->Enable(1, false);
    xaxis_rb->Enable(2, false);
    sizer->Add(xaxis_rb, wxSizerFlags().Border().Expand());

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
    // draw lines
    int Y0 = dc.GetSize().GetHeight();
    int Y1 = getY(0) + 2;
    dc.SetPen(*wxRED_PEN);
    double lambda1 = powder_book_->lambda_ctrl[0]->get_value();
    vector<double> dd = phase_panel_->get_dists();
    for (vector<double>::const_iterator i = dd.begin(); i != dd.end(); ++i) {
        double d = *i;
        double angle = 180 / M_PI * 2 * asin(lambda1 / (2*d));
        int X = getX(angle);
        dc.DrawLine(X, Y0, X, Y1);
    }
}

void PlotWithLines:: draw_active_data(wxDC& dc)
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
    //TODO
    int n = 10;
    wxPoint *points = new wxPoint[n];
#endif // STANDALONE_POWDIFPAT
    dc.DrawLines(n, points);
    delete [] points;
}

PhasePanel::PhasePanel(wxNotebook *parent, PowderBook *powder_book_)
        : wxPanel(parent), nb_parent(parent), powder_book(powder_book_)
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
    par_alpha = addMaybeRealCtrl(this, wxT("α ="),  par_sizer, flags);
    par_beta = addMaybeRealCtrl(this, wxT("β ="),  par_sizer, flags);
    par_gamma = addMaybeRealCtrl(this, wxT("γ ="),  par_sizer, flags);
    stp_sizer->Add(par_sizer, wxSizerFlags(1).Expand());
    vsizer->Add(stp_sizer, wxSizerFlags().Border().Expand());

    wxSplitterWindow *hkl_split = new ProportionalSplitter(this, -1, 0.4);
    hkl_list = new wxCheckListBox(hkl_split, -1);
    sample_plot = new PlotWithLines(hkl_split, this, powder_book);
    hkl_split->SplitVertically(hkl_list, sample_plot);
    vsizer->Add(hkl_split, wxSizerFlags(1).Expand().Border());

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

    vector<string> tokens;
    tokens.push_back(wx2s(name));
    tokens.push_back(wx2s(sg_tc->GetValue()));
    tokens.push_back(wx2s(par_a->get_string()));
    tokens.push_back(wx2s(par_b->get_string()));
    tokens.push_back(wx2s(par_c->get_string()));
    tokens.push_back(wx2s(par_alpha->get_string()));
    tokens.push_back(wx2s(par_beta->get_string()));
    tokens.push_back(wx2s(par_gamma->get_string()));

    quick_phase_lb->SetStringSelection(name);
    powder_book->quick_phase_list[wx2s(name)] = tokens;
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
    set_space_group(text);
    powder_book->deselect_phase_quick_list();
    sample_plot->refresh();
}

void PhasePanel::set_space_group(string const& text)
{
    int sg_number = SgSymbolLookup('A', text.c_str(), &sg_names);
    //cout << sg_number << " " << sg_names.SgNumber << " " << sg_names.Schoenfl
    //     << " " << sg_names.HM << " " << sg_names.Hall << endl;
    if (sg_number <= 0) {
        sg_tc->Clear();
        hkl_list->Clear();
        sg_nr_st->SetLabel(wxEmptyString);
        return;
    }

    ClrSgError();
    ResetSgOps(&sg_ops);
    ParseHallSymbol(sg_names.Hall, &sg_ops, PHSymOptPedantic);
    if (SgError)
        cerr << SgError << endl;
    //DumpSgOps(&sg_ops, stdout);

    sg_tc->ChangeValue(pchar2wx(sg_names.HM));
    sg_xs = ixXS(GetPG(&sg_ops));
    sg_nr_st->SetLabel(wxString::Format(wxT("no. %d, %s"), sg_names.SgNumber,
                                                           XS_Name[sg_xs]));
    enable_parameter_fields();
    update_disabled_parameters();
    update_miller_indices();
}

void PhasePanel::enable_parameter_fields()
{
    switch (sg_xs) {
        case XS_Triclinic:
            par_b->Enable(true);
            par_c->Enable(true);
            par_alpha->Enable(true);
            par_beta->Enable(true);
            par_gamma->Enable(true);
            break;
        case XS_Monoclinic:
            par_b->Enable(true);
            par_c->Enable(true);
            par_alpha->Enable(true);
            par_beta->Enable(false);
            par_beta->set_value(90.);
            par_gamma->Enable(false);
            par_gamma->set_value(90.);
            break;
        case XS_Orthorhombic:
            par_b->Enable(false);
            par_c->Enable(true);
            set_ortho_angles();
            break;
        case XS_Tetragonal:
            par_b->Enable(true);
            par_c->Enable(true);
            set_ortho_angles();
            break;
        case XS_Trigonal:
            par_b->Enable(false);
            par_c->Enable(false);
            par_alpha->Enable(true);
            par_beta->Enable(false);
            par_gamma->Enable(false);
            break;
        case XS_Hexagonal:
            par_b->Enable(false);
            par_c->Enable(true);
            par_alpha->set_value(90);
            par_beta->set_value(90);
            par_gamma->set_value(120);
            break;
        case XS_Cubic:
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

void PhasePanel::OnLineToggled(wxCommandEvent&)
{
    sample_plot->refresh();
}

void PhasePanel::update_disabled_parameters()
{
    if (!par_b->IsEnabled())
        par_b->set_string(par_a->get_string());

    if (!par_c->IsEnabled())
        par_c->set_string(par_a->get_string());

    if (sg_xs == XS_Trigonal) {
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

    double lambda = powder_book->get_lambda0();

    double max_2theta = 150 * M_PI / 180; //TODO

    hkl_list->Clear();
    if (a <= 0 || b <= 0 || c <= 0 || alpha <= 0 || beta <= 0 || gamma <= 0
            || lambda <= 0 || max_2theta <= 0) {
        return;
    }
    UnitCell uc(a, b, c, alpha, beta, gamma);

    double min_d = lambda / (2 * sin(max_2theta / 2.));
    hkls = generate_reflections(uc, min_d);

    for (vector<Plane>::const_iterator i = hkls.begin(); i != hkls.end(); ++i) {
        hkl_list->Append(wxString::Format(wxT("(%d,%d,%d)  *%d  d=%g"),
                                   i->h, i->k, i->l, i->multiplicity, i->d));
    }
    for (size_t i = 0; i < hkl_list->GetCount(); ++i)
        hkl_list->Check(i, true);
}

void PhasePanel::set_data(vector<string> const& tokens)
{
    name_tc->SetValue(s2wx(tokens[0]));
    set_space_group(tokens[1]);
    par_a->set_string(s2wx(tokens[2]));
    par_b->set_string(s2wx(tokens[3]));
    par_c->set_string(s2wx(tokens[4]));
    par_alpha->set_string(s2wx(tokens[5]));
    par_beta->set_string(s2wx(tokens[6]));
    par_gamma->set_string(s2wx(tokens[7]));
    update_miller_indices();
}

vector<double> PhasePanel::get_dists() const
{
    vector<double> dd;
    for (size_t i = 0; i != hkls.size(); ++i)
        if (hkl_list->IsChecked(i))
            dd.push_back(hkls[i].d);
    return dd;
}

wxPanel* PowderBook::PrepareSamplePanel()
{
    wxPanel *panel = new wxPanel(this);
    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    wxSizer *quick_sizer = new wxStaticBoxSizer(wxVERTICAL, panel,
                                                wxT("quick list"));
    quick_phase_lb = new wxListBox(panel, -1, wxDefaultPosition, wxDefaultSize,
                                   0, NULL, wxLB_SINGLE|wxLB_SORT);

    for (map<string, vector<string> >::const_iterator i
              = quick_phase_list.begin(); i != quick_phase_list.end(); ++i)
        quick_phase_lb->Append(s2wx(i->first));

    quick_sizer->Add(quick_phase_lb, wxSizerFlags(1).Border().Expand());
    wxButton *s_qremove_btn = new wxButton(panel, wxID_REMOVE);
    quick_sizer->Add(s_qremove_btn, wxSizerFlags().Center().Border());

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
    vector<string> const& tokens = quick_phase_list[name];
    PhasePanel *panel = get_current_phase_panel();
    assert(panel->GetParent() == sample_nb);
    panel->set_data(tokens);
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

void PowderBook::OnLambdaChange(wxCommandEvent&)
{
    anode_lb->SetSelection(wxNOT_FOUND);
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

double PowderBook::get_lambda0() const
{
    return lambda_ctrl[0]->get_value();
}


SpaceGroupChooser::SpaceGroupChooser(wxWindow* parent)
        : wxDialog(parent, -1, wxT("Choose space group"),
                   wxDefaultPosition, wxDefaultSize,
                   wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
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
    int total_width = 0;
    for (int i = 0; i < 4; i++) {
        list->SetColumnWidth(i, wxLIST_AUTOSIZE);
        total_width += list->GetColumnWidth(i);
    }
    // leave margin of 20 px for scrollbar
    int empty_width = GetClientSize().GetWidth() - total_width - 20;
    if (empty_width > 0)
        for (int i = 0; i < 4; i++)
            list->SetColumnWidth(i, list->GetColumnWidth(i) + empty_width / 4);

    sizer->Add(list, wxSizerFlags(1).Expand().Border());
    sizer->Add(CreateButtonSizer(wxOK|wxCANCEL),
               wxSizerFlags().Expand());
    SetSizerAndFit(sizer);
    for (size_t i = 0; i < sizeof(centering_cb)/sizeof(centering_cb[0]); ++i)
        Connect(centering_cb[i]->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
                (wxObjectEventFunction) &SpaceGroupChooser::OnCheckBox);
}

void SpaceGroupChooser::regenerate_list()
{
    list->DeleteAllItems();
    for (const T_Main_HM_Dict* i = Main_HM_Dict; i->HM != NULL; ++i) {
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
        list->InsertItem(n, wxString::Format(wxT("%d"), i->SgNumber));
        list->SetItem(n, 1, pchar2wx(i->HM));
        T_HM_as_Hall names;
        SgSymbolLookup('A', i->HM, &names);
        list->SetItem(n, 2, pchar2wx(names.Schoenfl));
        list->SetItem(n, 3, pchar2wx(names.Hall));
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

#endif

