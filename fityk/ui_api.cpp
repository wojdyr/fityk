// This file is part of fityk program. Copyright 2012 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "ui_api.h"
#include <cstring>
#include <cstdio>
#include "cparser.h"
#include "mgr.h"
#include "logic.h"
#include "func.h"

using namespace std;

namespace {
using namespace fityk;
// assumes that array ends with NULL.
void add_c_string_array(const char **array, const char* text,
                        vector<string> &entries)
{
    for (const char** p = array; *p != NULL; ++p)
        if (strncmp(*p, text, strlen(text)) == 0)
            entries.push_back(*p);
}

void type_completions(Full *F, const char *text, vector<string> &entries)
{
    v_foreach (Tplate::Ptr, i, F->get_tpm()->tpvec())
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

} // namespace


namespace fityk {

const char* config_dirname() { return ".fityk"; }
const char* startup_commands_filename() { return "init"; }

volatile bool user_interrupt = false;

static
void simple_show_message(UiApi::Style style, const string& s)
{
    if (style == UiApi::kWarning)
        printf("\a");
    printf("%s\n", s.c_str());
    fflush(stdout);
}

static
string simple_user_input(const string& prompt)
{
    printf("%s ", prompt.c_str());
    fflush(stdout);
    char s[100];
    fgets(s, 100, stdin);
    return strip_string(s);
}

UiApi::UiApi()
    : show_message_callback_(simple_show_message),
      draw_plot_callback_(NULL),
      exec_command_callback_(NULL),
      hint_ui_callback_(NULL),
      user_input_callback_(simple_user_input)
{
}

UiApi::t_draw_plot_callback*
UiApi::connect_draw_plot(UiApi::t_draw_plot_callback *func)
{
    UiApi::t_draw_plot_callback *old = draw_plot_callback_;
    draw_plot_callback_ = func;
    return old;
}

UiApi::t_show_message_callback*
UiApi::connect_show_message(UiApi::t_show_message_callback *func)
{
    UiApi::t_show_message_callback *old = show_message_callback_;
    show_message_callback_ = func;
    return old;
}

UiApi::t_exec_command_callback*
UiApi::connect_exec_command(UiApi::t_exec_command_callback *func)
{
    UiApi::t_exec_command_callback *old = exec_command_callback_;
    exec_command_callback_ = func;
    return old;
}

UiApi::t_hint_ui_callback*
UiApi::connect_hint_ui(UiApi::t_hint_ui_callback *func)
{
    UiApi::t_hint_ui_callback *old = hint_ui_callback_;
    hint_ui_callback_ = func;
    return old;
}

UiApi::t_user_input_callback*
UiApi::connect_user_input(UiApi::t_user_input_callback *func)
{
    UiApi::t_user_input_callback *old = user_input_callback_;
    user_input_callback_ = func;
    return old;
}

vector<string> complete_fityk_line(Fityk *F,
                                   const char* line_buffer, int start, int end,
                                   const char *text)
{
    vector<string> entries;
    Full *priv = F->priv();
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
        return entries;
    }
    const char *ptr = line_buffer+cmd_start;

    const char* prev_nonblank = line_buffer + start - 1;
    while (prev_nonblank > line_buffer && isspace(*prev_nonblank))
        --prev_nonblank;

    if (*prev_nonblank == '>' || *prev_nonblank == '<')
        // special value that requests rl_filename_completion_function
        entries.push_back("");

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
            entries = priv->settings_mgr()->get_key_list(text);
        else {
            string key = strip_string(string(ptr, has_eq));
            try {
                const char** allowed_values =
                        priv->settings_mgr()->get_allowed_values(key);
                if (allowed_values != NULL)
                            add_c_string_array(allowed_values, text, entries);
            }
            catch (ExecuteError&) {} // unknown option
        }
    }
    // FunctionType completion
    else if (starts_with_command(ptr, start - cmd_start, "g","uess")) {
        type_completions(priv, text, entries);
    }
    // FunctionType or "guess" completion
    else if (cmd_start <= start-3 && line_buffer[cmd_start] == '%'
               && strchr(line_buffer+cmd_start, '=')
               && !strchr(line_buffer+cmd_start, '(')) {
        type_completions(priv, text, entries);
        if (strncmp("guess", text, strlen(text)) == 0)
            entries.push_back("guess");
    }

    // %function completion
    else if (text[0] == '%') {
        v_foreach (Function*, i, priv->mgr.functions())
            if (!strncmp((*i)->name.c_str(), text+1, strlen(text+1)))
                entries.push_back("%" + (*i)->name);
    }
    // $variable completion
    else if (start > 0 && line_buffer[start-1] == '$') {
        v_foreach (Variable*, i, priv->mgr.variables())
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
            entries = priv->settings_mgr()->get_key_list(text);
        else {
            type_completions(priv, text, entries);
            add_c_string_array(info_args, text, entries);
        }
    }

    // debug completion
    else if (starts_with_command(ptr, start - cmd_start, "debug","")) {
        add_c_string_array(debug_args, text, entries);
    }

    // filename completion after exec
    else if (starts_with_command(ptr, start - cmd_start, "e","xecute"))
        // special value that requests rl_filename_completion_function
        entries.push_back("");

    return entries; // true = done
}

} // namespace fityk
