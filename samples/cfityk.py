#!/usr/bin/env python
"""
Equivalent of cfityk in Python.
"""

import os
import sys
try:
    import readline
except ImportError:
    readline = None
import atexit
import signal
from optparse import OptionParser
import subprocess
import bisect
import functools

import fityk

_PROMPT = "=-> "
_PROMPT2 = "... "

_gnuplot = None

# Python 3.2 has math.isfinite()
def finite(x):
    return x == x

def interrupt_handler(signum, frame):
    sys.stderr.write("\n(^C interrupts long calculations, use ^D to exit)\n")
    fityk.interrupt_computations()

def read_line():
    try:
        line = raw_input(_PROMPT)
    except EOFError:
        raise fityk.ExitRequestedException()
    while line.endswith("\\"):
        cont = raw_input(_PROMPT2)
        line = line[:-1] + cont
    return line

def get_filename_completions(text):
    head, tail = os.path.split(text)
    dirname = head or os.getcwd()
    try:
        return [os.path.join(head, f) for f in os.listdir(dirname)
                if f.startswith(tail)]
    except:
        return []

class Completer:
    def __init__(self, ftk):
        self.ftk = ftk
        self.clist = None
    def complete(self, text, state):
        if state == 0:
            self.clist = fityk.complete_fityk_line(self.ftk,
                                 readline.get_line_buffer(),
                                 readline.get_begidx(), readline.get_endidx(),
                                 text)
            if len(self.clist) == 1 and self.clist[0] == "":
                self.clist = get_filename_completions(text)
        if state < len(self.clist):
            return self.clist[state]
        else:
            return None


def main():
    config_dir = os.path.join(os.path.expanduser("~"), fityk.config_dirname())

    if readline:
        histfile = os.path.join(config_dir, "history")
        if hasattr(readline, "read_history_file"):
            try:
                readline.read_history_file(histfile)
            except IOError:
                pass
            atexit.register(readline.write_history_file, histfile)

    signal.signal(signal.SIGINT, interrupt_handler)

    parser = OptionParser("Usage: %prog [-h] [-V] [-c <str>]"
                          " [script or data file...]")
    parser.add_option("-V", "--version", action="store_true",
                      help="output version information and exit")
    parser.add_option("-c", "--cmd", action="append", default=[],
                      help="script passed in as string")
    parser.add_option("-I", "--no-init", action="store_true",
                      help="don't process $HOME/.fityk/init file")
    parser.add_option("-q", "--quit", action="store_true",
                      help="don't enter interactive shell")
    (options, args) = parser.parse_args()

    f = fityk.Fityk()
    ui = f.get_ui_api()

    ui.connect_draw_plot_py(functools.partial(plot_in_gnuplot, f))

    if options.version:
        print("cfityk.py %s" % f.get_info("version").split()[-1])
        return

    if not options.no_init:
        init_file = os.path.join(config_dir, fityk.startup_commands_filename())
        if os.path.exists(init_file):
            sys.stderr.write(" -- init file: %s --\n" % init_file)
            ui.exec_fityk_script(init_file)
            sys.stderr.write(" -- end of init file --\n")

    if readline:
        readline.parse_and_bind("tab: complete")
        readline.set_completer_delims(" \t\n\"\\'`@$><=;|&{(:") # default+":"
        completer = Completer(f)
        readline.set_completer(completer.complete)

    try:
        for s in options.cmd:
            ui.exec_and_log(s)
        for arg in args:
            f.process_cmd_line_arg(arg)

        if not options.quit:
            while True:
                line = read_line()
                ui.exec_and_log(line)
            print("")
    except fityk.ExitRequestedException:
        sys.stderr.write("\nbye...\n")


def plot_in_gnuplot(f, mode):
    global _gnuplot

    # _gnuplot is False -- failed to open
    # _gnuplot is None  -- not initialized
    if _gnuplot is False:
        return

    # plot only the active dataset and model
    dm_number = f.get_default_dataset()
    points = f.get_data(dm_number)
    left_x = f.get_view_boundary('L')
    right_x = f.get_view_boundary('R')
    begin = bisect.bisect_left(points, fityk.Point(left_x,0))
    end = bisect.bisect_right(points, fityk.Point(right_x,0))
    has_points = (begin != end)

    if _gnuplot is None and has_points:
        try:
            _gnuplot = subprocess.Popen(['gnuplot'], stdin=subprocess.PIPE)
        except OSError:
            _gnuplot = False
            sys.stderr.write("WARNING: Failed to open gnuplot.\n")

    if not _gnuplot:
        return

    # send "plot ..." through the pipe to gnuplot
    plot_string = ("plot %s '-' title \"data\", '-' title \"sum\" with line\n"
                    % f.get_info("view"))
    _gnuplot.stdin.write(plot_string)

    # data
    if has_points:
        for p in points[begin:end]:
            if p.is_active and finite(p.x) and finite(p.y):
                _gnuplot.stdin.write("%f  %f\n" % (p.x, p.y))
    else:
        _gnuplot.stdin.write("0.0  0.0\n")
    _gnuplot.stdin.write("e\n") # gnuplot needs 'e' at the end of data

    # model
    if has_points:
        for p in points[begin:end]:
            if p.is_active and finite(p.x):
                y = f.get_model_value(p.x, dm_number)
                if finite(y):
                    _gnuplot.stdin.write("%f  %f\n" % (p.x, y))
    else:
        _gnuplot.stdin.write("0.0  0.0\n")
    _gnuplot.stdin.write("e\n")

if __name__ == '__main__':
    main()
