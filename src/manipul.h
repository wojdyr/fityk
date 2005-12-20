// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__MANIPUL__H__
#define FITYK__MANIPUL__H__

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
    //it says: imagine that only following peaks exist ...
    std::vector<int> real_peaks;
    // ... and that there are some additional peaks.
    std::vector<VirtPeak> virtual_peaks;
};


class Manipul
{
public:
    Manipul() {}
    bool estimate_peak_parameters (fp approx_ctr, fp ctrplusmin, 
                            fp *center, fp *height, fp *area, fp *fwhm,
                            const EstConditions *ec=0) const;
    std::string print_simple_estimate(fp center, fp w = -1.) const; 
    std::string print_multiple_peakfind(int n, 
                                        std::vector<std::string> const& range);
    void guess_and_add(std::string const& name, std::string const& function,
                       std::vector<std::string> const& range,
                       std::vector<std::string> vars);
private:
    fp my_y (int n, const EstConditions *ec=0) const;
    fp data_area (int from, int to, const EstConditions *ec=0) const;
    int max_data_y_pos (int from, int to, const EstConditions *ec=0) const;
    fp compute_data_fwhm (int from, int max_pos, int to, fp level,
                          const EstConditions *ec=0) const;
    void parse_range(std::vector<std::string> const& range, 
                     fp& range_from, fp& range_to);
};

extern Manipul *my_manipul;

#endif

