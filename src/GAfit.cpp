// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "GAfit.h"
#include "ui.h"
#include "settings.h"
#include "numfuncs.h"
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <numeric>
#include <deque>
#include <sstream>
#include <math.h>


using namespace std;

GAfit::GAfit()
   : Fit("Genetic-Algorithms"),
     popsize (100), elitism(0),
     mutation_type('u'), p_mutation(0.1), mutate_all_genes(false), 
     mutation_strength(0.1), crossover_type('u'), p_crossover(0.3),
     selection_type('r'), rank_scoring(false), tournament_size(2), 
     window_size(-1), 
     linear_scaling_a(1.), linear_scaling_c(2.), linear_scaling_b (1.),
     std_dev_stop(0), iter_with_no_progresss_stop(0),
     autoplot_indiv_nr(-1),
     pop(0), opop(0),
     best_indiv(0)
{
    /*
    irpar["population-size"] = IntRange (&popsize, 2, 9999);
    irpar["steady-size"] = IntRange (&elitism, 0, 9999);
    epar.insert(pair<string, Enum_string>("mutation-type", 
                               Enum_string (Distrib_enum, &mutation_type)));
    fpar["p-mutation"] = &p_mutation;
    bpar["mutate-all-genes"] = &mutate_all_genes;
    fpar["mutation-strength"] = &mutation_strength;
    */
    Crossover_enum ['u'] = "uniform";
    Crossover_enum ['o'] = "one-point";
    Crossover_enum ['t'] = "two-point";
    Crossover_enum ['a'] = "arithmetic1";
    Crossover_enum ['A'] = "arithmetic2";
    Crossover_enum ['g'] = "guaranteed-avg";
    /*
    epar.insert(pair<string, Enum_string>("crossover-type", 
                               Enum_string (Crossover_enum, &crossover_type)));
    fpar["p-crossover"] = &p_crossover;
    */
    Selection_enum ['r'] = "roulette";
    Selection_enum ['t'] = "tournament";
    Selection_enum ['s'] = "SRS";
    Selection_enum ['d'] = "DS";
    /*
    epar.insert (pair<string, Enum_string>("selection-type", 
                               Enum_string (Selection_enum, &selection_type)));
    bpar["rank-scoring"] = &rank_scoring;
    irpar["tournament-size"] = IntRange (&tournament_size, 2, 999);
    irpar["window-size"] = IntRange (&window_size, -1, 199);
    fpar["linear-scaling-a"] = &linear_scaling_a;
    fpar["linear-scaling-c"] = &linear_scaling_c;
    fpar["linear-scaling-b"] = &linear_scaling_b;
    fpar["rel-std-dev-stop"] = &std_dev_stop;
    ipar["iterations-with-no-progresss-stop"] = &iter_with_no_progresss_stop;
    fpar["wssr-stop"] = &wssr_stop;
    irpar["autoplot-indiv-nr"] = IntRange(&autoplot_indiv_nr, -1, 999999999);
    spar["GA-log-file"] = &log_file;
    */
}

GAfit::~GAfit() {}

fp GAfit::init()
{
    pop = &pop1;
    opop = &pop2;
    pop->resize (popsize);
    vector<Individual>::iterator best = pop->begin();
    for (vector<Individual>::iterator i = pop->begin(); i != pop->end(); i++) {
        i->g.resize(na);
        for (int j = 0; j < na; j++)  
            i->g[j] = draw_a_from_distribution(j);
        compute_wssr_for_ind (i);
        if (i->raw_score < best->raw_score)
            best = i;
    }
    best_indiv = *best;
    if (!log_file.empty())
        log_ga_options();
    return 0;
}

void GAfit::autoiter()
{
    wssr_before = compute_wssr(a_orig, datsums);
    info ("WSSR before starting GA: " + S(wssr_before));
    assert (pop && opop); 
    if (elitism >= popsize) {
        warn ("hmm, now elitism >= popsize, setting elitism = 1");
        elitism = 1;
    }
    for (int iter = 0; !termination_criteria_and_print_info(iter); iter++) {
        autoplot_in_autoiter();
        iter_nr ++;
        pre_selection();
        crossover();
        mutation();
        post_selection();
    }
    post_fit (best_indiv.g, best_indiv.raw_score);
}

void GAfit::log_ga_options()
{
    ofstream os(log_file.c_str(), ios::out | ios::app);
    if (!os) {
        warn ("Can't open file for appending: " + log_file);
        return;
    }
    os << "####### Current time: " << time_now() << endl;
    istringstream iss(getSettings()->set_script());
    string s;
    while (getline (iss, s)) 
        os << "# " << s << endl;
    os << endl << "# generation evaluations best avg worst std-dev avg-age"
        << endl;
}

void GAfit::compute_wssr_for_ind (vector<Individual>::iterator ind)
{
    ind->raw_score = compute_wssr(ind->g, datsums);
    ind->generation = iter_nr;
}

void GAfit::autoplot_in_autoiter()
{
    const Individual& indiv = is_index(autoplot_indiv_nr, *pop) 
                                    ? (*pop)[autoplot_indiv_nr] : best_indiv;
    iteration_plot(indiv.g);
}

void GAfit::mutation()
{   
    for (vector<Individual>::iterator i = pop->begin(); i != pop->end(); i++) {
        if (mutate_all_genes) {
            if (rand() < RAND_MAX * p_mutation) {
                for (int j = 0; j < na; j++)  
                    i->g[j] = draw_a_from_distribution(j, mutation_type,
                                                            mutation_strength);
                compute_wssr_for_ind (i);
            }
        }
        else
            for (int j = 0; j < na; j++)  
                if (rand() < RAND_MAX * p_mutation) {
                    i->g[j] = draw_a_from_distribution(j, mutation_type,
                                                            mutation_strength);
                    compute_wssr_for_ind (i);
                }
    }
}

void GAfit::crossover()
{
    for (vector<Individual>::iterator i = pop->begin(); i != pop->end(); i++) 
        if (rand() < RAND_MAX / 2 * p_crossover) {
            vector<Individual>::iterator i2 = pop->begin() + rand()%pop->size();
            switch (crossover_type) {
                case 'u':
                    uniform_crossover (i, i2);
                    break;
                case 'o':
                    one_point_crossover (i, i2);
                    break;
                case 't':
                    two_points_crossover (i, i2);
                    break;
                case 'a':
                    arithmetic_crossover1 (i, i2);
                    break;
                case 'A':
                    arithmetic_crossover2 (i, i2);
                    break;
                case 'g':
                    guaranteed_avarage_crossover (i, i2);
                    break;
                default:
                    warn ("No such crossover-type: " + S(crossover_type)
                            + ". Setting to 'u'");
                    crossover_type = 'u';
                    uniform_crossover (i, i2);
                    break;
            }
            compute_wssr_for_ind (i);
            compute_wssr_for_ind (i2);
        }
}

void GAfit::uniform_crossover (vector<Individual>::iterator c1, 
                               vector<Individual>::iterator c2)
{
    for (int i = 0; i < na; i++) 
        if (rand() % 2)
            swap(c1->g[i], c2->g[i]);
}

void GAfit::one_point_crossover (vector<Individual>::iterator c1, 
                                 vector<Individual>::iterator c2)
{
    int p = rand() % na;
    for (int j = 0; j < p; j++)
            swap(c1->g[j], c2->g[j]);
}

void GAfit::two_points_crossover (vector<Individual>::iterator c1, 
                                  vector<Individual>::iterator c2)
{
    int p1 = rand() % na;
    int p2 = rand() % na;
    for (int j = min(p1, p2); j < max(p1, p2); j++) 
            swap(c1->g[j], c2->g[j]);
}

void GAfit::arithmetic_crossover1 (vector<Individual>::iterator c1, 
                                   vector<Individual>::iterator c2)
{
    fp a = rand_0_1();
    for (int j = 0; j < na; j++) {
        c1->g[j] = a * c1->g[j] + (1 - a) * c2->g[j]; 
        c2->g[j] = (1 - a) * c1->g[j] + a * c2->g[j]; ;
    }
}

void GAfit::arithmetic_crossover2 (vector<Individual>::iterator c1, 
                                   vector<Individual>::iterator c2)
{
    for (int j = 0; j < na; j++) {
        fp a = rand_0_1();
        c1->g[j] = a * c1->g[j] + (1 - a) * c2->g[j]; 
        c2->g[j] = (1 - a) * c1->g[j] + a * c2->g[j]; ;
    }
}

void GAfit::guaranteed_avarage_crossover (vector<Individual>::iterator c1, 
                                          vector<Individual>::iterator c2)
{
    for (int j = 0; j < na; j++) 
        c1->g[j] = c2->g[j] = (c1->g[j] + c2->g[j]) / 2; 
}

void GAfit::pre_selection()
{
    vector<int> next(popsize - elitism);
    switch (selection_type) {
        case 'r':
            scale_score();
            roulette_wheel_selection(next);
            break;
        case 't':
            tournament_selection(next);
            break;
        case 's':
            scale_score();
            stochastic_remainder_sampling(next);
            break;
        case 'd':
            scale_score();
            deterministic_sampling_selection(next);
            break;
        default:
            warn ("No such selection-type: " + S((char)selection_type)
                    + ". Setting to 'r'");
            selection_type = 'r';
            pre_selection ();
            return;
    }
    opop->resize(next.size(), Individual(na));
    for (int i = 0; i < size(next); i++)
        (*opop)[i] = (*pop)[next[i]];
    swap (pop, opop);
}

void GAfit::post_selection()
{
    if (elitism == 0)
        return;
    do_rank_scoring (opop);
    for (vector<Individual>::iterator i = opop->begin(); i != opop->end(); i++)
       if (i->phase_2_score < elitism) 
           pop->push_back (*i); 
    assert (size(*pop) == popsize);
}

struct ind_raw_sc_cmp 
{
    bool operator() (Individual* a, Individual* b) {
        return a->raw_score < b->raw_score;
    }
};
        
void GAfit::do_rank_scoring(std::vector<Individual> *popp)
{
    // rank in population is assigned to phase_2_score
    // e.g. 0 - the best, 1 - second, (popp.size() - 1) - worst
    static vector<Individual*> ind_p;
    ind_p.resize(popp->size());
    for (unsigned int i = 0; i < popp->size(); i++) 
        ind_p[i] = &(*popp)[i];
    sort (ind_p.begin(), ind_p.end(), ind_raw_sc_cmp());
    for (unsigned int i = 0; i < popp->size(); i++)
        ind_p[i]->phase_2_score = i;
}

void GAfit::roulette_wheel_selection(vector<int>& next)
{
    vector<unsigned int> roulette(pop->size()); //preparing roulette
    unsigned int t = 0;
    for (int i = 0; i < size(*pop) - 1; i++) {
        t += static_cast<unsigned int>
            ((*pop)[i].norm_score * RAND_MAX / size(*pop));
        roulette[i] = t;
    }                                 
    roulette[size(*pop) - 1] = RAND_MAX; //end of preparing roulette
    for (vector<int>::iterator i = next.begin(); i != next.end(); i++) 
        *i = lower_bound (roulette.begin(), roulette.end(), 
                         static_cast<unsigned int>(rand())) - roulette.begin();
}

void GAfit::tournament_selection(vector<int>& next)
{
    for (vector<int>::iterator i = next.begin(); i != next.end(); i++) {
        int best = rand() % pop->size();
        for (int j = 1; j < tournament_size; j++) {
            int n = rand() % pop->size();
            if ((*pop)[n].raw_score < (*pop)[best].raw_score)
                best = n;
        }
        *i = best;
    }
}

vector<int>::iterator 
GAfit::SRS_and_DS_common (vector<int>& next)
{
    vector<int>::iterator r = next.begin();
    fp f = 1.0 * next.size() / pop->size(); // rescaling for steady-state
    for (unsigned int i = 0; i < pop->size(); i++) {
        int n = static_cast<int>((*pop)[i].norm_score * f);
        fill (r, min (r + n, next.end()), i);
        r += n;
    }
    return min (r, next.end());
}

void GAfit::stochastic_remainder_sampling(vector<int>& next)
{
    vector<int>::iterator r = SRS_and_DS_common (next);
    if (r == next.end())
        return;
    vector<int> rest_of_next (next.end() - r);
    copy (rest_of_next.begin(), rest_of_next.end(), r);
}

struct Remainder_and_ptr {
    int ind;
    fp r;
    bool operator< (const Remainder_and_ptr &b) {
        return r < b.r;
    }
};

void GAfit::deterministic_sampling_selection(vector<int>& next)
{
    vector<int>::iterator r = SRS_and_DS_common (next);
    if (r == next.end())
        return;
    static vector<Remainder_and_ptr> rem;
    rem.resize(pop->size());
    for (unsigned int i = 0; i < pop->size(); i++) {
        rem[i].ind = i;
        fp x = (*pop)[i].norm_score;
        rem[i].r = x - floor(x);
    }
    int rest = next.end() - r;
    partial_sort (rem.begin(), rem.begin() + rest, rem.end());
    for (int i = 0; i < rest; i++, r++)
        *r = rem[i].ind;
    assert (r == next.end());
}

void GAfit::scale_score () //return value - are individuals varying?
{
    if (rank_scoring)
        do_rank_scoring(pop);
    else
        for (vector<Individual>::iterator i = pop->begin(); i !=pop->end(); i++)
            i->phase_2_score = i->raw_score;

    //scaling p -> q - p; p -> p / <p>
    fp q = max_in_window();
    if (q < 0) 
        q = std_dev_based_q();
    q += linear_scaling_b;
    fp sum = 0;
    for (vector<Individual>::iterator i = pop->begin(); i != pop->end(); i++) {
        i->reversed_score = max (q - i->phase_2_score, 0.);
        sum += i->reversed_score;
    }
    if (sum == 0) //to avoid x/0
        return;
    fp avg_rev_sc = sum / pop->size();
    for (vector<Individual>::iterator i = pop->begin(); i != pop->end(); i++) 
        i->norm_score = i->reversed_score / avg_rev_sc;
}

fp GAfit::std_dev_based_q()
{
    fp sum_p = 0, sum_p2 = 0;
    for (vector<Individual>::iterator i = pop->begin(); i != pop->end(); i++) {
        sum_p += i->phase_2_score;
        sum_p2 += i->phase_2_score * i->phase_2_score;
    }
    fp avg_p2 = sum_p2 / pop->size();
    fp avg_p = sum_p / pop->size();
    fp sq_sigma = avg_p2 - avg_p * avg_p;
    fp sigma = sq_sigma > 0 ? sqrt (sq_sigma) : 0;  //because of problem with
    return linear_scaling_a * avg_p + linear_scaling_c * sigma; // sqrt(0) 
}

fp GAfit::max_in_window ()
{
    // stores the worst raw_score in every of last hist_len generations
    // return the worst (max) raw_score in last window_size generations
    const int hist_len = 200;
    static deque<fp> max_raw_history (hist_len, 0);
    max_raw_history.push_front (tmp_max);
    max_raw_history.pop_back();
    assert (window_size <= hist_len);
    if (window_size > 0) {
        if (rank_scoring)
            return size(*pop) - 1;
        else 
            return *max_element (max_raw_history.begin(), 
                                 max_raw_history.begin() + window_size);
    }
    else
        return -1;
}

bool GAfit::termination_criteria_and_print_info (int iter)
{
    static int no_progress_iters = 0;
    fp sum = 0;
    fp min = pop->front().raw_score;
    tmp_max = min;
    vector<Individual>::iterator ibest = pop->begin();
    for (vector<Individual>::iterator i = pop->begin(); i != pop->end(); i++) {
        if (i->raw_score < min) {
            min = i->raw_score;
            ibest = i;
        }
        if (i->raw_score > tmp_max)
            tmp_max = i->raw_score;
        sum += i->raw_score;
    }
    fp avg = sum / pop->size();
    fp sq_sum = 0;
    fp generations_sum = 0;
    for (vector<Individual>::iterator i = pop->begin(); i != pop->end(); i++) {
        fp d = i->raw_score - avg;
        sq_sum += d * d;
        generations_sum += i->generation;
    }
    fp std_dev = sq_sum > 0 ? sqrt (sq_sum / pop->size()) : 0;
    info ("Population #" + S(iter_nr) + ": best " + S(min) 
                + ", avg " + S(avg) + ", worst " + S(tmp_max) 
                + ", std dev. " + S(std_dev));
    if (!log_file.empty()) {
        ofstream os(log_file.c_str(), ios::out | ios::app);
        if (os) {
            os << iter_nr << "  " << evaluations << "  " 
               << min << "  " << avg << "  " << tmp_max << "  " << std_dev 
               << "  " << iter_nr - generations_sum / pop->size() << endl;
        }
    }
    if (min < best_indiv.raw_score) {
        best_indiv = *ibest;
        no_progress_iters = 0;
    }
    else
        no_progress_iters ++;

    //checking stop conditions
    bool stop = false;

    if (common_termination_criteria(iter)) 
        stop = true;
    if (std_dev < std_dev_stop * avg) {
        info ("Standard deviation of results is small enough to stop");
        stop = true;
    }
    if (iter_with_no_progresss_stop > 0
          && no_progress_iters >= iter_with_no_progresss_stop) {
        info ("No progress in " + S(no_progress_iters) + " iterations. Stop");
        stop = true;
    }
    if (min <= wssr_stop) {
        info ("WSSR is small enough to stop");
        stop = true;
    }
    return stop;
}


