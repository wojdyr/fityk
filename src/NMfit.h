// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef NMfit__h__
#define NMfit__h__

#include <vector>
#include "common.h"
#include "fit.h"

/*     this class contains Nelder-Mead simplex method
 *     description of method: Numerical Recipes, www.nr.com
 */

struct Vertex
{
    std::vector<fp> a;
    bool computed;
    fp wssr;

    Vertex() : a(0), computed(false) {}
    Vertex(int n) : a(n), computed(false) {}
    Vertex (std::vector<fp>& a_) : a(a_), computed(false) {}
};

class NMfit : public Fit
{
public:
    NMfit ();
    ~NMfit ();
    fp init(); // called before autoiter()
    int autoiter ();
private:
    int iteration;
    std::vector<Vertex> vertices;
    std::vector<Vertex>::iterator best, s_worst /*second worst*/, worst;
    std::vector<fp> coord_sum;
    fp min_rel_diff;
    bool move_all; 
    char distrib_type;
    fp move_mult;
    fp volume_factor;

    void find_best_worst();
    void change_simplex();
    fp try_new_worst (fp f);
    void compute_coord_sum();
    bool termination_criteria (int iter);
    void compute_v (Vertex& v); 
};

#endif 

