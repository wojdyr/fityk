// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FUNCS__H__
#define FUNCS__H__
#include <math.h>
#include <string>
#include <vector>
#include "common.h"
#include "pag.h"
#include "fzgbase.h"

    /*        f-functions and zero-shift
     */


struct ParDefault 
{ 
    bool is_set, lower_set, upper_set;
    fp p; 
    fp lower, upper; 
    ParDefault(): is_set(false), lower_set(false), upper_set(false) {}
    ParDefault(fp p_): is_set(true), lower_set(false), upper_set(false), p(p_){}
    ParDefault(fp p_, fp l, fp u): is_set(true), lower_set(true), 
                                 upper_set(true), p(p_), lower(l), upper(u) {}
};

struct int_int_fp { int a_nr; fp factor; int der_nr; };

struct f_names_type : z_names_type
{ 
    const char *formula; 
    const char **pnames;
    const std::vector<ParDefault>& pdefaults;
    f_names_type (char t, std::string n, int s, const char *f, const char **p,
                  const std::vector<ParDefault> &pd) 
        : z_names_type (t, n, s), formula(f), pnames(p), pdefaults(pd) {}
};

class V_f_z_common : public V_fzg
{
public:
    //V_f_z_common (Sum* sum, int n_s) : vector_Pag(sum), av(n_s) {}
    V_f_z_common (Sum* sum, const std::vector<Pag>& v, char t, One_of_fzg fzg) 
        : V_fzg (sum, v, t, fzg) {}
    virtual ~V_f_z_common() {}
    void pre_compute (const std::vector<fp>& A, const std::vector<V_g*>& G);
    void pre_compute_value_only (const std::vector<fp>& A, 
                                 const std::vector<V_g*>& G);
    inline void put_deriv (std::vector<fp>& dy_da);

protected:
    std::vector<int_int_fp> multi;

    virtual void f_val_precomputations() {}
};

inline void V_f_z_common::put_deriv (std::vector<fp>& dy_da) 
{
    //pre: compute (x, dy_dx) with dy_dx != NULL
    //     was called just before calling this function
    //     or compute_deriv (x, dy_dx) for V_z
    //  V_f::compute() computes value, prepares derivatives and dy_dx  
    //  V_z::compute_deriv() prepares derivatives using dy_dx and x 
    for (std::vector<int_int_fp>::const_iterator i = multi.begin(); 
            i != multi.end(); i++)
        dy_da[i->a_nr] += i->factor * der[i->der_nr];
}

class V_f : public V_f_z_common
{
 public:
    fp range_l, range_r; // for cut-tails
    static std::string xstr;
    static const f_names_type f_names[];

    V_f(Sum* sum, char t, const std::vector<Pag>& g) 
        : V_f_z_common(sum, g, t, fType) {} 
    virtual ~V_f() {}
    One_of_fzg fzg_of() const { return fType; }

    virtual void get_range (fp level);
    virtual bool is_peak() const { return true; }
    virtual fp center() const { return 0; } 
    virtual fp area() const { return 0; }
    virtual fp height() const { return 0; }
    virtual fp fwhm() const { return 0; }
    std::string extra_description() const;
    virtual fp compute(fp x, fp* dy_dx) = 0;
    Pag change_arg(int n, Pag p) 
        { assert(n < g_size); Pag old = pags[n]; pags[n] = p; return old;}
    static V_f* factory (Sum* sum, char type, const std::vector<Pag> &p);
    static V_f* copy_factory (Sum* sum, char type, V_f* orig);
    static std::vector<fp> get_default_peak_parameters(const f_names_type *f, 
                                              const std::vector<fp> &peak_hcw);

 protected:
};

/**************     G  A  U  S  S  I  A  N    ********************/

class fGauss : public V_f
{
    fGauss (Sum* sum, char t, const std::vector<Pag>& g) 
        : V_f (sum, t, g) {}
    fGauss (const fGauss&);
    friend class V_f;
public:
    static const char *const wzor;
    static const char *p_names[];
    static const std::vector<ParDefault> p_defaults;
    fp compute (fp x, fp* dy_dx); 
    void f_val_precomputations () { if (fabs(av[2]) < TINY) av[2] = TINY; }
    void get_range (fp level);  
    fp center() const;
    fp height() const;
    fp fwhm() const;
    fp area() const;
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G);
};

/************   L  O  R  E  N  T  Z  I  A  N   ******************/

class fLorentz : public V_f
{
    fLorentz (Sum* sum, char t, const std::vector<Pag>& g) 
        : V_f (sum, t, g) {}
    fLorentz (const fLorentz&);
    friend class V_f;
public:
    static const char *const wzor;
    static const char *p_names[];
    static const std::vector<ParDefault> p_defaults;
    fp compute (fp x, fp* dy_dx); 
    void f_val_precomputations () { if (fabs(av[2]) < TINY) av[2] = TINY; }
    void get_range (fp level);
    fp center() const;
    fp height() const;
    fp fwhm() const;
    fp area() const;
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G);
};

     /************   P  E  A  R  S  O  N    V I I   ***************/

class fPearson : public V_f
{
    fPearson (Sum* sum, char t, const std::vector<Pag>& g) 
        : V_f (sum, t, g) {}
    fPearson (const fPearson&);
    friend class V_f;
public:
    static const char *const wzor;
    static const char *p_names[];
    static const std::vector<ParDefault> p_defaults;
    fp compute (fp x, fp* dy_dx);
    void f_val_precomputations ();
    void get_range (fp level); 
    fp center() const;
    fp height() const;
    fp fwhm() const;
    fp area() const;
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G);
private:
    bool too_big_a3, too_small_a3;
};

/**************  p s e u d o  -  V  O  I  G  T  ********************/

class fPsVoigt : public V_f
{
    fPsVoigt (Sum* sum, char t, const std::vector<Pag>& g) 
        : V_f (sum, t, g){}
    fPsVoigt (const fPsVoigt&);
    friend class V_f;
 public:
    static const char *const wzor;
    static const char *p_names[];
    static const std::vector<ParDefault> p_defaults;
    fp compute (fp x, fp* dy_dx);
    void f_val_precomputations () { if (fabs(av[2]) < TINY) av[2] = TINY; }
    void get_range (fp level);
    fp center() const;
    fp height() const;
    fp fwhm() const;
    fp area() const;
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G);
};

/**************    V  O  I  G  T    ********************/

class fVoigt : public V_f
{
    fVoigt (Sum* sum, char t, const std::vector<Pag>& g) 
        : V_f (sum, t, g){}
    fVoigt (const fVoigt&);
    friend class V_f;
 public:
    static const char *const wzor;
    static const char *p_names[];
    static const std::vector<ParDefault> p_defaults;
    fp compute (fp x, fp* dy_dx);
    void f_val_precomputations ();
    //void get_range (fp level);
    fp center() const;
    fp height() const;
    fp fwhm() const;
    fp area() const;
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G);
};

     /************     P  o  l  y  n  o  m  i  a  l   ******************/
      
class fPolynomial5 : public V_f
{      
    fPolynomial5 (Sum* sum, char t, const std::vector<Pag>& g) 
        : V_f (sum, t, g){}
    fPolynomial5 (const fPolynomial5&);
    friend class V_f;
public:
    static const char *const wzor; 
    static const char *p_names[];
    static const std::vector<ParDefault> p_defaults;
    fp compute (fp x, fp* dy_dx); 
    bool is_peak() const { return false; }
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G);
};


/********************   z e r o  -  s h i f t   ************************/

class V_z : public V_f_z_common
{
public:
    V_z (Sum* sum, char t, const std::vector<Pag>& g)
        : V_f_z_common (sum, g, t, zType) {}
    virtual ~V_z(){}
    One_of_fzg fzg_of() const { return zType; }
    virtual fp z_value (fp x) const = 0;
    virtual void compute_deriv (fp x, fp dy_dx) = 0;//dy_dx is from V_f::compute

    static const z_names_type z_names[];
    static V_z* factory (Sum* sum, char type, const std::vector<Pag> &p);
};
    
class Zero_shift_x : public V_z
{
    Zero_shift_x (Sum *sum, char t, const std::vector<Pag>& g) 
        : V_z (sum, t, g) {}
    friend class V_z;
public:
    fp z_value (fp /* x */) const { return av[0]; }
    void compute_deriv (fp /*x*/, fp dy_dx) { der[0] = dy_dx; } 
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G)
        { return ps(0, A, G); }
};
    
class Zero_shift_c : public V_z 
{
    Zero_shift_c (Sum* sum, char t, const std::vector<Pag>& g) 
        : V_z (sum, t, g) {}
    friend class V_z;
public:
    fp z_value (fp x) const { return av[0] * cos (M_PI / 360 * x); }
    void compute_deriv (fp x, fp dy_dx) {der[0] = dy_dx * cos (M_PI / 360 * x);}
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G)
        { return ps(0, A, G) + "*cos(pi/360*x)"; }
};

class Zero_shift_s : public V_z 
{
    Zero_shift_s (Sum* sum, char t, const std::vector<Pag>& g) 
        : V_z (sum, t, g) {}
    friend class V_z;
public:
    fp z_value (fp x) const { return av[0] * sin (M_PI / 180 * x); }
    void compute_deriv (fp x, fp dy_dx) {der[0] = dy_dx * sin (M_PI / 180 * x);}
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G)
        { return ps(0, A, G) + "*sin(pi/180*x)"; }
};

fp LnGammaE (fp);
    
#endif   

