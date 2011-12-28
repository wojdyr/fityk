// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

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
#ifdef _WIN32
# include <windows.h>
# include <direct.h> // _getcwd()
#else
# include <unistd.h>
# include <signal.h>
#endif
// readline header will be included later, unless NO_READLINE is defined

#include "../common.h"
#include "../ui.h"
#include "../logic.h"
#include "../settings.h"
#include "../func.h"
#include "../cparser.h" // command_list, info_args, debug_args
#include "gnuplot.h"

using namespace std;

Ftk* ftk = 0;

//------ UserInterface - implementation of CLI specific methods ------

void cli_show_message (UserInterface::Style style, const string& s)
{
    if (style == UserInterface::kWarning)
        cout << '\a';
    cout << s << endl;
}

void cli_do_draw_plot (UserInterface::RepaintMode /*mode*/)
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
    if (!home_dir) {
#ifdef _WIN32
        _getcwd(t, 200);
#else
        getcwd(t, 200);
#endif
        home_dir = t;
    }
    // '/' is assumed as path separator
    dir = S(home_dir) + "/" + config_dirname + "/";
    if (access(dir.c_str(), X_OK) != 0)
        dir = "";
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

// libedit (MacOs X, etc.) is not supported


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
    ftk->get_ui()->exec_and_log(s);
}


char *command_generator (const char *text, int state)
{
    static const char** p = NULL;
    if (!state)
        p = command_list;
    while (*p != NULL) {
        const char *name = *p;
        ++p;
        if (strncmp (name, text, strlen(text)) == 0)
            return strdup(name);
    }
    return NULL;
}

char *type_generator(const char *text, int state)
{
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        e.clear();
        v_foreach (Tplate::Ptr, i, ftk->get_tpm()->tpvec())
            if (!strncmp((*i)->name.c_str(), text, strlen(text)))
                e.push_back((*i)->name);
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup(e[list_index].c_str());
    else
        return NULL;
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
        v_foreach (Tplate::Ptr, i, ftk->get_tpm()->tpvec())
            if (!strncmp((*i)->name.c_str(), text, strlen(text)))
                e.push_back((*i)->name);
        for (const char** a = info_args; *a != NULL; ++a)
            if (strncmp (*a, text, strlen(text)) == 0)
                e.push_back(*a);
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup(e[list_index].c_str());
    else
        return NULL;
}

char *debug_generator (const char *text, int state)
{
    static const char** p = NULL;
    if (!state)
        p = debug_args;
    while (*p != NULL) {
        const char *name = *p;
        ++p;
        if (strncmp (name, text, strlen(text)) == 0)
            return strdup(name);
    }
    return NULL;
}


char *function_generator(const char *text, int state)
{
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        e.clear();
        v_foreach (Function*, i, ftk->functions())
            if (!strncmp((*i)->name.c_str(), text+1, strlen(text+1)))
                e.push_back("%" + (*i)->name);
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup(e[list_index].c_str());
    else
        return NULL;
}

char *variable_generator(const char *text, int state)
{
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        e.clear();
        v_foreach (Variable*, i, ftk->variables())
            if (!strncmp ((*i)->name.c_str(), text, strlen(text)))
                e.push_back((*i)->name);
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup(e[list_index].c_str());
    else
        return NULL;
}

char *set_generator(const char *text, int state)
{
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        e = ftk->settings_mgr()->get_key_list(text);
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup(e[list_index].c_str());
    else
        return NULL;
}

char *set_eq_generator(const char *text, int state)
{
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        try {
            e.clear();
            const char** a=ftk->settings_mgr()->get_allowed_values(set_eq_str);
            while (a != NULL && *a != NULL) {
                if (startswith(*a, text))
                    e.push_back(*a);
                ++a;
            }
        } catch (ExecuteError&) {
            return NULL;
        }
        list_index = 0;
    }
    else
        list_index++;
    if (list_index < e.size())
        return strdup(e[list_index].c_str());
    else
        return NULL;
}

bool starts_with_command(const char *cmd, int n,
                         const char* head, const char* tail)
{
    int hlen = strlen(head);
    if (strncmp(head, cmd, hlen) != 0)
        return false;
    for (int i = 0; hlen + i < n; ++i)
        if (isspace(cmd[hlen+i]))
            return i == 0 || strncmp(cmd+hlen, tail, i) == 0;
    return false;
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
    // skip "@m @n:"
    if (rl_line_buffer[cmd_start] == '@') {
        int t = cmd_start + 1;
        while (t < start && rl_line_buffer[t] != '.') {
            if (rl_line_buffer[t] == ':') {
                cmd_start = t+1;
                while (isspace(rl_line_buffer[cmd_start]))
                    ++cmd_start;
                break;
            }
            ++t;
        }
    }

    //command
    if (cmd_start == start)
        return rl_completion_matches(text, command_generator);
    char *ptr = rl_line_buffer+cmd_start;

    char* prev_nonblank = rl_line_buffer + start - 1;
    while (prev_nonblank > rl_line_buffer && isspace(*prev_nonblank))
        --prev_nonblank;
    if (*prev_nonblank == '>' || *prev_nonblank == '<') { //filename completion
        rl_attempted_completion_over = 0;
        return NULL;
    }

    //check if it is after set command or after with
    if (starts_with_command(ptr, start - cmd_start, "s","et")
        || starts_with_command(ptr, start - cmd_start, "w","ith")) {
        while (*ptr && !isspace(*ptr))
            ++ptr;
        ++ptr;
        char *has_eq = NULL;
        for (char *i = ptr; i <= rl_line_buffer+end; ++i) {
            if (*i == '=')
                has_eq = i;
            else if (*i == ',') {
                ptr = i+1;
                has_eq = NULL;
            }
        }
        if (!has_eq)
            return rl_completion_matches(text, set_generator);
        else {
            set_eq_str = strip_string(string(ptr, has_eq));
            if (ftk->settings_mgr()->get_allowed_values(set_eq_str) != NULL)
                return rl_completion_matches (text, set_eq_generator);
            else
                return NULL;
        }
    }
    // FunctionType completion
    if (starts_with_command(ptr, start - cmd_start, "g","uess")) {
        return rl_completion_matches(text, type_generator);
    }
    // FunctionType or "guess" completion
    if (cmd_start <= start-3 && rl_line_buffer[cmd_start] == '%'
               && strchr(rl_line_buffer+cmd_start, '=')
               && !strchr(rl_line_buffer+cmd_start, '(')) {
        return rl_completion_matches(text, type_or_guess_generator);
    }

    // %function completion
    if (text[0] == '%')
        return rl_completion_matches(text, function_generator);
    // $variable completion
    if (start > 0 && rl_line_buffer[start-1] == '$')
        return rl_completion_matches(text, variable_generator);

    // info completion
    if (starts_with_command(ptr, start - cmd_start, "i","nfo")) {
        // info set
        int arg_start = cmd_start;
        while (!isspace(rl_line_buffer[arg_start]))
            ++arg_start;
        while (isspace(rl_line_buffer[arg_start]))
            ++arg_start;
        char* arg_ptr = rl_line_buffer + arg_start;
        if (starts_with_command(arg_ptr, start - arg_start, "set",""))
            return rl_completion_matches(text, set_generator);

        return rl_completion_matches(text, info_generator);
    }

    // debug completion
    if (starts_with_command(ptr, start - cmd_start, "debug","")) {
        return rl_completion_matches(text, debug_generator);
    }

    // filename completion after exec
    if (starts_with_command(ptr, start - cmd_start, "e","xecute")) {
        rl_attempted_completion_over = 0;
        return NULL;
    }

    return NULL;
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
    // add colon to word breaks
    rl_basic_word_break_characters = " \t\n\"\\'`@$><=;|&{(:";
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
        ftk->get_ui()->exec_and_log(s);
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
            cout <<
              "Usage: cfityk [-h] [-V] [-c <str>] [script or data file...]\n"
              "  -h, --help            show this help message\n"
              "  -V, --version         output version information and exit\n"
              "  -c, --cmd=<str>       script passed in as string\n"
              "  -I, --no-init         don't process $HOME/.fityk/init file\n"
              "  -q, --quit            don't enter interactive shell\n";
            return 0;
        }
        else if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version")) {
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

    ftk = new Ftk;

    // set callbacks
    ftk->get_ui()->set_show_message(cli_show_message);
    ftk->get_ui()->set_do_draw_plot(cli_do_draw_plot);
    ftk->get_ui()->set_wait(cli_wait);

    if (exec_init_file) {
        // file with initial commands is executed first (if exists)
        string init_file = get_config_dir() + startup_commands_filename;
        if (access(init_file.c_str(), R_OK) == 0) {
            cerr << " -- reading init file: " << init_file << " --\n";
            ftk->get_ui()->exec_script(init_file);
            cerr << " -- end of init file --" << endl;
        }
    }

    try {
        //then string given with -c is executed
        if (!script_string.empty())
            ftk->get_ui()->exec_and_log(script_string);
        //the rest of parameters/arguments are scripts and/or data files
        for (int i = 1; i < argc; ++i) {
            if (argv[i])
                ftk->get_ui()->process_cmd_line_filename(argv[i]);
        }

        // there are two versions of main_loop(), depending on NO_READLINE
        if (!quit)
            main_loop();
    }
    catch(ExitRequestedException) {
        cerr << "\nbye...\n";
    }
    catch (runtime_error const& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

