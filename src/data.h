// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef DATA__H__
#define DATA__H__
#include <string>
#include <vector>
#include <fstream>
#include "common.h"
#include "dotset.h"

struct Simple_point
{
    fp x, y;
    Simple_point () : x(0), y(0) {}
    Simple_point (fp x_, fp y_) : x(x_), y(y_) {}
    std::string str() { return "(" + S(x) + "; " + S(y) + ")"; }
};
inline bool operator< (const Simple_point& p, const Simple_point& q) 
{ return p.x < q.x; }

struct B_point : public Simple_point 
{ 
    fp q; /* q is used for spline */ 
    B_point (fp x, fp y) : Simple_point(x, y) {}
};
    
struct Point : public Simple_point
{
    fp orig_x, orig_y, sigma;
    bool is_active;
    Point () : Simple_point(), orig_x (0), orig_y (0), sigma (0) {}
    Point (fp x_) : Simple_point(x_, 0), orig_x (0), orig_y (0), sigma (0) {}
    Point (fp x_, fp y_) : Simple_point(x_, y_), orig_x(x_), orig_y (y_), 
                           sigma (0) {}
    Point (fp x_, fp y_, fp sigma_) : Simple_point(x_, y_), 
                                      orig_x(x_), orig_y(y_), sigma(sigma_) {}
    fp get_bg() const { return orig_y - y; } 
    fp get_calibr() const { return orig_x - x; }
};

enum Bg_cl_enum { bg_ty = 0, cl_ty = 1 };

class Data : public DotSet
{
public :
    fp min_background_distance[2];
    bool spline_background[2];

    Data ();
    ~Data () {}
    void d_was_plotted() { d_was_changed = false; }
    bool was_changed() const { return d_was_changed; }
    std::string info() const;
    int load (std::string file, int type, 
              std::vector<int> usn, std::vector<int> evr, int merge);
    char guess_file_type (const std::string& filename);
    fp get_x (int n) const { return p[active_p[n]].x; }
    fp get_y (int n) const { return p[active_p[n]].y; } 
    fp get_original_x (int n) const { return p[active_p[n]].orig_x; }
    fp get_original_y (int n) const { return p[active_p[n]].orig_y; }
    fp get_background_y (int n) const { return p[active_p[n]].get_bg(); }
    fp get_sigma (int n) const { return p[active_p[n]].sigma; }
    int get_n () const {return active_p.size();}
    bool is_empty () const {return p.empty();}
    fp get_x_step() const {return x_step;}
    int change_range (fp left, fp right, bool state = true);
    int auto_range (fp y_level, fp x_margin);
    std::string range_as_string () const;
    int get_lower_bound_ac (fp x) const; 
    int get_upper_bound_ac (fp x) const;
    void change_sigma (char type = 'u', fp min = 1.);
    std::string print_sigma();
    std::string get_title() const { return title.empty() ? filename : title; }
    std::string get_filename() const { return filename; }

    void auto_background (int n, fp p1, bool is_proc1, fp p2, bool is_proc2);
    //background (modifying y)  or  calibration (modifying x) 
    void add_background_point (fp x, fp y, Bg_cl_enum bg_cl);
    void rm_background_point (fp x, Bg_cl_enum bg_cl);
    void clear_background (Bg_cl_enum bg_cl);
    std::string background_info (Bg_cl_enum bg_cl);
    void recompute_background (Bg_cl_enum bg_cl);
    void recompute_y_bounds();
    const std::vector<B_point>& get_background_points(Bg_cl_enum bg_cl)
        { return background[bg_cl]; }

    fp get_y_at (fp x) const;
    fp get_bg_at (fp x) const;
    //return points at x (if any) or (usually) after it.
    std::vector<Point>::const_iterator get_point_at(fp x) const;
    void export_to_file (std::string filename, bool append, char filetype);
    void export_as_script (std::ostream& os);
    fp get_x_min() { return p.front().x; }
    fp get_x_max() { return p.back().x; } 
    fp get_y_min (bool plus_bg) const {return plus_bg ? y_orig_min : y_min;}
    fp get_y_max (bool plus_bg) const {return plus_bg ? y_orig_max : y_max;}
    const std::vector<Point>& points() const {return p;}
    void add_point (const Point& pt);
private:
    bool d_was_changed;
    std::string title;
    std::string filename;
    int filetype;
                /*  0  unknown
                 *  1  x  y  
                 *  2  .mca
                 *  3  .rit  -> in 1.line: xmin xstep 
                 *              in next lines: y y y y y y y ....
                 *  4  Siemens-Bruker-AXS raw-files
                 */
    std::vector<int> col_nums;
    std::vector<int> every;
    std::vector<Point> merge_table;
    int every_idx, merging;
    fp x_step; // 0 if not fixed;
    std::vector<Point> p;
    std::vector<int> active_p;
    std::vector<B_point> background[2];
    bool background_infl_sigma;
    fp y_orig_min, y_orig_max, y_min, y_max;
    char sigma_type;
    fp sigma_minim;

    Data (Data&); //disable
    bool get_seq_num (std::istream &is, fp *num);
    int get_one_line_with_numbers (std::istream &is, 
                                    std::vector<fp>& result_numbers);
    void add_point_2nd_stage (const Point& pt);
    static double pdp11_f (char* fl);  
    fp find_step();
    void load_xy_filetype (std::ifstream& plik, std::vector<int>& usn);
    void load_mca_filetype (std::ifstream& plik);
    void load_rit_filetype (std::ifstream& plik);
    void load_cpi_filetype (std::ifstream& plik);
    void recompute_sigma();

    void spline_interpolation (Bg_cl_enum bg_cl);
    void linear_interpolation (Bg_cl_enum bg_cl);
    static std::vector<B_point>::iterator 
    bg_interpolation_find_place(std::vector<B_point> &background, fp x);
    static void prepare_spline_interpolation (std::vector<B_point> &background);

    void export_as_dat (std::ostream& os);
    void export_bg_as_dat (std::ostream& os);
};

extern Data *my_data;

#endif
