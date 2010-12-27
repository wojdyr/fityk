// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$


#ifndef FITYK__UI__H__
#define FITYK__UI__H__
#include "common.h"
#include <vector>
#include <utility>
#include <fstream>
#include <string>

class wxString;
struct NumberedLine;
class Data;
class Ftk;
class Parser;
class Runner;

/// used for storing commands and logging commands to file
class Commands
{
public:
    static const int max_cmd = 4096;
    enum Status { kStatusOk, kStatusExecuteError, kStatusSyntaxError };

    struct Cmd
    {
        std::string cmd;
        Status status;

        Cmd(std::string const& c, Status s) : cmd(c), status(s) {}
        std::string str() const;
    };

    Commands() : command_counter(0) {}
    void put_command(std::string const& c, Status s);
    void put_output_message(std::string const& s) const;
    std::vector<Cmd> const& get_cmds() const { return cmds; }
    std::string get_history_summary() const;
    void start_logging(std::string const& filename, bool with_output,
                       Ftk const* F);
    void stop_logging();
    std::string get_log_file() const { return log_filename; }
    bool get_log_with_output() const { return log_with_output; }

  private:
    int command_counter; //!=cmds.size() if max_cmd was exceeded
    std::vector<Cmd> cmds;
    std::string log_filename;
    mutable std::ofstream log;
    bool log_with_output;
};

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

    /// it's used to disable all messages
    bool keep_quiet;

    UserInterface(Ftk* F);
    ~UserInterface();

    /// redraw plot
    void draw_plot(RepaintMode mode);

    /// sent message - to user input and to log file (if logging is on)
    void output_message (Style style, std::string const &s) const;

    void start_log (std::string const &filename, bool with_output)
                    { commands_.start_logging(filename, with_output, F_); }
    void stop_log() { commands_.stop_logging(); }
    Commands const& get_commands() const { return commands_; }

    /// Excute commands from file, i.e. run a script (.fit).
    void exec_script (std::string const &filename);

    void exec_stream (FILE *fp);
    void exec_string_as_script(const char* s);

    Commands::Status exec_and_log(std::string const &c);

    // Calls Parser::parse_statement() and Runner::execute_statement(),
    // catches exceptions and returns status.
    Commands::Status execute_line(const std::string& str);

    /// return true if the syntax is correct
    bool check_syntax(std::string const& str);


    void process_cmd_line_filename(std::string const& par);

    // callbacks
    typedef void t_do_draw_plot(RepaintMode mode);
    void set_do_draw_plot(t_do_draw_plot *func) { do_draw_plot_ = func; }

    typedef void t_show_message(Style style, std::string const& s);
    void set_show_message(t_show_message *func) { show_message_ = func; }

    typedef Commands::Status t_exec_command (std::string const &s);
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

private:
    Ftk* F_;
    t_show_message *show_message_;
    t_do_draw_plot *do_draw_plot_;
    t_exec_command *exec_command_;
    t_refresh *refresh_;
    t_compute_ui *compute_ui_;
    t_wait *wait_;
    Commands commands_;
    Parser *parser_;
    Runner *runner_;

    UserInterface (UserInterface const&); //disable
    UserInterface& operator= (UserInterface const&); //disable

    /// show message to user
    void show_message (Style style, std::string const& s) const
        { if (show_message_) (*show_message_)(style, s); }

    /// Execute command(s) from string
    /// It can finish the program (eg. if s=="quit").
    Commands::Status exec_command (std::string const &s);
};


extern const char* startup_commands_filename;
extern const char* config_dirname;

#endif
