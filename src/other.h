// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef other__H__
#define other__H__

#include <string>
#include "dotset.h"

struct Rect 
{
    fp left, right, bottom, top;
    Rect (fp l, fp r, fp b, fp u) : left(l), right(r), bottom(b), top(u) {}
    Rect (fp l, fp r) : left(l), right(r), bottom(0), top(0) {}
    Rect () : left(0), right(0), bottom(0), top(0) {}
    fp width() const { return right - left; }
    fp height() const { return top - bottom; }
    std::string str() const
    { 
        return "[" + (left!=right ? S(left) + ":" + S(right) : std::string(" "))
            + "] [" + (bottom!=top ? S(bottom) + ":" + S(top) 
                                               : std::string (" ")) + "]";
    }
};

class Various_commands : public DotSet
{
public:    

    Various_commands();
    Various_commands& operator= (const Various_commands& v);
    void start_logging_to_file (std::string filename, char mode);
    void stop_logging_to_file ();
    std::string logging_info() const;
    char get_log_mode() const { return logging_mode; };
    std::string get_log_filename() const { return log_filename; };
    void log_input  (const std::string& s);
    void log_output (const std::string& s);
    bool include_file (std::string name, std::vector<int> lines);
    int sleep (int seconds);

private:
    char logging_mode;
    std::string log_filename;
    std::ofstream logfile;
    std::map<char, std::string> autoplot_enum;
    std::map<char, std::string> verbosity_enum;

    Various_commands (const Various_commands&); //disable
};


class PlotCore
{
public:
    static const fp relative_view_x_margin, relative_view_y_margin;
    Rect view;
    bool plus_background;

    PlotCore();
    ~PlotCore();

    bool activate_data(int n); // for n=-1 create new dataset
    std::vector<std::string> get_data_titles() const;
    int get_data_count() const { return datasets.size(); }
    const Data *get_data(int n) const; 
    int get_active_data_position() const { return active_data; }
    const Data *get_active_data() const { return get_data(active_data);}
    void set_my_vars() const;
    void del_data(int n);

    std::string view_info() const;
    void set_view (Rect rect, bool fit = false);
    void set_view_h (fp l, fp r) {set_view (Rect(l, r, view.bottom, view.top));}
    void set_view_v (fp b, fp t) {set_view (Rect(view.left, view.right, b, t));}
    void set_view_y_fit();
    void set_plus_background(bool b) {plus_background=b; v_was_changed = true;}

    bool was_changed() const; 
    void was_plotted();

private:
    ///each dataset (class Data) usually comes from one datafile
    std::vector<Data*> datasets;
    Sum *sum;
#ifdef USE_XTAL
    Crystal *crystal;
#endif //USE_XTAL
    int active_data; //position of selected dataset in vector<Data*> datasets
    bool ds_was_changed; //selection of dataset changed
    bool v_was_changed; //view was changed

    PlotCore (const PlotCore&); //disable
};


//now it's only initilizing all classes...
//
class ApplicationLogic
{
public:
    ApplicationLogic() : c_was_changed(false) { reset_all(); }
    ~ApplicationLogic() { reset_all(true); }
    void reset_all (bool finish=false); 
    void dump_all_as_script (std::string filename);

    bool activate_core(int n); // for n=-1 create new plot-core
    int get_core_count() const { return cores.size(); }
    const PlotCore *get_core(int n) const; 
    int get_active_core_position() const { return active_core; }
    const PlotCore *get_active_core() const { return get_core(active_core); }
    void del_core(int n);
    bool was_changed() const; 
    void was_plotted();
protected:
    std::vector<PlotCore*> cores;
    bool c_was_changed;
    int active_core;
};


DotSet *set_class_p (char c);

extern Various_commands *my_other;
extern PlotCore *my_core;
extern ApplicationLogic *AL;

#endif 
