// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__GNUPLOT__H__
#define FITYK__GNUPLOT__H__

#include <stdio.h>

namespace fityk { class Fityk; }
extern fityk::Fityk* ftk; // defined in cli/main.cpp

class GnuPlot
{
public:
    GnuPlot();
    ~GnuPlot();
    void plot();

private:
    bool failed_;
    FILE *gnuplot_pipe_;

    void fork_and_make_pipe();
    bool test_gnuplot_pipe();
};

#endif
