// ISO14976 VAMAS Surface Chemical Analysis Standard Data Transfer Format File
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

#include <cstring>
#include "vamas.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo VamasDataSet::fmt_info(
    "vamas",
    "VAMAS (ISO-14976)",
    vector_string("vms"),
    false,                       // whether binary
    true,                        // whether has multi-blocks
    &VamasDataSet::ctor,
    &VamasDataSet::check
);

} // namespace

namespace {

// NULL-terminated arrays -  dictionaries for VAMAS data

const char* exps[] = {
    "MAP","MAPDP","MAPSV","MAPSVDP","NORM",
    "SDP","SDPSV","SEM","NOEXP", NULL
};

const char* techs[] = {
    "AES diff","AES dir","EDX","ELS","FABMS",
    "FABMS energy spec","ISS","SIMS","SIMS energy spec","SNMS",
    "SNMS energy spec","UPS","XPS","XRF", NULL
};

const char* scans[] = {
    "REGULAR","IRREGULAR","MAPPING", NULL
};

// simple utilities

int read_line_int(istream& is)
{
    return my_strtol(read_line(is));
}

string read_line_trim(istream& is)
{
    return str_trim(read_line(is));
}

void assert_in_array(string const& val, const char** array, string const& name)
{
    for (const char** i = &array[0]; *i != NULL; ++i)
        if (strcmp(val.c_str(), *i) == 0)
            return;
    throw xylib::FormatError(name + "has an invalid value");
}

// skip "count" lines in f
void skip_lines(istream &f, int count)
{
    using namespace xylib;
    string line;
    for (int i = 0; i < count; ++i) {
        if (!getline(f, line)) {
            throw FormatError("unexpected end of file");
        }
    }
}


} // anonymous namespace


namespace xylib {


bool VamasDataSet::check(istream &f)
{
    static const string magic =
     "VAMAS Surface Chemical Analysis Standard Data Transfer Format 1988 May 4";
    string line;
    return getline(f, line) && str_trim(line) == magic;
}


// Vamas file is organized as
// file_header block_header block_data [block_header block_data] ...
// There is only a value in every line without a key or label; the meaning is
// determined by its position and values in preceding lines
void VamasDataSet::load_data(std::istream &f)
{
    int n;
    skip_lines(f, 1);   // magic number: "VAMAS Sur..."
    meta["institution identifier"] = read_line_trim(f);
    meta["institution model identifier"] = read_line_trim(f);
    meta["operator identifier"] = read_line_trim(f);
    meta["experiment identifier"] = read_line_trim(f);

    // skip comment lines, n is number of lines
    n = read_line_int(f);
    skip_lines(f, n);

    exp_mode = read_line_trim(f);
    // make sure exp_mode has a valid value
    assert_in_array(exp_mode, exps, "exp_mode");

    scan_mode = read_line_trim(f);
    // make sure scan_mode has a valid value
    assert_in_array(scan_mode, scans, "scan_mode");

    // some exp_mode specific file-scope meta-info
    if ("MAP" == exp_mode || "MAPD" == exp_mode ||
        "NORM" == exp_mode || "SDP" == exp_mode) {
        meta["number of spectral regions"] = read_line_trim(f);
    }
    if ("MAP" == exp_mode || "MAPD" == exp_mode) {
        meta["number of analysis positions"] = read_line_trim(f);
        meta["number of discrete x coordinates available in full map"]
                                                            = read_line_trim(f);
        meta["number of discrete y coordinates available in full map"]
                                                            = read_line_trim(f);
    }

    // experimental variables
    exp_var_cnt = read_line_int(f);
    for (int i = 1; i <= exp_var_cnt; ++i) {
        meta["experimental variable label " + S(i)] = read_line_trim(f);
        meta["experimental variable unit " + S(i)] = read_line_trim(f);
    }

    include = vector<bool>(40, false);
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
        Block *block = read_block(f);
        blocks.push_back(block);
    }
}


// read one block from file
Block* VamasDataSet::read_block(istream &f)
{
    Block *block = new Block;
    double x_start=0., x_step=0.;
    string x_name;

    int cor_var = 0;    // # of corresponding variables

    block->meta["block id"] = read_line_trim(f);
    block->meta["sample identifier"] = read_line_trim(f);

    if (include[0])
        block->meta["year"] = read_line_trim(f);
    if (include[1])
        block->meta["month"] = read_line_trim(f);
    if (include[2])
        block->meta["day"] = read_line_trim(f);
    if (include[3])
        block->meta["hour"] = read_line_trim(f);
    if (include[4])
        block->meta["minute"] = read_line_trim(f);
    if (include[5])
        block->meta["second"] = read_line_trim(f);
    if (include[6])
        block->meta["no. of hours in advanced GMT"] = read_line_trim(f);

    if (include[7]) {   // skip comments on this block
        int cmt_lines = read_line_int(f);
        skip_lines(f, cmt_lines);
    }

    string tech;
    if (include[8]) {
        tech = read_line_trim(f);
        block->meta["tech"] = tech;
        assert_in_array(tech, techs, "tech");
    }

    if (include[9]) {
        if ("MAP" == exp_mode || "MAPDP" == exp_mode) {
            block->meta["x coordinate"] = read_line_trim(f);
            block->meta["y coordinate"] = read_line_trim(f);
        }
    }

    if (include[10]) {
        for (int i = 0; i < exp_var_cnt; ++i) {
            block->meta["experimental variable value " + S(i)] = read_line_trim(f);
        }
    }

    if (include[11])
        block->meta["analysis source label"] = read_line_trim(f);

    if (include[12]) {
        if ("MAPDP" == exp_mode || "MAPSVDP" == exp_mode
                || "SDP" == exp_mode || "SDPSV" == exp_mode
                || "SNMS energy spec" == tech || "FABMS" == tech
                || "FABMS energy spec" == tech || "ISS" == tech
                || "SIMS" == tech || "SIMS energy spec" == tech
                || "SNMS" == tech) {
            block->meta["sputtering ion oratom atomic number"]
                                                          = read_line_trim(f);
            block->meta["number of atoms in sputtering ion or atom particle"]
                                                          = read_line_trim(f);
            block->meta["sputtering ion or atom charge sign and number"]
                                                          = read_line_trim(f);
        }
    }

    if (include[13])
        block->meta["analysis source characteristic energy"]
                                                           = read_line_trim(f);
    if (include[14])
        block->meta["analysis source strength"] = read_line_trim(f);

    if (include[15]) {
        block->meta["analysis source beam width x"] = read_line_trim(f);
        block->meta["analysis source beam width y"] = read_line_trim(f);
    }

    if (include[16]) {
        if ("MAP" == exp_mode || "MAPDP" == exp_mode || "MAPSV" == exp_mode
                || "MAPSVDP" == exp_mode || "SEM" == exp_mode) {
            block->meta["field of view x"] = read_line_trim(f);
            block->meta["field of view y"] = read_line_trim(f);
        }
    }

    if (include[17]) {
        if ("SEM" == exp_mode || "MAPSV" == exp_mode || "MAPSVDP" == exp_mode) {
            throw FormatError("unsupported MAPPING mode");
        }
    }

    if (include[18])
        block->meta["analysis source polar angle of incidence"] = read_line_trim(f);
    if (include[19])
        block->meta["analysis source azimuth"] = read_line_trim(f);
    if (include[20])
        block->meta["analyser mode"] = read_line_trim(f);
    if (include[21])
        block->meta["analyser pass energy or retard ratio or mass resolution"]
                                                            = read_line_trim(f);
    if (include[22]) {
        if ("AES diff" == tech) {
            block->meta["differential width"] = read_line_trim(f);
        }
    }

    if (include[23])
        block->meta["magnification of analyser transfer lens"] = read_line_trim(f);
    if (include[24])
        block->meta["analyser work function or acceptance energy of atom or ion"] = read_line_trim(f);
    if (include[25])
        block->meta["target bias"] = read_line_trim(f);

    if (include[26]) {
        block->meta["analysis width x"] = read_line_trim(f);
        block->meta["analysis width y"] = read_line_trim(f);
    }

    if (include[27]) {
        block->meta["analyser axis take off polar angle"] = read_line_trim(f);
        block->meta["analyser axis take off azimuth"] = read_line_trim(f);
    }

    if (include[28])
        block->meta["species label"] = read_line_trim(f);

    if (include[29]) {
        block->meta["transition or charge state label"] = read_line_trim(f);
        block->meta["charge of detected particle"] = read_line_trim(f);
    }

    if (include[30]) {
        if ("REGULAR" == scan_mode) {
            x_name = read_line_trim(f);
            block->meta["abscissa label"] = x_name;
            block->meta["abscissa units"] = read_line_trim(f);
            x_start = my_strtod(read_line(f));
            x_step = my_strtod(read_line(f));
        }
        else
            throw FormatError("Only REGULAR scans are supported now");
    }
    else
        throw FormatError("how to find abscissa properties in this file?");

    if (include[31]) {
        cor_var = read_line_int(f);
        skip_lines(f, 2 * cor_var);   // 2 lines per corresponding_var
    }

    if (include[32])
        block->meta["signal mode"] = read_line_trim(f);
    if (include[33])
        block->meta["signal collection time"] = read_line_trim(f);
    if (include[34])
        block->meta["# of scans to compile this blk"] = read_line_trim(f);
    if (include[35])
        block->meta["signal time correction"] = read_line_trim(f);

    if (include[36]) {
        if (("AES1" == tech || "AES2" == tech || "EDX" == tech || "ELS" == tech
             || "UPS" == tech || "XPS" == tech || "XRF" == tech)
            && ("MAPDP" == exp_mode || "MAPSVDP" == exp_mode
                || "SDP" == exp_mode || "SDPSV" == exp_mode)) {
            skip_lines(f, 7);
        }
    }

    if (include[37]) {
        block->meta["sample normal polar angle of tilt"] = read_line_trim(f);
        block->meta["sample normal polar tilt azimuth"] = read_line_trim(f);
    }

    if (include[38])
        block->meta["sample rotate angle"] = read_line_trim(f);

    if (include[39]) {
        unsigned n = read_line_int(f);   // # of additional numeric parameters
        for (unsigned i = 0; i < n; ++i) {
            // 3 items in every loop: param_label, param_unit, param_value
            string param_label = read_line_trim(f);
            string param_unit = read_line_trim(f);
            block->meta[param_label] = read_line_trim(f) + param_unit;
        }
    }

    skip_lines(f, blk_fue);
    int cur_blk_steps = read_line_int(f);
    skip_lines(f, 2 * cor_var);   // min & max ordinate

    StepColumn *xcol = new StepColumn(x_start, x_step);
    block->add_column(xcol, x_name);

    VecColumn *ycol = new VecColumn;
    for (int i = 0; i < cur_blk_steps; ++i) {
        double y = my_strtod(read_line_trim(f));
        ycol->add_val(y);
    }
    block->add_column(ycol);
    return block;
}

} // namespace xylib

