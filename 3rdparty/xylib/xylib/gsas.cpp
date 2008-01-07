// GSAS File Format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_gsas.cpp $

// based on GSAS Manual, chapter "GSAS Standard Powder Data File"

#include <cstdio>

#include "gsas.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo GsasDataSet::fmt_info(
    "gsas",
    "GSAS Standard Powder Data File",
    vector<string>(1, "gss"), // also .gsa, .gsas?, .dat?
    false,                       // whether binary
    false,                        // whether has multi-blocks
    &GsasDataSet::ctor,
    &GsasDataSet::check
);

bool GsasDataSet::check(istream &f) {
    string line;
    getline(f, line); // first line is title
    getline(f, line);
    while (line.empty() || line[0] == '#')
        getline(f, line);
    return str_startwith(line, "BANK") 
        || str_startwith(line, "TIME_MAP")
        || str_startwith(line, "Instrument parameter");
}

void GsasDataSet::load_data(std::istream &f) 
{
    string line;
    getline(f, line); // first line is title
    meta["title"] = str_trim(line);

    // optional Instrument parameter
    string const ip = "Instrument parameter";
    getline(f, line); 
    if (str_startwith(line, ip)) {
        meta[ip] = str_trim(line.substr(ip.size()));
        getline(f, line); 
    }

    // optional comments 
    while (line.empty() || line[0] == '#')
        getline(f, line);

    //TODO: implement reading the file format
    // based on GSAS Manual


}

} // end of namespace xylib

