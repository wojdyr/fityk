// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__DATA__H__
#define FITYK__DATA__H__
#include <string>
#include <vector>
#include <fstream>
#include <limits.h>
#include "common.h"

#include "fityk.h" // struct Point
using fityk::Point;
using fityk::operator<;

class Ftk;
namespace xylib { class DataSet; }

std::string get_file_basename(std::string const& path);

/// dataset
class Data
{
public :
    std::string title;
    static int count_blocks(std::string const& fn,
                            std::string const& format,
                            std::string const& options);
    static int count_columns(std::string const& fn,
                             std::string const& format,
                             std::string const& options,
                             int first_block);

    Data(Ftk const* F_)
        : F(F_), given_x_(INT_MAX), given_y_(INT_MAX), given_s_(INT_MAX),
                  x_step_(0.), y_min_(0.), y_max_(1e3) {}
    ~Data() {}
    std::string get_info() const;

    void load_file(std::string const& fn,
                   int idx_x, int idx_y, int idx_s,
                   std::vector<int> const& blocks,
                   std::string const& format, std::string const& options);

    int load_arrays(std::vector<fp> const& x, std::vector<fp> const& y,
                   std::vector<fp> const& sigma, std::string const& data_title);
    void load_data_sum(std::vector<Data const*> const& dd,
                       std::string const& op);
    void clear();
    void add_point(Point const& pt) { p_.push_back(pt); }; //don't use it
    void add_one_point(double x, double y, double sigma);
    fp get_x(int n) const { return p_[active_p_[n]].x; }
    fp get_y(int n) const { return p_[active_p_[n]].y; }
    fp get_sigma (int n) const { return p_[active_p_[n]].sigma; }
    int get_n() const { return active_p_.size(); }
    bool is_empty() const { return p_.empty(); }
    bool has_any_info() const { return !is_empty() || !get_title().empty(); }
    fp get_x_step() const { return x_step_; } /// 0.0 if not fixed
    void transform(const std::string &s);
    void after_transform(); // update x_step_, active_p_, y_min_, y_max_
    void update_active_p();
    //int auto_range (fp y_level, fp x_margin);
    std::string range_as_string () const;
    int get_lower_bound_ac (fp x) const;
    int get_upper_bound_ac (fp x) const;
    std::string const& get_title() const { return title; }
    std::string const& get_filename() const { return filename_; }

    void recompute_y_bounds();
    fp get_y_at (fp x) const;
    //return points at x (if any) or (usually) after it.
    std::vector<Point>::const_iterator get_point_at(fp x) const;
    fp get_x_min() const;
    fp get_x_max() const;
    fp get_y_min() const { return y_min_; }
    fp get_y_max() const { return y_max_; }
    std::vector<Point> const& points() const { return p_; }
    std::vector<Point>& get_mutable_points() { return p_; }
    int get_given_x() const { return given_x_; }
    int get_given_y() const { return given_y_; }
    int get_given_s() const { return given_s_; }
    static std::string read_one_line_as_title(std::ifstream& f, int column=-1);
    void revert();

private:
    Ftk const* F;
    std::string filename_;
    int given_x_, given_y_, given_s_;/// columns given when loading the file
    std::vector<int> given_blocks_;
    std::string given_format_;
    std::string given_options_;
    fp x_step_; // 0.0 if not fixed;
    bool has_sigma_;
    std::vector<Point> p_;
    std::vector<int> active_p_;
    fp y_min_, y_max_;

    Data (Data&); //disable
    fp find_step();
    void load_xy_filetype (std::ifstream& f, std::vector<int> const& cols);
    void post_load();
    /// open file with columns embedded into filename (foo.xy:1,2),
    // returns next files, if `file' stands for a sequance
    //std::vector<std::string>
    //open_filename_with_columns(std::string const& file, std::ifstream& f);
};

#endif

