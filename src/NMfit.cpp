// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "NMfit.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <vector>
#include <ui.h>

using namespace std;

NMfit::NMfit ()
    :Fit ('s', "Nelder-Mead simplex"), 
     min_rel_diff(0.0001), move_all(false), distrib_type('b'), move_mult(1.)
{
    default_max_iterations = 500;
    fpar["min-fract-range"] = &min_rel_diff;
    bpar["move-all"] = &move_all;
    epar.insert(pair<string, Enum_string>("distrib-type", 
                               Enum_string (Distrib_enum, &distrib_type)));
    fpar["move-multiplier"] = &move_mult;
}

NMfit::~NMfit() {}

fp NMfit::init()
{
    // 1. all n+1 vertices are the same
    Vertex v(a_orig);
    vertices = vector<Vertex> (na + 1, v);
    // 2. na of na+1 vertices has one coordinate changed; computing WSSR
    for (int i = 0; i < na; i++) {
        vertices[i + 1].a[i] = draw_a_from_distribution (i, distrib_type, 
                                                                    move_mult);
        if (move_all) {
            fp d2 = (vertices[i + 1].a[i] - vertices[0].a[i]) / 2;
            for (vector<Vertex>::iterator j = vertices.begin(); 
                                                    j != vertices.end(); j++)
                j->a[i] -= d2;
        }
    }
    for (vector<Vertex>::iterator i = vertices.begin(); i!=vertices.end(); i++)
        compute_v (*i);
    // 3.
    find_best_worst();
    compute_coord_sum();
    volume_factor = 1.;
    return best->wssr;
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
    for (vector<Vertex>::iterator i = vertices.begin(); i!=vertices.end() ;i++){
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

int NMfit::autoiter ()
{
    wssr_before = compute_wssr(a_orig);
    info ("WSSR before starting simplex fit: " + S(wssr_before));
    for (int iter = 0; !termination_criteria (iter); iter ++) {
        iteration_plot(best->a);
        iter_nr++;
        change_simplex();
        find_best_worst();
    }
    post_fit (best->a, best->wssr);
    return 0;
}

void NMfit::change_simplex()
{
    fp t = try_new_worst (-1.);
    if (t <= best->wssr)
        try_new_worst (2.);
    else if (t >= s_worst->wssr) {
        fp old = worst->wssr;
        fp t = try_new_worst(0.5);
        if (t >= old) { // than multiple contraction
            for (vector<Vertex>::iterator i = vertices.begin(); 
                                                    i != vertices.end() ;i++) {
                if (i == best)
                    continue;
                for (int j = 0; j < na; j++)
                    i->a[j] = (i->a[j] + best->a[j]) / 2;
                compute_v (*i);
                volume_factor *= 0.5;
            }
            compute_coord_sum();
        }
    }
}

fp NMfit::try_new_worst (fp f)
    //extrapolates by a factor f through the face of the simplex across
    //from the high point, tries it, 
    //and replaces the high point if the new point is better.
{
    Vertex t(na);
    fp f1 = (1 - f) / na;
    fp f2 = f1 - f;
    for (int i = 0; i < na; i++)
        t.a[i] = coord_sum[i] * f1 - worst->a[i] * f2;
    compute_v (t);
    if (t.wssr < worst->wssr) {
        for (int i = 0; i < na; i++)
            coord_sum[i] += t.a[i] - worst->a[i];
        *worst = t;
        volume_factor *= f;
    }
    return t.wssr;
}

void NMfit::compute_coord_sum()
{
    coord_sum.resize(na);
    fill (coord_sum.begin(), coord_sum.end(), 0.);
    for (int i = 0; i < na; i++)
        for (vector<Vertex>::iterator j = vertices.begin(); 
                                                j != vertices.end(); j++) 
            coord_sum[i] += j->a[i];
}

bool NMfit::termination_criteria (int iter)
{
    if (iter % output_one_of == 0) 
        info ("#" + S(iter_nr) + " (ev:" + S(evaluations) + "): best:" 
                + S(best->wssr) + " worst:" + S(worst->wssr) + ", " 
                + S(s_worst->wssr) + " [V * |" + S(volume_factor) + "|]");
    bool stop = false;
    if (volume_factor == 1. && iter != 0) {
        info ("Simplex got stuck.");
        stop = true;
    }
    volume_factor = 1.;

/*DEBUG - BEGIN* 
    string s = "WSSR:";
    for (vector<Vertex>::iterator i = vertices.begin(); i!=vertices.end(); i++)
        s += " " + S(i->wssr);
    info (s);
*DEBUG - END*/
    //checking stop conditions
    if (common_termination_criteria(iter))
        stop=true;
    fp r_diff = 2 * (worst->wssr - best->wssr) 
                                        / (best->wssr + worst->wssr + EPSILON);
    if (r_diff < min_rel_diff) {
        info ("Relative difference between worst and best vertex is only "
                + S(r_diff) + ". Stop");
        stop = true;
    }
    return stop;
}

void NMfit::compute_v (Vertex& v) 
{
    assert (!v.a.empty()); 
    v.wssr = compute_wssr(v.a); 
    v.computed = true;
}

