// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

// CLI-only file

#include "gnuplot.h"

#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <cmath>
#include <string>
#include <algorithm>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "fityk/fityk.h"
#include "fityk/common.h" // is_finite()

#define GNUPLOT_PATH "gnuplot"

using namespace std;
using fityk::Point;
using fityk::is_finite;


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
    int ret = pipe(fd);
    if (ret == -1) {
        perror("pipe");
        exit(1);
    }
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
        fprintf(stderr, "** Calling `" GNUPLOT_PATH
                        "' failed. Plotting disabled. **\n");
        abort(); // exit() messed with open files.
    } else {
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
        failed_ = true;
    return errno == 0;
#endif //_WIN32
}

void GnuPlot::plot()
{
    // plot only the active dataset and model
    int dm_number = ftk->get_default_dataset();
    const vector<Point>& points = ftk->get_data(dm_number);
    double left_x = ftk->get_view_boundary('L');
    double right_x = ftk->get_view_boundary('R');
    vector<Point>::const_iterator begin
        = lower_bound(points.begin(), points.end(), Point(left_x,0));
    vector<Point>::const_iterator end
        = upper_bound(points.begin(), points.end(), Point(right_x,0));
    bool has_points = (begin != end);

    if (gnuplot_pipe_ == NULL && has_points)
        fork_and_make_pipe();

    if (gnuplot_pipe_ == NULL || failed_ || !test_gnuplot_pipe())
        return;

    // send "plot ..." through the pipe to gnuplot
    string plot_string = "plot " + ftk->get_info("view")
        + " \'-\' title \"data\", '-' title \"sum\" with line\n";
    fprintf(gnuplot_pipe_, "%s", plot_string.c_str());
    if (fflush(gnuplot_pipe_) != 0)
        fprintf(stderr, "Flushing pipe program-to-gnuplot failed.\n");

    // data
    if (has_points) {
        for (vector<Point>::const_iterator i = begin; i != end; ++i)
            if (i->is_active && is_finite(i->x) && is_finite(i->y))
                fprintf(gnuplot_pipe_, "%f  %f\n", double(i->x), double(i->y));
    } else
        // if there are no points, we send empty dataset to reset the plot
        fprintf(gnuplot_pipe_, "0.0  0.0\n");
    fprintf(gnuplot_pipe_, "e\n");//gnuplot needs 'e' at the end of data

    // model
    if (has_points) {
        for (vector<Point>::const_iterator i = begin; i != end; ++i)
            if (i->is_active && is_finite(i->x)) {
                double y = ftk->get_model_value(i->x, dm_number);
                if (is_finite(y))
                    fprintf(gnuplot_pipe_, "%f  %f\n", double(i->x), y);
            }
    } else
        fprintf(gnuplot_pipe_, "0.0  0.0\n");
    fprintf(gnuplot_pipe_, "e\n");

    fflush(gnuplot_pipe_);
}

