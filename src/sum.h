// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef SUM__H__
#define SUM__H__
#include <vector>
#include <string>
#include <set>
#include <utility>
#include <memory>
#include "common.h"
#include "dotset.h"
#include "pag.h"

/*      
 *      This class contains description of curve (sum of f-functions)
 *      which we are trying to fit to data.
 */

class Domain 
{ 
    bool set, ctr_set;
    fp ctr, sigma; 

public:
    Domain () : set(false), ctr_set(false) {}
    Domain (fp sigm) : set(true), ctr_set(false), sigma(sigm) {}
    Domain (fp c, fp sigm) : set(true), ctr_set(true), ctr(c), sigma(sigm) {}
    Domain (pre_Domain &p) : set(p.set), ctr_set(p.ctr_set), //used in parser.y
                             ctr(p.ctr), sigma(p.sigma) {}
    bool is_set() const { return set; }
    bool is_ctr_set() const { return ctr_set; }
    fp Ctr() const { assert(set && ctr_set); return ctr; }
    fp Sigma() const { assert(set); return sigma; }
    std::string str() const 
        { return set ? "[" + (ctr_set ? S(ctr) : S()) 
                                         + " +- " + S(sigma) + "]" : S(""); }
};

struct History_item 
{ 
    std::vector<fp> a; 
    std::string comment; 
    bool saved;
    History_item (std::vector<fp> a_, std::string c) : a(a_), comment(c), 
                                                       saved(false) {}
};

class PagContainer;
class FuncContainer;
class V_fzg;

class Sum : public DotSet
{
public:
    Sum(); 
    ~Sum(); 
    void s_was_plotted() { s_was_changed = false; }
    bool was_changed() const { return s_was_changed; }
    void use_param_a_for_value (const std::vector<fp>& A = fp_v0);
    fp value (fp x) const;
    fp f_value (fp x, int fn) const;
    fp funcs_value (std::vector<int>& fn, fp x) const;
    fp value_and_put_deriv (fp x, std::vector<fp>& dy_da);
    fp value_and_add_numeric_deriv (fp x, bool both_sides, 
                                    std::vector<fp>& dy_da);
    fp get_ffunc_deriv (int f, fp x, std::vector<fp>& dy_da);
    std::string general_info() const;
    std::string sum_full_formula (const std::vector<fp>& localA = fp_v0) const;
    std::string sum_of_peaks_formula (std::vector<int>& peaks, 
                                  const std::vector<fp>& localA = fp_v0) const;
    std::string print_sum_value (fp x, bool with_deriv = true);
    std::string test_g (int g_num);
    void export_to_file (std::string filename, bool append, char filetype, 
                                            std::vector<int> &peaks = int_v0);
    void export_as_script (std::ostream& os) const;
    void register_fc (FuncContainer* cnt, bool flag);
    void register_pc (PagContainer* vp, bool flag);
    bool is_remove_recursive() const { return recursive_remove; }
    fp get_def_rel_domain_width() const { return def_rel_domain_width; }

/*** fzg common ***/

    int fzg_size (One_of_fzg fzg) const;
    int add_fzg (One_of_fzg fzg, char type, const std::vector<Pag> &p,
                 const std::string &desc = "");
    std::string Sum::info_fzg (One_of_fzg fzg, int n) const;
    std::string info_fzg_parameters(One_of_fzg fzg, int n, bool only_val) const;
    int rm_fzg (One_of_fzg fzg, int n, bool silent = false);
    int multi_rm_fzg (One_of_fzg fzg, std::vector<int> nn, bool silent = false);
    int Sum::rm_all();
    const V_fzg *get_fzg (One_of_fzg fzg, int n) const;
    V_fzg *get_fzg_m (One_of_fzg fzg, int n)
        { return const_cast<V_fzg*>(get_fzg (fzg, n)); }
    std::string print_fzg_value (One_of_fzg fzg, int n, fp x = 0, 
                                 bool with_deriv = true);

/*** f-func ***/
    void change_f (int f, char type);
    void change_in_f (int f, std::vector<int> arg, std::vector<Pag> np);

/*** zero-shift ***/
    fp zero_shift (fp x) const;
    int find_zshift (char type) const;

/*** g-parameter ***/
    Pag add_g (char gtype, const std::vector<Pag> &params, 
               const std::string &desc = "");
    const std::vector<V_g*>& gfuncs_vec() const { return gvec; }

/*** history of a-parameter ***/
    std::string print_history() const;
    std::string history_diff (std::vector<int> hist_items) const;
    void move_in_history (int k, bool relative);
    void toggle_history_item_saved(int k);
    int history_size() { return history.size(); }
    int history_position() { return hp; }
    const History_item& history_item (int nr) 
                { assert (nr >= 0 && nr < size(history)); return history[nr]; }
    void write_avec (const std::vector<fp>& a, std::string comment, 
                                                        bool no_move = false);
    const std::vector<fp>& current_a() const { return history[hp].a; }
/*** a-parameter ***/
    int count_a() const { return nA; }
    fp get_a(int n) const { assert(0 <= n && n < nA); return current_a()[n]; }
    Pag add_a (fp value, Domain d);
    std::string info_a (int nr) const;
    int rm_a (int nr, bool silent = false);
    bool rm_a_core (int nr);
    const Domain& get_domain(int n) const 
        { assert (0 <= n && n < nA); return adomain[n]; }
    fp change_a (int nr, fp value, char c = '=', bool add_to_history = true); 
                // returns old value        //c: +,-,*,/,=,% (105 % == 1.05 *)
    void change_domain (int nr, Domain d);
    fp variation_of_a (int n, fp variat) const;
    int freeze(int nr, bool frozen);
    bool is_frozen (int n) const {assert(0 <= n && n < nA); return afrozen[n];}
    std::string frozen_info () const;
    int count_frozen() const;

/*** a and g common ***/
/** will be changed **/
    int refs_to_ag (Pag p) const;
    int refs_to_f (int n) const;
    std::string descr_refs_to_ag (Pag p) const;
    std::string descr_refs_to_f (int n) const;
    std::string descr_refs_to_fzg (One_of_fzg fzg, int n) const;
    
private:
    bool s_was_changed;
    std::vector <V_f*> fvec;
    std::vector <V_z*> zvec;
    std::vector <V_g*> gvec;
    int nA;
    std::vector <History_item> history;
    int hp; //current position in history
    std::vector<Domain> adomain;
    std::vector<bool> afrozen;
    bool cut_tails;
    fp cut_level;
    bool numerical_deriv;
    fp numerical_h;
    bool two_side;
    bool recursive_remove;
    fp def_rel_domain_width; 
    std::vector<fp> av_numder;
    std::set<FuncContainer*> f_usage;
    std::set<PagContainer*> vp_usage;

    std::string format_der_output(const std::vector<fp>& df_s, 
                   const std::vector<fp>& df_n1, const std::vector<fp>& df_n2);
    void export_fzg_as_script (std::ostream& os, One_of_fzg fzg) const;
    //TODO int remove_spare_brackets(std::string);
/** will be changed **/
    void synch_after_rm_ag (Pag p);
    void synch_after_rm_f (int n);

    void export_as_dat (std::ostream& os, std::vector<int>&peaks=int_v0);
    void export_as_peaks (std::ostream& os, std::vector<int> &peaks) const;
    void export_as_xfit(std::ostream& os, std::vector<int>&peaks=int_v0) const;
    Sum (const Sum&); //disable
    Sum& operator= (Sum&); //disable
};

extern Sum *my_sum;

#endif  

