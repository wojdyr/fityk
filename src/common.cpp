// This file is part of fityk program. Copyright (C) Marcin Wojdyr 
#include "common.h"
RCSID ("$Id$")

#include <string>
#include <time.h>
#include "v_IO.h"
#include "other.h"

using namespace std;

char verbosity = 3;
bool exit_on_error = false;
char auto_plot = 2;
int smooth_limit = 0;
volatile bool user_interrupt = false;
vector<fp> fp_v0; //just empty vector
vector<int> int_v0; //just empty vector

const fp INF = 1e99; //almost ininity. floating points limits are about:
                     // double: 10^+308, float: 10^+38, long double: 10^+4932

vector<int> range_vector (int l, int u)
{
    vector<int> v(u - l);
    for (int i = l; i < u; i++)
        v[i - l] = i;
    return v;
}

void gmessage (const string& str) 
{
    my_IO->message (str.c_str());
    my_other->log_output (str);
}

int warn (const string &s) {
    if (verbosity >= 1) {
	string st = "! " + s;
	gmessage (st);
    }
    if (exit_on_error)
	exit(-1);
    return -1;
}

std::string time_now ()
{
    const time_t czas = time(0);
    return ctime (&czas);
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

