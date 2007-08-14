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
    
data_ALUMINA_publ   // block start identifier

// the following line uses a double quote string as value
_audit_creation_method  "from EXP file using GSAS2CIF"  

// the following line uses a string as value
_audit_creation_date                   2002-12-21T19:04

// the following line uses multi-lines block marked by ';' as value
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

// end of file flag
#--eof--eof--eof--eof--eof--eof--eof--eof--eof--eof--eof--eof--eof--eof--eof--#

///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref of xylib: 
mainly based on the file format specification mentioned above, the tcl/tk 
source code of in script browsecif.tcl in the ciftools_Linux package which can 
be downloaded from:
http://www.iucr.org/iucr-top/cif/index.html

*/

#include "ds_pdcif.h"
#include "util.h"

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
    bool ret = get_valid_line(f, line, "#") && str_startwith(line, "data_");

    f.seekg(0);
    return ret;
}

void PdCifDataSet::load_data() 
{
    if (!check(f)) {
        throw XY_Error("file is not the expected " + get_filetype() + " format");
    }

    string line;
    FixedStepRange *p_rg = NULL;
    string last_key;             // store the key name whose value is given later
    int loop_flg(OUT_OF_LOOP);   // indicates where we are
    int loop_i(0);
    vector<string> loop_vars;    // variable names during a loop
    
    while (get_valid_line(f, line, "#")) {
        if (str_startwith(line, "data_")) {         // range start
            if ((p_rg != NULL) && (p_rg->get_pt_count() > 0)) {
                ranges.push_back(p_rg);
                p_rg = NULL;
            }

            p_rg = new FixedStepRange;
            my_assert(p_rg != NULL, "memory allocation failed");

            if (!add_key_val(p_rg, "name", line.substr(5))) {
                throw XY_Error("range name empty or already exists");
            }
            
         } else if (str_startwith(line, "_")) {      // key name
            my_assert(p_rg != NULL, "key name apears before 'data_'");

            // key_epos: first position beyond the end of key_name
            string::size_type key_epos = line.find_first_of(" \t"); 
            bool followed_by_val = (key_epos != string::npos);
            switch (loop_flg) {
            case IN_LOOP_BODY:
                loop_flg = OUT_OF_LOOP;
                // NOTE: no break here
            case OUT_OF_LOOP:
                if (followed_by_val) {
                    string key = line.substr(1, key_epos - 1); // start without the start '_'
                    string val = line.substr(key_epos);
                    add_key_val(p_rg, key, val);

                    // TODO: x-y data structure need to be modified to adapt this format
                    // incomplete here: there are many possible X-Ys
                    if ("pd_meas_2theta_range_min" == key) {
                        p_rg->set_x_start(my_strtod(val));
                    } else if ("pd_meas_2theta_range_inc" == key) {
                        p_rg->set_x_step(my_strtod(val));
                    }
                } else {
                    // val must be in the following valid lines
                    my_assert(last_key.empty(), "last key " + last_key + " has not value");
                    my_assert(line.size() > 1, "wrong key_name of '_'");
                    last_key = line.substr(1);
                }
                break;
                
            case IN_LOOP_HEAD:
                // must not followed by value: key_name only
                my_assert(!followed_by_val, "in loop head, there must be key_name only");
                loop_vars.push_back(line.substr(1));
                break;

            default:
                break;
            }

        } else if (str_startwith(line, "loop_")) {  // loop start
            my_assert(p_rg != NULL, "'loop_' appears before 'data_'");
            my_assert(loop_flg != 0, "loop without values");

            loop_flg = IN_LOOP_HEAD;
            loop_vars.clear();
            loop_i = 0;
            my_assert(last_key.empty(), "last key " + last_key + " has not value");
            
            // may followed by some key names (start with a '_')
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
                loop_vars.push_back(key_name);
            }
            
        } else {    // should be values
            my_assert(p_rg != NULL, "values appears before 'data_'");
            vector<string> values;

            get_all_values(line, f, values);

            switch (loop_flg) {
            case OUT_OF_LOOP:
                my_assert(!last_key.empty(), "unexpected value without key_name");
                my_assert(values.size() == 1, "");
//                my_assert(values.size() != 0, "");
                add_key_val(p_rg, last_key, values[0]);
                last_key = "";
                break;

            case IN_LOOP_HEAD:
                loop_flg = IN_LOOP_BODY;
                // NOTE: no break here

            case IN_LOOP_BODY:
                my_assert(loop_vars.size() != 0, "no loop variables given");

                for (unsigned i = 0; i < values.size(); ++i) {
                    string key = loop_vars[loop_i] + "#" + S(loop_i);
                    add_key_val(p_rg, key, S(values[i]));
    				
    				if (loop_vars[loop_i] == "pd_meas_intensity_total") {
    					my_assert(p_rg->get_x_step() != 0, "x_step has not been read in");

                        // this value may followed by measuring-uncertainty in "()"
                        string val = values[i].substr(0, values[i].find("("));
    					p_rg->add_y(my_strtod(val));
    				}

    				loop_i = (loop_i + 1) % loop_vars.size();
                }
                break;
                
            default:    
                break;
            }

        }
    }

    if ((p_rg != NULL) && (p_rg->get_pt_count() > 0)) {
        ranges.push_back(p_rg);
        p_rg = NULL;
    }
}


void PdCifDataSet::get_all_values(const string &line, istream &f, vector<string> &values) 
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
                val_epos = line.find_first_of(" \t", val_spos);
                break;
            }

            string val = line.substr(val_spos, val_epos - val_spos);
            values.push_back(val);
        }
    }
}


// return: true if meta-info is added successfully, false otherwise
bool PdCifDataSet::add_key_val(FixedStepRange *p_rg, const string &key, const string &val)
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

} // end of namespace xylib


