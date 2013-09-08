// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__NMFIT__H__
#define FITYK__NMFIT__H__

#include <vector>
#include "common.h"
#include "fit.h"

namespace fityk {

/// Vertex used in Nelder-Mead method
struct Vertex
{
    std::vector<realt> a;
    bool computed;
    realt wssr;

    Vertex() : a(0), computed(false), wssr(0.) {}
    Vertex(int n) : a(n), computed(false), wssr(0.) {}
    Vertex(std::vector<realt>& a_) : a(a_), computed(false), wssr(0.) {}
};

///              Nelder-Mead simplex method
class NMfit : public Fit
{
public:
    NMfit(Full* F, const char* name) : Fit(F, name) {}
    virtual double run_method(std::vector<realt>* best_a);
private:
    std::vector<Vertex> vertices;
    std::vector<Vertex>::iterator best, s_worst /*second worst*/, worst;
    std::vector<realt> coord_sum;
    realt volume_factor;

    void init();
    void find_best_worst();
    void change_simplex();
    realt try_new_worst(realt f);
    void compute_coord_sum();
    bool termination_criteria(int iter, realt convergence);
    void compute_v(Vertex& v);
};

} // namespace fityk
#endif

