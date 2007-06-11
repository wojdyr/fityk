// XYlib library test program
// Import x-y data from the sample files in "test" dir
// Licence: GNU General Public License version 2
// $Id: testmain.cpp $

#include "testmain.h"
#include "common.h"

using namespace std;
using namespace xylib;


int main(int argc, char* argv[])
{
    test_xy_file();
    test_uxd_file();
    test_diffracat_v2_raw_file();

	return 0;
} 


// test files at: test/text
void test_xy_file()
{
    vector<string> fnames;
    fnames.push_back("xy_text.TXt");
    fnames.push_back("with_sigma.txt"); // sigma has not been read, modify later
    
    load_export(fnames, "text");
}


// test files at: test/uxd
void test_uxd_file()
{
    vector<string> fnames;
    for (int i=1; i<7; ++i)
        fnames.push_back(string("sample") + S(i) + ".uxd");

    fnames.push_back("synchro.uxd"); 

    load_export(fnames, "uxd");
}


// test files at: test/diffracat_v2_raw
void test_diffracat_v2_raw_file()
{
    vector<string> fnames;
    fnames.push_back("QTZQUIN.RAW"); 
    fnames.push_back("CORKLZ20.RAW"); 

    load_export(fnames, "diffracat_v2_raw");
}


// helper functions
//////////////////////////////////////////////////////////////////////////

// load and export
void load_export(const std::vector<std::string>& fnames,
                 const std::string& ftype
                 )
{
    XY_Data data;

    vector<string>::const_iterator it;
    string src_base = TEST_DIR + ftype + PATH_SEP;
    string out_base = OUT_DIR + ftype + PATH_SEP;
    string src, out;

    for (it = fnames.begin(); it != fnames.end(); ++it)
    {
        src = src_base + *it;
        out = out_base + *it + "_tr.txt";
        try
        {
            XYlib::load_file(src, data, ftype);
            data.export_xy_file(out);
        }
        catch (const runtime_error e)
        {
            cerr << e.what() << endl;
            continue;
        }
        
        cout << src << " has been exported to " << out << endl;
    }
}
