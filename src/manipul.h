// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef MANIPUL__H__
#define MANIPUL__H__

#include "dotset.h"

//class Data;
//class Sum;

struct VirtPeak { fp center, height, fwhm; };

// struct used for passing peak-estimation conditions 
// to e.g. Manipul::estimate_peak_parameters() 
struct EstConditions
{
    //it says: imagine that there are some additional peaks...
    std::vector<VirtPeak> virtual_peaks;
    //it says: imagine that only following peaks exist (+ these virtual above)
    std::vector<int> peaks;
    EstConditions() {} 
    EstConditions(const std::vector<VirtPeak> &v,  const std::vector<int> &p)
        : virtual_peaks(v), peaks(p) {} 
};

class Manipul : public DotSet
{
public:
    Manipul();
    bool estimate_peak_parameters (fp approx_ctr, fp ctrplusmin, 
                            fp *center, fp *height, fp *area, fp *fwhm,
                            EstConditions *ec=0) const;
    std::string print_simple_estimate (fp center, fp w = -1.) const; 
    std::string print_global_peakfind ();
    fp trapezoid_area_of_peaks (const std::vector<int> &peaks) const;
private:
    bool estimate_consider_sum;
    fp search_width;
    bool cancel_peak_out_of_search;
    fp height_correction, fwhm_correction;

    fp my_y (int n, EstConditions *ec=0) const;
    fp data_area (int from, int to, EstConditions *ec=0) const;
    int max_data_y_pos (int from, int to, EstConditions *ec=0) const;
    fp compute_data_fwhm (int from, int max_pos, int to, fp level,
                          EstConditions *ec=0) const;
};

extern Manipul *my_manipul;

#endif

