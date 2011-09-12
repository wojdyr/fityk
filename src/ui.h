// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__UI__H__
#define FITYK__UI__H__

#include "common.h"

class Ftk;
class Parser;
class Runner;

/// commands, messages and plot refreshing
/// it has callbacks that can be set by user interface
class UserInterface
{
public:
    enum RepaintMode {
        kRepaint, // repaint can be delayed
        kRepaintImmediately // repaint immediately
    };

    // 4 styles are supported by output_message()
    enum Style
    {
        kNormal,
        kWarning,
        kQuoted,
        kInput
    };

    static const int max_cmd = 4096;
    enum Status { kStatusOk, kStatusExecuteError, kStatusSyntaxError };

    struct Cmd
    {
        std::string cmd;
        Status status;

        Cmd(const std::string& c, Status s) : cmd(c), status(s) {}
        std::string str() const;
    };

    UserInterface(Ftk* F);
    ~UserInterface();

    /// Redraw the plot.
    void draw_plot(RepaintMode mode);

    /// Calls the show_message(), logs the message to file if logging is on,
    /// handles option on_error=exit.
    void output_message (Style style, const std::string& s) const;

    /// Excute commands from file, i.e. run a script (.fit).
    void exec_script(const std::string& filename);

    void exec_stream(FILE *fp);
    void exec_string_as_script(const char* s);

    UserInterface::Status exec_and_log(const std::string& c);

    // Calls Parser::parse_statement() and Runner::execute_statement().
    void raw_execute_line(const std::string& str);

    // Calls raw_execute_line(), catches exceptions and returns status code.
    UserInterface::Status execute_line(const std::string& str);

    /// return true if the syntax is correct
    bool check_syntax(const std::string& str);


    void process_cmd_line_filename(const std::string& par);

    // callbacks
    typedef void t_do_draw_plot(RepaintMode mode);
    void set_do_draw_plot(t_do_draw_plot *func) { do_draw_plot_ = func; }

    typedef void t_show_message(Style style, const std::string& s);
    void set_show_message(t_show_message *func) { show_message_ = func; }

    typedef UserInterface::Status t_exec_command(const std::string &s);
    void set_exec_command(t_exec_command *func) { exec_command_ = func; }

    typedef void t_refresh();
    void set_refresh(t_refresh *func) { refresh_ = func; }
    /// refresh the screen if needed, for use during time-consuming tasks
    void refresh() { if (refresh_) (*refresh_)(); }

    typedef void t_compute_ui(bool);
    void set_compute_ui(t_compute_ui *func) { compute_ui_ = func; }
    void enable_compute_ui(bool enable)
            { if (compute_ui_) (*compute_ui_)(enable); }

    typedef void t_wait(float seconds);
    void set_wait(t_wait *func) { wait_ = func; }
    /// Wait and disable UI for ... seconds.
    void wait(float seconds) { if (wait_) (*wait_)(seconds); }

    /// share parser -- it can be safely reused
    Parser* parser() const { return parser_; }

    const std::vector<Cmd>& cmds() const { return cmds_; }
    std::string get_history_summary() const;

private:
    Ftk* F_;
    t_show_message *show_message_;
    t_do_draw_plot *do_draw_plot_;
    t_exec_command *exec_command_;
    t_refresh *refresh_;
    t_compute_ui *compute_ui_;
    t_wait *wait_;
    int cmd_count_; //!=cmds_.size() if max_cmd was exceeded
    std::vector<Cmd> cmds_;
    Parser *parser_;
    Runner *runner_;

    /// show message to user
    void show_message(Style style, const std::string& s) const
        { if (show_message_) (*show_message_)(style, s); }

    /// Execute command(s) from string
    /// It can finish the program (eg. if s=="quit").
    UserInterface::Status exec_command(const std::string& s);

    DISALLOW_COPY_AND_ASSIGN(UserInterface);
};


extern const char* startup_commands_filename;
extern const char* config_dirname;

#endif
