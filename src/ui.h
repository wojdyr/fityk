// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__UI__H__
#define FITYK__UI__H__

#include "common.h"
#include "ui_api.h"

class Ftk;
class Parser;
class Runner;
struct lua_State;
using fityk::UiApi;

/// commands, messages and plot refreshing
/// it has callbacks that can be set by user interface
class UserInterface : public UiApi
{
public:
    static const int max_cmd = 4096;

    struct Cmd
    {
        std::string cmd;
        UiApi::Status status;

        Cmd(const std::string& c, UiApi::Status s) : cmd(c), status(s) {}
        std::string str() const;
    };

    UserInterface(Ftk* F);
    ~UserInterface();

    /// Redraw the plot.
    void draw_plot(RepaintMode mode);

    /// Calls the show_message(), logs the message to file if logging is on,
    /// handles option on_error=exit.
    void output_message(Style style, const std::string& s) const;

    /// Send warning
    void warn(std::string const &s) const { output_message(kWarning, s); }

    /// Send implicitely requested message
    void mesg(std::string const &s) const { output_message(kNormal, s); }

    /// Excute commands from file, i.e. run a script (.fit).
    void exec_script(const std::string& filename);

    void exec_stream(FILE *fp);
    void exec_string_as_script(const char* s);

    UiApi::Status exec_and_log(const std::string& c);

    // Calls Parser::parse_statement() and Runner::execute_statement().
    void raw_execute_line(const std::string& str);

    // Calls raw_execute_line(), catches exceptions and returns status code.
    UiApi::Status execute_line(const std::string& str);

    /// return true if the syntax is correct
    bool check_syntax(const std::string& str);


    void process_cmd_line_arg(const std::string& arg);

    void hint_ui(int hint)
          { if (hint_ui_callback_) (*hint_ui_callback_)(hint); }

    std::string get_input_from_user(const std::string& prompt) {
        return user_input_callback_ ? (*user_input_callback_)(prompt)
                                    : std::string();
    }

    /// wait doing nothing for given number of seconds (can be fractional).
    void wait(float seconds) const;

    /// share parser -- it can be safely reused
    Parser* parser() const { return parser_; }

    const std::vector<Cmd>& cmds() const { return cmds_; }
    std::string get_history_summary() const;

    void close_lua();
    void exec_lua_string(const std::string& str);
    void exec_lua_script(const std::string& str);
    bool is_lua_line_incomplete(const char* str);

private:
    Ftk* F_;
    int cmd_count_; //!=cmds_.size() if max_cmd was exceeded
    std::vector<Cmd> cmds_;
    Parser *parser_;
    Runner *runner_;
    lua_State *L_;

    /// show message to user
    void show_message(Style style, const std::string& s) const
        { if (show_message_callback_) (*show_message_callback_)(style, s); }

    /// Execute command(s) from string
    /// It can finish the program (eg. if s=="quit").
    UiApi::Status exec_command(const std::string& s);

    lua_State* get_lua();
    void handle_lua_error();

    DISALLOW_COPY_AND_ASSIGN(UserInterface);
};

#endif
