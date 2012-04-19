#!/usr/bin/env python
"""
Equivalent of cfityk in Python (work in progress).
"""

import os
import sys
import readline
import atexit
import signal
from optparse import OptionParser
import fityk

_PROMPT = "=-> "
_PROMPT2 = "... "

def interrupt_handler(signum, frame):
    sys.stderr.write("\n(^C interrupts long calculations, use ^D to exit)\n")
    fityk.cvar.user_interrupt = True;

def read_line():
    try:
        line = raw_input(_PROMPT)
    except EOFError:
        raise fityk.ExitRequestedException()
    while line.endswith("\\"):
        cont = raw_input(_PROMPT2)
        line = line[:-1] + cont
    return line


class Completer:
    def __init__(self, ftk):
        self.ftk = ftk
        self.clist = None
    def complete(self, text, state):
        if state == 0:
            self.clist = fityk.StringVector()
            over = fityk.complete_fityk_line(self.ftk,
                            readline.get_line_buffer(),
                            readline.get_begidx(), readline.get_endidx(),
                            text, self.clist)
            #rl_attempted_completion_over = int(over)
        if state < len(self.clist):
            return self.clist[state]
        else:
            return None


def main():
    config_dir = os.path.join(os.path.expanduser("~"),
                             fityk.cvar.config_dirname)

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

    # f.get_ui_api().connect_draw_plot(.....)

    if options.version:
        print("cfityk.py %s" % f.get_info("version").split()[-1])
        return

    if not options.no_init:
        init_file = os.path.join(config_dir,
                                 fityk.cvar.startup_commands_filename)
        if os.path.exists(init_file):
            sys.stderr.write(" -- init file: %s --\n" % init_file)
            f.get_ui_api().exec_script(init_file)
            sys.stderr.write(" -- end of init file --\n")

    readline.parse_and_bind("tab: complete")
    readline.set_completer_delims(" \t\n\"\\'`@$><=;|&{(:") # default+":"
    completer = Completer(f)
    readline.set_completer(completer.complete)

    try:
        for s in options.cmd:
            f.get_ui_api().exec_and_log(s)
        for arg in args:
            f.get_ui_api().process_cmd_line_arg(arg)

        if not options.quit:
            while True:
                line = read_line()
                f.get_ui_api().exec_and_log(line)
            print("")
    except fityk.ExitRequestedException:
        sys.stderr.write("\nbye...\n")


if __name__ == '__main__':
    main()
