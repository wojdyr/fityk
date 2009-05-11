// Convert file supported by xylib to ascii format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

#include <iostream>
#include <iomanip>
#include <string>
#include <string.h>

#include "xylib/xylib.h"

using namespace std;
 
void print_usage()
{
    cout << "Usage:\n" 
            "\txyconv [-t FILETYPE] INPUT_FILE OUTPUT_FILE\n"
            "\txyconv -l\n"
            "\txyconv -i FILETYPE\n"
            "\txyconv -g INPUT_FILE\n"
            "  Converts INPUT_FILE to ascii OUTPUT_FILE\n"
            "  -t     specify filetype of input file\n"
            "  -l     list all supported file types\n"
            "  -i     show information about filetype\n"
            "  -g     guess filetype of file \n";
}

void list_supported_formats()
{
    for (int i = 0; xylib::formats[i] != NULL; ++i) 
        cout << setw(20) << left << xylib::formats[i]->name << ": "
             << xylib::formats[i]->desc << endl;
}

int print_guessed_filetype(string const& path)
{
    try {
        xylib::FormatInfo const* fi = xylib::guess_filetype(path);
        if (fi)
            cout << fi->name << ": " << fi->desc << endl;
        else
            cout << "Format of the file was not detected";
        return 0;
    } catch (runtime_error const& e) {
        cerr << "Error: " << e.what() << endl;
        return -1;
    }
}

void print_filetype_info(string const& filetype)
{
        xylib::FormatInfo const* fi = xylib::string_to_format(filetype);
        if (fi) {
            cout << "Name: " << fi->name << endl;
            cout << "Description: " << fi->desc << endl;
            cout << "Possible extensions:";
            for (size_t i = 0; i != fi->exts.size(); ++i)
                cout << " " << fi->exts[i];
            if (fi->exts.empty())
                cout << " (not specified)";
            cout << endl;
            cout << "Other flags: "
                << (fi->binary ? "binary-file" : "text-file") << " "
                << (fi->multiblock ? "multi-block" : "single-block") << endl;
        }
        else
            cout << "Unknown file format." << endl;
}

void export_metadata(ostream &of, xylib::MetaData const& meta)
{
    for (map<string,string>::const_iterator i = meta.begin();
                                                        i != meta.end(); ++i) {
        of << "# " << i->first << ": "; 
        for (string::const_iterator j = i->second.begin(); 
                                                   j != i->second.end(); ++j) {
            of << *j;
            if (*j == '\n')
                of << "# " << i->first << ": "; 
        }
        of << endl;
    }
}

void export_plain_text(xylib::DataSet const *d, string const &fname, 
                       bool with_metadata)
{
    int range_num = d->get_block_count();
    ofstream of(fname.c_str());
    if (!of) 
        throw xylib::RunTimeError("can't create file: " + fname);

    // output the file-level meta-info
    of << "# exported by xylib from a " << d->fi->name << " file" << endl;
    if (with_metadata)
        export_metadata(of, d->meta);
    
    for (int i = 0; i < range_num; ++i) {
        const xylib::Block *block = d->get_block(i);
        if (range_num > 1 || !block->name.empty())
            of << endl << "### block #" << i << " " << block->name << endl;
        if (with_metadata)
            export_metadata(of, block->meta);

        int ncol = block->get_column_count();
        of << "# ";
        // column 0 is pseudo-column with point indices, we skip it
        for (int k = 1; k <= ncol; ++k) {
            string const& name = block->get_column(k).name;
            if (k > 1)
                of << "\t";
            if (name.empty())
                of << "column_" << k;
            else
                of << name;
        }
        of << endl;

        int nrow = block->get_point_count();

        for (int j = 0; j < nrow; ++j) {
            for (int k = 1; k <= ncol; ++k) {
                if (k > 1)
                    of << "\t";
                of << setfill(' ') << setiosflags(ios::fixed) 
                    << setprecision(6) << setw(8) 
                    << block->get_column(k).get_value(j); 
            }
            of << endl;
        }
    }
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "-l") == 0) {
        list_supported_formats();
        return 0;
    }

    if (argc == 2 && 
            (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage();
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "-i") == 0) {
        print_filetype_info(argv[2]);
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "-g") == 0) 
        return print_guessed_filetype(argv[2]);

    if (argc < 3) {
        print_usage();
        return -1;
    }

    string filetype;
    for (int i = 1; i < argc - 2; ++i) {
        if (strcmp(argv[i], "-t") == 0 && i+1 < argc - 2) {
            ++i;
            filetype = argv[i];
        }
        else {
            print_usage();
            return -1;
        }
    }

    try {
        xylib::DataSet *d = xylib::load_file(argv[argc-2], filetype);
        export_plain_text(d, argv[argc-1], true);
        delete d;
    } catch (runtime_error const& e) {
        cerr << "Error. " << e.what() << endl;
        return -1;
    }
    return 0;
}

