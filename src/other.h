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
    Rect view;
    bool plus_background;
    static const fp relative_view_x_margin, relative_view_y_margin;

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
    std::string view_info() const;
    void set_view (Rect rect, bool fit = false);
    void set_view_h (fp l, fp r) {set_view (Rect(l, r, view.bottom, view.top));}
    void set_view_v (fp b, fp t) {set_view (Rect(view.left, view.right, b, t));}
    void set_view_y_fit();
    void set_plus_background(bool b) {plus_background=b; v_was_changed = true;}
    void v_was_plotted() { v_was_changed = false; } 
    bool was_changed() const { return v_was_changed; } 

private:
    char logging_mode;
    std::string log_filename;
    std::ofstream logfile;
    std::map<char, std::string> autoplot_enum;
    std::map<char, std::string> verbosity_enum;
    bool v_was_changed;

    Various_commands (const Various_commands&); //disable
};

void reset_all (bool finito = false); 
void dump_all_as_script (std::string filename);

DotSet *set_class_p (char c);
std::string list_help_topics();
std::string print_help_on_topic (std::string topic);

extern Various_commands *my_other;

class MainManager
{
public:
    MainManager() { reset_all(); }
    ~MainManager() {reset_all (true); }
};

#endif 
