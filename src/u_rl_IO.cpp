// This file is part of fityk program. Copyright (C) Marcin Wojdyr
#include "common.h"
RCSID ("$Id$")
            
#include "v_IO.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#ifndef NO_READLINE
#    include <readline/readline.h>
#    include <readline/history.h>
#endif
#include <ctype.h>
#include "u_gnuplot.h"
#include "other.h"

using namespace std;

static const char* prompt = "=-> ";

void readline_IO::message (const char *s)
{
    if (s && s[0] == '!')
        cout << '\a';
    cout << s << endl;
}

void readline_IO::plot_now (const vector<fp>& a)
{
    static GnuPlot my_gnuplot;
    my_gnuplot.plot (a);
}

#ifndef NO_READLINE

void initialize_readline();
static char set_kind = 0;
static const char* set_eq_str;

extern string fityk_dir; /*u_main.cpp*/

bool readline_IO::start (const char* /*parameter*/)
{
    initialize_readline();
    string hist_file;
    if (!fityk_dir.empty()) {
        hist_file = fityk_dir + "history";
        read_history (hist_file.c_str());
    }
    else 
        hist_file = string(); /*empty*/
    for (;;) {
        char *line = readline (prompt);
        if (!line)
            break;
        if (line && *line)
            add_history (line);
        string s = line;
        free ((void*) line);
        bool r = parser (s);
        if (!r)
            break;
    }
    cout << endl;
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
    return true;
}

//TODO strdup was used, changed to enable compilation with -ansi switch,
//but probably should be changed to strdup again 
char *dupstr (const char* s)
{
    char *r = (char*) malloc (strlen (s) + 1);
    if (r == 0) {
        warn("virtual memory exhausted");
        exit(-1);
    }
    strcpy (r, s);
    return (r);
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
            return dupstr(name);
    }
    //if (!state)
        //return dupstr(text); // to prevent file expansion
    return 0;
}

char *help_generator (const char * /*text*/, int /*state*/)
{
/*
    static unsigned int list_index = 0;
    static vector<string> e;
    if (!state) {
        e.clear();
        list_index = 0;
        ifstream hfile (help_filename.c_str());
        if (!hfile) {
            warn ("Can't open help file: " + help_filename);
            return 0;
        }
        string t;
        while (getline(hfile, t))
            if (t.size() > 1 && !isspace(t[0]) && t[0] != '#') {
                int counter = 0;
                const char *ptr = t.c_str();
                while (*ptr && !isspace(*ptr))
                    ptr++, counter++;
                string s(t, 0, counter);
                unsigned int n = strlen(text);
                if (s.size() >= n && !string(s, 0, n).compare (text))
                    e.push_back (s);
            }
    }
    else
        list_index++;
    if (list_index < e.size())
        return dupstr (e[list_index].c_str());
    else
        return 0;
*/  return 0;
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
        return dupstr (e[list_index].c_str());
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
        return dupstr (e[list_index].c_str());
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
            else if (start > i && !strncmp (rl_line_buffer + i, "h ", 2)
                    || start > i + 4
                    && !strncmp (rl_line_buffer + i, "help ", 5)) 
                return rl_completion_matches (text, help_generator);
            else if (is_before(end, '\'')) {
                rl_attempted_completion_over = 0;
                return 0;
            }
            else
                return 0;
        }
    return rl_completion_matches (text, command_generator);
}

void initialize_readline ()
{
    rl_readline_name = "fit";
    rl_attempted_completion_function = my_completion;
}

#else //NO_READLINE 

bool readline_IO::start (const char* /*parameter*/)
{
    string s;
    for (;;) {
        cout << prompt;
        if (!getline(cin, s) || !parser(s))
            break;
    }
    cout << endl;
    return true;
}

#endif //NO_READLINE 

