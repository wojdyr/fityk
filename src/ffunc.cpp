// This file is part of fityk program. Copyright (C) Marcin Wojdyr
#include "common.h"
RCSID ("$Id$")

#include "ffunc.h"
#include "gfunc.h"
#include "voigt.h"

using namespace std; 

std::string V_f::xstr = "x";
#define LN2_str "0.69314718"

void V_f_z_common::pre_compute (const vector<fp>& A, const vector<V_g*>& G) 
{
    prepare_av (A, G);
    multi.clear();
    for (unsigned int i = 0; i < pags.size(); i++) {
        vector<int_fp> v = pags[i].get_derivatives (A, G);  
        for (vector<int_fp>::iterator j = v.begin(); j != v.end(); j++) {
            int_int_fp q = { j->nr, j->der, i };
            multi.push_back (q);
        }
    }
    f_val_precomputations();
}

void V_f_z_common::pre_compute_value_only (const vector<fp>& A, 
                                           const vector<V_g*>& G) 
{
    prepare_av (A, G);
    f_val_precomputations();
}

////////////////////////////

void V_f::get_range (fp /*level*/) //default 
{ 
    range_l = -INF;  
    range_r = +INF;
}

string V_f::extra_description() const
{
    string s;
    fp a = area(), h = height(), c = center(), fw = fwhm(), beta = 0;
    if (a)   s += " Area:" + S(a);
    if (h) s += " H:"    + S(h);
    if (c) s += " Ctr:"  + S(c);
    if (fw)   s += " FWHM:" + S(fw);
    if (a && h) s += " Integr.breadth(beta):" + S(beta = a / h);
    if (fw && beta) s += " shape(FWHM/beta):" + S(fw / beta);
    return s;
}


//---------------

const char *const fGauss::wzor =
        "       + - - - - - - - - - - - - - - - - - - +  \n"
        "       |                               2     |  \n"
        "       |                  /  x - a1  \\       |  \n"
        "       |  a0 Exp[ -ln(2) ( ---------- )  ]   |  \n"
        "       |                  \\    a2    /       |  \n"
        "       |                                     |  \n"
        "       + - - - - - - - - - - - - - - - - - - + ";  

const char *fGauss::p_names[] = { "height", "center", "HWHM" };
const vector<ParDefault> fGauss::p_defaults = vector<ParDefault>(3);

fp fGauss::compute (fp x, fp* dy_dx) 
{
    fp xa1a2 = (x - av[1]) / av[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    if (dy_dx) {
        der[0] = ex;
        fp dcenter = 2 * M_LN2 * av[0] * ex * xa1a2 / av[2];
        der[1] = dcenter;
        der[2] = dcenter * xa1a2;
        *dy_dx -= dcenter;
    }
    return av[0] * ex;
}

void fGauss::get_range (fp level) 
{  
    if (level <= 0)
        range_l = -INF, range_r = +INF;
    else if (level >= av[0])
        range_l = range_r = 0;
    else {
        fp w = sqrt (log (av[0] / level) / M_LN2) * av[2]; 
        range_l = av[1] - w;             
        range_r = av[1] + w;
    }
}

fp fGauss::height() const { return av[0]; }
fp fGauss::center() const { return av[1]; }
fp fGauss::fwhm() const { return 2 * fabs(av[2]); }
fp fGauss::area() const { return av[0] * fabs(av[2]) * sqrt(M_PI / M_LN2); }

string fGauss::formula (const vector<fp>& A, const vector<V_g*>& G) 
{
    return ps(0, A, G) + "*exp(-" LN2_str "*((" + xstr  + "-"
        + ps(1, A, G) + ")/" + ps(2, A, G) + ")**2)";
}


const char *const fLorentz::wzor =
  "     + - - - - - - - - - - - - - - - - - +  \n"
  "     |                                   |  \n"
  "     |                 a0                |  \n"
  "     |        --------------------       |  \n"
  "     |                / x-a1 \\ 2         |  \n"
  "     |           1 + ( -------)          |  \n"
  "     |                \\  a2  /           |  \n"   
  "     + - - - - - - - - - - - - - - - - - +  ";

const char *fLorentz::p_names[] = { "height", "center", "HWHM" };
const vector<ParDefault> fLorentz::p_defaults = vector<ParDefault>(3);

fp fLorentz::compute (fp x, fp* dy_dx) 
{
    fp xa1a2 = (x - av[1]) / av[2];
    fp inv_denomin = 1. / (1 + xa1a2 * xa1a2); 
    if (dy_dx) {
        der[0] = inv_denomin ;
        fp dcenter = 2 * av[0] * xa1a2 / av[2] * inv_denomin * inv_denomin;
        der[1] = dcenter;
        der[2] = dcenter * xa1a2;
        *dy_dx -= dcenter;
    }
    return av[0] * inv_denomin;
}

void fLorentz::get_range (fp level) 
{  
    if (level <= 0)
        range_l = -INF, range_r = +INF;
    else if (level >= av[0])
        range_l = range_r = 0;
    else {
        fp w = sqrt (av[0] / level - 1) * av[2]; 
        range_l = av[1] - w;             
        range_r = av[1] + w;
    }
}

fp fLorentz::height() const { return av[0]; }
fp fLorentz::center() const { return av[1]; }
fp fLorentz::fwhm() const { return 2 * fabs(av[2]); }
fp fLorentz::area() const { return av[0] * fabs(av[2]) * M_PI; }

string fLorentz::formula (const vector<fp>& A, const vector<V_g*>& G) 
{
    return ps(0, A, G) + "/(1+((" + xstr + "-"
        + ps(1, A, G) + ")/" + ps(2, A, G) + ")**2)";
}


const char *const fPearson::wzor =
  "     + - - - - - - - - - - - - - - - - - - - - - - - - - - + \n"
  "     |                                                     | \n"
  "     |                                                     | \n"
  "     |                          a0                         | \n"
  "     |   -----------------------------------------------   | \n"
  "     |     [          / x-a1 \\ 2   /  1/a3    \\   ] a3     | \n"
  "     |     [   1  +  ( ------ )   (  2     - 1 )  ]        | \n"
  "     |     [          \\  a2  /     \\          /   ]        | \n"
  "     + - - - - - - - - - - - - - - - - - - - - - - - - - - + ";

const char *fPearson::p_names[] = { "height", "center", "HWHM", "shape" };
const vector<ParDefault> fPearson::p_defaults = vector4(ParDefault(), 
                        ParDefault(), ParDefault(), ParDefault(2., 0.6, 5));

void fPearson::f_val_precomputations ()
{
    if (fabs(av[2]) < TINY) 
        av[2] = TINY; 
    if (av.size() != 6)
        av.resize(6);
    const fp max_a3 = 1e10;
    const fp min_a3 = 0.5 + TINY;// > 1/2
    too_big_a3 = av[3] > max_a3; //to prevent large numerical errors
    too_small_a3 = av[3] < min_a3; //to prevent infinite area 
    fp a3 = too_big_a3 ? max_a3 : (too_small_a3 ? min_a3 : av[3]);
    av[4] = pow (2, 1. / a3) - 1;
    av[5] = a3;
}

fp fPearson::compute (fp x, fp* dy_dx) 
{
    fp xa1a2 = (x - av[1]) / av[2];
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = av[4]; //pow (2, 1. / a3) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow (denom_base, - av[5]);
    if (dy_dx) {
        der[0] = inv_denomin;
        fp dcenter = 2 * av[0] * av[5] * pow_2_1_a3_1 * xa1a2 * inv_denomin /
                                                          (denom_base * av[2]);
        der[1] = dcenter;
        der[2] = dcenter * xa1a2;
        fp da3 = av[0] * inv_denomin * (M_LN2 * (pow_2_1_a3_1 + 1) 
                              * xa1a2sq / (denom_base * av[5]) - log(denom_base));
        if ((!too_big_a3 || da3 > 0) && (!too_small_a3 || da3 < 0))
            der[3] = da3;
        *dy_dx -= dcenter;
    }
    return av[0] * inv_denomin;
}

void fPearson::get_range (fp level) 
{
    if (level <= 0 || av[0] <= 0)
        range_l = -INF, range_r = +INF;
    else if (level >= av[0])
        range_l = range_r = 0;
    else {
        fp t = (pow (av[0]/level, 1./av[3]) - 1) / (pow (2, 1./av[3]) - 1);
        fp w = sqrt(t) * av[2];
        range_l = av[1] - w;             
        range_r = av[1] + w;
    }
}

fp fPearson::height() const { return av[0]; }
fp fPearson::center() const { return av[1]; }
fp fPearson::fwhm() const { return 2 * fabs(av[2]); }

fp fPearson::area() const  
{ 
    if (av[3] <= 0.5)
        return +INF;
    fp g = exp_ (LnGammaE (av[5] - 0.5) - LnGammaE (av[5]));
    return av[0] * 2 * fabs(av[2]) 
        * sqrt(M_PI) * g / (2 * sqrt (av[4]));
    //in f_val_precomputations(): av[4] = pow (2, 1. / a3) - 1;
    //                            av[5] = a3;
}

string fPearson::formula (const vector<fp>& A, const vector<V_g*>& G) 
{
    return ps(0, A, G) + "/((1+(((" + xstr + "-" 
        + ps(1, A, G) + ")/" + ps(2, A, G) + ")**2)*(2**(1/"
        + ps(3, A, G) + ")-1))**" + ps(3, A, G) + ")";
}


const char *const fPsVoigt::wzor =
"    + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +\n"
"    |    [                                                   2    ]   |\n"
"    |    [        1                        /         / x-a1 \\  \\  ]   |\n"
"    | a0 [ a3 ----------2--  +  (1-a3) exp( -(ln2)  ( ------ )  ) ]   |\n"
"    |    [         /x-a1\\                  \\         \\  a2  /  /  ]   |\n"
"    |    [     1+ ( ---- )                                        ]   |\n"
"    |              \\ a2 /                                             |\n"
"    + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - + "; 

const char *fPsVoigt::p_names[] = { "height", "center", "width", "shape" };
const vector<ParDefault> fPsVoigt::p_defaults = vector4(ParDefault(), 
                         ParDefault(), ParDefault(), ParDefault(0.5, 0., 1.));

fp fPsVoigt::compute (fp x, fp* dy_dx) 
{
    fp xa1a2 = (x - av[1]) / av[2];
    fp x_2 = xa1a2 * xa1a2;
    fp ex = exp_ (-M_LN2 * x_2);
    fp lor = 1 / (1 + x_2);
    fp without_a0 = (av[3] * lor + (1 - av[3]) * ex);
    if (dy_dx) {
        der[0] = without_a0;
        fp dcenter = 2 * av[0] * xa1a2 / av[2] 
            * (av[3] * lor * lor + (1 - av[3]) * M_LN2 * ex);
        der[1] = dcenter;
        der[2] = dcenter * xa1a2;
        der[3] = av[0] * (lor - ex);
        *dy_dx -= dcenter;
    }
    return av[0] * without_a0;
}

void fPsVoigt::get_range (fp level) 
{  
    if (level <= 0 || av[0] <= 0)
        range_l = -INF, range_r = +INF;
    else if (level >= av[0] * av[3])
        range_l = range_r = 0;
    else {
        // neglecting Gaussian part and adding 4.0 to compensate it 
        fp w = (sqrt (av[0] * av[3] / level - 1) + 4.) * av[2];
        range_l = av[1] - w;             
        range_r = av[1] + w;
    }
}

fp fPsVoigt::height() const { return av[0]; }
fp fPsVoigt::center() const { return av[1]; }
fp fPsVoigt::fwhm() const { return 2 * fabs(av[2]); }  

fp fPsVoigt::area() const  
{ 
    return av[0] * fabs(av[2]) 
        * ((av[3] * M_PI) + (1 - av[3]) * sqrt (M_PI / M_LN2));
}

string fPsVoigt::formula (const vector<fp>& A, const vector<V_g*>& G) 
{
    string a0 = ps(0, A, G); 
    string a1 = ps(1, A, G); 
    string a2 = ps(2, A, G); 
    string a3 = ps(3, A, G); 
    return a0 + "*(" + a3 + "/(1+((x-" + a1 + ")/" + a2 + ")**2)+(1-"
        + a3 + ")*exp(-" LN2_str "*((x-" + a1 +")/" + a2 + ")**2))";
}



const char *const fVoigt::wzor =
  "     + - - - - - - - - - - - - - - - - - - - - - - - - - - + \n"
  "     |    _+oo                                             | \n"
  "     |   /                                                 | \n"
  "     |   |                                                 | \n"
  "     |   |     .....                                       | \n"
  "     |   |                                                 | \n"
  "     |  _/                                                 | \n"
  "     |    -oo                                              | \n"
  "     + - - - - - - - - - - - - - - - - - - - - - - - - - - + ";

const char *fVoigt::p_names[] = { "height", "center", "Gaussian width", 
                                    "shape" };
const vector<ParDefault> fVoigt::p_defaults = vector<ParDefault>(4);

void fVoigt::f_val_precomputations ()
{
    if (av.size() != 6)
        av.resize(6);
    float k, l, dkdx, dkdy; 
    humdev(0, av[3], k, l, dkdx, dkdy);
    av[4] = 1. / k;  
    av[5] = dkdy / k;
}

fp fVoigt::compute (fp x, fp* dy_dx) 
{
    float k;
    fp xa1a2 = (x - av[1]) / av[2];
    fp a0a4 = av[0] * av[4];
    if (dy_dx) {
        float l, dkdx, dkdy; 
        humdev(xa1a2, av[3], k, l, dkdx, dkdy); 
        der[0] = av[4] * k;
        fp dcenter = -a0a4 * dkdx / av[2]; 
        der[1] = dcenter;
        der[2] = dcenter * xa1a2;
        der[3] = a0a4 * (dkdy - k * av[5]);  
        *dy_dx -= dcenter;
    }
    else
        k = humlik(xa1a2, av[3]);
    return a0a4 * k;
}

//void fVoigt::get_range (fp level) //TODO
//{
//}

fp fVoigt::height() const { return av[0]; }
fp fVoigt::center() const { return av[1]; }
fp fVoigt::fwhm() const { return 2 * fabs(av[2]) * sqrt(av[3] + 1); }
                                                            //Posener, 1959

fp fVoigt::area() const  
{ 
    return fabs(av[0] * av[2] * sqrt(M_PI) / humlik(0, av[3]));//* av[4];//TODO
}

string fVoigt::formula (const vector<fp>& /*A*/, const vector<V_g*>& /*G*/) 
{
    return "[integral from -INF to +INF of ...]";
}



const char *const fPolynomial5::wzor =
  "     + - - - - - - - - - - - - - - - - - - - - - - - - + \n"
  "     |  a0 + a1 x + a2 x^2 + a3 x^3 + a4 x^4 + a5 x^5  | \n"
  "     + - - - - - - - - - - - - - - - - - - - - - - - - + ";

const char *fPolynomial5::p_names[] = { "a0", "a1", "a2", "a3", "a4", "a5" };
const vector<ParDefault> fPolynomial5::p_defaults = vector<ParDefault>(6);

fp fPolynomial5::compute (fp x, fp* dy_dx) 
{
    if (dy_dx) {
        der[0] = 1;
        der[1] = x;
        der[2] = x * x;
        der[3] = x * x * x;
        der[4] = x * x * x * x;
        der[5] = x * x * x * x * x;
        *dy_dx -= av[1] + 2 * av[2] * x + 3 * av[3] * x * x 
            + 4 * av[4] * x * x * x + 5 * av[5] * x * x * x * x;
    }
    return av[0] + av[1] * x + av[2] * x * x + av[3] * x * x * x 
        + av[4] * x * x * x * x + av[5] * x * x * x * x * x;
}

string fPolynomial5::formula (const vector<fp>& A, const vector<V_g*>& G) 
{
    return ps(0, A, G) + "+" + ps(1, A, G) + "*x+" 
        + ps(2, A, G) + "*x**2+" + ps(3, A, G) + "*x**3+" 
        + ps(4, A, G) + "*x**4+" + ps(5, A, G) + "*x**5";
}

//================

const f_names_type V_f::f_names[] = 
{ 
  f_names_type( 'G', "Gaussian",     3, fGauss::wzor,     fGauss::p_names,
                                        fGauss::p_defaults),
  f_names_type( 'L', "Lorentzian",   3, fLorentz::wzor,   fLorentz::p_names,
                                        fLorentz::p_defaults),
  f_names_type( 'P', "PearsonVII",   4, fPearson::wzor,   fPearson::p_names,
                                        fPearson::p_defaults),
  f_names_type( 'S', "Pseudo-Voigt", 4, fPsVoigt::wzor,   fPsVoigt::p_names,
                                        fPsVoigt::p_defaults),
  f_names_type( 'V', "Voigt",        4, fVoigt::wzor,     fVoigt::p_names,
                                        fVoigt::p_defaults),
  f_names_type( 'n', "polynomial5",  6, fPolynomial5::wzor, 
                              fPolynomial5::p_names, fPolynomial5::p_defaults),
  f_names_type( 0 , "the end", 0, 0, 0, vector<ParDefault>())
};

V_f* V_f::factory (Sum* sum, char type, const vector<Pag>& p) 
{
    switch (type) {
        case 'G':
            return new fGauss (sum, type, p);
        case 'L':
            return new fLorentz (sum, type, p);
        case 'P':
            return new fPearson (sum, type, p);
        case 'S':
            return new fPsVoigt (sum, type, p);
        case 'V':
            return new fVoigt (sum, type, p);
        case 'n':
            return new fPolynomial5 (sum, type, p);
        default:
            warn("Unknown type -- " + V_fzg::full_type(fType) + S(type));
            return 0;
    }
}

V_f* V_f::copy_factory (Sum* sum, char type, V_f* orig) 
{
    V_f* tmp = factory (0, type, vector<Pag>(0));
    if (!tmp)
        return 0;
    int copy_size = tmp->g_size;
    delete tmp;
    vector<Pag> p = orig->pags;
    if (size(p) > copy_size)
        p.resize(copy_size);
    else if (size(p) < copy_size) {
        int diff = copy_size - p.size();
        vector<Pag> appendix(diff, Pag(1.));
        p.insert(p.end(), appendix.begin(), appendix.end());
        verbose ("Additional parameter(s) for ^" + S(type) + " set to _1.0");
    }
    return factory (sum, type, p);
}

vector<fp> V_f::get_default_peak_parameters(const f_names_type &f, 
                                            const vector<fp> &peak_hcw)
{
    assert(peak_hcw.size() >= 3);
    fp height = peak_hcw[0]; 
    fp center = peak_hcw[1];
    fp hwhm = peak_hcw[2]; 
    vector<fp> ini(f.psize);
    for (int i = 0; i < f.psize; i++) {
        string pname = f.pnames[i];
        const ParDefault &pd = f.pdefaults[i];
        fp t = 0;
        if (i > 2 && size(peak_hcw) > i) t = peak_hcw[i];
        else if (pname == "height") t = height;
        else if (pname == "center") t = center;
        else if (pname == "HWHM")   t = hwhm;
        else if (pname == "FWHM")   t = 2*hwhm;
        else if (pname.find("width") < pname.size())   t = hwhm; 
        else if (pd.is_set) t = pd.p;
        ini[i] = t;
    }
    return ini;
}

/********************   z e r o  -  s h i f t   ************************/

const z_names_type V_z::z_names[] = 
{ 
  z_names_type ( 'x', "a0",               1 ),
  z_names_type ( 'c', "a0 cos(x[DEG]/2)", 1 ),
  z_names_type ( 's', "a0 sin(x[DEG])", 1 ),
  z_names_type ( 0 , "the end", 0 )
};

V_z* V_z::factory (Sum* sum, char type, const vector<Pag> &p) 
{
    switch (type) {
        case 'x':
            return new Zero_shift_x (sum, type, p);
        case 'c':
            return new Zero_shift_c (sum, type, p);
        case 's':
            return new Zero_shift_s (sum, type, p);
        default:
            warn("Unknown type -- " + V_fzg::full_type(zType) + S(type));
            return 0;
    }
}

//-----------------------------------------------------


fp LnGammaE (fp x) //log_e of Gamma function
    // x > 0
{
    const fp c[6] = { 76.18009172947146, -86.50532032941677, 
        24.01409824083091, -1.231739572450155, 
        0.1208650973866179e-2, -0.5395239384953e-5 };
    fp tmp = x + 5.5 - (x + 0.5) * log(x + 5.5); 
    fp s = 1.000000000190015; 
    for (int j = 0; j <= 5; j++) 
        s += c[j] / (x + j + 1); 
    return - tmp + log (2.5066282746310005 * s / x);
}

