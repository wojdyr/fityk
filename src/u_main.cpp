// This file is part of fityk program. Copyright (C) Marcin Wojdyr
#include "common.h"
RCSID ("$Id$")

#include <fstream>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include "ui.h"
#include "other.h"

using namespace std;
void sys_depen_init();

string fityk_dir;

bool start_loop(); //defined in u_rl_IO.cpp

int main (int argc, char **argv)
{
    sys_depen_init();
    AL = new ApplicationLogic;
    char *home_dir = getenv("HOME");
    if (!home_dir) home_dir = "";
    // '/' is assumed as path separator
    fityk_dir = S(home_dir) + "/" + config_dirname + "/"; 
    string conf_file = fityk_dir + startup_commands_filename;
    ifstream file (conf_file.c_str(), ios::in);//only checking if can be opened
    if (file) {
        cerr << " -- reading init file: " << conf_file << " --\n";
        getUI()->execScript(conf_file);
        cerr << " -- end of init file --" << endl;
    }
    else
        ;//cerr << "Init file not found: " << conf_file << endl;

    for (int i = 1; i < argc; i++) 
        getUI()->execScript(argv[i]);
    if (0 /*TODO -q option*/) 
        return 0;
    start_loop();
    return 0;
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

