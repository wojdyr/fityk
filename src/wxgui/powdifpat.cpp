// Author: Marcin Wojdyr 
// Licence: GNU General Public License version 2
// $Id$

#include <wx/wx.h>  

#include <wx/imaglist.h>
#include <wx/cmdline.h>
#include <wx/listbook.h>

#include <xylib/xylib.h>

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

private:
    wxListBox *anode_lb;
    vector<LockableRealCtrl*> lambda_ctrl, intensity_ctrl, corr_ctrl;

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
          { text->SetValue(wxString::Format(wxT("%g"), val)); }
    void Clear() { text->Clear(); }

private:
    KFTextCtrl *text;
    LockButton *lock;
};

LockableRealCtrl::LockableRealCtrl(wxWindow* parent, bool percent)
    : wxPanel(parent, -1)
{
    if (percent)
        text = new KFTextCtrl(this, -1, wxT(""), 50, wxTE_RIGHT);
    else
        text = new KFTextCtrl(this, -1, wxT(""));
    lock = new LockButton(this);
    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(text);
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
    wxSizer *wave_sizer = new wxStaticBoxSizer(wxHORIZONTAL, panel, 
                                               wxT("wavelengths"));
    anode_lb = new wxListBox(panel, -1);
    for (t_wavelength const* i = wavelengths; i->anode; ++i) {
        anode_lb->Append(pchar2wx(i->anode));
        anode_lb->Append(pchar2wx(i->anode) + wxT(" A1"));
        anode_lb->Append(pchar2wx(i->anode) + wxT(" A12"));
    }
    wave_sizer->Add(anode_lb, wxSizerFlags(1).Border().Expand());

    wxSizer *lambda_sizer = new wxFlexGridSizer(2);
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

    Connect(anode_lb->GetId(), wxEVT_COMMAND_LISTBOX_SELECTED, 
            (wxObjectEventFunction) &PowderBook::OnAnodeSelected);

    return panel;
}

wxPanel* PowderBook::PrepareSamplePanel()
{
    wxPanel *panel = new wxPanel(this); 
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

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
#ifdef __WXGTK__
    frame->SetSize(-1, 550);
#endif
    
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

