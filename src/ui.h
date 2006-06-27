// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$


#ifndef FITYK__UI__H__
#define FITYK__UI__H__
#include "common.h"
#include <vector>
#include <utility>

class wxString;
struct NumberedLine;

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
        std::string str() const { 
            return cmd + " #>" + (status == status_ok ? "OK" 
                           : (status == status_execute_error ? "Runtime Error"
                               : "Syntax Error" )); }
    };
  
    Commands() : command_counter(0) {}
    void put_command(std::string const& s, Status s);
    void put_output_message(std::string const& s); 
    std::string get_command(int n) const { assert(is_index(n, cmds));
                                           return cmds[n].cmd; }
    Status get_status(int n) const { assert(is_index(n, cmds)); 
                                     return cmds[n].status; }
    std::vector<std::string> get_commands(int from, int to, 
                                          bool with_status) const;
    std::string get_info() const;
    void start_logging(std::string const& filename, bool with_output);
    void stop_logging();
    std::string get_log_file() const { return log_filename; }
    bool get_log_with_output() const { return log_with_output; }
  
  protected:
    int command_counter; //!=cmds.size() if max_cmd was exceeded
    std::vector<Cmd> cmds;
    std::string log_filename;
    std::ofstream log;
    bool log_with_output;

    int count_commands_with_status(Status st) const;
};

/// A Singleton class.
/// Some methods (plot, plotNow, wait, execCommand, showMessage) 
/// are different and defined separatly for GUI and CLI versions.
/// The program is always linked only with one version of each method.
class UserInterface 
{
public:
    /// it's used to disable all messages 
    bool keep_quiet;
    
    /// get Singleton class instance
    static UserInterface* getInstance();

    /// Update plot if pri<=auto_plot.   If !now, update can be delayed
    /// Different definition for GUI and CLI
    void drawPlot(int pri=0, bool now=false);

    /// sent message - to user input and to log file (if logging is on)
    void outputMessage (int level, std::string const &s);

    /// Wait and disable UI for ... seconds. Different for GUI and CLI.
    void wait(float seconds); 

    void startLog (std::string const &filename, bool with_output)
                            { commands.start_logging(filename, with_output); }
    void stopLog() { commands.stop_logging(); }
    Commands const& getCommands() const { return commands; }

    /// Excute all commands (or these from specified lines) from file. 
    /// In other words, run a script (.fit).
    void execScript (std::string const &filename, 
                     std::vector<std::pair<int,int> > const &selected_lines);
    void execScript (std::string const &filename) 
    { execScript(filename, std::vector<std::pair<int,int> >()); }

    Commands::Status execAndLogCmd(std::string const &s) 
     {Commands::Status r=execCommand(s); commands.put_command(s, r); return r;}
    int getVerbosity();
    void process_cmd_line_filename(std::string const& par);

private:
    UserInterface() : keep_quiet(false) {}
    UserInterface (UserInterface const&); //disable
    UserInterface& operator= (UserInterface const&); //disable

    void doDrawPlot(bool now=false);
    /// show message to user; different definition for GUI and CLI
    void showMessage (OutputStyle style, std::string const& s);

    /// Execute command(s) from string; different definition for GUI and CLI.
    /// It can finish the program (eg. if s=="quit").
    Commands::Status execCommand (std::string const &s);

    static UserInterface* instance;
    Commands commands;
};


    
extern const char* startup_commands_filename;
extern const char* config_dirname;

inline UserInterface* getUI() { return UserInterface::getInstance(); }

/// execute command(s) from string
inline Commands::Status exec_command (std::string const &s) 
                                        { return getUI()->execAndLogCmd(s); }


// small utilities for outputing messages to active UI.
/// the smaller level - more important message 
inline void gmessage (int level, std::string const &str) 
                                    { getUI()->outputMessage(level, str); }
//
/// Send warning to UI. Message priority: 1 (lower - more important)
inline void warn(std::string const &s) { gmessage (1, s); }
/// Send message to UI. Message priority: 2 (lower - more important)
inline void mesg(std::string const &s) { gmessage(2, s); }
/// Send information to UI. Message priority: 3 (lower - more important)
inline void info(std::string const &s) { gmessage (3, s); }
/// Send verbose message to UI. Message priority: 4 (lower - more important)
inline void verbose(std::string const &s) { gmessage (4, s); }
/// Send very verbose message to UI. Message priority: 5 (the least important)
inline void very_verbose(std::string const &s) { gmessage (5, s); }
/// The same as verbose(), but string is evaluated only when needed.
#define verbose_lazy(x)   if(getUI()->getVerbosity() >= 4)  verbose((x));  

#endif 
