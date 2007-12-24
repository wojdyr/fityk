// Implementation of class VamasDataSet for reading meta-info and xy-data from 
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
    * Multi-blocks:     Y

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

#include "vamas.h"
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
    "vamas_iso14976",
    "ISO14976 VAMAS Surface Chemical Analysis Standard Data Transfer Format",
    vector<string>(1, "vms"),
    false,                       // whether binary
    true                         // whether has multi-blocks
);

bool VamasDataSet::check(istream &f)
{
    static const string magic = 
     "VAMAS Surface Chemical Analysis Standard Data Transfer Format 1988 May 4";
    string line;
    return getline(f, line) && str_trim(line) == magic;
}


void VamasDataSet::load_data(std::istream &f) 
{
    int n;
    skip_lines(f, 1);   // magic number: "VAMAS Sur..."
    meta["institution identifier"] = read_line(f);
    meta["institution model identifier"] = read_line(f);
    meta["operator identifier"] = read_line(f);
    meta["experiment identifier"] = read_line(f);

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
    if ("MAP" == exp_mode || "MAPD" == exp_mode ||
        "NORM" == exp_mode || "SDP" == exp_mode) {
        meta["number of spectral regions"] = read_line(f);
    }
    if ("MAP" == exp_mode || "MAPD" == exp_mode) {
        meta["number of analysis positions"] = read_line(f);
        meta["number of discrete x coordinates available in full map"] 
                                                            = read_line(f);
        meta["number of discrete y coordinates available in full map"] 
                                                            = read_line(f);
    }

    // experimental variables
    exp_var_cnt = read_line_int(f);
    for (int i = 1; i <= exp_var_cnt; ++i) {
        meta["experimental variable label " + S(i)] = read_line(f);
        meta["experimental variable unit " + S(i)] = read_line(f);
    }

    // fill `include' table
    n = read_line_int(f);    // # of entries in inclusion or exclusion list
    bool d = (n > 0);
    for (int i = 0; i < 40; ++i) {
        include[i] = !d;
    }
    n = (d ? n : -n);
    for (int i = 0; i < n; ++i) {
        int idx = read_line_int(f) - 1;
        include[idx] = d;
    }

    // # of manually entered items in block
    n = read_line_int(f);
    skip_lines(f, n);

    exp_fue = read_line_int(f);
    skip_lines(f, exp_fue);

    blk_fue = read_line_int(f);
    skip_lines(f, blk_fue);

    // handle the blocks
    unsigned blk_cnt = read_line_int(f);
    for (unsigned i = 0; i < blk_cnt; ++i) {
        Block *block = read_blk(f);
        blocks.push_back(block);
    }
}


// read one block from file
Block* VamasDataSet::read_blk(istream &f)
{
    Block *block = new Block;
    double x_start=0., x_step=0.;
    string x_name;

    int cor_var = 0;    // # of corresponding variables
    
    block->meta["block id"] = read_line(f);
    block->meta["sample identifier"] = read_line(f);

    if (include[0]) 
        block->meta["year"] = read_line(f);
    if (include[1]) 
        block->meta["month"] = read_line(f);
    if (include[2]) 
        block->meta["day"] = read_line(f);
    if (include[3]) 
        block->meta["hour"] = read_line(f);
    if (include[4]) 
        block->meta["minute"] = read_line(f);
    if (include[5]) 
        block->meta["second"] = read_line(f);
    if (include[6]) 
        block->meta["no. of hours in advanced GMT"] = read_line(f);

    if (include[7]) {   // skip comments on this block
        int cmt_lines = read_line_int(f);
        skip_lines(f, cmt_lines);
    }

    if (include[8]) 
        block->meta["tech"] = read_line(f);
    string tech = read_line(f);
    if (include[8]) 
        block->meta["tech"] = tech;
    // make sure tech has a valid value
    if (-1 == get_array_idx(techs, 14, tech)) {
        throw XY_Error("tech has an invalid value");
    }

    if (include[9]) {
        if ("MAP" == exp_mode || "MAPDP" == exp_mode) {
            block->meta["x coordinate"] = read_line(f);
            block->meta["y coordinate"] = read_line(f);
        }
    }

    if (include[10]) {
        for (int i = 0; i < exp_var_cnt; ++i) {
            block->meta["experimental variable value " + S(i)] = read_line(f);
        }
    }

    if (include[11]) 
        block->meta["analysis source label"] = read_line(f);

    if (include[12]) {
        if ("MAPDP" == exp_mode || "MAPSVDP" == exp_mode 
                || "SDP" == exp_mode || "SDPSV" == exp_mode 
                || "SNMS energy spec" == tech || "FABMS" == tech 
                || "FABMS energy spec" == tech || "ISS" == tech 
                || "SIMS" == tech || "SIMS energy spec" == tech 
                || "SNMS" == tech) {
            block->meta["sputtering ion oratom atomic number"] = read_line(f);
            block->meta["number of atoms in sputtering ion or atom particle"] 
                                                                = read_line(f);
            block->meta["sputtering ion or atom charge sign and number"] 
                                                                = read_line(f);
        }
    }

    if (include[13]) 
        block->meta["analysis source characteristic energy"] = read_line(f);
    if (include[14]) 
        block->meta["analysis source strength"] = read_line(f);

    if (include[15]) {
        block->meta["analysis source beam width x"] = read_line(f);
        block->meta["analysis source beam width y"] = read_line(f);
    }

    if (include[16]) {
        if ("MAP" == exp_mode || "MAPDP" == exp_mode || "MAPSV" == exp_mode 
                || "MAPSVDP" == exp_mode || "SEM" == exp_mode) {
            block->meta["field of view x"] = read_line(f);
            block->meta["field of view y"] = read_line(f);
        }
    }

    if (include[17]) {
        if ("SEM" == exp_mode || "MAPSV" == exp_mode || "MAPSVDP" == exp_mode) {
            throw XY_Error("do not support MAPPING mode");
        }
    }

    if (include[18]) 
        block->meta["analysis source polar angle of incidence"] = read_line(f);
    if (include[19]) 
        block->meta["analysis source azimuth"] = read_line(f);
    if (include[20]) 
        block->meta["analyser mode"] = read_line(f);
    if (include[21]) 
        block->meta["analyser pass energy or retard ratio or mass resolution"] 
                                                                = read_line(f);
    if (include[22]) {
        if ("AES diff" == tech) {
            block->meta["differential width"] = read_line(f);
        }
    }

    if (include[23]) 
        block->meta["magnification of analyser transfer lens"] = read_line(f);
    if (include[24]) 
        block->meta["analyser work function or acceptance energy of atom or ion"] = read_line(f);
    if (include[25]) 
        block->meta["target bias"] = read_line(f);

    if (include[26]) {
        block->meta["analysis width x"] = read_line(f);
        block->meta["analysis width y"] = read_line(f);
    }

    if (include[27]) {
        block->meta["analyser axis take off polar angle"] = read_line(f);
        block->meta["analyser axis take off azimuth"] = read_line(f);
    }

    if (include[28]) 
        block->meta["species label"] = read_line(f);

    if (include[29]) {
        block->meta["transition or charge state label"] = read_line(f);
        block->meta["charge of detected particle"] = read_line(f);
    }

    if (include[30]) {
        if ("REGULAR" == scan_mode) {
            x_name = read_line(f);
            block->meta["abscissa label"] = x_name;
            block->meta["abscissa units"] = read_line(f);
            x_start = read_line_double(f);
            x_step = read_line_double(f);
        }
        else
            throw XY_Error("Only REGULAR scans are supported now");
    }
    else
        throw XY_Error("how to find abscissa properties in this file?");

    if (include[31]) {
        cor_var = read_line_int(f);
        skip_lines(f, 2 * cor_var);   // 2 lines per corresponding_var
    }
    
    if (include[32]) 
        block->meta["signal mode"] = read_line(f);
    if (include[33]) 
        block->meta["signal collection time"] = read_line(f);
    if (include[34]) 
        block->meta["# of scans to compile this blk"] = read_line(f);
    if (include[35]) 
        block->meta["signal time correction"] = read_line(f);

    if (include[36]) {
        if (("AES1" == tech || "AES2" == tech || "EDX" == tech || "ELS" == tech
             || "UPS" == tech || "XPS" == tech || "XRF" == tech) 
            && ("MAPDP" == exp_mode || "MAPSVDP" == exp_mode 
                || "SDP" == exp_mode || "SDPSV" == exp_mode)) {
            skip_lines(f, 7);
        }
    }

    if (include[37]) {
        block->meta["sample normal polar angle of tilt"] = read_line(f);
        block->meta["sample normal polar tilt azimuth"] = read_line(f);
    }

    if (include[38]) 
        block->meta["sample rotate angle"] = read_line(f);
    
    if (include[39]) {
        unsigned n = read_line_int(f);   // # of additional numeric parameters
        for (unsigned i = 0; i < n; ++i) {
            // 3 items in every loop: param_label, param_unit, param_value
            string param_label = read_line(f);
            string param_unit = read_line(f);
            block->meta[param_label] = read_line(f) + param_unit;
        }
    }

    skip_lines(f, blk_fue);
    int cur_blk_steps = read_line_int(f);
    skip_lines(f, 2 * cor_var);   // min & max ordinate

    StepColumn *xcol = new StepColumn(x_start, x_step);
    xcol->set_name(x_name);
    VecColumn *ycol = new VecColumn;

    for (int i = 0; i < cur_blk_steps; ++i) {
        double y = read_line_double(f);
        ycol->add_val(y);
    }
    block->set_xy_columns(xcol, ycol);
    return block;
}

} // end of namespace xylib

