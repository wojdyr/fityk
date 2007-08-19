// Implementation of class PdCifDataSet for reading meta-data 
// and xy-data from pdCIF Format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_pdcif.cpp $

/*

FORMAT DESCRIPTION:
====================
Crystallographic Information File (CIF) is a standard text file format for 
representing crystallographic information, promulgated by the International 
Union of Crystallography (IUCr).

CIF for Powder Diffraction (pdCIF) is an extension to the basic CIF frame-
work in XRD field.

For more info, see the wikipedia entry of CIF:
    http://en.wikipedia.org/wiki/Crystallographic_Information_File
and the IUCr official website:
    http://www.iucr.org

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   pdcif
    * Extension name:   cif
    * Binary/Text:      text
    * Multi-ranged:     Y
    
///////////////////////////////////////////////////////////////////////////////
    * Format details: 
See the official format specification at
    http://www.iucr.org/iucr-top/cif/index.html

Every block starts with "data_xxx". Key names of meta-info starts with a '_', 
the value of that key can be given followed after the key name in the same line, 
or use a block quoted with ';'
"loop_" indicate that there is a loop (like a data table) there. The key names 
given right after "loop_" are the table header, and the following data is the 
table contents.
It uses '#" to comment out all stuff afterwards like in shell or perl script.
    
///////////////////////////////////////////////////////////////////////////////
    * Sample Fragment: ("//xxx": comments added by me; ...: omitted lines)
    
// block start identifier: block name is the string follows 'data_'
data_ALUMINA_publ   

// the following line uses a double quote string as value
_audit_creation_method  "from EXP file using GSAS2CIF"  

// the following line uses a single-line string as value
_audit_creation_date                   2002-12-21T19:04

// the following line uses a multi-lines block marked by ';' as value
_audit_update_record
; 2002-12-21T19:04  Initial CIF as created by GSAS2CIF
;
...
_pd_meas_2theta_range_min              3.0      // x_start used in current impl
_pd_meas_2theta_range_max              167.95
_pd_meas_2theta_range_inc              0.05     // x_step used in current impl
_pd_proc_2theta_range_min              2.9824
_pd_proc_2theta_range_max              167.9324
_pd_proc_2theta_range_inc              0.05

loop_
      _pd_meas_intensity_total
      _pd_proc_ls_weight
      _pd_proc_intensity_bkg_calc
      _pd_calc_intensity_total
   119(17)     0.0         101.9   .
   149(19)     0.002770    101.5   101.5
...

data_ALUMINA_pub2
...

///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref of xylib: 
mainly based on the file format specification mentioned above, and the tcl/tk 
source code of script browsecif.tcl in the ciftools_Linux package which can 
be downloaded from:
http://www.iucr.org/iucr-top/cif/index.html

///////////////////////////////////////////////////////////////////////////////
    * Known issues:
There are still some problems when handling some pdCIF files with multiple
data blocks in a range. 
Like in NISI.cif, there are 2 data blocks in 1 range, and the counts of the data 
points are different. This will cause the program to crash. Need be fixed.

*/

#include "ds_pdcif.h"
#include "util.h"
#include <map>

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo PdCifDataSet::fmt_info(
    FT_PDCIF,
    "pdcif",
    "The Crystallographic Information File for Powder Diffraction",
    vector<string>(1, "cif"),
    false,                      // whether binary
    true                        // whether multi-ranged
);

bool PdCifDataSet::check(istream &f) {
    string line;

    // the 1st valid line must start with "data_"
    if (!get_valid_line(f, line, "#") && str_startwith(line, "data_")) {
        f.seekg(0);
        return false;
    }

    // in pdCIF, there must be at least a key whose name starts with "_pd"
    // avoid mistaking other non-pdCIF CIF files (e.g mmCIF, core CIF) as pdCIF
    bool ret(false);
    while (get_valid_line(f, line, "#")) {
        if (str_startwith(line, "_pd")) {
            ret = true;
            break;
        }
    }
    f.seekg(0);
    return ret;
}

void PdCifDataSet::load_data(std::istream &f) 
{
    if (!check(f)) {
        throw XY_Error("file is not the expected " + get_filetype() + " format");
    }
    clear();

    // names of keys, whose values may be used as X or Y
    static const string valued_keys[] = {
        "pd_meas_intensity_total",
        "pd_proc_ls_weight",
        "pd_proc_intensity_bkg_calc",
        "pd_calc_intensity_total",
    };
    static const unsigned VALUED_KEYS_NUM = sizeof(valued_keys) / sizeof(string);

    // indicate where we are
    enum {
        OUT_OF_LOOP = -1,   // not in a loop now
        IN_LOOP_HEAD = 0,   // in the loop head, where loop key names are given
        IN_LOOP_BODY = 1,   // in the loop body, where the data is given
    } loop_flg = OUT_OF_LOOP;


    string line;
    Range *p_rg = NULL;
    string last_key;             // store the key name whose value will be given later
    int loop_i(0);
    vector<string> loop_keys;    // names of the keys in a loop
    map<string, VecColumn*> mapper;

    while (get_valid_line(f, line, "#")) {
        if (str_startwith(line, "data_")) {         // range start
            loop_flg = OUT_OF_LOOP;
        
            if ((p_rg != NULL) && (p_rg->get_column_cnt() > 0)) {
                // save last range
                add_range(p_rg);
                p_rg = NULL;
            }

            p_rg = new Range;
            if (!add_key_val(p_rg, "name", line.substr(5))) {
                throw XY_Error("range name empty or already exists");
            }
            
         } else if (str_startwith(line, "_")) {      // key name
            my_assert(p_rg != NULL, "key name apears before 'data_'");

            // key_epos: first position beyond the end of key_name
            string::size_type key_epos = line.find_first_of(" \t"); 
            bool followed_by_val = (key_epos != string::npos);
            switch (loop_flg) {
            case IN_LOOP_HEAD:
                if (!followed_by_val) {
                    // names of the keys in the loop
                    loop_keys.push_back(line.substr(1));
                    break;
                } else {
                    // empty loop body
                    // NOTE: no break here
                }
            case IN_LOOP_BODY:
                loop_flg = OUT_OF_LOOP;
                // NOTE: no break here
            case OUT_OF_LOOP:
                if (followed_by_val) {
                    string key = line.substr(1, key_epos - 1); // start without the leading '_'
                    string val = line.substr(key_epos);
                    add_key_val(p_rg, key, val);
                } else {
                    // val must be in the following valid lines
                    my_assert(line.size() > 1, "wrong key_name of '_'");
                    last_key = line.substr(1);
                }
                break;
            default:
                break;
            }

        } else if (str_startwith(line, "loop_")) {  // loop start
            my_assert(p_rg != NULL, "'loop_' appears before 'data_'");
            my_assert(loop_flg != IN_LOOP_HEAD, "loop without values");

            loop_flg = IN_LOOP_HEAD;
            loop_keys.clear();
            mapper.clear();
            loop_i = 0;
            
            // "loop_" may followed by some key names (start with a '_')
            string::size_type key_spos(5), key_epos(5);     // start_pos & end_pos
            while (true) {
                key_spos = line.find_first_not_of(" \t", key_epos);
                if (string::npos == key_spos) {
                    break;
                }

                my_assert('_' == line[key_spos], "not a key_name followed by 'loop_'");
                key_epos = line.find_first_of(" \t", key_spos);
                string key_name = line.substr(key_spos + 1, key_epos - key_spos - 1);
                my_assert(!key_name.empty(), "key_name empty followed by 'loop_'");
                loop_keys.push_back(key_name);
            }
            
        } else {    // should be values
            my_assert(p_rg != NULL, "values appears before 'data_'");
            vector<string> values;

            get_all_values(line, f, values, loop_flg != OUT_OF_LOOP);

            switch (loop_flg) {
            case OUT_OF_LOOP:
                my_assert(!last_key.empty(), "unexpected value without key_name");
                my_assert(1 == values.size(), "value.size() != 1");
                add_key_val(p_rg, last_key, values[0]);
                last_key = "";
                break;

            case IN_LOOP_HEAD:
                loop_flg = IN_LOOP_BODY;
                // NOTE: no break here

            case IN_LOOP_BODY:
                my_assert(loop_keys.size() != 0, "no loop keys given");

                for (unsigned i = 0; i < values.size(); ++i) {
                    string key = loop_keys[loop_i];

                    if (get_array_idx(valued_keys, VALUED_KEYS_NUM, key) != -1) {
                        // this is a point can be used as X or Y

                        VecColumn *p_col = get_col_ptr(p_rg, key, mapper);

                        // this value may followed by measuring-uncertainty in "()"
                        string::size_type spos = values[i].find("(");    // start pos of uncertainty
                        string::size_type epos = values[i].find(")");    // end pos of uncertainty
                        string val;

                        if (spos != string::npos) {
                            my_assert(epos != string::npos, "'(' does not have a matching ')'");

                            // this column also has an uncertainty column 
                            // we store these data in a VecColumn named $key_uncertainty
                            string key_uncty = key + "_uncertainty";
                            VecColumn *p_uncty_col = get_col_ptr(p_rg, key_uncty, mapper);

                            string val_uncty = values[i].substr(spos + 1, epos - spos);
                            p_uncty_col->add_val(my_strtod(val_uncty));

                            val = values[i].substr(0, spos);
                        } else {
                            val = values[i];
                        }

                        p_col->add_val(my_strtod(val)); // add the value without uncertainty
                    } else {
                        string my_key = key + "#" + S(loop_i);
                        add_key_val(p_rg, my_key, S(values[i]));
                    }

                    loop_i = (loop_i + 1) % loop_keys.size();
                }
                break;
                
            default:    
                break;
            }
        }
    }

    if ((p_rg != NULL) && (p_rg->get_column_cnt() > 0)) {
        add_range(p_rg);
        p_rg = NULL;
    }
}


// get all "values" (in the key-value pair) from f. 
// line: already read in line at caller place
// values: ref of vector to hold the values
// NOTE: multi-range value starts with a ';' will be read in until its end
void PdCifDataSet::get_all_values(const string &line, istream &f, 
    vector<string> &values, bool in_loop) 
{
    if (str_startwith(line, ";")) {      // multi-lined value
        string val = line + '\n';
        
        string ln;
        while (get_valid_line(f, ln, "#")) {
            if (str_startwith(ln, ";")) {
                break;
            }

            val += ln + '\n';
        }
        my_assert(!f.eof(), "unexpected EOF");
        values.push_back(val);
    } else {                            // single-line value
        string::size_type val_spos(0), val_epos(0);
        while (true) {
            val_spos = line.find_first_not_of(" \t", val_epos);
            if (string::npos == val_spos) {
                break;
            }
            
            switch (line[val_spos]) {
            case '"':
                // NOTE: no break here
            case '\'':
                val_epos = line.find(line[val_spos], val_spos + 1);
                my_assert(val_epos != string::npos, "quote " + S(line[val_spos]) + " not match");
                ++val_epos;     // val_epos is the first char after the value string
                break;
            default:
                val_epos = in_loop ? line.find_first_of(" \t", val_spos) : string::npos;
                break;
            }

            string val = in_loop ? line.substr(val_spos, val_epos - val_spos) : line.substr(val_spos);
            values.push_back(val);
        }
    }
}


// add the "key-val" as a meta to p_rg
// return: true if meta-info is added successfully, false otherwise
bool PdCifDataSet::add_key_val(Range *p_rg, const string &key, const string &val)
{
    string k = str_trim(key);
    string v = str_trim(val);

    if (NULL == p_rg || k.empty() || v.empty()) {
        throw XY_Error("key or value of meta-info in empty");
    }

    // remove the heading ';' of multi-line val, note ending ';' not in v
    if (';' == v[0]) {
        v = str_trim(v.substr(1));
    } else if ('"' == v[0] || '\'' == v[0]) {
        string::size_type epos = v.rfind(v[0]);
        my_assert(epos != 0, "close quote not found");

        v = str_trim(v.substr(1, epos - 1));    
    }
    
    if ("?" == v) {
        // unknown value
        return false;
    }

    return p_rg->add_meta(k, v);
}


// find step columns in p_rg, add p_rg to ranges
void PdCifDataSet::add_range(Range* p_rg)
{
    // add the columns implied in the meta-keys
    static const string x_names[] = {"pd measurement 2theta", "pd processed 2theta"};
    static const string x_start_keys[] = {"pd_meas_2theta_range_min", "pd_proc_2theta_range_min"};
    static const string x_step_keys[] = {"pd_meas_2theta_range_inc", "_pd_proc_2theta_range_inc"};
    static const int X_NUM = sizeof(x_names) / sizeof(string);

    for (int i = 0; i < X_NUM; ++i) {
        if (p_rg->has_meta_key(x_start_keys[i]) && 
            p_rg->has_meta_key(x_step_keys[i])) {
            double start_x = my_strtod(p_rg->get_meta(x_start_keys[i]));
            my_assert((-180.0 <= start_x) && (start_x <= 360.0), 
                "measurement_2theta_range_min should be between -180 and 360");
            double step_x = my_strtod(p_rg->get_meta(x_step_keys[i]));
            Column *p_x = new StepColumn(start_x, step_x);
            p_x->set_name(x_names[i]);
            p_rg->add_column(p_x, Range::CT_X);
        }
    }
   
    ranges.push_back(p_rg);
}


// get the VecColumn ptr whose name is "name". 
// mapping info is stored in "map"
// NOTE: if no VecColumn in map matches "name", a new vecColumn is created
VecColumn* PdCifDataSet::get_col_ptr(Range *p_rg, const string name, map<string, VecColumn*>& mapper)
{
    VecColumn *p_col;
    map<string, VecColumn*>::const_iterator it = mapper.find(name);
    if (mapper.end() == it) {
        p_col = new VecColumn;
        p_col->set_name(name);
        p_rg->add_column(p_col, Range::CT_Y);
        pair<map<string, VecColumn*>::iterator, bool> ret = 
            mapper.insert(make_pair(name, p_col));
        my_assert(ret.second, "mapper insertion failed");
    } else {
        p_col = mapper.find(name)->second;
    }

    return  p_col;
}

} // end of namespace xylib


