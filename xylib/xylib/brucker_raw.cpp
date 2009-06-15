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
    "Siemens/Bruker RAW ver. 1/2/3/4",
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
           || (head == "RAW1" && read_string(f, 3) == ".01"); // ver. 1.01
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

// contributed by Andreas Breslau
void BruckerRawDataSet::load_version1_01(std::istream &f)
{
    meta["format version"] = "1.01";

    // unknown values (maybe)
    float unknown_flt=0.0;

    // start at pos 4
    f.ignore(8); 				// now pos 12
    unsigned range_cnt = read_uint16_le(f);	// now pos 14

    // add file-scope meta-info
    f.ignore(2);				// now pos 16
    meta["MEASURE_DATE"] = read_string(f, 8);	// now pos 24
    f.ignore(2);				// now pos 26
    meta["MEASURE_TIME"] = read_string(f, 8);	// now pos 34
    f.ignore(74);				// now pos 108
    meta["SITE"] = read_string(f, 218);	// now pos 326
    meta["SAMPLE_ID"] = read_string(f, 59);	// now pos 385
    f.ignore(1);				// now pos 386
    meta["COMMENT"] = read_string(f,159);	// now pos 545
    f.ignore(23);				// now pos 568

    unknown_flt = read_flt_le(f);		// now pos 572
// maybe AntiScatteringSlit/DivergenceSlit/NearSampleSlit
    unknown_flt = read_flt_le(f);		// now pos 576
// same as last
    f.ignore(8);				// now pos 584

    unknown_flt = read_flt_le(f);		// now pos 588
// same as last
    unknown_flt = read_flt_le(f);		// now pos 592
// maybe DetectorSlit

    f.ignore(16);				// now pos 608
    meta["ANODE_MATERIAL"] = read_string(f,2);		// now pos 610
    f.ignore(6);				// now pos 616
    meta["ALPHA_AVERAGE"] = S(read_dbl_le(f));	// now pos 624
    meta["ALPHA1"] = S(read_dbl_le(f));		// now pos 632
    meta["ALPHA2"] = S(read_dbl_le(f));		// now pos 640
    meta["BETA"] = S(read_dbl_le(f));		// now pos 648
    meta["ALPHA_RATIO"] = S(read_dbl_le(f));	// now pos 656
    f.ignore(44);				// now pos 700

    unknown_flt =read_flt_le(f);		// now pos 704
// maybe DetectorSlit/SampleChanger

    f.ignore(8);				// now pos 712, end of global parameters

    for (unsigned cur_range = 0; cur_range < range_cnt; ++cur_range) {
        Block* blk = new Block;
        unsigned cur_header_len = read_uint16_le(f);	// now pos 714
        format_assert (cur_header_len > 0);//FIXME: what values are allowed here
	f.ignore(2);				// now pos 716
	unsigned steps = read_uint16_le(f);		// now pos 718
	blk->meta["STEPS"] = S(steps);
	f.ignore(2);				// now pos 720
	double start_theta = read_dbl_le(f);	// now pos 728
	blk->meta["START_THETA"]= S(start_theta);
	double start_2theta = read_dbl_le(f);	// now pos 736
	blk->meta["START_2THETA"] = S(start_2theta);
	f.ignore(76);				// now pos 812
	blk->meta["HIGH_VOLTAGE"] = S(read_flt_le(f));	// now pos 816
	blk->meta["AMPLIFIER_GAIN"] = S(read_flt_le(f));	// now pos 820
	blk->meta["DISCRIMINATOR_1_LOWER_LEVEL"] = S(read_flt_le(f));	// now pos 824
	f.ignore(24);				// now pos 848 ('unkn')
	f.ignore(40);				// now pos 888
	double step_size = read_dbl_le(f);		// now pos 896
	blk->meta["STEP_SIZE"] = S(step_size);
	f.ignore(8);				// now pos 904
	blk->meta["TIME_PER_STEP"] = S(read_flt_le(f));	// now pos 908
	f.ignore(12);				// now pos 920
	blk->meta["ROTATION_SPEED [rpm]"] = S(read_flt_le(f));	// now pos 924

	unknown_flt = read_flt_le(f);		// now pos 928
	unknown_flt = read_flt_le(f);		// now pos 932
	unknown_flt = read_flt_le(f);		// now pos 936

	blk->meta["GENERATOR_VOLTAGE"] = S(read_uint16_le(f));	// now pos 938
	f.ignore(2);				// now pos 940
	blk->meta["GENERATOR_CURRENT"] = S(read_uint16_le(f));	// now pos 942
	f.ignore(10);				// now pos 952
	blk->meta["USED_LAMBDA"] = S(read_dbl_le(f));	// pow pos 960
        f.ignore(56);  // move ptr to the data_start

        unsigned cur_range_steps = steps;
        float x_step = step_size;
        float x_start = start_2theta;
        StepColumn *xcol = new StepColumn(x_start, x_step);
        blk->add_column(xcol);

        VecColumn *ycol = new VecColumn;
        for(unsigned i = 0; i < cur_range_steps; ++i) {
            float y = read_flt_le(f);
            ycol->add_val(y);
        }
        blk->add_column(ycol);

        blocks.push_back(blk);
    }
}


} // end of namespace xylib

