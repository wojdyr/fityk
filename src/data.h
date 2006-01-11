// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__DATA__H__
#define FITYK__DATA__H__
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
    Data() {}
    ~Data() {}
    std::string getInfo() const;
    void load_file (std::string const& file, std::string const& type, 
                    std::vector<int> const& cols);
    int load_arrays(std::vector<fp> const& x, std::vector<fp> const& y, 
                   std::vector<fp> const& sigma, std::string const& data_title);
    void load_data_sum(std::vector<Data const*> const& dd);
    void add_point(Point const& pt) { p.push_back(pt); };
    static std::string guess_file_type (std::string const& filename);
    fp get_x(int n) const { return p[active_p[n]].x; }
    fp get_y(int n) const { return p[active_p[n]].y; } 
    fp get_sigma (int n) const { return p[active_p[n]].sigma; }
    int get_n () const { return active_p.size(); }
    bool is_empty() const { return p.empty(); }
    bool has_any_info() const { return !is_empty() || !get_title().empty(); }
    fp get_x_step() const { return x_step; }
    void transform(const std::string &s);
    void update_active_p();
    //int auto_range (fp y_level, fp x_margin);
    std::string range_as_string () const;
    int get_lower_bound_ac (fp x) const; 
    int get_upper_bound_ac (fp x) const;
    std::string const& get_title() const {return title.empty()?filename:title;}
    std::string const& get_filename() const { return filename; }

    void recompute_y_bounds();
    fp get_y_at (fp x) const;
    //return points at x (if any) or (usually) after it.
    std::vector<Point>::const_iterator get_point_at(fp x) const;
    void export_to_file (std::string filename, bool append=false);
    fp get_x_min() { return p.empty() ? 0 : p.front().x; }
    fp get_x_max() { return p.empty() ? 180. : p.back().x; } 
    fp get_y_min() const { return p.empty() ? 0 : y_min; }
    fp get_y_max() const { return p.empty() ? 1e3 : y_max; }
    std::vector<Point> const& points() const { return p; }

    std::string title;
private:
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
    void load_xy_filetype (std::ifstream& f, std::vector<int> const& cols);
    void load_mca_filetype (std::ifstream& f);
    void load_rit_filetype (std::ifstream& f);
    void load_cpi_filetype (std::ifstream& f);
    void clear();
    void post_load();
};

#endif

