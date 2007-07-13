// XYlib library test program
// Import x-y data from the sample files in "test" dir
// Licence: GNU General Public License version 2
// $Id: testmain.cpp $

#include "testmain.h"
#include "common.h"
#include <string.h>
#include <iomanip>

using namespace std;
using namespace xylib;


// p: a ptr to a Range or a DataSet, depending on is_range
template <typename T>
void output_meta(T *pds)
{
    if (pds->has_meta()) {
        cout << "meta-key" << "\t" << "meta_val" << endl;
        vector<string> meta_keys = pds->get_all_meta_keys();
        for (vector<string>::iterator it = meta_keys.begin(); it != meta_keys.end(); ++it) {
            cout << *it << ":\t" << pds->get_meta(*it) << endl;
        }
    }
}

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
            using namespace xylib::util;
            string ss = "  dgkey   =    good    ";
            cout << ss << endl;
            trim_space(ss);
            cout << ss << endl;
            string strk, strv;
            parse_line(ss, "=", strk, strv);
*/

            // test the meta-info access methods
            DataSet *pds = NULL;
            try {
//                pds = getNewDataSet("test/diffracat_v1_raw/sample2.raw");
//                pds = getNewDataSet("test/diffracat_v2_raw/CORKLZ20.RAW", FT_BR_RAW23);
//                pds = getNewDataSet("test/uxd/sample1.uxd", FT_UXD);
//                pds = getNewDataSet("test/text/xy_text.txt", FT_TEXT);
//                pds = getNewDataSet("test/rigaku_dat/sample5r.dat", FT_RIGAKU);
                pds = getNewDataSet("test/vamas_iso14976/pbn.vms", FT_VAMAS);

                cout << "ftype:" << pds->get_filetype() << endl;
                cout << "ftype:" << pds->get_filetype_desc() << endl;

#if 0
                pds = new BruckerV1RawDataSet("test/diffracat_v1_raw/sample2.raw");
                pds->load_data(true);
#endif
                // output the file-scope meta
                output_meta(pds);
                
                unsigned num = pds->get_range_cnt();
                cout << num << " ranges in total" << endl;
                pds->export_xy_file("./xy.txt");
                for (unsigned i = 0; i < num; ++i) {
                    const Range& range = pds->get_range(i);
                    cout << "===============================================" << endl;
                    cout << "* range " << i << endl;
                    // output the range-scope meta-info
                    output_meta(&range);
                    cout << "this range has " << range.get_pt_count() << " points.\n";
#if 0
                    for (unsigned j = 0; j < range.get_pt_count(); ++j) {
                        cout << j << " " << range.get_x(j) << " ";
                        cout << range.get_y(j) << endl;
                    }
#endif
                }
                cout << "done!" << endl;

            } catch (const runtime_error e) {
                cerr << e.what() << endl;
            }

            if (pds) {
                delete pds;
            }

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

	return 0;
} 


void usage(char* prog_name)
{
    cout << "usage:" << endl;
    //cout << "\t" << prog_name << "[-a] [-i in_file -o out_file -t file_type]" << endl;
    cout << "" << "testmain " << "[-a] [-i in_file -o out_file -t file_type]" << endl;
    cout << "\t-a: auto test using the existing test files." << endl;
    cout << "\t-i: input file path." << endl;
    cout << "\t-o: output file path." << endl;
    cout << "\t-t: format type of the input file. Supported formats:" << endl;

    cout << "\t\t" << "text             " << ": the ascii plain text format" << endl;
    cout << "\t\t" << "uxd              " << ": Siemens/Bruker Diffrac-AT UXD File" << endl;
    cout << "\t\t" << "diffracat_v1_raw " << ": Siemens/Bruker Diffrac-AT Raw File v1" << endl;
    cout << "\t\t" << "diffracat_v2_raw " << ": Siemens/Bruker Diffrac-AT Raw File v2/v3" << endl;
    cout << "\t\t" << "rigaku_dat       " << ": Rigaku dat File" << endl;
    cout << "\t\t" << "vamas_iso14976   " << 
        ": ISO14976 VAMAS Surface Chemical Analysis Standard Data Transfer Format File" << endl;
}
