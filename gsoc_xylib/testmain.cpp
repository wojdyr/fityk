// XYlib library test program
// Import x-y data from the sample files in "test" dir
// Licence: GNU General Public License version 2
// $Id: testmain.cpp $

#include "testmain.h"
#include "common.h"
#include <string.h>

using namespace std;
using namespace xylib;


int main(int argc, char* argv[])
{
    string in_file, out_file, ft;

    if (1 == argc)
    {
        usage(argv[0]);
        return -1;
    }

    for (int i=1; i<argc; ++i)
    {
        // auto test mode
        if (0 == strncmp("-a", argv[i], 2))
        {
            /*
            test_xy_file();
            test_uxd_file();
            test_diffracat_v2_raw_file();
            */
            test_diffracat_v1_raw_file();
            return 0;
        }
        // user specifying mode
        else
        {
            if (0 == strncmp("-i", argv[i], 2))
            {
                in_file = argv[++i];
            }
            else if (0 == strncmp("-o", argv[i], 2))
            {
                out_file = argv[++i];
            }
            else if (0 == strncmp("-t", argv[i], 2))
            {
                ft = argv[++i];
            }
            else
            {
                usage(argv[0]);
                return -1;
            }
        }
    }

    if (in_file.empty() || out_file.empty() || ft.empty())
    {
        usage(argv[0]);
        return -1;
    }

    XY_Data data;
    try
    {
        XYlib::load_file(in_file, data, ft);
        data.export_xy_file(out_file);
        const << in_file << "has been exported to " << out_file << endl;
    }
    catch (const runtime_error e)
    {
        cerr << e.what() << endl;
    }
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


// test files at: test/diffracat_v1_raw
void test_diffracat_v1_raw_file()
{
    vector<string> fnames;
    
    for (int i=2; i<7; ++i)
        fnames.push_back(string("sample") + S(i) + ".raw");

    load_export(fnames, "diffracat_v1_raw");
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


void usage(char* prog_name)
{
    cout << "usage:" << endl;
    //cout << "\t" << prog_name << "[-a] [-i in_file -o out_file -t file_type]" << endl;
    cout << "\t" << "testmain " << "[-a] [-i in_file -o out_file -t file_type]" << endl;
    cout << "\t\t-a: auto test using the existing test files." << endl;
    cout << "\t\t-i: input file path." << endl;
    cout << "\t\t-o: output file path." << endl;
    cout << "\t\t-t: format type of the input file." << endl;
}