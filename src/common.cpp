// This file is part of fityk program. Copyright (C) Marcin Wojdyr 
// $Id$

#include "common.h"
#include <string>
#include <time.h>
#include "ui.h"
#include "other.h"

using namespace std;

int smooth_limit = 0; //TODO use it in GUI version
volatile bool user_interrupt = false;
const vector<fp> fp_v0; //just empty vector
const vector<int> int_v0; //just empty vector

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
    const time_t t = time(0);
    return ctime (&t);
}

bool is_double (string s) {
    if (s.empty()) return false;
    const char *c = s.c_str();
    char *endptr;
    strtod(c, &endptr);
    while (isspace(*endptr))
        endptr++;
    return (*endptr == 0); 
}

void replace_all(string &s, const string &old, const string &new_)
{
    string::size_type pos = 0; 
    while ((pos = s.find(old, pos)) != string::npos) 
        s.replace(pos, old.size(), new_);
}

