// Implementation of class VamasDataSet for reading meta-data and xy-data from 
// ISO14976 VAMAS Surface Chemical Ana0lysis Standard Data Transfer Format File
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_vamas.cpp $

/*

FORMAT DESCRIPTION:
====================

ISO14976 VAMAS Surface Chemical Ana0lysis Standard Data Transfer Format File
For more info about VAMAS, see http://www.biomateria.com/vamas.htm

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   vamas_iso14976
    * Extension name:   vms
    * Binary/Text:      text
    * Multi-ranged:     Y

///////////////////////////////////////////////////////////////////////////////
    * Format details: 
It is also organized as 
"file_header - (range_header - range_data) - (range_header - range_data) ..."
like many other text-based format. However, there is only a value in every 
line without a key or label; the value sequence and maybe several other values
determin what a given line means.

For more info, see the following:
1. W.A. Dench, L. B. Hazell and M. P. Seah, VAMAS Surface Chemical Analysis 
Standard Data Transfer Format with Skeleton Decoding Programs, published on 
Surface and Interface Analysis, 13(1988)63-122 or National Physics Laboratory 
Report DMA(A)164 July 1988
2. The ISO standard (iso14976)

///////////////////////////////////////////////////////////////////////////////
    Implementation Ref of xylib: 
based on the paper 1 mentioned above and the analysis of the sample files.

*/

#include "ds_vamas.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

// static string arrays to help check the file sanity

const static string exps[9] = {
    "MAP","MAPDP","MAPSV","MAPSVDP","NORM",
    "SDP","SDPSV","SEM","NOEXP",
};

const static string techs[14] = {
    "AES diff","AES dir","EDX","ELS","FABMS",
    "FABMS energy spec","ISS","SIMS","SIMS energy spec","SNMS",
    "SNMS energy spec","UPS","XPS","XRF"
};

const static string scans[3] = {
    "REGULAR","IRREGULAR","MAPPING",
};


const FormatInfo VamasDataSet::fmt_info(
    FT_VAMAS,
    "vamas_iso14976",
    "ISO14976 VAMAS Surface Chemical Analysis Standard Data Transfer Format",
    vector<string>(1, "vms"),
    false,                       // whether binary
    true                         // whether multi-ranged
);

bool VamasDataSet::check(istream &f)
{
    // the first line must be "VAMAS Surface ..."
    static string magic = "VAMAS Surface Chemical Analysis Standard Data Transfer Format 1988 May 4";
    string line;

    bool re = (getline(f, line) && str_trim(line) == magic);

    f.seekg(0);
    return re;
}


void VamasDataSet::load_data() 
{
    if (!check(f)) {
        throw XY_Error("file is not the expected " + get_filetype() + " format");
    }

    int n;

    skip_lines(f, 1);   // magic number: "VAMAS Sur..."
    add_meta("institution identifier", read_line(f));
    add_meta("institution model identifier", read_line(f));
    add_meta("operator identifier", read_line(f));
    add_meta("experiment identifier", read_line(f));

    // skip comment lines, n is number of lines
    n = read_line_int(f);
    skip_lines(f, n);

    exp_mode = str_trim(read_line(f));
    // make sure exp_mode has a valid value
    if (-1 == get_array_idx(exps, 9, exp_mode)) {
        throw XY_Error("exp_mode has an invalid value");
    }
    
    scan_mode = str_trim(read_line(f));
    // make sure scan_mode has a valid value
    if (-1 == get_array_idx(scans, 3, scan_mode)) {
        throw XY_Error("scan_mode has an invalid value");
    }

    // some exp_mode specific file-scope meta-info
    if (("MAP" == exp_mode) || ("MAPD" == exp_mode) ||
        ("NORM" == exp_mode) || ("SDP" == exp_mode)) {
        add_meta("number of spectral regions", read_line(f));
    }
    if (("MAP" == exp_mode) || ("MAPD" == exp_mode)) {
        add_meta("number of analysis positions", read_line(f));
        add_meta("number of discrete x coordinates available in full map", 
            read_line(f));
        add_meta("number of discrete y coordinates available in full map", 
            read_line(f));
    }

    // experimental variables
    exp_var_cnt = read_line_int(f);
    for (int i = 1; i <= exp_var_cnt; ++i) {
        add_meta("experimental variable label" + S(i), read_line(f));
        add_meta("experimental variable unit" + S(i), read_line(f));
    }

    n = read_line_int(f);	// # of entries in inclusion or exclusion list
    bool d = (n > 0);
    for (int i = 0; i < 40; ++i) {
        include[i] = !d;
    }
    n = (d ? n : -n);
    for (int i = 0; i < n; ++i) {
        int idx = read_line_int(f) - 1;
        include[idx] = d;
    }

    // # of manually entered items in blk
    n = read_line_int(f);
    skip_lines(f, n);

    exp_fue = read_line_int(f);
    skip_lines(f, exp_fue);

    blk_fue = read_line_int(f);
    skip_lines(f, blk_fue);

    // handle the blocks
    unsigned blk_cnt = read_line_int(f);
    for (unsigned i = 0; i < blk_cnt; ++i) {
        FixedStepRange *p_rg = new FixedStepRange;
        vamas_read_blk(p_rg);
        ranges.push_back(p_rg);
    }
}


// read one blk, used by load_vamas_file()
void VamasDataSet::vamas_read_blk(FixedStepRange *p_rg)
{
    int cor_var = 0;    // # of corresponding variables
    
    p_rg->add_meta("block id", read_line(f));
    p_rg->add_meta("sample identifier", read_line(f));

    read_meta_line(0, p_rg, "year");
    read_meta_line(1, p_rg, "month");
    read_meta_line(2, p_rg, "day");
    read_meta_line(3, p_rg, "hour");
    read_meta_line(4, p_rg, "minite");
    read_meta_line(5, p_rg, "second");
    read_meta_line(6, p_rg, "no. of hours in advanced GMT");

    if (include[7]) {   // skip comments on this blk
        int cmt_lines = read_line_int(f);
        skip_lines(f, cmt_lines);
    }

    read_meta_line(8, p_rg, "tech");
    string tech = p_rg->get_meta("tech");
    // make sure tech has a valid value
    if (-1 == get_array_idx(techs, 14, tech)) {
        throw XY_Error("tech has an invalid value");
    }

    if (include[9]) {
        if (("MAP" == exp_mode) || ("MAPDP" == exp_mode)) {
            p_rg->add_meta("x coordinate", read_line(f));
            p_rg->add_meta("y coordinate", read_line(f));
        }
    }

    if (include[10]) {
        for (int i = 0; i < exp_var_cnt; ++i) {
            p_rg->add_meta("experimental variable value" + S(i), read_line(f));
        }
    }

    read_meta_line(11, p_rg, "analysis source label");

    if (include[12]) {
        if (("MAPDP" == exp_mode) || ("MAPSVDP" == exp_mode) || 
            ("SDP" == exp_mode) || ("SDPSV" == exp_mode) || ("SNMS energy spec" == tech) ||
            ("FABMS" == tech) || ("FABMS energy spec" == tech) || ("ISS" == tech) ||
            ("SIMS" == tech) || ("SIMS energy spec" == tech) || ("SNMS" == tech)) {
            p_rg->add_meta("sputtering ion oratom atomic number", read_line(f));
            p_rg->add_meta("number of atoms in sputtering ion or atom particle", read_line(f));
            p_rg->add_meta("sputtering ion or atom charge sign and number", read_line(f));            
        }
    }

    read_meta_line(13, p_rg, "analysis source characteristic energy");
    read_meta_line(14, p_rg, "analysis source strength");

    if (include[15]) {
        p_rg->add_meta("analysis source beam width x", read_line(f));
        p_rg->add_meta("analysis source beam width y", read_line(f));
    }

    if (include[16]) {
        if (("MAP" == exp_mode) || ("MAPDP" == exp_mode) || ("MAPSV" == exp_mode) ||
            ("MAPSVDP" == exp_mode) || ("SEM" == exp_mode)) {
            p_rg->add_meta("field of view x", read_line(f));
            p_rg->add_meta("field of view y", read_line(f));
        }
    }

    if (include[17]) {
        if (("SEM" == exp_mode) || ("MAPSV" == exp_mode) || ("MAPSVDP" == exp_mode)) {
            throw XY_Error("do not support MAPPING mode");
        }
    }

    read_meta_line(18, p_rg, "analysis source polar angle of incidence");
    read_meta_line(19, p_rg, "analysis source azimuth");
    read_meta_line(20, p_rg, "analyser mode");
    read_meta_line(21, p_rg, "analyser pass energy or retard ratio or mass resolution");

    if (include[22]) {
        if ("AES diff" == tech) {
            p_rg->add_meta("differential width", read_line(f));
        }
    }

    read_meta_line(23, p_rg, "magnification of analyser transfer lens");
    read_meta_line(24, p_rg, "analyser work function or acceptance energy of atom or ion");
    read_meta_line(25, p_rg, "target bias");

    if (include[26]) {
        p_rg->add_meta("analysis width x", read_line(f));
        p_rg->add_meta("analysis width y", read_line(f));
    }

    if (include[27]) {
        p_rg->add_meta("analyser axis take off polar angle", read_line(f));
        p_rg->add_meta("analyser axis take off azimuth", read_line(f));
    }

    read_meta_line(28, p_rg, "species label");

    if (include[29]) {
        p_rg->add_meta("transition or charge state label", read_line(f));
        p_rg->add_meta("charge of detected particle", read_line(f));
    }

    if (include[30]) {
        if ("REGULAR" == scan_mode) {
            p_rg->add_meta("abscissa label", read_line(f));            
            p_rg->add_meta("abscissa units", read_line(f));  
            p_rg->set_x_start(read_line_double(f));
            p_rg->set_x_step(read_line_double(f));
        }
    }

    if (include[31]) {
        cor_var = read_line_int(f);
        skip_lines(f, 2 * cor_var);   // 2 lines per corresponding_var
    }
    
    read_meta_line(32, p_rg, "signal mode");
    read_meta_line(33, p_rg, "signal collection time");
    read_meta_line(34, p_rg, "# of scans to compile this blk");
    read_meta_line(35, p_rg, "signal time correction");

    if (include[36]) {
        if (("AES1" == tech || "AES2" == tech || "EDX" == tech || "ELS" == tech || "UPS" == tech || 
            "XPS" == tech || "XRF" == tech) &&
            ("MAPDP" == exp_mode || "MAPSVDP" == exp_mode || "SDP" == exp_mode || "SDPSV" == exp_mode)) {
            skip_lines(f, 7);
        }
    }

    if (include[37]) {
        p_rg->add_meta("sample normal polar angle of tilt", read_line(f));
        p_rg->add_meta("sample normal polar tilt azimuth", read_line(f));
    }

    read_meta_line(38, p_rg, "sample rotate angle");
    
    if (include[39]) {
        unsigned n = read_line_int(f);   // # of additional numeric parameters
        for (unsigned i = 0; i < n; ++i) {
            // 3 items in every loop: param_label, param_unit, param_value
            string param_label = read_line(f);
            string param_unit = read_line(f);
            p_rg->add_meta(param_label, read_line(f) + param_unit);
        }
    }

    skip_lines(f, blk_fue);
    int cur_blk_steps = read_line_int(f);
    skip_lines(f, 2 * cor_var);   // min & max ordinate

    double y;
    for (int i = 0; i < cur_blk_steps; ++i) {
        y = read_line_double(f);
        p_rg->add_y(y);
    }

}


// a simple wrapper to simplify the code
void VamasDataSet::read_meta_line(int idx, FixedStepRange *p_rg, string meta_key)
{
    if (include[idx]) {
        p_rg->add_meta(meta_key, read_line(f));
    }
}

} // end of namespace xylib

