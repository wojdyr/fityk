// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef other__H__
#define other__H__

#include <string>
#include "dotset.h"
#include "pag.h" //temporary

class PlotCore;
class Parameters;


//now it's only initilizing all classes...
//
class ApplicationLogic
{
public:
    ApplicationLogic() : c_was_changed(false), params(0) { reset_all(); }
    ~ApplicationLogic() { reset_all(true); }
    void reset_all (bool finish=false); 
    void dump_all_as_script (std::string filename);

    void activate(int p, int d);
    int append_core(); // append and activate new PlotCore
    int append_data(int p);
    void remove(int p, int d);
    void remove_core(int p);
    int get_core_count() const { return cores.size(); }
    const PlotCore *get_core(int n) const; 
    int get_active_core_position() const { return active_core; }
    const PlotCore *get_active_core() const { return get_core(active_core); }
    bool was_changed() const; 
    void was_plotted();
    const Parameters* pars() const { return params; } 
    Parameters* pars() { return params; } 

    //temporary - these 3 funcs will be removed/changed
    int refs_to_a (Pag p) const;
    std::string descr_refs_to_a (Pag p) const;
    void synch_after_rm_a (Pag p);
protected:
    std::vector<PlotCore*> cores;
    bool c_was_changed;
    int active_core;
    Parameters *params; //this Parameters instance is common for all PlotCore's

    bool activate_core(int n); 
};


DotSet *set_class_p (char c);

extern ApplicationLogic *AL;

#endif 
