// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#include "common.h"
#include "data.h" 
#include "fileroutines.h"
#include "ui.h"
#include "numfuncs.h"
#include "datatrans.h" 
#include "settings.h" 

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


Point::Point() : x(0), y(0), sigma(1), is_active(true) {}
Point::Point(fp x_, fp y_) : x(x_), y(y_), sigma(1), is_active(true) {}
Point::Point(fp x_, fp y_, fp sigma_) : x(x_), y(y_), sigma(sigma_), 
                                        is_active(true) {}
std::string Point::str() { return "(" + S(x) + "; " + S(y) + "; " + S(sigma) 
                               + (is_active ? ")*" : ") "); }


string Data::getInfo() const
{
    string s;
    if (p.empty())
        s = "No data points.";
    else
        s = S(p.size()) + " points, " + S(active_p.size()) + " active.";
    if (!filename.empty())
        s += "\nFilename: " + filename;
    if (!title.empty())
        s += "\nData title: " + title;
    if (active_p.size() != p.size())
        s += "\nActive data range: " + range_as_string();
    return s;
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

string Data::guess_file_type(string const& filename)
{ //using only filename extension
    if (filename.size() > 4) {
        string file_ext = filename.substr (filename.size() - 4, 4);
        if (file_ext == ".mca"   || file_ext == ".MCA") 
            return "MCA";
        else if (file_ext == ".rit" || file_ext == ".RIT")
            return "RIT";
        else if (file_ext == ".cpi" || file_ext == ".CPI")
            return "CPI";
        else if (file_ext == ".raw" || file_ext == ".RAW")
            return "BrukerRAW";
    }
    return "text";
}

void Data::clear()
{
    filename = "";   
    title = "";
    given_type = "";
    given_cols.clear();
    p.clear();
    active_p.clear();
    has_sigma = false;
}

void Data::post_load()
{
    if (p.empty())
        return;
    string inf = S(p.size()) + " points.";
    if (!has_sigma) {
        int dds = getSettings()->get_e("data-default-sigma");
        if (dds == 's') {
            for (vector<Point>::iterator i = p.begin(); i < p.end(); i++) 
                i->sigma = i->y > 1. ? sqrt (i->y) : 1.;
            inf += " No explicit std. dev. Set as sqrt(y)";
        }
        else if (dds == '1') {
            for (vector<Point>::iterator i = p.begin(); i < p.end(); i++) 
                i->sigma = 1.; 
            inf += " No explicit std. dev. Set as equal 1.";
        }
        else
            assert(0);
    }
    msg(inf);
    if (title.empty())
        title = get_file_basename(filename);
    update_active_p();
    recompute_y_bounds();
}

void Data::recompute_y_bounds() {
    bool ini = false;
    for (vector<Point>::iterator i = p.begin(); i != p.end(); i++) {
        if (!is_finite(i->y))
            continue;
        if (!ini) {
            y_min = y_max = i->y;
            ini = true;
        }
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
    title = data_title;
    if (sigma.empty()) 
        for (size_t i = 0; i < size; ++i)
            p.push_back (Point (x[i], y[i]));
    else {
        for (size_t i = 0; i < size; ++i)
            p.push_back (Point (x[i], y[i], sigma[i]));
        has_sigma = true;
    }
    sort(p.begin(), p.end());
    x_step = find_step();
    post_load();
    return p.size();
}

namespace {

void apply_operation(vector<Point> &pp, string const& op)
{
    assert (!pp.empty());
    assert (!op.empty());
    if (op == "sum_same_x" || op == "avg_same_x") {
        bool avg = (op == "avg_same_x");
        int count_same = 1;
        fp x0;
        for (int i = pp.size() - 2; i >= 0; --i) {
            if (count_same == 1)
                x0 = pp[i+1].x;
            if (is_eq(pp[i].x, x0)) {
                pp[i].x += pp[i+1].x;
                pp[i].y += pp[i+1].y;
                pp[i].sigma += pp[i+1].sigma;
                pp[i].is_active = pp[i].is_active || pp[i+1].is_active;
                pp.erase(pp.begin() + i+1);
                count_same++;
                if (i > 0)
                    continue;
                else
                    i = -1; // to change pp[0]
            }
            if (count_same > 1) {
                pp[i+1].x /= count_same;
                if (avg) {
                    pp[i+1].y /= count_same;
                    pp[i+1].sigma /= count_same;
                }
                count_same = 1;
            }
        }
    }
    else if (op == "fft") {
        throw ExecuteError("Fourier Transform not implemented yet");
    }
    else if (op == "ifft") {
        throw ExecuteError("Inverse FFT not implemented yet");
    }
    else
        throw ExecuteError("Unknown dataset operation: " + op);
}

} // anonymous namespace

void Data::load_data_sum(vector<Data const*> const& dd, string const& op)
{
    assert(!dd.empty());
    // dd can contain this, we can't change p or title in-place.
    std::vector<Point> new_p;
    string new_title = dd[0]->get_title();
    string new_filename = dd.size() == 1 ? dd[0]->get_filename() : "";
    for (vector<Data const*>::const_iterator i = dd.begin()+1; 
                                                          i != dd.end(); ++i)
        new_title += " + " + (*i)->get_title();
    for (vector<Data const*>::const_iterator i = dd.begin(); 
                                                          i != dd.end(); ++i) {
        vector<Point> const& points = (*i)->points();
        new_p.insert(new_p.end(), points.begin(), points.end());
    }
    sort(new_p.begin(), new_p.end());
    if (!new_p.empty() && !op.empty())
        apply_operation(new_p, op);
        // data should be sorted after apply_operation()
    clear();
    title = new_title;
    filename = new_filename;
    p = new_p;
    has_sigma = true;
    x_step = find_step();
    post_load();
}

void Data::load_file (string const& file, string const& type, 
                     vector<int> const& cols, bool preview)
{   
    string const& ft = type.empty() ? guess_file_type(file) // "detect" format
                                    : type;
    ifstream f (file.c_str(), ios::in | ios::binary);
    if (!f) {
        throw ExecuteError("Can't open file: " + file);
    }
    clear(); //removing previous file
    filename = file;   
    given_type = type;
    given_cols = cols;

    if (ft == "text")                         // x y x y ... 
        load_xy_filetype(f, cols);
    else if (ft == "htext")                   // header\n x y x y ... 
        load_header_xy_filetype(f, cols);
    else if (ft == "MCA")                     // .mca
        load_mca_filetype(f);
    else if (ft == "RIT")                     // .rit
        load_rit_filetype(f);
    else if (ft == "CPI")                     // .cpi
        load_cpi_filetype(f);
    else if (ft == "BrukerRAW")               // .raw
        load_siemensbruker_filetype(filename, this);
    else {                                  // other ?
        throw ExecuteError("Unknown filetype.");
    }
    if (preview) {
        recompute_y_bounds();
        return;
    }
    if (p.size() < 5)
        warn ("Only " + S(p.size()) + " data points found in file.");
    if (!f.eof() && ft != "MCA") //!=mca; .mca doesn't have to reach EOF
        warn ("Unexpected char when reading " + S (p.size() + 1) + ". point");
    post_load();
}


void Data::load_xy_filetype (ifstream& f, vector<int> const& columns)
{
    /* format  x y \n x y \n x y \n ...
    *           38.834110      361
    *           38.872800  ,   318
    *           38.911500      352.431
    * delimiters: white spaces and  , : ;
     */
    if (columns.size() == 1 || columns.size() > 3) //0, 2, or 3 columns allowed
        throw ExecuteError("If columns are specified, two or three of them"
                           " must be given (eg. 1,2,3)");
    vector<int> const& cols = columns.empty() ? vector2<int>(1, 2) : columns;
    vector<fp> xy;
    has_sigma = (columns.size() == 3);
    int maxc = *max_element (cols.begin(), cols.end());
    int minc = *min_element (cols.begin(), cols.end());
    if (minc < 0) {
        warn ("Invalid (negative) column number: " + S(minc));
        return;
    }
    int not_enough_cols = 0, non_data_lines = 0, non_data_blocks = 0;
    bool prev_empty = false;
    //TODO: optimize loop below
    //most of time (?) is spent in getline() in read_line_and_get_all_numbers()
    while (read_line_and_get_all_numbers(f, xy)) {
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

        fp x = cols[0] == 0 ? p.size() : xy[cols[0] - 1];
        fp y = cols[1] == 0 ? p.size() : xy[cols[1] - 1];
        if (cols.size() == 2)
            p.push_back (Point(x, y));
        else {// cols.size() == 3
            fp sig = cols[2] == 0 ? p.size() : xy[cols[2] - 1];
            if (sig > 0) 
                p.push_back (Point(x, y, sig));
            else
                warn ("Point " + S(p.size()) + " has sigma = " + S(sig) 
                        + ". Point canceled.");
        }
    }
    if (non_data_lines > 0)
        msg (S(non_data_lines) +" (not empty and not `#...') non-data lines "
                "in " + S(non_data_blocks) + " blocks.");
    if (not_enough_cols > 0)
        warn ("Less than " + S(maxc) + " numbers in " + S(not_enough_cols) 
                + " lines.");
    sort(p.begin(), p.end());
    x_step = find_step();
}

string Data::read_one_line_as_title(ifstream& f, int column)
{
    if (!f)
        return "";
    string s;
    getline(f, s);
    --column;
    if (column >= 0) {
        vector<string> v = split_string(s, " \t");
        if (column < size(v)) 
            return v[column];
    }
    return strip_string(s);
}

/// read first line as file's title, and than run load_xy_filetype()
void Data::load_header_xy_filetype(ifstream& f, vector<int> const& columns)
{
    int title_col = columns.size() > 1 ? columns[1] : 0;
    title = Data::read_one_line_as_title(f, title_col);
    load_xy_filetype(f, columns);
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
        p.push_back (Point(x, y));
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
        p.push_back (Point (x, y));
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
    bool r = read_line_and_get_all_numbers(f, num);
    if (!r || num.size() < 2 ){
        warn ("Bad format in \"header\" of .rit file");
        return;
    }
    fp xmin = num[0];
    x_step = static_cast<int>(num[1] * 1e4) / 1e4; //only 4 digits after '.'
    vector<fp> ys;
    while (read_line_and_get_all_numbers(f, ys)) {
        if (ys.size() == 0)
            warn ("Error when trying to read " + S (p.size() + 1) + ". point. " 
                    "Ignoring line.");
        for (vector<fp>::iterator i = ys.begin(); i != ys.end(); i++) {
            fp x = xmin + p.size() * x_step;
            p.push_back (Point(x, *i));
        }
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

void Data::transform(const string &s) 
{
    p = transform_data(s, p);
    sort(p.begin(), p.end());
    update_active_p();
}

void Data::update_active_p() 
    // pre: p.x sorted
    // post: active_p sorted
{
    active_p.clear();
    for (int i = 0; i < size(p); i++)
        if (p[i].is_active) 
            active_p.push_back(i);
}

// does anyone need it ? 
/*
int Data::auto_range (fp y_level, fp x_margin)
{
    // pre: p.x sorted
    if (p.empty()) {
        warn("No points loaded.");
        return -1;
    }
    bool state = (p.begin()->y >= y_level);
    for (vector<Point>::iterator i = p.begin(); i < p.end(); i++) {
        if (state == (i->y >= y_level)) 
            i->is_active = state;
        else if (state && i->y < y_level) {
            i->is_active = false;
            vector<Point>::iterator e = lower_bound (p.begin(), p.end(),
                                                        Point(i->x + x_margin));
            for ( ; i < e; i++)
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
    update_active_p();
    return active_p.size();
}
*/

//FIXME to remove it or to leave it?
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
        
int Data::read_line_and_get_all_numbers(istream &is, vector<fp>& result_numbers)
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
    int pit = lower_bound (p.begin(), p.end(), Point(x,0)) - p.begin();
    return lower_bound (active_p.begin(), active_p.end(), pit) 
        - active_p.begin();
}

int Data::get_upper_bound_ac (fp x) const
{
    //pre: p.x is sorted, active_p is sorted
    int pit = upper_bound (p.begin(), p.end(), Point(x,0)) - p.begin();
    return upper_bound (active_p.begin(), active_p.end(), pit) 
        - active_p.begin();
}

vector<Point>::const_iterator Data::get_point_at(fp x) const
{
    return lower_bound (p.begin(), p.end(), Point(x,0));
}

void Data::export_to_file(string filename, vector<string> const& vt, 
                          vector<string> const& ff_names,
                          bool append) 
{
    ofstream os(filename.c_str(), ios::out | (append ? ios::app : ios::trunc));
    if (!os) {
        warn ("Can't open file: " + filename);
        return;
    }
    os << "# " << title << endl;
    vector<string> cols;
    if (vt.empty()) {
        cols.push_back("x");
        cols.push_back("y");
        cols.push_back("s");
    }
    else {
        for (vector<string>::const_iterator i=vt.begin(); i != vt.end(); ++i) 
            if (startswith(*i, "*F")) {
                for (vector<string>::const_iterator j = ff_names.begin(); 
                                                    j != ff_names.end(); ++j)
                    cols.push_back("%" + *j + string(*i, 2));
            }
            else
                cols.push_back(*i);
    }
    os << join_vector(cols, "\t") << "\t#exported by fityk " VERSION << endl;

    vector<vector<fp> > r;
    bool only_active = !contains_element(vt, "a");
    for (vector<string>::const_iterator i = cols.begin(); i != cols.end(); ++i) 
        r.push_back(get_all_point_expressions(*i, this, only_active));

    size_t nc = cols.size();
    for (size_t i = 0; i != r[0].size(); ++i) {
        for (size_t j = 0; j != nc; ++j) {
            os << r[j][i] << (j < nc-1 ? '\t' : '\n'); 
        }
    }
}

