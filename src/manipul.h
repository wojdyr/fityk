// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef MANIPUL__H__
#define MANIPUL__H__

#include "dotset.h"

//class Data;
//class Sum;

// simple "virtual" peak, used for rought estimations, what would happen 
// if there were a peak with given position, height and FWHM.
// It has shape:     ___         ;
//                  /   \        ;
//                 /     \       ;
//
class VirtPeak 
{ 
public:
    VirtPeak() {}
    VirtPeak(fp center_, fp height_, fp fwhm_) 
        : center(center_), height(height_), fwhm(fwhm_)  {}
    fp get_approx_y(fp x) const; //get value of y at x, using assumed shape
private:
    fp center, height, fwhm; 
};


// struct used for passing peak-estimation conditions 
// to e.g. Manipul::estimate_peak_parameters() 
struct EstConditions
{
    EstConditions() {} 
    EstConditions(const std::vector<VirtPeak> &v,  const std::vector<int> &ip,
                  const std::vector<int> &ep)
        : incl_peaks(ip), excl_peaks(ep), virtual_peaks(v) {} 

    //it says: imagine that only following peaks exist (+ these virtual below)
    //empty == all peaks
    std::vector<int> incl_peaks;
    //it says: imagine that following peaks do not exist 
    std::vector<int> excl_peaks;
    //it says: imagine that there are some additional peaks...
    std::vector<VirtPeak> virtual_peaks;
};


class Manipul : public DotSet
{
public:
    Manipul();
    bool estimate_peak_parameters (fp approx_ctr, fp ctrplusmin, 
                            fp *center, fp *height, fp *area, fp *fwhm,
                            const EstConditions *ec=0) const;
    std::string print_simple_estimate (fp center, fp w = -1.) const; 
    std::string print_global_peakfind ();
    fp trapezoid_area_of_peaks (const std::vector<int> &peaks) const;
private:
    bool estimate_consider_sum;
    fp search_width;
    bool cancel_peak_out_of_search;
    fp height_correction, fwhm_correction;

    fp my_y (int n, const EstConditions *ec=0) const;
    fp data_area (int from, int to, const EstConditions *ec=0) const;
    int max_data_y_pos (int from, int to, const EstConditions *ec=0) const;
    fp compute_data_fwhm (int from, int max_pos, int to, fp level,
                          const EstConditions *ec=0) const;
};

extern Manipul *my_manipul;

#endif

