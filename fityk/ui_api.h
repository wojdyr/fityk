// This file is part of fityk program. Copyright 2012 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_UI_API_H_
#define FITYK_UI_API_H_

#include <string>
#include <vector>
#include "fityk.h" // FITYK_API

namespace fityk {

// this is API needed mostly to implement own user interface to libfityk.
class FITYK_API UiApi
{
public:
    /// 4 styles are supported by output_message()
    enum Style
    {
        kNormal,
        kWarning,
        kQuoted,
        kInput
    };

    /// modes supported by draw_plot()
    enum RepaintMode {
        kRepaint, // repaint can be delayed
        kRepaintImmediately // repaint immediately
    };

    /// command status
    enum Status { kStatusOk, kStatusExecuteError, kStatusSyntaxError };

    UiApi();
    virtual ~UiApi() {}

    /// in addition to executing command, log it if logging option is set
    virtual Status exec_and_log(const std::string& c) = 0;

    /// Excute commands from file, i.e. run a script (.fit).
    virtual void exec_fityk_script(const std::string& filename) = 0;

    // Callbacks. connect_*() returns old callback.

    // Callback for plotting.
    typedef void t_draw_plot_callback(RepaintMode mode, const char* filename);
    t_draw_plot_callback* connect_draw_plot(t_draw_plot_callback *func);

    // Callback for text output. Initially, a callback suitable for CLI
    // is connected (messages go to stdout).
    typedef void t_show_message_callback(Style style, const std::string& s);
    t_show_message_callback*
        connect_show_message(t_show_message_callback *func);

    // If set, this callback is used instead of execute_line(s) function.
    typedef Status t_exec_command_callback(const std::string &s);
    t_exec_command_callback*
        connect_exec_command(t_exec_command_callback *func);

    // This callback is called with arg=0 before time-consuming computation,
    // after the computation with arg=1,
    // and periodically during computations with arg=-1.
    typedef void t_hint_ui_callback(const std::string& key,
                                    const std::string& value);
    t_hint_ui_callback* connect_hint_ui(t_hint_ui_callback *func);

    // Callback for querying user.
    typedef std::string t_user_input_callback(const std::string& prompt);
    t_user_input_callback* connect_user_input(t_user_input_callback *func);

    // Callback for ui state
    typedef std::string t_ui_state_callback();
    t_ui_state_callback* connect_ui_state(t_ui_state_callback *func);

protected:
    t_show_message_callback *show_message_callback_;
    t_draw_plot_callback *draw_plot_callback_;
    t_exec_command_callback *exec_command_callback_;
    t_hint_ui_callback *hint_ui_callback_;
    t_user_input_callback *user_input_callback_;
    t_ui_state_callback *ui_state_callback_;
};

/// Helper for readline tab-completion.
/// Returns completions of the word `text' in the context of line_buffer,
/// or a single empty string if filename completion is to be used instead.
FITYK_API std::vector<std::string>
complete_fityk_line(Fityk *F, const char* line_buffer, int start, int end,
                    const char *text);

FITYK_API const char* startup_commands_filename(); // "init"
FITYK_API const char* config_dirname(); // ".fityk"
/// stops fitting after the current iteration
FITYK_API void interrupt_computations();
FITYK_API void interrupt_computations_on_sigint();

} // namespace fityk
#endif // FITYK_UI_API_H_

