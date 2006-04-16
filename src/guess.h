// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__GUESS__H__
#define FITYK__GUESS__H__

/// simple "virtual" peak, used for rought estimations of what would happen 
/// if there was a peak with the given position, height and FWHM.
class VirtPeak 
{ 
// It has shape:               ; 
//                   ____      ;  
//                  /    \     ; 
//                 /      \    ;
public:
    VirtPeak() {}
    VirtPeak(fp center_, fp height_, fp fwhm_) 
        : center(center_), height(height_), fwhm(fwhm_)  {}
    fp get_approx_y(fp x) const; //get value of y at x, using assumed shape
private:
    fp center, height, fwhm; 
};


/// used for passing peak-estimation conditions 
/// to e.g. Manipul::estimate_peak_parameters() 
struct EstConditions
{
    //it says: imagine that only following peaks exist ...
    std::vector<int> real_peaks;
    // ... and that there are some additional peaks.
    std::vector<VirtPeak> virtual_peaks;
};

class DataWithSum;

void estimate_peak_parameters (DataWithSum const* ds,
                               fp range_from, fp range_to, 
                               fp *center, fp *height, fp *area, fp *fwhm,
                               EstConditions const* ec=0);
std::string print_simple_estimate(DataWithSum const* ds, fp center, fp w);
std::string print_multiple_peakfind(DataWithSum const* ds, int n, 
                                    std::vector<std::string> const& range);
void guess_and_add(DataWithSum* ds,
                   std::string const& name, std::string const& function,
                   std::vector<std::string> const& range,
                   std::vector<std::string> vars);

#endif

