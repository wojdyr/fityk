// Siemens/Bruker Diffrac-AT Raw Format version 1/2/3
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

#include "brucker_raw.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo BruckerRawDataSet::fmt_info(
    "bruker_raw",
    "Siemens/Bruker RAW",
    vector<string>(1, "raw"),
    true,                       // whether binary
    true,                       // whether has multi-blocks
    &BruckerRawDataSet::ctor,
    &BruckerRawDataSet::check
);


bool BruckerRawDataSet::check(istream &f)
{
    string head = read_string(f, 4);
    return head == "RAW " // ver. 1
           || head == "RAW2" // ver. 2
           || (head == "RAW1" && read_string(f, 3) == ".01"); // ver. 3
}


void BruckerRawDataSet::load_data(std::istream &f)
{
    string head = read_string(f, 4);
    format_assert(head == "RAW " || head == "RAW2" || head == "RAW1");
    if (head[3] == ' ')
        load_version1(f);
    else if (head[3] == '2')
        load_version2(f);
    else // head[3] == '1'
        load_version1_01(f);
}

void BruckerRawDataSet::load_version1(std::istream &f)
{
    meta["format version"] = "1";

    unsigned following_range = 1;

    while (following_range > 0) {
        Block* blk = new Block;

        unsigned cur_range_steps = read_uint32_le(f);
        // early DIFFRAC-AT raw data files didn't repeat the "RAW "
        // on additional ranges
        // (and if it's the first block, 4 bytes from file were already read)
        if (!blocks.empty()) {
            istringstream raw_stream("RAW ");
            unsigned raw_int = read_uint32_le(raw_stream);
            if (cur_range_steps == raw_int)
                cur_range_steps = read_uint32_le(f);
        }

        blk->meta["MEASUREMENT_TIME_PER_STEP"] = S(read_flt_le(f));
        float x_step = read_flt_le(f);
        blk->meta["SCAN_MODE"] = S(read_uint32_le(f));
        f.ignore(4);
        float x_start = read_flt_le(f);

        StepColumn *xcol = new StepColumn(x_start, x_step);
        blk->add_column(xcol);

        float t = read_flt_le(f);
        if (-1e6 != t)
            blk->meta["THETA_START"] = S(t);

        t = read_flt_le(f);
        if (-1e6 != t)
            blk->meta["KHI_START"] = S(t);

        t = read_flt_le(f);
        if (-1e6 != t)
            blk->meta["PHI_START"], S(t);

        blk->meta["SAMPLE_NAME"] = read_string(f, 32);
        blk->meta["K_ALPHA1"] = S(read_flt_le(f));
        blk->meta["K_ALPHA2"] = S(read_flt_le(f));

        f.ignore(72);   // unused fields
        following_range = read_uint32_le(f);

        VecColumn *ycol = new VecColumn();

        for(unsigned i = 0; i < cur_range_steps; ++i) {
            float y = read_flt_le(f);
            ycol->add_val(y);
        }
        blk->add_column(ycol);

        blocks.push_back(blk);
    }
}

void BruckerRawDataSet::load_version2(std::istream &f)
{
    meta["format version"] = "2";

    unsigned range_cnt = read_uint16_le(f);

    // add file-scope meta-info
    f.ignore(162);
    meta["DATE_TIME_MEASURE"] = read_string(f, 20);
    meta["CEMICAL SYMBOL FOR TUBE ANODE"] = read_string(f, 2);
    meta["LAMDA1"] = S(read_flt_le(f));
    meta["LAMDA2"] = S(read_flt_le(f));
    meta["INTENSITY_RATIO"] = S(read_flt_le(f));
    f.ignore(8);
    meta["TOTAL_SAMPLE_RUNTIME_IN_SEC"] = S(read_flt_le(f));

    f.ignore(42);   // move ptr to the start of 1st block
    for (unsigned cur_range = 0; cur_range < range_cnt; ++cur_range) {
        Block* blk = new Block;

        // add the block-scope meta-info
        unsigned cur_header_len = read_uint16_le(f);
        format_assert (cur_header_len > 48);

        unsigned cur_range_steps = read_uint16_le(f);
        f.ignore(4);
        blk->meta["SEC_PER_STEP"] = S(read_flt_le(f));

        float x_step = read_flt_le(f);
        float x_start = read_flt_le(f);
        StepColumn *xcol = new StepColumn(x_start, x_step);
        blk->add_column(xcol);

        f.ignore(26);
        blk->meta["TEMP_IN_K"] = S(read_uint16_le(f));

        f.ignore(cur_header_len - 48);  // move ptr to the data_start
        VecColumn *ycol = new VecColumn;
        for(unsigned i = 0; i < cur_range_steps; ++i) {
            float y = read_flt_le(f);
            ycol->add_val(y);
        }
        blk->add_column(ycol);

        blocks.push_back(blk);
    }
}

// Contributed by Andreas Breslau.
// Changed by Marcin Wojdyr, based on the file structure specification:
//  DIFFRAC^plus XCH V.5.0 Release 2002. USER'S MANUAL. APPENDIX A.
void BruckerRawDataSet::load_version1_01(std::istream &f)
{
    meta["format version"] = "3";

    // file header - 712 bytes
    // the offset is already 4
    f.ignore(4); // ignore bytes 4-7
    int file_status = read_uint32_le(f);        // address 8 
    if (file_status == 1)
        meta["file status"] = "done";
    else if (file_status == 2)
        meta["file status"] = "active";
    else if (file_status == 3)
        meta["file status"] = "aborted";
    else if (file_status == 4)
        meta["file status"] = "interrupted";
    int range_cnt = read_uint32_le(f);          // address 12

    meta["MEASURE_DATE"] = read_string(f, 10);  // address 16
    meta["MEASURE_TIME"] = read_string(f, 10);  // address 26
    meta["USER"] = read_string(f, 72);          // address 36
    meta["SITE"] = read_string(f, 218);         // address 108
    meta["SAMPLE_ID"] = read_string(f, 60);     // address 326
    meta["COMMENT"] = read_string(f,160);       // address 386
    f.ignore(2); // apparently there is a bug in docs, 386+160 != 548
    f.ignore(4); // goniometer code             // address 548
    f.ignore(4); // goniometer stage code       // address 552
    f.ignore(4); // sample loader code          // address 556
    f.ignore(4); // goniometer controller code  // address 560
    f.ignore(4); // (R4) goniometer radius      // address 564
    f.ignore(4); // (R4) fixed divergence...    // address 568
    f.ignore(4); // (R4) fixed sample slit...   // address 572
    f.ignore(4); // primary Soller slit         // address 576
    f.ignore(4); // primary monochromator       // address 580
    f.ignore(4); // (R4) fixed antiscatter...   // address 584
    f.ignore(4); // (R4) fixed detector slit... // address 588
    f.ignore(4); // secondary Soller slit       // address 592
    f.ignore(4); // fixed thin film attachment  // address 596
    f.ignore(4); // beta filter                 // address 600
    f.ignore(4); // secondary monochromator     // address 604
    meta["ANODE_MATERIAL"] = read_string(f,4);  // address 608
    f.ignore(4); // unused                      // address 612
    meta["ALPHA_AVERAGE"] = S(read_dbl_le(f));  // address 616
    meta["ALPHA1"] = S(read_dbl_le(f));         // address 624
    meta["ALPHA2"] = S(read_dbl_le(f));         // address 632
    meta["BETA"] = S(read_dbl_le(f));           // address 640
    meta["ALPHA_RATIO"] = S(read_dbl_le(f));    // address 648
    f.ignore(4); // (C4) unit name              // address 656
    f.ignore(4); // (R4) intensity beta:a1      // address 660
    meta["measurement time"] = S(read_flt_le(f)); // address 664
    f.ignore(43); // unused                     // address 668
    f.ignore(1); // hardware dependency ...     // address 711
    //assert(f.tellg() == 712);

    // range header
    for (int cur_range = 0; cur_range < range_cnt; ++cur_range) {
        Block* blk = new Block;
        int header_len = read_uint32_le(f);     // address 0
        format_assert (header_len == 304);
        int steps = read_uint32_le(f);          // address 4
        blk->meta["STEPS"] = S(steps);
        double start_theta = read_dbl_le(f);    // address 8
        blk->meta["START_THETA"]= S(start_theta);
        double start_2theta = read_dbl_le(f);   // address 16
        blk->meta["START_2THETA"] = S(start_2theta);

        f.ignore(8); // Chi drive start         // address 24
        f.ignore(8); // Phi drive start         // address 32
        f.ignore(8); // x drive start           // address 40
        f.ignore(8); // y drive start           // address 48
        f.ignore(8); // z drive start           // address 56
        f.ignore(8);                            // address 64
        f.ignore(6);                            // address 72
        f.ignore(2); // unused                  // address 78
        f.ignore(8); // (R8) variable antiscat. // address 80
        f.ignore(6);                            // address 88
        f.ignore(2); // unused                  // address 94
        f.ignore(4); // detector code           // address 96
        blk->meta["HIGH_VOLTAGE"] = S(read_flt_le(f)); // address 100
        blk->meta["AMPLIFIER_GAIN"] = S(read_flt_le(f)); // 104
        blk->meta["DISCRIMINATOR_1_LOWER_LEVEL"] = S(read_flt_le(f)); // 108
        f.ignore(4);                            // address 112
        f.ignore(4);                            // address 116
        f.ignore(8);                            // address 120
        f.ignore(4);                            // address 128
        f.ignore(4);                            // address 132
        f.ignore(5);                            // address 136
        f.ignore(3); // unused                  // address 141
        f.ignore(8);                            // address 144
        f.ignore(8);                            // address 152
        f.ignore(8);                            // address 160
        f.ignore(4);                            // address 168
        f.ignore(4); // unused                  // address 172
        double step_size = read_dbl_le(f);      // address 176
        blk->meta["STEP_SIZE"] = S(step_size);
        f.ignore(8);                            // address 184
        blk->meta["TIME_PER_STEP"] = S(read_flt_le(f)); // 192
        f.ignore(4);                            // address 196
        f.ignore(4);                            // address 200
        f.ignore(4);                            // address 204
        blk->meta["ROTATION_SPEED [rpm]"] = S(read_flt_le(f));  // 208
        f.ignore(4);                            // address 212
        f.ignore(4);                            // address 216
        f.ignore(4);                            // address 220
        blk->meta["GENERATOR_VOLTAGE"] = S(read_uint32_le(f)); // 224
        blk->meta["GENERATOR_CURRENT"] = S(read_uint32_le(f)); // 228
        f.ignore(4);                            // address 232
        f.ignore(4); // unused                  // address 236
        blk->meta["USED_LAMBDA"] = S(read_dbl_le(f)); // 240
        f.ignore(4);                            // address 248
        f.ignore(4);                            // address 252
        int supplementary_headers_size = read_uint32_le(f); // address 256
        f.ignore(4);                            // address 260
        f.ignore(4);                            // address 264
        f.ignore(4);  // unused                 // address 268
        f.ignore(8);                            // address 272
        f.ignore(24); // unused                 // address 280
        //assert(f.tellg() == 712 + (cur_range + 1) * header_len);

        if (supplementary_headers_size > 0)
            f.ignore(supplementary_headers_size);

        StepColumn *xcol = new StepColumn(start_2theta, step_size);
        blk->add_column(xcol);

        VecColumn *ycol = new VecColumn;
        for (int i = 0; i < steps; ++i) {
            float y = read_flt_le(f);
            ycol->add_val(y);
        }
        blk->add_column(ycol);

        blocks.push_back(blk);
    }
}


} // end of namespace xylib

