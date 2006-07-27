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
#include <wx/image.h>

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
    ID_SPINBUTTON              ,
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
    ID_DL_SELECTALL            ,
    ID_DL_SWITCHINFO           ,
    ID_CGD_RADIO               
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
    cf->SetPath(wxT("/PlotPane"));
    cf->Write(wxT("PlotPaneProportion"), GetProportion());
    cf->Write(wxT("AuxPlotsProportion"), aux_split->GetProportion());
    cf->Write(wxT("ShowAuxPane0"), aux_visible(0));
    cf->Write(wxT("ShowAuxPane1"), aux_visible(1));
    plot->save_settings(cf);
    for (int i = 0; i < 2; ++i)
        aux_plot[i]->save_settings(cf);
}

void PlotPane::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/PlotPane"));
    SetProportion(cfg_read_double(cf, wxT("PlotPaneProportion"), 0.75));
    aux_split->SetProportion(cfg_read_double(cf, wxT("AuxPlotsProportion"), 
                                                         0.5));
    show_aux(0, cfg_read_bool(cf, wxT("ShowAuxPane0"), true));
    show_aux(1, cfg_read_bool(cf, wxT("ShowAuxPane1"), false));
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
    EVT_SPIN_UP(ID_SPINBUTTON, IOPane::OnSpinButtonUp)
    EVT_SPIN_DOWN(ID_SPINBUTTON, IOPane::OnSpinButtonDown)
END_EVENT_TABLE()

IOPane::IOPane(wxWindow *parent, wxWindowID id)
    : wxPanel(parent, id), output_win(0), input_field(0)
{
    wxBoxSizer *io_sizer = new wxBoxSizer (wxVERTICAL);

    output_win = new OutputWin (this, -1);
    io_sizer->Add (output_win, 1, wxEXPAND);

    input_field = new InputField (this, -1, wxT(""),
                            wxDefaultPosition, wxDefaultSize, 
                            wxWANTS_CHARS|wxTE_PROCESS_ENTER|wxTE_PROCESS_TAB);
    spin_button = new wxSpinButton(this, ID_SPINBUTTON, 
                                   wxDefaultPosition, wxDefaultSize,
                                   wxSP_VERTICAL|wxSP_ARROW_KEYS|wxNO_BORDER);
    spin_button->SetRange(0, 0); 
    spin_button->SetValue(0); 
    input_field->set_spin_button(spin_button); 
    wxBoxSizer *io_h_sizer = new wxBoxSizer(wxHORIZONTAL);
    io_h_sizer->Add(input_field, 1);
    io_h_sizer->Add(spin_button, 0);
    io_sizer->Add (io_h_sizer, 0, wxEXPAND);

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
        input_field->WriteText(wxString::Format(wxT("%c"), key));
    }
}

void IOPane::edit_in_input(string const& s) 
{
    input_field->WriteText(s2wx(s));
    input_field->SetFocus(); 
}

//===============================================================
//                            ListPlusText
//===============================================================
BEGIN_EVENT_TABLE(ListPlusText, ProportionalSplitter)
    EVT_MENU (ID_DL_SWITCHINFO, ListPlusText::OnSwitchInfo)
END_EVENT_TABLE()

ListPlusText::ListPlusText(wxWindow *parent, wxWindowID id, wxWindowID list_id,
                           vector<pair<string,int> > const& columns_)
: ProportionalSplitter(parent, id, 0.75) 
{
    list = new ListWithColors(this, list_id, columns_);
    inf = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize,
                         wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE);
}

void ListPlusText::OnSwitchInfo(wxCommandEvent &WXUNUSED(event))
{
    if (IsSplit())
        Unsplit(inf);
    else
        SplitHorizontally(list, inf);
}

//===============================================================
//                      ValueChangingWidget
//===============================================================

// first tiny callback class

class ValueChangingHandler 
{
public:
    virtual ~ValueChangingHandler() {}
    virtual void change_value(fp factor) = 0;
    virtual void on_stop_changing() = 0;
};

/// small widget used to change value of associated wxTextCtrl with real number
class ValueChangingWidget : public wxSlider
{
public:
    ValueChangingWidget(wxWindow* parent, wxWindowID id, 
                        ValueChangingHandler* cb)
        : wxSlider(parent, id, 0, -100, 100, wxDefaultPosition, wxSize(60, -1)),
          callback(cb), timer(this, -1), button(0) {}

    void OnTimer(wxTimerEvent &event);

    void OnThumbTrack(wxScrollEvent &WXUNUSED(event)) { 
        if (!timer.IsRunning()) {
            timer.Start(100);
        }
    }
    void OnMouse(wxMouseEvent &event);

private:
    ValueChangingHandler *callback;
    wxTimer timer;
    char button;
    
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ValueChangingWidget, wxSlider)
    EVT_TIMER(-1, ValueChangingWidget::OnTimer)
    EVT_MOUSE_EVENTS(ValueChangingWidget::OnMouse)
    EVT_SCROLL_THUMBTRACK(ValueChangingWidget::OnThumbTrack)
END_EVENT_TABLE()

void ValueChangingWidget::OnTimer(wxTimerEvent &WXUNUSED(event))
{
    if (button == 'l') {
        callback->change_value(GetValue()*0.001);
    }
    else if (button == 'm') {
        callback->change_value(GetValue()*0.0001);
    }
    else if (button == 'r') {
        callback->change_value(GetValue()*0.00001);
    }
    else {
        assert (button == 0);
        timer.Stop();
        SetValue(0);
        callback->on_stop_changing();
    }
}

void ValueChangingWidget::OnMouse(wxMouseEvent &event)
{
    if (event.LeftIsDown())
        button = 'l';
    else if (event.RightIsDown())
        button = 'r';
    else if (event.MiddleIsDown())
        button = 'm';
    else
        button = 0;
    event.Skip();
}

//===============================================================
//                          FancyRealCtrl
//===============================================================

class FancyRealCtrl : public wxPanel, public ValueChangingHandler
{
public:
    FancyRealCtrl(wxWindow* parent, wxWindowID id, 
                  fp value, string const& tc_name, 
                  SideBar const* draw_handler_);
    void change_value(fp factor);
    void on_stop_changing();
    void OnTextEnter(wxCommandEvent &WXUNUSED(event)) { on_stop_changing(); }
    void set(fp value, string const& tc_name);
    fp get_value() const;
    string get_name() const { return name; }
    DECLARE_EVENT_TABLE()
private:
    fp initial_value;
    std::string name;
    SideBar const* draw_handler;
    wxTextCtrl *tc;
};

BEGIN_EVENT_TABLE(FancyRealCtrl, wxPanel)
    EVT_TEXT_ENTER(-1, FancyRealCtrl::OnTextEnter)
END_EVENT_TABLE()

FancyRealCtrl::FancyRealCtrl(wxWindow* parent, wxWindowID id, 
                             fp value, string const& tc_name, 
                             SideBar const* draw_handler_)
    : wxPanel(parent, id), initial_value(value), name(tc_name),
      draw_handler(draw_handler_)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    tc = new wxTextCtrl(this, -1, s2wx(S(value)), 
                        wxDefaultPosition, wxDefaultSize,
                        wxTE_PROCESS_ENTER);
    tc->SetToolTip(s2wx(name));
    sizer->Add(tc, 1, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 1);
    ValueChangingWidget *vch = new ValueChangingWidget(this, -1, this);
    sizer->Add(vch, 0, wxALL|wxALIGN_CENTER_VERTICAL, 1);
    SetSizer(sizer);
}

void FancyRealCtrl::set(fp value, string const& tc_name)
{
    tc->SetValue(s2wx(S(value)));
    if (name != tc_name) {
        name = tc_name;
        tc->SetToolTip(s2wx(name));
    }
}

void FancyRealCtrl::change_value(fp factor)
{
    double t;
    bool ok = tc->GetValue().ToDouble(&t);
    if (!ok)
        return;
    if (t != initial_value)
        draw_handler->draw_function_draft(this);
    t += fabs(initial_value) * factor;
    tc->SetValue(s2wx(S(t)));
    if (t != initial_value)
        draw_handler->draw_function_draft(this);
}

fp FancyRealCtrl::get_value() const
{
    double t;
    bool ok = tc->GetValue().ToDouble(&t);
    return ok ? t : initial_value;
}

void FancyRealCtrl::on_stop_changing()
{
    if (wx2s(tc->GetValue()) != S(initial_value)) {
        double t;
        bool ok = tc->GetValue().ToDouble(&t);
        if (ok) {
            initial_value = t;
            exec_command(name + " = ~" + wx2s(tc->GetValue()));
        }
        else
            tc->SetValue(s2wx(S(initial_value)));
    }
}


//===============================================================
//                           SideBar
//===============================================================

void add_bitmap_button(wxWindow* parent, wxWindowID id, char** xpm,
                       wxString const& tip, wxSizer* sizer)
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
    EVT_BUTTON (ID_VP_NEW, SideBar::OnVarButtonNew)
    EVT_BUTTON (ID_VP_DEL, SideBar::OnVarButtonDel)
    EVT_BUTTON (ID_VP_EDIT, SideBar::OnVarButtonEdit)
    EVT_LIST_ITEM_FOCUSED(ID_VP_LIST, SideBar::OnVarFocusChanged)
END_EVENT_TABLE()

SideBar::SideBar(wxWindow *parent, wxWindowID id)
: ProportionalSplitter(parent, id, 0.75), bp_func(0), active_function(-1)
{
    //wxPanel *upper = new wxPanel(this, -1);
    //wxBoxSizer *upper_sizer = new wxBoxSizer(wxVERTICAL);
    nb = new wxNotebook(this, -1);
    //upper_sizer->Add(nb, 1, wxEXPAND);
    //upper->SetSizerAndFit(upper_sizer);

    //-----  data page  -----
    data_page = new wxPanel(nb, -1);
    wxBoxSizer *data_sizer = new wxBoxSizer(wxVERTICAL);
    d = new ListPlusText(data_page, -1, ID_DP_LIST, 
                            vector4(pair<string,int>("No", 43),
                                    pair<string,int>("#F+#Z", 43),
                                    pair<string,int>("Name", 108),
                                    pair<string,int>("File", 0)));
    d->list->set_side_bar(this);
    data_sizer->Add(d, 1, wxEXPAND|wxALL, 1);

    wxBoxSizer *data_look_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_bitmap_button(data_page, ID_DP_COL, colorsel_xpm, 
                      wxT("change color"), data_look_sizer);
    wxArrayString choices;
    choices.Add(wxT("show all datasets"));
    choices.Add(wxT("show only selected"));
    choices.Add(wxT("shadow unselected"));
    choices.Add(wxT("hide all"));
    data_look = new wxChoice(data_page, ID_DP_LOOK,
                             wxDefaultPosition, wxDefaultSize, choices);
    data_look->Select(0);
    data_look_sizer->Add(data_look, 1, wxEXPAND);
    data_look_sizer->Add(new wxStaticBitmap(data_page, -1, 
                                            wxBitmap(shiftup_xpm)), 
                         0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    shiftup_sc = new wxSpinCtrl(data_page, ID_DP_SHIFTUP, wxT("0"),
                                wxDefaultPosition, wxSize(40, -1),
                                wxSP_ARROW_KEYS, 0, 80, 0);
    shiftup_sc->SetToolTip(wxT("shift up in \% of plot Y size"));
    data_look_sizer->Add(shiftup_sc, 0, wxEXPAND);
    data_sizer->Add(data_look_sizer, 0, wxEXPAND);

    wxBoxSizer *data_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_bitmap_button(data_page, ID_DP_NEW, add_xpm, 
                      wxT("new data"), data_buttons_sizer);
    add_bitmap_button(data_page, ID_DP_DUP, sum_xpm, 
                      wxT("duplicate/sum"), data_buttons_sizer);
    add_bitmap_button(data_page, ID_DP_REN, rename_xpm, 
                      wxT("rename"), data_buttons_sizer);
    add_bitmap_button(data_page, ID_DP_DEL, close_xpm, 
                      wxT("delete"), data_buttons_sizer);
    add_bitmap_button(data_page, ID_DP_CPF, copyfunc_xpm, 
                      wxT("copy F to next dataset"), data_buttons_sizer);
    data_sizer->Add(data_buttons_sizer, 0, wxEXPAND);
    data_page->SetSizerAndFit(data_sizer);
    nb->AddPage(data_page, wxT("data"));

    //-----  functions page  -----
    func_page = new wxPanel(nb, -1);
    wxBoxSizer *func_sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *func_filter_sizer = new wxBoxSizer(wxHORIZONTAL);
    func_filter_sizer->Add(new wxStaticBitmap(func_page, -1, 
                                              wxBitmap(filter_xpm)),
                           0, wxALIGN_CENTER_VERTICAL);
    filter_ch = new wxChoice(func_page, ID_FP_FILTER,
                             wxDefaultPosition, wxDefaultSize, 0, 0);
    filter_ch->Append(wxT("list all functions"));
    filter_ch->Select(0);
    func_filter_sizer->Add(filter_ch, 1, wxEXPAND);
    func_sizer->Add(func_filter_sizer, 0, wxEXPAND);

    vector<pair<string,int> > fdata;
    fdata.push_back( pair<string,int>("Name", 54) );
    fdata.push_back( pair<string,int>("Type", 80) );
    fdata.push_back( pair<string,int>("Center", 60) );
    fdata.push_back( pair<string,int>("Area", 0) );
    fdata.push_back( pair<string,int>("Height", 0) );
    fdata.push_back( pair<string,int>("FWHM", 0) );
    f = new ListPlusText(func_page, -1, ID_FP_LIST, fdata);
    f->list->set_side_bar(this);
    func_sizer->Add(f, 1, wxEXPAND|wxALL, 1);
    wxBoxSizer *func_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_bitmap_button(func_page, ID_FP_NEW, add_xpm, 
                      wxT("new function"), func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_DEL, close_xpm, 
                      wxT("delete"), func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_EDIT, editf_xpm, 
                      wxT("edit function"), func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_CHTYPE, convert_xpm, 
                      wxT("change type of function"), func_buttons_sizer);
    add_bitmap_button(func_page, ID_FP_COL, colorsel_xpm, 
                      wxT("change color"), func_buttons_sizer);
    func_sizer->Add(func_buttons_sizer, 0, wxEXPAND);
    func_page->SetSizerAndFit(func_sizer);
    nb->AddPage(func_page, wxT("functions"));

    //-----  variables page  -----
    var_page = new wxPanel(nb, -1);
    wxBoxSizer *var_sizer = new wxBoxSizer(wxVERTICAL);
    vector<pair<string,int> > vdata = vector4(
                                     pair<string,int>("Name", 52),
                                     pair<string,int>("#/#", 72), 
                                     pair<string,int>("value", 70),
                                     pair<string,int>("formula", 0) );
    v = new ListPlusText(var_page, -1, ID_VP_LIST, vdata);
    v->list->set_side_bar(this);
    var_sizer->Add(v, 1, wxEXPAND|wxALL, 1);
    wxBoxSizer *var_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_bitmap_button(var_page, ID_VP_NEW, add_xpm, 
                      wxT("new variable"), var_buttons_sizer);
    add_bitmap_button(var_page, ID_VP_DEL, close_xpm, 
                      wxT("delete"), var_buttons_sizer);
    add_bitmap_button(var_page, ID_VP_EDIT, editf_xpm, 
                      wxT("edit variable"), var_buttons_sizer);
    var_sizer->Add(var_buttons_sizer, 0, wxEXPAND);
    var_page->SetSizerAndFit(var_sizer);
    nb->AddPage(var_page, wxT("variables"));

    //-----
    bottom_panel = new wxPanel(this, -1);
    wxBoxSizer* bp_topsizer = new wxBoxSizer(wxVERTICAL);
    bp_label = new wxStaticText(bottom_panel, -1, wxT(""), 
                                wxDefaultPosition, wxDefaultSize, 
                                wxST_NO_AUTORESIZE|wxALIGN_CENTRE);
    bp_topsizer->Add(bp_label, 0, wxEXPAND|wxALL, 5);
    bp_sizer = new wxFlexGridSizer(2, 0, 0);
    bp_sizer->AddGrowableCol(1);
    bp_topsizer->Add(bp_sizer, 1, wxEXPAND);
    bottom_panel->SetSizer(bp_topsizer);
    bottom_panel->SetAutoLayout(true);
    SplitHorizontally(nb, bottom_panel);
    d->split();
    f->split();
    v->split();
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
    wxString old_title = s2wx(data->get_title());

    wxString s = wxGetTextFromUser(wxT("New name for dataset @") + s2wx(S(n)), 
                                   wxT("Rename dataset"),
                                   old_title);
    if (!s.IsEmpty() && s != old_title)
        exec_command("@" + S(n) + ".title = '" + wx2s(s) + "'");
}

void SideBar::delete_selected_items()
{
    int n = nb->GetSelection();
    if (n < 0)
        return;
    string txt = wx2s(nb->GetPageText(n));
    if (txt == "data")
        exec_command("delete " + join_vector(get_selected_data(), ", "));
    else if (txt == "functions")
        exec_command("delete " + join_vector(get_selected_func(), ", "));
    else if (txt == "variables")
        exec_command("delete " + join_vector(get_selected_vars(), ", "));
    else
        assert(0);
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
    int sel_size = d->list->GetSelectedItemCount();
    if (sel_size == 0)
        return;
    else if (sel_size == 1) {
        int n = d->list->GetFirstSelected();
        wxColour col = frame->get_main_plot()->get_data_color(n);
        if (change_color_dlg(col)) {
            frame->get_main_plot()->set_data_color(n, col);
            update_lists();
            frame->refresh_plots(true, false, true);
        }
    }
    else {//sel_size > 1
        vector<string> sel = SideBar::get_selected_data();
        int first_sel = d->list->GetFirstSelected();
        int last_sel = 0;
        for (int i = first_sel; i != -1; i = d->list->GetNextSelected(i))
            last_sel = i;
        wxColour first_col = frame->get_main_plot()->get_data_color(first_sel);
        wxColour last_col = frame->get_main_plot()->get_data_color(last_sel);
        GradientDlgWithApply<SideBar> gd(this, -1, first_col, last_col, 
                                         this, &SideBar::OnDataColorsChanged);
        gd.ShowModal();
    }
}

void SideBar::OnDataColorsChanged(GradientDlg *gd)
{
    vector<int> selected;
    for (int i = d->list->GetFirstSelected(), c = 0; i != -1; 
                                        i = d->list->GetNextSelected(i), ++c) {
        selected.push_back(i);
        wxColour col = gd->get_value(c / (d->list->GetSelectedItemCount()-1.));
        frame->get_main_plot()->set_data_color(i, col);
    }
    update_lists();
    frame->refresh_plots(true, false, true);
    for (vector<int>::const_iterator i = selected.begin(); 
            i != selected.end(); ++i) 
    for (int i = 0; i != d->list->GetItemCount(); ++i)
        d->list->Select(i, contains_element(selected, i));
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
    string peak_type = frame->get_peak_type();
    string formula = Function::get_formula(peak_type);
    vector<string> varnames = Function::get_varnames_from_formula(formula);
    string t = "%put_name_here = " + peak_type + "(" 
                                      + join_vector(varnames, "= , ") + "= )";
    frame->edit_in_input(t);
}

void SideBar::OnFuncButtonEdit (wxCommandEvent& WXUNUSED(event))
{
    if (!bp_func)
        return;
    string t = bp_func->get_current_definition(AL->get_variables(), 
                                               AL->get_parameters());
    frame->edit_in_input(t);
}

void SideBar::OnFuncButtonChType (wxCommandEvent& WXUNUSED(event))
{
    mesg("Sorry. Changing type of function is not implemented yet.");
}

void SideBar::OnFuncButtonCol (wxCommandEvent& WXUNUSED(event))
{
    if (active_function == -1)
        return;
    wxColour col = frame->get_main_plot()->get_func_color(active_function);
    if (change_color_dlg(col)) {
        frame->get_main_plot()->set_func_color(active_function, col);
        update_lists();
        frame->refresh_plots(true, false, true);
    }
}

void SideBar::OnVarButtonNew (wxCommandEvent& WXUNUSED(event))
{
    frame->edit_in_input("$put_name_here = ");
}

void SideBar::OnVarButtonEdit (wxCommandEvent& WXUNUSED(event))
{
    int n = get_focused_var();
    if (n < 0 || n >= size(AL->get_variables()))
        return;
    Variable const* var = AL->get_variable(n);
    string t = var->xname + " = "+ var->get_formula(AL->get_parameters());
    frame->edit_in_input(t);
}

void SideBar::update_lists(bool nondata_changed)
{
    Freeze();
    update_data_list(nondata_changed);
    update_func_list(nondata_changed);
    update_var_list();
    
    //-- enable/disable buttons
    bool not_the_last = get_focused_data()+1 < AL->get_ds_count();
    data_page->FindWindow(ID_DP_CPF)->Enable(not_the_last);
    bool has_any_funcs = (f->list->GetItemCount() > 0);
    func_page->FindWindow(ID_FP_DEL)->Enable(has_any_funcs);
    func_page->FindWindow(ID_FP_EDIT)->Enable(has_any_funcs);
    func_page->FindWindow(ID_FP_CHTYPE)->Enable(has_any_funcs);
    func_page->FindWindow(ID_FP_COL)->Enable(has_any_funcs);
    bool has_any_vars = (v->list->GetItemCount() > 0);
    var_page->FindWindow(ID_VP_DEL)->Enable(has_any_vars);
    var_page->FindWindow(ID_VP_EDIT)->Enable(has_any_vars);

    update_data_inf();
    update_func_inf();
    update_var_inf();
    update_bottom_panel();
    Thaw();
}

void SideBar::update_data_list(bool nondata_changed)
{
    MainPlot const* mplot = frame->get_main_plot();
    wxColour const& bg_col = mplot->get_bg_color();

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
    if (nondata_changed || AL->get_ds_count() > d->list->GetItemCount()) {
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
    d->list->populate(data_data, data_images, active_ds_pos);
}

void SideBar::update_func_list(bool nondata_changed)
{
    MainPlot const* mplot = frame->get_main_plot();
    wxColour const& bg_col = mplot->get_bg_color();

    //functions filter
    if (AL->get_ds_count()+1 != filter_ch->GetCount()) {
        while (filter_ch->GetCount() > AL->get_ds_count()+1)
            filter_ch->Delete(filter_ch->GetCount()-1);
        for (int i = filter_ch->GetCount()-1; i < AL->get_ds_count(); ++i)
            filter_ch->Append(wxString::Format(wxT("only functions from @%i"),
                                               i));
    }

    //functions
    static vector<int> func_col_id;
    static int old_func_size;
    vector<string> func_data;
    vector<int> new_func_col_id;
    int active_ds_pos = AL->get_active_ds_position();
    Sum const* sum = AL->get_sum(active_ds_pos);
    Sum const* filter_sum = 0;
    if (filter_ch->GetSelection() > 0)
        filter_sum = AL->get_sum(filter_ch->GetSelection()-1);
    int func_size = AL->get_functions().size();
    if (active_function == -1)
        active_function = func_size - 1;
    else {
        if (active_function >= func_size || 
                AL->get_function(active_function) != bp_func)
            active_function = AL->find_function_nr(active_function_name);
        if (active_function == -1 || func_size == old_func_size+1)
            active_function = func_size - 1;
    }
    if (active_function != -1)
        active_function_name = AL->get_function(active_function)->name;
    else
        active_function_name = "";

    int pos = -1;
    for (int i = 0; i < func_size; ++i) {
        if (filter_sum && !contains_element(filter_sum->get_ff_idx(), i)
                           && !contains_element(filter_sum->get_zz_idx(), i))
            continue;
        if (i == active_function)
            pos = new_func_col_id.size();
        Function const* f = AL->get_function(i);
        func_data.push_back(f->name);
        func_data.push_back(f->type_name);
        func_data.push_back(f->has_center() ? S(f->center()).c_str() : "-");
        func_data.push_back(f->has_area() ? S(f->area()).c_str() : "-");
        func_data.push_back(f->has_height() ? S(f->height()).c_str() : "-");
        func_data.push_back(f->has_fwhm() ? S(f->fwhm()).c_str() : "-");
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
    old_func_size = func_size;
    f->list->populate(func_data, func_images, pos);
}

void SideBar::update_var_list()
{
    vector<string> var_data;
    //  count references first
    vector<Variable*> const& variables = AL->get_variables();
    vector<int> var_vrefs(variables.size(), 0), var_frefs(variables.size(), 0);
    for (vector<Variable*>::const_iterator i = variables.begin();
                                                i != variables.end(); ++i) {
        for (int j = 0; j != (*i)->get_vars_count(); ++j)
            var_vrefs[(*i)->get_var_idx(j)]++;
    }
    for (vector<Function*>::const_iterator i = AL->get_functions().begin();
                                         i != AL->get_functions().end(); ++i) {
        for (int j = 0; j != (*i)->get_vars_count(); ++j)
            var_frefs[(*i)->get_var_idx(j)]++;
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
    v->list->populate(var_data);
}

int SideBar::get_focused_var() const 
{ 
    if (AL->get_variables().empty())
        return -1;
    else {
        int n = v->list->GetFocusedItem(); 
        return n > 0 ? n : 0;
    }
}

void SideBar::activate_function(int n)
{
    active_function = n;
    do_activate_function();
    int pos = n;
    if (filter_ch->GetSelection() > 0)
        pos = f->list->FindItem(-1, s2wx(active_function_name));
    f->list->Focus(pos);
    for (int i = 0; i != f->list->GetItemCount(); ++i)
        f->list->Select(i, i==pos);
}

void SideBar::do_activate_function()
{
    if (active_function != -1)
        active_function_name = AL->get_function(active_function)->name;
    else
        active_function_name = "";
    frame->refresh_plots(true, false, true);
    update_func_inf();
    update_bottom_panel();
}

vector<string> SideBar::get_selected_data() const
{
    vector<string> dd;
    for (int i = d->list->GetFirstSelected(); i != -1; 
                                           i = d->list->GetNextSelected(i))
        dd.push_back("@" + S(i));
    if (dd.empty()) {
        int n = d->list->GetFocusedItem();
        dd.push_back("@" + S(n == -1 ? 0 : n));
    }
    return dd;
}

void SideBar::OnDataFocusChanged(wxListEvent& WXUNUSED(event))
{
    int n = d->list->GetFocusedItem();
    if (n < 0)
        return;
    int length = AL->get_ds_count();
    if (length > 1 && AL->get_active_ds_position() != n) 
        exec_command("plot @" + S(n) + " ..");
    data_page->FindWindow(ID_DP_CPF)->Enable(n+1<length);
}

void SideBar::update_data_inf()
{
    int n = get_focused_data();
    if (n < 0)
        return;
    wxTextCtrl* inf = d->inf;
    inf->Clear();
    wxTextAttr defattr = inf->GetDefaultStyle();
    wxFont font = defattr.GetFont();
    wxTextAttr boldattr = defattr;
    font.SetWeight(wxFONTWEIGHT_BOLD);
    boldattr.SetFont(font);

    inf->SetDefaultStyle(boldattr);
    inf->AppendText(s2wx("@" + S(n) + "\n"));
    inf->SetDefaultStyle(defattr);
    inf->AppendText(s2wx(AL->get_data(n)->getInfo()));
    inf->ShowPosition(0);
}

void SideBar::update_func_inf()
{
    wxTextCtrl* inf = f->inf;
    inf->Clear();
    if (active_function < 0)
        return;
    wxTextAttr defattr = inf->GetDefaultStyle();
    wxFont font = defattr.GetFont();
    wxTextAttr boldattr = defattr;
    font.SetWeight(wxFONTWEIGHT_BOLD);
    boldattr.SetFont(font);

    Function const* func = AL->get_function(active_function);
    inf->SetDefaultStyle(boldattr);
    inf->AppendText(s2wx(func->xname));
    inf->SetDefaultStyle(defattr);
    if (func->has_center()) 
        inf->AppendText(wxT("\nCenter: ") + s2wx(S(func->center())));
    if (func->has_area()) 
        inf->AppendText(wxT("\nArea: ") + s2wx(S(func->area())));
    if (func->has_height()) 
        inf->AppendText(wxT("\nHeight: ") + s2wx(S(func->height())));
    if (func->has_fwhm()) 
        inf->AppendText(wxT("\nFWHM: ") + s2wx(S(func->fwhm())));
    if (func->has_iwidth()) 
        inf->AppendText(wxT("\nInt. Width: ") + s2wx(S(func->iwidth())));
    vector<string> in;
    for (int i = 0; i < AL->get_ds_count(); ++i) {
        if (contains_element(AL->get_sum(i)->get_ff_idx(), active_function))
            in.push_back("@" + S(i) + ".F");
        if (contains_element(AL->get_sum(i)->get_zz_idx(), active_function))
            in.push_back("@" + S(i) + ".Z");
    }
    if (!in.empty())
        inf->AppendText(s2wx("\nIn: " + join_vector(in, ", ")));
    inf->ShowPosition(0);
}

void SideBar::update_var_inf()
{
    int n = get_focused_var();
    if (n < 0)
        return;
    wxTextCtrl* inf = v->inf;
    inf->Clear();
    wxTextAttr defattr = inf->GetDefaultStyle();
    wxFont font = defattr.GetFont();
    wxTextAttr boldattr = defattr;
    font.SetWeight(wxFONTWEIGHT_BOLD);
    boldattr.SetFont(font);

    Variable const* var = AL->get_variable(n);
    inf->SetDefaultStyle(boldattr);
    string t = var->xname + " = " + var->get_formula(AL->get_parameters());
    inf->AppendText(s2wx(t));
    inf->SetDefaultStyle(defattr);
    vector<string> in = AL->get_variable_references(var->name);
    if (!in.empty())
        inf->AppendText(s2wx("\nIn:\n    " + join_vector(in, "\n    ")));
    inf->ShowPosition(0);
}

void SideBar::add_variable_to_bottom_panel(Variable const* var, 
                                           string const& tv_name)
{
    wxStaticText* name_st = new wxStaticText(bottom_panel, -1, s2wx(tv_name));
    bp_sizer->Add(name_st, 0, wxALL|wxALIGN_CENTER_VERTICAL, 1);
    bp_statict.push_back(name_st);
    if (var->is_simple()) {
        FancyRealCtrl *frc = new FancyRealCtrl(bottom_panel, -1,
                                               var->get_value(), var->xname,
                                               this);
        bp_sizer->Add(frc, 1, wxALL|wxEXPAND, 1);
        bp_frc.push_back(frc);
    }
    else {
        string t = var->xname + " = " + var->get_formula(AL->get_parameters());
        wxStaticText *var_st = new wxStaticText(bottom_panel, -1, s2wx(t));
        bp_sizer->Add(var_st, 1, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 1);
        bp_statict.push_back(var_st);
    }
}

void SideBar::clear_bottom_panel()
{
    for (vector<wxStaticText*>::iterator i = bp_statict.begin(); 
                                                i != bp_statict.end(); ++i)
        (*i)->Destroy();
    bp_statict.clear();
    for (vector<FancyRealCtrl*>::iterator i = bp_frc.begin(); 
                                                      i != bp_frc.end(); ++i)
        (*i)->Destroy();
    bp_frc.clear();
    bp_label->SetLabel(wxT(""));
    bp_sig.clear();
}

vector<bool> SideBar::make_bottom_panel_sig(Function const* func)
{
    vector<bool> sig;
    for (int i = 0; i < size(func->type_var_names); ++i) {
        Variable const* var = AL->get_variable(func->get_var_idx(i));
        sig.push_back(var->is_simple());
    }
    return sig;
}

void SideBar::draw_function_draft(FancyRealCtrl const* frc) const
{
    vector<fp> p_values(bp_func->nv);
    for (int i = 0; i < bp_func->nv; ++i) {
        string xname = "$" + bp_func->get_var_name(i);
        p_values[i] = frc->get_name() != xname ? bp_func->get_var_value(i) 
                                               : frc->get_value();
    }
    frame->get_main_plot()->draw_xor_peak(bp_func, p_values);
}

void SideBar::update_bottom_panel()
{
    if (active_function < 0) {
        clear_bottom_panel();
        bp_func = 0;
        return;
    }
    bottom_panel->Freeze();
    bp_func = AL->get_function(active_function);
    bp_label->SetLabel(s2wx(bp_func->xname + " : " + bp_func->type_name));
    vector<bool> sig = make_bottom_panel_sig(bp_func);
    if (sig != bp_sig) {
        clear_bottom_panel();
        bp_sig = sig;
        for (int i = 0; i < bp_func->nv; ++i) {
            Variable const* var = AL->get_variable(bp_func->get_var_idx(i));
            add_variable_to_bottom_panel(var, bp_func->type_var_names[i]);
        }
        int sash_pos = GetClientSize().GetHeight() - 3
                         - bottom_panel->GetSizer()->GetMinSize().GetHeight();
        if (sash_pos < GetSashPosition())
            SetSashPosition(max(50, sash_pos));
    }
    else {
        vector<wxStaticText*>::iterator st = bp_statict.begin();
        vector<FancyRealCtrl*>::iterator frc = bp_frc.begin();
        for (int i = 0; i < bp_func->nv; ++i) {
            string const& t = bp_func->type_var_names[i];
            (*st)->SetLabel(s2wx(t));
            ++st;
            Variable const* var = AL->get_variable(bp_func->get_var_idx(i));
            if (var->is_simple()) {
                  (*frc)->set(var->get_value(), var->xname);
                  ++frc;
            }
            else {
                string f = var->xname + " = " 
                                   + var->get_formula(AL->get_parameters());
                (*st)->SetLabel(s2wx(f));
                ++st;
            }
        }
    }
    bottom_panel->Layout();
    bottom_panel->Thaw();
}

bool SideBar::howto_plot_dataset(int n, bool& shadowed, int& offset) const
{
    // choice_idx: 0: "show all datasets" 
    //             1: "show only selected" 
    //             2: "shadow unselected" 
    //             3: "hide all"
    int choice_idx = data_look->GetSelection();
    bool sel = d->list->IsSelected(n) || d->list->GetFocusedItem() == n;
    if ((choice_idx == 1 && !sel) || choice_idx == 3)
        return false;
    shadowed = (choice_idx == 2 && !sel);
    offset = n * shiftup_sc->GetValue();
    return true;
}

vector<string> SideBar::get_selected_func() const
{
    vector<string> dd;
    for (int i = f->list->GetFirstSelected(); i != -1; 
                                            i = f->list->GetNextSelected(i))
        dd.push_back(AL->get_function(i)->xname);
    if (dd.empty() && f->list->GetItemCount() > 0) {
        int n = f->list->GetFocusedItem();
        dd.push_back(AL->get_function(n == -1 ? 0 : n)->xname);
    }
    return dd;
}

vector<string> SideBar::get_selected_vars() const
{
    vector<string> dd;
    for (int i = v->list->GetFirstSelected(); i != -1; 
                                             i = v->list->GetNextSelected(i))
        dd.push_back(AL->get_variable(i)->xname);
    if (dd.empty() && v->list->GetItemCount() > 0) {
        int n = v->list->GetFocusedItem();
        dd.push_back(AL->get_function(n == -1 ? 0 : n)->xname);
    }
    return dd;
}


void SideBar::OnFuncFocusChanged(wxListEvent& WXUNUSED(event))
{
    int n = f->list->GetFocusedItem(); 
    if (n == -1)
        active_function = -1;
    else {
        string name = wx2s(f->list->GetItemText(n));
        active_function = AL->find_function_nr(name);
    }
    do_activate_function();
}

void SideBar::OnVarFocusChanged(wxListEvent& WXUNUSED(event))
{
    update_var_inf();
}



//===============================================================
//                    GradientDlg and helpers
//===============================================================


BEGIN_EVENT_TABLE(ColorSpinSelector, wxPanel)
    EVT_BUTTON (-1, ColorSpinSelector::OnSelector)
END_EVENT_TABLE()

ColorSpinSelector::ColorSpinSelector(wxWindow *parent, wxString const& title,
                                     wxColour const& col)
    : wxPanel(parent, -1)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, title);
    sizer->Add(new wxStaticText(this, -1, wxT("R")), 
                    0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    r = new SpinCtrl(this, -1, col.Red(), 0, 255);
    sizer->Add(r, 0, wxALL, 5);
    sizer->Add(new wxStaticText(this, -1, wxT("G")), 
                    0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    g = new SpinCtrl(this, -1, col.Green(), 0, 255);
    sizer->Add(g, 0, wxALL, 5);
    sizer->Add(new wxStaticText(this, -1, wxT("B")), 
                    0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);
    b = new SpinCtrl(this, -1, col.Blue(), 0, 255);
    sizer->Add(b, 0, wxALL, 5);
    sizer->Add(new wxButton(this, -1, wxT("Selector...")), 0, wxALL, 5);
    top_sizer->Add(sizer, 1, wxEXPAND);
    SetSizerAndFit(top_sizer);
}

void ColorSpinSelector::OnSelector(wxCommandEvent &)
{
    wxColour c(r->GetValue(), g->GetValue(), b->GetValue());
    if (change_color_dlg(c)) {
        r->SetValue(c.Red());
        g->SetValue(c.Green());
        b->SetValue(c.Blue());
    }
    // parent is notified about changes by handling all wxSpinCtrl events
    wxSpinEvent event(wxEVT_COMMAND_SPINCTRL_UPDATED);
    wxPostEvent(this, event);
}


BEGIN_EVENT_TABLE(GradientDlg, wxDialog)
    EVT_SPINCTRL (-1, GradientDlg::OnSpinEvent)
    EVT_RADIOBOX (ID_CGD_RADIO, GradientDlg::OnRadioChanged)
END_EVENT_TABLE()

GradientDlg::GradientDlg(wxWindow *parent, wxWindowID id, 
                         wxColour const& first_col, wxColour const& last_col)
    : wxDialog(parent, id, wxT("Select color gradient")) 
{
    wxBoxSizer* top_sizer = new wxBoxSizer(wxVERTICAL);

    from = new ColorSpinSelector(this, wxT("from"), first_col);
    top_sizer->Add(from, 0, wxALL, 5);

    to = new ColorSpinSelector(this, wxT("to"), last_col);
    top_sizer->Add(to, 0, wxALL, 5);

    wxArrayString choices;
    choices.Add(wxT("HSV gradient, clockwise hue"));
    choices.Add(wxT("HSV gradient, counter-clockwise"));
    choices.Add(wxT("RGB gradient"));
    choices.Add(wxT("one color"));
    kind_rb = new wxRadioBox(this, ID_CGD_RADIO, wxT("how to extrapolate..."),
                             wxDefaultPosition, wxDefaultSize, choices,
                             1, wxRA_SPECIFY_COLS);
    top_sizer->Add(kind_rb, 0, wxALL|wxEXPAND, 5);
    display = new ColorGradientDisplay<GradientDlg>(this, 
                                 this, &GradientDlg::update_gradient_display);
    display->SetMinSize(wxSize(-1, 15));
    top_sizer->Add(display, 0, wxALL|wxEXPAND, 5);

    add_apply_close_buttons(this, top_sizer);
    SetSizerAndFit(top_sizer);
    update_gradient_display();
}

void GradientDlg::update_gradient_display()
{
    int display_width = display->GetClientSize().GetWidth();
    display->data.resize(display_width);
    for (int i = 0; i < display_width; ++i)
        display->data[i] = get_value(i / (display_width-1.0));
    display->Refresh();
}

static void rgb2hsv(unsigned char r, unsigned char g, unsigned char b,
                    unsigned char &h, unsigned char &s, unsigned char &v)
{
    v = max(max(r, g), b);
    h = s = 0;
    if (v == 0) 
        return;
    unsigned char delta = v - min(min(r, g), b);
    s = 255 * delta / v;
    if (s == 0) 
        return;
    if (v == r) 
        h = 43 * (g - b) / delta;
    else if (v == g) 
        h = 85 + 43 * (b - r) / delta;
    else  //  v == b
        h = 171 + 43 * (r - g) / delta;
}

static wxColour hsv2wxColour(unsigned char h, unsigned char s, unsigned char v)
{
    if (s == 0) 
        return wxColour(v, v, v);

    float hx = h / 42.501;      // 0 <= hx <= 5.
    int i = (int) floor(hx);
    float f = hx - i; 
    float sx = s / 255.;
    unsigned char p = iround(v * (1 - sx));
    unsigned char q = iround(v * (1 - sx * f));
    unsigned char t = iround(v * (1 - sx * (1 - f)));

    switch(i) {
        case 0:
            return wxColour(v, t, p);
        case 1:
            return wxColour(q, v, p);
        case 2:
            return wxColour(p, v, t);
        case 3:
            return wxColour(p, q, v);
        case 4:
            return wxColour(t, p, v);
        case 5:                
        default:
            return wxColour(v, p, q);
    }
}


wxColour GradientDlg::get_value(float x)
{
    wxColour c;
    if (x < 0) 
        x = 0;
    if (x > 1)
        x = 1;
    int kind = kind_rb->GetSelection();
    if (kind == 0 || kind == 1) { //hsv
        unsigned char h1, s1, v1, h2, s2, v2;
        rgb2hsv(from->r->GetValue(), from->g->GetValue(), from->b->GetValue(), 
                h1, s1, v1);
        rgb2hsv(to->r->GetValue(), to->g->GetValue(), to->b->GetValue(), 
                h2, s2, v2);
        int corr = 0;
        if (kind == 0 && h1 > h2)
            corr = 256;
        else if (kind == 1 && h1 < h2)
            corr = -256;
        c = hsv2wxColour(iround(h1 * (1-x) + (h2 + corr) * x), 
                         iround(s1 * (1-x) + s2 * x), 
                         iround(v1 * (1-x) + v2 * x)); 
    }
    else if (kind == 2) { //rgb
        c = wxColour(
                iround(from->r->GetValue() * (1-x) + to->r->GetValue() * x), 
                iround(from->g->GetValue() * (1-x) + to->g->GetValue() * x), 
                iround(from->b->GetValue() * (1-x) + to->b->GetValue() * x)); 
    }
    else { //one color
        c = wxColour(from->r->GetValue(), from->g->GetValue(), 
                        from->b->GetValue());
    }
    return c;
}

//===============================================================
//                            ListWithColors
//===============================================================

BEGIN_EVENT_TABLE(ListWithColors, wxListView)
    EVT_LIST_COL_CLICK(-1, ListWithColors::OnColumnMenu)
    EVT_LIST_COL_RIGHT_CLICK(-1, ListWithColors::OnColumnMenu)
    EVT_RIGHT_DOWN (ListWithColors::OnRightDown)
    EVT_MENU_RANGE (ID_DL_CMENU_SHOW_START, ID_DL_CMENU_SHOW_END, 
                    ListWithColors::OnShowColumn)
    EVT_MENU (ID_DL_CMENU_FITCOLS, ListWithColors::OnFitColumnWidths)
    EVT_MENU (ID_DL_SELECTALL, ListWithColors::OnSelectAll)
    EVT_KEY_DOWN (ListWithColors::OnKeyDown)
END_EVENT_TABLE()
    
ListWithColors::ListWithColors(wxWindow *parent, wxWindowID id, 
                               vector<pair<string,int> > const& columns_)
    : wxListView(parent, id, wxDefaultPosition, wxDefaultSize,
                 wxLC_REPORT|wxLC_HRULES|wxLC_VRULES),
      columns(columns_), sidebar(0)
{
    for (int i = 0; i < size(columns); ++i)
        if (columns[i].second != 0)
            InsertColumn(i, s2wx(columns[i].first), wxLIST_FORMAT_LEFT,
                         columns[i].second);
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
            InsertItem(i, wxT(""));
    }
    for (int i = 0; i < length; ++i) {
        int c = 0;
        for (int j = 0; j < size(columns); ++j) {
            if (columns[j].second) {
                SetItem(i, c, s2wx(data[i*columns.size()+j]), 
                              c == 0 ? i : -1);
                ++c;
            }
        }
        if (active != -2)
            Select(i, i == active);
    }
    list_data = data;
    
    if (active >= 0)
        Focus(active);
    Thaw();
}

void ListWithColors::OnColumnMenu(wxListEvent& WXUNUSED(event))
{
    wxMenu popup_menu; 
    for (int i = 0; i < size(columns); ++i) {
        popup_menu.AppendCheckItem(ID_DL_CMENU_SHOW_START+i, 
                                   s2wx(columns[i].first));
        popup_menu.Check(ID_DL_CMENU_SHOW_START+i, columns[i].second);
    }
    popup_menu.AppendSeparator();
    popup_menu.Append(ID_DL_CMENU_FITCOLS, wxT("Fit Columns"));
    PopupMenu (&popup_menu, 10, 3);
}

void ListWithColors::OnRightDown(wxMouseEvent &event)
{
    wxMenu popup_menu; 
    popup_menu.Append(ID_DL_SELECTALL, wxT("Select &All"));
    popup_menu.Append(ID_DL_SWITCHINFO, wxT("Show/Hide &Info"));
    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void ListWithColors::OnShowColumn(wxCommandEvent &event)
{
    int n = event.GetId() - ID_DL_CMENU_SHOW_START;
    int col=0;
    for (int i = 0; i < n; ++i)
        if (columns[i].second)
            ++col;
    //TODO if col==0 take care about images
    bool show = event.IsChecked();
    if (show) {
        InsertColumn(col, s2wx(columns[n].first));
        for (int i = 0; i < GetItemCount(); ++i)
            SetItem(i, col, s2wx(list_data[i*columns.size()+n]));
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

void ListWithColors::OnKeyDown (wxKeyEvent& event)
{
    switch (event.m_keyCode) {
        case WXK_DELETE:
            if (sidebar)
                sidebar->delete_selected_items();
            break;
        default:
            event.Skip();
    }
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
    : wxTextCtrl(parent, id, wxT(""), pos, size,
                 wxTE_MULTILINE|wxTE_RICH|wxNO_BORDER|wxTE_READONLY)
{}

void OutputWin::fancy_dashes() {
    for (int i = 0; i < 16; i++) {
        SetDefaultStyle (wxTextAttr (wxColour(i * 16, i * 16, i * 16)));
        AppendText (wxT("-"));
    }
    AppendText (wxT("\n"));
}

void OutputWin::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/OutputWin/Colors"));
    text_color[os_normal] = cfg_read_color(cf, wxT("normal"), 
                                                      wxColour(150, 150, 150));
    text_color[os_warn] = cfg_read_color(cf, wxT("warn"), 
                                                    wxColour(200, 0, 0));
    text_color[os_quot] = cfg_read_color(cf, wxT("quot"), 
                                                    wxColour(50, 50, 255));
    text_color[os_input] = cfg_read_color(cf, wxT("input"), 
                                                     wxColour(0, 200, 0));
    bg_color = cfg_read_color(cf, wxT("bg"), wxColour(20, 20, 20));
    cf->SetPath(wxT("/OutputWin"));
    wxFont font = cfg_read_font(cf, wxT("font"), wxNullFont);
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
    cf->SetPath(wxT("/OutputWin/Colors"));
    cfg_write_color (cf, wxT("normal"), text_color[os_normal]);  
    cfg_write_color (cf, wxT("warn"), text_color[os_warn]); 
    cfg_write_color (cf, wxT("quot"), text_color[os_quot]); 
    cfg_write_color (cf, wxT("input"), text_color[os_input]); 
    cfg_write_color (cf, wxT("bg"), bg_color); 
    cf->SetPath(wxT("/OutputWin"));
    cfg_write_font (cf, wxT("font"), GetDefaultStyle().GetFont());
}

void OutputWin::append_text (OutputStyle style, const wxString& str)
{
    SetDefaultStyle (wxTextAttr (text_color[style]));
    AppendText (str);
    ShowPosition(GetLastPosition());
    //TODO TextCtrl is not refreshed when program is busy
    //I think it used to work with older versions of wx
    //Refresh() and Update() here don't help
    Refresh(true);
    Update();
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
    wxMenu popup_menu (wxT("output text menu"));

    wxMenu *color_menu = new wxMenu;
    color_menu->Append (ID_OUTPUT_C_BG, wxT("&Background"));
    color_menu->Append (ID_OUTPUT_C_IN, wxT("&Input"));
    color_menu->Append (ID_OUTPUT_C_OU, wxT("&Output"));
    color_menu->Append (ID_OUTPUT_C_QT, wxT("&Quotation"));
    color_menu->Append (ID_OUTPUT_C_WR, wxT("&Warning"));
    popup_menu.Append  (ID_OUTPUT_C   , wxT("&Color"), color_menu);

    popup_menu.Append  (ID_OUTPUT_P_FONT, wxT("&Font"));
    popup_menu.Append  (ID_OUTPUT_P_CLEAR, wxT("Clea&r"));

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
            frame->set_status_text(wx2s(s));
            history.insert(++history.begin(), s);
            h_pos = history.begin();
            Clear();
            exec_command(wx2s(s)); //displaying and executing command
            }
            if (spin_button) {
                spin_button->SetRange(0, history.size() - 1);
                spin_button->SetValue(0);
            }
            SetFocus();
            break;
        case WXK_TAB:
            {
            IOPane *parent = static_cast<IOPane*>(GetParent());//to not use RTTI
            parent->focus_output();
            }
            break;
        case WXK_UP:
        case WXK_NUMPAD_UP:
            history_up();
            if (spin_button)
                spin_button->SetValue(spin_button->GetValue()+1);
            SetFocus();
            break;
        case WXK_DOWN:
        case WXK_NUMPAD_DOWN:
            history_down();
            if (spin_button)
                spin_button->SetValue(spin_button->GetValue()-1);
            SetFocus();
            break;
        case WXK_PAGEUP:
        case WXK_NUMPAD_PAGEUP:
            h_pos == --history.end();
            SetValue(*h_pos);
            if (spin_button)
                spin_button->SetValue(spin_button->GetMax());
            SetFocus();
            break;
        case WXK_PAGEDOWN:
        case WXK_NUMPAD_PAGEDOWN:
            h_pos == history.begin();
            SetValue(*h_pos);
            if (spin_button)
                spin_button->SetValue(spin_button->GetMin());
            SetFocus();
            break;
        default:
            event.Skip();
    }
}

void InputField::history_up()
{
    if (h_pos == --history.end())
        return;
    ++h_pos;
    SetValue(*h_pos);
}

void InputField::history_down()
{
    if (h_pos == history.begin())
        return;
    --h_pos;
    SetValue(*h_pos);
}


