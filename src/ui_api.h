// This file is part of fityk program. Copyright 2012 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_UI_API_H_
#define FITYK_UI_API_H_

// this is API needed mostly to implement own user interface to libfityk.
class UiApi
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

    UiApi() : show_message_(NULL), do_draw_plot_(NULL), exec_command_(NULL),
              refresh_(NULL), compute_ui_(NULL), wait_(NULL) {}
    virtual ~UiApi() {}

    /// in addition to executing command, log it if logging option is set
    virtual Status exec_and_log(const std::string& c) = 0;

    /// Excute commands from file, i.e. run a script (.fit).
    virtual void exec_script(const std::string& filename) = 0;

    // interprets command-line argument as data or script file or as command
    virtual void process_cmd_line_arg(const std::string& arg) = 0;


    // callbacks

    typedef void t_do_draw_plot(RepaintMode mode);
    void set_do_draw_plot(t_do_draw_plot *func) { do_draw_plot_ = func; }

    typedef void t_show_message(Style style, const std::string& s);
    void set_show_message(t_show_message *func) { show_message_ = func; }

    typedef Status t_exec_command(const std::string &s);
    void set_exec_command(t_exec_command *func) { exec_command_ = func; }

    typedef void t_refresh();
    void set_refresh(t_refresh *func) { refresh_ = func; }

    typedef void t_compute_ui(bool);
    void set_compute_ui(t_compute_ui *func) { compute_ui_ = func; }

    typedef void t_wait(float seconds);
    void set_wait(t_wait *func) { wait_ = func; }

protected:
    t_show_message *show_message_;
    t_do_draw_plot *do_draw_plot_;
    t_exec_command *exec_command_;
    t_refresh *refresh_;
    t_compute_ui *compute_ui_;
    t_wait *wait_;
};

#endif // FITYK_UI_API_H_

