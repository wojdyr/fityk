// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__DATA__H__
#define FITYK__DATA__H__
#include <string>
#include <vector>
#include <limits.h>
#include "common.h"

#include "fityk.h" // struct Point, FITYK_API

namespace fityk {

class Ftk;

FITYK_API std::string get_file_basename(std::string const& path);

/// dataset
class FITYK_API Data
{
public :
    static int count_blocks(const std::string& fn,
                            const std::string& format,
                            const std::string& options);
    static int count_columns(const std::string& fn,
                             const std::string& format,
                             const std::string& options,
                             int first_block);

    Data(const Ftk* F)
        : F_(F), given_x_(INT_MAX), given_y_(INT_MAX), given_s_(INT_MAX),
                  x_step_(0.) {}
    ~Data() {}
    std::string get_info() const;

    void load_file(const std::string& fn,
                   int idx_x, int idx_y, int idx_s,
                   const std::vector<int>& blocks,
                   const std::string& format, const std::string& options);

    int load_arrays(const std::vector<realt>& x, const std::vector<realt>& y,
                    const std::vector<realt>& sigma,
                    const std::string& data_title);
    //void load_data_sum(const std::vector<const Data*>& dd,
    //                   const std::string& op);
    void set_points(const std::vector<Point>& p);
    void clear();
    //void add_point(Point const& pt) { p_.push_back(pt); }; //don't use it
    void add_one_point(realt x, realt y, realt sigma);
    realt get_x(int n) const { return p_[active_[n]].x; }
    realt get_y(int n) const { return p_[active_[n]].y; }
    realt get_sigma (int n) const { return p_[active_[n]].sigma; }
    int get_n() const { return active_.size(); }
    std::vector<realt> get_xx() const;
    bool is_empty() const { return p_.empty(); }
    bool has_any_info() const { return !is_empty() || !get_title().empty(); }
    double get_x_step() const { return x_step_; } /// 0.0 if not fixed
    void after_transform(); // update x_step_, active_
    std::string range_as_string () const;
    int get_lower_bound_ac (double x) const;
    int get_upper_bound_ac (double x) const;
    const std::string& get_title() const { return title_; }
    void set_title(const std::string& title) { title_ = title; }
    const std::string& get_filename() const { return filename_; }

    void find_step();
    void sort_points();

    // update active points bookkeeping
    void update_active_p();
    // quick change in active points bookkeeping
    void update_active_for_one_point(int idx);
    void append_point() { size_t n = p_.size(); p_.resize(n+1);
                                                active_.push_back(n); }
    //double get_y_at (double x) const;
    // return points at x (if any) or (usually) after it.
    std::vector<Point>::const_iterator get_point_at(double x) const;
    double get_x_min() const;
    double get_x_max() const;
    std::vector<Point> const& points() const { return p_; }
    std::vector<Point>& get_mutable_points() { return p_; }
    int get_given_x() const { return given_x_; }
    int get_given_y() const { return given_y_; }
    int get_given_s() const { return given_s_; }
    void revert();

private:
    const Ftk* F_;
    std::string title_;
    std::string filename_;
    int given_x_, given_y_, given_s_;/// columns given when loading the file
    std::vector<int> given_blocks_;
    std::string given_format_;
    std::string given_options_;
    double x_step_; // 0.0 if not fixed;
    bool has_sigma_;
    std::vector<Point> p_;
    std::vector<int> active_;

    void post_load();
    DISALLOW_COPY_AND_ASSIGN(Data);
};

inline std::vector<realt> Data::get_xx() const
{
    std::vector<realt> xx(get_n());
    for (size_t j = 0; j < xx.size(); ++j)
        xx[j] = get_x(j);
    return xx;
}

} // namespace fityk
#endif

