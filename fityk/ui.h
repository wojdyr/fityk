// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_UI_H_
#define FITYK_UI_H_

#include "common.h"
#include "ui_api.h"

namespace fityk {
class BasicContext;
class CommandExecutor;

/// commands, messages and plot refreshing
/// it has callbacks that can be set by user interface
class FITYK_API UserInterface : public UiApi
{
public:
    static const int max_cmd = 4096;

    struct FITYK_API Cmd
    {
        std::string cmd;
        UiApi::Status status;

        Cmd(const std::string& c, UiApi::Status s) : cmd(c), status(s) {}
        std::string str() const;
    };

    UserInterface(BasicContext* ctx, CommandExecutor* ce);

    /// Redraw the plot.
    void draw_plot(RepaintMode mode, const char* filename=NULL);

    void mark_plot_dirty() { dirty_plot_ = true; }

    /// Calls the show_message(), logs the message to file if logging is on,
    /// handles option on_error=exit.
    void output_message(Style style, const std::string& s) const;

    /// Send warning
    void warn(std::string const &s) const { output_message(kWarning, s); }

    /// Send implicitely requested message
    void mesg(std::string const &s) const { output_message(kNormal, s); }


    /// Excute commands from file, i.e. run a script (.fit).
    void exec_fityk_script(const std::string& filename);

    void exec_stream(FILE *fp);
    void exec_string_as_script(const char* s);

    // logs the command and calls execute_line_via_callback()
    UiApi::Status exec_and_log(const std::string& c);

    // Calls raw_execute_line(), catches exceptions and returns status code.
    UiApi::Status execute_line(const std::string& str);

    void hint_ui(const std::string& key, const std::string& value)
          { if (hint_ui_callback_) (*hint_ui_callback_)(key, value); }

    std::string get_input_from_user(const std::string& prompt) {
        return user_input_callback_ ? (*user_input_callback_)(prompt)
                                    : std::string();
    }

    std::string ui_state_as_script() const
        { return ui_state_callback_ ? (*ui_state_callback_)() : std::string(); }

    /// wait doing nothing for given number of seconds (can be fractional).
    void wait(float seconds) const;

    const std::vector<Cmd>& cmds() const { return cmds_; }
    std::string get_history_summary() const;

private:
    BasicContext* ctx_;
    CommandExecutor* cmd_executor_;
    int cmd_count_; //!=cmds_.size() if max_cmd was exceeded
    std::vector<Cmd> cmds_;
    bool dirty_plot_;

    /// show message to user
    void show_message(Style style, const std::string& s) const
        { if (show_message_callback_) (*show_message_callback_)(style, s); }

    // It can finish the program (eg. if s=="quit").
    UiApi::Status execute_line_via_callback(const std::string& s);

    DISALLOW_COPY_AND_ASSIGN(UserInterface);
};

} // namespace fityk
#endif
