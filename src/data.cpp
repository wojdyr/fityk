// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "data.h" 
#include "fileroutines.h"
#include "ui.h"
#include "numfuncs.h"

#include <math.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

// filename utils
#if defined(_WIN32) || defined(WIN32) || defined(__NT__) || defined(__WIN32__) || defined(__OS2__)
#define FILE_SEP_PATH '\\'
#elif defined(__MAC__) || defined(__APPLE__) || defined(macintosh)
#define FILE_SEP_PATH ':'
#else
#define FILE_SEP_PATH '/'
#endif

string get_file_basename(const string &path)
{
    string::size_type last_sep = path.rfind(FILE_SEP_PATH);
    string::size_type last_dot = path.rfind('.');
    size_t basename_begin = (last_sep == string::npos ? 0 : last_sep + 1);
    if (last_dot != string::npos && last_dot > basename_begin)
        return string(path, basename_begin, last_dot-basename_begin);
    else
        return string(path, basename_begin);
}


Data *my_data;

Data::Data ()
    : d_was_changed(true) {}

string Data::getInfo () const
{
    if (filename.empty()) 
        return "No file loaded.";
    else {
        string s;
        s = "Loaded " + S(p.size()) + " points.\n"
             + S(active_p.size()) + " active now. Filename: " + filename;
        if (!title.empty())
            s += "Data title: " + title;
        return s;
    }
}

double Data::pdp11_f (char* fl)  //   function that converts:
{                          //   single precision 32-bit floating point
                           //   DEC PDP-11 format
                           //   to double
    int znak = *(fl+1) & 0x80;
    int unbiased = ((*(fl+1)&0x7F)<<1) + (((*fl)&0x80)>>7) -128 ;
    if (unbiased == -128)
        return (0);
    double h = ( (*(fl+2)&0x7F)/256./256./256. +
            (*(fl+3)&0x7F)/256./256.  +
            (128+((*fl)&0x7F))/256. );
    return (znak==0 ? 1. : -1.) * h * pow(2., (double)unbiased);
}

char Data::guess_file_type (const string& filename)
{ //using only filename extension
    if (filename.size() > 4) {
        string file_ext = filename.substr (filename.size() - 4, 4);
        if (file_ext == ".mca"   || file_ext == ".MCA") 
            return 'm';
        else if (file_ext == ".rit" || file_ext == ".RIT")
            return 'r';
        else if (file_ext == ".cpi" || file_ext == ".CPI")
            return 'c';
        else if (file_ext == ".raw" || file_ext == ".RAW")
            return 's';
        else 
            return 'd';
    }
    else 
        return 'd';
}

void Data::clear()
{
    d_was_changed = true;
    //gcc2.95 has no `basic_string<...>::clear ()'
    filename = "";   //removing previos file
    title = "";
    p.clear();
    active_p.clear();
    col_nums.clear();
    every.clear();
    every_idx = 1;
    merging = 0;
    merge_table.clear();
}

void Data::post_load()
{
    change_range (-INF, +INF, true);
    if (!p[0].sigma)
        change_sigma('r');
    if (title.empty())
        title = get_file_basename(filename);
    recompute_y_bounds();
}

void Data::recompute_y_bounds() {
    y_min = y_max = p.front().y;
    for (vector<Point>::iterator i = p.begin(); i != p.end(); i++) {
        if (i->y < y_min)
            y_min = i->y;
        if (i->y > y_max)
            y_max = i->y;
    }
}

int Data::load_arrays(const vector<fp> &x, const vector<fp> &y, 
                      const vector<fp> &sigma, const string &data_title)
{
    size_t size = y.size();
    assert(y.size() == size);
    assert(sigma.empty() || sigma.size() == size);
    clear();
    filename = "-";
    title = data_title;
    if (sigma.empty()) 
        for (size_t i = 0; i < size; ++i)
            add_point (Point (x[i], y[i]));
    else
        for (size_t i = 0; i < size; ++i)
            add_point (Point (x[i], y[i], sigma[i]));
    sort(p.begin(), p.end());
    x_step = find_step();
    post_load();
    return p.size();
}


int Data::load_file (const string &file, int type, 
                     vector<int> col, vector<int> evr, int merge)
{ 
    if (type == 0) {                  // "detect" file format
        type = guess_file_type(file);
    }
    ifstream f (file.c_str(), ios::in | ios::binary);
    if (!f) {
        warn ("Can't open file: " + file );
        return -1;
    }
    clear(); //removing previous file
    filename = file;   
    col_nums = col;
    every = evr;
    merging = merge;

    if (type=='d')                            // x y x y ... 
        load_xy_filetype(f, col);
    else if (type=='m')                       // .mca
        load_mca_filetype(f);
    else if (type=='r')                       // .rit
        load_rit_filetype(f);
    else if (type=='c')                       // .rit
        load_cpi_filetype(f);
    else if (type=='s')                       // .raw
        load_siemensbruker_filetype(filename, this);
    else {                                  // other ?
        warn ("Unknown filetype.");
        return -1;
    }
    if (p.size() < 5)
        warn ("Only " + S(p.size()) + " data points found in file.");
    if (!f.eof() && type != 'm') //!=mca; .mca doesn't have to reach EOF
        warn ("Unexpected char when reading " + S (p.size() + 1) + ". point");
    if (p.empty())
        return 0;

    post_load();
    return p.size();
}

void Data::add_point (const Point& pt)
{
    if (every.size() == 0)
        add_point_2nd_stage (pt);
    else if (every.size() == 2) {
        if (every[0] <= every_idx && every_idx <= every[1])
            add_point_2nd_stage (pt);
        ++every_idx;
    }
    else if (every.size() == 3) {
        if (every[0] <= every_idx && every_idx <= every[1])
            add_point_2nd_stage (pt);
        ++every_idx;
        if (every_idx > every[2])
            every_idx -= every[2];
    }
    else
        assert(0);
}

void Data::add_point_2nd_stage (const Point& pt)
{
    if (!merging)
        p.push_back (pt);
    else {
        merge_table.push_back(pt);
        if (size(merge_table) == abs(merging)) {
            fp x_sum = 0, y_sum = 0;
            for (vector<Point>::const_iterator i = merge_table.begin();
                 i != merge_table.end(); ++i) {
                x_sum += i->x;
                y_sum += i->y;
            }
            merge_table.clear();
            //merging > 0 --> y_sum; merging < 0 --> y_avg
            p.push_back (Point (x_sum / abs(merging),
                                merging > 0 ? y_sum : y_sum / (-merging)));
        }
    }
}

void Data::load_xy_filetype (ifstream& f, vector<int>& usn)
{
    /* format  x y \n x y \n x y \n ...
    *           38.834110      361
    *           38.872800  ,   318
    *           38.911500      352.431
    * delimiters: white spaces and  , : ;
     */
    assert (usn.empty() || usn.size() == 2 || usn.size() == 3);
    if (usn.empty()) {
        usn.push_back(1);
        usn.push_back(2);
    }
    vector<fp> xy;
    int maxc = *max_element (usn.begin(), usn.end());
    int minc = *min_element (usn.begin(), usn.end());
    if (minc < 1) {
        warn ("Invalid column number: " + S(minc) 
                + ". (First column is 1, column number has to be positive)");
        return;
    }
    int not_enough_cols = 0, non_data_lines = 0, non_data_blocks = 0;
    bool prev_empty = false;
    //TODO: optimize loop below
    //most of time (?) is spent in getline() in get_one_line_with_numbers()
    while (get_one_line_with_numbers(f, xy)) {
        if (xy.empty()) {
            non_data_lines++;
            if (!prev_empty) {
                non_data_blocks++;
                prev_empty = true;
            }
            continue;
        }
        else 
            prev_empty = false;

        if (size(xy) < maxc) {
            not_enough_cols++;
            continue;
        }

        fp x = xy[usn[0] - 1];
        fp y = xy[usn[1] - 1];
        if (usn.size() == 2)
            add_point (Point (x, y));
        else {// usn.size() == 3
            fp sig = xy[usn[2] - 1];
            if (sig <= 0) 
                warn ("Point " + S(p.size()) + " has sigma = " + S(sig) 
                        + ". Point canceled.");
            else
                add_point (Point (x, y, sig));
        }
    }
    if (non_data_lines > 0)
        info (S(non_data_lines) +" (not empty and not `#...') non-data lines "
                "in " + S(non_data_blocks) + " blocks.");
    if (not_enough_cols > 0)
        warn ("Less than " + S(maxc) + " numbers in " + S(not_enough_cols) 
                + " lines.");
    if (usn.size() == 3) {
        sigma_type = 'f';
        info ("Std. dev. read from file.");
    }
    sort(p.begin(), p.end());
    x_step = find_step();
}

void Data::load_mca_filetype (ifstream& f) 
{
    typedef unsigned short int ui2b;
    char all_data [9216];//2*512+2048*4];
    f.read (all_data, sizeof(all_data));
    if (f.gcount() != static_cast<int>(sizeof(all_data)) 
            || *reinterpret_cast<ui2b*>(all_data) !=0 
            || *reinterpret_cast<ui2b*>(all_data+34) != 4
            || *reinterpret_cast<ui2b*>(all_data+36) != 2048
            || *reinterpret_cast<ui2b*>(all_data+38) != 1) {
        warn ("file format different than expected: "+ filename);
        return;
    }

    double energy_offset = pdp11_f (all_data + 108);
    double energy_slope = pdp11_f (all_data + 112);
    double energy_quadr = pdp11_f (all_data + 116);
    p.clear();
    ui2b* pw = reinterpret_cast<ui2b*>(all_data + 
                                *reinterpret_cast<ui2b*>(all_data+24));
    for (int i = 1; i <= 2048; i++, pw += 2){ //FIXME should it be from 1 ?
                        // perhaps from 0 to 2047, description was not clear.
        fp x = energy_offset + energy_slope * i + energy_quadr * i * i;
        fp y = *pw * 65536 + *(pw+1);
        add_point (Point (x, y));
    }
    x_step = energy_quadr ? 0 : energy_slope;
}

void Data::load_cpi_filetype (ifstream& f) 
{
   /* format example:
        SIETRONICS XRD SCAN
        10.00
        155.00
        0.010
        Cu
        1.54056
        1-1-1900
        0.600
        HH117 CaO:Nb2O5 neutron batch .0
        SCANDATA
        8992
        9077
        9017
        9018
        9129
        9057
        ...
    */
    string header = "SIETRONICS XRD SCAN";
    string start_flag = "SCANDATA";
    string s;
    getline (f, s);
    if (s.substr(0, header.size()).compare (header) != 0){
        warn ("No \"" + header + "\" header found.");
        return;
    }
    getline (f, s);//xmin
    fp xmin = strtod (s.c_str(), 0);
    getline (f, s); //xmax
    getline (f, s); //xstep
    x_step = strtod (s.c_str(), 0); 
    //the rest of header
    while (s.substr(0, start_flag.size()).compare(start_flag) != 0)
        getline (f, s);
    //data
    while (getline(f, s)) {
        fp y = strtod (s.c_str(), 0);
        fp x = xmin + p.size() * x_step;
        add_point (Point (x, y));
    }
}

void Data::load_rit_filetype (ifstream& f) 
{
   /* format example:
    *    10.0000   .0200150 foobar
    *         0.      2.    132.     84.     92.    182.     86.
    *       240.    306.    588.    639.    697.    764.    840.
    *           
    * delimiters: white spaces and  , : ;
    *  above xmin=10.0 and step .02, so data points:
    *  (10.00, 0), (10.02, 2), (10.04, 132), ...
    *  !! if second number has dot '.', reading max. 4 digits after dots,
    *  because in example above, in .0200150 means 0.0200 150.0 
    */

    vector<fp> num;
    bool r = get_one_line_with_numbers(f, num);
    if (!r || num.size() < 2 ){
        warn ("Bad format in \"header\" of .rit file");
        return;
    }
    fp xmin = num[0];
    x_step = static_cast<int>(num[1] * 1e4) / 1e4; //only 4 digits after '.'
    vector<fp> ys;
    while (get_one_line_with_numbers(f, ys)) {
        if (ys.size() == 0)
            warn ("Error when trying to read " + S (p.size() + 1) + ". point. " 
                    "Ignoring line.");
        for (vector<fp>::iterator i = ys.begin(); i != ys.end(); i++) {
            fp x = xmin + p.size() * x_step;
            add_point (Point (x, *i));
        }
    }
}

void Data::change_sigma(char type, fp minim) 
{
    //changing sigma for all points. Not only active points.
    if (is_empty()) {
        warn("No points loaded.");
        return;
    }
    if (minim <= 0.) minim = 1.;
    switch(type){
        case 'u':
        case 'r':
            sigma_type = type;
            sigma_minim = minim;
            recompute_sigma();
            break;
        case 'f':
            warn ("Assigning std. dev. from file is possible only when "
                    "loading data. Canceled");
            break;
        default:
            warn("Unknown standard deviation symbol.");
    }
    info (print_sigma());
}

void Data::recompute_sigma()
{
    fp sigma_minim_2 = sigma_minim * sigma_minim;
    switch (sigma_type) {
        case 'u':
            for (vector<Point>::iterator i = p.begin(); i < p.end(); i++)
                i->sigma = sigma_minim;
            break;
        case 'r':
            for (vector<Point>::iterator i = p.begin(); i < p.end(); i++) {
                fp y = i->y;
                i->sigma = y > sigma_minim_2 ? sqrt (y) : sigma_minim;
            }
            break;
    }
}

string Data::print_sigma()
{
    if (is_empty()) {
        return "No points loaded.";
    }
    switch(sigma_type){
        case 'u':
            return "Standard deviations assumed equal " + S(sigma_minim) 
                + " for all points.";
        case 'r':
            return "Standard deviation assumed as square root of value "
                "(not less then " + S(sigma_minim) + ")";
        case 'f':
            return "Standard deviation loaded from file.";
        default:
            return "! Unknown standard deviation symbol. Ooops.";
    }
}

fp Data::get_y_at (fp x) const
{
    int n = get_upper_bound_ac (x);
    if (n > size(active_p) || n <= 0)
        return 0;
    fp y1 = get_y (n - 1);
    fp y2 = get_y (n);
    fp x1 = get_x (n - 1);
    fp x2 = get_x (n);
    return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
}

int Data::change_range (fp left, fp right, bool state) 
    // pre: p.x sorted
    // post: active_p sorted
    // returns number of active points
{
    if (is_empty()) {
        warn("No points loaded.");
        return -1;
    }
    vector<Point>::iterator l = lower_bound (p.begin(), p.end(), Point(left));
    vector<Point>::iterator r = upper_bound (p.begin(), p.end(), Point(right));
    if (l > r)
        return 0;
    d_was_changed = true;
    for (vector<Point>::iterator i = l; i < r; i++)
        i->is_active = state;
    active_p.clear();
    for (unsigned int i = 0; i < p.size(); i++)
        if (p[i].is_active) 
            active_p.push_back (i);
    return active_p.size();
}

int Data::auto_range (fp y_level, fp x_margin)
{
    // pre: p.x sorted
    if (p.empty()) {
        warn("No points loaded.");
        return -1;
    }
    d_was_changed = true;
    bool state = (p.begin()->y >= y_level);
    for (vector<Point>::iterator i = p.begin(); i < p.end(); i++) {
        if (state == (i->y >= y_level)) 
            i->is_active = state;
        else if (state && i->y < y_level) {
            i->is_active = false;
            vector<Point>::iterator e = lower_bound (p.begin(), p.end(),
                                                        Point(i->x + x_margin));
            for (/*`i' can be changed*/; i < e; i++)
                i->is_active = true;
            state = false;
        }
        else { // !state && i->y >= y_level
            vector<Point>::iterator b =  lower_bound(p.begin(), p.end(), 
                                                        Point(i->x - x_margin));
            for (vector<Point>::iterator j = b; j <= i; j++)
                j->is_active = true;
            state = true;
        }
    }
    active_p.clear();
    for (unsigned int i = 0; i < p.size(); i++)
        if (p[i].is_active) 
            active_p.push_back (i);
    return active_p.size();
}

string Data::range_as_string () const 
{
    if (active_p.empty()) {
        warn ("File not loaded or all points inactive.");
        return "[]";
    }
    vector<Point>::const_iterator old_p = p.begin() + active_p[0];
    fp left =  old_p->x;
    string s = "[" + S (left) + " : ";
    for (vector<int>::const_iterator i = active_p.begin() + 1; 
                                                    i != active_p.end(); i++) {
        if (p.begin() + *i != old_p + 1) {
            fp right = old_p->x;
            left = p[*i].x;
            s += S(right) + "], + [" + S(left) + " : ";
        }
        old_p = p.begin() + *i;
    }
    fp right = old_p->x;
    s += S(right) + "]";
    return s;
}

///check for fixed step
fp Data::find_step() 
{
    const fp tiny_relat_diff=0.01;
    if (p.size() <= 2)
        return 0;
    fp min_step, max_step, step;
    min_step = max_step = p[1].x - p[0].x;
    for (vector<Point>::iterator i = p.begin() + 2; i < p.end(); i++) {
        step = i->x - (i-1)->x;
        min_step = min (min_step, step);
        max_step = max (max_step, step);
    }
    fp avg = (max_step + min_step) / 2;
    if ((max_step - min_step) < tiny_relat_diff * fabs(avg)) 
        return avg;
    else 
        return 0.;
}
        
int Data::get_one_line_with_numbers(istream &is, vector<fp>& result_numbers) 
{
    // returns number of numbers in line
    result_numbers.clear();
    string s;
    while (getline (is, s) && (s.empty() || s[0] == '#' 
                             || s.find_first_not_of(" \t\r\n") == string::npos))
        ;//ignore lines with '#' at first column or empty lines
    for (string::iterator i = s.begin(); i != s.end(); i++)
        if (*i == ',' || *i == ';' || *i == ':')
            *i = ' ';
    istringstream q(s);
    fp d;
    while (q >> d)
        result_numbers.push_back (d);
    return !is.eof();
}


int Data::get_lower_bound_ac (fp x) const
{
    //pre: p.x is sorted, active_p is sorted
    int pit = lower_bound (p.begin(), p.end(), Point(x)) - p.begin();
    return lower_bound (active_p.begin(), active_p.end(), pit) 
        - active_p.begin();
}

int Data::get_upper_bound_ac (fp x) const
{
    //pre: p.x is sorted, active_p is sorted
    int pit = upper_bound (p.begin(), p.end(), Point(x)) - p.begin();
    return upper_bound (active_p.begin(), active_p.end(), pit) 
        - active_p.begin();
}

vector<Point>::const_iterator Data::get_point_at(fp x) const
{
    return lower_bound (p.begin(), p.end(), Point(x));
}

void Data::export_as_script (ostream& os)
{
    if (filename.empty()) {
        os << "## no data loaded ##";
        return;
    }
    //TODO embed data
    os << "### data settings exported as script -- begin" << endl;
    os << set_script('d');
    os << "d.load '";
    //TODO explicit filetype, when needed.
    for (vector<int>::iterator i = col_nums.begin(); i != col_nums.end(); i++)
        os << *i << (i != col_nums.end() - 1 ? " : " : "  ");
    if (!every.empty()) {
        assert (size(every) == 2 || size(every) == 3);
        os << every[0] << "-" << every[1];
        if (size(every) == 3)  os << "/" << every[2];
        os << "  ";
    }
    if (merging) 
            os << (merging > 0 ? "+*" : "*") << abs(merging) << "  ";
    os << filename << "'\n";
    if (sigma_type != 'f')
        os << "d.deviation " << sigma_type << " " << sigma_minim << endl;
    os << "d.range " << range_as_string() << endl;
    os << "### data settings -- end" << endl;
}

void Data::export_as_dat (ostream& os)
{
    for (int i = 0; i < get_n(); i++)
        os << get_x(i) << "\t" << get_y(i) << "\t" << get_sigma(i) << endl;
}

void Data::export_to_file (string filename, bool append, char filetype) 
{
    ofstream os(filename.c_str(), ios::out | (append ? ios::app : ios::trunc));
    if (!os) {
        warn ("Can't open file: " + filename);
        return;
    }
    int dot = 0;//for filetype detection
    switch (filetype) {
        case 'd': 
            export_as_dat (os);
            break;
        case 's':
            export_as_script (os);
            break;
        case 0:
            //guessing filetype
            dot = filename.rfind('.');
            if (dot > 0 && dot < static_cast<int>(filename.length()) - 1) {
                string ex(filename.begin() + dot, filename.end());
                if (ex == ".dat" || ex == ".DAT" || ex == ".xy" || ex == ".XY")
                    return export_to_file (filename, append, 'd');
            }
            info ("exporting as script");
            return export_to_file (filename, append, 's');
        default:
            warn ("Unknown filetype letter: " + S(filetype));
    }
}

