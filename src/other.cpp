// This file is part of fityk program. Copyright (C) Marcin Wojdyr
#include "common.h"
RCSID ("$Id$")

#include "other.h"
#include <stdio.h>
#include <fstream>
#include "ffunc.h"
#include "data.h"
#include "sum.h"
#include "v_IO.h"
#include "v_fit.h"
#include "manipul.h"
#ifdef USE_XTAL
    #include "crystal.h"
#endif

using namespace std;

Various_commands *my_other;
PlotCore *my_core;
ApplicationLogic *AL;

const fp PlotCore::relative_view_x_margin = 1./20.;
const fp PlotCore::relative_view_y_margin = 1./20.;

//==================================================================


Various_commands::Various_commands() 
    : logging_mode('n'), log_filename()
{
    verbosity_enum [0] = "silent";
    verbosity_enum [1] = "only-warnings";
    verbosity_enum [2] = "rather-quiet";
    verbosity_enum [3] = "normal";
    verbosity_enum [4] = "verbose";
    verbosity_enum [5] = "very-verbose";
    epar.insert (pair<string, Enum_string> ("verbosity", 
                               Enum_string (verbosity_enum, &verbosity)));
    autoplot_enum [0] = "really-never";
    autoplot_enum [1] = "never";
    autoplot_enum [2] = "on-plot-change";
    autoplot_enum [3] = "on-fit-iteration";
    epar.insert (pair<string, Enum_string> ("autoplot", 
                               Enum_string (autoplot_enum, &auto_plot)));
    bpar ["exit-on-error"] = &exit_on_error;
    //ipar ["plot-min-curve-points"] = &smooth_limit;
}

Various_commands& Various_commands::operator= (const Various_commands& v)
{
    this->DotSet::operator=(v);
    logging_mode = v.logging_mode;
    if (logging_mode != 'n')
        warn ("Unexpected error in Various_commands::operator=()");
    log_filename = v.log_filename;
    return *this;
}

void Various_commands::start_logging_to_file (std::string filename, char mode)
{
    if (mode == 0)
        mode = 'a';
    stop_logging_to_file();
    logfile.open (filename.c_str(), ios::app);
    if (!logfile) {
        warn ("Can't open file for writing: " + filename);
        return;
    }
    logfile << "\n### AT "<< time_now() << "### START LOGGING ";
    switch (mode) {
        case 'i':
            mesg ("Logging input to file: " + filename);
            logfile << "INPUT";
            break;
        case 'o':
            mesg ("Logging output to file: " + filename);
            logfile << "OUTPUT";
            break;
        case 'a':
            mesg ("Logging input and output to file: " + filename);
            logfile << "INPUT AND OUTPUT";
            break;
        default:
            assert(0);
    }
    logfile << " TO THIS FILE (" << filename << ")\n";
    log_filename = filename;
    logging_mode = mode;
}

void Various_commands::stop_logging_to_file ()
{
    if (logging_mode != 'n') {  
        logfile.close();
        logging_mode = 'n';
    }
}

string Various_commands::logging_info() const
{
    switch (logging_mode) {
        case 'a':
            return "Logging input and output to file: " + log_filename;
        case 'i':
            return "Logging input to file: " + log_filename;
        case 'o':
            return "Logging output to file: " + log_filename;
        case 'n':
            return "No logging to file now.";
        default: 
            assert(0);
            return "";
    }
}

void Various_commands::log_input (const string& s)
{
     if (logging_mode == 'i' || logging_mode == 'a')  
                 logfile << " " << s << endl;

}

void Various_commands::log_output (const string& s)
{
    if (logging_mode == 'o' || logging_mode == 'a') {
        logfile << "# ";
        for (const char *p = s.c_str(); *p; p++) {
            logfile << *p;
            if (*p == '\n')
                logfile << "# ";
        }
        logfile << endl;
    }
}

bool Various_commands::include_file (std::string name, std::vector<int> lines)
{
    assert (lines.size() % 2 == 0);
    file_I_stdout_O fio(lines);
    return fio.start(name.c_str());
}

int Various_commands::sleep (int seconds)
{
    return my_sleep (seconds);
}

//==================================================================

PlotCore::PlotCore()
    : view(0, 180, 0, 1e3), plus_background(false),
      ds_was_changed(false), v_was_changed(false)
{
    sum = new Sum;
#ifdef USE_XTAL
    crystal = new Crystal(sum);
#endif //USE_XTAL
    activate_data(-1); //and set_my_vars() is called from there
}

PlotCore::~PlotCore()
{
#ifdef USE_XTAL
    delete crystal;
#endif //USE_XTAL
    delete sum;
    for (vector<Data*>::iterator i = datasets.begin(); i != datasets.end(); i++)
        delete *i;
}

bool PlotCore::activate_data(int n)
{
    int sel = 0;
    if (n == -1) {
        datasets.push_back(new Data);
        sel = datasets.size() - 1;
    }
    else if (n >= 0 && n < size(datasets)) {
        sel = n;
    }
    else {
        warn("No such datafile in this dataset: " + S(n));
        return false;
    }

    if (sel != active_data) {
        active_data = sel;
        ds_was_changed = true;
    }
    set_my_vars();
    return true;
}

void PlotCore::set_my_vars() const
{
    my_data = datasets[active_data];//TODO
    my_sum = sum;
    my_crystal = crystal;
}

std::string PlotCore::view_info() const
{ 
    return view.str() + 
        (plus_background ? "\nAdding background while plotting." : "");
}

void PlotCore::set_view (Rect rt, bool fit)
{
    v_was_changed = true;
    if (my_data->is_empty()) {
        view.left = (rt.left < rt.right && rt.left != -INF ? rt.left : 0);
        view.right = (rt.left < rt.right && rt.right != +INF ? rt.right : 180);
        view.bottom = (rt.bottom < rt.top && rt.bottom != -INF ? rt.bottom : 0);
        view.top = (rt.bottom < rt.top && rt.top != +INF ? rt.top : 1e3);
        return;
    }
    fp x_min = my_data->get_x_min();
    fp x_max = my_data->get_x_max();
    if (x_min == x_max) x_min -= 0.1, x_max += 0.1;
    view = rt;
    // if rt is too wide or too high, 
    // it is narrowed to sensible size (containing all data + margin)
    // the same if rt.left >= rt.right or rt.bottom >= rt.top
    fp x_size = x_max - x_min;
    const fp sens_mult = 10.;
    if (view.left <  x_min - sens_mult * x_size)
        view.left = x_min - x_size * relative_view_x_margin;
    if (view.right >  x_max + sens_mult * x_size)
        view.right = x_max + x_size * relative_view_x_margin; 
    if (view.left >= view.right) {
        view.left = x_min - x_size * relative_view_x_margin;
        view.right = x_max + x_size * relative_view_x_margin; 
    }
    if (fit || view.bottom == -INF && view.top == INF 
            || view.bottom >= view.top)
        set_view_y_fit();
    else {
        fp y_min = my_data->get_y_min(false);
        fp y_max = my_data->get_y_max(false);
        if (y_min == y_max) y_min -= 0.1, y_max += 0.1;
        fp y_size = y_max - y_min;
        if (view.bottom <  y_min - sens_mult * y_size)
            view.bottom = min (y_min, 0.);
        if (view.top >  y_max + y_size)
            view.top = y_max + y_size * relative_view_y_margin;;
    }
}

void PlotCore::set_view_y_fit()
{
    if (my_data->is_empty()) {
        warn ("Can't set view when no points are loaded");
        return;
    }
    /*
    if (my_data->get_n() == 0) {
        warn ("No active points. Y range not changed.");
        return;
    }
    */
    vector<Point>::const_iterator f = my_data->get_point_at(view.left);
    vector<Point>::const_iterator l = my_data->get_point_at(view.right);
    if (f >= l) {
        view.bottom = 0.;
        view.top = 1.;
        return;
    }
    fp y_max = 0., y_min = 0.;
    //first we are searching for minimal and max. y in active points
    bool min_max_set = false;
    for (vector<Point>::const_iterator i = f; i < l; i++) {
        if (i->is_active) {
            fp y = plus_background ? i->orig_y : i->y;
            if (!min_max_set) min_max_set = true;
            if (y > y_max) y_max = y;
            if (y < y_min) y_min = y;
        }
    }
    if (!min_max_set || y_min == y_max) { //none or 1 active point, so now we  
        min_max_set = false;       // search for min. and max. y in all points 
        for (vector<Point>::const_iterator i = f; i < l; i++) { 
            fp y = plus_background ? i->orig_y : i->y;
            if (!min_max_set) min_max_set = true;
            if (y > y_max) y_max = y;
            if (y < y_min) y_min = y;
        }
    }
    if (!min_max_set || y_min == y_max) { //none or 1 point, so now we  
        if (min_max_set) y_min -= 0.1, y_min += 0.1;
        else y_min = 0, y_max = +1;
    }
    view.bottom = y_min;
    view.top = y_max + (y_max - y_min) * relative_view_y_margin;;
}


bool PlotCore::was_changed() const
{ 
    for (vector<Data*>::const_iterator i = datasets.begin(); 
                                                    i != datasets.end(); i++)
        if ((*i)->was_changed())
            return true;
    //if here - no Data changed
    return v_was_changed || ds_was_changed || sum->was_changed();
}

void PlotCore::was_plotted() 
{ 
    ds_was_changed = false; 
    v_was_changed = false;
    for (vector<Data*>::iterator i = datasets.begin(); i != datasets.end(); i++)
        (*i)->d_was_plotted();
    sum->s_was_plotted();
}

vector<string> PlotCore::get_data_titles() const
{
    vector<string> v;
    //v.reserve(datasets.size());
    for (vector<Data*>::const_iterator i = datasets.begin(); 
                                                    i != datasets.end(); i++)
        v.push_back((*i)->get_title());
    return v;
}

const Data *PlotCore::get_data(int n) const
{
    return (n >= 0 && n < size(datasets)) ?  datasets[n] : 0;
}

void PlotCore::del_data(int n)
{
    if (n >= 0 && n < size(datasets)) {
        if (n == active_data) 
            active_data = n > 0 ? n-1 : 0;
        delete datasets[n];
        datasets.erase(datasets.begin() + n);
        if (datasets.empty()) //it should not be empty
            datasets.push_back(new Data);
        ds_was_changed = true;
    }
    else
        warn("No such dataset number: " + S(n));
}

//==========================================================================

void ApplicationLogic::reset_all (bool finish) 
{
    delete my_manipul;
    delete my_other;
    delete fitMethodsContainer;
    for (vector<PlotCore*>::iterator i = cores.begin(); i != cores.end(); i++)
        delete *i;
    cores.clear();
    if (finish)
        return;
    fitMethodsContainer = new FitMethodsContainer;
    my_other = new Various_commands;
    my_manipul = new Manipul;
    activate_core(-1);
}


void ApplicationLogic::dump_all_as_script (string filename)
{
    ofstream os(filename.c_str(), ios::out);
    if (!os) {
        warn ("Can't open file: " + filename);
        return;
    }
    os << "####### Dump time: " << time_now() << endl;
    os << my_other->set_script('o') << endl;
    //TODO all datasets, etc
    my_data->export_as_script (os);
    os << endl;
    my_sum->export_as_script (os);
    os << endl;
#ifdef USE_XTAL
    my_crystal->export_as_script (os);
    os << endl;
#endif //USE_XTAL
    fitMethodsContainer->export_methods_settings_as_script(os);
    os << "f.method " << my_fit->symbol <<" ### back to current method\n";
    os << endl;
    os << "o.plot " << my_core->view.str() << endl;
    os << endl << "####### End of dump " << endl; 
}


bool ApplicationLogic::activate_core(int p)
{
    int sel = 0;
    if (p == -1) {
        cores.push_back(new PlotCore);
        sel = cores.size() - 1;
    }
    else if (p >= 0 && p < size(cores)) {
        sel = p;
    }
    else {
        warn("No such plot: " + S(p));
        return false;
    }

    if (sel != active_core) {
        active_core = sel;
        c_was_changed = true;
    }
    my_core = cores[active_core];//TODO
    my_core->set_my_vars();
    return true;
}

bool ApplicationLogic::was_changed() const
{ 
    return c_was_changed || get_active_core()->was_changed();
}

void ApplicationLogic::was_plotted() 
{ 
    c_was_changed = false;
    cores[active_core]->was_plotted();
}

const PlotCore *ApplicationLogic::get_core(int n) const
{
    return (n >= 0 && n < size(cores)) ?  cores[n] : 0;
}

void ApplicationLogic::del_core(int n)
{
    if (n >= 0 && n < size(cores)) {
        if (n == active_core) 
            active_core = n > 0 ? n-1 : 0;
        delete cores[n];
        cores.erase(cores.begin() + n);
        if (cores.empty()) //it should not be empty
            cores.push_back(new PlotCore);
        c_was_changed = true;
    }
    else
        warn("Not a plot number: " + S(n));
}


//==================================================================

DotSet *
set_class_p (char c)
{
    switch (c) {
        case 'd': return my_data;
        case 'f': return my_fit;
        case 's': return my_sum;
        case 'm': return my_manipul;
#ifdef USE_XTAL
        case 'c': return my_crystal;
#endif //USE_XTAL
        case 'o': return my_other;
        default : return 0;
    }
}


