// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef gnuplot__H__
#define gnuplot__H__
#include "common.h"
#include <stdio.h>
#include <vector>

class GnuPlot
{
    public:
        GnuPlot();
        ~GnuPlot();
        int plot (const std::vector<fp>& workingA);
        void raw_command(char *command);// no syntax checking
        static char path_to_gnuplot[] ; 
    private:
        FILE *gnuplot_pipe;

        void fork_and_make_pipe ();
};

int dmplot_export(char *filename);

#endif 
