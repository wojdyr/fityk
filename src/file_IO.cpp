// This file is part of fityk program. Copyright (C) Marcin Wojdyr
#include "common.h"
RCSID ("$Id$")

#include "v_IO.h"
#include <fstream>
#include <string>
#include <iostream>

using namespace std;

v_IO *my_IO;

const char* config_dirname = ".fityk";
const char* startup_commands_filename = "init";

//utility for storing lines from script file: line number and line as a string
struct NumberedLine
{
    int nr;  //1 - first line, etc.
    string txt;
    NumberedLine(int nr_, string txt_) : nr(nr_), txt(txt_) {}
};


void exec_commands_from_file(const char *filename)
{
    file_I_stdout_O f_IO;
    my_IO = &f_IO;
    my_IO->start(filename);
}


bool file_I_stdout_O::start(const char* filename)
{
    ifstream file (filename, ios::in);
    if (!file) {
        warn ("Can't open file: " + S(filename));
        return false;
    }

    vector<NumberedLine> nls, //all lines from file
                         exec_nls; //lines to execute (in order of execution)

    //fill nls for easier manipulation of file lines
    string s;
    int line_index = 1;
    while (getline (file, s)) 
        nls.push_back(NumberedLine(line_index++, s));

    if (!lines.empty())
        //TODO lines will be changed from vector<int> to vector<pair<int, int> >
        for (vector<int>::iterator i = lines.begin(); i < lines.end(); i += 2) {
            int f = max(*i, 1);  // f and t are 1-based (not 0-based)
            int t = min(*(i+1), size(nls));
            exec_nls.insert (exec_nls.end(), nls.begin()+f-1, nls.begin()+t);
        }
    else
        exec_nls = nls;

    for (vector<NumberedLine>::const_iterator i = exec_nls.begin(); 
                                                    i != exec_nls.end(); i++) {
        bool r = exec_line(*i);
        if (!r)
            break;
    }

    mesg("");
    return true;
}


//return value mean the same as parser()
bool file_I_stdout_O::exec_line(const NumberedLine& nl)
{
    if (nl.txt.length() == 0)
        return true;
    // there was a problem with Borland C++ 5.5 if nl.text ends with '\0',
    // but now only GCC is supported...
    mesg (S(nl.nr) + "> " + nl.txt); 
    return parser(nl.txt, true);    //parser(...,true) ->don't log
}
        

void file_I_stdout_O::message (const char *s)
{
        cout << s << endl;
}

void file_I_stdout_O::plot_now (const vector<fp>& /* a */) {}

