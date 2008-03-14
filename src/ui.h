// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
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

/// used for storing commands and logging commands to file
class Commands 
{
public:
    static const int max_cmd = 1024;
    enum Status { status_ok, status_execute_error, status_syntax_error };

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
    std::string get_command(int n) const { assert(is_index(n, cmds));
                                           return cmds[n].cmd; }
    Status get_status(int n) const { assert(is_index(n, cmds)); 
                                     return cmds[n].status; }
    std::vector<std::string> get_commands(int from, int to, 
                                          bool with_status) const;
    std::string get_info(bool extended) const;
    void start_logging(std::string const& filename, bool with_output, 
                       Ftk const* F);
    void stop_logging();
    std::string get_log_file() const { return log_filename; }
    bool get_log_with_output() const { return log_with_output; }
  
  protected:
    int command_counter; //!=cmds.size() if max_cmd was exceeded
    std::vector<Cmd> cmds;
    std::string log_filename;
    mutable std::ofstream log;
    bool log_with_output;

    int count_commands_with_status(Status st) const;
};

/// commands, messages and plot refreshing
/// it has callbacks that can be set by user interface
class UserInterface 
{
public:
    /// it's used to disable all messages 
    bool keep_quiet;

    UserInterface(Ftk const* F_) 
        : keep_quiet(false), F(F_), m_show_message(NULL), m_do_draw_plot(NULL), 
          m_exec_command(NULL), m_refresh(NULL), m_wait(NULL) {}
    
    /// Update plot if pri<=auto_plot.   If !now, update can be delayed
    /// Different definition for GUI and CLI
    void draw_plot(int pri=0, bool now=false);

    /// sent message - to user input and to log file (if logging is on)
    void output_message (OutputStyle style, std::string const &s) const;

    void start_log (std::string const &filename, bool with_output)
                    { commands.start_logging(filename, with_output, F); }
    void stop_log() { commands.stop_logging(); }
    Commands const& get_commands() const { return commands; }

    /// Excute all commands (or these from specified lines) from file. 
    /// In other words, run a script (.fit).
    void exec_script (std::string const &filename, 
                      std::vector<std::pair<int,int> > const &selected_lines);
    void exec_script (std::string const &filename) 
        { exec_script(filename, std::vector<std::pair<int,int> >()); }

    Commands::Status exec_and_log(std::string const &c);
    int get_verbosity() const;
    void process_cmd_line_filename(std::string const& par);

    // callbacks
    typedef void t_do_draw_plot(bool now);
    void set_do_draw_plot(t_do_draw_plot *func) { m_do_draw_plot = func; }

    typedef void t_show_message(OutputStyle style, std::string const& s);
    void set_show_message(t_show_message *func) { m_show_message = func; }

    typedef Commands::Status t_exec_command (std::string const &s);
    void set_exec_command(t_exec_command *func) { m_exec_command = func; }

    typedef void t_refresh();
    void set_refresh(t_refresh *func) { m_refresh = func; }
    /// refresh the screen if needed, for use during time-consuming tasks
    void refresh() { if (m_refresh) (*m_refresh)(); } 

    typedef void t_wait(float seconds); 
    void set_wait(t_wait *func) { m_wait = func; }
    /// Wait and disable UI for ... seconds. 
    void wait(float seconds) { if (m_wait) (*m_wait)(seconds); }

private:
    Ftk const* F;
    t_show_message *m_show_message;
    t_do_draw_plot *m_do_draw_plot;
    t_exec_command *m_exec_command;
    t_refresh *m_refresh;
    t_wait *m_wait;
    Commands commands;

    UserInterface (UserInterface const&); //disable
    UserInterface& operator= (UserInterface const&); //disable

    void do_draw_plot(bool now=false) 
        { if (m_do_draw_plot) (*m_do_draw_plot)(now); }
    /// show message to user
    void show_message (OutputStyle style, std::string const& s) const
        { if (m_show_message) (*m_show_message)(style, s); }

    /// Execute command(s) from string
    /// It can finish the program (eg. if s=="quit").
    Commands::Status exec_command (std::string const &s);
};


    
extern const char* startup_commands_filename;
extern const char* config_dirname;


#endif 
