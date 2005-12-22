// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__SETTINGS__H__
#define FITYK__SETTINGS__H__
#include <map>
#include "common.h"

struct IntRange 
{
    int v, l, u;
    IntRange() : v(0), l(0), u(0) {}
    IntRange(int v_, int l_, int u_) : v(v_), l(l_), u(u_) {}
};

struct EnumString
{
    std::map<char, std::string> const e;
    char v;
    EnumString (std::map<char,std::string> const& e_, char v_): e(e_), v(v_) {}
};

/// it stores all setting - variables of various types with names, 
/// such as lambda-starting-value (used in LMfit class)
/// singleton
class Settings 
{
public:
    /// get Singleton class instance
    static Settings* getInstance();

    /// get value of integer option
    inline int get_i(std::string const& k);
    /// get value of real (floating-point) option
    fp get_f(std::string const& k) 
                  { assert(fpar.count(k)); return fpar.find(k)->second; }
    /// get value of boolean option
    bool get_b(std::string const& k) 
                  { assert(bpar.count(k)); return bpar.find(k)->second; }
    /// get value of string-enumeration option
    char get_e(std::string const& k) 
                  { assert(epar.count(k)); return epar.find(k)->second.v; }
    /// get value of string option
    std::string get_s(std::string const& k) 
                  { assert(spar.count(k)); return spar.find(k)->second; }

    /// set value of option (string v is parsed according to option type)
    void setp (std::string const& k, std::string const& v);
    /// get info about option k
    std::string infop (std::string const& k);
    /// get text information about type of option k
    std::string typep(std::string const& k) const;
    /// get all option keys that start with k
    std::vector<std::string> expanp (std::string const& k = "") const;
    std::vector<std::string> 
    expand_enum(std::string const& k, std::string const& t="") const;
    std::string print_usage() const;
    std::string set_script() const; 
    /// get value of option as string
    std::string getp(std::string const& k) const;

private:
    static Settings* instance;
    std::map <std::string, int> ipar;
    std::map <std::string, fp> fpar;
    std::map <std::string, bool> bpar;
    std::map <std::string, IntRange> irpar;
    std::map <std::string, EnumString> epar;
    std::map <std::string, std::string> spar;

    Settings();
    Settings(const Settings&); //disable
    Settings& operator= (const Settings&); //disable
    void setp_core(std::string const& k, std::string const& v);
};

int Settings::get_i(std::string const& k) { 
    std::map <std::string, int>::const_iterator t = ipar.find(k);
    if (t != ipar.end())
        return t->second;
    else {
        assert(irpar.count(k)); 
        return irpar.find(k)->second.v; 
    }
}

inline Settings* getSettings() { return Settings::getInstance(); }

#endif

