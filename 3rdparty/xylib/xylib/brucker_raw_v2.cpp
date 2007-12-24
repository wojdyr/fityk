// Implementation of class BruckerV23RawDataSet for reading meta-info and 
// xy-data from Siemens/Bruker Diffrac-AT Raw Format v2/3
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// ds_brucker_raw_v23.cpp $

/*
FORMAT DESCRIPTION:
====================

Siemens/Bruker Diffrac-AT Raw Format version 2/3, Data format used in 
Siemens/Brucker X-ray diffractors.

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   diffracat_v2v3_raw, 
    * Extension name:   raw
    * Binary/Text:      binary
    * Multi-blocks:     Y

///////////////////////////////////////////////////////////////////////////////
    * Format details:   
It is of the same type in the data organizaton with the v1 format, except that 
the fields have different meanings and offsets. See brucker_raw_v1.cpp for 
reference.

///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref: See brucker_raw_v1.cpp
*/


#include "brucker_raw_v2.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo BruckerV23RawDataSet::fmt_info(
    "diffracat_v2_raw",
    "Siemens/Bruker Diffrac-AT Raw File v2/v3",
    vector<string>(1, "raw"),
    true,                       // whether binary
    true,                       // whether has multi-blocks
    &BruckerV23RawDataSet::check 
);


bool BruckerV23RawDataSet::check(istream &f)
{
    string head = read_string(f, 4);
    return head == "RAW2";
}



} // end of namespace xylib

