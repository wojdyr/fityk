// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef SUM__H__
#define SUM__H__
#include <vector>
#include <string>
#include <set>
#include <utility>
#include <memory>
#ifdef USE_HASHMAP
#include <hash_map>
#endif
#include "common.h"
#include "dotset.h"
#include "pag.h"

/*      
 *      This class contains description of curve (sum of f-functions)
 *      which we are trying to fit to data.
 */

class Parameters;
class PagContainer;
class FuncContainer;
class V_fzg;

class Sum : public DotSet
{
public:
    Sum(Parameters *params); 
    ~Sum(); 
    void s_was_plotted() { s_was_changed = false; }
    bool was_changed() const { return s_was_changed; }
    void use_param_a_for_value (const std::vector<fp>& A = fp_v0) const;
    fp value (fp x) const;
    fp f_value (fp x, int fn) const;
    fp funcs_value (const std::vector<int>& fn, fp x) const;
    fp value_and_put_deriv (fp x, std::vector<fp>& dy_da) const;
    fp value_and_add_numeric_deriv (fp x, bool both_sides, 
                                    std::vector<fp>& dy_da) const;
    fp get_ffunc_deriv (int f, fp x, std::vector<fp>& dy_da);
    std::string general_info() const;
    std::string sum_full_formula (const std::vector<fp>& localA = fp_v0) const;
    std::string sum_of_peaks_formula (const std::vector<int>& peaks, 
                                  const std::vector<fp>& localA = fp_v0) const;
    std::string print_sum_value (fp x, bool with_deriv = true);
    std::string test_g (int g_num);
    void export_to_file (std::string filename, bool append, char filetype, 
                                      const std::vector<int> &peaks = int_v0);
    void export_as_script (std::ostream& os) const;
    void register_fc (FuncContainer* cnt, bool flag);
    void register_pc (PagContainer* vp, bool flag);
    bool is_remove_recursive() const { return recursive_remove; }
    fp get_def_rel_domain_width() const { return def_rel_domain_width; }

/*** fzg common ***/

    int fzg_size (One_of_fzg fzg) const;
    int add_fzg (One_of_fzg fzg, char type, const std::vector<Pag> &p,
                 const std::string &desc = "");
    std::string info_fzg (One_of_fzg fzg, int n, bool extra=false) const;
    std::string info_fzg_parameters(One_of_fzg fzg, int n, bool only_val) const;
    int rm_fzg (One_of_fzg fzg, int n, bool silent = false);
    int multi_rm_fzg (One_of_fzg fzg, std::vector<int> nn, bool silent = false);
    int rm_all();
    const V_fzg *get_fzg (One_of_fzg fzg, int n) const;
    std::string print_fzg_value (One_of_fzg fzg, int n, fp x = 0, 
                                 bool with_deriv = true);

/*** f-func ***/
    const V_f* get_f(int n) { assert(n>=0 && n<size(fvec)); return fvec[n]; }
    void change_f (int f, char type);
    void change_in_f (int f, std::vector<int> arg, std::vector<Pag> np);
    void guess_f(int n); 

/*** zero-shift ***/
    fp zero_shift (fp x) const;
    int find_zshift (char type) const;

/*** g-parameter ***/
    Pag add_g (char gtype, const std::vector<Pag> &params, 
               const std::string &desc = "");
    const std::vector<V_g*>& gfuncs_vec() const { return gvec; }

/*** return Parameters ***/
    const Parameters* pars() const { return parameters; } 
    Parameters* pars() { return parameters; } 

/*** references to... ***/
    int refs_to_ag (Pag p) const;
    int refs_to_f (int n) const;
    std::string descr_refs_to_ag (Pag p) const;
    std::string descr_refs_to_f (int n) const;
    std::string descr_refs_to_fzg (One_of_fzg fzg, int n) const;
/** will be changed **/
    void synch_after_rm_f (int n);
    void synch_after_rm_ag (Pag p);

private:
    bool s_was_changed;
    std::vector <V_f*> fvec;
    std::vector <V_z*> zvec;
    std::vector <V_g*> gvec;
    Parameters *parameters;

    bool cut_tails;
    fp cut_level;
    bool numerical_deriv;
    fp numerical_h;
    bool two_side;
    bool recursive_remove;
    fp def_rel_domain_width; 
    mutable std::vector<fp> av_numder;
    std::set<FuncContainer*> f_usage;
    std::set<PagContainer*> vp_usage;
#ifdef USE_HASHMAP
    std::hash_map<fp, fp> sum_buffer
#endif

    void notify_change();
    std::string format_der_output(const std::vector<fp>& df_s, 
                   const std::vector<fp>& df_n1, const std::vector<fp>& df_n2);
    void export_fzg_as_script (std::ostream& os, One_of_fzg fzg) const;
    //TODO int remove_spare_brackets(std::string);

    void export_as_dat (std::ostream& os, const std::vector<int>&peaks=int_v0);
    void export_as_peaks (std::ostream& os, const std::vector<int>&peaks) const;
    void export_as_xfit(std::ostream& os, 
                        const std::vector<int> &peaks=int_v0) const;
    Sum (const Sum&); //disable
    Sum& operator= (Sum&); //disable
};

extern Sum *my_sum;

#endif  

