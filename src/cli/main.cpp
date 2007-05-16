// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

// CLI-only file 
// in this file: main loop, readline support (command expansion)
// and part of UserInterface implementation (CLI-specific)

            
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <locale.h>
// readline header will be included later, unless NO_READLINE is defined

#include "../common.h"
#include "../ui.h"
#include "../logic.h"
#include "../cmd.h"
#include "../settings.h"
#include "../func.h"
#include "gnuplot.h"

using namespace std;


//------ UserInterface - implementation of CLI specific methods ------

void cli_show_message (OutputStyle style, const string& s)
{
    if (style == os_warn)
        cout << '\a';
    cout << s << endl;
}

void cli_do_draw_plot (bool /*now*/)
{
    static GnuPlot my_gnuplot;
    my_gnuplot.plot();
}

void cli_wait(float seconds) 
{
    seconds = fabs(seconds);
    timespec ts;
    ts.tv_sec = static_cast<int>(seconds);
    ts.tv_nsec = static_cast<int>((seconds - ts.tv_sec) * 1e9);
    nanosleep(&ts, 0);
}

//----------------------------------------------------------------- 

namespace {

const char* prompt = "=-> ";

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

    // readline library headers can have old-style typedefs like:
    // typedef int Function ();
    // it would clash with class Function in fityk
    //  anti-Function workaround #1: works with libreadline >= 4.2
#   define _FUNCTION_DEF 
    //  anti-Function workaround #2 (should work always), part 1
#   define Function Function_Bn4MtsgO3fQXM4Ag4z

#   include <readline/readline.h>
#   include <readline/history.h>

#   ifdef Function     
#       undef Function // anti-Function workaround #2, part 2   
#   endif

//TODO support for libedit (MacOs X, etc.)


string set_eq_str;


void read_and_execute_input()
{
    char *line = readline (prompt);
    if (!line)
        throw ExitRequestedException();
    if (line && *line)
        add_history (line);
    string s = line;
    free ((void*) line);
    AL->get_ui()->exec_and_log(s);
}


char *commands[] = { "info", "plot", "delete", "set", "fit",
        "commands", "dump", "sleep", "reset", "quit", "guess", "define"
        };

char *after_info[] = { "variables", "types", "functions", "datasets",
         "commands", "view", "set", "fit", "fit-history", "errors", "formula",
         "peaks", "guess", "F", "Z", "formula", "dF"
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

char *type_generator(const char *text, int state)
{
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        e.clear();
        vector<string> const tt = Function::get_all_types(); 
        for (vector<string>::const_iterator i = tt.begin(); i != tt.end(); ++i)
            if (!strncmp(i->c_str(), text, strlen(text)))
                e.push_back(*i);
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup(e[list_index].c_str());
    else
        return 0;
}

char *type_or_guess_generator(const char *text, int state)
{
    static bool give_guess = false;
    const char *guess = "guess";
    if (!state) 
        give_guess = true;
    char *r = type_generator(text, state);
    if (!r && give_guess) {
        give_guess = false;
        if (strncmp (guess, text, strlen(text)) == 0)
            return strdup(guess);
    }
    return r;
}

char *info_generator(const char *text, int state)
{
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        e.clear();
        vector<string> const tt = Function::get_all_types(); 
        for (vector<string>::const_iterator i = tt.begin(); i != tt.end(); ++i)
            if (!strncmp(i->c_str(), text, strlen(text)))
                e.push_back(*i);
        for (size_t i = 0; i < sizeof(after_info) / sizeof(char*); ++i) {
            char *name = after_info[i];
            if (strncmp (name, text, strlen(text)) == 0)
                e.push_back(name);
        }
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup(e[list_index].c_str());
    else
        return 0;
}


char *function_generator(const char *text, int state)
{
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        e.clear();
        vector<Function*> const& ff = AL->get_functions(); 
        for (vector<Function*>::const_iterator i=ff.begin(); i != ff.end(); ++i)
            if (!strncmp ((*i)->xname.c_str(), text, strlen(text)))
                e.push_back((*i)->xname);
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup(e[list_index].c_str());
    else
        return 0;
}

char *variable_generator(const char *text, int state)
{
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        e.clear();
        vector<Variable*> const& vv = AL->get_variables(); 
        for (vector<Variable*>::const_iterator i=vv.begin(); i != vv.end(); ++i)
            if (!strncmp ((*i)->name.c_str(), text, strlen(text)))
                e.push_back((*i)->name);
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup(e[list_index].c_str());
    else
        return 0;
}

char *set_generator(const char *text, int state)
{
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        e = AL->get_settings()->expanp(text);
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup(e[list_index].c_str());
    else
        return 0;
}

char *set_eq_generator (const char *text, int state)
{
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        e = AL->get_settings()->expand_enum(set_eq_str, text);
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup(e[list_index].c_str());
    else
        return 0;
}

char **my_completion (const char *text, int start, int end)
{
    rl_attempted_completion_over = 1;
    //find start of the command, and skip blanks
    int cmd_start = start;
    while (cmd_start > 0 && rl_line_buffer[cmd_start-1] != ';')
        --cmd_start;
    while (isspace(rl_line_buffer[cmd_start])) 
        ++cmd_start;
    //command
    if (cmd_start == start)
        return rl_completion_matches(text, command_generator);
    char *ptr = rl_line_buffer+cmd_start;
    //check if it is after set command or after with
    if (cmd_start <= start-2 && !strncmp(ptr, "s ", 2)
            || cmd_start <= start-3 && !strncmp(ptr, "se ", 3) 
            || cmd_start <= start-4 && !strncmp(ptr, "set ", 4)
            || cmd_start <= start-2 && !strncmp(ptr, "w ", 2)
            || cmd_start <= start-3 && !strncmp(ptr, "wi ", 3) 
            || cmd_start <= start-4 && !strncmp(ptr, "wit ", 4)
            || cmd_start <= start-5 && !strncmp(ptr, "with ", 5)) {
        while (*ptr && !isspace(*ptr))
            ++ptr;
        ++ptr;
        char *has_eq = 0;
        for (char *i = ptr; i <= rl_line_buffer+end; ++i) {
            if (*i == '=')
                has_eq = i;
            else if (*i == ',') {
                ptr = i+1;
                has_eq = 0;
            }
        }
        if (!has_eq)
            return rl_completion_matches(text, set_generator);
        else {
            set_eq_str = strip_string(string(ptr, has_eq));
            return rl_completion_matches (text, set_eq_generator);
        }
    }
    // FunctionType completion
    if (cmd_start <= start-2 && !strncmp(ptr, "g ", 2)
            || cmd_start <= start-3 && !strncmp(ptr, "gu ", 3) 
            || cmd_start <= start-4 && !strncmp(ptr, "gue ", 4) 
            || cmd_start <= start-5 && !strncmp(ptr, "gues ", 5) 
            || cmd_start <= start-6 && !strncmp(ptr, "guess ", 6)) {
        return rl_completion_matches(text, type_generator);
    }
    // FunctionType or "guess" completion
    if (cmd_start <= start-3 && rl_line_buffer[cmd_start] == '%'
               && strchr(rl_line_buffer+cmd_start, '=')
               && !strchr(rl_line_buffer+cmd_start, '(')) {
        return rl_completion_matches(text, type_or_guess_generator);
    }

    // %function completion
    if (strlen(text) > 0 && text[0] == '%')
        return rl_completion_matches(text, function_generator);
    // $variable completion
    if (start > 0 && rl_line_buffer[start-1] == '$')
        return rl_completion_matches(text, variable_generator);

    // info completion
    if (cmd_start <= start-2 && (!strncmp(ptr, "i ", 2) 
                                 || !strncmp(ptr, "i+", 2))
            || cmd_start <= start-3 && (!strncmp(ptr, "in ", 3) 
                                        || !strncmp(ptr, "in+", 3))
            || cmd_start <= start-4 && (!strncmp(ptr, "inf ", 4) 
                                        || !strncmp(ptr, "inf+", 4))
            || cmd_start <= start-5 && (!strncmp(ptr, "info ", 5)
                                        || !strncmp(ptr, "info+", 5))) {
        return rl_completion_matches(text, info_generator);
    }

    ptr = rl_line_buffer + start - 1;
    while (ptr > rl_line_buffer && isspace(*ptr)) 
        --ptr;
    if (*ptr == '>' || *ptr == '<') { //filename completion
        rl_attempted_completion_over = 0; 
        return 0;
    }

    return 0;
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
        AL->get_ui()->exec_and_log(s);
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

} // anonymous namespace

int main (int argc, char **argv)
{
    setlocale(LC_NUMERIC, "C");
    // setting ^C handler
    if (signal (SIGINT, interrupt_handler) == SIG_IGN) 
        signal (SIGINT, SIG_IGN);

    // process command-line arguments
    bool exec_init_file = true;
    bool quit = false;
    string script_string;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            cout << 
              "Usage: cfityk [-h] [-V] [-c <str>] [script or data file...]\n"
              "  -h, --help            show this help message\n"
              "  -V, --version         output version information and exit\n"
              "  -c, --cmd=<str>       script passed in as string\n"
              "  -I, --no-init         don't process $HOME/.fityk/init file\n"
              "  -q, --quit            don't enter interactive shell\n";
            return 0;
        }
        else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
            cout << "fityk version " VERSION "\n";
            return 0;
        }
        else if (startswith(argv[i], "-c") || startswith(argv[i], "--cmd")) {
            if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--cmd")) {
                argv[i] = 0;
                ++i;
                if (i < argc) {
                    script_string = argv[i];
                    argv[i] = 0;
                }
                else {
                    cerr << "Option " << argv[i] << " requires parameter\n";
                    return 1;
                }
            }
            else if (startswith(argv[i], "-c")) {
                script_string = string(argv[i] + 2);
                argv[i] = 0;
            }
            else if (startswith(argv[i], "--cmd")) {
                if (argv[i][5] != '=') {
                    cerr << "Unknown option: " << argv[i] << "\n";
                    return 1;
                }
                script_string = string(argv[i] + 6);
                argv[i] = 0;
            }
            else
                assert(0);
        }
        else if (!strcmp(argv[i], "-I") || !strcmp(argv[i], "--no-init")) {
            argv[i] = 0;
            exec_init_file = false;
        }
        else if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "--quit")) {
            argv[i] = 0;
            quit = true;
        }
    }

    AL = new Fityk;

    // set callbacks
    AL->get_ui()->set_show_message(cli_show_message);
    AL->get_ui()->set_do_draw_plot(cli_do_draw_plot);
    AL->get_ui()->set_wait(cli_wait);

    if (exec_init_file) {
        // file with initial commands is executed first (if exists)
        string init_file = get_config_dir() + startup_commands_filename;
        if (access(init_file.c_str(), R_OK) == 0) {
            cerr << " -- reading init file: " << init_file << " --\n";
            AL->get_ui()->exec_script(init_file);
            cerr << " -- end of init file --" << endl;
        }
    }

    try {
        //then string given with -c is executed
        if (!script_string.empty())
            AL->get_ui()->exec_and_log(script_string);
        //the rest of parameters/arguments are scripts and/or data files
        for (int i = 1; i < argc; ++i) {
            if (argv[i])
                AL->get_ui()->process_cmd_line_filename(argv[i]);
        }

        // the version of main_loop() depends on NO_READLINE  
        if (!quit)
            main_loop(); 
    } 
    catch(ExitRequestedException) {
        cerr << "\nbye...\n";
    }

    return 0;
}

