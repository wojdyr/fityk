// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_GUI__H__
#define FITYK__WX_GUI__H__

#include <list>
#include <wx/spinctrl.h>
#include <wx/help.h>
#include <wx/filename.h>
#include "cmn.h"  // Output_style_enum
#include "../common.h" //s2wx

class wxCmdLineParser;
//struct z_names_type;
struct f_names_type;
class ApplicationLogic;
class FDXLoadDlg;
class PlotPane;
class MainPlot;
class IOPane;
class SideBar;
class ProportionalSplitter;
class DataEditorDlg;
class PrintManager;

extern std::vector<fp> params4plot;

//status bar fields
enum StatusBarField { sbf_text, sbf_hint1, sbf_hint2, sbf_coord, sbf_max };  

/// Fityk-GUI "main loop"
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

/// Toolbar bar in Fityk
class FToolBar : public wxToolBar
{
public:
    FToolBar (wxFrame *parent, wxWindowID id); 
    void update_peak_type(int nr, std::vector<std::string> const* peak_types=0);

    void OnPeakChoice (wxCommandEvent& event);
    void OnChangeMouseMode (wxCommandEvent& event);
    void OnSwitchSideBar (wxCommandEvent& event);
    void OnClickTool (wxCommandEvent& event);

private:
    wxChoice *peak_choice; 

    DECLARE_EVENT_TABLE()
};

/// Status bar in Fityk
class FStatusBar: public wxStatusBar 
{
public:
    wxString fmt_main, fmt_aux;

    FStatusBar(wxWindow *parent);
    void OnSize(wxSizeEvent& event) { move_bitmaps(); event.Skip(); }
    void move_bitmaps();
    void set_hint(std::string const& left, std::string const& right);
    int get_coord_width() const { return m_statusWidths[sbf_coord]; }
    int get_hint_width() const { return m_statusWidths[sbf_hint1]; }
    void set_widths(int hint, int coord);
    void set_hint_width(int w);
    void set_coord_info(fp x, fp y, bool aux=false);
    bool set_extra_value(std::string const& s);
    wxString const& get_extra_value() const { return extra_value; }
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);

private:
    int widths[4]; //4==sbf_max
    wxString extra_value;
    std::vector<int> e_code;
    std::vector<fp> e_numbers;
    wxStaticBitmap *statbmp1, *statbmp2;
    DECLARE_EVENT_TABLE()
};

/// Status bar configuration dialog
class ConfStatBarDlg: public wxDialog
{
public:
    ConfStatBarDlg(wxWindow* parent, wxWindowID id, FStatusBar* sb_);
    void OnApply (wxCommandEvent& event);
    void OnClose (wxCommandEvent&) { close_it(this); }
private:
    FStatusBar *sb;
    SpinCtrl *width_sc, *whint_sc;
    wxTextCtrl *fm_tc, *fa_tc, *ff_tc;
    DECLARE_EVENT_TABLE()
};



/// dialog Help->About
class AboutDlg : public wxDialog
{
public:
    AboutDlg(wxWindow* parent);
    void OnTextURL(wxTextUrlEvent& event);
private:
    wxTextCtrl *txt;
    DECLARE_EVENT_TABLE()
};


/// Fityk-GUI main window
class FFrame: public wxFrame
{
    friend class FToolBar;
    friend class FApp;
public:
    FFrame(wxWindow *parent, const wxWindowID id, const wxString& title, 
            const long style);
    ~FFrame();
    //void OnSize (wxSizeEvent& event);

    void OnShowHelp (wxCommandEvent& event);
    void OnTipOfTheDay (wxCommandEvent& event);
    void OnAbout (wxCommandEvent& event);
    void OnContact (wxCommandEvent& event);
    void OnQuit (wxCommandEvent& event);

    void OnDLoad         (wxCommandEvent& event);   
    void OnDXLoad        (wxCommandEvent& event);   
    void OnDRecent       (wxCommandEvent& event);
    void OnDEditor       (wxCommandEvent& event);
    void OnFastDT        (wxCommandEvent& event);
    void OnFastDTUpdate  (wxUpdateUIEvent& event);           
    void OnAllDatasetsUpdate (wxUpdateUIEvent& event);           
    void OnDExport       (wxCommandEvent& event);

    void OnSEditor       (wxCommandEvent& event);            
    void OnDefinitionMgr (wxCommandEvent&);         
    void OnSGuess        (wxCommandEvent& event);         
    void OnSPFInfo       (wxCommandEvent& event);         
    void OnSFuncList     (wxCommandEvent& event);        
    void OnSVarList      (wxCommandEvent& event);           
    void OnSExport       (wxCommandEvent& event);           

    void OnFMethodUpdate (wxUpdateUIEvent& event);           
    void OnFOneOfMethods (wxCommandEvent& event);
    void OnFRun          (wxCommandEvent& event);        
    void OnFInfo         (wxCommandEvent& event);         
    void OnFUndo         (wxCommandEvent& event);         
    void OnFRedo         (wxCommandEvent& event);         
    void OnFHistory      (wxCommandEvent& event);         
    void OnFClearH       (wxCommandEvent& event);         

    void OnLogUpdate     (wxUpdateUIEvent& event);        
    void OnLogStart      (wxCommandEvent& event);        
    void OnLogStop       (wxCommandEvent& event);        
    void OnLogWithOutput (wxCommandEvent& event);        
    void OnLogDump       (wxCommandEvent& event);        
    void OnOInclude      (wxCommandEvent& event);            
    void OnOReInclude    (wxCommandEvent& event);            
    void OnSDebugger     (wxCommandEvent&) { show_debugger(true); }            
    void show_debugger (bool show);
    void OnO_Reset       (wxCommandEvent& event);
    void OnODump         (wxCommandEvent& event);         
    void OnSettings     (wxCommandEvent& event);        
    void OnPrintPreview  (wxCommandEvent& event);
    void OnPageSetup     (wxCommandEvent& event);
    void OnPrint         (wxCommandEvent& event);
    void OnPrintPSFile   (wxCommandEvent& event);
    void OnPrintToClipboard (wxCommandEvent& event);
    void OnChangeMouseMode (wxCommandEvent& event);
    void OnUpdateFuncList(wxUpdateUIEvent& event);
    void OnChangePeakType(wxCommandEvent& event);
    void OnGMBgUpdate    (wxUpdateUIEvent& event);
    void OnStripBg       (wxCommandEvent& event);
    void OnUndoBg        (wxCommandEvent& event);
    void OnClearBg       (wxCommandEvent& event);
    void OnSplineBg      (wxCommandEvent& event);
    void GViewAll();
    void OnGViewAll      (wxCommandEvent& WXUNUSED(event)) { GViewAll(); }
    void OnGFitHeight    (wxCommandEvent& event);
    void OnGScrollLeft   (wxCommandEvent& event);
    void OnGScrollRight  (wxCommandEvent& event);
    void OnGScrollUp     (wxCommandEvent& event);
    void OnGExtendH      (wxCommandEvent& event);
    void OnPreviousZoom  (wxCommandEvent& event);
    void OnConfigRead    (wxCommandEvent& event);
    void OnConfigBuiltin (wxCommandEvent& event);
    void OnConfigSave    (wxCommandEvent& event);
    void OnGuiShowUpdate (wxUpdateUIEvent& event);
    void SwitchSideBar(bool show);
    void OnSwitchSideBar(wxCommandEvent& ev) {SwitchSideBar(ev.IsChecked());}
    void OnSwitchAuxPlot(wxCommandEvent& ev);
    void SwitchIOPane(bool show);
    void OnSwitchIOPane(wxCommandEvent& ev) {SwitchIOPane(ev.IsChecked());}
    void SwitchToolbar(bool show);
    void OnSwitchToolbar(wxCommandEvent& ev) {SwitchToolbar(ev.IsChecked());}
    void SwitchStatbar(bool show);
    void OnSwitchStatbar(wxCommandEvent& ev) {SwitchStatbar(ev.IsChecked());}
    void SwitchCrosshair(bool show);
    void OnShowPopupMenu(wxCommandEvent& ev);
    void OnConfigureStatusBar(wxCommandEvent&);
    void OnSwitchCrosshair(wxCommandEvent& ev){SwitchCrosshair(ev.IsChecked());}
    void OnSwitchFullScreen(wxCommandEvent& event);
    void OnShowMenuZoomPrev(wxUpdateUIEvent& event);
    void save_all_settings(wxConfigBase *cf) const;
    void save_settings(wxConfigBase *cf) const;
    void read_all_settings(wxConfigBase *cf);
    void read_settings(wxConfigBase *cf);
    const FToolBar* get_toolbar() const { return toolbar; }
    std::string get_peak_type() const;
    void set_status_hint(std::string const& left, std::string const& right);
    void set_status_coord_info(fp x, fp y, bool aux=false);
    void output_text(OutputStyle style, std::string const& str);
    void change_zoom(const std::string& s);
    void scroll_view_horizontally(fp step);
    void refresh_plots(bool update=false, bool only_main=false);
    void draw_crosshair(int X, int Y);
    void focus_input(int key=0);
    void edit_in_input(std::string const& s);
    void set_status_text(std::string const& text, StatusBarField field=sbf_text)
            { if (status_bar) SetStatusText(s2wx(text), field); }
    bool display_help_section(std::string const& s);
    void after_cmd_updates();
    void update_toolbar();
    void update_autoadd_enabled();
    std::string get_active_data_str();
    std::string get_in_dataset();
    std::string get_in_one_or_all_datasets();
    MainPlot* get_main_plot(); 
    MainPlot const* get_main_plot() const; 
    void update_data_pane(); 
    bool get_apply_to_all_ds();
    SideBar const* get_sidebar() const { return sidebar; }
    SideBar* get_sidebar() { return sidebar; }
    void activate_function(int n);
    void update_app_title();
    void add_recent_data_file(std::string const& filename);

protected:
    ProportionalSplitter *main_pane;
    PlotPane *plot_pane;
    IOPane *io_pane;
    SideBar *sidebar;
    FStatusBar *status_bar;

    int peak_type_nr;
    std::vector<std::string> peak_types;
    FToolBar *toolbar;
    ProportionalSplitter *v_splitter;
    PrintManager* print_mgr;
    wxHelpController help;
    std::string last_include_path;
    std::list<wxFileName> recent_data_files;
    wxMenu *data_menu_recent, *data_ft_menu;

    void place_plot_and_io_windows(wxWindow *parent);
    void create_io_panel(wxWindow *parent);
    void set_menubar();
    void update_peak_type_list();
    void read_recent_data_files();
    void write_recent_data_files();

DECLARE_EVENT_TABLE()
};

extern FFrame *frame;

#endif 

