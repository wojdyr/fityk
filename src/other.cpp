// This file is part of fityk program. Copyright (C) Marcin Wojdyr
#include "common.h"
RCSID ("$Id$")

#include "other.h"
#include <stdio.h>
#include <fstream>
#include "ffunc.h"
#include "data.h"
#include "sum.h"
#include "ui.h"
#include "v_fit.h"
#include "manipul.h"
#ifdef USE_XTAL
    #include "crystal.h"
#endif
#include "pcore.h"

using namespace std;

ApplicationLogic *AL;


void ApplicationLogic::reset_all (bool finish) 
{
    delete my_manipul;
    delete fitMethodsContainer;
    purge_all_elements(cores);
    cores.clear();
    delete params;
    if (finish)
        return;
    fitMethodsContainer = new FitMethodsContainer;
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
    os << fityk_version_line << endl;
    os << "####### Dump time: " << time_now() << endl << endl;
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
        case 'o': return getUI();
        default : return 0;
    }
}


