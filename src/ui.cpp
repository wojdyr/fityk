// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ui.h"
#include "settings.h"
#include "logic.h"
#include "data.h"
#include "cparser.h"
#include "runner.h"

using namespace std;

const char* config_dirname = ".fityk";
const char* startup_commands_filename = "init";


// utils for reading FILE
class LineReader
{
public:
    LineReader(FILE *fp) : fp_(fp), len_(160), buf_((char*) malloc(len_)) {}
    ~LineReader() { free(buf_); }
    char *next();
private:
    FILE *fp_;
    size_t len_;
    char* buf_;
};

// simple replacement for GNU getline() (returns int, not ssize_t)
int our_getline(char **lineptr, size_t *n, FILE *stream)
{
    int c;
    int counter = 0;
    while ((c = getc (stream)) != EOF && c != '\n') {
        if (counter >= (int) *n - 1) {
            *n = 2 * (*n) + 80;
            *lineptr = (char *) realloc (*lineptr, *n); // let's hope it worked
        }
        (*lineptr)[counter] = c;
        ++counter;
    }
    (*lineptr)[counter] = '\0';
    return c == EOF ? -1 : counter;
}

char* LineReader::next()
{
#if HAVE_GETLINE
    int n = getline(&buf_, &len_, fp_);
#else
    // if GNU getline() is not available, use very simple replacement
    int n = our_getline(&buf_, &len_, fp_);
#endif
    // we don't need '\n' at all
    if (n > 0 && buf_[n-1] == '\n')
        buf_[n-1] = '\0';
    return n == -1 ? NULL : buf_;
}



string UserInterface::Cmd::str() const
{
    switch (status) {
        case kStatusOk:           return cmd;
        case kStatusExecuteError: return cmd + " #>Runtime Error";
        case kStatusSyntaxError:  return cmd + " #>Syntax Error";
    }
    return ""; // avoid compiler warnings
}

string UserInterface::get_history_summary() const
{
    string s = S(cmd_count_) + " commands since the start of the program,";
    if (cmd_count_ == size(cmds_))
        s += " of which:";
    else
        s += "\nin last " + S(cmds_.size()) + " commands:";
    int n_ok = 0, n_execute_error = 0, n_syntax_error = 0;
    for (vector<Cmd>::const_iterator i = cmds_.begin(); i != cmds_.end(); ++i)
        if (i->status == kStatusOk)
            ++n_ok;
        else if (i->status == kStatusExecuteError)
            ++n_execute_error;
        else if (i->status == kStatusSyntaxError)
            ++n_syntax_error;
    s += "\n  " + S(n_ok) + " executed successfully"
        + "\n  " + S(n_execute_error) + " finished with execute error"
        + "\n  " + S(n_syntax_error) + " with syntax error";
    return s;
}



UserInterface::UserInterface(Ftk* F)
        : F_(F), show_message_(NULL), do_draw_plot_(NULL),
          exec_command_(NULL), refresh_(NULL), compute_ui_(NULL), wait_(NULL),
          cmd_count_(0)
{
    parser_ = new Parser(F);
    runner_ = new Runner(F);
}

UserInterface::~UserInterface()
{
    delete parser_;
    delete runner_;
}

UserInterface::Status UserInterface::exec_and_log(const string& c)
{
    if (strip_string(c).empty())
        return UserInterface::kStatusOk;

    // we want to log the input before the output
    if (!F_->get_settings()->logfile.empty()) {
        FILE* f = fopen(F_->get_settings()->logfile.c_str(), "a");
        if (f) {
            fprintf(f, "%s\n", c.c_str());
            fclose(f);
        }
    }

    UserInterface::Status r = this->exec_command(c);
    cmds_.push_back(Cmd(c, r));
    ++cmd_count_;
    return r;
}

void UserInterface::raw_execute_line(const string& str)
{
    Lexer lex(str.c_str());
    while (parser_->parse_statement(lex))
        runner_->execute_statement(parser_->statement());
}

UserInterface::Status UserInterface::execute_line(const string& str)
{
    try {
        raw_execute_line(str);
    }
    catch (fityk::SyntaxError &e) {
        F_->warn(string("Syntax error: ") + e.what());
        return UserInterface::kStatusSyntaxError;
    }
    // ExecuteError and xylib::FormatError and xylib::RunTimeError
    // are derived from std::runtime_error
    catch (runtime_error &e) {
        F_->warn(string("Error: ") + e.what());
        return UserInterface::kStatusExecuteError;
    }

    if (F_->is_plot_outdated() && F_->get_settings()->autoplot)
        draw_plot(UserInterface::kRepaint);

    return UserInterface::kStatusOk;
}

bool UserInterface::check_syntax(const string& str)
{
    return parser_->check_syntax(str);
}

void UserInterface::output_message(Style style, const string& s) const
{
    show_message(style, s);

    if (!F_->get_settings()->logfile.empty() &&
            F_->get_settings()->log_full) {
        FILE* f = fopen(F_->get_settings()->logfile.c_str(), "a");
        if (f) {
            // insert "# " at the beginning of string and before every new line
            fprintf(f, "# ");
            for (const char *p = s.c_str(); *p; p++) {
                fputc(*p, f);
                if (*p == '\n')
                    fprintf(f, "# ");
            }
            fprintf(f, "\n");
            fclose(f);
        }
    }

    if (style == kWarning && F_->get_settings()->exit_on_warning) {
        show_message(kNormal, "Warning -> exiting program.");
        throw ExitRequestedException();
    }
}


void UserInterface::exec_script(const string& filename)
{
    user_interrupt = false;
    FILE *fp = fopen(filename.c_str(), "r");
    if (!fp) {
        F_->warn("Can't open file: " + filename);
        return;
    }

    string dir = get_directory(filename);

    LineReader reader(fp);
    int line_index = 0;
    char *line;
    while ((line = reader.next()) != NULL) {
        ++line_index;
        string s = line;
        if (s.empty())
            continue;
        if (F_->get_verbosity() >= 0)
            show_message (kQuoted, S(line_index) + "> " + s);
        replace_all(s, "_EXECUTED_SCRIPT_DIR_/", dir);
        bool r = execute_line(s);
        if (r != kStatusOk)
            break;
        if (user_interrupt) {
            F_->msg ("Script stopped by signal INT.");
            break;
        }
    }
    fclose(fp);
}

void UserInterface::exec_stream(FILE *fp)
{
    LineReader reader(fp);
    char *line;
    while ((line = reader.next()) != NULL) {
        string s = line;
        if (F_->get_verbosity() >= 0)
            show_message (kQuoted, "> " + s);
        bool r = execute_line(s);
        if (r != kStatusOk)
            break;
    }
}

void UserInterface::exec_string_as_script(const char* s)
{
    const char* start = s;
    for (;;) {
        const char* end = start;
        while (*end != '\0' && *end != '\n')
            ++end;
        while (isspace(*(end-1)) && end > start)
            --end;
        if (end > start) { // skip blank lines
            string line(start, end);
            if (!F_->get_settings()->logfile.empty()) {
                FILE* f = fopen(F_->get_settings()->logfile.c_str(), "a");
                if (f) {
                    fprintf(f, "    %s\n", line.c_str());
                    fclose(f);
                }
            }
            if (F_->get_verbosity() >= 0)
                show_message(kQuoted, "> " + line);
            bool r = execute_line(line);
            if (r != kStatusOk)
                break;
        }
        if (*end == '\0')
            break;
        start = end + 1;
    }
}

void UserInterface::draw_plot(RepaintMode mode)
{
    if (do_draw_plot_)
        (*do_draw_plot_)(mode);
    F_->updated_plot();
}


UserInterface::Status UserInterface::exec_command(const string& s)
{
    return exec_command_ ? (*exec_command_)(s) : execute_line(s);
}

bool is_fityk_script(string filename)
{
    const char *magic = "# Fityk";

    FILE *f = fopen(filename.c_str(), "rb");
    if (!f)
        return false;

    int n = filename.size();
    if ((n > 4 && string(filename, n-4) == ".fit")
            || (n > 6 && string(filename, n-6) == ".fityk")) {
        fclose(f);
        return true;
    }

    const int magic_len = strlen(magic);
    char buffer[32];
    fgets(buffer, magic_len, f);
    fclose(f);
    return !strncmp(magic, buffer, magic_len);
}

void UserInterface::process_cmd_line_filename(const string& par)
{
    if (startswith(par, "=->"))
        exec_and_log(string(par, 3));
    else if (is_fityk_script(par))
        exec_script(par);
    else {
        exec_and_log("@+ <'" + par + "'");
    }
}


