// This file is part of fityk program. Copyright (C) Marcin Wojdyr

// CLI-only file 
// in this file: main loop, readline support (command expansion)
// and part of UserInterface implementation (CLI-specific)

#include "common.h"
RCSID ("$Id$")
            
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <locale.h>
#ifndef NO_READLINE
#    include <readline/readline.h>
#    include <readline/history.h>
#endif
#include "ui.h"
#include "u_gnuplot.h"
#include "other.h"

using namespace std;

struct ExitRequestedException : exception {};


//------ UserInterface - implementation of CLI specific methods ------

void UserInterface::showMessage (OutputStyle style, const string& s)
{
    if (style == os_warn)
        cout << '\a';
    cout << s << endl;
}

void UserInterface::doDrawPlot (bool /*now*/, const vector<fp>& a)
{
    static GnuPlot my_gnuplot;
    my_gnuplot.plot (a);
}

void UserInterface::wait (float seconds) 
{
    seconds = fabs(seconds);
    timespec ts;
    ts.tv_sec = static_cast<int>(seconds);
    ts.tv_nsec = static_cast<int>((seconds - ts.tv_sec) * 1e9);
    nanosleep(&ts, 0);
}

void UserInterface::execCommand(const string& s)
{
    bool r = parser(s);
    if (!r)
        close();
}

void UserInterface::close()
{
    throw ExitRequestedException();
}

//----------------------------------------------------------------- 

static const char* prompt = "=-> ";

/// returns absolute path to config directory
string get_config_dir()
{
    static bool first_run = true;
    static string dir;

    if (!first_run)
        return dir;
    char t[200];
    char *home_dir = getenv("HOME");
    if (!home_dir) {
        getcwd(t, 200);
        home_dir = t;
    }
    // '/' is assumed as path separator
    dir = S(home_dir) + "/" + config_dirname + "/"; 
    if (access(dir.c_str(), X_OK) != 0)
        dir = ""; //gcc 2.95 have no std::string.clear() method
    first_run = false;
    return dir;
}



#ifndef NO_READLINE

static char set_kind = 0;
static const char* set_eq_str;


void read_and_execute_input()
{
    char *line = readline (prompt);
    if (!line)
        getUI()->close(); 
    if (line && *line)
        add_history (line);
    string s = line;
    free ((void*) line);
    getUI()->execAndLogCmd(s);
}


char *commands[] = { "d.load", "d.background", "d.calibrate", "d.range", 
        "d.deviation", "d.info", "d.set", "d.export",
        "f.run", "f.continue", "f.set", "f.method", "f.info",
        "s.add", "s.info", "s.remove", "s.change", "s.set", "s.freeze", 
        "s.value", "s.history", "s.export", 
        "m.findpeak", "m.set",
#ifdef USE_XTAL
        "c.wavelength", "c.add", "c.info", "c.remove", "c.set", "c.estimate", 
#endif
        "o.set", "o.plot", "o.log", "o.include", "o.wait", "o.dump",
        "help", "quit" 
        };

char *command_generator (const char *text, int state)
{
    static unsigned int list_index = 0;
    if (!state) 
        list_index = 0;
    while (list_index < sizeof(commands) / sizeof(char*)) {
        char *name = commands[list_index];
        list_index++;
        if (strncmp (name, text, strlen(text)) == 0)
            return strdup(name);
    }
    //if (!state)
        //return strdup(text); // to prevent file expansion
    return 0;
}


char *set_generator (const char *text, int state)
{
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        DotSet *vs = set_class_p (set_kind);
        if (!vs)
            return 0;
        vs->expanp (text, e);
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup (e[list_index].c_str());
    else
        return 0;
}

char *set_eq_generator (const char * /*text*/, int state)
{
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        DotSet *vs = set_class_p (set_kind);
        if (!vs)
            return 0;
        char *p = strchr (set_eq_str, '=');
        while (isspace(*set_eq_str))
            set_eq_str++;
        char *b_eq = p - 1;
        while (b_eq && isspace(*b_eq))
            b_eq--;
        char *a_eq = p + 1;
        while (a_eq && isspace(*a_eq))
            a_eq++;
        vs->expand_enum (string (set_eq_str, b_eq - set_eq_str + 1), 
                                                            string (a_eq), e);
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup (e[list_index].c_str());
    else
        return 0;
}

static bool is_before (int pos, char c)
{
    for (char* i = rl_line_buffer; i <= rl_line_buffer + pos; i++)
        if (*i == c)
            return true;
    return false;
}

char **my_completion (const char *text, int start, int end)
{
    rl_attempted_completion_over = 1;
    for (int i = 0; i < start; i++) 
        if (!isspace (rl_line_buffer[i])) {
            if (start > i + 3 && !strncmp (rl_line_buffer + i + 1, ".s ", 3)
                    || start > i + 5 
                       && !strncmp (rl_line_buffer + i + 1, ".set ", 5)) {
                set_kind = rl_line_buffer[i];
                if (!is_before(start, '='))
                    return rl_completion_matches (text, set_generator);
                else {
                    char *f = rl_line_buffer + i + 2;
                    while (f && !isspace(*f))
                        ++f;
                    while (isspace(*f))
                        ++f;
                    string s(f, rl_line_buffer + end - f + 1);
                    set_eq_str = s.c_str();
                    return rl_completion_matches (text, set_eq_generator);
                }
            }
            else if (is_before(end, '\'')) {
                rl_attempted_completion_over = 0;
                return 0;
            }
            else
                return 0;
        }
    return rl_completion_matches (text, command_generator);
}


/// Reads history (for readline) in ctor and saves it to file in dtor.
/// Proper use: single instance created at the beginning of the program
/// and destroyed at the end.
struct RlHistoryManager
{
    RlHistoryManager();
    ~RlHistoryManager();
    string hist_file;
};

/// read history file
RlHistoryManager::RlHistoryManager()
{
    string fityk_dir = get_config_dir();
    if (!fityk_dir.empty()) {
        hist_file = fityk_dir + "history";
        read_history (hist_file.c_str());
    }
}

/// save history to file
RlHistoryManager::~RlHistoryManager()
{
    //saving command history to file
    if (!hist_file.empty()) {
        int hist_file_size = -1;
        char *hfs = getenv ("HISTFILESIZE");
        if (hfs)
            hist_file_size = atoi (hfs);
        if (hist_file_size <= 0)
            hist_file_size = 500;
        write_history (hist_file.c_str());
        history_truncate_file (hist_file.c_str(), hist_file_size);
    }
}



bool main_loop()
{
    //initialize readline
    rl_readline_name = "fit";
    rl_attempted_completion_function = my_completion;

    RlHistoryManager hm;//it takes care about reading/saving readline history

    //the main loop -- reading input and executing commands
    for (;;)
        read_and_execute_input();

    cout << endl;


    return true;
}


#else //if NO_READLINE 

// the simplest version of user interface -- when readline is not available
bool main_loop()
{
    string s;
    for (;;) {
        cout << prompt;
        if (!getline(cin, s))
            break;
        getUI()->execAndLogCmd(s);
    }
    cout << endl;
    return true;
}

#endif //NO_READLINE 



void interrupt_handler (int /*signum*/)
{
    //set flag for breaking long computations
    user_interrupt = true;
}


int main (int argc, char **argv)
{
    setlocale(LC_NUMERIC, "C");
    // setting ^C handler
    if (signal (SIGINT, interrupt_handler) == SIG_IGN) 
        signal (SIGINT, SIG_IGN);

    AL = new ApplicationLogic;

    // file with initial commands is executed first (if exists)
    string init_file = get_config_dir() + startup_commands_filename;
    if (access(init_file.c_str(), R_OK) == 0) {
        cerr << " -- reading init file: " << init_file << " --\n";
        getUI()->execScript(init_file);
        cerr << " -- end of init file --" << endl;
    }

    // executing files specified as program arguments
    for (int i = 1; i < argc; i++) 
        getUI()->execScript(argv[i]);

    try {
        // the version of main_loop() depends on NO_READLINE  
        main_loop(); 
    } 
    catch(ExitRequestedException) {
        cerr << "\nbye...\n";
    }

    return 0;
}

