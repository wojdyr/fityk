// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "ui.h"
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <boost/scoped_ptr.hpp>
#ifdef _WIN32
# include <windows.h>
#else
# include <time.h>
#endif

#include "settings.h"
#include "logic.h"
#include "runner.h" // CommandExecutor

using namespace std;

namespace fityk {

// utils for reading FILE

#if !HAVE_GETLINE
// simple replacement for GNU getline() (returns int, not ssize_t)
static
int our_getline(char **lineptr, size_t *n, FILE *stream)
{
    int c;
    int counter = 0;
    while ((c = getc (stream)) != EOF) {
        if (counter >= (int) *n - 1) {
            *n = 2 * (*n) + 80;
            *lineptr = (char *) realloc (*lineptr, *n);
            if (lineptr == NULL)
                return -1;
        }
        (*lineptr)[counter] = c;
        ++counter;
        if (c == '\n')
            break;
    }
    if (counter == 0)
        return -1;
    (*lineptr)[counter] = '\0';
    return counter;
}
#endif //!HAVE_GETLINE


// the same as our_getline(), but works with gzFile instead of FILE
static
int gzipped_getline(char **lineptr, size_t *n, gzFile stream)
{
    int c;
    int counter = 0;
    while ((c = gzgetc (stream)) != EOF) {
        if (counter >= (int) *n - 1) {
            *n = 2 * (*n) + 80;
            *lineptr = (char *) realloc (*lineptr, *n);
            if (lineptr == NULL)
                return -1;
        }
        (*lineptr)[counter] = c;
        ++counter;
        if (c == '\n')
            break;
    }
    if (counter == 0)
        return -1;
    (*lineptr)[counter] = '\0';
    return counter;
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



UserInterface::UserInterface(BasicContext* ctx, CommandExecutor* ce)
        : ctx_(ctx), cmd_executor_(ce), cmd_count_(0), dirty_plot_(false)
{
}

UiApi::Status UserInterface::exec_and_log(const string& c)
{
    if (strip_string(c).empty())
        return UiApi::kStatusOk;

    // we want to log the input before the output
    if (!ctx_->get_settings()->logfile.empty()) {
        FILE* f = fopen(ctx_->get_settings()->logfile.c_str(), "a");
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

UiApi::Status UserInterface::execute_line(const string& str)
{
    UiApi::Status status = UiApi::kStatusOk;
    try {
        cmd_executor_->raw_execute_line(str);
    }
    catch (SyntaxError &e) {
        warn(string("Syntax error: ") + e.what());
        status = UiApi::kStatusSyntaxError;
    }
    // ExecuteError and xylib::FormatError and xylib::RunTimeError
    // are derived from std::runtime_error
    catch (runtime_error &e) {
        warn(string("Error: ") + e.what());
        status = UiApi::kStatusExecuteError;
    }

    if (dirty_plot_ && ctx_->get_settings()->autoplot)
        draw_plot(UiApi::kRepaint);

    return status;
}

void UserInterface::output_message(Style style, const string& s) const
{
    show_message(style, s);

    if (!ctx_->get_settings()->logfile.empty() &&
            ctx_->get_settings()->log_full) {
        FILE* f = fopen(ctx_->get_settings()->logfile.c_str(), "a");
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

    if (style == kWarning && ctx_->get_settings()->on_error[0] == 'e'/*exit*/) {
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
    NormalFileOpener() : f_(NULL) {}
    virtual ~NormalFileOpener() { if (f_) fclose(f_); }
    virtual bool open(const char* fn) { f_=fopen(fn, "r"); return f_!=NULL; }
    virtual char* read_line() { return reader.next(f_); }
private:
    FILE *f_;
};

class GzipFileOpener : public FileOpener
{
public:
    GzipFileOpener() : f_(NULL) {}
    virtual ~GzipFileOpener() { if (f_) gzclose(f_); }
    virtual bool open(const char* fn) { f_=gzopen(fn, "rb"); return f_!=NULL; }
    virtual char* read_line() { return reader.gz_next(f_); }
private:
    gzFile f_;
};


void UserInterface::exec_fityk_script(const string& filename)
{
    user_interrupt = false;

    boost::scoped_ptr<FileOpener> opener;
    if (endswith(filename, ".gz"))
        opener.reset(new GzipFileOpener);
    else
        opener.reset(new NormalFileOpener);
    if (!opener->open(filename.c_str())) {
        warn("Can't open file: " + filename);
        return;
    }

    int line_index = 0;
    char *line;
    string s;
    while ((line = opener->read_line()) != NULL) {
        ++line_index;
        if (line[0] == '\0')
            continue;
        if (ctx_->get_verbosity() >= 0)
            show_message (kQuoted, S(line_index) + "> " + line);
        s += line;
        if (*(s.end() - 1) == '\\') {
            s.resize(s.size()-1);
            continue;
        }
        if (s.find("_SCRIPT_DIR_/") != string::npos) {
            string dir = get_directory(filename);
            replace_all(s, "_EXECUTED_SCRIPT_DIR_/", dir); // old magic string
            replace_all(s, "_SCRIPT_DIR_/", dir); // new magic string
        }
        Status r = execute_line(s);
        if (r != kStatusOk &&
                ctx_->get_settings()->on_error[0] != 'n' /*nothing*/)
            break;
        if (user_interrupt) {
            mesg("Script stopped by signal INT.");
            break;
        }
        s.clear();
    }
    if (line == NULL && !s.empty())
        throw SyntaxError("unfinished line");
}


void UserInterface::exec_stream(FILE *fp)
{
    LineReader reader;
    char *line;
    string s;
    while ((line = reader.next(fp)) != NULL) {
        if (ctx_->get_verbosity() >= 0)
            show_message(kQuoted, string("> ") + line);
        s += line;
        if (*(s.end() - 1) == '\\') {
            s.resize(s.size()-1);
            continue;
        }
        Status r = execute_line(s);
        if (r != kStatusOk)
            break;
        s.clear();
    }
    if (line == NULL && !s.empty())
        throw SyntaxError("unfinished line");
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
            if (!ctx_->get_settings()->logfile.empty()) {
                FILE* f = fopen(ctx_->get_settings()->logfile.c_str(), "a");
                if (f) {
                    fprintf(f, "    %s\n", line.c_str());
                    fclose(f);
                }
            }
            if (ctx_->get_verbosity() >= 0)
                show_message(kQuoted, "> " + line);
            Status r = execute_line(line);
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
    if (draw_plot_callback_)
        (*draw_plot_callback_)(mode);
    dirty_plot_ = false;
}


UiApi::Status UserInterface::exec_command(const string& s)
{
    return exec_command_callback_ ? (*exec_command_callback_)(s)
                                  : execute_line(s);
}

void UserInterface::wait(float seconds) const
{
#ifdef _WIN32
    Sleep(iround(seconds*1e3));
#else //!_WIN32
    seconds = fabs(seconds);
    timespec ts;
    ts.tv_sec = static_cast<int>(seconds);
    ts.tv_nsec = static_cast<int>((seconds - ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
#endif //_WIN32
}

} // namespace fityk
