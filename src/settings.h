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

    inline int get_i(std::string const& k);
    fp get_f(std::string const& k) 
                  { assert(fpar.count(k)); return fpar.find(k)->second; }
    bool get_b(std::string const& k) 
                  { assert(bpar.count(k)); return bpar.find(k)->second; }
    char get_e(std::string const& k) 
                  { assert(epar.count(k)); return epar.find(k)->second.v; }
    std::string get_s(std::string const& k) 
                  { assert(spar.count(k)); return spar.find(k)->second; }

    bool setp (std::string const& k, std::string const& v);
    bool getp (std::string const& k);
    bool typep (std::string const& k, std::string& v) const;
    int expanp (std::string const& k, std::vector<std::string>& e) const;
    int expand_enum(std::string const& left, std::string const& k, 
                    std::vector<std::string>& r) const;
    std::string print_usage() const;
    std::string set_script() const; 
    bool getp_core (std::string const& k, std::string &v) const;

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
    bool setp_core (const std::string &k, const std::string &v);
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

