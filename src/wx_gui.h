// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef WX_GUI__H__
#define WX_GUI__H__

#ifdef __GNUG__
#pragma interface
#endif

#include "common.h"
#include "pag.h"
#include <list>
#include <wx/spinctrl.h>
#include <wx/print.h>
#include <wx/html/helpctrl.h>
#ifdef __WXMSW__
#  include <wx/msw/helpbest.h>
#endif

struct z_names_type;
struct f_names_type;
class MainManager;
class FDXLoadDlg;

enum Output_style_enum  { os_ty_normal, os_ty_warn, os_ty_quot, os_ty_input };

extern std::vector<fp> a_copy4plot;

enum { sbf_text, sbf_hint1, sbf_hint2, sbf_coord, sbf_max };  

class FApp: public wxApp
{
public:
    wxString conf_filename, alt_conf_filename;

    bool OnInit(void);
    MainManager *main_manager;
    int OnExit();
};

DECLARE_APP(FApp)

class FCombo : public wxComboBox
{
public:
    FCombo(wxWindow *parent, wxWindowID id,
                const wxString& value = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                int n = 0, const wxString choices[] = NULL,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxComboBoxNameStr)
    : wxComboBox(parent, id, value, pos, size, n, choices, style,
                    validator, name) { }
protected:
    void OnKeyDown (wxKeyEvent& event);

    DECLARE_EVENT_TABLE()
};

class Output_win : public wxTextCtrl
{
public:
    Output_win (wxWindow *parent, wxWindowID id,
                const wxPoint& pos = wxDefaultPosition, 
                const wxSize& size = wxDefaultSize);
    void append_text (const wxString& str, Output_style_enum style);
    void OnRightDown (wxMouseEvent& event);
    void OnPopupColor  (wxCommandEvent& event);       
    void OnPopupFont   (wxCommandEvent& event);  
    void OnPopupClear  (wxCommandEvent& event); 
    void OnKeyDown     (wxKeyEvent& event);
    void read_settings(wxConfigBase *cf);
    void save_settings(wxConfigBase *cf);

private:
    wxColour text_color[4]; 
    wxColour bg_color;

    void fancy_dashes();

    DECLARE_EVENT_TABLE()
};

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

private:
    wxChoice *peak_choice; 

    void OnPeakChoice (wxCommandEvent& event);
    void OnChangeMouseMode (wxCommandEvent& event);
    void OnSwitchDPane (wxCommandEvent& event);
    void OnClickTool (wxCommandEvent& event);
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
public:
    Output_win* output_win;
    Plot_common plot_common;
    MainPlot *plot;
    DiffPlot *diff_plot;
    FStatusBar *status_bar;

    FFrame(wxWindow *parent, const wxWindowID id, const wxString& title, 
            const long style);
    ~FFrame();
    void OnSize (wxSizeEvent& event);
    void OnKeyDown (wxKeyEvent& event);

    void OnShowHelp (wxCommandEvent& event);
    void OnTipOfTheDay (wxCommandEvent& event);
    void OnAbout (wxCommandEvent& event);
    void OnQuit (wxCommandEvent& event);

    void OnDLoad         (wxCommandEvent& event);   
    void OnDXLoad        (wxCommandEvent& event);   
    void OnDRecent       (wxCommandEvent& event);
    void OnDInfo         (wxCommandEvent& event);
    void OnDDeviation    (wxCommandEvent& event);
    void OnDRange        (wxCommandEvent& event);
    void OnDBInfo        (wxCommandEvent& event);
    void OnDBClear       (wxCommandEvent& event);
    void OnDCalibrate    (wxCommandEvent& event); 
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

    void OnFMethod       (wxCommandEvent& event);           
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

    void OnOPlot         (wxCommandEvent& event);         
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
    void OnModePeak      (wxCommandEvent& event);
    void OnChangePeakType(wxCommandEvent& event);
    void OnGViewAll      (wxCommandEvent& event);
    void OnGFitHeight    (wxCommandEvent& event);
    void OnPreviousZoom  (wxCommandEvent& event);
    void OnConfigRead    (wxCommandEvent& event);
    void OnConfigBuiltin (wxCommandEvent& event);
    void OnConfigSave    (wxCommandEvent& event);
    void OnSwitchDPane   (wxCommandEvent& event);
    void OnSwitchToolbar (wxCommandEvent& event);
    void OnSwitchStatbar (wxCommandEvent& event);

    void OnSashDrag (wxSashEvent& event);

    FCombo *get_input_combo() { return input_combo; }
    void save_all_settings(wxConfigBase *cf);
    void zoom_history_changed();
    void read_settings(wxConfigBase *cf);
    void read_all_settings(wxConfigBase *cf);
    const FToolBar* get_toolbar() const { return toolbar; }
    const f_names_type& get_peak_type() const;

protected:
    int peak_type_nr;
    FToolBar *toolbar;
    FCombo   *input_combo;
    FDXLoadDlg *dxload_dialog;
    wxSashLayoutWindow* plot_window;
    wxSashLayoutWindow* aux_window;
    wxSashLayoutWindow* bottom_window;
    wxPrintData *print_data;
    wxPageSetupData* page_setup_data;
#ifdef __WXMSW__
    wxBestHelpController help;
#else
    wxHtmlHelpController help; 
#endif
    wxString last_include_path;
    std::list<wxFileName> recent_data_files;
    wxMenu *data_menu_recent;

    void OnXSet (std::string name, char letter);
    void set_menubar();
    void not_implemented_menu_item (const std::string &command) const; 
    void read_recent_data_files();
    void write_recent_data_files();
    void add_recent_data_file(wxString filename);

DECLARE_EVENT_TABLE()
};


class FPrintout: public wxPrintout
{
public:
    FPrintout();
    bool HasPage(int page) { return (page == 1); }
    bool OnPrintPage(int page);
    void GetPageInfo(int *minPage,int *maxPage,int *selPageFrom,int *selPageTo)
        { *minPage = *maxPage = *selPageFrom = *selPageTo = 1; }
};


class wxConfigBase;
wxColour read_color_from_config(const wxConfigBase *config, const wxString& key,
                                const wxColour& default_value);

void write_color_to_config (wxConfigBase *config, const wxString& key,
                            const wxColour& value);
inline bool read_bool_from_config (const wxConfigBase *config, 
                                   const wxString& key, bool def_val)
                    { bool b; config->Read(key, &b, def_val); return b; }
void save_size_and_position (wxConfigBase *config, wxWindow *window);

void exec_command (const std::string& s);
void exec_command (const wxString& s);
void exec_command (const char* s);
extern FFrame *frame;

#endif //WXGUI__H__

