// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef PAG__H__
#define PAG__H__

#include <vector>
#include <set>
#include "common.h"

enum One_of_fzg
{
    fType, // ^-function (usually peak)
    zType, // zero-shift of whole sum 
    gType  // $-parameter (e.g. sum of two simple parameters -- @n+@m
};

struct pre_Hkl { int h, k, l; }; //used in parser.[yl] and crystal.h 

struct Pre_string //used in parser.y and parser.l
{ 
    char *c; int l; 
    std::string str() { return std::string(c, l); }
};  

//used in parser.y (in union); Domain defined in sum.h
struct pre_Domain { bool set, ctr_set; fp ctr, sigma; };

struct Pre_Pag { char c; int n; fp p;};

struct int_fp 
{ 
    int nr; 
    fp der; 
    int_fp () : nr(-1), der(0) {};
    int_fp (int n, fp d) : nr(n), der(d) {}
    bool operator< (const int_fp &b) const { return nr < b.nr; }
    bool operator== (const int_fp &b) const { return nr == b.nr; }
    bool operator!= (const int_fp &b) const { return nr != b.nr; }
};

class Pag
{
public:
    explicit Pag () : type(0) {}
    explicit Pag (V_g*, int n) : type('g'), nr(n) {}
    explicit Pag (fp, int n) : type('a'), nr(n) {}
    explicit Pag (fp p) : type('p'), nr(-1), constans_parameter(p) {}
    //Pre_Pag is used only in parser.y:
    explicit Pag (Pre_Pag r) : type(r.c), nr(r.n), constans_parameter(r.p) {}
    bool is_g() const { return type == 'g'; }
    bool is_a() const { return type == 'a'; }
    bool is_p() const { return type == 'p'; }
    fp value (const std::vector<fp>& A, const std::vector<V_g*>& G) const;
    std::vector<int_fp> get_derivatives (const std::vector<fp>& A, 
                                         const std::vector<V_g*>& G) const;
    std::string str(const std::vector<fp>& A, const std::vector<V_g*>& G) const;
    std::string str() const;
    bool operator== (const Pag& p) const
        { return (type == 'a' || type == 'g') && type == p.type && nr == p.nr; }
    void synch (Pag p);
    int g() const { assert (is_g()); return nr; }
    int a() const { assert (is_a()); return nr; }
    fp p() const { assert (is_p()); return constans_parameter; }
    bool empty() const { return type != 'g' && type != 'a' && type != 'p'; }

protected:
    char type;
    int nr; // g or a
    fp constans_parameter;  //p
};

#endif

