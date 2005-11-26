// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__DOTSET__H__
#define FITYK__DOTSET__H__
#include <vector>
#include <map>
#include <string>
#include "common.h"

struct IntRange 
{
    int *v, l, u;
    IntRange() : v(0), l(0), u(0) {}
    IntRange(int *v_, int l_, int u_) : v(v_), l(l_), u(u_) {}
};

struct Enum_string
{
    std::map<char, std::string>& e;
    char* v;
    Enum_string (std::map<char, std::string>& e_, char* v_) : e(e_), v(v_) {}
};

/// it stores setting - variables of various types, 
/// such as lambda-starting-value in LMfit class
class DotSet 
{
protected:
    std::map <std::string, int*> ipar;
    std::map <std::string, fp*> fpar;
    std::map <std::string, bool*> bpar;
    std::map <std::string, IntRange> irpar;
    std::map <std::string, Enum_string> epar;
    std::map <std::string, std::string*> spar;
public:
    DotSet() : ipar(), fpar(), bpar(), irpar(), epar(), spar() {}
    bool setp (const std::string k, std::string v);
    bool getp (const std::string k);
    bool typep (const std::string k, std::string& v) const;
    int expanp (const std::string k, std::vector<std::string>& e) const;
    int expand_enum (std::string left, std::string k, 
                                            std::vector<std::string>& r) const;
    std::string print_usage(char c) const;
    std::string set_script (char c) const; 
    DotSet& operator= (const DotSet &vo);
    bool getp_core (const std::string &k, std::string &v) const;

private:
    DotSet (const DotSet&);
    bool setp_core (const std::string &k, const std::string &v);
};

#endif
