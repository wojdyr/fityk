// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef GFUNC__H__
#define GFUNC__H__

        /*   g-functions (parameters for f-functions) defined here
         */

#include <math.h>
#include <string>
#include <vector>
#include "common.h"
#include "pag.h"
#include "fzgbase.h"

inline int sign(fp x) { return x >= 0 ? (x != 0 ? 1 : 0) : -1; }

class V_g : public V_fzg
{
public:
    static const z_names_type g_names[];

    V_g (Sum* sum, char t, const std::vector<Pag> &v);
    virtual ~V_g() {}
    One_of_fzg fzg_of() const { return gType; }
    fp g_value (const std::vector<fp>& A, const std::vector<V_g*>& G); 
    std::vector<int_fp> get_derivatives (const std::vector<fp>& A, 
                                         const std::vector<V_g*>& G);
    static std::vector<char> all_types();
    static V_g* V_g::factory (Sum* sum, char type, const std::vector<Pag> &p);

protected:
    virtual void prepare_deriv () = 0;
    virtual fp g_val () = 0; 
};


/********  sign change  ***********/
class Gsign : public V_g  // -a0
{
    Gsign (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val () { return -av[0]; }
    void prepare_deriv () { der[0] = -1.; }
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G) 
        { return "-" + ps(0,A,G); }
};   

/********  sum  ***********/
class Gsum : public V_g  // a0 + a1
{
    Gsum (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val () { return av[0] + av[1]; }
    void prepare_deriv () { der[0] = der[1] = 1.; }
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G) 
        { return ps(0,A,G) + "+" + ps(1,A,G); }
};   

/********  product  ***********/
class Gproduct : public V_g  // a0 * a1
{
    Gproduct (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val () { return av[0] * av[1]; }
    void prepare_deriv () { der[0] = av[1]; der[1] = av[0]; }
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G) 
        { return ps(0,A,G) + "*" + ps(1,A,G); }
};   

/********  division  ***********/
class Gdivision : public V_g  // a0 / a1
{
    Gdivision (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val () { return av[0] / av[1]; }
    void prepare_deriv () { 
        der[0] = 1. / av[1];
        der[1] = - av[0] / (av[1] * av[1]); 
    }
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G) { return ps(0,A,G) + "/" + ps(1,A,G); }
};   

/********  sqrt  ***********/
class Gsqrt : public V_g  // sqrt (a0)
{
    Gsqrt (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val () { return sqrt (fabs(av[0])); }
    void prepare_deriv() { der[0] = sign(av[0]) * 0.5 / sqrt(fabs(av[0])); }
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G) { return "sqrt(" + ps(0,A,G) + ")"; }
};   

/********  cosinus  ***********/
class Gcosinus : public V_g  // cos (a0*a1)
{
    Gcosinus (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val () { return cos (av[0] * av[1]); }
    void prepare_deriv() { fp t = - sin (av[0] * av[1]); 
                           der[0] = av[1] * t; der[1] = av[0] * t; }
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G) { return "cos(" + ps(0,A,G) + "*" + ps(0,A,G) + ")"; }
};   

/********  tangent  ***********/
class Gtangent : public V_g  // tan (a0*a1)
{
    Gtangent (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val () { return tan (av[0] * av[1]); }
    void prepare_deriv() { fp c = cos(av[0] * av[1]); 
                           der[0] = av[1] / (c*c); der[1] = av[0] / (c*c); }
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G) { return "tan(" + ps(0,A,G) + "*" + ps(1,A,G) + ")"; }
};   

/********  sum of products  ***********/
class Gsum_of_prod : public V_g  // a0 * a1 + a2 * a3
{
    Gsum_of_prod (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val () { return av[0] * av[1] + av[2] * av[3]; }
    void prepare_deriv () 
            { der[0] = av[1]; der[1] = av[0]; der[2] = av[3]; der[3] = av[2]; }
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G) 
       {return ps(0,A,G) + "*" + ps(1,A,G) + "+" + ps(2,A,G) + "*" + ps(3,A,G);}
};   

/********  polynomial  ***********/
class Gpolynomial : public V_g  // a1 + a2*a0 + a3*a0^2 + a4*a0^3 
{
    Gpolynomial (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val ();
    void prepare_deriv ();
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G);
};   

/********  rational  ***********/
class Grational : public V_g  // a1*a0^2 + a2*a0 + a3 + a4/a0 + a5/a0^2
{
    Grational (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val ();
    void prepare_deriv (); 
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G); 
};   

/********  2 * ArcSin(a0 / 2 * a1) [DEG] ***********/
class Gtwo_arcsin_a_half_a_in_deg : public V_g  
{
    Gtwo_arcsin_a_half_a_in_deg (Sum *sum, char t, const std::vector<Pag>&g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val () { return 360/M_PI * asin (av[0] / 2 * av[1]); }
    void prepare_deriv (); 
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G) 
        { return "360/pi*asin(" + ps(0,A,G) + "/2*" + ps(1,A,G) + ")"; }
};   

/************  sqrt(a0/a1^2+a2/a3^2+a4/a5^2) *************/
class Gsqrt_a0_a1a1_a2_a3a3_a4_a5a5 : public V_g  
{
    Gsqrt_a0_a1a1_a2_a3a3_a4_a5a5 (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val () {
        return sqrt (av[0] / (av[1] * av[1]) + av[2] / (av[3] * av[3]) 
                     + av[4] / (av[5] * av[5]));
    }
    void prepare_deriv (); 
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G);
};   

/********  TCHZ-FWHM  ***********/
  // (a0^5 + p0 a0^4 a1 + p1 a0^3 a1^2 + p2 a0^2 a1^3 + p3 a0 a1^4 + a1^5)^0.2
  // p0, p1, p2, p3 = 2.69269, 2.42843, 4.47163, 0.07842
class Gtchz_fwhm : public V_g  
{
    Gtchz_fwhm (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val ();
    void prepare_deriv ();
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G);
private:
    static const fp p[4];
};   

/********  minimum  *********/
class Gminimum : public V_g  // a0^2 + a1
{
    Gminimum (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val () { return av[0] * av[0] + av[1]; }
    void prepare_deriv () { der[0] = 2 * av[0]; der[1] = 1.; }
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G) { return ps(0,A,G) + "**2+" + ps(1,A,G); }
};   

/********  maximum  *********/
class Gmaximum : public V_g  // -a0^2 + a1
{
    Gmaximum (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val () { return - av[0] * av[0] + av[1]; }
    void prepare_deriv () { der[0] = -2 * av[0]; der[1] = 1.; }
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G) { return "-(" + ps(0,A,G) + ")**2+" + ps(0,A,G); }
};   


/********  bounds  ***********/
class Gbounds : public V_g  // (a2-a1) * (sin(a0)+1)/2 + a1
{
    Gbounds (Sum *sum, char t, const std::vector<Pag>& g)
        : V_g(sum, t, g) {}
    friend class V_g;
public:
    fp g_val () { return (av[2] - av[1]) * (sin(av[0]) + 1) / 2. + av[1]; }
    void prepare_deriv ();
    std::string formula (const std::vector<fp>& A, const std::vector<V_g*>& G) {
        return "(" + ps(2,A,G) + "-" + ps(1,A,G) +")*(sin("+ ps(0,A,G) + ")+1)/2+" + ps(1,A,G);
    }
};   

#endif

