// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
/// virtual machine for dataset transformations (@n = ...)

#define BUILDING_LIBFITYK
#include "transform.h"
#include "logic.h"
#include "data.h"
#include "root/background.hpp"
#include <stdio.h>
#include <assert.h>

using namespace std;

namespace {
using namespace fityk;

struct DtStackItem
{
    bool is_num;
    realt num;
    vector<Point> points;
    string title;
};

realt find_extrapolated_y(vector<Point> const& pp, realt x)
{
    if (pp.empty())
        return 0;
    else if (x <= pp.front().x)
        return pp.front().y;
    else if (x >= pp.back().x)
        return pp.back().y;
    vector<Point>::const_iterator i = lower_bound(pp.begin(), pp.end(),
                                                  Point(x, 0));
    assert(i > pp.begin() && i < pp.end());
    if (is_eq(x, i->x))
        return i->y;
    else
        return (i-1)->y + (i->y - (i-1)->y) * (i->x - x) / (i->x - (i-1)->x);
}

void merge_same_x(vector<Point> &pp, bool avg)
{
    int count_same = 1;
    double x0 = 0; // 0 is assigned only to avoid compiler warnings
    for (int i = pp.size() - 2; i >= 0; --i) {
        if (count_same == 1)
            x0 = pp[i+1].x;
        if (is_eq(pp[i].x, x0)) {
            pp[i].x += pp[i+1].x;
            pp[i].y += pp[i+1].y;
            pp[i].sigma += pp[i+1].sigma;
            pp[i].is_active = pp[i].is_active || pp[i+1].is_active;
            pp.erase(pp.begin() + i+1);
            count_same++;
            if (i > 0)
                continue;
            else
                i = -1; // to change pp[0]
        }
        if (count_same > 1) {
            pp[i+1].x /= count_same;
            if (avg) {
                pp[i+1].y /= count_same;
                pp[i+1].sigma /= count_same;
            }
            count_same = 1;
        }
    }
}

void shirley_bg(vector<Point> &pp)
{
    /* Set criteria to stop itertions */
    const int max_iter = 100; 
    const double max_rdiff = 1e-6;  // implement: check for differences
    const unsigned long n = pp.size();

    /* The user has to set an appropriate region around the peak since the outcome is sensitive to that */
    /* Determine the range for the shirley background calculation from the active range in the dataset
     * the shirley BG will be calculated between the first and last active datapoint */
    unsigned long startXindex = 0;
    unsigned long endXindex = n-1;

    for (unsigned long i = 0; i < n; i++) {
        if( pp[i].is_active==true){
            startXindex = i;
            break;
        }
    }

    for (unsigned long i = endXindex; i > 0; i--) {
        if( pp[i].is_active==true){
            endXindex = i;
            break;
        }
    }

    /* To set ymin and ymax, we assume that we are in BE representation and Alow belongs to smaller BE and Ahigh to higher BE */

    /* Straight forward determination of ymin and ymax, but this can result in poor results when the SNR is high. */
    //double ymin = pp[startXindex].y;
    //double ymax = pp[endXindex].y;

    /* To improve the behaviour for bad SNR we average up to 5 y values around the x-index*/
    double ymin = 0.00001;
    double ymax = 0.00001;

    int count_start = 0;
    int count_end = 0;

    for(unsigned long i=0; i<5; i++){
        /* check if argument is within the range of the array indices */
        if( (int(startXindex+i)-2 >= 0) && ( startXindex+i-2 < n) ){
            ymin = ymin + pp[startXindex+i-2].y;
            count_start++;
        }

        /* check if argument is within the range of the array indices */
        if( (int(endXindex+i)-2 >= 0) && ( endXindex+i-2 < n) ){
            ymax = ymax + pp[endXindex+i-2].y;
            count_end++;
        }
    }
    /* divide by number of summations */
    if(count_start>0 && count_end>0){
        ymin=ymin/double(count_start);
        ymax=ymax/double(count_end);
    }

    /* Determine minimum element from data to obtain a reasonable starting value for the fit */
    double yminElement = ymin;
    for (unsigned long i = 0; i < n; i++) {
        if( pp[i].y < yminElement){
            yminElement =  pp[i].y;
        }
    }
    /* create starting background */
    std::vector<double> BG(n, yminElement);

    /* set initial BG values by assuming a linear background of the form  y = a* x + y0*/
    double divisor =  1.0; 
    if( (endXindex - startXindex) > 0){
        divisor = (double)(endXindex - startXindex);
    }

    double initial_slope = (pp[endXindex].y - pp[startXindex].y) / divisor;
    
    for (unsigned long EnergyIdx = 0; EnergyIdx < n ;  EnergyIdx++){
      
        if (EnergyIdx < startXindex){
            BG[EnergyIdx] = pp[startXindex].y;
        }   

        if (EnergyIdx >= startXindex && EnergyIdx <= endXindex){
            BG[EnergyIdx] = initial_slope * (EnergyIdx-startXindex) + pp[startXindex].y;
        }

        if (EnergyIdx > endXindex){
            BG[EnergyIdx] = pp[endXindex].y;
        }     

    } 

    /* recursive determination of the shirley background */
    for (int iter = 0; iter < max_iter; iter++) {

        /* Determine all background BG(Energy) values in dependence of the binding energy */
        for (unsigned long EnergyIdx = 0; EnergyIdx < n ;  EnergyIdx++){

            double Alow = 0.0;
            double Ahigh = 0.0;

            /* calculate Areas A1 and A2 for each energy value */
            for (unsigned long x = startXindex+1; x <= EnergyIdx; x++){

                Alow = Alow + ((pp[x].x-pp[x-1].x)*(pp[x].y+pp[x-1].y)/2.0) - BG[x]*(pp[x].x-pp[x-1].x);

            }

            for (unsigned long x = EnergyIdx+1; x < endXindex; x++){

                Ahigh = Ahigh + ((pp[x].x-pp[x-1].x)*(pp[x].y+pp[x-1].y)/2.0) - BG[x]*(pp[x].x-pp[x-1].x);

            }

            /* The formula is implemented as given in the CasaXPS manual (2006) "Peak fitting in XPS" chapter */
            BG[EnergyIdx] = ymin + (ymax-ymin) * (Alow/(Ahigh+Alow));

        }
    }

    /* copy the resulting background */
    for (unsigned long i = 0; i < n; i++){
        pp[i].y = BG[i];
    }
}



// Calculates the SNIP background iteratively in the
// given slice of the vector of points.
// The active points are modified in-place, inactive
// points are left as they are, assuming that they
// are already considered background.
vector<Point>::iterator snip_bg_slice(vector<Point>::iterator begin,
                                      vector<Point>::iterator end,
                                      int window_width,
                                      int direction,
                                      int filter_order,
                                      bool smoothing,
                                      bool smooth_window,
                                      bool estimate_compton)
{
    // Determine the first active slice of the data points.
    while (begin != end && !begin->is_active)
        ++begin;
    if (begin == end)
        return end;
    for (vector<Point>::iterator it = begin + 1; it != end; ++it)
        if (!it->is_active) {
            end = it;
            break;
        }
    if (end <= begin)  // just in case
        return end;

    vector<double> bg_input(end - begin);
    for (vector<Point>::iterator it = begin; it != end; it++)
        bg_input[it - begin] = it->y;

    vector<double> bg_output = ROOT::background(bg_input,
                                                window_width,
                                                direction,
                                                filter_order,
                                                smoothing,
                                                smooth_window,
                                                estimate_compton);

    if (bg_output.size() == bg_input.size()) {
        for (vector<Point>::iterator it = begin; it != end; it++)
            it->y = bg_output[it - begin];
    }

    return end;
}

} // anonymous namespace

namespace fityk {

/// executes VM code and stores results in dataset `data_out'
void run_data_transform(const DataKeeper& dk, const VMData& vm, Data* data_out)
{
    DtStackItem stack[6];
    DtStackItem* stackPtr = stack - 1; // will be ++'ed first

    v_foreach (int, i, vm.code()) {
        switch (*i) {

            case OP_NUMBER:
                stackPtr += 1;
                if (stackPtr - stack >= 6)
                    throw ExecuteError("stack overflow");
                ++i;
                stackPtr->is_num = true;
                stackPtr->num = vm.numbers()[*i];
                break;

            case OP_DATASET:
                stackPtr += 1;
                if (stackPtr - stack >= 6)
                    throw ExecuteError("stack overflow");
                stackPtr->is_num = false;
                ++i;
                stackPtr->points = dk.data(*i)->points();
                stackPtr->title = dk.data(*i)->get_title();
                if (stackPtr->title.empty())
                    stackPtr->title = "nt"; // no title
                break;

            case OP_NEG:
                if (stackPtr->is_num)
                    stackPtr->num = -stackPtr->num;
                else {
                    vm_foreach (Point, j, stackPtr->points)
                        j->y = -j->y;
                    stackPtr->title = "-" + stackPtr->title;
                }
                break;

            case OP_ADD:
                stackPtr -= 1;
                if (stackPtr->is_num && (stackPtr+1)->is_num)
                    stackPtr->num += (stackPtr+1)->num;
                else if (!stackPtr->is_num && !(stackPtr+1)->is_num) {
                    vm_foreach (Point, j, stackPtr->points)
                        j->y += find_extrapolated_y((stackPtr+1)->points, j->x);
                    stackPtr->title += "+" + (stackPtr+1)->title;
                } else
                    throw ExecuteError("adding number and dataset");
                break;

            case OP_SUB:
                stackPtr -= 1;
                if (stackPtr->is_num && (stackPtr+1)->is_num)
                    stackPtr->num -= (stackPtr+1)->num;
                else if (!stackPtr->is_num && !(stackPtr+1)->is_num) {
                    vm_foreach (Point, j, stackPtr->points)
                        j->y -= find_extrapolated_y((stackPtr+1)->points, j->x);
                    stackPtr->title += "-" + (stackPtr+1)->title;
                } else
                    throw ExecuteError("substracting number and dataset");
                break;

            case OP_MUL:
                stackPtr -= 1;
                if (stackPtr->is_num && (stackPtr+1)->is_num)
                    stackPtr->num *= (stackPtr+1)->num;
                else if (!stackPtr->is_num && !(stackPtr+1)->is_num) {
                    throw ExecuteError("multiplying two datasets");
                } else if (!stackPtr->is_num && (stackPtr+1)->is_num) {
                    realt mult = (stackPtr+1)->num;
                    vm_foreach (Point, j, stackPtr->points)
                        j->y *= mult;
                    stackPtr->title += "*" + S(mult);
                } else if (stackPtr->is_num && !(stackPtr+1)->is_num) {
                    realt mult = stackPtr->num;
                    stackPtr->points.swap((stackPtr+1)->points);
                    stackPtr->is_num = false;
                    vm_foreach (Point, j, stackPtr->points)
                        j->y *= mult;
                    stackPtr->title = S(mult) + "*" + (stackPtr+1)->title;
                }
                break;

            case OP_DT_SUM_SAME_X:
                if (stackPtr->is_num)
                    throw ExecuteError(op2str(*i) + " is defined only for @n");
                merge_same_x(stackPtr->points, false);
                break;

            case OP_DT_AVG_SAME_X:
                if (stackPtr->is_num)
                    throw ExecuteError(op2str(*i) + " is defined only for @n");
                merge_same_x(stackPtr->points, true);
                break;

            case OP_DT_SHIRLEY_BG:
                if (stackPtr->is_num)
                    throw ExecuteError(op2str(*i) + " is defined only for @n");
                shirley_bg(stackPtr->points);
                break;

            case OP_DT_SNIP_BG: {
                stackPtr -= 4;

                if ((stackPtr)->is_num)
                    throw ExecuteError(op2str(*i) + " is defined only for @n");
                int window_width = (stackPtr+1)->num;
                int direction = (stackPtr+2)->num >= 0
                                ? ROOT::kBackIncreasingWindow
                                : ROOT::kBackDecreasingWindow;
                int filter_order = (stackPtr+3)->num;
                bool smoothing = false;
                int smooth_window = ROOT::kBackSmoothing3;
                bool estimate_compton = (stackPtr+4)->num > 0 ? true : false;

                vector<Point>::iterator start = stackPtr->points.begin();
                while (start != stackPtr->points.end())
                    start = snip_bg_slice(start,
                                          stackPtr->points.end(),
                                          window_width,
                                          direction,
                                          filter_order,
                                          smoothing,
                                          smooth_window,
                                          estimate_compton);
                break;
            }

            case OP_AND:
                // do nothing
                break;

            case OP_AFTER_AND:
                stackPtr -= 1;
                if (stackPtr->is_num || (stackPtr+1)->is_num)
                    throw ExecuteError("expected @n on both sides of `and'");
                stackPtr->points.insert(stackPtr->points.end(),
                                        (stackPtr+1)->points.begin(),
                                        (stackPtr+1)->points.end());
                sort(stackPtr->points.begin(), stackPtr->points.end());
                stackPtr->title += "&" + (stackPtr+1)->title;
                break;

            default:
                throw ExecuteError("op " + op2str(*i) +
                               " is not allowed in dataset transformations.");
        }
    }
    assert(stackPtr == stack);

    if (!stackPtr->is_num) {
        data_out->set_points(stackPtr->points);
        data_out->set_title(stackPtr->title);
    } else if (stackPtr->num == 0.)
        data_out->clear();
    else
        throw ExecuteError("dataset or 0 expected on RHS");
}

} // namespace fityk
 
