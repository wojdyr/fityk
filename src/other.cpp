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
#include "pcore.h"

using namespace std;

Various_commands *my_other;
ApplicationLogic *AL;



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

//==========================================================================

void ApplicationLogic::reset_all (bool finish) 
{
    delete my_manipul;
    delete my_other;
    delete fitMethodsContainer;
    purge_all_elements(cores);
    cores.clear();
    delete params;
    if (finish)
        return;
    fitMethodsContainer = new FitMethodsContainer;
    my_other = new Various_commands;
    my_manipul = new Manipul;
    params = new Parameters;
    append_core();
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
    params->export_as_script(os);
    os << endl;

    for (int i = 0; i != size(cores); i++) {
        if (cores.size() > 1) 
            os << endl << "### core of plot #" << i << endl;
        if (i != 0)
            os << "d.activate *::" << endl; 
        cores[i]->export_as_script(os);
        os << endl;
    }
    if (active_core != size(cores) - 1)
        os << "d.activate " << active_core << ":: # set active" << endl; 
    os << "o.plot " << my_core->view.str() << endl;

    fitMethodsContainer->export_methods_settings_as_script(os);
    os << "f.method " << my_fit->symbol <<" ### back to current method\n";
    os << endl;
    os << endl << "####### End of dump " << endl; 
}

void ApplicationLogic::activate(int p, int d)
{
    if (p != -1)
        activate_core(p);
    cores[active_core]->activate_data(d);
}

void ApplicationLogic::remove(int p, int d)
{
    if (p != -1 && d == -1) //eg. ! 2::
        remove_core(p);
    else if (p != -1 && d != -1) { //eg. ! 2::3
        if (p < 0 || p >= size(cores)) {
            warn("No such plot: " + S(p));
            return;
        }
        cores[p]->remove_data(d);
    }
    else if (p == -1) //eg. ! ::  or  ! ::3
        my_core->remove_data(d);
}

bool ApplicationLogic::activate_core(int p)
{
    if (p < 0 || p >= size(cores)) {
        warn("No such plot: " + S(p));
        return false;
    }

    if (p == active_core) 
        return true;

    active_core = p;
    c_was_changed = true;
    cores[active_core]->set_my_vars();
    return true;
}

int ApplicationLogic::append_core()
{
    cores.push_back(new PlotCore(params));
    active_core = cores.size() - 1;
    c_was_changed = true;
    cores[active_core]->set_my_vars();
    return active_core;
}

int ApplicationLogic::append_data(int p)
{
    if (p != -1)
        activate_core(p);
    return my_core->append_data(); 
}

void ApplicationLogic::remove_core(int p)
{
    if (p >= 0 && p < size(cores)) {
        if (p == active_core) 
            active_core = p > 0 ? p-1 : 0;
        purge_element(cores, p);
        if (cores.empty()) //it should not be empty
            cores.push_back(new PlotCore(params));
        c_was_changed = true;
        cores[active_core]->set_my_vars();
    }
    else
        warn("Not a plot number: " + S(p));
}

bool ApplicationLogic::was_changed() const
{ 
    return c_was_changed || params->was_changed() 
           || get_active_core()->was_changed();
}

void ApplicationLogic::was_plotted() 
{ 
    c_was_changed = false;
    params->was_plotted();
    cores[active_core]->was_plotted();
}

const PlotCore *ApplicationLogic::get_core(int n) const
{
    return (n >= 0 && n < size(cores)) ?  cores[n] : 0;
}


int ApplicationLogic::refs_to_a (Pag p) const
{
    int n = 0;
    for (vector<PlotCore*>::const_iterator i = cores.begin(); i != cores.end();
                                                                        i++)
        n += (*i)->sum->refs_to_ag(p);
    return n;
}

string ApplicationLogic::descr_refs_to_a (Pag p) const
{
    string s = "";
    for (vector<PlotCore*>::const_iterator i = cores.begin(); i != cores.end();
                                                                        i++)
        s += (*i)->sum->descr_refs_to_ag(p);
    return s;
}

void ApplicationLogic::synch_after_rm_a (Pag p)
{
    for (vector<PlotCore*>::iterator i = cores.begin(); i != cores.end(); i++)
        (*i)->sum->synch_after_rm_ag(p);
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


