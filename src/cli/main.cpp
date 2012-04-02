// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

// Command-line user interface.
// In this file: main loop,callbacks for UiApi (CLI-specific)
// and readline support (command expansion).


#include <stdio.h>
#include <stdlib.h>
//#include <ctype.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#ifdef _WIN32
# include <windows.h>
# include <direct.h> // _getcwd()
#else
# include <unistd.h>
# include <signal.h>
#endif
// readline header will be included later, unless NO_READLINE is defined

#include "../fityk.h"
#include "../ui_api.h"
#include "gnuplot.h"
#include <config.h> // VERSION

using namespace std;
using namespace fityk;

Fityk* ftk = 0;

//------ implementation of CLI specific methods for UiApi callbacks ------

void cli_show_message (UiApi::Style style, const string& s)
{
    if (style == UiApi::kWarning)
        printf("\a");
    printf("%s\n", s.c_str());
    fflush(stdout);
}

void cli_do_draw_plot (UiApi::RepaintMode /*mode*/)
{
    static GnuPlot my_gnuplot;
    my_gnuplot.plot();
}

void cli_wait(float seconds)
{
#ifdef _WIN32
    Sleep(int(seconds*1e3));
#else //!_WIN32
    seconds = fabs(seconds);
    timespec ts;
    ts.tv_sec = static_cast<int>(seconds);
    ts.tv_nsec = static_cast<int>((seconds - ts.tv_sec) * 1e9);
    nanosleep(&ts, 0);
#endif //_WIN32
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
    if (home_dir == NULL) {
#ifdef _WIN32
        home_dir = _getcwd(t, 200);
#else
        home_dir = getcwd(t, 200);
#endif
    }
    if (home_dir != NULL) {
        // '/' is assumed as path separator
        dir = string(home_dir) + "/" + config_dirname + "/";
        if (access(dir.c_str(), X_OK) != 0)
            dir = "";
    }
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

#   undef Function // anti-Function workaround #2, part 2

// libedit (MacOs X, etc.) is not supported


void read_and_execute_input()
{
    char *line = readline (prompt);
    if (!line)
        throw ExitRequestedException();
    if (line && *line)
        add_history (line);
    string s = line;
    free ((void*) line);
    ftk->get_ui_api()->exec_and_log(s);
}

int f_start = -1;
int f_end = -1;

char *completion_generator(const char *text, int state)
{
    static size_t list_index = 0;
    static vector<string> entries;
    if (!state) {
        entries.clear();
        bool over = complete_fityk_line(ftk, rl_line_buffer, f_start, f_end,
                                        text, entries);
        rl_attempted_completion_over = (int) over;
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < entries.size())
        return strdup(entries[list_index].c_str());
    else
        return NULL;
}

char **my_completion (const char *text, int start, int end)
{
    f_start = start;
    f_end = end;
    return rl_completion_matches(text, completion_generator);
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



void main_loop()
{
    //initialize readline
    rl_readline_name = "fit";
    // add colon to word breaks
    rl_basic_word_break_characters = " \t\n\"\\'`@$><=;|&{(:";
    rl_attempted_completion_function = my_completion;

    RlHistoryManager hm;//it takes care about reading/saving readline history

    //the main loop -- reading input and executing commands
    for (;;)
        read_and_execute_input();

    printf("\n");
}


#else //if NO_READLINE

// the simplest version of user interface -- when readline is not available
void main_loop()
{
    string s;
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!getline(cin, s))
            break;
        ftk->get_ui_api()->exec_and_log(s);
    }
    printf("\n");
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
#ifndef _WIN32
    // setting Ctrl-C handler
    if (signal (SIGINT, interrupt_handler) == SIG_IGN)
        signal (SIGINT, SIG_IGN);
#endif //_WIN32

    // process command-line arguments
    bool exec_init_file = true;
    bool quit = false;
    string script_string;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            printf(
              "Usage: cfityk [-h] [-V] [-c <str>] [script or data file...]\n"
              "  -h, --help            show this help message\n"
              "  -V, --version         output version information and exit\n"
              "  -c, --cmd=<str>       script passed in as string\n"
              "  -I, --no-init         don't process $HOME/.fityk/init file\n"
              "  -q, --quit            don't enter interactive shell\n");
            return 0;
        }
        else if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version")) {
            printf("fityk version " VERSION "\n");
            return 0;
        }
        else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--cmd")) {
            argv[i] = 0;
            ++i;
            if (i < argc) {
                script_string = argv[i];
                argv[i] = 0;
            }
            else {
                fprintf(stderr, "Option %s requires parameter\n", argv[i]);
                return 1;
            }
        }
        else if (!strncmp(argv[i], "-c", 2)) {
            script_string = string(argv[i] + 2);
            argv[i] = 0;
        }
        else if (!strncmp(argv[i], "--cmd=", 6)) {
            script_string = string(argv[i] + 6);
            argv[i] = 0;
        }
        else if (!strcmp(argv[i], "-I") || !strcmp(argv[i], "--no-init")) {
            argv[i] = 0;
            exec_init_file = false;
        }
        else if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "--quit")) {
            argv[i] = 0;
            quit = true;
        }
        else if (strlen(argv[i]) > 1 && argv[i][0] == '-') {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            return 1;
        }
    }

    ftk = new Fityk;

    // set callbacks
    ftk->get_ui_api()->set_show_message(cli_show_message);
    ftk->get_ui_api()->set_do_draw_plot(cli_do_draw_plot);
    ftk->get_ui_api()->set_wait(cli_wait);

    if (exec_init_file) {
        // file with initial commands is executed first (if exists)
        string init_file = get_config_dir() + startup_commands_filename;
        if (access(init_file.c_str(), R_OK) == 0) {
            fprintf(stderr, " -- init file: %s --\n", init_file.c_str());
            ftk->get_ui_api()->exec_script(init_file);
            fprintf(stderr, " -- end of init file --\n");
        }
    }

    try {
        //then string given with -c is executed
        if (!script_string.empty())
            ftk->get_ui_api()->exec_and_log(script_string);
        //the rest of parameters/arguments are scripts and/or data files
        for (int i = 1; i < argc; ++i) {
            if (argv[i])
                ftk->get_ui_api()->process_cmd_line_arg(argv[i]);
        }

        // there are two versions of main_loop(), depending on NO_READLINE
        if (!quit)
            main_loop();
    }
    catch(ExitRequestedException) {
        fprintf(stderr, "\nbye...\n");
    }
    catch (runtime_error const& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }

    return 0;
}

