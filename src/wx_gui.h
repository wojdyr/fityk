// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef WX_GUI__H__
#define WX_GUI__H__

#ifdef __GNUG__
#pragma interface
#endif

#include "common.h"
#include "pag.h"
#include <wx/spinctrl.h>
#include <wx/print.h>
#include <wx/html/helpctrl.h>
#ifdef __WXMSW__
#  include <wx/msw/helpbest.h>
#endif

struct z_names_type;
class MainManager;
class MyDXLoadDlg;
enum Plot_mode_enum;

enum Output_style_enum  { os_ty_normal, os_ty_warn, os_ty_quot, os_ty_input };

extern std::vector<fp> a_copy4plot;

class MyApp: public wxApp
{
public:
    wxString conf_filename, alt_conf_filename;

    bool OnInit(void);
    MainManager *main_manager;
    int OnExit();
};

DECLARE_APP(MyApp)

class MyCombo : public wxComboBox
{
public:
    MyCombo(wxWindow *parent, wxWindowID id,
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
    void read_settings();
    void save_settings();

private:
    wxColour text_color[4]; 
    wxColour bg_color;

    void fancy_dashes();

    DECLARE_EVENT_TABLE()
};

class DotSet;

class MySetDlg : public wxDialog
{
public:
    std::vector<std::string>& opt_names;
    std::vector<std::string>& opt_values;
    std::vector<wxControl*> tc_v;
    MySetDlg(wxWindow* parent, const wxWindowID id, const wxString& title,
             std::vector<std::string>& names, std::vector<std::string>& vals,
             std::vector<std::string>& types, DotSet* myset);
};

class BgToolBar : public wxToolBar
{
public:
    wxTextCtrl *tc_dist;
    BgToolBar (wxFrame *parent, wxWindowID id); 

    void OnDistText (wxCommandEvent& event); 
    void OnAddPoint (wxCommandEvent& event); 
    void OnDelPoint (wxCommandEvent& event); 
    void OnPlusBg   (wxCommandEvent& event); 
    void OnClearBg  (wxCommandEvent& event); 
    void OnInfoBg   (wxCommandEvent& event); 
    void OnSplineBg (wxCommandEvent& event); 
    void OnImportB  (wxCommandEvent& event); 
    void OnExportB  (wxCommandEvent& event); 
    void OnReturnToNormal (wxCommandEvent& event); 

private:
    DECLARE_EVENT_TABLE()
};

class SimSumToolBar : public wxToolBar
{
public:
    SimSumToolBar (wxFrame *parent, wxWindowID id); 
    void OnArrowTool (wxCommandEvent& event);
    void OnAddAuto (wxCommandEvent& event);
    void OnRmPeak (wxCommandEvent& event);
    void OnFreezeTool (wxCommandEvent& event);
    void OnTreeTool (wxCommandEvent& event);
    void OnParamTool (wxCommandEvent& event);
    void OnRParamTool (wxCommandEvent& event);
    void OnPeakSpinC (wxSpinEvent& event);
    void add_peak(fp height, fp ctr, fp hwhm);
    void add_peak_in_range (fp xmin, fp xmax);
    bool left_button_clicked (fp x, fp y);
    void set_peakspin_value (int n);
    void parameter_was_selected (int n);
    int get_selected_peak() 
                { return (peak_n_sc->IsEnabled() ? peak_n_sc->GetValue(): -1); }
private:
    std::vector<Pag> par_of_peak;
    int selected_param;
    wxChoice *peak_choice; 
    wxTextCtrl *par_val_tc;
    wxSpinCtrl *peak_n_sc;
    std::vector<const z_names_type*> all_t;
    DECLARE_EVENT_TABLE()
};

// Define a new frame
class MyFrame: public wxFrame
{
public:
    Output_win* output_win;
    Plot_common plot_common;
    MainPlot *plot;
    DiffPlot *diff_plot;
    SimSumToolBar *simsum_tb;

    MyFrame(wxWindow *parent, const wxWindowID id, const wxString& title, 
            const long style);
    ~MyFrame();
    void OnSize (wxSizeEvent& event);
    void OnKeyDown (wxKeyEvent& event);

    void OnShowHelp (wxCommandEvent& event);
    void OnTipOfTheDay (wxCommandEvent& event);
    void OnAbout (wxCommandEvent& event);
    void OnQuit (wxCommandEvent& event);

    void OnDLoad         (wxCommandEvent& event);   
    void OnDXLoad        (wxCommandEvent& event);   
    void OnDInfo         (wxCommandEvent& event);
    void OnDDeviation    (wxCommandEvent& event);
    void OnDRange        (wxCommandEvent& event);
    void OnDBackground   (wxCommandEvent& event); 
    void OnDCalibrate    (wxCommandEvent& event); 
    void OnDExport       (wxCommandEvent& event);
    void OnDSet          (wxCommandEvent& event); 

    void OnSHistory      (wxCommandEvent& event);            
    void OnSSimple       (wxCommandEvent& event);            
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

    void OnSashDrag (wxSashEvent& event);

    void set_mode (Plot_mode_enum md);
    MyCombo *get_input_combo() { return input_combo; }
    void save_all_settings();
    void read_settings();
    void read_all_settings();

protected:
    MyCombo   *input_combo;
    MyDXLoadDlg *dxload_dialog;
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

    void OnXSet (std::string name, char letter);
    void set_menubar();
    void not_implemented_menu_item (const std::string &command) const; 

DECLARE_EVENT_TABLE()
};

class MyPrintout: public wxPrintout
{
public:
    MyPrintout();
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
extern MyFrame *frame;

#endif //WXGUI__H__

