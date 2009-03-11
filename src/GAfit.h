// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

#ifndef FITYK__GAFIT__H__
#define FITYK__GAFIT__H__
#include <vector>
#include "common.h"
#include "fit.h"

/// Individual in Genetic Algorithms (i.e. candidate solutions)
struct Individual 
{ 
    std::vector<fp> g; 
    fp raw_score, phase_2_score, reversed_score, norm_score; 
    int generation;
    Individual (int n) : g(n), raw_score(0) {}
    Individual () : g(), raw_score(0) {}
};

/// Genetic Algorithm method
class GAfit : public Fit
{
public:
    GAfit(Ftk* F);
    ~GAfit();
    fp init(); // called before autoiter()
    void autoiter();
private:
    int popsize;
    int elitism; // = 0, 1, ... popsize
    char mutation_type;
    fp p_mutation;
    bool mutate_all_genes;
    fp mutation_strength;
    char crossover_type;
    fp p_crossover;
    char selection_type;
    bool rank_scoring;
    int tournament_size;
    int window_size;
    fp linear_scaling_a, linear_scaling_c, linear_scaling_b;
    fp std_dev_stop;
    int iter_with_no_progresss_stop;
    fp wssr_stop;
    int autoplot_indiv_nr;
    std::vector<Individual> pop1, pop2, *pop, *opop;
    int iteration;
    Individual best_indiv;
    fp tmp_max;
    std::map<char, std::string> Crossover_enum;
    std::map<char, std::string> Selection_enum;

    void mutation();
    void crossover();
    void uniform_crossover (std::vector<Individual>::iterator c1, 
                            std::vector<Individual>::iterator c2);
    void one_point_crossover (std::vector<Individual>::iterator c1, 
                              std::vector<Individual>::iterator c2);
    void two_points_crossover (std::vector<Individual>::iterator c1, 
                               std::vector<Individual>::iterator c2);
    void arithmetic_crossover1 (std::vector<Individual>::iterator c1, 
                                std::vector<Individual>::iterator c2);
    void arithmetic_crossover2 (std::vector<Individual>::iterator c1, 
                                std::vector<Individual>::iterator c2);
    void guaranteed_avarage_crossover (std::vector<Individual>::iterator c1, 
                                       std::vector<Individual>::iterator c2);
    void scale_score ();
    void pre_selection();
    void post_selection();
    fp max_in_window ();
    fp std_dev_based_q();
    void do_rank_scoring(std::vector<Individual> *popp);
    void roulette_wheel_selection (std::vector<int>& next);
    void tournament_selection (std::vector<int>& next);
    void stochastic_remainder_sampling(std::vector<int>& next);
    void deterministic_sampling_selection(std::vector<int>& next);
    std::vector<int>::iterator SRS_and_DS_common (std::vector<int>& next);
    bool termination_criteria_and_print_info (int iter);
    void print_post_fit_info (fp wssr_before);
    void autoplot_in_autoiter();
    void compute_wssr_for_ind (std::vector<Individual>::iterator ind);
}; 

#endif 

