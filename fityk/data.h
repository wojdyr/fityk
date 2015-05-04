// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__DATA__H__
#define FITYK__DATA__H__
#include <string>
#include <vector>
#include <limits.h>
#include <utility>
#include "common.h"

#include "fityk.h" // struct Point, FITYK_API

namespace xylib { class DataSet; }

namespace fityk {

class BasicContext;
class Model;

FITYK_API std::string get_file_basename(std::string const& path);

/// dataset
class FITYK_API Data
{
public :
    static int count_blocks(const std::string& filename,
                            const std::string& format,
                            const std::string& options);
    static int count_columns(const std::string& filename,
                             const std::string& format,
                             const std::string& options,
                             int first_block);

    Data(BasicContext *ctx, Model *model);
    ~Data();
    std::string get_info() const;

    void load_file(const LoadSpec& spec);

    int load_arrays(const std::vector<realt>& x, const std::vector<realt>& y,
                    const std::vector<realt>& sigma,
                    const std::string& title);
    //void load_data_sum(const std::vector<const Data*>& dd,
    //                   const std::string& op);
    void set_points(const std::vector<Point>& p);
    void clear();
    void add_one_point(realt x, realt y, realt sigma);
    realt get_x(int n) const { return p_[active_[n]].x; }
    realt get_y(int n) const { return p_[active_[n]].y; }
    realt get_sigma (int n) const { return p_[active_[n]].sigma; }
    int get_n() const { return active_.size(); }
    std::vector<realt> get_xx() const;
    bool is_empty() const { return p_.empty(); }
    bool completely_empty() const;
    bool has_any_info() const;
    double get_x_step() const { return x_step_; } /// 0.0 if not fixed
    void after_transform(); // update x_step_, active_
    std::string range_as_string() const;
    std::pair<int,int> get_index_range(const RealRange& range) const;
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
    // return points at x (if any) or (usually) after it.
    std::vector<Point>::const_iterator get_point_at(double x) const;
    double get_x_min() const;
    double get_x_max() const;
    std::vector<Point> const& points() const { return p_; }
    std::vector<Point>& get_mutable_points() { return p_; }
    int get_given_x() const { return spec_.x_col; }
    int get_given_y() const { return spec_.y_col; }
    int get_given_s() const { return spec_.sig_col; }
    void revert();
    Model* model() { return model_; }
    const Model* model() const { return model_; }

private:
    const BasicContext* ctx_;
    Model* const model_;
    std::string title_;
    std::string filename_;
    LoadSpec spec_; // given when loading file
    double x_step_; // 0.0 if not fixed;
    bool has_sigma_;
    std::vector<Point> p_;
    std::vector<int> active_;

    void post_load();
    void verify_options(const xylib::DataSet* ds, const std::string& options);
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

