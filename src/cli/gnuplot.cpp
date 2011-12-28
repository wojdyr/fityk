// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

// CLI-only file

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#include "gnuplot.h"
#include "../data.h"
#include "../model.h"
#include "../logic.h"
#include "../ui.h"

#define GNUPLOT_PATH "gnuplot"

using namespace std;

extern Ftk* ftk; // defined in cli/main.cpp

GnuPlot::GnuPlot()
    : failed_(false), gnuplot_pipe_(NULL)
{
}

GnuPlot::~GnuPlot()
{
    if (gnuplot_pipe_)
        fclose(gnuplot_pipe_);
}

void GnuPlot::fork_and_make_pipe()
{
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
    int fd[2];
    pipe(fd);
    pid_t childpid = fork();
    if (childpid == -1) {
        perror("fork");
        exit(1);
    }

    if (childpid == 0) {
        // Child process closes up output side of pipe
        close(fd[1]);
        // and input side is stdin
        dup2(fd[0], 0);
        if (fd[0] > 2)
            close(fd[0]);
        //putenv("PAGER=");
        execlp(GNUPLOT_PATH, GNUPLOT_PATH, /*"-",*/ NULL);
        // if we are here, sth went wrong
        ftk->msg("** Calling `" GNUPLOT_PATH "' failed. Plotting disabled. **");
        exit(0); // terminate only the child process 
    }
    else {
        // Parent process closes up input side of pipe
        close(fd[0]);
        gnuplot_pipe_ = fdopen(fd[1], "w"); //fdopen() - POSIX, not ANSI
    }
#endif //!_WIN32
}

bool GnuPlot::test_gnuplot_pipe()
{
#ifdef _WIN32
    return false;
#else //!_WIN32
    errno = 0;
    fprintf(gnuplot_pipe_, " "); //pipe test
    fflush(gnuplot_pipe_);
    if (errno != 0) // errno == EPIPE if the pipe doesn't work
        failed_ = false;
    return errno == 0;
#endif //_WIN32
}

int GnuPlot::plot()
{
    // plot only the active dataset and model
    int dm_number = ftk->default_dm();
    const DataAndModel* dm = ftk->get_dm(dm_number);
    const Data* data = dm->data();
    const Model* model = dm->model();
    int i_f = data->get_lower_bound_ac(ftk->view.left());
    int i_l = data->get_upper_bound_ac(ftk->view.right());
    bool no_points = (i_l - i_f <= 0);

    // if the pipe is open and there are no points, we send empty dataset
    // to reset the plot
    if (gnuplot_pipe_ == NULL && no_points)
        return 0;

    // prepare a pipe
    if (gnuplot_pipe_ == NULL)
        fork_and_make_pipe();
    if (gnuplot_pipe_ == NULL || failed_ || !test_gnuplot_pipe())
        return -1;

    // send "plot ..." through the pipe to gnuplot
    string plot_string = "plot "+ ftk->view.str()
        + " \'-\' title \"data\", '-' title \"sum\" with line\n";
    fprintf(gnuplot_pipe_, "%s", plot_string.c_str());
    if (fflush(gnuplot_pipe_) != 0)
        ftk->warn("Flushing pipe program-to-gnuplot failed.");

    // data
    if (no_points)
        fprintf(gnuplot_pipe_, "0.0  0.0\n");
    else
        for (int i = i_f; i < i_l; i++) {
            double x = data->get_x(i);
            double y = data->get_y(i);
            if (is_finite(x) && is_finite(y)) {
                fprintf(gnuplot_pipe_, "%f  %f\n", x, y);
            }
        }
    fprintf(gnuplot_pipe_, "e\n");//gnuplot needs 'e' at the end of data

    // model
    if (no_points)
        fprintf(gnuplot_pipe_, "0.0  0.0\n");
    else
        for (int i = i_f; i < i_l; i++) {
            double x = data->get_x(i);
            double y = model->value(x);
            if (is_finite(x) && is_finite(y))
                fprintf(gnuplot_pipe_, "%f  %f\n", x, y);
        }
    fprintf(gnuplot_pipe_, "e\n");

    fflush(gnuplot_pipe_);
    return 0;
}

