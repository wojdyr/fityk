// This file is part of fityk program. Copyright (C) Marcin Wojdyr 
// $Id$

#include "common.h"
#include <string>
#include <time.h>
#include <stdlib.h>
#include "ui.h"
#include "logic.h"

using namespace std;

volatile bool user_interrupt = false;

const fp INF = 1e99; //almost ininity. floating points limits are about:
                     // double: 10^+308, float: 10^+38, long double: 10^+4932

const char* fityk_version_line = "# Fityk script. Fityk version: " VERSION;

vector<int> range_vector (int l, int u)
{
    vector<int> v(u - l);
    for (int i = l; i < u; i++)
        v[i - l] = i;
    return v;
}

std::string time_now ()
{
    time_t const t = time(0);
    return ctime (&t);
}

bool is_double (string const& s) {
    char const *c = s.c_str();
    char *endptr;
    strtod(c, &endptr);
    if (c == endptr)
        return false;
    while (isspace(*endptr))
        endptr++;
    return (*endptr == 0); 
}

bool is_int (string const& s) {
    char const *c = s.c_str();
    char *endptr;
    strtol(c, &endptr, 10);
    if (c == endptr)
        return false;
    while (isspace(*endptr))
        endptr++;
    return (*endptr == 0); 
}

void replace_all(string &s, string const &old, string const &new_)
{
    string::size_type pos = 0; 
    while ((pos = s.find(old, pos)) != string::npos) {
        s.replace(pos, old.size(), new_);
        pos += new_.size();
    }
}

/// replaces all words `old_word' in text `str' with `new_word'
///  word `foo' is in: "4*foo+1" but not in "foobar" nor in "_foo"
void replace_words(string &t, string const &old_word, string const &new_word)
{
    string::size_type pos = 0;
    while ((pos=t.find(old_word, pos)) != string::npos) {
        int k = old_word.size();
        if ((pos == 0 || !(isalnum(t[pos-1]) || t[pos-1]=='_'))
                && (pos+k==t.size() || !(isalnum(t[pos+k]) || t[pos+k]=='_'))) {
            t.replace(pos, k, new_word);
            pos += new_word.size();
        }
        else
            pos++;
    }
}

