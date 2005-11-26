// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$


#ifndef FITYK__UI__H__
#define FITYK__UI__H__
#include "common.h"
#include <vector>
#include <utility>
#include "dotset.h"

class wxString;
struct NumberedLine;


// return value: false -> quit
bool cmd_parser(std::string cmd); 

/// A Singleton class.
/// Some methods (plot, plotNow, wait, execCommand, showMessage) 
/// are different and defined separatly for GUI and CLI versions.
/// The program is always linked only with one version of each method.
class UserInterface : public DotSet
{
public:
    /// get Singleton class instance
    static UserInterface* getInstance();

    /// Update plot if pri<=auto_plot.   If !now, update can be delayed
    /// Different definition for GUI and CLI
    void drawPlot(int pri=0, bool now=false);

    /// sent message - to user input and to log file (if logging is on)
    void outputMessage (int level, std::string const &s);

    /// Wait and disable UI for ... seconds. Different for GUI and CLI.
    void wait(float seconds); 

    void startLog (char mode, std::string const &filename);
    void stopLog();
    std::string getLogInfo() const;
    char getLogMode() const { return log_mode; };
    std::string getLogFilename() const { return log_filename; };

    /// Excute all commands (or these from specified lines) from file. 
    /// In other words, run a script (.fit).
    void execScript (std::string const &filename, 
                     std::vector<std::pair<int,int> > const &selected_lines);
    void execScript (std::string const &filename) 
    { execScript(filename, std::vector<std::pair<int,int> >()); }

    void execAndLogCmd(std::string const &s) {log_input(s); execCommand(s);}

    bool displayHelpTopic(std::string const &topic); 
    int getVerbosity() { return verbosity; }
    /// exit UI and program
    void close();

private:
    UserInterface();
    UserInterface (UserInterface const&); //disable
    UserInterface& operator= (UserInterface const&); //disable

    void doDrawPlot(bool now=false);
    /// show message to user; different definition for GUI and CLI
    void showMessage (OutputStyle style, std::string const& s);

    /// Execute command(s) from string; different definition for GUI and CLI.
    /// It can finish the program (eg. if s=="quit").
    void execCommand (std::string const &s);

    void log_output (std::string const &s);
    void log_input (std::string const &s) 
           { if (log_mode=='i' || log_mode=='a')  logfile << " " << s << "\n"; }

    static UserInterface* instance;
    char log_mode; //i, a, o, n  //TODO: change to enum 
    std::string log_filename;
    std::ofstream logfile;
    char verbosity;
    bool exit_on_warning;
    char auto_plot;
    std::map<char, std::string> autoplot_enum;
    std::map<char, std::string> verbosity_enum;
};


    
extern const char* startup_commands_filename;
extern const char* config_dirname;

inline UserInterface* getUI() { return UserInterface::getInstance(); }

/// execute command(s) from string
inline void exec_command (std::string const &s) { getUI()->execAndLogCmd(s); }


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
