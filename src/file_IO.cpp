// This file is part of fityk program. Copyright (C) Marcin Wojdyr
#include "common.h"
RCSID ("$Id$")

#include "v_IO.h"
#include <fstream>
#include <string>
#include <iostream>

using namespace std;

v_IO *my_IO;

bool file_I_stdout_O::start (const char* filename)
{
    ifstream file (filename, ios::in);
    if (!file) {
        warn ("Can't open file: " + S(filename));
        return false;
    }
    string s;
    vector<string> all_lines, exec_lines;
    vector<int> exec_lines_nr;
    while (getline (file, s)) 
        all_lines.push_back (s);
    if (lines.empty()) {
        exec_lines = all_lines;
        exec_lines_nr.resize(all_lines.size());
        for (unsigned int i = 0; i < exec_lines.size(); i++)
            exec_lines_nr[i] = i + 1;
    }
    else
        for (vector<int>::iterator i = lines.begin(); i < lines.end(); i += 2){
            int f = *i, t = *(i+1);
            if (t > size(all_lines)) t = all_lines.size();
            if (f == 0) f = 1;
            if (f > t || f < 0)
                continue;
            exec_lines.insert (exec_lines.end(), all_lines.begin() + f - 1, 
                                                 all_lines.begin() + t);
            for (int i = f; i <= t; i++)
                exec_lines_nr.push_back(i);
        }
    assert (exec_lines_nr.size() == exec_lines.size());
    for (unsigned int i = 0; i < exec_lines.size(); i++) {
        if (exec_lines[i].length() == 0)
            continue;
        if (*(exec_lines[i].end() - 1) == 0) //there was a problem with Borland
            exec_lines[i].erase (exec_lines[i].end() - 1);//C++ 5.5; it's a fix
        string out = S(exec_lines_nr[i]) + "> " + exec_lines[i];
        mesg (out.c_str()); 
        if (parser (exec_lines[i], true) == false)//parser(...,true) ->don't log
            break;
    }
    mesg("");
    return true;
}
        

void file_I_stdout_O::message (const char *s)
{
        cout << s << endl;
}

void file_I_stdout_O::plot_now (const vector<fp>& /* a */) {}

