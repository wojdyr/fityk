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



string Commands::Cmd::str() const
{
    switch (status) {
        case kStatusOk:           return cmd;
        case kStatusExecuteError: return cmd + " #>Runtime Error";
        case kStatusSyntaxError:  return cmd + " #>Syntax Error";
    }
    return ""; // avoid compiler warnings
}

void Commands::put_command(string const &c, Commands::Status r)
{
    if (strip_string(c).empty())
        return;
    cmds.push_back(Cmd(c, r));
    ++command_counter;
    if (!log_filename.empty())
        log << " " << c << endl;
}

void Commands::put_output_message(string const& s) const
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

string Commands::get_history_summary() const
{
    string s = S(command_counter) + " commands since the start of the program,";
    if (command_counter == size(cmds))
        s += " of which:";
    else
        s += "\nin last " + S(cmds.size()) + " commands:";
    int n_ok = 0, n_execute_error = 0, n_syntax_error = 0;
    for (vector<Cmd>::const_iterator i = cmds.begin(); i != cmds.end(); ++i)
        if (i->status == kStatusOk)
            ++n_ok;
        else if (i->status == kStatusExecuteError)
            ++n_execute_error;
        else if (i->status == kStatusSyntaxError)
            ++n_syntax_error;
    s += "\n  " + S(n_ok) + " executed successfully"
        + "\n  " + S(n_execute_error) + " finished with execute error"
        + "\n  " + S(n_syntax_error) + " with syntax error";
    if (log_filename.empty())
        s += "\nCommands are not logged to any file.";
    else
        s += S("\nCommands (") + (log_with_output ? "with" : "without")
            + " output) are logged to file: " + log_filename;
    return s;
}


void Commands::start_logging(string const& filename, bool with_output,
                             Ftk const* F)
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
        log.clear();
        log.open(filename.c_str(), ios::out | ios::app);
        if (!log)
            throw ExecuteError("Can't open file for writing: " + filename);
        log << fityk_version_line << endl;
        log << "### AT "<< time_now() << "### START LOGGING ";
        log_with_output = false; //don't put info() below into log
        if (with_output) {
            log << "INPUT AND OUTPUT";
            F->msg("Logging input and output to file: " + filename);
        }
        else {
            log << "INPUT";
            F->msg("Logging input to file: " + filename);
        }
        log << " TO THIS FILE (" << filename << ")\n";
        log_with_output = with_output;
        log_filename = filename;
    }
}

void Commands::stop_logging()
{
    log.close();
    log_filename = "";
}



UserInterface::UserInterface(Ftk* F)
        : keep_quiet(false), F_(F), show_message_(NULL), do_draw_plot_(NULL),
          exec_command_(NULL), refresh_(NULL), compute_ui_(NULL), wait_(NULL)
{
    parser_ = new Parser(F);
    runner_ = new Runner(F);
}

UserInterface::~UserInterface()
{
    delete parser_;
    delete runner_;
}

Commands::Status UserInterface::exec_and_log(string const &c)
{
    Commands::Status r = this->exec_command(c);
    commands_.put_command(c, r);
    return r;
}

Commands::Status UserInterface::execute_line(const string& str)
{
    try {
        Lexer lex(str.c_str());
        while (parser_->parse_statement(lex))
            runner_->execute_statement(parser_->statement());
    }
    catch (fityk::SyntaxError &e) {
        F_->warn(string("Syntax error: ") + e.what());
        return Commands::kStatusSyntaxError;
    }
    // ExecuteError and xylib::FormatError and xylib::RunTimeError
    // are derived from std::runtime_error
    catch (runtime_error &e) {
        F_->warn(string("Error: ") + e.what());
        return Commands::kStatusExecuteError;
    }

    if (F_->is_plot_outdated() && F_->get_settings()->autoplot)
        draw_plot(UserInterface::kRepaint);

    return Commands::kStatusOk;
}

bool UserInterface::check_syntax(string const& str)
{
    return parser_->check_syntax(str);
}

void UserInterface::output_message(Style style, const string& s) const
{
    if (keep_quiet)
        return;
    show_message(style, s);
    commands_.put_output_message(s);
    if (style == kWarning && F_->get_settings()->exit_on_warning) {
        show_message(kNormal, "Warning -> exiting program.");
        throw ExitRequestedException();
    }
}


void UserInterface::exec_script(const string& filename)
{
    user_interrupt = false;
    ifstream file(filename.c_str(), ios::in);
    if (!file) {
        F_->warn("Can't open file: " + filename);
        return;
    }

    string dir = get_directory(filename);

    int line_index = 0;
    string s;
    while (getline (file, s)) {
        ++line_index;
        if (s.empty())
            continue;
        if (F_->get_verbosity() >= 0)
            show_message (kQuoted, S(line_index) + "> " + s);
        replace_all(s, "_EXECUTED_SCRIPT_DIR_/", dir);
        execute_line(s);
        if (user_interrupt) {
            F_->msg ("Script stopped by signal INT.");
            return;
        }
    }
}

void UserInterface::exec_stream(FILE *fp)
{
    LineReader reader(fp);
    char *line;
    while ((line = reader.next()) != NULL) {
        string s = line;
        if (F_->get_verbosity() >= 0)
            show_message (kQuoted, "> " + s);
        execute_line(s);
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
            if (F_->get_verbosity() >= 0)
                show_message (kQuoted, "> " + line);
            execute_line(line);
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


Commands::Status UserInterface::exec_command(string const &s)
{
    return exec_command_ ? (*exec_command_)(s) : execute_line(s);
}

bool is_fityk_script(string filename)
{
    const char *magic = "# Fityk";

    ifstream f(filename.c_str(), ios::in | ios::binary);
    if (!f)
        return false;

    int n = filename.size();
    if ((n > 4 && string(filename, n-4) == ".fit")
            || (n > 6 && string(filename, n-6) == ".fityk"))
        return true;

    const int magic_len = strlen(magic);
    char buffer[100];
    f.read(buffer, magic_len);
    return !strncmp(magic, buffer, magic_len);
}

void UserInterface::process_cmd_line_filename(string const& par)
{
    if (startswith(par, "=->"))
        exec_and_log(string(par, 3));
    else if (is_fityk_script(par))
        exec_script(par);
    else {
        exec_and_log("@+ <'" + par + "'");
    }
}


