// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef MANIPUL__H__
#define MANIPUL__H__

#include "dotset.h"

class Data;
class Sum;

struct VirtPeak { fp center, height, fwhm; };
class Manipul : public DotSet
{
public:
    Manipul (Data *data_, Sum *sum_);
    fp my_y (int n) const;
    fp data_area (int from, int to) const;
    int max_data_y_pos (int from, int to) const;
    fp compute_data_fwhm (int from, int max_pos, int to, fp level) const;
    bool estimate_peak_parameters (fp approx_ctr, fp ctrplusmin, 
                            fp *center, fp *height, fp *area, fp *fwhm) const;
    bool global_peakfind (fp *center, fp *height, fp *area, fp *fwhm);
    std::string print_simple_estimate (fp center, fp w = -1.) const; 
    std::string print_global_peakfind ();
    fp trapezoid_area_of_peaks (std::vector<int> peaks) const;
private:
    Data *data; 
    Sum *sum;
    bool estimate_consider_sum;
    fp search_width;
    bool cancel_peak_out_of_search;
    fp height_correction, fwhm_correction;
    std::vector<VirtPeak> virtual_peaks;
};

extern Manipul *my_manipul;

#endif

