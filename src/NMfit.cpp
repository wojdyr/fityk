// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <vector>

#include "common.h"
#include "NMfit.h"
#include "logic.h"
#include "settings.h"

using namespace std;

void NMfit::init()
{
    bool move_all = F_->get_settings()->nm_move_all;
    char distrib = F_->get_settings()->nm_distribution[0];
    realt factor = F_->get_settings()->nm_move_factor;

    // 1. all n+1 vertices are the same
    Vertex v(a_orig_);
    vertices = vector<Vertex> (na_ + 1, v);
    // 2. na_ of na_+1 vertices has one coordinate changed; computing WSSR
    for (int i = 0; i < na_; ++i) {
        vertices[i + 1].a[i] = draw_a_from_distribution(i, distrib, factor);
        if (move_all) {
            realt d2 = (vertices[i + 1].a[i] - vertices[0].a[i]) / 2;
            for (vector<Vertex>::iterator j = vertices.begin();
                                                    j != vertices.end(); ++j)
                j->a[i] -= d2;
        }
    }
    for (vector<Vertex>::iterator i = vertices.begin(); i!=vertices.end(); ++i)
        compute_v (*i);
    // 3.
    find_best_worst();
    compute_coord_sum();
    volume_factor = 1.;
    //return best->wssr;
}

void NMfit::find_best_worst()
{
    // finding best, second best and worst vertex
    if (vertices[0].wssr < vertices[1].wssr) {
        worst = vertices.begin() + 1;
        s_worst = best = vertices.begin();
    }
    else {
        worst = vertices.begin();
        s_worst = best = vertices.begin() + 1;
    }
    for (vector<Vertex>::iterator i = vertices.begin(); i!=vertices.end(); ++i){
        if (i->wssr < best->wssr)
            best = i;
        if (i->wssr > worst->wssr) {
            s_worst = worst;
            worst = i;
        }
        else if (i->wssr > s_worst->wssr && i != worst)
            s_worst = i;
    }
}

void NMfit::autoiter()
{
    realt convergence = F_->get_settings()->nm_convergence;
    wssr_before_ = compute_wssr(a_orig_, dmdm_);
    F_->msg("WSSR before starting simplex fit: " + S(wssr_before_));
    for (int iter = 0; !termination_criteria(iter, convergence); ++iter) {
        ++iter_nr_;
        change_simplex();
        find_best_worst();
        iteration_plot(best->a, best->wssr);
    }
    post_fit (best->a, best->wssr);
}

void NMfit::change_simplex()
{
    realt t = try_new_worst (-1.);
    if (t <= best->wssr)
        try_new_worst (2.);
    else if (t >= s_worst->wssr) {
        realt old = worst->wssr;
        realt t = try_new_worst(0.5);
        if (t >= old) { // than multiple contraction
            for (vector<Vertex>::iterator i = vertices.begin();
                                                    i != vertices.end() ;++i) {
                if (i == best)
                    continue;
                for (int j = 0; j < na_; ++j)
                    i->a[j] = (i->a[j] + best->a[j]) / 2;
                compute_v (*i);
                volume_factor *= 0.5;
            }
            compute_coord_sum();
        }
    }
}

realt NMfit::try_new_worst(realt f)
    //extrapolates by a factor f through the face of the simplex across
    //from the high point, tries it,
    //and replaces the high point if the new point is better.
{
    Vertex t(na_);
    realt f1 = (1 - f) / na_;
    realt f2 = f1 - f;
    for (int i = 0; i < na_; ++i)
        t.a[i] = coord_sum[i] * f1 - worst->a[i] * f2;
    compute_v (t);
    if (t.wssr < worst->wssr) {
        for (int i = 0; i < na_; ++i)
            coord_sum[i] += t.a[i] - worst->a[i];
        *worst = t;
        volume_factor *= f;
    }
    return t.wssr;
}

void NMfit::compute_coord_sum()
{
    coord_sum.resize(na_);
    fill (coord_sum.begin(), coord_sum.end(), 0.);
    for (int i = 0; i < na_; ++i)
        for (vector<Vertex>::iterator j = vertices.begin();
                                                j != vertices.end(); ++j)
            coord_sum[i] += j->a[i];
}

bool NMfit::termination_criteria(int iter, realt convergence)
{
    if (F_->get_verbosity() >= 1)
        F_->ui()->mesg("#" + S(iter_nr_) + " (ev:" + S(evaluations_) + "):"
                       " best:" + S(best->wssr) +
                       " worst:" + S(worst->wssr) + ", " + S(s_worst->wssr) +
                       " [V * |" + S(volume_factor) + "|]");
    bool stop = false;
    if (volume_factor == 1. && iter != 0) {
        F_->msg ("Simplex got stuck.");
        stop = true;
    }
    volume_factor = 1.;

/*DEBUG - BEGIN*
    string s = "WSSR:";
    for (vector<Vertex>::iterator i = vertices.begin(); i!=vertices.end(); ++i)
        s += " " + S(i->wssr);
    F_->msg (s);
*DEBUG - END*/
    //checking stop conditions
    if (common_termination_criteria(iter))
        stop = true;
    if (is_zero(worst->wssr)) {
        F_->msg ("All vertices have WSSR < epsilon=" + S(epsilon));
        return true;
    }
    realt r_diff = 2 * (worst->wssr - best->wssr) / (best->wssr + worst->wssr);
    if (r_diff < convergence) {
        F_->msg ("Relative difference between worst and best vertex is only "
                 + S(r_diff) + ". Stop");
        stop = true;
    }
    return stop;
}

void NMfit::compute_v(Vertex& v)
{
    assert (!v.a.empty());
    v.wssr = compute_wssr(v.a, dmdm_);
    v.computed = true;
}

