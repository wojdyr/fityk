// This file is part of fityk program. Copyright (C) Marcin Wojdyr
#include "common.h"
RCSID ("$Id$")

#include "data.h" 
#include "fileroutines.h"
#include <math.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

Data *my_data;
DataSets *my_datasets;

Data::Data ()
    : d_was_changed(true), background_infl_sigma(false)
{
    min_background_distance[bgc_bg] = 0.5;
    spline_background[bgc_bg] = true;
    min_background_distance[bgc_cl] = 0.002, 
    spline_background[bgc_cl] = false, 
    fpar ["min-background-points-distance"] = &min_background_distance[bgc_bg];
    bpar ["spline-background"] = &spline_background[bgc_bg];
    bpar ["spline-calibration"] = &spline_background[bgc_cl];
    bpar ["background-influences-error"] = &background_infl_sigma;
}

string Data::info () const
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

int Data::load (string file, int type, 
                vector<int> col, vector<int> evr, int merge)
{ 
    if (type == 0) {                  // "detect" file format
        type = guess_file_type(file);
    }
    ifstream plik (file.c_str(), ios::in | ios::binary);
    if (!plik) {
        warn ("Can't open file: " + file );
        return -1;
    }
    d_was_changed = true;
    filename = file;   //removing previos file
    title = "";
    p.clear();
    active_p.clear();
    filetype = type;
    col_nums = col;
    every = evr;
    every_idx = 1;
    merging = merge;
    merge_table.clear();

    if (type=='d')                            // x y x y ... 
        load_xy_filetype (plik, col);
    else if (type=='m')                       // .mca
        load_mca_filetype (plik);
    else if (type=='r')                       // .rit
        load_rit_filetype (plik);
    else if (type=='c')                       // .rit
        load_cpi_filetype (plik);
    else if (type=='s')                       // .raw
        load_siemensbruker_filetype(filename, this);
    else {                                  // other ?
        warn ("Unknown filetype.");
        return -1;
    }
    if (p.size() < 5)
        warn ("Only " + S(p.size()) + " data points found in file.");
    if (!plik.eof() && type != 'm') //!=mca; .mca doesn't have to reach EOF
        warn ("Unexpected char when reading " + S (p.size() + 1) + ". point");
    if (p.empty())
        return 0;
    change_range (-INF, +INF, true);
    if (!p[0].sigma)
        change_sigma('r');
    y_orig_min = y_orig_max = p.front().orig_y;
    for (vector<Point>::iterator i = p.begin(); i != p.end(); i++) {
        if (i->orig_y < y_orig_min )
            y_orig_min = i->orig_y;
        if (i->orig_y > y_orig_max)
            y_orig_max = i->orig_y;
    }
    if (!background[bgc_bg].empty())
        recompute_background (bgc_bg);
    if (!background[bgc_cl].empty())
        recompute_background (bgc_cl);
    else
        recompute_y_bounds();
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

void Data::load_xy_filetype (ifstream& plik, vector<int>& usn)
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
    while (get_one_line_with_numbers(plik, xy)) {
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
        mesg (S(non_data_lines) +" (not empty and not `#...') non-data lines "
                "in " + S(non_data_blocks) + " blocks.");
    if (not_enough_cols > 0)
        warn ("Less than " + S(maxc) + " numbers in " + S(not_enough_cols) 
                + " lines.");
    if (usn.size() == 3) {
        sigma_type = 'f';
        mesg ("Std. dev. read from file.");
    }
    sort(p.begin(), p.end());
    x_step = find_step();
}

void Data::load_mca_filetype (ifstream& plik) 
{
    typedef unsigned short int ui2b;
    char all_data [9216];//2*512+2048*4];
    plik.read (all_data, sizeof(all_data));
    if (plik.gcount() != static_cast<int>(sizeof(all_data)) 
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

void Data::load_cpi_filetype (ifstream& plik) 
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
    getline (plik, s);
    if (s.substr(0, header.size()).compare (header) != 0){
        warn ("No \"" + header + "\" header found.");
        return;
    }
    getline (plik, s);//xmin
    fp xmin = strtod (s.c_str(), 0);
    getline (plik, s); //xmax
    getline (plik, s); //xstep
    x_step = strtod (s.c_str(), 0); 
    //the rest of header
    while (s.substr(0, start_flag.size()).compare(start_flag) != 0)
        getline (plik, s);
    //data
    while (getline(plik, s)) {
        fp y = strtod (s.c_str(), 0);
        fp x = xmin + p.size() * x_step;
        add_point (Point (x, y));
    }
}

void Data::load_rit_filetype (ifstream& plik) 
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
    bool r = get_one_line_with_numbers(plik, num);
    if (!r || num.size() < 2 ){
        warn ("Bad format in \"header\" of .rit file");
        return;
    }
    fp xmin = num[0];
    x_step = static_cast<int>(num[1] * 1e4) / 1e4; //only 4 digits after '.'
    vector<fp> ys;
    while (get_one_line_with_numbers(plik, ys)) {
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
    mesg (print_sigma());
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
                fp y = background_infl_sigma ? i->y : i->orig_y;
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

fp Data::get_bg_at (fp x) const
{
    int n = get_upper_bound_ac (x);
    if (n >= size(active_p) || n <= 0) {
        if (active_p.empty()) return 0;
        else if (n == 0) return get_background_y (0);
        else if (n == size(active_p)) return get_background_y (n - 1);
        else return 0;
    }
    fp y1 = get_background_y (n - 1);
    fp y2 = get_background_y (n);
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
    if (p.empty()) 
        return warn("No points loaded.");
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
    if ((max_step - min_step) / fabs(avg) < tiny_relat_diff) 
        return avg;
    else 
        return 0;
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

void Data::auto_background (int n, fp p1, bool is_proc1, fp p2, bool is_proc2)
{
    //FIXME: Do you know any good algorithm, that can extract background
    //       from data?
    if (n <= 0 || n >= size(p) / 2)
        return;
    int ps = p.size();
    for (int i = 0; i < n; i++) {
        int l = ps * i / n;
        int u = ps * (i + 1) / n;
        vector<fp> v (u - l);
        for (int k = l; k < u; k++)
            v[k - l] = p[k].orig_y;
        sort (v.begin(), v.end());
        int y_avg_beg = 0, y_avg_end = v.size();
        if (is_proc1) {
            p1 = min (max (p1, 0.), 100.);
            y_avg_beg = static_cast<int>(v.size() * p1 / 100); 
        }
        else {
            y_avg_beg = upper_bound (v.begin(), v.end(), v[0] + p1) - v.begin();
            if (y_avg_beg == size(v))
                y_avg_beg--;
        }
        if (is_proc2) {
            p2 = min (max (p2, 0.), 100.);
            y_avg_end = y_avg_beg + static_cast<int>(v.size() * p2 / 100);
        }
        else {
            fp end_val = v[y_avg_beg] + p2;
            y_avg_end = upper_bound (v.begin(), v.end(), end_val) - v.begin();
        }
        if (y_avg_beg < 0) 
            y_avg_beg = 0;
        if (y_avg_end > size(v))
            y_avg_end = v.size();
        if (y_avg_beg >= y_avg_end) {
            if (y_avg_beg >= size(v))
                y_avg_beg = v.size() - 1;;
            y_avg_end = y_avg_beg + 1;
        }
        int counter = 0;
        fp y = 0;
        for (int j = y_avg_beg; j < y_avg_end; j++){
            counter++;
            y += v[j];
        }
        y /= counter;
        add_background_point ((p[l].x + p[u - 1].x) / 2, y, bgc_bg);
    }
}

////background/calibration functions
//default value of bg_cl is bgc_bg (background), in this case names of functions
//are proper. In case bg_cl = bgc_cl (calibration) functions' names are
//not adequate.

void Data::add_background_point (fp x, fp y, Bg_cl_enum bg_cl)
{
    rm_background_point (x, bg_cl);
    B_point t(x, y);
    vector<B_point> &bc = background[bg_cl];
    vector<B_point>::iterator l = lower_bound (bc.begin(), bc.end(), t);
    bc.insert (l, t);
    recompute_background(bg_cl);
}

void Data::rm_background_point (fp x, Bg_cl_enum bg_cl)
{
    vector<B_point> &bc = background[bg_cl];
    fp min_dist = min_background_distance[bg_cl];
    vector<B_point>::iterator l = lower_bound (bc.begin(), bc.end(), 
                                               B_point(x - min_dist, 0));
    vector<B_point>::iterator u = upper_bound (bc.begin(), bc.end(), 
                                               B_point(x + min_dist, 0));
    if (u - l) {
        bc.erase (l, u);
        mesg (S (u - l) + " points removed.");
        recompute_background(bg_cl);
    }
}

void Data::clear_background (Bg_cl_enum bg_cl)
{
    vector<B_point> &bc = background[bg_cl];
    int n = bc.size();
    bc.clear();
    recompute_background(bg_cl);
    mesg (S(n) + " points deleted.");
}

string Data::background_info (Bg_cl_enum bg_cl) 
{
    string msg = bg_cl == bgc_bg ? "Background " : "Calibration ";
    recompute_background(bg_cl);
    vector<B_point> &bc = background[bg_cl];
    int s = bc.size();
    if (s == 0)
        msg += "not defined";
    else if (s == 1)
        msg += "constant and equal " + S(bc[0].y);
    else if (s == 2)
        msg += "linear given by two points: " + bc[0].str()
                + " and " + bc[1].str();
    else if (s <= 16) {
        msg += "given as interpolation of " + S(s) +" points: ";
        for (vector<B_point>::iterator i = bc.begin(); i != bc.end(); i++)
            msg += (i == bc.begin() ? "" : " , ") + S(i->x) + " " + S(i->y);
            //older version: msg += " " + i->str();
    }
    else
        msg += "given as interpolation of " + S(s) + " points";
    return msg;
}

void Data::recompute_background (Bg_cl_enum bg_cl)
{
    vector<B_point> &bc = background[bg_cl];
    if (active_p.empty()) {
        warn ("File not loaded or all points inactive.");
        return;
    }
    d_was_changed = true;
    int size = bc.size();
    if (size == 0) 
        for (vector<Point>::iterator i = p.begin(); i != p.end(); i++)
            if (bg_cl == bgc_bg)
                i->y = i->orig_y; 
            else
                i->x = i->orig_x;
    else if (size == 1) 
        for (vector<Point>::iterator i = p.begin(); i != p.end(); i++)
            if (bg_cl == bgc_bg)
                i->y = i->orig_y - bc[0].y;
            else
                i->x = i->orig_x - bc[0].y;
    else if (size == 2) {
        fp a = (bc[1].y - bc[0].y) / (bc[1].x - bc[0].x);
        fp b = bc[0].y - a * bc[0].x;
        for (vector<Point>::iterator i = p.begin(); i != p.end(); i++)
            if (bg_cl == bgc_bg)
                i->y = i->orig_y - (a * i->orig_x + b);
                                          //^^^^^^it's not a typo
            else
                i->x = i->orig_x - (a * i->orig_x + b);
    }
    else {//size > 2
        if (spline_background[bg_cl])
            spline_interpolation(bg_cl);
        else
            linear_interpolation(bg_cl);
    }
    if (bg_cl == bgc_bg) {
        recompute_y_bounds();
        if (background_infl_sigma)
            recompute_sigma();
    }
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

vector<B_point>::iterator 
Data::bg_interpolation_find_place (vector<B_point> &b,  fp x)
{
    //optimized for sequence of x = x1, x2, x3, x1 < x2 < x3...
    static vector<B_point>::iterator pos = b.begin();
    assert (size(b) > 1);
    assert (x > b.front().x && x < b.back().x);
    if (pos < b.begin() || pos >= b.end() - 1) 
        pos = b.begin();
    if (pos->x <= x && ((pos + 1)->x >= b.back().x || x <= (pos + 1)->x )) 
        return pos;
    else {
        pos++;
        if (pos->x <= x && ((pos + 1)->x >= b.back().x || x <= (pos + 1)->x )) 
            return pos;
        else {
            pos = lower_bound (b.begin(), b.end(), Simple_point(x, 0)) - 1;
            return pos;
        }
    }
}

void Data::prepare_spline_interpolation (vector<B_point> &background)
{// based on Numerical Recipes www.nr.com
    //first wroten for background interpolation, then generalized
    const int n = background.size();
        //find d2y/dx2 and put it in .q
    background[0].q = 0; //natural spline
    vector<fp> u (n);
    for (int k = 1; k <= n - 2; k++) {
        B_point *b = &background[k];
        fp sig = (b->x - (b - 1)->x) / ((b + 1)->x - (b - 1)->x);
        fp t = sig * (b - 1)->q + 2.0;
        b->q = (sig - 1.0) / t;
        u[k] = ((b + 1)->y - b->y) / ((b + 1)->x - b->x) - (b->y - (b - 1)->y)
                            / (b->x - (b - 1)->x);
        u[k] = (6.0 * u[k] / ((b + 1)->x - (b - 1)->x) - sig * u[k - 1]) / t;
    }
    background.back().q = 0; 
    for (int k = n - 2; k >= 0; k--) {
        B_point *b = &background[k];
        b->q = b->q * (b + 1)->q + u[k];
    }
}

void Data::spline_interpolation (Bg_cl_enum bg_cl)
{ // based on Numerical Recipes www.nr.com
    //first wroten for background interpolation, then generalized
    vector<B_point> &bc = background[bg_cl];
    prepare_spline_interpolation (bc);

    //put background/calibration into `p'
    for (vector<Point>::iterator i = p.begin(); i != p.end(); i++) {
        vector<B_point>::iterator pos;
        if (i->orig_x <= bc.front().x)
            pos = bc.begin();//i->y = i->orig_y;
        else if (i->orig_x >= bc.back().x)
            pos = bc.end() - 2;//i->y = i->orig_y;
        else 
            pos = bg_interpolation_find_place (bc, i->orig_x);

        fp h = (pos + 1)->x - pos->x;
        fp a = ((pos + 1)->x - i->orig_x) / h;
        fp b = (i->orig_x - pos->x) / h;
        fp t = a * pos->y + b * (pos + 1)->y + ((a * a * a - a) * pos->q 
                + (b * b * b - b) * (pos + 1)->q) * (h * h) / 6.0;

        if (bg_cl == bgc_bg)
            i->y = i->orig_y - t;
        else 
            i->x = i->orig_x - t;
    }
}

void Data::linear_interpolation (Bg_cl_enum bg_cl)
{
    //first wroten for background interpolation, then generalized
    vector<B_point> &bc = background[bg_cl];
    for (vector<Point>::iterator i = p.begin(); i != p.end(); i++) {
        vector<B_point>::iterator pos; 
        if (i->orig_x <= bc.front().x)
            pos = bc.begin();
        else if (i->orig_x >= bc.back().x)
            pos = bc.end() - 2;
        else 
            pos = bg_interpolation_find_place (bc, i->orig_x);
        fp a = ((pos + 1)->y - pos->y) / ((pos + 1)->x - pos->x);
        fp b = pos->y - a * pos->x;

        if (bg_cl == bgc_bg)
            i->y = i->orig_y - (a * i->orig_x + b);
        else 
            i->x = i->orig_x - (a * i->orig_x + b);
    }
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
    for (int bg_cl = bgc_bg; bg_cl <= bgc_cl; bg_cl++) {
        vector<B_point> &bc = background[bg_cl];
        if (!bc.empty()) {
            os << (bg_cl == bgc_bg ? "d.background " : "d.calibrate ");
            for (vector<B_point>::iterator i = bc.begin(); i != bc.end(); i++) {
                if (i != bc.begin())
                    os << " , ";
                os << i->x << " " << i->y;
            }
            os << endl;
        }
    }
    os << "### data settings -- end" << endl;
}

void Data::export_as_dat (ostream& os)
{
    for (int i = 0; i < get_n(); i++)
        os << get_x(i) << "\t" << get_y(i) << "\t" << get_sigma(i) << endl;
}

void Data::export_bg_as_dat (ostream& os)
{
    for (int i = 0; i < get_n(); i++)
        os << get_x(i) << "\t" << get_background_y(i) << endl; 
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
        case 'b': 
            export_bg_as_dat (os);
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
            mesg ("exporting as script");
            return export_to_file (filename, append, 's');
        default:
            warn ("Unknown filetype letter: " + S(filetype));
    }
}

//==========================================================================

DataSets::~DataSets()
{
    for (vector<Data*>::iterator i = datasets.begin(); i != datasets.end(); i++)
        delete *i;
}

bool DataSets::activate(int n)
{
    if (n == -1) {
        datasets.push_back(new Data);
        my_data = *(datasets.end() - 1);
        ds_was_changed = true;
        return true;
    }
    else if (n >= 0 && n < size(datasets)) {
        my_data = datasets[n];
        ds_was_changed = true;
        return true;
    }
    else {
        warn("No such datafile in this dataset: " + S(n));
        return false;
    }
}

int DataSets::find_active() const
{
    for (unsigned int i = 0; i < datasets.size(); i++)
        if (datasets[i] == my_data)
            return i;
    //not found
    return -1; 
}

const Data *DataSets::get_data(int n) const
{
    if (n >= 0 && n < size(datasets)) {
        return datasets[n];
    }
    else
        return 0;
}


vector<string> DataSets::get_data_titles() const
{
    vector<string> v;
    //v.reserve(datasets.size());
    for (vector<Data*>::const_iterator i = datasets.begin(); 
                                                    i != datasets.end(); i++)
        v.push_back((*i)->get_title());
    return v;
}

void DataSets::del_data(int n)
{
    if (n >= 0 && n < size(datasets)) {
        if (my_data == datasets[n]) {
            if (n > 0)
                my_data = datasets[n-1];
            else
                my_data = 0;
        }
        delete datasets[n];
        datasets.erase(datasets.begin() + n);
        if (datasets.empty()) //it should not be empty
            datasets.push_back(new Data);
        if (!my_data)
            my_data = datasets.front();
        ds_was_changed = true;
    }
    else
        warn("No such dataset number: " + S(n));
}

void DataSets::was_plotted() 
{ 
    ds_was_changed = false; 
    for (vector<Data*>::iterator i = datasets.begin(); i != datasets.end(); i++)
        (*i)->d_was_plotted();
}

bool DataSets::was_changed() const 
{ 
    for (vector<Data*>::const_iterator i = datasets.begin(); 
                                                    i != datasets.end(); i++)
        if ((*i)->was_changed())
            return true;
    //if here - no Data changed
    return ds_was_changed; 
}


