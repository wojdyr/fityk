// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$


// wxwindows headers, see wxwindows samples for description
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/fontdlg.h>
#include <wx/treectrl.h>
#include <wx/notebook.h>
#include <wx/textdlg.h>
#include <wx/spinctrl.h>

#include "common.h"
#include "wx_pane.h" 
#include "wx_gui.h" 
#include "wx_plot.h" 
#include "wx_mplot.h" 
#include "data.h" 
#include "logic.h" 
#include "ui.h" 
#include "sum.h" 

#include "img/add.xpm"
#include "img/sum.xpm"
#include "img/rename.xpm"
#include "img/close.xpm"
#include "img/colorsel.xpm"
#include "img/editf.xpm"
#include "img/filter.xpm"
#include "img/shiftup.xpm"
#include "img/convert.xpm"
#include "img/color.xpm"
#include "img/copyfunc.xpm"
#include "img/unused.xpm"
#include "img/zshift.xpm"

using namespace std;

enum { 
    ID_OUTPUT_C_BG      = 27001,
    ID_OUTPUT_C_IN             ,
    ID_OUTPUT_C_OU             ,
    ID_OUTPUT_C_QT             ,
    ID_OUTPUT_C_WR             ,
    ID_OUTPUT_C                ,
    ID_OUTPUT_P_FONT           ,
    ID_OUTPUT_P_CLEAR          ,
    ID_DP_LIST                 ,
    ID_DP_LOOK                 ,
    ID_DP_SHIFTUP              ,
    ID_DP_NEW                  ,
    ID_DP_DUP                  ,
    ID_DP_REN                  ,
    ID_DP_DEL                  ,
    ID_DP_CPF                  ,
    ID_DP_COL                  ,
    ID_FP_FILTER               ,
    ID_FP_LIST                 ,
    ID_FP_NEW                  ,
    ID_FP_DEL                  ,
    ID_FP_EDIT                 ,
    ID_FP_CHTYPE               ,
    ID_FP_COL                  ,
    ID_VP_LIST                 ,
    ID_VP_NEW                  ,
    ID_VP_DEL                  ,
    ID_VP_EDIT                 ,
    ID_DL_CMENU_SHOW_START     ,
    ID_DL_CMENU_SHOW_END = ID_DL_CMENU_SHOW_START+20,
    ID_DL_CMENU_FITCOLS        ,
    ID_DL_SELECTALL
};


//===============================================================
//                            PlotPane
//===============================================================

BEGIN_EVENT_TABLE(PlotPane, ProportionalSplitter)
END_EVENT_TABLE()

PlotPane::PlotPane(wxWindow *parent, wxWindowID id)
    : ProportionalSplitter(parent, id, 0.75), 
      crosshair_cursor(false), plot_shared() 
{
    plot = new MainPlot(this, plot_shared);
    aux_split = new ProportionalSplitter(this, -1, 0.5);
    SplitHorizontally(plot, aux_split);

    aux_plot[0] = new AuxPlot(aux_split, plot_shared, "0");
    aux_plot[1] = new AuxPlot(aux_split, plot_shared, "1");
    aux_plot[1]->Show(false);
    aux_split->Initialize(aux_plot[0]);
}

void PlotPane::zoom_forward()
{
    const int max_length_of_zoom_history = 10;
    zoom_hist.push_back(AL->view.str());
    if (size(zoom_hist) > max_length_of_zoom_history)
        zoom_hist.erase(zoom_hist.begin());
}

string PlotPane::zoom_backward(int n)
{
    if (n < 1 || zoom_hist.empty()) 
        return "";
    int pos = zoom_hist.size() - n;
    if (pos < 0) pos = 0;
    string val = zoom_hist[pos];
    zoom_hist.erase(zoom_hist.begin() + pos, zoom_hist.end());
    return val;
}

void PlotPane::save_settings(wxConfigBase *cf) const
{
    cf->SetPath("/PlotPane");
    cf->Write("PlotPaneProportion", GetProportion());
    cf->Write("AuxPlotsProportion", aux_split->GetProportion());
    cf->Write("ShowAuxPane0", aux_visible(0));
    cf->Write("ShowAuxPane1", aux_visible(1));
    plot->save_settings(cf);
    for (int i = 0; i < 2; ++i)
        aux_plot[i]->save_settings(cf);
}

void PlotPane::read_settings(wxConfigBase *cf)
{
    cf->SetPath("/PlotPane");
    SetProportion(read_double_from_config(cf, "PlotPaneProportion", 0.75));
    aux_split->SetProportion(read_double_from_config(cf, "AuxPlotsProportion", 
                                                         0.5));
    show_aux(0, read_bool_from_config(cf, "ShowAuxPane0", true));
    show_aux(1, read_bool_from_config(cf, "ShowAuxPane1", false));
    plot->read_settings(cf);
    for (int i = 0; i < 2; ++i)
        aux_plot[i]->read_settings(cf);
}

void PlotPane::refresh_plots(bool refresh, bool update, bool only_main)
{
    if (only_main) {
        if (refresh)
            plot->Refresh(false);
        if (update) 
            plot->Update();
        return;
    }
    // not only main
    vector<FPlot*> vp = get_visible_plots();
    for (vector<FPlot*>::const_iterator i = vp.begin(); i != vp.end(); ++i) {
        if (refresh)
            (*i)->Refresh(false);
        if (update) 
            (*i)->Update();
    }
}

void PlotPane::set_mouse_mode(MouseModeEnum m) 
{ 
    plot->set_mouse_mode(m); 
}

void PlotPane::update_mouse_hints() 
{ 
    plot->update_mouse_hints();
}

bool PlotPane::is_background_white() 
{ 
    //have all visible plots white background?
    vector<FPlot*> vp = get_visible_plots();
    for (vector<FPlot*>::const_iterator i = vp.begin(); i != vp.end(); ++i) 
        if ((*i)->get_bg_color() != *wxWHITE)
            return false;
    return true;
}

BgManager* PlotPane::get_bg_manager() { return plot; }

const std::vector<FPlot*> PlotPane::get_visible_plots() const
{
    vector<FPlot*> visible;
    visible.push_back(plot);
    for (int i = 0; i < 2; ++i)
        if (aux_visible(i))
            visible.push_back(aux_plot[i]);
    return visible;
}

bool PlotPane::aux_visible(int n) const
{
    return IsSplit() && (aux_split->GetWindow1() == aux_plot[n]
                         || aux_split->GetWindow2() == aux_plot[n]);
}

void PlotPane::show_aux(int n, bool show)
{
    if (aux_visible(n) == show) return;

    if (show) {
        if (!IsSplit()) { //both where invisible
            SplitHorizontally(plot, aux_split);
            aux_split->Show(true);
            assert(!aux_split->IsSplit());
            if (aux_split->GetWindow1() == aux_plot[n])
                ;
            else {
                aux_split->SplitHorizontally(aux_plot[0], aux_plot[1]);
                aux_plot[n]->Show(true);
                aux_split->Unsplit(aux_plot[n==0 ? 1 : 0]);
            }
        }
        else {//one was invisible
            aux_split->SplitHorizontally(aux_plot[0], aux_plot[1]);
            aux_plot[n]->Show(true);
        }
    }
    else { //hide
        if (aux_split->IsSplit()) //both where visible
            aux_split->Unsplit(aux_plot[n]);
        else // only one was visible
            Unsplit(); //hide whole aux_split
    }
}


/// draw "crosshair cursor" -> erase old and draw new
void PlotPane::draw_crosshair(int X, int Y)
{
    static bool drawn = false;
    static int oldX = 0, oldY = 0;
    if (drawn) {
        do_draw_crosshair(oldX, oldY);
        drawn = false;
    }
    if (crosshair_cursor && X >= 0) {
        do_draw_crosshair(X, Y);
        oldX = X;
        oldY = Y;
        drawn = true;
    }
}

void PlotPane::do_draw_crosshair(int X, int Y)
{
    plot->draw_crosshair(X, Y);
    for (int i = 0; i < 2; ++i)
        if (aux_visible(i))
            aux_plot[i]->draw_crosshair(X, -1);
}


//===============================================================
//                            IOPane
//===============================================================

BEGIN_EVENT_TABLE(IOPane, wxPanel)
END_EVENT_TABLE()

IOPane::IOPane(wxWindow *parent, wxWindowID id)
    : wxPanel(parent, id), output_win(0), input_field(0)
{
    wxBoxSizer *io_sizer = new wxBoxSizer (wxVERTICAL);

    output_win = new OutputWin (this, -1);
    io_sizer->Add (output_win, 1, wxEXPAND);

    input_field = new InputField (this, -1, "",
                            wxDefaultPosition, wxDefaultSize, 
                            wxWANTS_CHARS|wxTE_PROCESS_ENTER|wxTE_PROCESS_TAB);
    io_sizer->Add (input_field, 0, wxEXPAND);

    SetSizer(io_sizer);
    io_sizer->SetSizeHints(this);
}

void IOPane::save_settings(wxConfigBase *cf) const
{
    output_win->save_settings(cf);
}

void IOPane::read_settings(wxConfigBase *cf)
{
    output_win->read_settings(cf);
}

void IOPane::focus_input(int key) 
{ 
    input_field->SetFocus(); 
    if (key != WXK_TAB && key > 0 && key < 256) {
        input_field->WriteText(wxString(static_cast<char>(key)).Lower());
    }
}

void IOPane::edit_in_input(string const& s) 
{
    input_field->WriteText(s.c_str());
    input_field->SetFocus(); 
}

//===============================================================
//                            SideBar
//===============================================================

void add_bitmap_button(wxWindow* parent, wxWindowID id, char** xpm,
                       char const* tip, wxSizer* sizer)
{
    wxBitmapButton *btn = new wxBitmapButton(parent, id, wxBitmap(xpm));
    btn->SetToolTip(tip);
    sizer->Add(btn);
}


BEGIN_EVENT_TABLE(SideBar, ProportionalSplitter)
    EVT_BUTTON (ID_DP_NEW, SideBar::OnDataButtonNew)
    EVT_BUTTON (ID_DP_DUP, SideBar::OnDataButtonDup)
    EVT_BUTTON (ID_DP_REN, SideBar::OnDataButtonRen)
    EVT_BUTTON (ID_DP_DEL, SideBar::OnDataButtonDel)
    EVT_BUTTON (ID_DP_CPF, SideBar::OnDataButtonCopyF)
    EVT_BUTTON (ID_DP_COL, SideBar::OnDataButtonCol)
    EVT_CHOICE (ID_DP_LOOK, SideBar::OnDataLookChanged)
    EVT_SPINCTRL (ID_DP_SHIFTUP, SideBar::OnDataShiftUpChanged)
    EVT_LIST_ITEM_SELECTED(ID_DP_LIST, SideBar::OnDataFocusChanged)
    EVT_LIST_ITEM_DESELECTED(ID_DP_LIST, SideBar::OnDataFocusChanged)
    EVT_LIST_ITEM_FOCUSED(ID_DP_LIST, SideBar::OnDataFocusChanged)
    EVT_CHOICE (ID_FP_FILTER, SideBar::OnFuncFilterChanged)
    EVT_BUTTON (ID_FP_NEW, SideBar::OnFuncButtonNew)
    EVT_BUTTON (ID_FP_DEL, SideBar::OnFuncButtonDel)
    EVT_BUTTON (ID_FP_EDIT, SideBar::OnFuncButtonEdit)
    EVT_BUTTON (ID_FP_CHTYPE, SideBar::OnFuncButtonChType)
    EVT_BUTTON (ID_FP_COL, SideBar::OnFuncButtonCol)
    EVT_LIST_ITEM_FOCUSED(ID_FP_LIST, SideBar::OnFuncFocusChanged)
END_EVENT_TABLE()

SideBar::SideBar(wxWindow *parent, wxWindowID id)
: ProportionalSplitter(parent, id, 0.75) 
{
    //wxPanel *upper = new wxPanel(this, -1);
    //wxBoxSizer *upper_sizer = new wxBoxSizer(wxVERTICAL);
    nb = new wxNotebook(this, -1);
    //upper_sizer->Add(nb, 1, wxEXPAND);
    //upper->SetSizerAndFit(upper_sizer);

    //-----  data page  -----
    data_page = new wxPanel(nb, -1);
    wxBoxSizer *data_sizer = new wxBoxSizer(wxVERTICAL);
    dl = new ListWithColors(data_page, ID_DP_LIST, 
                            vector4(pair<string,bool>("No", true),
                                    pair<string,bool>("#F+#Z", true),
                                    pair<string,bool>("Name", true),
                                    pair<string,bool>("File", false)));
    data_sizer->Add(dl, 1, wxEXPAND|wxALL, 1);

    wxBoxSizer *data_look_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_bitmap_button(data_page, ID_DP_COL, colorsel_xpm, 
                      "change color", data_look_sizer);
    wxChar const *choices[] = { "show all datasets", "show only selected", 
                                "shadow unselected" };
    int choices_len = sizeof(choices)/sizeof(choices[0]);
    data_look = new wxChoice(data_page, ID_DP_LOOK,
                             wxDefaultPosition, wxDefaultSize,
                             wxArrayString(choices_len, choices));
    data_look_sizer->Add(data_look, 1, wxEXPAND);
    data_look_sizer->Add(new wxStaticBitmap(data_page, -1, 
                                            wxBitmap(shiftup_xpm)), 
                         0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    shiftup_sc = new wxSpinCtrl(data_page, ID_DP_SHIFTUP, "0",
                                wxDefaultPosition, wxSize(40, -1),
                                wxSP_ARROW_KEYS, 0, 80, 0);
    shiftup_sc->SetToolTip("shift up in \% of plot Y size");
    data_look_sizer->Add(shiftup_sc, 0, wxEXPAND);
    data_sizer->Add(data_look_sizer, 0, wxEXPAND);

    wxBoxSizer *data_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_bitmap_button(data_page, ID_DP_NEW, add_xpm, 
                      "new data", data_buttons_sizer);
    add_bitmap_button(data_page, ID_DP_DUP, sum_xpm, 
                      "duplicate/sum", data_buttons_sizer);
    add_bitmap_button(data_page, ID_DP_REN, rename_xpm, 
                      "rename", data_buttons_sizer);
    add_bitmap_button(data_page, ID_DP_DEL, close_xpm, 
                      "delete", data_buttons_sizer);
    add_bitmap_button(data_page, ID_DP_CPF, copyfunc_xpm, 
                      "copy F to next dataset", data_buttons_sizer);
    data_sizer->Add(data_buttons_sizer, 0, wxEXPAND);
    data_page->SetSizerAndFit(data_sizer);
    nb->AddPage(data_page, "data");

    //-----  functions page  -----
    wxPanel *func_page = new wxPanel(nb, -1);
    wxBoxSizer *func_sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *func_filter_sizer = new wxBoxSizer(wxHORIZONTAL);
    func_filter_sizer->Add(new wxStaticBitmap(func_page, -1, 
                                              wxBitmap(filter_xpm)),
                           0, wxALIGN_CENTER_VERTICAL);
    wxArrayString filter_choices; 
    filter_choices.Add("list all functions");
    filter_ch = new wxChoice(func_page, ID_FP_FILTER,
                             wxDefaultPosition, wxDefaultSize, filter_choices);
    func_filter_sizer->Add(filter_ch, 1, wxEXPAND);
    func_sizer->Add(func_filter_sizer, 0, wxEXPAND);

    vector<pair<string,bool> > fdata;
    fdata.push_back( pair<string,bool>("Name", true) );
    fdata.push_back( pair<string,bool>("Type", true) );
    fdata.push_back( pair<string,bool>("Center", true) );
    fdata.push_back( pair<string,bool>("Area", false) );
    fdata.push_back( pair<string,bool>("Height", false) );
    fdata.push_back( pair<string,bool>("FWHM", false) );
    fl = new ListWithColors(func_page, ID_FP_LIST, fdata);
    func_sizer->Add(fl, 1, wxEXPAND|wxALL, 1);
    wxBoxSizer *func_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_bitmap_button(func_page, ID_FP_NEW, add_xpm, 
                      "new function", func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_DEL, close_xpm, 
                      "delete", func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_EDIT, editf_xpm, 
                      "edit function", func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_CHTYPE, convert_xpm, 
                      "change type of function", func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_COL, colorsel_xpm, 
                      "change color", func_buttons_sizer);
    func_sizer->Add(func_buttons_sizer, 0, wxEXPAND);
    func_page->SetSizerAndFit(func_sizer);
    nb->AddPage(func_page, "functions");

    //-----  variables page  -----
    wxPanel *var_page = new wxPanel(nb, -1);
    wxBoxSizer *var_sizer = new wxBoxSizer(wxVERTICAL);
    vector<pair<string,bool> > vdata = vector4(
                                     pair<string,bool>("Name", true),
                                     pair<string,bool>("#/#", true), 
                                     pair<string,bool>("value", true),
                                     pair<string,bool>("formula", false) );
    vl = new ListWithColors(var_page, ID_VP_LIST, vdata);
    var_sizer->Add(vl, 1, wxEXPAND|wxALL, 1);
    wxBoxSizer *var_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_bitmap_button(var_page, ID_VP_NEW, add_xpm, 
                      "new variable", var_buttons_sizer);
    add_bitmap_button(var_page, ID_VP_DEL, close_xpm, 
                      "delete", var_buttons_sizer);
    add_bitmap_button(var_page, ID_VP_EDIT, editf_xpm, 
                      "edit variable", var_buttons_sizer);
    var_sizer->Add(var_buttons_sizer, 0, wxEXPAND);
    var_page->SetSizerAndFit(var_sizer);
    nb->AddPage(var_page, "variables");

    //-----
    wxPanel *bottom = new wxPanel(this, -1);
    SplitHorizontally(nb, bottom);
}

void SideBar::OnDataButtonNew (wxCommandEvent& WXUNUSED(event))
{
    exec_command("@+");
}

void SideBar::OnDataButtonDup (wxCommandEvent& WXUNUSED(event))
{
    exec_command("@+ < " + join_vector(get_selected_data(), " + "));
}

void SideBar::OnDataButtonRen (wxCommandEvent& WXUNUSED(event))
{
    int n = get_focused_data();
    Data *data = AL->get_data(n);
    wxString old_title = data->get_title().c_str();

    wxString s = wxGetTextFromUser(wxString("New name for dataset @") 
                                                              + S(n).c_str(), 
                                   "Rename dataset",
                                   old_title);
    if (!s.IsEmpty() && s != old_title)
        exec_command("@" + S(n) + ".title = \"" + s.c_str() + "\"");
}

void SideBar::OnDataButtonDel (wxCommandEvent& WXUNUSED(event))
{
    exec_command("delete " + join_vector(get_selected_data(), ", "));
}

void SideBar::OnDataButtonCopyF (wxCommandEvent& WXUNUSED(event))
{
    int n = get_focused_data();
    if (n+1 < AL->get_ds_count())
        exec_command("@" + S(n+1) + ".F=copy(@" + S(n) + ".F); "
                     "@" + S(n+1) + ".Z=copy(@" + S(n) + ".Z)");
}

void SideBar::OnDataButtonCol (wxCommandEvent& WXUNUSED(event))
{
    int n = get_focused_data();
    wxColour col = frame->get_main_plot()->get_data_color(n);
    if (change_color_dlg(col)) {
        frame->get_main_plot()->set_data_color(n, col);
        update_lists();
        frame->refresh_plots(true, false, true);
    }
}

void SideBar::OnDataLookChanged (wxCommandEvent& WXUNUSED(event))
{
    frame->refresh_plots(true, false, true);
}

void SideBar::OnDataShiftUpChanged (wxSpinEvent& WXUNUSED(event))
{
    frame->refresh_plots(true, false, true);
}

void SideBar::OnFuncFilterChanged (wxCommandEvent& WXUNUSED(event))
{
    update_lists(false);
}

void SideBar::OnFuncButtonNew (wxCommandEvent& WXUNUSED(event))
{
    //TODO
}

void SideBar::OnFuncButtonDel (wxCommandEvent& WXUNUSED(event))
{
    exec_command("delete " + join_vector(get_selected_func(), ", "));
}

void SideBar::OnFuncButtonEdit (wxCommandEvent& WXUNUSED(event))
{
    int n = get_focused_func();
    string t= AL->get_function(n)->get_current_definition(AL->get_variables(),
                                                          AL->get_parameters());
    frame->edit_in_input(t);
}

void SideBar::OnFuncButtonChType (wxCommandEvent& WXUNUSED(event))
{
}

void SideBar::OnFuncButtonCol (wxCommandEvent& WXUNUSED(event))
{
    int n = get_focused_func();
    wxColour col = frame->get_main_plot()->get_func_color(n);
    if (change_color_dlg(col)) {
        frame->get_main_plot()->set_func_color(n, col);
        update_lists();
        frame->refresh_plots(true, false, true);
    }
}

void SideBar::update_lists(bool nondata_changed)
{
    MainPlot const* mplot = frame->get_main_plot();
    wxColour const& bg_col = mplot->get_bg_color();
    //data
    vector<string> data_data;
    for (int i = 0; i < AL->get_ds_count(); ++i) {
        DataWithSum const* ds = AL->get_ds(i);
        data_data.push_back(S(i));
        data_data.push_back(S(ds->get_sum()->get_ff_count()) 
                        + "+" + S(ds->get_sum()->get_zz_count()));
        data_data.push_back(ds->get_data()->get_title());
        data_data.push_back(ds->get_data()->get_filename());
    }
    wxImageList* data_images = 0;
    if (nondata_changed || AL->get_ds_count() > dl->GetItemCount()) {
        data_images = new wxImageList(16, 16);
        for (int i = 0; i < AL->get_ds_count(); ++i) {
            wxColour const& d_col = mplot->get_data_color(i);
            wxImage image(color_xpm);
            image.Replace(0, 0, 0, bg_col.Red(), bg_col.Green(), bg_col.Blue());
            image.Replace(255, 255, 255,  
                          d_col.Red(), d_col.Green(), d_col.Blue());
            data_images->Add(wxBitmap(image));
        }
    }
    int active_ds_pos = AL->get_active_ds_position();
    dl->populate(data_data, data_images, active_ds_pos);

    //functions filter
    if (AL->get_ds_count()+1 != filter_ch->GetCount()) {
        while (filter_ch->GetCount() > AL->get_ds_count()+1)
            filter_ch->Delete(filter_ch->GetCount()-1);
        for (int i = filter_ch->GetCount()-1; i < AL->get_ds_count(); ++i)
            filter_ch->Append("only functions from @" + S(i));
    }

    //functions
    static vector<int> func_col_id;
    vector<string> func_data;
    vector<int> new_func_col_id;
    Sum const* sum = AL->get_sum(active_ds_pos);
    Sum const* filter_sum = 0;
    if (filter_ch->GetSelection() > 0)
        filter_sum = AL->get_sum(filter_ch->GetSelection()-1);
    for (int i = 0; i < size(AL->get_functions()); ++i) {
        if (filter_sum && !contains_element(filter_sum->get_ff_idx(), i)
                           && !contains_element(filter_sum->get_zz_idx(), i))
            continue;
        Function const* f = AL->get_function(i);
        func_data.push_back(f->name);
        func_data.push_back(f->type_name);
        func_data.push_back(f->has_center() ? S(f->center()).c_str() : "-");
        func_data.push_back(f->is_peak() ? S(f->area()).c_str() : "-");
        func_data.push_back(f->is_peak() ? S(f->height()).c_str() : "-");
        func_data.push_back(f->is_peak() ? S(f->fwhm()).c_str() : "-");
        vector<int> const& ffi = sum->get_ff_idx();
        vector<int> const& zzi = sum->get_zz_idx();
        vector<int>::const_iterator in_ff = find(ffi.begin(), ffi.end(), i);
        int col_id = -2;
        if (in_ff != ffi.end())
            col_id = in_ff - ffi.begin();
        else if (find(zzi.begin(), zzi.end(), i) != zzi.end())
            col_id = -1;
        new_func_col_id.push_back(col_id);
    }
    wxImageList* func_images = 0;
    if (nondata_changed || func_col_id != new_func_col_id) {
        func_col_id = new_func_col_id;
        func_images = new wxImageList(16, 16);
        for (vector<int>::const_iterator i = func_col_id.begin(); 
                                                i != func_col_id.end(); ++i) {
            if (*i == -2)
                func_images->Add(wxBitmap(unused_xpm));
            else if (*i == -1)
                func_images->Add(wxBitmap(zshift_xpm));
            else {
                wxColour const& d_col = mplot->get_func_color(*i);
                wxImage image(color_xpm);
                image.Replace(0, 0, 0, 
                              bg_col.Red(), bg_col.Green(), bg_col.Blue());
                image.Replace(255, 255, 255,  
                              d_col.Red(), d_col.Green(), d_col.Blue());
                func_images->Add(wxBitmap(image));
            }
        }
    }
    fl->populate(func_data, func_images, 0);

    //variables
    vector<string> var_data;
    //  count references first
    vector<Variable*> const& variables = AL->get_variables();
    vector<int> var_vrefs(variables.size(), 0), var_frefs(variables.size(), 0);
    for (vector<Variable*>::const_iterator i = variables.begin();
                                                i != variables.end(); ++i) {
        vector<int> const& var_idx = (*i)->get_var_idx(); 
        for (vector<int>::const_iterator j = var_idx.begin(); 
                                                    j != var_idx.end(); ++j)
            var_vrefs[*j]++;
    }
    for (vector<Function*>::const_iterator i = AL->get_functions().begin();
                                         i != AL->get_functions().end(); ++i) {
        vector<int> const& var_idx = (*i)->get_var_idx(); 
        for (vector<int>::const_iterator j = var_idx.begin(); 
                                                    j != var_idx.end(); ++j)
            var_frefs[*j]++;
    }

    for (int i = 0; i < size(variables); ++i) {
        Variable const* v = variables[i];
        var_data.push_back(v->name);           //name
        string refs = S(var_frefs[i]) + "+" + S(var_vrefs[i]) + " / " 
                      + S(v->get_vars_count());
        var_data.push_back(refs); //refs
        var_data.push_back(S(v->get_value())); //value
        var_data.push_back(v->get_formula(AL->get_parameters()));  //formula
    }
    vl->populate(var_data);
    
    //-- non-lists
    bool not_the_last = get_focused_data()+1 < AL->get_ds_count();
    data_page->FindWindow(ID_DP_CPF)->Enable(not_the_last);
}

void SideBar::activate_function(int n)
{
    for (int i = 0; i != fl->GetItemCount(); ++i)
        fl->Select(i, i==n);
    frame->refresh_plots(true, false, true);
}

vector<string> SideBar::get_selected_data() const
{
    vector<string> dd;
    for (int i = dl->GetFirstSelected(); i != -1; i = dl->GetNextSelected(i))
        dd.push_back("@" + S(i));
    if (dd.empty()) {
        int n = dl->GetFocusedItem();
        dd.push_back("@" + S(n == -1 ? 0 : n));
    }
    return dd;
}

void SideBar::OnDataFocusChanged(wxListEvent& WXUNUSED(event))
{
    int n = dl->GetFocusedItem();
    int length = AL->get_ds_count();
    if (n >= 0 && length > 1 && AL->get_active_ds_position() != n) 
        exec_command("plot @" + S(n));
    data_page->FindWindow(ID_DP_CPF)->Enable(n+1<length);
}

bool SideBar::howto_plot_dataset(int n, bool& shadowed, int& offset) const
{
    //wxChar const *choices[] = { "show all datasets", "show only selected", 
    //                           "shadow unselected", 
    //                           "draw aside vert. (10px)", 
    //                           "draw aside vert. (30px)" };
    int choice_idx = data_look->GetSelection();
    bool sel = dl->IsSelected(n) || dl->GetFocusedItem() == n;
    if (choice_idx == 1 && !sel)
        return false;
    shadowed = (choice_idx == 2 && !sel);
    offset = n * shiftup_sc->GetValue();
    return true;
}

vector<string> SideBar::get_selected_func() const
{
    vector<string> dd;
    for (int i = fl->GetFirstSelected(); i != -1; i = fl->GetNextSelected(i))
        dd.push_back(AL->get_function(i)->xname);
    if (dd.empty()) {
        int n = fl->GetFocusedItem();
        dd.push_back(AL->get_function(n == -1 ? 0 : n)->xname);
    }
    return dd;
}

void SideBar::OnFuncFocusChanged(wxListEvent& WXUNUSED(event))
{
    frame->refresh_plots(true, false, true);
}


//===============================================================
//                            ListWithColors
//===============================================================

BEGIN_EVENT_TABLE(ListWithColors, wxListView)
    EVT_LIST_COL_CLICK(-1, ListWithColors::OnColumnMenu)
    EVT_LIST_COL_RIGHT_CLICK(-1, ListWithColors::OnColumnMenu)
    EVT_RIGHT_DOWN(              ListWithColors::OnRightDown)
    EVT_MENU_RANGE (ID_DL_CMENU_SHOW_START, ID_DL_CMENU_SHOW_END, 
                    ListWithColors::OnShowColumn)
    EVT_MENU (ID_DL_CMENU_FITCOLS, ListWithColors::OnFitColumnWidths)
    EVT_MENU (ID_DL_SELECTALL, ListWithColors::OnSelectAll)
END_EVENT_TABLE()
    
ListWithColors::ListWithColors(wxWindow *parent, wxWindowID id, 
                               vector<pair<string,bool> > const& columns_)
    : wxListView(parent, id, wxDefaultPosition, wxDefaultSize,
                 wxLC_REPORT|wxLC_HRULES|wxLC_VRULES),
      columns(columns_)
{
    for (int i = 0; i < size(columns); ++i)
        if (columns[i].second)
            InsertColumn(i, columns[i].first.c_str(), wxLIST_FORMAT_LEFT);
}

void ListWithColors::populate(vector<string> const& data, 
                              wxImageList* image_list,
                              int active)
{
    assert(data.size() % columns.size() == 0);
    if (!image_list && data == list_data)
        return;
    int length = data.size() / columns.size();
    Freeze();
    if (image_list) 
        AssignImageList(image_list, wxIMAGE_LIST_SMALL);
    if (GetItemCount() != length) {
        DeleteAllItems();
        for (int i = 0; i < length; ++i)
            InsertItem(i, "");
    }
    for (int i = 0; i < length; ++i) {
        int c = 0;
        for (int j = 0; j < size(columns); ++j) {
            if (columns[j].second) {
                SetItem(i, c, data[i*columns.size()+j].c_str(), 
                              c == 0 ? i : -1);
                ++c;
            }
        }
        if (active != -2)
            Select(i, i == active);
    }
    list_data = data;
    
    Focus(active);
    Thaw();
}

void ListWithColors::OnColumnMenu(wxListEvent& WXUNUSED(event))
{
    wxMenu popup_menu; 
    for (int i = 0; i < size(columns); ++i) {
        popup_menu.AppendCheckItem(ID_DL_CMENU_SHOW_START+i, 
                                   columns[i].first.c_str());
        popup_menu.Check(ID_DL_CMENU_SHOW_START+i, columns[i].second);
    }
    popup_menu.AppendSeparator();
    popup_menu.Append(ID_DL_CMENU_FITCOLS, "Fit Columns");
    PopupMenu (&popup_menu, 10, 3);
}

void ListWithColors::OnRightDown(wxMouseEvent &event)
{
    wxMenu popup_menu; 
    popup_menu.Append(ID_DL_SELECTALL, "Select &All");
    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void ListWithColors::OnShowColumn(wxCommandEvent &event)
{
    int n = event.GetId() - ID_DL_CMENU_SHOW_START;
    int col;
    for (int i = 0; i < n; ++i)
        if (columns[i].second)
            ++col;
    //TODO if col==0 take care about images
    bool show = event.IsChecked();
    if (show) {
        InsertColumn(col, columns[col].first.c_str());
        for (int i = 0; i < GetItemCount(); ++i)
            SetItem(i, col, list_data[i*columns.size()+n]);
    }
    else
        DeleteColumn(col);
    columns[n].second = show;
    Refresh();
}

void ListWithColors::OnFitColumnWidths(wxCommandEvent &WXUNUSED(event))
{
    for (int i = 0; i < GetColumnCount(); ++i)
        SetColumnWidth(i, wxLIST_AUTOSIZE);
}

void ListWithColors::OnSelectAll(wxCommandEvent &WXUNUSED(event))
{
    for (int i = 0; i < GetItemCount(); ++i)
        Select(i, true);
}

//===============================================================
//                            OutputWin
//===============================================================

BEGIN_EVENT_TABLE(OutputWin, wxTextCtrl)
    EVT_RIGHT_DOWN (                      OutputWin::OnRightDown)
    EVT_MENU_RANGE (ID_OUTPUT_C_BG, ID_OUTPUT_C_WR, OutputWin::OnPopupColor)
    EVT_MENU       (ID_OUTPUT_P_FONT    , OutputWin::OnPopupFont)
    EVT_MENU       (ID_OUTPUT_P_CLEAR   , OutputWin::OnPopupClear)
    EVT_KEY_DOWN   (                      OutputWin::OnKeyDown)
END_EVENT_TABLE()

OutputWin::OutputWin (wxWindow *parent, wxWindowID id, 
                      const wxPoint& pos, const wxSize& size)
    : wxTextCtrl(parent, id, "", pos, size,
                 wxTE_MULTILINE|wxTE_RICH|wxNO_BORDER|wxTE_READONLY)
{}

void OutputWin::fancy_dashes() {
    for (int i = 0; i < 16; i++) {
        SetDefaultStyle (wxTextAttr (wxColour(i * 16, i * 16, i * 16)));
        AppendText ("-");
    }
    AppendText ("\n");
}

void OutputWin::read_settings(wxConfigBase *cf)
{
    cf->SetPath("/OutputWin/Colors");
    text_color[os_normal] = read_color_from_config(cf, "normal", 
                                                      wxColour(150, 150, 150));
    text_color[os_warn] = read_color_from_config(cf, "warn", 
                                                    wxColour(200, 0, 0));
    text_color[os_quot] = read_color_from_config(cf, "quot", 
                                                    wxColour(50, 50, 255));
    text_color[os_input] = read_color_from_config(cf, "input", 
                                                     wxColour(0, 200, 0));
    bg_color = read_color_from_config(cf, "bg", wxColour(20, 20, 20));
    cf->SetPath("/OutputWin");
    wxFont font = read_font_from_config(cf, "font", wxNullFont);
    SetDefaultStyle (wxTextAttr(text_color[os_quot], bg_color, font));

    // this "if" is needed on GTK 1.2 (I don't know why)
    // if it is called before window is shown, it doesn't work 
    // and it is impossible to change the background later.
    if (frame->IsShown())  
        SetBackgroundColour(bg_color); 
    Refresh(); 
} 

void OutputWin::save_settings(wxConfigBase *cf) const
{
    cf->SetPath("/OutputWin/Colors");
    write_color_to_config (cf, "normal", text_color[os_normal]);  
    write_color_to_config (cf, "warn", text_color[os_warn]); 
    write_color_to_config (cf, "quot", text_color[os_quot]); 
    write_color_to_config (cf, "input", text_color[os_input]); 
    write_color_to_config (cf, "bg", bg_color); 
    cf->SetPath("/OutputWin");
    write_font_to_config (cf, "font", GetDefaultStyle().GetFont());
}

void OutputWin::append_text (OutputStyle style, const wxString& str)
{
    SetDefaultStyle (wxTextAttr (text_color[style]));
    AppendText (str);
    ShowPosition(GetLastPosition());
    //TODO TextCtrl is not refreshed when program is busy
    //I think it used to work with older versions of wx
    //Refresh() and Update() here don't help
}

void OutputWin::OnPopupColor (wxCommandEvent& event)
{
    int n = event.GetId();
    wxColour *col;
    if (n == ID_OUTPUT_C_BG)
        col = &bg_color;
    else if (n == ID_OUTPUT_C_OU)
        col = &text_color[os_normal];
    else if (n == ID_OUTPUT_C_QT)
        col = &text_color[os_quot];
    else if (n == ID_OUTPUT_C_WR)
        col = &text_color[os_warn];
    else if (n == ID_OUTPUT_C_IN)
        col = &text_color[os_input];
    else 
        return;
    if (change_color_dlg(*col)) {
        SetBackgroundColour (bg_color);
        SetDefaultStyle (wxTextAttr(wxNullColour, bg_color));
        Refresh();
    }
}

void OutputWin::OnPopupFont (wxCommandEvent& WXUNUSED(event))
{
    wxFontData data; 
    data.SetInitialFont (GetDefaultStyle().GetFont());
    wxFontDialog dlg (frame, &data);
    int r = dlg.ShowModal();
    if (r == wxID_OK) {
        wxFont f = dlg.GetFontData().GetChosenFont();
        SetDefaultStyle (wxTextAttr (wxNullColour, wxNullColour, f));
        Refresh();
    }
}

void OutputWin::OnPopupClear (wxCommandEvent& WXUNUSED(event))
{
    Clear();
    fancy_dashes();
}

    
void OutputWin::OnRightDown (wxMouseEvent& event)
{
    wxMenu popup_menu ("output text menu");

    wxMenu *color_menu = new wxMenu;
    color_menu->Append (ID_OUTPUT_C_BG, "&Background");
    color_menu->Append (ID_OUTPUT_C_IN, "&Input");
    color_menu->Append (ID_OUTPUT_C_OU, "&Output");
    color_menu->Append (ID_OUTPUT_C_QT, "&Quotation");
    color_menu->Append (ID_OUTPUT_C_WR, "&Warning");
    popup_menu.Append  (ID_OUTPUT_C   , "&Color", color_menu);

    popup_menu.Append  (ID_OUTPUT_P_FONT, "&Font");
    popup_menu.Append  (ID_OUTPUT_P_CLEAR, "Clea&r");

    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void OutputWin::OnKeyDown (wxKeyEvent& event)
{
    if (should_focus_input(event.GetKeyCode())) {
        IOPane *parent = static_cast<IOPane*>(GetParent()); //to not use RTTI
        parent->focus_input(event.GetKeyCode());
    }
    else
        event.Skip();
}

//===============================================================
//                            input field
//===============================================================

BEGIN_EVENT_TABLE(InputField, wxTextCtrl)
    EVT_KEY_DOWN (InputField::OnKeyDown)
END_EVENT_TABLE()

void InputField::OnKeyDown (wxKeyEvent& event)
{
    switch (event.m_keyCode) {
        case WXK_RETURN:
        case WXK_NUMPAD_ENTER:
            {
            wxString s = GetValue().Trim();
            if (s.IsEmpty())
                return;
            frame->set_status_text(s);
            history.insert(++history.begin(), s);
            h_pos = history.begin();
            Clear();
            exec_command (s.c_str()); //displaying and executing command
            }
            break;
        case WXK_TAB:
            {
            IOPane *parent = static_cast<IOPane*>(GetParent());//to not use RTTI
            parent->focus_output();
            }
            break;
        case WXK_UP:
        case WXK_NUMPAD_UP:
            if (h_pos == --history.end())
                return;
            ++h_pos;
            SetValue(*h_pos);
            break;
        case WXK_DOWN:
        case WXK_NUMPAD_DOWN:
            if (h_pos == history.begin())
                return;
            --h_pos;
            SetValue(*h_pos);
            break;
        case WXK_PAGEUP:
        case WXK_NUMPAD_PAGEUP:
            h_pos == --history.end();
            SetValue(*h_pos);
            break;
        case WXK_PAGEDOWN:
        case WXK_NUMPAD_PAGEDOWN:
            h_pos == history.begin();
            SetValue(*h_pos);
            break;
        default:
            event.Skip();
    }
}

//===============================================================
//                            FPrintout
//===============================================================

FPrintout::FPrintout(const PlotPane *p_pane) 
    : wxPrintout("fityk printout"), pane(p_pane) 
{}

bool FPrintout::OnPrintPage(int page)
{
    if (page != 1) return false;
    wxDC *dc = GetDC();
    if (!dc) return false;

    // Set the scale and origin
    const int space = 20; //vertical space between plots
    const int marginX = 50, marginY = 50; //page margins
    //width is the same for all plots
    int width = pane->plot->GetClientSize().GetWidth(); 
    vector<FPlot*> vp = pane->get_visible_plots();
    int height = -space;  //height = sum of all heights + (N-1)*space
    for (vector<FPlot*>::const_iterator i = vp.begin(); i != vp.end(); ++i) 
        height += (*i)->GetClientSize().GetHeight() + space;
    int w, h;
    dc->GetSize(&w, &h);
    fp scaleX = w / (width + 2.*marginX);
    fp scaleY = h / (height + 2.*marginY);
    fp actualScale = min (scaleX, scaleY);
    dc->SetUserScale (actualScale, actualScale);

    const int posX = iround((w - width * actualScale) / 2.);
    int posY = iround((h - height * actualScale) / 2.);

    //drawing all visible plots, every at different posY
    for (vector<FPlot*>::const_iterator i = vp.begin(); i != vp.end(); ++i) {
        dc->SetDeviceOrigin (posX, posY);
        (*i)->Draw(*dc);
        posY += iround(((*i)->GetClientSize().GetHeight()+space) * actualScale);
    }
    return true;
}


//===============================================================
//                            ProportionalSplitter
//===============================================================

ProportionalSplitter::ProportionalSplitter(wxWindow* parent, wxWindowID id, 
                                           float proportion, const wxSize& size,
                                           long style, const wxString& name) 
    : wxSplitterWindow(parent, id, wxDefaultPosition, size, style, name),
      m_proportion(proportion), m_firstpaint(true)
{
    wxASSERT(m_proportion >= 0. && m_proportion <= 1.);
    SetMinimumPaneSize(20);
    ResetSash();
    Connect(GetId(), wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED,
                (wxObjectEventFunction) &ProportionalSplitter::OnSashChanged);
    Connect(GetId(), wxEVT_SIZE, 
                     (wxObjectEventFunction) &ProportionalSplitter::OnReSize);
    //hack to set sizes on first paint event
    Connect(GetId(), wxEVT_PAINT, 
                      (wxObjectEventFunction) &ProportionalSplitter::OnPaint);
}

bool ProportionalSplitter::SplitHorizontally(wxWindow* win1, wxWindow* win2,
                                             float proportion) 
{
    if (proportion >= 0. && proportion <= 1.)
        m_proportion = proportion;
    int height = GetClientSize().GetHeight();
    int h = iround(height * m_proportion);
    //sometimes there is a strange problem without it (why?)
    if (h < GetMinimumPaneSize() || h > height-GetMinimumPaneSize())
        h = 0; 
    return wxSplitterWindow::SplitHorizontally(win1, win2, h);
}

bool ProportionalSplitter::SplitVertically(wxWindow* win1, wxWindow* win2,
                                           float proportion) 
{
    if (proportion >= 0. && proportion <= 1.)
        m_proportion = proportion;
    int width = GetClientSize().GetWidth();
    int w = iround(width * m_proportion);
    if (w < GetMinimumPaneSize() || w > width-GetMinimumPaneSize())
        w = 0;
    return wxSplitterWindow::SplitVertically(win1, win2, w);
}

int ProportionalSplitter::GetExpectedSashPosition()
{
    return iround(GetWindowSize() * m_proportion);
}

void ProportionalSplitter::ResetSash()
{
    SetSashPosition(GetExpectedSashPosition());
}

void ProportionalSplitter::OnReSize(wxSizeEvent& event)
{
    // We may need to adjust the sash based on m_proportion.
    ResetSash();
    event.Skip();
}

void ProportionalSplitter::OnSashChanged(wxSplitterEvent &event)
{
    // We'll change m_proportion now based on where user dragged the sash.
    const wxSize& s = GetSize();
    int t = GetSplitMode() == wxSPLIT_HORIZONTAL ? s.GetHeight() : s.GetWidth();
    m_proportion = float(GetSashPosition()) / t;
    event.Skip();
}

void ProportionalSplitter::OnPaint(wxPaintEvent &event)
{
    if (m_firstpaint) {
        if (GetSashPosition() != GetExpectedSashPosition())
            ResetSash();
        m_firstpaint = false;
    }
    event.Skip();
}

