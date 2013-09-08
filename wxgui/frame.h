// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_FRAME_H_
#define FITYK_WX_FRAME_H_

#include <list>
#include <wx/spinctrl.h>
#include <wx/filename.h>
#include "cmn.h"  // enums
#include "plotpane.h"
#include "fityk/ui.h" // UserInterface::Style

class TextPane;
class SideBar;
class ProportionalSplitter;
class PrintManager;
class FStatusBar;

namespace fityk { class Full; struct RealRange; class Data; }
extern fityk::Full *ftk;
using fityk::UserInterface;
using fityk::RealRange;
using fityk::Data;

// execute command(s) from string
UserInterface::Status exec(const std::string &s);

/// Toolbar bar in Fityk
class FToolBar : public wxToolBar
{
public:
    FToolBar (wxFrame *parent, wxWindowID id);
    void update_peak_type(int nr, std::vector<std::string> const* peak_types);

    void OnPeakChoice (wxCommandEvent& event);
    void OnChangeMouseMode (wxCommandEvent& event);
    void OnSwitchSideBar (wxCommandEvent& event);
    void OnClickTool (wxCommandEvent& event);
    void OnToolEnter(wxCommandEvent& event);

private:
    wxChoice *peak_choice;
    void on_addpeak_hover();

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

    void OnShowHelp(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnOnline(wxCommandEvent& event);
    void OnExample(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);

    void OnDataRevertUpdate (wxUpdateUIEvent& event);
    void OnDataQLoad     (wxCommandEvent& event);
    void OnDataXLoad     (wxCommandEvent& event);
    void OnDataRecent    (wxCommandEvent& event);
    void OnDataRevert    (wxCommandEvent& event);
    void OnDataTable     (wxCommandEvent& event);
    void OnDataEditor    (wxCommandEvent& event);
    void OnSavedDT       (wxCommandEvent& event);
    void OnDataMerge     (wxCommandEvent&);
    void OnDataCalcShirley (wxCommandEvent&);
    void OnDataRmShirley (wxCommandEvent&);
    void OnDataExport    (wxCommandEvent&);

    void OnDefinitionMgr (wxCommandEvent&);
    void OnSGuess        (wxCommandEvent& event);
    void OnSPFInfo       (wxCommandEvent& event);
    void OnAutoFreeze    (wxCommandEvent& event);
    void OnParametersExport (wxCommandEvent& event);
    void OnModelExport   (wxCommandEvent& event);

    void OnFMethodUpdate (wxUpdateUIEvent& event);
    void OnMenuFitRunUpdate (wxUpdateUIEvent& event);
    void OnMenuFitUndoUpdate (wxUpdateUIEvent& event);
    void OnMenuFitRedoUpdate (wxUpdateUIEvent& event);
    void OnMenuFitHistoryUpdate (wxUpdateUIEvent& event);
    void OnFOneOfMethods (wxCommandEvent& event);
    void OnFRun          (wxCommandEvent& event);
    void OnFInfo         (wxCommandEvent& event);
    void OnFUndo         (wxCommandEvent& event);
    void OnFRedo         (wxCommandEvent& event);
    void OnFHistory      (wxCommandEvent& event);

    void OnPowderDiffraction (wxCommandEvent&);

    void OnMenuLogStartUpdate (wxUpdateUIEvent& event);
    void OnMenuLogStopUpdate (wxUpdateUIEvent& event);
    void OnMenuLogOutputUpdate (wxUpdateUIEvent& event);
    void OnLogStart      (wxCommandEvent& event);
    void OnLogStop       (wxCommandEvent& event);
    void OnLogWithOutput (wxCommandEvent& event);
    void OnSaveHistory   (wxCommandEvent& event);
    void OnInclude      (wxCommandEvent& event);
    void OnRecentScript (wxCommandEvent& event);
    void OnNewFitykScript(wxCommandEvent&);
    void OnNewLuaScript(wxCommandEvent&);
    void OnNewHistoryScript(wxCommandEvent&);
    void OnScriptEdit   (wxCommandEvent&);
    void show_editor (const wxString& path, const wxString& content);
    void OnReset       (wxCommandEvent&);
#ifdef __WXMAC__
    void OnNewWindow   (wxCommandEvent&);
#endif
    void OnSessionLoad(wxCommandEvent&);
    void OnSessionSave(wxCommandEvent&);
    void OnSettings      (wxCommandEvent&);
    void OnEditInit      (wxCommandEvent&);
    void OnPageSetup     (wxCommandEvent&);
    void OnPrint         (wxCommandEvent&);
    void OnCopyToClipboard (wxCommandEvent&);
    void OnSaveAsImage (wxCommandEvent&);
    void OnChangeMouseMode (wxCommandEvent&);
    void OnChangePeakType(wxCommandEvent& event);
    void OnMenuBgStripUpdate(wxUpdateUIEvent& event);
    void OnMenuBgUndoUpdate(wxUpdateUIEvent& event);
    void OnMenuBgClearUpdate(wxUpdateUIEvent& event);
    void OnStripBg       (wxCommandEvent& event);
    void OnUndoBg        (wxCommandEvent& event);
    void OnClearBg       (wxCommandEvent& event);
    void OnRecentBg      (wxCommandEvent& event);
    void OnConvexHullBg  (wxCommandEvent& event);
    void OnSplineBg      (wxCommandEvent& event);
    void GViewAll();
    void OnGViewAll      (wxCommandEvent&) { GViewAll(); }
    void OnGFitHeight    (wxCommandEvent& event);
    void OnGScrollLeft   (wxCommandEvent& event);
    void OnGScrollRight  (wxCommandEvent& event);
    void OnGScrollUp     (wxCommandEvent& event);
    void OnGExtendH      (wxCommandEvent& event);
    void OnPreviousZoom  (wxCommandEvent& event);
    void OnConfigBuiltin (wxCommandEvent& event);
    void OnConfigX (wxCommandEvent& event);
    void OnSaveDefaultConfig(wxCommandEvent& event);
    void OnSaveConfigAs(wxCommandEvent&);
    void OnMenuShowAuxUpdate(wxUpdateUIEvent& event);
    void SwitchSideBar(bool show);
    void OnSwitchSideBar(wxCommandEvent& ev) {SwitchSideBar(ev.IsChecked());}
    void OnSwitchAuxPlot(wxCommandEvent& ev);
    void SwitchTextPane(bool show);
    void OnSwitchTextPane(wxCommandEvent& ev) {SwitchTextPane(ev.IsChecked());}
    void SwitchToolbar(bool show);
    void OnSwitchToolbar(wxCommandEvent& ev) {SwitchToolbar(ev.IsChecked());}
    void SwitchStatbar(bool show);
    void OnSwitchStatbar(wxCommandEvent& ev) {SwitchStatbar(ev.IsChecked());}
    void SwitchCrosshair(bool show);
    void OnShowPrefDialog(wxCommandEvent& ev);
    void OnConfigureStatusBar(wxCommandEvent& event);
    void OnConfigureOutputWin(wxCommandEvent&);
    void OnConfigureDirectories(wxCommandEvent&);
    void OnSwitchCrosshair(wxCommandEvent& e) {SwitchCrosshair(e.IsChecked());}
    void OnSwitchAntialias(wxCommandEvent& event);
    void OnSwitchFullScreen(wxCommandEvent& event);
    void OnGShowY0(wxCommandEvent& e);
    void save_config_as(wxString const& name);
    void read_config(wxString const& name);
    void save_all_settings(wxConfigBase *cf) const;
    void save_settings(wxConfigBase *cf) const;
    void read_all_settings(wxConfigBase *cf);
    void read_settings(wxConfigBase *cf);
    const FToolBar* get_toolbar() const { return toolbar_; }
    std::string get_peak_type() const;
    void set_status_text(std::string const& text);
    void set_status_coords(double x, double y, PlotTypeEnum pte);
    void clear_status_coords();
    void output_text(UserInterface::Style style, std::string const& str);
    void change_zoom(const RealRange& h, const RealRange& v);
    void scroll_view_horizontally(double step);
    void focus_input(wxKeyEvent& event);
    void edit_in_input(std::string const& s);
    void after_cmd_updates();
    void update_toolbar();
    void update_config_menu(wxMenu *menu);
    int get_focused_data_index();
    std::vector<int> get_selected_data_indices();
    std::vector<Data*> get_selected_datas();
    std::string get_datasets();
    std::string get_guess_string(const std::string& name);
    PlotPane *plot_pane() { return plot_pane_; }
    ZoomHistory& zoom_hist() { return zoom_hist_; }
    MainPlot* get_main_plot();
    MainPlot const* get_main_plot() const;
    void update_data_pane();
    SideBar const* get_sidebar() const { return sidebar_; }
    SideBar* get_sidebar() { return sidebar_; }
    void activate_function(int n);
    void update_app_title();
    void add_recent_data_file(std::string const& filename);
    void update_menu_functions();
    void update_menu_saved_transforms();
    void update_menu_recent_baselines();
    void update_menu_previous_zooms();
    // overridden from wxFrameBase, to show help in our status bar replacement
    void DoGiveHelp(const wxString& help, bool show);
    bool antialias() const { return antialias_; }
    // script_dir_ is accessed by EditorDlg
    const wxString& script_dir() const { return script_dir_; }
    void set_script_dir(const wxString& s) { script_dir_ = s; }

private:
    ProportionalSplitter *v_splitter_;
    ProportionalSplitter *main_pane_;
    PlotPane *plot_pane_;
    TextPane *text_pane_;
    SideBar *sidebar_;
    FStatusBar *status_bar_;
    FToolBar *toolbar_;
    ZoomHistory zoom_hist_;

    int peak_type_nr_;
    std::vector<std::string> peak_types_;
    PrintManager* print_mgr_;
    std::list<wxFileName> recent_script_files_;
    std::list<wxFileName> recent_data_files_;
    wxMenu *data_menu_recent_, *session_menu_recent_,
           *data_ft_menu_, *func_type_menu_;
    wxString script_dir_, data_dir_, export_dir_;
    bool antialias_;

    void place_plot_and_io_windows(wxWindow *parent);
    void create_io_panel(wxWindow *parent);
    void set_menubar();
    wxMenu* add_recent_menu(const std::list<wxFileName>& files, int id);
    void add_recent_file(std::string const& filename, wxMenu* menu_recent,
                         std::list<wxFileName> &recent_files, int base_id);
    void update_peak_type_list();
    void change_mouse_mode(MouseModeEnum mode);
    void export_as_info(const std::string& info, const char* caption,
                        const char* ext, const char* wildcards);

    DECLARE_EVENT_TABLE()
};

extern FFrame *frame;

#endif // FITYK_WX_FRAME_H_

