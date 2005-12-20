// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "ui.h"
#include "settings.h"
#include <fstream>
#include <string>
#include <iostream>
#include "logic.h"
#include "cmd.h"

using namespace std;

const char* config_dirname = ".fityk";
const char* startup_commands_filename = "init";


void Commands::put_command(string const& c, Status s)
{
    if (strip_string(c).empty())
        return;
    cmds.push_back(Cmd(c, s));
    ++command_counter;
    if (!log_filename.empty())  
        log << " " << c << endl; 
}

void Commands::put_output_message(string const& s)
{
    if (!log_filename.empty() && log_with_output) {
        // insert "# " at the beginning of string and before every new line
        log << "# ";
        for (const char *p = s.c_str(); *p; p++) {
            log << *p;
            if (*p == '\n')
                log << "# ";
        }
        log << endl;
    }
}

vector<string> Commands::get_commands(int from, int to, bool with_status) const
{
    vector<string> r;
    if (!cmds.empty())
        for (int i = max(from, 0); i < min(to, size(cmds)); ++i) {
            string s;
            if (with_status)
                s = cmds[i].str();
            else
                s = cmds[i].cmd;
            r.push_back(s);
        }
    return r;
}

int Commands::count_commands_with_status(Status st) const
{
    int cnt = 0;
    for (vector<Cmd>::const_iterator i = cmds.begin(); i != cmds.end(); ++i)
        if (i->status == st)
            ++cnt;
    return cnt;
}

string Commands::get_info() const
{
    string s = S(command_counter) + " commands since the start of the program,";
    if (command_counter == size(cmds))
        s += " of which:";
    else
        s += "\nin last " + S(cmds.size()) + " commands:";
    s += "\n  " + S(count_commands_with_status(status_ok)) 
              + " executed successfully"
        + "\n  " + S(count_commands_with_status(status_execute_error)) 
          + " finished with execute error"
        + "\n  " + S(count_commands_with_status(status_syntax_error))
          + " with syntax error";
    if (log_filename.empty())
        s += "\nCommands are not logged to any file.";
    else
        s += S("\nCommands (") + (log_with_output ? "with" : "without") 
            + " output) are logged to file: " + log_filename;
    return s;
}


void Commands::start_logging(string const& filename, bool with_output)
{
    if (filename.empty())
       stop_logging(); 
    else if (filename == log_filename) {
        if (with_output != log_with_output) {
            log_with_output = with_output;
            log << "### AT "<< time_now() << "### CHANGED TO LOG "
                << (log_with_output ? "WITH" : "WITHOUT") << " OUTPUT\n";
        }
    }
    else {
        stop_logging();
        log_filename = filename;
        log_with_output = with_output;
        log.open(filename.c_str(), ios::app);
        if (!log) 
            throw ExecuteError("Can't open file for writing: " + filename);
        log << fityk_version_line << endl;
        log << "### AT "<< time_now() << "### START LOGGING ";
        if (with_output) {
            info ("Logging input and output to file: " + filename);
            log << "INPUT AND OUTPUT";
        }
        else {
            info ("Logging input to file: " + filename);
            log << "INPUT";
        }
        log << " TO THIS FILE (" << filename << ")\n";
    }
}

void Commands::stop_logging()
{
    log.close();
    log_filename = "";
}


//utility for storing lines from script file: line number and line as a string
struct NumberedLine
{
    int nr;  //1 - first line, etc.
    string txt;
    NumberedLine(int nr_, string txt_) : nr(nr_), txt(txt_) {}
};


//this is a part of Singleton design pattern
UserInterface* UserInterface::instance = 0; 
UserInterface* UserInterface::getInstance() 
{
    if (instance == 0)  // is it the first call?
        instance = new UserInterface; // create sole instance
    return instance; // address of sole instance
}


void UserInterface::outputMessage (int level, const string& s)
{
    OutputStyle style = (level <= 1 ? os_warn : os_normal);
    if (level <= getVerbosity()) {
        showMessage(style, s);
        commands.put_output_message(s);
    }
    if (style == os_warn && getSettings()->get_b("exit-on-warning")) {
        showMessage(os_normal, "Warning -> exiting program.");
        throw ExitRequestedException();
    }
}


/// items in selected_lines are ranges (first, after-last).
/// if selected_lines are empty - all lines from file are executed
void UserInterface::execScript (const string& filename, 
                                const vector<pair<int,int> >& selected_lines)
{
    ifstream file (filename.c_str(), ios::in);
    if (!file) {
        warn ("Can't open file: " + filename);
        return;
    }

    vector<NumberedLine> nls, //all lines from file
                         exec_nls; //lines to execute (in order of execution)

    //fill nls for easier manipulation of file lines
    string s;
    int line_index = 0;
    while (getline (file, s)) 
        nls.push_back(NumberedLine(line_index++, s));

    if (selected_lines.empty())
        exec_nls = nls;
    else
        for (vector<pair<int,int> >::const_iterator i = selected_lines.begin(); 
                                            i != selected_lines.end(); i++) {
            int f = max(i->first, 0);  // f and t are 1-based (not 0-based)
            int t = min(i->second, size(nls));
            exec_nls.insert(exec_nls.end(), nls.begin()+f, nls.begin()+t);
        }

    for (vector<NumberedLine>::const_iterator i = exec_nls.begin(); 
                                                    i != exec_nls.end(); i++) {
        if (i->txt.length() == 0)
            continue;
        showMessage (os_quot, S(i->nr) + "> " + i->txt); 
        parse_and_execute(i->txt);
    }
}

void UserInterface::drawPlot (int pri, bool now)
{
    if (pri <= getSettings()->get_e("autoplot")) 
        doDrawPlot(now);
}


int UserInterface::getVerbosity() { return getSettings()->get_e("verbosity"); }
