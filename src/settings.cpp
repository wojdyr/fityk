// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "settings.h"
#include "ui.h"
#include <ctype.h>
#include <algorithm>

using namespace std;

//this is a part of Singleton design pattern
Settings* Settings::instance = 0;
Settings* Settings::getInstance()
{
    if (instance == 0)  // is it the first call?
        instance = new Settings; // create sole instance
    return instance; // address of sole instance
}


Settings::Settings() 
{
    // UserInterface
    std::map<char, std::string> verbosity_enum;
    verbosity_enum [0] = "silent";
    verbosity_enum [1] = "only-warnings";
    verbosity_enum [2] = "rather-quiet";
    verbosity_enum [3] = "normal";
    verbosity_enum [4] = "verbose";
    verbosity_enum [5] = "very-verbose";
    epar.insert (pair<string, EnumString> ("verbosity", 
                                           EnumString (verbosity_enum, 3)));

    std::map<char, std::string> autoplot_enum;
    autoplot_enum [1] = "never";
    autoplot_enum [2] = "on-plot-change";
    autoplot_enum [3] = "on-fit-iteration";
    epar.insert (pair<string, EnumString> ("autoplot", 
                                           EnumString (autoplot_enum, 2)));

    bpar["exit-on-warning"] = false;

    // Function
    fpar["cut-function-level"] = 0.;

    // Manipul
    fpar ["search-width"] = 1.;   
    bpar ["cancel-peak-out-of-search"] = true;
    fpar ["height-correction"] = 1.;
    fpar ["fwhm-correction"] = 1.;
}

bool Settings::getp_core (const string &k, string &v) const
{
    if (ipar.count (k)){
        v = S (ipar.find(k)->second);
        return true;
    }
    else if (fpar.count (k)){
        v = S (fpar.find(k)->second);
        return true;
    }
    else if (bpar.count (k)){
        v = S (bpar.find(k)->second);
        return true;
    }
    else if (irpar.count (k)){
        v = S (irpar.find(k)->second.v);
        return true;
    }
    else if (epar.count (k)){
        EnumString const& ens = epar.find(k)->second;
        v = ens.e.find(ens.v)->second;
        return true;
    }
    else if (spar.count (k)){
        v = spar.find(k)->second;
        return true;
    }
    else 
        return false;
}

bool Settings::setp_core (const string &k, const string &v)
{
    if (ipar.count (k)){
        int d;
        if (istringstream (v) >> d) {
            ipar[k] = d;
            return true;
        }
    }
    else if (fpar.count (k)){
        fp d;
        if (istringstream (v) >> d) {
            fpar[k] = d;
            return true;
        }
    }
    else if (bpar.count (k)){
        bool d;
        if (istringstream (v) >> d) {
            bpar[k] = d;
            return true;
        }
    }
    else if (irpar.count (k)) {
        int d = -1;
        istringstream (v) >> d;
        if (irpar[k].l <= d && d <= irpar[k].u) {
            irpar[k].v = d;
            return true;
        }
    }
    else if (epar.count (k)){
        EnumString& t = epar.find(k)->second;
        for (map<char,string>::const_iterator i = t.e.begin(); 
                                                           i != t.e.end(); i++)
            if (i->second == v) {
                    t.v = i->first;
                    return true;
            }
    }
    else if (spar.count (k)){
        spar[k] = v;
        return true;
    }
    return false;
}

bool Settings::getp (string const& k)
{
    string s;
    bool r = getp_core (k, s);
    if (!r)
        warn ("Unknown option: " +  k);
    else {
        string t;
        typep (k, t);
        info ("Option '" + k + "' (" + t + ") has value: " + s);
    }
    return r;
}

bool Settings::setp (string const& k, string const& v)
{
    string sp;
    bool r = getp_core (k, sp);
    if (!r)
        warn ("Unknown option: " +  k);
    else if (sp == v)
        info ("Option '" + k + "' already has value: " + v);
    else {
        r = setp_core (k, v);
        if (r)
            info ("Value for '" + k + "' changed from '"+ sp
                    + "' to '" + v + "'");
        else
            warn ("'" + v + "' is not valid value for '" + k + "'");
    }
    return r;
}

bool Settings::typep (string const& k, string& v) const
{
    if (ipar.count (k)){
        v = "<integer number>";
        return true;
    }
    else if (fpar.count (k)){
        v = "<floating point number>";
        return true;
    }
    else if (bpar.count (k)){
        v = "<boolean (0/1)>";
        return true;
    }
    else if (irpar.count (k)){
        int u = irpar.find(k)->second.u;
        int l = irpar.find(k)->second.l;
        if (u - l  < 1)
            assert(0);
        else if (u - l == 1)
            v = S(l) + ", " + S(u);
        else
            v = S(l) + ", ..., " + S(u);
        return true;
    }
    else if (epar.count (k)){
        v = "<enumeration (" + S(epar.find(k)->second.e.size()) + ")>";
        return true;
    }
    else if (spar.count (k)){
        v = "<string (a-zA-Z0-9+-.>";
        return true;
    }
    else 
        return false;
}

int Settings::expanp (string const& k, vector<string>& e) const
{
    int len = k.size();
    e.clear();
    for (map<string,int>::const_iterator i = ipar.begin(); i!=ipar.end();i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map<string,fp>::const_iterator i = fpar.begin(); i!=fpar.end(); i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map<string,bool>::const_iterator i = bpar.begin();i!=bpar.end();i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map<string, IntRange>::const_iterator i = irpar.begin(); 
                                                        i != irpar.end(); i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map<string, EnumString>::const_iterator i = epar.begin(); 
                                                          i != epar.end(); i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map<string, string>::const_iterator i = spar.begin(); 
                                                          i != spar.end(); i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    sort(e.begin(), e.end());
    return e.size();
}

int Settings::expand_enum(string const& left, string const& k, 
                          vector<string>& r) const
{
    r.clear();
    if (epar.count(left) == 0)
        return 0;
    map<char, string> const& es = epar.find(left)->second.e;
    for (map<char,string>::const_iterator i = es.begin(); i != es.end(); i++)
        if (!string(i->second, 0, k.size()).compare(k))
            r.push_back (i->second);
    return r.size();
}

string Settings::print_usage() const
{
    string s = "Usage: \n\tset option = value\n"
        "or, to see the current value: \n\t"
        "set option\n"
        "Available options:";
    vector<string> e;
    expanp ("", e);
    string t, v;
    for (vector<string>::const_iterator i = e.begin(); i != e.end(); i++){
        typep (*i, t);
        getp_core (*i, v);
        s += "\n " + *i + " = " + t + ", current value: " + v;
    }
    return s;
}

string Settings::set_script() const
{
    vector<string> e;
    expanp ("", e);
    string s, t, v;
    for (vector<string>::const_iterator i = e.begin(); i != e.end(); i++){
        typep (*i, t);
        getp_core (*i, v);
        s += "set " + *i + " = " + (v.empty() ? "\"\"" : v) 
            + " # " + t + "\n";
    }
    return s;
}


