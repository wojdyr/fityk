// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <boost/scoped_ptr.hpp>

#include "ui.h"
#include "settings.h"
#include "logic.h"
#include "data.h"
#include "cparser.h"
#include "runner.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "swig/luarun.h"   // the SWIG external runtime
extern int luaopen_fityk(lua_State*L); // the SWIG wrappered library
}

using namespace std;

const char* config_dirname = ".fityk";
const char* startup_commands_filename = "init";


// utils for reading FILE

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


// the same as our_getline(), but works with gzFile instead of FILE
int gzipped_getline(char **lineptr, size_t *n, gzFile stream)
{
    int c;
    int counter = 0;
    while ((c = gzgetc (stream)) != EOF && c != '\n') {
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

// buffer for reading lines from file, with minimalistic interface
class LineReader
{
public:
    LineReader() : len_(160), buf_((char*) malloc(len_)) {}
    ~LineReader() { free(buf_); }

    char* next(FILE *fp)
    {
#if HAVE_GETLINE
        return return_buf(getline(&buf_, &len_, fp));
#else
        // if GNU getline() is not available, use very simple replacement
        return return_buf(our_getline(&buf_, &len_, fp));
#endif
    }

    char* gz_next(gzFile gz_stream)
    {
        return return_buf(gzipped_getline(&buf_, &len_, gz_stream));
    }

private:
    size_t len_;
    char* buf_;

    char* return_buf(int n)
    {
        // we don't need '\n' at all
        if (n > 0 && buf_[n-1] == '\n')
            buf_[n-1] = '\0';
        return n == -1 ? NULL : buf_;
    }

};


string UserInterface::Cmd::str() const
{
    switch (status) {
        case UiApi::kStatusOk:           return cmd;
        case UiApi::kStatusExecuteError: return cmd + " #>Runtime Error";
        case UiApi::kStatusSyntaxError:  return cmd + " #>Syntax Error";
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
        if (i->status == UiApi::kStatusOk)
            ++n_ok;
        else if (i->status == UiApi::kStatusExecuteError)
            ++n_execute_error;
        else if (i->status == UiApi::kStatusSyntaxError)
            ++n_syntax_error;
    s += "\n  " + S(n_ok) + " executed successfully"
        + "\n  " + S(n_execute_error) + " finished with execute error"
        + "\n  " + S(n_syntax_error) + " with syntax error";
    return s;
}



UserInterface::UserInterface(Ftk* F)
        : F_(F),
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

UiApi::Status UserInterface::exec_and_log(const string& c)
{
    if (strip_string(c).empty())
        return UiApi::kStatusOk;

    // we want to log the input before the output
    if (!F_->get_settings()->logfile.empty()) {
        FILE* f = fopen(F_->get_settings()->logfile.c_str(), "a");
        if (f) {
            fprintf(f, "%s\n", c.c_str());
            fclose(f);
        }
    }

    UiApi::Status r = this->exec_command(c);
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

UiApi::Status UserInterface::execute_line(const string& str)
{
    UiApi::Status status = UiApi::kStatusOk;
    try {
        raw_execute_line(str);
    }
    catch (fityk::SyntaxError &e) {
        F_->warn(string("Syntax error: ") + e.what());
        status = UiApi::kStatusSyntaxError;
    }
    // ExecuteError and xylib::FormatError and xylib::RunTimeError
    // are derived from std::runtime_error
    catch (runtime_error &e) {
        F_->warn(string("Error: ") + e.what());
        status = UiApi::kStatusExecuteError;
    }

    if (F_->is_plot_outdated() && F_->get_settings()->autoplot)
        draw_plot(UiApi::kRepaint);

    return status;
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

    if (style == kWarning && F_->get_settings()->on_error[0] == 'e'/*exit*/) {
        show_message(kNormal, "Warning -> exiting program.");
        throw ExitRequestedException();
    }
}


class FileOpener
{
public:
    virtual ~FileOpener() {}
    virtual bool open(const char* fn) = 0;
    virtual char* read_line() = 0;
protected:
    LineReader reader;
};

class NormalFileOpener : public FileOpener
{
public:
    virtual ~NormalFileOpener() { if (f_) fclose(f_); }
    virtual bool open(const char* fn) { f_ = fopen(fn, "r"); return f_; }
    virtual char* read_line() { return reader.next(f_); }
private:
    FILE *f_;
};

class GzipFileOpener : public FileOpener
{
public:
    virtual ~GzipFileOpener() { if (f_) gzclose(f_); }
    virtual bool open(const char* fn) { f_ = gzopen(fn, "rb"); return f_; }
    virtual char* read_line() { return reader.gz_next(f_); }
private:
    gzFile f_;
};


void exec_lua_script(Ftk *F, const string& str, bool as_filename)
{
    lua_State *L = lua_open();
    luaL_openlibs(L);
    luaopen_fityk(L);
    swig_type_info *type_info = SWIG_TypeQuery(L, "fityk::Fityk *");
    assert(type_info != NULL);
    int owned = 1;
    fityk::Fityk *f = new fityk::Fityk(F);

    SWIG_NewPointerObj(L, f, type_info, owned);
    lua_setglobal(L, "F");

    int status;
    if (as_filename) {
        // pass filename in arg[0], like in the Lua stand-alone interpreter
        lua_createtable(L, 1, 0);
        lua_pushstring(L, str.c_str());
        lua_rawseti(L, -2, 0);
        lua_setglobal(L, "arg");

        status = luaL_dofile(L, str.c_str());
    }
    else
        status = luaL_dostring(L, str.c_str());

    if (status != 0) {
        const char *msg = lua_tostring(L, -1);
        F->warn("Lua Error:\n" + S(msg ? msg : "(non-string error)"));
    }
    lua_close(L);
}

void UserInterface::exec_script(const string& filename)
{
    user_interrupt = false;

    if (endswith(filename, ".lua")) {
        exec_lua_script(F_, filename, true);
        return;
    }

    boost::scoped_ptr<FileOpener> opener;
    if (endswith(filename, ".gz"))
        opener.reset(new GzipFileOpener);
    else
        opener.reset(new NormalFileOpener);
    if (!opener->open(filename.c_str())) {
        F_->warn("Can't open file: " + filename);
        return;
    }

    string dir = get_directory(filename);

    int line_index = 0;
    char *line;
    while ((line = opener->read_line()) != NULL) {
        ++line_index;
        string s = line;
        if (s.empty())
            continue;
        if (F_->get_verbosity() >= 0)
            show_message (kQuoted, S(line_index) + "> " + s);
        replace_all(s, "_EXECUTED_SCRIPT_DIR_/", dir);
        bool r = execute_line(s);
        if (r != kStatusOk && F_->get_settings()->on_error[0] != 'n'/*nothing*/)
            break;
        if (user_interrupt) {
            F_->msg ("Script stopped by signal INT.");
            break;
        }
    }
}


void UserInterface::exec_stream(FILE *fp)
{
    LineReader reader;
    char *line;
    while ((line = reader.next(fp)) != NULL) {
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


UiApi::Status UserInterface::exec_command(const string& s)
{
    return exec_command_ ? (*exec_command_)(s) : execute_line(s);
}

bool is_fityk_script(string filename)
{
    const char *magic = "# Fityk";

    FILE *f = fopen(filename.c_str(), "rb");
    if (!f)
        return false;

    if (endswith(filename, ".fit") || endswith(filename, ".fityk") ||
            endswith(filename, ".fit.gz") || endswith(filename, ".fityk.gz")) {
        fclose(f);
        return true;
    }

    const int magic_len = strlen(magic);
    char buffer[32];
    fgets(buffer, magic_len, f);
    fclose(f);
    return !strncmp(magic, buffer, magic_len);
}

void UserInterface::process_cmd_line_arg(const string& arg)
{
    if (startswith(arg, "=->"))
        exec_and_log(string(arg, 3));
    else if (is_fityk_script(arg) || endswith(arg, ".lua"))
        exec_script(arg);
    else {
        exec_and_log("@+ <'" + arg + "'");
    }
}

