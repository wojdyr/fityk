// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__GNUPLOT__H__
#define FITYK__GNUPLOT__H__
#include <stdio.h>
#include <vector>

class GnuPlot
{
public:
    GnuPlot();
    ~GnuPlot();
    int plot();

private:
    bool failed_;
    FILE *gnuplot_pipe_;

    void fork_and_make_pipe();
    bool test_gnuplot_pipe();
};

#endif
