// XYlib library test program
// Import x-y data from the sample files in "test" dir
// Licence: GNU General Public License version 2
// $Id: testmain.cpp $

#include "testmain.h"
#include "common.h"
#include <string.h>
#include <iomanip>
#if !(defined(_WIN32) || defined(WIN32) || defined(__NT__) || defined(__WIN32__))
#include <dirent.h>
#endif

using namespace std;
using namespace xylib;
 
int main(int argc,char* argv[])
{
	string in_file, out_file, ft;

	if(1 == argc){
		usage(argv[0]);
		return -1;
	}

	for(int i = 1;i < argc; ++i){
		// auto test mode
		if(0 == strncmp("-a", argv[i], 2)){
#if !(defined(_WIN32) || defined(WIN32) || defined(__NT__) || defined(__WIN32__))
		    for (int j = 1; j < FT_NUM; ++j) {
		        cout << "* subdir: " << g_ftype[j] << endl;
                test_file(static_cast<xy_ftype>(j));
		    } 
#else
            cout << "option -a not implemented in Windows!" << endl;
#endif
			return 0;
		}
		// user specifying mode
		else{
			if(0 == strncmp("-i", argv[i], 2)){
				in_file = argv[++i];
			} else if(0 == strncmp("-o", argv[i], 2)){
				out_file = argv[++i];
			} else if(0 == strncmp("-t", argv[i], 2)){
				ft = argv[++i];
			} else{
				usage(argv[0]);
				return -1;
			}
		}
	}

	if(in_file.empty() || out_file.empty()){
		usage(argv[0]);
		return -1;
	} else{
		DataSet *pds;
		if(ft.empty()){
			pds = getNewDataSet(in_file);
		} else{
			pds = getNewDataSet(in_file, string_to_ftype(ft));
		}

		pds->export_xy_file(out_file);
		cout << in_file <<  endl;
		cout << "type:" << pds->get_filetype() << endl;
		cout << "successfully exported to " << out_file << endl;

		if (pds) {
            delete pds;
            pds = NULL;
		}
	}

	return 0;
} 

#if !(defined(_WIN32) || defined(WIN32) || defined(__NT__) || defined(__WIN32__))
void test_file(xy_ftype ftype)
{

    string src_dir = S("test/") + g_ftype[ftype] + "/";
    string out_dir = S("out/") + g_ftype[ftype] + "/";

    DIR *dir = opendir(src_dir.c_str());
    if (!dir) {
        throw XY_Error("cannot open dir " + src_dir);
    }
    
    struct dirent *ent;
    vector<string> fnames;
    while((ent = readdir(dir))) {
        string fname = ent->d_name;
        if (fname != "." && 
            fname != ".." && 
            guess_file_type(src_dir + fname) != FT_UNKNOWN) {
            fnames.push_back(fname);
        }
    }

    if (closedir(dir)) {
        throw XY_Error("cannot close dir " + src_dir);
    }

    // perform the test
    vector<string>::const_iterator it;
    string src, out;
    DataSet *pds = NULL;
    for (it = fnames.begin(); it != fnames.end(); ++it) {
        src = src_dir + *it;
        out = out_dir + *it + "_tr.txt";
        cout << "processing file " << src << "..." << endl;
        
        try {
            pds = getNewDataSet(src);
            pds->export_xy_file(out);
            cout << "done!" << endl;
            if (pds) {
                delete pds;
                pds = NULL;
    		}
        } catch (const runtime_error &e) {
            cerr << e.what() << endl;
            if (pds) {
                delete pds;
                pds = NULL;
    		}
            continue;
        }
    }
}
#endif


void usage(char* prog_name)
{
    cout << "usage:" << endl;
    cout << "" << prog_name << " [-a] [-i in_file -o out_file -t file_type]" << endl;
    cout << "\t-a: auto test using the existing test files." << endl;
    cout << "\t-i: input file path." << endl;
    cout << "\t-o: output file path." << endl;
    cout << "\t-t: format type of the input file. Supported formats:" << endl;

    for (int i = 1; i < FT_NUM; ++i) {
        cout << "\t\t" << g_ftype[i] << setw(21 - g_ftype[i].size()) << ": " << g_desc[i] << endl;
    }
}

