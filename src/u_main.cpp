// This file is part of fityk program. Copyright (C) Marcin Wojdyr
#include "common.h"
RCSID ("$Id$")

#include <fstream>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include "v_IO.h"
#include "other.h"

#define Interactive_IO readline_IO

using namespace std;
void sys_depen_init();

string fityk_dir;

int main (int argc, char **argv)
{
    sys_depen_init();
    MainManager main_manager;
    char *home_dir = getenv("HOME");
    if (!home_dir) home_dir = "";
    fityk_dir = S(home_dir) + "/.fityk/";
    string conf_file = fityk_dir + "fitykrc";
    ifstream file (conf_file.c_str(), ios::in);//only checking if can be opened
    if (file) {
        cerr << "Reading init file: " << conf_file << endl;
        file_I_stdout_O f_IO;
        my_IO = &f_IO;
        my_IO->start(conf_file.c_str());
        cerr << " End of init file." << endl;
    }
    else
        ;//cerr << "Init file not found: " << conf_file << endl;

    if (argc == 1) {
        Interactive_IO interact_IO;
        my_IO = &interact_IO;
        my_IO->start(0);
    }
    else
        for (int i = 1; i < argc; i++) 
            if (!strcmp(argv[i], "-")) {
                Interactive_IO interact_IO;
                my_IO = &interact_IO;
                my_IO->start(0);
            }
            else {
                file_I_stdout_O f_IO;
                my_IO = &f_IO;
                my_IO->start(argv[i]);
            }
    return 0;
}


int my_sleep (int seconds) 
{
        return sleep (seconds);
}


void interrupt_handler (int /*signum*/)
{
    user_interrupt = true;
}

void sys_depen_init() 
{
    if (signal (SIGINT, interrupt_handler) == SIG_IGN) 
        signal (SIGINT, SIG_IGN);
}

