// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef DATA__H__
#define DATA__H__
#include <string>
#include <vector>
#include <fstream>
#include "common.h"


/// data points
struct Point 
{
    fp x, y, sigma;
    bool is_active;
    Point () : x(0), y(0), sigma(0), is_active(true) {}
    Point (fp x_) : x(x_), y(0), sigma(0), is_active(true) {}
    Point (fp x_, fp y_) : x(x_), y(y_), sigma (0), is_active(true) {}
    Point (fp x_, fp y_, fp sigma_) : x(x_), y(y_), sigma(sigma_), 
                                      is_active(true) {}
    std::string str() { return "(" + S(x) + "; " + S(y) + "; " + S(sigma) 
                               + (is_active ? ")*" : ") "); }
};

inline bool operator< (const Point& p, const Point& q) 
{ return p.x < q.x; }
    

class Data 
{
public :
    Data ();
    ~Data () {}
    void d_was_plotted() { d_was_changed = false; }
    bool was_changed() const { return d_was_changed; }
    std::string getInfo() const;
    int load_file (const std::string &file, int type, 
                   const std::vector<int> &cols, bool append=false);
    int load_arrays(const std::vector<fp> &x, const std::vector<fp> &y, 
                   const std::vector<fp> &sigma, const std::string &data_title);
    void add_point(const Point &pt) { p.push_back(pt); };
    static char guess_file_type (const std::string& filename);
    fp get_x (int n) const { return p[active_p[n]].x; }
    fp get_y (int n) const { return p[active_p[n]].y; } 
    fp get_sigma (int n) const { return p[active_p[n]].sigma; }
    int get_n () const {return active_p.size();}
    bool is_empty () const {return p.empty();}
    fp get_x_step() const {return x_step;}
    bool transform(const std::string &s);
    void update_active_p();
    //int auto_range (fp y_level, fp x_margin);
    std::string range_as_string () const;
    int get_lower_bound_ac (fp x) const; 
    int get_upper_bound_ac (fp x) const;
    std::string get_title() const { return title.empty() ? filename : title; }
    std::string get_filename() const { return filename; }

    void recompute_y_bounds();
    fp get_y_at (fp x) const;
    //return points at x (if any) or (usually) after it.
    std::vector<Point>::const_iterator get_point_at(fp x) const;
    void export_to_file (std::string filename, bool append);
    std::string Data::get_load_cmd();
    void export_as_script (std::ostream& os);
    fp get_x_min() { return p.front().x; }
    fp get_x_max() { return p.back().x; } 
    fp get_y_min() const { return y_min; }
    fp get_y_max() const { return y_max; }
    const std::vector<Point>& points() const { return p; }
private:
    bool d_was_changed;
    std::string title;
    std::string filename;
    std::vector<int> col_nums;
    fp x_step; // 0.0 if not fixed;
    std::vector<Point> p;
    std::vector<int> active_p;
    fp y_min, y_max;

    Data (Data&); //disable
    bool get_seq_num (std::istream &is, fp *num);
    int get_one_line_with_numbers (std::istream &is, 
                                    std::vector<fp>& result_numbers);
    static double pdp11_f (char* fl);  
    fp find_step();
    void load_xy_filetype (std::ifstream& f, const std::vector<int>& cols);
    void load_mca_filetype (std::ifstream& f);
    void load_rit_filetype (std::ifstream& f);
    void load_cpi_filetype (std::ifstream& f);
    void clear();
    void post_load();
};

extern Data *my_data;

#endif
