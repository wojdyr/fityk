// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

// Command-line user interface.
// In this file: main loop,callbacks for UiApi (CLI-specific)
// and readline support (command expansion).


#include <stdio.h>
#include <stdlib.h>
//#include <ctype.h>
#include <string.h>
#include <cmath>
#include <assert.h>
#ifdef _WIN32
# include <direct.h> // _getcwd()
# ifdef _MSC_VER
#  include <io.h>
#  define access _access
#  define X_OK 0
#  define R_OK 4
# endif
#else
# include <unistd.h>
# include <signal.h>
#endif

#include "fityk/fityk.h"
#include "fityk/ui_api.h"
#include "gnuplot.h"
#if HAVE_CONFIG_H
#  include <config.h> // VERSION, HAVE_LIBREADLINE, etc
#endif

#if HAVE_LIBREADLINE
# if defined(HAVE_READLINE_READLINE_H)
#  include <readline/readline.h>
# elif defined(HAVE_READLINE_H)
#  include <readline.h>
# endif
# if defined(HAVE_READLINE_HISTORY_H)
#  include <readline/history.h>
# elif defined(HAVE_HISTORY_H)
#  include <history.h>
# elif defined(HAVE_READLINE_HISTORY)
   extern "C" { extern void add_history(const char*); }
# endif
#endif // HAVE_LIBREADLINE

using namespace std;
using namespace fityk;

Fityk* ftk = 0;

//------ implementation of CLI specific methods for UiApi callbacks ------

static
void cli_draw_plot (UiApi::RepaintMode /*mode*/, const char* filename)
{
    static GnuPlot my_gnuplot;
    if (filename) {
        fprintf(stderr, "Saving plot to file is not implemented.\n");
        return;
    }
    my_gnuplot.plot();
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
        dir = string(home_dir) + "/" + config_dirname() + "/";
        if (access(dir.c_str(), X_OK) != 0)
            dir = "";
    }
    first_run = false;
    return dir;
}


#if HAVE_LIBREADLINE

void read_and_execute_input()
{
    char *line = readline(prompt);
    if (!line)
        throw ExitRequestedException();
    if (*line)
        add_history(line);
    string s = line;
    free(line);
    while (!s.empty() && *(s.end()-1) == '\\') {
        s.resize(s.size()-1);
        char *cont = readline("... ");
        s += cont;
        free(cont);
    }
    ftk->get_ui_api()->exec_and_log(s);
}

int f_start = -1;
int f_end = -1;

char *completion_generator(const char *text, int state)
{
    static size_t list_index = 0;
    static vector<string> entries;
    if (!state) {
        entries = complete_fityk_line(ftk, rl_line_buffer, f_start, f_end,
                                      text);
        list_index = 0;
    } else
        list_index++;
    rl_attempted_completion_over = 1;

    // special value - request for filename completion
    if (entries.size() == 1 && entries[0].empty())
        return rl_filename_completion_function(text, state);

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


#if defined(HAVE_READLINE_HISTORY_H) || defined(HAVE_HISTORY_H)
/// Reads history (for readline) in ctor and saves it to file in dtor.
/// Proper use: single instance created at the beginning of the program
/// and destroyed at the end.
class RlHistoryManager
{
public:
    RlHistoryManager();
    ~RlHistoryManager();
private:
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
#else
typedef int RlHistoryManager;
#endif


void main_loop()
{
    //initialize readline
    char name[] = "fit";
    rl_readline_name = name;
    char word_break_characters[] = " \t\n\"\\'`@$><=;|&{(:"; // default+":"
    rl_basic_word_break_characters = word_break_characters;
    rl_attempted_completion_function = my_completion;

    RlHistoryManager hm; // reads and saves readline history (RAII)

    // the main loop -- reading input and executing commands
    for (;;)
        read_and_execute_input();
}


#else //HAVE_LIBREADLINE

// the simplest version of user interface -- when readline is not available
void main_loop()
{
    string s;
    char line_buffer[1024];
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!fgets(line_buffer, sizeof(line_buffer), stdin))
            break;
        s = line_buffer;
        while (!s.empty() && *(s.end()-1) == '\\') {
            s.resize(s.size()-1);
            printf("... ");
            fflush(stdout);
            if (!fgets(line_buffer, sizeof(line_buffer), stdin))
                break;
            s += line_buffer;
        }
        ftk->get_ui_api()->exec_and_log(s);
    }
}

#endif //HAVE_LIBREADLINE



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
    bool enable_plot = true;
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
              "  -n, --no-plot         disable plotting (gnuplot)\n"
              "  -q, --quit            don't enter interactive shell\n");
            return 0;
        } else if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version")) {
            printf("fityk version " VERSION "\n");
            return 0;
        } else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--cmd")) {
            argv[i] = 0;
            ++i;
            if (i < argc) {
                script_string = argv[i];
                argv[i] = 0;
            } else {
                fprintf(stderr, "Option %s requires parameter\n", argv[i]);
                return 1;
            }
        } else if (!strncmp(argv[i], "-c", 2)) {
            script_string = string(argv[i] + 2);
            argv[i] = 0;
        } else if (!strncmp(argv[i], "--cmd=", 6)) {
            script_string = string(argv[i] + 6);
            argv[i] = 0;
        } else if (!strcmp(argv[i], "-I") || !strcmp(argv[i], "--no-init")) {
            argv[i] = 0;
            exec_init_file = false;
        } else if (!strcmp(argv[i], "-n") || !strcmp(argv[i], "--no-plot")) {
            argv[i] = 0;
            enable_plot = false;
        } else if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "--quit")) {
            argv[i] = 0;
            quit = true;
        } else if (strlen(argv[i]) > 1 && argv[i][0] == '-') {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            return 1;
        }
    }

    ftk = new Fityk;
    // set callbacks
    if (enable_plot)
        ftk->get_ui_api()->connect_draw_plot(cli_draw_plot);

    if (exec_init_file) {
        // file with initial commands is executed first (if exists)
        string init_file = get_config_dir() + startup_commands_filename();
        if (access(init_file.c_str(), R_OK) == 0) {
            fprintf(stderr, " -- init file: %s --\n", init_file.c_str());
            ftk->get_ui_api()->exec_fityk_script(init_file);
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
                ftk->process_cmd_line_arg(argv[i]);
        }

        // there are two versions of main_loop(), w/ and w/o libreadline
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

