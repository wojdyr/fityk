// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef v_IO__h__
#define v_IO__h__
#include "common.h"
#include <vector>

bool //return value: false -> quit
parser (std::string cmd, bool from_file = false); 

class v_IO 
{
public:
    virtual ~v_IO() {}
    virtual bool start (const char* parameter) = 0;
    virtual void message (const char *s) = 0;
    virtual void plot_now (const std::vector<fp>& a = fp_v0) = 0;
    virtual void plot () { plot_now(); }; 
};

class file_I_stdout_O : public v_IO
{
    std::vector<int> lines;
public:
    file_I_stdout_O() {}
    file_I_stdout_O (std::vector<int> li) : lines(li) {}
    bool start (const char* parameter);
    void message (const char *s);
    void plot_now (const std::vector<fp>& a);
};
    
class gui_IO : public v_IO
{
public:
    bool start (const char* /*parameter*/);
    void message (const char *s);
    void plot ();
    void plot_now (const std::vector<fp>& a);
};

class readline_IO : public v_IO
{
public:
    bool start (const char* /*parameter*/);
    void message (const char *s);
    void plot_now (const std::vector<fp>& a);
};

extern v_IO* my_IO;

#endif
