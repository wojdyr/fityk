// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "ui.h"
#include <fstream>
#include <string>
#include <iostream>
#include "other.h"

using namespace std;

const char* config_dirname = ".fityk";
const char* startup_commands_filename = "init";

//utility for storing lines from script file: line number and line as a string
struct NumberedLine
{
    int nr;  //1 - first line, etc.
    string txt;
    NumberedLine(int nr_, string txt_) : nr(nr_), txt(txt_) {}
};


//this is a part of Singleton design pattern
UserInterface* UserInterface::instance = 0; // initialize pointer
UserInterface* UserInterface::getInstance() 
{
    if (instance == 0)  // is it the first call?
        instance = new UserInterface; // create sole instance
    return instance; // address of sole instance
}


UserInterface::UserInterface() 
    : log_mode('n'), log_filename(), verbosity(3), exit_on_warning(false),
      auto_plot(2)
{
    verbosity_enum [0] = "silent";
    verbosity_enum [1] = "only-warnings";
    verbosity_enum [2] = "rather-quiet";
    verbosity_enum [3] = "normal";
    verbosity_enum [4] = "verbose";
    verbosity_enum [5] = "very-verbose";
    epar.insert (pair<string, Enum_string> ("verbosity", 
                               Enum_string (verbosity_enum, &verbosity)));
    autoplot_enum [1] = "never";
    autoplot_enum [2] = "on-plot-change";
    autoplot_enum [3] = "on-fit-iteration";
    epar.insert (pair<string, Enum_string> ("autoplot", 
                               Enum_string (autoplot_enum, &auto_plot)));
    bpar ["exit-on-warning"] = &exit_on_warning;
    //ipar ["plot-min-curve-points"] = &smooth_limit;
}

void UserInterface::startLog(char mode, const string& filename)
{
    if (mode == 0)
        mode = 'a';
    stopLog();
    logfile.open (filename.c_str(), ios::app);
    if (!logfile) {
        warn ("Can't open file for writing: " + filename);
        return;
    }
    logfile << fityk_version_line << endl;
    logfile << "### AT "<< time_now() << "### START LOGGING ";
    switch (mode) {
        case 'i':
            info ("Logging input to file: " + filename);
            logfile << "INPUT";
            break;
        case 'o':
            info ("Logging output to file: " + filename);
            logfile << "OUTPUT";
            break;
        case 'a':
            info ("Logging input and output to file: " + filename);
            logfile << "INPUT AND OUTPUT";
            break;
        default:
            assert(0);
    }
    logfile << " TO THIS FILE (" << filename << ")\n";
    log_filename = filename;
    log_mode = mode;
}

void UserInterface::stopLog()
{
    if (log_mode != 'n') {  
        logfile.close();
        log_mode = 'n';
    }
}

string UserInterface::getLogInfo() const
{
    switch (log_mode) {
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

void UserInterface::outputMessage (int level, const string& s)
{
    OutputStyle style = level <= 1 ? os_warn : os_normal;
    if (level <= verbosity) {
        showMessage(style, s);
        log_output (s);
    }
    if (exit_on_warning && style == os_warn) {
        showMessage(os_normal, "Warning -> exiting program.");
        close();
    }
}


void UserInterface::log_output (const string& s)
{
    if (log_mode == 'o' || log_mode == 'a') {
        // insert "# " at the beginning of string and before every new line
        logfile << "# ";
        for (const char *p = s.c_str(); *p; p++) {
            logfile << *p;
            if (*p == '\n')
                logfile << "# ";
        }
        logfile << endl;
    }
}


void UserInterface::execScript (const string& filename, 
                                const vector<int>& selected_lines)
{
    // every two int's in selected_lines are treated as range (first-second).
    // TODO change it to vector<pair<int, int> >
    assert (selected_lines.size() % 2 == 0);

    ifstream file (filename.c_str(), ios::in);
    if (!file) {
        warn ("Can't open file: " + filename);
        return;
    }

    vector<NumberedLine> nls, //all lines from file
                         exec_nls; //lines to execute (in order of execution)

    //fill nls for easier manipulation of file lines
    string s;
    int line_index = 1;
    while (getline (file, s)) 
        nls.push_back(NumberedLine(line_index++, s));

    if (!selected_lines.empty())
        //TODO will be changed from vector<int> to vector<pair<int, int> >
        for (vector<int>::const_iterator i = selected_lines.begin(); 
                                         i < selected_lines.end(); i += 2) {
            int f = max(*i, 1);  // f and t are 1-based (not 0-based)
            int t = min(*(i+1), size(nls));
            exec_nls.insert (exec_nls.end(), nls.begin()+f-1, nls.begin()+t);
        }
    else
        exec_nls = nls;

    for (vector<NumberedLine>::const_iterator i = exec_nls.begin(); 
                                                    i != exec_nls.end(); i++) {
        if (i->txt.length() == 0)
            continue;
        showMessage (os_quot, S(i->nr) + "> " + i->txt); 
        bool r = cmd_parser(i->txt);
        if (!r)
            break;
    }
}

void UserInterface::drawPlot (int pri, bool now, const std::vector<fp>& a)
{
    if (pri <= auto_plot && (now || AL->was_changed())) {
            doDrawPlot(now, a);
            if (a.empty())
                AL->was_plotted();
    }
}


