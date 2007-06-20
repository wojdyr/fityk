// XYlib library test program
// Import x-y data from the sample files in "test" dir
// Licence: GNU General Public License version 2
// $Id: testmain.h $

#include "xylib.h"
#include <vector>
#include <iostream>
#include <cstdlib>

const std::string PATH_SEP = "/";
const std::string TEST_DIR = "test" + PATH_SEP;
const std::string OUT_DIR = "out" + PATH_SEP;

void load_export(const std::vector<std::string>& fnames,
                 const std::string& ftype
                 );

void test_xy_file();
void test_uxd_file();
void test_diffracat_v1_raw_file();
void test_diffracat_v2_raw_file();
void test_rigaku_dat_file();
void test_vamas_file();

void usage(char* prog_name);
