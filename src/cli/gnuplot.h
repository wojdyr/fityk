// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

#ifndef FITYK__GNUPLOT__H__
#define FITYK__GNUPLOT__H__
#include <stdio.h>
#include <vector>
#include "../common.h"

class GnuPlot
{
public:
    GnuPlot();
    ~GnuPlot();
    int plot();
    static char path_to_gnuplot[] ;

private:
    FILE *gnuplot_pipe;

    void fork_and_make_pipe ();
    bool gnuplot_pipe_ok();
};

#endif
