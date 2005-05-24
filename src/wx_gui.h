// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef WX_GUI__H__
#define WX_GUI__H__

#include "common.h"
#include "pag.h"
#include "wx_common.h"  // Output_style_enum
#include <list>
#include <wx/spinctrl.h>
#include <wx/html/helpctrl.h>
#ifdef __WXMSW__
#  include <wx/msw/helpbest.h>
#endif

class wxCmdLineParser;
//struct z_names_type;
struct f_names_type;
class ApplicationLogic;
class FDXLoadDlg;
class PlotPane;
class IOPane;
class DataPane;
class ProportionalSplitter;
class DataEditorDlg;

extern std::vector<fp> params4plot;

//status bar fields
enum StatusBarField { sbf_text, sbf_hint1, sbf_hint2, sbf_coord, sbf_max };  

class FApp: public wxApp
{
public:
    wxString conf_filename, alt_conf_filename;

    bool OnInit(void);
    int OnExit();

private:
    bool is_fityk_script(std::string filename);
    void process_argv(wxCmdLineParser &cmdLineParser);
};

DECLARE_APP(FApp)

class DotSet;

class FSetDlg : public wxDialog
{
public:
    std::vector<std::string>& opt_names;
    std::vector<std::string>& opt_values;
    std::vector<wxControl*> tc_v;
    FSetDlg(wxWindow* parent, const wxWindowID id, const wxString& title,
             std::vector<std::string>& names, std::vector<std::string>& vals,
             std::vector<std::string>& types, DotSet* myset);
};


class FToolBar : public wxToolBar
{
public:
    FToolBar (wxFrame *parent, wxWindowID id); 
    void update_peak_type(); 

    void OnIdle(wxIdleEvent &event);
    void OnPeakChoice (wxCommandEvent& event);
    void OnChangeMouseMode (wxCommandEvent& event);
    void OnSwitchDPane (wxCommandEvent& event);
    void OnClickTool (wxCommandEvent& event);

private:
    wxChoice *peak_choice; 

    DECLARE_EVENT_TABLE()
};

class FStatusBar: public wxStatusBar 
{
public:
    FStatusBar(wxWindow *parent);
    void OnSize(wxSizeEvent& event);
    void set_hint(const char *left, const char *right);
private:
    wxStaticBitmap *statbmp1, *statbmp2;
    DECLARE_EVENT_TABLE()
};


// Define a new frame
class FFrame: public wxFrame
{
    friend class FToolBar;
    friend class FApp;
public:
    FFrame(wxWindow *parent, const wxWindowID id, const wxString& title, 
            const long style);
    ~FFrame();
    //void OnSize (wxSizeEvent& event);

    void OnIdle(wxIdleEvent &event); 
    void OnShowHelp (wxCommandEvent& event);
    void OnTipOfTheDay (wxCommandEvent& event);
    void OnAbout (wxCommandEvent& event);
    void OnQuit (wxCommandEvent& event);

    void OnDLoad         (wxCommandEvent& event);   
    void OnDXLoad        (wxCommandEvent& event);   
    void OnDRecent       (wxCommandEvent& event);
    void OnDEditor       (wxCommandEvent& event);
    void OnDInfo         (wxCommandEvent& event);
    void OnDExport       (wxCommandEvent& event);
    void OnDSet          (wxCommandEvent& event); 

    void OnSHistory      (wxCommandEvent& event);            
    void OnSInfo         (wxCommandEvent& event);         
    void OnSAdd          (wxCommandEvent& event);        
    void OnSRemove       (wxCommandEvent& event);           
    void OnSChange       (wxCommandEvent& event);           
    void OnSValue        (wxCommandEvent& event);          
    void OnSExport       (wxCommandEvent& event);           
    void OnSSet          (wxCommandEvent& event);        

    void OnMFindpeak     (wxCommandEvent& event);        
    void OnMSet          (wxCommandEvent& event);        

    void OnFMethodUpdate (wxUpdateUIEvent& event);           
    void OnFOneOfMethods (wxCommandEvent& event);
    void OnFRun          (wxCommandEvent& event);        
    void OnFContinue     (wxCommandEvent& event);             
    void OnFInfo         (wxCommandEvent& event);         
    void OnFSet          (wxCommandEvent& event);        

    void OnCWavelength   (wxCommandEvent& event);               
    void OnCAdd          (wxCommandEvent& event);        
    void OnCInfo         (wxCommandEvent& event);         
    void OnCRemove       (wxCommandEvent& event);           
    void OnCEstimate     (wxCommandEvent& event);             
    void OnCSet          (wxCommandEvent& event);        

    void OnOLog          (wxCommandEvent& event);        
    void OnOInclude      (wxCommandEvent& event);            
    void OnOReInclude    (wxCommandEvent& event);            
    void OnO_Reset       (wxCommandEvent& event);
    void OnOWait         (wxCommandEvent& event);         
    void OnODump         (wxCommandEvent& event);         
    void OnOSet          (wxCommandEvent& event);        
    void OnPrintPreview  (wxCommandEvent& event);
    void OnPrintSetup    (wxCommandEvent& event);
    void OnPrint         (wxCommandEvent& event);
    void OnChangeMouseMode (wxCommandEvent& event);
    void OnModePeak      (wxUpdateUIEvent& event);
    void OnChangePeakType(wxCommandEvent& event);
    void OnGViewAll      (wxCommandEvent& event);
    void OnGFitHeight    (wxCommandEvent& event);
    void OnGScrollLeft   (wxCommandEvent& event);
    void OnGScrollRight  (wxCommandEvent& event);
    void OnPreviousZoom  (wxCommandEvent& event);
    void OnConfigRead    (wxCommandEvent& event);
    void OnConfigBuiltin (wxCommandEvent& event);
    void OnConfigSave    (wxCommandEvent& event);
    void OnGuiShowUpdate (wxUpdateUIEvent& event);
    void SwitchDPane(bool show);
    void OnSwitchDPane(wxCommandEvent& ev) {SwitchDPane(ev.IsChecked());}
    void OnSwitchAuxPlot(wxCommandEvent& ev);
    void SwitchIOPane(bool show);
    void OnSwitchIOPane(wxCommandEvent& ev) {SwitchIOPane(ev.IsChecked());}
    void SwitchToolbar(bool show);
    void OnSwitchToolbar(wxCommandEvent& ev) {SwitchToolbar(ev.IsChecked());}
    void SwitchStatbar(bool show);
    void OnSwitchStatbar(wxCommandEvent& ev) {SwitchStatbar(ev.IsChecked());}
    void SwitchCrosshair(bool show);
    void OnSwitchCrosshair(wxCommandEvent& ev){SwitchCrosshair(ev.IsChecked());}
    void OnShowMenuZoomPrev(wxUpdateUIEvent& event);
    void save_all_settings(wxConfigBase *cf) const;
    void save_settings(wxConfigBase *cf) const;
    void read_all_settings(wxConfigBase *cf);
    void read_settings(wxConfigBase *cf);
    const FToolBar* get_toolbar() const { return toolbar; }
    const f_names_type& get_peak_type() const;
    void set_status_hint(const char *left, const char *right);
    void output_text(OutputStyle style, const std::string& str);
    void change_zoom(const std::string& s);
    void scroll_view_horizontally(fp step);
    void refresh_plots(bool refresh=true, bool update=false);
    void draw_crosshair(int X, int Y);
    void focus_input();
    void set_status_text(const wxString &text, StatusBarField field=sbf_text) 
            { if (status_bar) SetStatusText(text, field); }

protected:
    ProportionalSplitter *main_pane;
    PlotPane *plot_pane;
    IOPane *io_pane;
    DataPane *data_pane;
    FStatusBar *status_bar;

    int peak_type_nr;
    FToolBar *toolbar;
    FDXLoadDlg *dxload_dialog;
    DataEditorDlg *data_editor;
    ProportionalSplitter *v_splitter;
    wxPrintData *print_data;
    wxPageSetupData* page_setup_data;
#ifdef __WXMSW__
    wxBestHelpController help;
#else
    wxHtmlHelpController help; 
#endif
    std::string last_include_path;
    std::list<wxFileName> recent_data_files;
    wxMenu *data_menu_recent;

    void place_plot_and_io_windows(wxWindow *parent);
    void create_io_panel(wxWindow *parent);
    void OnXSet (std::string name, char letter);
    void set_menubar();
    void not_implemented_menu_item (const std::string &command) const; 
    void read_recent_data_files();
    void write_recent_data_files();
    void add_recent_data_file(wxString filename);

DECLARE_EVENT_TABLE()
};


void add_peak(fp height, fp ctr, fp hwhm);
void add_peak_in_range(fp xmin, fp xmax);
extern FFrame *frame;

#endif //WXGUI__H__

