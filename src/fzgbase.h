// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FZGBASE__H__
#define FZGBASE__H__

#include "common.h"
#include "pag.h"

static const fp TINY = 1e-10;

class Sum;

class PFContainer
{
public:
    std::string c_name; //container's name, eg. "^3" or  "%1(002)"  
    std::string c_description;

    PFContainer (Sum *s) : sum(s) {} 
    void bye_from_sum (Sum* s) { assert(s == sum); sum = 0; }
protected: 
    Sum* sum;
};

class PagContainer : public PFContainer
{
public:
    PagContainer (Sum *s); 
    PagContainer (Sum *s, const std::vector<Pag> &v);
    PagContainer (const PagContainer& p);
    ~PagContainer ();
    PagContainer& operator= (const PagContainer& r);
    void synch_after_rm (Pag p);
    int count_refs (Pag p) const; 
    std::string pag_str (int n) const { return n >= 0 && n < size(pags) ? 
                                                          pags[n].str() : S(); }
    const std::vector<Pag>& copy_of_pags() const { return pags; }
    std::vector<fp> values_of_pags() const;
    void recursive_rm ();
    void clear() { pags.clear(); }//remove items from container, not delete them
    bool is_ok (const Sum* with_sum = 0) const;
    int pags_size() const { return pags.size(); }
    Pag get_pag(int n) const {assert(n >= 0 && n < size(pags)); return pags[n];}
protected: 
    std::vector<Pag> pags;
};

class FuncContainer : public PFContainer 
{
public:
    FuncContainer(Sum *s, const std::vector<int> &f);
    FuncContainer(const FuncContainer& v); 
    ~FuncContainer();
    FuncContainer& operator= (const FuncContainer& r);
    void synch_after_rm_f (int fn);
    int count_refs (int fn) const;
    const std::vector<int> &get_ff() const { return ff; } 
    bool is_ok (const Sum *with_sum) const;
protected:
    std::vector<int> ff;
};

struct z_names_type 
{ 
    char type; 
    std::string name; 
    int psize;
    z_names_type (char t, std::string n, int s) : type(t), name(n), psize(s) {}
};

class V_fzg: public PagContainer
{
public:
    const char type;
    const int g_size; //number of parameters

    V_fzg(Sum* sum, const std::vector<Pag> &v, char t, One_of_fzg fzg) 
        : PagContainer(sum, v), type(t), g_size(type_info(fzg, t)->psize), 
          av(v.size()), der(v.size()){}
    virtual ~V_fzg() {}
    bool init_ok() const { return size(pags) == g_size; }
    static V_fzg* add (One_of_fzg fzg, Sum *sum, char type, 
                              const std::vector<Pag>& p);
    virtual std::string formula (const std::vector<fp>& A = fp_v0, 
                         const std::vector<V_g*>& G = std::vector<V_g*>(0)) = 0;
    virtual One_of_fzg fzg_of () const = 0;
    static const z_names_type* type_info(One_of_fzg fzg, char t);
    const z_names_type* type_info() const { return type_info(fzg_of(), type); }
    static std::string full_type (One_of_fzg fzg, bool with_symbol = true);
    std::string full_type() const 
        { return full_type(fzg_of()) + S(type_info()->type); }
    static std::string short_type (One_of_fzg fzg);
    std::string short_type() const { return short_type(fzg_of()); }
    static One_of_fzg type_of_symbol (char symbol); 
    static std::vector<const z_names_type*> all_types(One_of_fzg t);
    static std::string print_type_info (One_of_fzg fzg, char t);

protected:
    std::vector<fp> av;
    std::vector<fp> der;

    void prepare_av (const std::vector<fp>& A, const std::vector<V_g*>& G); 
    std::string ps(int n, const std::vector<fp>& A, const std::vector<V_g*>& G);
};


#endif

