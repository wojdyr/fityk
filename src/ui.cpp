// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#include <string>
#include <iostream>
#include <string.h>

#include "ui.h"
#include "settings.h"
#include "logic.h"
#include "cmd.h"
#include "data.h"

using namespace std;

const char* config_dirname = ".fityk";
const char* startup_commands_filename = "init";

string Commands::Cmd::str() const
{
    string s = cmd + " #>";
    if (status == status_ok)
        s += "OK";
    else if (status == status_execute_error)
        s += "Runtime Error";
    else //status_syntax_error
        s += "Syntax Error";
    return s;
}

void Commands::put_command(string const &c, Commands::Status r)
{
    if (strip_string(c).empty())
        return;
    cmds.push_back(Cmd(c, r));
    ++command_counter;
    if (!log_filename.empty())
        log << " " << c << endl;
}

void Commands::put_output_message(string const& s) const
{
    if (!log_filename.empty() && log_with_output) {
        // insert "# " at the beginning of string and before every new line
        log << "# ";
        for (const char *p = s.c_str(); *p; p++) {
            log << *p;
            if (*p == '\n')
                log << "# ";
        }
        log << endl;
    }
}

vector<string> Commands::get_commands(int from, int to, bool with_status) const
{
    if (from < 0)
        from += cmds.size();
    if (to < 0)
        to += cmds.size();
    vector<string> r;
    if (!cmds.empty())
        for (int i = max(from, 0); i < min(to, size(cmds)); ++i) {
            string s;
            if (with_status)
                s = cmds[i].str();
            else
                s = cmds[i].cmd;
            r.push_back(s);
        }
    return r;
}

int Commands::count_commands_with_status(Status st) const
{
    int cnt = 0;
    for (vector<Cmd>::const_iterator i = cmds.begin(); i != cmds.end(); ++i)
        if (i->status == st)
            ++cnt;
    return cnt;
}

string Commands::get_info(bool extended) const
{
    string s = S(command_counter) + " commands since the start of the program,";
    if (command_counter == size(cmds))
        s += " of which:";
    else
        s += "\nin last " + S(cmds.size()) + " commands:";
    s += "\n  " + S(count_commands_with_status(status_ok))
              + " executed successfully"
        + "\n  " + S(count_commands_with_status(status_execute_error))
          + " finished with execute error"
        + "\n  " + S(count_commands_with_status(status_syntax_error))
          + " with syntax error";
    if (log_filename.empty())
        s += "\nCommands are not logged to any file.";
    else
        s += S("\nCommands (") + (log_with_output ? "with" : "without")
            + " output) are logged to file: " + log_filename;
    if (extended) {
        // no extended info for now
    }
    return s;
}


void Commands::start_logging(string const& filename, bool with_output,
                             Ftk const* F)
{
    if (filename.empty())
       stop_logging();
    else if (filename == log_filename) {
        if (with_output != log_with_output) {
            log_with_output = with_output;
            log << "### AT "<< time_now() << "### CHANGED TO LOG "
                << (log_with_output ? "WITH" : "WITHOUT") << " OUTPUT\n";
        }
    }
    else {
        stop_logging();
        log.clear();
        log.open(filename.c_str(), ios::out | ios::app);
        if (!log)
            throw ExecuteError("Can't open file for writing: " + filename);
        log << fityk_version_line << endl;
        log << "### AT "<< time_now() << "### START LOGGING ";
        log_with_output = false; //don't put info() below into log
        if (with_output) {
            log << "INPUT AND OUTPUT";
            F->msg("Logging input and output to file: " + filename);
        }
        else {
            log << "INPUT";
            F->msg("Logging input to file: " + filename);
        }
        log << " TO THIS FILE (" << filename << ")\n";
        log_with_output = with_output;
        log_filename = filename;
    }
}

void Commands::stop_logging()
{
    log.close();
    log_filename = "";
}


Commands::Status UserInterface::exec_and_log(string const &c)
{
    Commands::Status r = this->exec_command(c);
    commands.put_command(c, r);
    return r;
}

void UserInterface::output_message (OutputStyle style, const string& s) const
{
    if (keep_quiet)
        return;
    show_message(style, s);
    commands.put_output_message(s);
    if (style == os_warn && F->get_settings()->get_b("exit-on-warning")) {
        show_message(os_normal, "Warning -> exiting program.");
        throw ExitRequestedException();
    }
}


/// Items in selected_lines are ranges (first, after-last).
/// Lines in a file are numbered from 1.
/// If selected_lines are empty all lines from file are executed.
void UserInterface::exec_script(const string& filename,
                                const vector<pair<int,int> >& selected_lines)
{
    ifstream file(filename.c_str(), ios::in);
    if (!file) {
        F->warn("Can't open file: " + filename);
        return;
    }

    string dir = get_directory(filename);

    // most common case - execute all file
    // optimized for reading large embedded datasets
    if (selected_lines.empty()) {
        int line_index = 0;
        string s;
        int dirty_data = -1;
        while (getline (file, s)) {
            ++line_index;
            if (s.empty())
                continue;
            if (F->get_verbosity() >= 0)
                show_message (os_quot, S(line_index) + "> " + s);

            // optimize reading data lines like this:
            // X[93]=23.5124, Y[93]=122, S[93]=11.0454, A[93]=1 in @0
            if (s.size() > 20 && s[0] == 'X') {
                int nx, ny, ns, na, a, nd;
                double x, y, sigma;
                if (sscanf(s.c_str(),
                           "X[%d]=%lf, Y[%d]=%lf, S[%d]=%lf, A[%d]=%d in @%d",
                           &nx, &x, &ny, &y, &ns, &sigma, &na, &a, &nd) == 9
                     && nx >= 0 && nx < size(F->get_data(nd)->points())
                     && nx == ny && nx == ns && nx == na) {
                    vector<Point>& p = F->get_data(nd)->get_mutable_points();
                    p[nx].x = x;
                    p[ny].y = y;
                    p[ns].sigma = sigma;
                    p[na].is_active = a;
                    // check if we need to sort the data
                    if ((nx > 0 && p[nx-1].x > x)
                        || (nx+1 < size(p) && x > p[nx+1].x))
                        sort(p.begin(), p.end());
                    if (dirty_data != -1 && dirty_data != nd)
                        F->get_data(dirty_data)->after_transform();
                    dirty_data = nd;
                    continue;
                }
            }
            if (dirty_data != -1) {
                F->get_data(dirty_data)->after_transform();
                dirty_data = -1;
            }
            replace_all(s, "_EXECUTED_SCRIPT_DIR_/", dir);
            parse_and_execute(s);
        }
        if (dirty_data != -1)
            F->get_data(dirty_data)->after_transform();
    }

    else { // execute specified lines from the file
        // read in all lines from the file for easier manipulations
        vector<string> lines;
        string s;
        while (getline (file, s)) {
            replace_all(s, "_EXECUTED_SCRIPT_DIR_/", dir);
            lines.push_back(s);
        }

        for (vector<pair<int,int> >::const_iterator i = selected_lines.begin();
                                            i != selected_lines.end(); i++) {
            int f = i->first;
            int t = i->second;
            if (f < 0)
                f += lines.size();
            if (t < 0)
                t += lines.size();
            // f and t are 1-based (not 0-based)
            if (f < 1)
                f = 1;
            if (t > (int) lines.size())
                t = lines.size();
            for (int j = f; j <= t; ++j) {
                if (lines[j-1].empty())
                    continue;
                if (F->get_verbosity() >= 0)
                    show_message (os_quot, S(j) + "> " + lines[j-1]);
                // result of parse_and_execute here is neglected. Errors in
                // script don't change status of command, which executes script
                parse_and_execute(lines[j-1]);
            }
        }
    }
}

void UserInterface::draw_plot (int pri, bool now)
{
    if (pri <= F->get_settings()->get_autoplot()) {
        do_draw_plot(now);
        F->updated_plot();
    }
}


Commands::Status UserInterface::exec_command (std::string const &s)
{
    return exec_command_ ? (*exec_command_)(s) : parse_and_execute(s);
}

bool is_fityk_script(string filename)
{
    const char *magic = "# Fityk";

    ifstream f(filename.c_str(), ios::in | ios::binary);
    if (!f)
        return false;

    int n = filename.size();
    if ((n > 4 && string(filename, n-4) == ".fit")
            || (n > 6 && string(filename, n-6) == ".fityk"))
        return true;

    const int magic_len = strlen(magic);
    char buffer[100];
    f.read(buffer, magic_len);
    return !strncmp(magic, buffer, magic_len);
}

void UserInterface::process_cmd_line_filename(string const& par)
{
    if (startswith(par, "=->"))
        exec_and_log(string(par, 3));
    else if (is_fityk_script(par))
        exec_script(par);
    else {
        exec_and_log("@+ <'" + par + "'");
    }
}


