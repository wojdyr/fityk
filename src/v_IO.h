// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

// v_IO 
//

#ifndef v_IO__h__
#define v_IO__h__
#include "common.h"
#include <vector>

struct NumberedLine;

// return value: false -> quit
// if flag from_file is set, input is not logged
bool parser (std::string cmd, bool from_file=false); 

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
public:
    file_I_stdout_O() {}
    file_I_stdout_O (std::vector<int> li) : lines(li) {}
    bool start (const char* parameter);
    void message (const char *s);
    void plot_now (const std::vector<fp>& a);

protected:
    std::vector<int> lines;

    bool exec_line(const NumberedLine& nl);
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
extern const char* startup_commands_filename;
extern const char* config_dirname;
void exec_commands_from_file(const char *filename);

#endif
