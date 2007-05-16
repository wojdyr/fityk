// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__DATA__H__
#define FITYK__DATA__H__
#include <string>
#include <vector>
#include <fstream>
#include "common.h"

#include "fityk.h" // struct Point
using fityk::Point;
using fityk::operator<;

class Fityk;

/// dataset
class Data 
{
public :
    std::string title;

    Data(Fityk const* F_) : F(F_), y_min(0.), y_max(1e3) {}
    ~Data() {}
    std::string getInfo() const;
    void load_file (std::string const& file, std::string const& type, 
                    std::vector<int> const& cols, bool preview=false);
    int load_arrays(std::vector<fp> const& x, std::vector<fp> const& y, 
                   std::vector<fp> const& sigma, std::string const& data_title);
    void load_data_sum(std::vector<Data const*> const& dd, 
                       std::string const& op);
    void clear();
    void add_point(Point const& pt) { p.push_back(pt); }; //don't use it
    void add_one_point(double x, double y, double sigma);
    static std::string guess_file_type (std::string const& filename);
    fp get_x(int n) const { return p[active_p[n]].x; }
    fp get_y(int n) const { return p[active_p[n]].y; } 
    fp get_sigma (int n) const { return p[active_p[n]].sigma; }
    int get_n () const { return active_p.size(); }
    bool is_empty() const { return p.empty(); }
    bool has_any_info() const { return !is_empty() || !get_title().empty(); }
    fp get_x_step() const { return x_step; } /// 0.0 if not fixed
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
    void export_to_file(std::string filename, std::vector<std::string>const &vt,
                        std::vector<std::string> const& ff_names,
                        bool append=false);
    fp get_x_min() const { return p.empty() ? 0 : p.front().x; }
    fp get_x_max() const { return p.empty() ? 180. : p.back().x; } 
    fp get_y_min() const { return y_min; }
    fp get_y_max() const { return y_max; }
    std::vector<Point> const& points() const { return p; }
    std::string get_given_type() const { return given_type; }
    std::vector<int> get_given_cols() const { return given_cols; }
    static std::string read_one_line_as_title(std::ifstream& f, int column=0);
private:
    Fityk const* F;
    std::string filename;
    std::string given_type; // filetype explicitely given when loading the file
    std::vector<int> given_cols; /// columns given when loading the file
    fp x_step; // 0.0 if not fixed;
    bool has_sigma;
    std::vector<Point> p;
    std::vector<int> active_p;
    fp y_min, y_max;

    Data (Data&); //disable
    bool get_seq_num (std::istream &is, fp *num);
    int read_line_and_get_all_numbers(std::istream &is, 
                                      std::vector<fp>& result_numbers);
    static double pdp11_f (char* fl);  
    fp find_step();
    void load_xy_filetype (std::ifstream& f, std::vector<int> const& cols);
    void load_header_xy_filetype(std::ifstream& f, std::vector<int>const& cols);
    void load_mca_filetype (std::ifstream& f);
    void load_rit_filetype (std::ifstream& f);
    void load_cpi_filetype (std::ifstream& f);
    void post_load();
    void open_filename_with_columns(std::string const& file, std::ifstream& f);
};

#endif

