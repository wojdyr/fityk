// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef SUM__H__
#define SUM__H__
#include <vector>
#include <string>
#include <utility>
#include <memory>
#include "common.h"
#include "dotset.h"
#include "var.h"

class Function;

///  This class contains description of curve which we are trying to fit 
///  to data. This curve is described simply by listing names of functions
///  in F and in Z (in zero-shift)
class Sum : public DotSet
{
public:
    bool replot_needed;

    Sum(VariableManager *mgr_);
    ~Sum();
    void find_function_indices();
    void add_function_to(std::string const &name, char add_to);
    fp value(fp x) const;
    void calculate_sum_value(std::vector<fp> &x, std::vector<fp> &y) const; 
    void calculate_sum_value_deriv(std::vector<fp> &x, std::vector<fp> &y,
                                   std::vector<fp> &dy_da) const;

    fp funcs_value (const std::vector<int>& fn, fp x) const;

    fp value_and_put_deriv (fp x, std::vector<fp>& dy_da) const;
    fp value_and_add_numeric_deriv (fp x, bool both_sides, 
                                    std::vector<fp>& dy_da) const;
    fp approx_max(fp x_min, fp x_max);
    std::string general_info() const;
    std::string get_sum_formula() const;
    void export_to_file (std::string filename, bool append, char filetype);
    void export_as_script (std::ostream& os) const;
    fp get_def_rel_domain_width() const { return def_rel_domain_width; }
    std::vector<fp> get_numeric_derivatives(fp x, fp numerical_h) const;
    fp zero_shift (fp x) const;
    std::vector<int> const& get_ff_idx() { return ff_idx; }

private:
    fp cut_level;
    fp def_rel_domain_width; 
    VariableManager &mgr;
    std::vector<std::string> ff_names;
    std::vector<std::string> zz_names;
    std::vector<int> ff_idx;
    std::vector<int> zz_idx;

    void export_as_dat (std::ostream& os);
    void export_as_peaks(std::ostream& os) const;
    void export_as_xfit(std::ostream& os) const;
    Sum (const Sum&); //disable
    Sum& operator= (Sum&); //disable
};

extern Sum *my_sum;

#endif  

