// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

// CLI-only file

#include "common.h"
#include "u_gnuplot.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include "data.h"
#include "sum.h"
#include "logic.h"
#include "ui.h"

using namespace std;

char GnuPlot::path_to_gnuplot[]="gnuplot";

GnuPlot::GnuPlot()
{
    fork_and_make_pipe ();
}

GnuPlot::~GnuPlot() 
{
    fclose(gnuplot_pipe);  //it closes gnuplot
}

void GnuPlot::fork_and_make_pipe ()
{
    int     fd[2];
    pid_t   childpid;
    pipe(fd);
    if ((childpid = fork()) == -1) {
        perror("fork");
        exit(1);
    }

    if (childpid == 0) {
        // Child process closes up output side of pipe 
        close (fd[1]);
        // and input side is stdin
        dup2 (fd[0], 0);
        if (fd[0] > 2)
            close (fd[0]);
        //putenv("PAGER="); //putenv() - POSIX, not ANSI
        execlp (path_to_gnuplot, path_to_gnuplot, /*"-",*/NULL);
        // if we are here, sth went wrong
        warn("Problem encountered when trying to run `" 
                + S(path_to_gnuplot) + "'.");
        exit(0);
    }
    else {
        // Parent process closes up input side of pipe 
        close (fd[0]);
        gnuplot_pipe  = fdopen (fd[1], "w"); //fdopen() - POSIX, not ANSI
    }
}

bool GnuPlot::gnuplot_pipe_ok()
{
    static bool give_up = false;
    if (give_up) 
        return false;
    //sighandler_t and sig_t are not portable
    typedef void (*my_sighandler_type) (int);
    my_sighandler_type shp = signal (SIGPIPE, SIG_IGN);
    errno = 0;
    fprintf (gnuplot_pipe, " "); //pipe test
    fflush(gnuplot_pipe);
    if (errno == EPIPE) {
        errno = 0;
        fork_and_make_pipe();
        signal (SIGPIPE, SIG_IGN);
        fprintf (gnuplot_pipe, " "); //test again
        fflush(gnuplot_pipe);
        if (errno == EPIPE) {
            give_up = true;
            signal (SIGPIPE, shp);
            return false;
        }
    }                    
    signal (SIGPIPE, shp);
    return true;
}

int GnuPlot::plot() 
{
    // plot only active data with sum
    DataWithSum const* ds = AL->get_ds(AL->get_active_ds_position());
    Data const* data = ds->get_data();
    Sum const* sum = ds->get_sum();
    if (!gnuplot_pipe_ok())
        return -1;
    // Send commands through the pipe to gnuplot
    int i_f = data->get_lower_bound_ac (AL->view.left);
    int i_l = data->get_upper_bound_ac (AL->view.right);
    if (i_l - i_f <= 0) 
        return 0;
    string plot_string = "plot "+ AL->view.str() 
        + " \'-\' title \"data\", ";
    plot_string += " '-' title \"sum\" with line\n ";
    fprintf (gnuplot_pipe, plot_string.c_str());
    if (fflush (gnuplot_pipe) != 0)
        warn("Flushing pipe program-to-gnuplot failed.");
    bool at_least_one_point = false;
    for (int i = i_f; i < i_l; i++) {
        fp x = data->get_x(i);
        fp y = data->get_y(i);
        if (is_finite(x) && is_finite(y)) {
            fprintf(gnuplot_pipe, "%f  %f\n", x, y);
            at_least_one_point = true;
        }
    }
    if (!at_least_one_point)
        fprintf(gnuplot_pipe, "0.0  0.0\n");
    fprintf (gnuplot_pipe, "e\n");//gnuplot needs 'e' at the end of data
    for (int i = i_f; i < i_l; i++) {
        fp x = data->get_x(i);
        fp y = sum->value(x);
        if (is_finite(x) && is_finite(y))
            fprintf(gnuplot_pipe, "%f  %f\n", x, y);
    }
    fprintf(gnuplot_pipe, "e\n");
    fflush(gnuplot_pipe);
    return 0;
}

void GnuPlot::raw_command(char *command) 
{
    fprintf (gnuplot_pipe, "%s\n", command);
    fflush(gnuplot_pipe);
}


