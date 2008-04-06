// Author: Marcin Wojdyr 
// Licence: GNU General Public License version 2
// $Id$

#include <wx/wx.h>  

#include <wx/imaglist.h>
#include <wx/cmdline.h>
#include <wx/listbook.h>
#include <wx/listctrl.h>

#include <xylib/xylib.h>

#include <cctbx/sgtbx/symbols.h>

#include "../common.h"
#include "cmn.h"
#include "uplot.h" // BufferedPanel, scale_tics_step()

#include "img/lock.xpm"
#include "img/lock_open.xpm"


using namespace std;

class LockableRealCtrl;


class PowderBook : public wxListbook
{
public:
    static const int max_wavelengths = 5;
    PowderBook(wxWindow* parent, wxWindowID id);
    void OnAnodeSelected(wxCommandEvent& event);
    void OnLambdaChange(wxCommandEvent& event);

private:
    wxListBox *anode_lb;
    vector<LockableRealCtrl*> lambda_ctrl, intensity_ctrl, corr_ctrl;
    wxNotebook *sample_nb;

    wxPanel* PrepareIntroPanel();
    wxPanel* PrepareInstrumentPanel();
    wxPanel* PrepareSamplePanel();
    wxPanel* PreparePeakPanel();
    wxPanel* PrepareActionPanel();
};


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
        PrepareDC( dc );
        dc.DrawBitmap(bitmap, 0, 0, true);
    }

private:
    wxBitmap bitmap;
};


class LockButton : public wxBitmapButton
{
public:
    LockButton(wxWindow* parent /*, wxTextCtrl *text_*/)
        : wxBitmapButton(parent, -1, wxBitmap(lock_xpm),
                         wxDefaultPosition, wxDefaultSize, wxNO_BORDER),
          //text(text_), 
          locked(true)
    {
        Connect(GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
                (wxObjectEventFunction) &LockButton::OnClick);
    }

    void OnClick(wxCommandEvent&)
    {
        locked = !locked;
        SetBitmapLabel(wxBitmap(locked ? lock_xpm : lock_open_xpm));
        //text->Enable(!locked);
        //text->SetEditable(!locked);
    }

    bool is_locked() const { return locked; }

private:
    //wxTextCtrl *text;
    bool locked;
};


class LockableRealCtrl : public wxPanel
{
public:
    LockableRealCtrl(wxWindow* parent, bool percent=false);
    bool is_locked() const { return lock->is_locked(); }
    bool is_null() const { return text->IsEmpty(); }
    double get_value() const;
    void set_value(double val) 
          { text->ChangeValue(wxString::Format(wxT("%g"), val)); }
    void Clear() { text->ChangeValue(wxT("")); }

private:
    KFTextCtrl *text;
    LockButton *lock;
};


class SpaceGroupChooser : public wxDialog
{
public:
    SpaceGroupChooser(wxWindow* parent);
    wxString get_value() const;
private:
    wxListView *list;
};


class PhasePanel : public wxPanel
{
public:
    PhasePanel(wxNotebook *parent);
    void OnSpaceGroupButton(wxCommandEvent& event);
    void OnClearButton(wxCommandEvent& event);
    void OnNameChanged(wxCommandEvent& event);
    void OnSpaceGroupChanged(wxCommandEvent& event);

private:
    wxNotebook *nb_parent;
    wxTextCtrl *name_tc, *sg_tc;
    LockableRealCtrl *par_a, *par_b, *par_c, *par_alpha, *par_beta, *par_gamma;
    wxButton *s_qadd_btn;
    wxCheckListBox *hkl_list;
};


namespace {

struct t_wavelength 
{
    const char *anode;
    double alpha1, alpha2;
};

const t_wavelength wavelengths[] = {
    { "Cu", 1.54056, 1.54439 }, 
    { "Cr", 2.28970, 2.29361 }, 
    { "Fe", 1.93604, 1.93998 }, 
    { "Mo", 0.70930, 0.71359 }, 
    { "Ag", 0.55941, 0.56380 }, 
    { "Co", 1.78901, 1.79290 },
    { NULL, 0, 0 }
};

} // anonymous namespace


LockableRealCtrl::LockableRealCtrl(wxWindow* parent, bool percent)
    : wxPanel(parent, -1)
{
    if (percent)
        text = new KFTextCtrl(this, -1, wxT(""), 50, wxTE_RIGHT);
    else
        text = new KFTextCtrl(this, -1, wxT(""));
    lock = new LockButton(this);
    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(text, wxSizerFlags().Center());
    if (percent)
        sizer->Add(new wxStaticText(this, -1, wxT("%")),
                   wxSizerFlags().Center());
    sizer->Add(lock, wxSizerFlags().Center());
    SetSizerAndFit(sizer);
}

double LockableRealCtrl::get_value() const
{
    return strtod(text->GetValue().c_str(), 0);
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


PowderBook::PowderBook(wxWindow* parent, wxWindowID id)
    : wxListbook(parent, id) 
{
    wxImageList *image_list = new wxImageList(32, 32);
    image_list->Add(wxBitmap(wxImage("img/info32.png")));
    image_list->Add(wxBitmap(wxImage("img/radiation32.png")));
    image_list->Add(wxBitmap(wxImage("img/rubik32.png")));
    image_list->Add(wxBitmap(wxImage("img/info32.png")));
    image_list->Add(wxBitmap(wxImage("img/run32.png")));
    AssignImageList(image_list);

    AddPage(PrepareIntroPanel(), wxT("intro"), false, 0);
    AddPage(PrepareInstrumentPanel(), wxT("instrument"), false, 1);
    AddPage(PrepareSamplePanel(), wxT("sample"), false, 2);
    AddPage(PreparePeakPanel(), wxT("peak"), false, 3);
    AddPage(PrepareActionPanel(), wxT("action"), false, 4);
}


wxPanel* PowderBook::PrepareIntroPanel()
{
    static const char* intro_str = 
    "Before you start, you should have:\n"
    "  - data (powder diffraction pattern) loaded,\n"
    "  - only interesting range active,\n"
    "  - and baseline (background) either removed manually,\n"
    "    or modeled with e.g. polynomial.\n"
    "\n"
    "This window will help you build a model for your data. "
    "The model has constrained position of peaks and "
    "not constrained intensities. Fitting this model is "
    "known as using Pawley's method. It's similar to LeBail's method, "
    "where the model is the same, but fitting procedure is more complex.\n"
    "\n"
    "This window will not analyze the final result for you. "
    "But there is another window that can help with size-strain analysis.\n"
    "\n"
    "Good luck!\n";

    wxPanel *panel = new wxPanel(this); 
    wxSizer *intro_sizer = new wxBoxSizer(wxVERTICAL);
    wxTextCtrl *intro_tc = new wxTextCtrl(panel, -1, pchar2wx(intro_str), 
                                          wxDefaultPosition, wxDefaultSize, 
                                          wxTE_READONLY|wxTE_MULTILINE
                                          |wxNO_BORDER);
    intro_tc->SetBackgroundColour(GetBackgroundColour());
    intro_sizer->Add(intro_tc, wxSizerFlags(1).Expand().Border(wxALL, 20));
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
    for (t_wavelength const* i = wavelengths; i->anode; ++i) {
        anode_lb->Append(pchar2wx(i->anode));
        anode_lb->Append(pchar2wx(i->anode) + wxT(" A1"));
        anode_lb->Append(pchar2wx(i->anode) + wxT(" A12"));
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

    wxSizer *lo_sizer = new wxBoxSizer(wxHORIZONTAL);
    lo_sizer->Add(new wxStaticBitmap(panel, -1, lock_xpm), 
                  wxSizerFlags().Center().Right());
    lo_sizer->Add(new wxStaticText(panel, -1, wxT(" = const")),
                  wxSizerFlags().Center().Left());
    lambda_sizer->Add(lo_sizer);
    lambda_sizer->AddSpacer(1);

    wxSizer *lc_sizer = new wxBoxSizer(wxHORIZONTAL);
    lc_sizer->Add(new wxStaticBitmap(panel, -1, lock_open_xpm), 
                  wxSizerFlags().Center().Right());
    lc_sizer->Add(new wxStaticText(panel, -1, wxT(" = fit")),
                  wxSizerFlags().Center().Left());
    lambda_sizer->Add(lc_sizer);
    lambda_sizer->AddSpacer(1);

    wave_sizer->Add(lambda_sizer, wxSizerFlags(1).Border());
    sizer->Add(wave_sizer, wxSizerFlags().Expand());

    wxSizer *corr_sizer = new wxStaticBoxSizer(wxVERTICAL, panel, 
                                           wxT("corrections (use with care)"));
    wxBitmap formula(wxT("img/correction.png"), wxBITMAP_TYPE_PNG);
    corr_sizer->Add(new StaticBitmap(panel, formula), wxSizerFlags().Border());

    wxSizer *g_sizer = new wxFlexGridSizer(2);
    for (int i = 1; i <= 6; ++i) {
        wxString label = wxString::Format(wxT("p%d = "), i);
        g_sizer->Add(new wxStaticText(panel, -1, label), 
                     wxSizerFlags().Center());
        LockableRealCtrl *cor = new LockableRealCtrl(panel);
        g_sizer->Add(cor);
        corr_ctrl.push_back(cor);
    }
    corr_sizer->Add(g_sizer, wxSizerFlags().Expand());
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


PhasePanel::PhasePanel(wxNotebook *parent)
        : wxPanel(parent), nb_parent(parent)
{
    wxSizer *vsizer = new wxBoxSizer(wxVERTICAL);

    wxSizer *h0sizer = new wxBoxSizer(wxHORIZONTAL);
    h0sizer->Add(new wxStaticText(this, -1, wxT("Name:")),
                 wxSizerFlags().Center().Border());
    name_tc = new KFTextCtrl(this, -1, wxEmptyString, 150);
    h0sizer->Add(name_tc, wxSizerFlags().Center().Border());

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

    hkl_list = new wxCheckListBox(this, -1);
    vsizer->Add(hkl_list, wxSizerFlags(1).Expand().Border());

    wxBoxSizer *h2sizer = new wxBoxSizer(wxHORIZONTAL);
    s_qadd_btn = new wxButton(this, wxID_ADD, "Add to quick list");
    h2sizer->Add(s_qadd_btn, wxSizerFlags().Border());
    h2sizer->AddStretchSpacer();
    wxButton *s_clear_btn = new wxButton(this, wxID_CLEAR);
    h2sizer->Add(s_clear_btn, wxSizerFlags().Border());
    vsizer->Add(h2sizer, wxSizerFlags().Expand());

    SetSizerAndFit(vsizer);

    Connect(sg_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, 
            (wxObjectEventFunction) &PhasePanel::OnSpaceGroupButton);
    Connect(s_clear_btn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, 
            (wxObjectEventFunction) &PhasePanel::OnClearButton);
    Connect(name_tc->GetId(), wxEVT_COMMAND_TEXT_UPDATED, 
            (wxObjectEventFunction) &PhasePanel::OnNameChanged);
    Connect(sg_tc->GetId(), wxEVT_COMMAND_TEXT_ENTER, 
            (wxObjectEventFunction) &PhasePanel::OnSpaceGroupChanged);
}

void PhasePanel::OnSpaceGroupButton(wxCommandEvent& event)
{
    SpaceGroupChooser dlg(this);
    if (dlg.ShowModal() == wxID_OK)
        sg_tc->SetValue(dlg.get_value());
    OnSpaceGroupChanged(event);
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
            PhasePanel *page = new PhasePanel(nb_parent); 
            nb_parent->AddPage(page, wxT("+"));
        }
    }
    else if (!valid && active != last && last_empty) {
        nb_parent->DeletePage(last);
    }
}

void PhasePanel::OnSpaceGroupChanged(wxCommandEvent&)
{
    bool valid = true;
    if (valid) {
    }
    else {
        sg_tc->Clear();
        hkl_list->Clear();
    }
}

wxPanel* PowderBook::PrepareSamplePanel()
{
    wxPanel *panel = new wxPanel(this);
    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    wxSizer *quick_sizer = new wxStaticBoxSizer(wxVERTICAL, panel, 
                                                wxT("quick list"));
    wxListBox *quick_lb = new wxListBox(panel, -1);
    quick_lb->Append(wxT("SiC"));
    quick_lb->Append(wxT("ZnS"));
    quick_lb->Append(wxT("NaCl"));

    quick_sizer->Add(quick_lb, wxSizerFlags(1).Border().Expand());
    wxButton *s_qremove_btn = new wxButton(panel, wxID_REMOVE);
    quick_sizer->Add(s_qremove_btn, wxSizerFlags().Center().Border());

    sizer->Add(quick_sizer, wxSizerFlags().Expand());

    sample_nb = new wxNotebook(panel, -1, wxDefaultPosition, wxDefaultSize,
                               wxNB_BOTTOM);
    PhasePanel *page = new PhasePanel(sample_nb); 
    sample_nb->AddPage(page, wxT("+"));

    sizer->Add(sample_nb, wxSizerFlags(1).Expand());

    panel->SetSizerAndFit(sizer);
    return panel;
}

wxPanel* PowderBook::PreparePeakPanel()
{
    wxPanel *panel = new wxPanel(this); 
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

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
    for (t_wavelength const* i = wavelengths; i->anode; ++i) {
        if (s.StartsWith(i->anode)) {
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

void PowderBook::OnLambdaChange(wxCommandEvent&)
{
    int n = anode_lb->GetSelection();
    if (n >= 0)
        anode_lb->Deselect(n);
}

SpaceGroupChooser::SpaceGroupChooser(wxWindow* parent)
        : wxDialog(parent, -1, wxT("Choose space group"),
                   wxDefaultPosition, wxDefaultSize, 
                   wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    list = new wxListView(this, -1,
                          wxDefaultPosition, wxSize(300, 500),
                          wxLC_REPORT|wxLC_HRULES|wxLC_VRULES);
    list->InsertColumn(0, wxT("No"));
    list->InsertColumn(1, wxT("H-M symbol"));
    list->InsertColumn(2, wxT("Schoenflies symbol"));

    cctbx::sgtbx::space_group_symbol_iterator it; 
    cctbx::sgtbx::space_group_symbols sgs = it.next();
    for (int i = 0; sgs.number() != 0; ++i, sgs = it.next()) {
        list->InsertItem(i, wxString::Format(wxT("%d"), sgs.number()));
        list->SetItem(i, 1, s2wx(sgs.universal_hermann_mauguin()));
        list->SetItem(i, 2, s2wx(sgs.schoenflies()));
    }
    for (int i = 0; i < 3; i++)
        list->SetColumnWidth(i, wxLIST_AUTOSIZE);

    sizer->Add(list, wxSizerFlags(1).Expand().Border());
    sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 
               wxSizerFlags().Expand());
    SetSizerAndFit(sizer);
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


#if 1

#include <wx/aboutdlg.h>
//#include "img/xyconvert16.xpm"
//#include "img/xyconvert48.xpm"

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
    { wxCMD_LINE_NONE, 0, 0, 0,  wxCMD_LINE_VAL_NONE, 0 }
};


bool App::OnInit()
{
    // to make life simpler, use the same version number as xylib
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

//    //frame->SetIcon(wxICON(xyconvert));
//#ifdef __WXMSW__
//    frame->SetIcon(wxIcon("xyconvert")); // load from a resource
//#else
//    wxIconBundle ib;
//    ib.AddIcon(wxIcon(xyconvert48_xpm));
//    ib.AddIcon(wxIcon(xyconvert16_xpm));
//    frame->SetIcons(ib); // load from a resource
//#endif

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
    wxString copyright = "(C) 2008 Marcin Wojdyr <wojdyr@gmail.com>";
#ifdef __WXGTK__
    copyright.Replace("(C)", "\xc2\xa9");
#endif
    adi.SetCopyright(copyright);
    wxAboutBox(adi);
}

#endif

