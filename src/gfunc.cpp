// This file is part of fityk program. Copyright (C) Marcin Wojdyr
#include "common.h"
RCSID ("$Id$")

#include "gfunc.h"
#include "ui.h"
#include <algorithm>

using namespace std;

V_g::V_g (Sum* sum, char t, const vector<Pag> &v) 
    : V_fzg(sum, v, t, gType) 
{
}

vector<int_fp> V_g::get_derivatives (const vector<fp>& A, const vector<V_g*>& G)
{
    der.resize(pags.size());
    prepare_av (A, G);
    prepare_deriv ();
    vector<int_fp> v;
    for (unsigned int i = 0; i != pags.size(); i++) {
        vector<int_fp> d = pags[i].get_derivatives(A, G);
        for (vector<int_fp>::iterator j = d.begin(); j != d.end(); j++)
            j->der *= der[i];
        for (vector<int_fp>::iterator j = d.begin(); j != d.end(); j++) {
            vector<int_fp>::iterator f = find (v.begin(), v.end(), *j);
            if (f != v.end()) 
                f->der += j->der;
            else
                v.push_back (*j);
        }
    }
    sort (v.begin(), v.end());  
    return v;
}

fp V_g::g_value (const std::vector<fp>& A, const std::vector<V_g*>& G)
{
    prepare_av (A, G);
    return g_val();
}

vector<char> V_g::all_types()
{
    vector<char> v;
    for (int i = 0; V_g::g_names[i].type != 0; i++) 
        v.push_back(g_names[i].type);
    return v;
}
//================

const z_names_type V_g::g_names[] = 
{ 
  z_names_type( 's', "sum (a0 + a1)",                                 2 ),
  z_names_type( 'i', "sign-change (-a0)",                             1 ),
  z_names_type( 'p', "product (a0 * a1)",                             2 ),
  z_names_type( 'd', "division (a0 / a1)",                            2 ),
  z_names_type( 'q', "sqrt (a0)",                                     1 ),
  z_names_type( 't', "tangent (tan (a0 * a1))",                       2 ),
  z_names_type( 'c', "cosinus (cos (a0 * a1))",                       2 ),
  z_names_type( 'm', "minimum (a0^2 + a1)",                           2 ),
  z_names_type( 'M', "maximum (-a0^2 + a1)",                          2 ),
  z_names_type( 'b', "bounds ((a2-a1) * (sin(a0)+1)/2 + a1)",         3 ),
  z_names_type( 'k', "(a0 * a1 + a2 * a3)",                           4 ),
  z_names_type( 'n', "polynomial (a1 + a2*a0 + a3*a0^2 + a4*a0^3)",   5 ),
  z_names_type( 'r', "(a1*a0^2 + a2*a0 + a3 + a4/a0 + a5/a0^2)",      6 ),
  z_names_type( 'X', "(2 * ArcSin(a0/2*a1) [DEG])",                   2 ),
  z_names_type( 'x', "(sqrt(a0/a1^2+a2/a3^2+p4/a5^2))",               6 ),
  z_names_type( 'F', "((a0^5+2.69*a0^4*a1+2.43*a0^3*a1^2"
                     "+4.47*a0^2*a1^3+0.078*a0*a1^4+a1^5)^0.2)", 2 ),
  z_names_type( 0 , "the end", 0 )
};

V_g* V_g::factory (Sum* sum, char type, const vector<Pag> &p) 
{
    switch (type) {
        case 's': return new Gsum      (sum, type, p);
        case 'i': return new Gsign     (sum, type, p);
        case 'p': return new Gproduct  (sum, type, p);
        case 'd': return new Gdivision (sum, type, p);

        case 'q': return new Gsqrt     (sum, type, p);
        case 't': return new Gtangent  (sum, type, p);
        case 'c': return new Gcosinus  (sum, type, p);

        case 'm': return new Gminimum  (sum, type, p);
        case 'M': return new Gmaximum  (sum, type, p);
        case 'b': return new Gbounds   (sum, type, p);

        case 'k': return new Gsum_of_prod                  (sum, type, p);
        case 'n': return new Gpolynomial                   (sum, type, p);
        case 'r': return new Grational                     (sum, type, p);
        case 'X': return new Gtwo_arcsin_a_half_a_in_deg   (sum, type, p);
        case 'x': return new Gsqrt_a0_a1a1_a2_a3a3_a4_a5a5 (sum, type, p);
        case 'F': return new Gtchz_fwhm                    (sum, type, p);

        default:
            warn("Unknown type -- " + V_fzg::full_type(gType) + S(type));
            return 0;
    }
}

fp Gpolynomial::g_val () 
{ 
    return av[1] + av[2] * av[0] + av[3] * av[0] * av[0] 
        + av[4] * av[0] * av[0] * av[0]; 
}

void Gpolynomial::prepare_deriv () 
{ 
    fp t = av[0]; 
    der[0] = av[2] + 2 * av[3] * t + 3 * av[4] * t * t;
    der[1] = 1.;
    der[2] = t;
    der[3] = t * t;
    der[4] = t * t * t;
}

std::string Gpolynomial::formula (const vector<fp>& A, const vector<V_g*>& G) 
{
    return ps(1,A,G) + "+" + ps(2,A,G) + "*" + ps(0,A,G)  + "+" 
        + ps(3,A,G) + "*" + ps(0,A,G) + "**2+" 
        + ps(4,A,G) + "*" + ps(0,A,G) + "**3";
}

fp Grational::g_val () 
{
    fp t = av[0]; 
    return av[1] * t * t + av[2] * t + av[3] 
           + (av[4] != 0 ? av[4] / t : 0) 
           + (av[5] != 0 ? av[5] / t / t : 0);
}

void Grational::prepare_deriv () 
{ 
    fp t = av[0]; 
    der[0] = 2 * t * av[1] + av[2] 
                  - (av[4] != 0 ? av[4] / t / t : 0) 
                  - (av[5] != 0 ? 2 * av[5] / t / t / t : 0);
    der[1] = t * t;
    der[2] = t;
    der[3] = 1.;
    if (t != 0) {
        der[4] = 1. / t;
        der[5] = 1. / t / t;
    }
}

string Grational::formula (const vector<fp>& A, const vector<V_g*>& G) 
{
    string s0 = ps(0,A,G);
    return ps(1,A,G) + "*" + s0 + "**2+" + ps(2,A,G) + "*" + s0
        + "+" + ps(3,A,G) + "+"
        + ps(4,A,G) + "/" + s0 + "+" + ps(5,A,G) + "/" + s0 + "**2";
}

void Gtwo_arcsin_a_half_a_in_deg::prepare_deriv () 
{ 
    fp arg = av[0] / 2 * av[1];
    fp common_factor = 180/M_PI / sqrt(1 - arg * arg);
    der[0] = av[1] * common_factor;
    der[1] = av[0] * common_factor;
}                                               

void Gsqrt_a0_a1a1_a2_a3a3_a4_a5a5::prepare_deriv () 
{ 
    fp a = av[1];
    fp b = av[3];
    fp c = av[5];
    fp sq = sqrt(av[0] / (a * a) + av[2] / (b * b) + av[4] / (c * c));
    der[0] = 0.5 / (sq * a * a);
    der[1] = - av[0] / (sq * a * a * a);
    der[2] = 0.5 / (sq * b * b);
    der[3] = - av[2] / (sq * b * b * b);
    der[4] = 0.5 / (sq * c * c);
    der[5] = - av[4] / (sq * c * c * c);
}                                               

std::string Gsqrt_a0_a1a1_a2_a3a3_a4_a5a5::formula (const vector<fp>& A, 
                                                    const vector<V_g*>& G) 
{
    return "sqrt(" + ps(0,A,G) + "/(" + ps(1,A,G) + ")**2+"
                   + ps(2,A,G) + "/(" + ps(3,A,G) + ")**2+"
                   + ps(4,A,G) + "/(" + ps(5,A,G) + ")**2)";
}

const fp Gtchz_fwhm::p[4] = { 2.69269, 2.42843, 4.47163, 0.07842 };

fp Gtchz_fwhm::g_val()
{
    fp a0 = av[0], a1 = av[1];
    fp a0_2 = a0 * a0, a1_2 = a1 * a1;
    fp a0_3 = a0_2 * a0, a1_3 = a1_2 * a1;
    fp a0_4 = a0_3 * a0, a1_4 = a1_3 * a1;
    fp a0_5 = a0_4 * a0, a1_5 = a1_4 * a1;
    fp t = a0_5 + p[0] * a0_4 * a1 + p[1] * a0_3 * a1_2 
        + p[2] * a0_2 * a1_3 + p[3] * a0 * a1_4 + a1_5;
    return pow (t, 0.2);
}

void Gtchz_fwhm::prepare_deriv () 
{ 
    //--8<--- copy&paste from g_val() --  end  --
    fp a0 = av[0], a1 = av[1];
    fp a0_2 = a0 * a0, a1_2 = a1 * a1;
    fp a0_3 = a0_2 * a0, a1_3 = a1_2 * a1;
    fp a0_4 = a0_3 * a0, a1_4 = a1_3 * a1;
    fp a0_5 = a0_4 * a0, a1_5 = a1_4 * a1;
    fp t = a0_5 + p[0] * a0_4 * a1 + p[1] * a0_3 * a1_2 
        + p[2] * a0_2 * a1_3 + p[3] * a0 * a1_4 + a1_5;
    //--8<--- copy&paste from g_val() -- begin --
    fp q = 0.2 * pow (t, -0.8);
    der[0] = q * (5 * a0_4 + 4 * p[0] * a1 * a0_3 + 3 * p[1] * a1_2 * a0_2 
                        + 2 * p[2] * a1_3 * a0 +  p[3] * a1_4);
    der[1] = q * (5 * a1_4 + 4 * p[3] * a0 * a1_3 + 3 * p[2] * a0_2 * a1_2 
                        + 2 * p[1] * a0_3 * a1 +  p[0] * a0_4);
}                                               

std::string Gtchz_fwhm::formula (const vector<fp>& A, const vector<V_g*>& G) 
{
    return "("          + ps(0,A,G) + "**5+" 
        + S(p[0]) + "*" + ps(0,A,G) + "**4*" + ps(1,A,G) +    "+"
        + S(p[1]) + "*" + ps(0,A,G) + "**3*" + ps(1,A,G) + "**2+" 
        + S(p[2]) + "*" + ps(0,A,G) + "**2*" + ps(1,A,G) + "**3+"
        + S(p[3]) + "*" + ps(0,A,G) +    "*" + ps(1,A,G) + "**4+"  
                                             + ps(1,A,G) + "**5)**0.2";
}

void Gbounds::prepare_deriv ()
{
    der[0] = (av[2] - av[1]) * 0.5 * cos(av[0]); 
    der[1] = -(sin(av[0]) + 1) / 2. + 1.;
    der[2] =  (sin(av[0]) + 1) / 2.; 
}

//=======================   Pag   ===========================

fp Pag::value (const std::vector<fp>& A, const std::vector<V_g*>& G) const 
{
    if (is_g())
        return G[g()]->g_value (A, G);
    else if (is_a())
        return A[a()];
    else if (is_p())
        return p();
    else { assert(0); return 0; }
}

vector<int_fp> Pag::get_derivatives (const vector<fp>& A, 
                                                const vector<V_g*>& G) const
{
    if (is_g())
        return G[g()]->get_derivatives (A, G);
    else if (is_p())
        return vector<int_fp>(0);
    else if (is_a())
        return vector1(int_fp(a(), 1.));
    else { assert(0); return vector<int_fp>(0); }
}

string Pag::str (const vector<fp>& A, const vector<V_g*>& G) const
{
    return S(value(A, G));
}

string Pag::str() const
{
    if (is_g())
        return "$" + S(g());
    else if (is_a())
        return "@" + S(a());
    else if (is_p())
        return "_" + S(p());
    else { assert(0); return 0; }
}

void Pag::synch(Pag p)
{
    if (type != 'p' && type == p.type && nr >= p.nr) {
        assert (nr != p.nr);
        nr --;
    }
}


