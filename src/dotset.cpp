// This file is part of fityk program. Copyright (C) Marcin Wojdyr
#include "common.h"
RCSID ("$Id$")


#include "dotset.h"
#include <string.h>
#include <ctype.h>
#include <algorithm>

using namespace std;

DotSet& DotSet::operator= (const DotSet &v)
{
    // pre: both DotSet's are similar, only values are different
    for (map<string, int*>::const_iterator i = v.ipar.begin(); 
                                                        i != v.ipar.end(); i++)
        *ipar[i->first] = *i->second;
    for (map<string, fp*>::const_iterator i = v.fpar.begin(); 
                                                        i != v.fpar.end(); i++)
        *fpar[i->first] = *i->second;
    for (map<string, bool*>::const_iterator i = v.bpar.begin(); 
                                                        i !=v.bpar.end(); i++)
        *bpar[i->first] = *i->second;
    for (map<string, IntRange>::const_iterator i = v.irpar.begin(); 
                                                        i != v.irpar.end(); i++)
        *irpar[i->first].v = *i->second.v;
    for (map<string, Enum_string>::const_iterator i = v.epar.begin(); 
                                                        i != v.epar.end(); i++)
        *epar.find(i->first)->second.v = *i->second.v;
    for (map<string, string*>::const_iterator i = v.spar.begin(); 
                                                        i != v.spar.end(); i++)
        *spar[i->first] = *i->second;
    return *this;
}

bool DotSet::getp_core (const string &k, string &v) const
{
    if (ipar.count (k)){
        v = S (*ipar.find(k)->second);
        return true;
    }
    else if (fpar.count (k)){
        v = S (*fpar.find(k)->second);
        return true;
    }
    else if (bpar.count (k)){
        v = S (*bpar.find(k)->second);
        return true;
    }
    else if (irpar.count (k)){
        v = S (*irpar.find(k)->second.v);
        return true;
    }
    else if (epar.count (k)){
        v = epar.find(k)->second.e[*epar.find(k)->second.v];
        return true;
    }
    else if (spar.count (k)){
        v = *spar.find(k)->second;
        return true;
    }
    else 
        return false;
}

bool DotSet::setp_core (const string &k, const string &v)
{
    if (ipar.count (k)){
        int d;
        if (istringstream (v) >> d) {
            *ipar[k] = d;
            return true;
        }
    }
    else if (fpar.count (k)){
        fp d;
        if (istringstream (v) >> d) {
            *fpar[k] = d;
            return true;
        }
    }
    else if (bpar.count (k)){
        bool d;
        if (istringstream (v) >> d) {
            *bpar[k] = d;
            return true;
        }
    }
    else if (irpar.count (k)) {
        int d = -1;
        istringstream (v) >> d;
        if (irpar[k].l <= d && d <= irpar[k].u) {
            *irpar[k].v = d;
            return true;
        }
    }
    else if (epar.count (k)){
        Enum_string& t = epar.find(k)->second;
        for (map<char, string>::iterator i = t.e.begin(); i != t.e.end(); i++)
            if (i->second == v) {
                    *t.v = i->first;
                    return true;
            }
    }
    else if (spar.count (k)){
        *spar[k] = v;
        return true;
    }
    return false;
}

bool DotSet::getp (const string k)
{
    string s;
    bool r = getp_core (k, s);
    if (!r)
        warn ("Unknown option: " +  k);
    else {
        string t;
        typep (k, t);
        mesg ("Option '" + k + "' (" + t + ") has value: " + s);
    }
    return r;
}

bool DotSet::setp (const string k, string v)
{
    string sp;
    bool r = getp_core (k, sp);
    if (v.size() > 1 && v[0] == '"' && v[v.size()-1] == '"')//remove ""
        v = string(v.begin() + 1, v.end() - 1);
    if (!r)
        warn ("Unknown option: " +  k);
    else if (sp == v)
        mesg ("Option '" + k + "' already has value: " + v);
    else {
        r = setp_core (k, v);
        if (r)
            mesg ("Value for '" + k + "' changed from '"+ sp
                    + "' to '" + v + "'");
        else
            warn ("'" + v + "' is not valid value for '" + k + "'");
    }
    return r;
}

bool DotSet::typep (const string k, string& v) const
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

int DotSet::expanp (const string k, vector<string>& e) const
{
    int len = k.size();
    e.clear();
    for (map <string, int*>::const_iterator i = ipar.begin(); i!=ipar.end();i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map <string, fp*>::const_iterator i = fpar.begin(); i!=fpar.end(); i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map <string, bool*>::const_iterator i = bpar.begin();i!=bpar.end();i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map <string, IntRange>::const_iterator i = irpar.begin(); 
                                                        i != irpar.end(); i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map <string, Enum_string>::const_iterator i = epar.begin(); 
                                                          i != epar.end(); i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    for (map <string, string*>::const_iterator i = spar.begin(); 
                                                          i != spar.end(); i++)
        if (!string(i->first, 0, len).compare (k))
            e.push_back (i->first);
    sort(e.begin(), e.end());
    return e.size();
}

int DotSet::expand_enum (string left, string k, vector<string>& r) const
{
    int len = k.size();
    r.clear();
    if (epar.count(left) == 0)
        return 0;
    map<char, string>& es = epar.find(left)->second.e;
    for (map<char, string>::iterator i = es.begin(); i != es.end(); i++)
        if (!string(i->second, 0, len).compare (k))
            r.push_back (i->second);
    return r.size();
}

string DotSet::print_usage (char c) const
{
    string s = "Usage: \n\t" + S(c) + ".set option = value\n"
        "or, to see current value: \n\t"
        + S(c) + ".set option\n"
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

string DotSet::set_script (char c) const
{
    vector<string> e;
    expanp ("", e);
    string s, t, v;
    for (vector<string>::const_iterator i = e.begin(); i != e.end(); i++){
        typep (*i, t);
        getp_core (*i, v);
        s += S(c) + ".set " + *i + " = " + (v.empty() ? "\"\"" : v) 
            + " # " + t + "\n";
    }
    return s;
}


