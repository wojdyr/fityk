// Convert file supported by xylib to ascii format
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <string.h>

#include "xylib/xylib.h"

using namespace std;

void print_usage()
{
    cout <<
"Usage:\n"
"\txyconv [-t FILETYPE] INPUT_FILE OUTPUT_FILE\n"
"\txyconv [-t FILETYPE] -m INPUT_FILE1 ...\n"
"\txyconv -i FILETYPE\n"
"\txyconv -g INPUT_FILE\n"
"\txyconv [-l|-v|-h]\n"
"  Converts INPUT_FILE to ascii OUTPUT_FILE\n"
"  -t     specify filetype of input file\n"
"  -m     convert one or multiple files; output files have the same name\n"
"         as input, but with extension changed to .xy\n"
"  -l     list all supported file types\n"
"  -v     output version information and exit\n"
"  -h     show this help message and exit\n"
"  -i     show information about filetype\n"
"  -s     do not output metadata\n"
"  -g     guess filetype of file \n";
}

// Print version of the library. This program is too small to have own version.
void print_version()
{
    cout << XYLIB_VERSION / 10000 << "."
         << XYLIB_VERSION / 100 % 100 << "."
         << XYLIB_VERSION % 100 << endl;
}

void list_supported_formats()
{
    const xylib::FormatInfo* format = NULL;
    for (int i = 0; (format = xylib::get_format(i)) != NULL; ++i)
        cout << setw(20) << left << format->name << ": "
             << format->desc << endl;
}

int print_guessed_filetype(string const& path)
{
    try {
        ifstream is(path.c_str());
        if (!is) {
            cerr << "Error: can't open input file: " << path;
            return -1;
        }
        xylib::FormatInfo const* fi = xylib::guess_filetype(path, is);
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
    if (with_metadata) {
        export_metadata(of, d->meta);
        of << endl;
    }

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


int convert_file(string const& input, string const& output,
                 string const& filetype, bool with_metadata)
{
    try {
        xylib::DataSet *d = xylib::load_file(input, filetype);
        export_plain_text(d, output, with_metadata);
        delete d;
    } catch (runtime_error const& e) {
        cerr << "Error. " << e.what() << endl;
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    // options -l -h -i -g -v are not combined with other options

    if (argc == 2 && strcmp(argv[1], "-l") == 0) {
        list_supported_formats();
        return 0;
    }
    else if (argc == 2 &&
            (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage();
        return 0;
    }
    else if (argc == 2 &&
            (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)) {
        print_version();
        return 0;
    }
    else if (argc == 3 && strcmp(argv[1], "-i") == 0) {
        print_filetype_info(argv[2]);
        return 0;
    }
    else if (argc == 3 && strcmp(argv[1], "-g") == 0)
        return print_guessed_filetype(argv[2]);
    else if (argc < 3) {
        print_usage();
        return -1;
    }

    string filetype;
    bool option_m = false;
    bool option_s = false;
    int n = 1;
    while (n < argc - 1) {
        if (strcmp(argv[n], "-m") == 0) {
            option_m = true;
            ++n;
        }
        else if (strcmp(argv[n], "-s") == 0) {
            option_s = true;
            ++n;
        }
        else if (strcmp(argv[n], "-t") == 0 && n+1 < argc - 1) {
            filetype = argv[n+1];
            n += 2;
        }
        else
            break;
    }
    if (!option_m && n != argc - 2) {
        print_usage();
        return -1;
    }
    if (option_m) {
        for ( ; n < argc; ++n) {
            string out = argv[n];
            size_t p = out.rfind('.');
            if (p != string::npos)
                out.erase(p);
            out += ".xy";
            cout << "converting " << argv[n] << " to " << out << endl;
            convert_file(argv[n], out, filetype, !option_s);
        }
        return 0;
    }
    else
        return convert_file(argv[argc-2], argv[argc-1], filetype, !option_s);
}

