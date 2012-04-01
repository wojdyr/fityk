// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

// CLI-only file
// in this file: main loop, readline support (command expansion)
// and part of UserInterface implementation (CLI-specific)


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
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
    if (home_dir == NULL) {
#ifdef _WIN32
        home_dir = _getcwd(t, 200);
#else
        home_dir = getcwd(t, 200);
#endif
    }
    if (home_dir != NULL) {
        // '/' is assumed as path separator
        dir = S(home_dir) + "/" + config_dirname + "/";
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
    ftk->get_ui()->exec_and_log(s);
}

// assumes that array ends with NULL.
void add_c_string_array(const char **array, const char* text,
                        vector<string> &entries)
{
    for (const char** p = array; *p != NULL; ++p)
        if (strncmp(*p, text, strlen(text)) == 0)
            entries.push_back(*p);
}

void type_completions(const char *text, vector<string> &entries)
{
    v_foreach (Tplate::Ptr, i, ftk->get_tpm()->tpvec())
        if (strncmp((*i)->name.c_str(), text, strlen(text)) == 0)
            entries.push_back((*i)->name);
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

// returns false if filename completion is to be used instead
bool complete_fityk_line(const char* line_buffer, int start, int end,
                         const char *text,
                         vector<string> &entries)
{
    //find start of the command, and skip blanks
    int cmd_start = start;
    while (cmd_start > 0 && line_buffer[cmd_start-1] != ';')
        --cmd_start;
    while (isspace(line_buffer[cmd_start]))
        ++cmd_start;
    // skip "@m @n:"
    if (line_buffer[cmd_start] == '@') {
        int t = cmd_start + 1;
        while (t < start && line_buffer[t] != '.') {
            if (line_buffer[t] == ':') {
                cmd_start = t+1;
                while (isspace(line_buffer[cmd_start]))
                    ++cmd_start;
                break;
            }
            ++t;
        }
    }

    //command
    if (cmd_start == start) {
        add_c_string_array(command_list, text, entries);
        return true;
    }
    const char *ptr = line_buffer+cmd_start;

    const char* prev_nonblank = line_buffer + start - 1;
    while (prev_nonblank > line_buffer && isspace(*prev_nonblank))
        --prev_nonblank;

    if (*prev_nonblank == '>' || *prev_nonblank == '<') { //filename completion
        return false; // use filename completion
    }

    //check if it is after set command or after with
    else if (starts_with_command(ptr, start - cmd_start, "s","et")
        || starts_with_command(ptr, start - cmd_start, "w","ith")) {
        while (*ptr && !isspace(*ptr))
            ++ptr;
        ++ptr;
        const char *has_eq = NULL;
        for (const char *i = ptr; i <= line_buffer+end; ++i) {
            if (*i == '=')
                has_eq = i;
            else if (*i == ',') {
                ptr = i+1;
                has_eq = NULL;
            }
        }
        if (!has_eq)
            entries = ftk->settings_mgr()->get_key_list(text);
        else {
            string key = strip_string(string(ptr, has_eq));
            try {
                const char** allowed_values =
                        ftk->settings_mgr()->get_allowed_values(key);
                if (allowed_values != NULL)
                            add_c_string_array(allowed_values, text, entries);
            }
            catch (ExecuteError&) {} // unknown option
        }
    }
    // FunctionType completion
    else if (starts_with_command(ptr, start - cmd_start, "g","uess")) {
        type_completions(text, entries);
    }
    // FunctionType or "guess" completion
    else if (cmd_start <= start-3 && line_buffer[cmd_start] == '%'
               && strchr(line_buffer+cmd_start, '=')
               && !strchr(line_buffer+cmd_start, '(')) {
        type_completions(text, entries);
        if (strncmp("guess", text, strlen(text)) == 0)
            entries.push_back("guess");
    }

    // %function completion
    else if (text[0] == '%') {
        v_foreach (Function*, i, ftk->functions())
            if (!strncmp((*i)->name.c_str(), text+1, strlen(text+1)))
                entries.push_back("%" + (*i)->name);
    }
    // $variable completion
    else if (start > 0 && line_buffer[start-1] == '$') {
        v_foreach (Variable*, i, ftk->variables())
            if (!strncmp ((*i)->name.c_str(), text, strlen(text)))
                entries.push_back((*i)->name);
    }

    // info completion
    else if (starts_with_command(ptr, start - cmd_start, "i","nfo")) {
        // info set
        int arg_start = cmd_start;
        while (!isspace(line_buffer[arg_start]))
            ++arg_start;
        while (isspace(line_buffer[arg_start]))
            ++arg_start;
        const char* arg_ptr = line_buffer + arg_start;
        if (starts_with_command(arg_ptr, start - arg_start, "set",""))
            entries = ftk->settings_mgr()->get_key_list(text);
        else {
            type_completions(text, entries);
            add_c_string_array(info_args, text, entries);
        }
    }

    // debug completion
    else if (starts_with_command(ptr, start - cmd_start, "debug","")) {
        add_c_string_array(debug_args, text, entries);
    }

    // filename completion after exec
    else if (starts_with_command(ptr, start - cmd_start, "e","xecute")) {
        return false; // use filename completion
    }

    return true; // true = done
}

int f_start = -1;
int f_end = -1;

char *completion_generator(const char *text, int state)
{
    static size_t list_index = 0;
    static vector<string> entries;
    if (!state) {
        entries.clear();
        bool over = complete_fityk_line(rl_line_buffer, f_start, f_end, text,
                                        entries);
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

    cout << endl;
}


#else //if NO_READLINE

// the simplest version of user interface -- when readline is not available
void main_loop()
{
    string s;
    for (;;) {
        cout << prompt;
        if (!getline(cin, s))
            break;
        ftk->get_ui()->exec_and_log(s);
    }
    cout << endl;
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
                ftk->get_ui()->process_cmd_line_arg(argv[i]);
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

