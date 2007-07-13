// Implementation of class RigakuDataSet for reading meta-data and xy-data from 
// Licence: GNU General Public License version 2
// $Id: RigakuDataSet.h $

#include "ds_rigaku_dat.h"
#include "common.h"

using namespace std;
using namespace boost;
using namespace xylib;
using namespace xylib::util;

namespace xylib {

bool RigakuDataSet::is_filetype() const
{
    // the first 5 letters must be "*TYPE"
    ifstream &f = *p_ifs;

    f.clear();
    string head = read_string(f, 0, 5);
    if(f.rdstate() & ios::failbit) {
        throw XY_Error("error when reading file head");
    }

    return ("*TYPE" == head) ? true : false;
}


void RigakuDataSet::load_data() 
{
    init();
    ifstream &f = *p_ifs;

    
}


void RigakuDataSet::parse_range()
{

}

} // end of namespace xylib

