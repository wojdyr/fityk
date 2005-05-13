// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "crystal.h"
#include <math.h>
#include <vector>
#include <algorithm>
#include "sum.h"
#include "data.h"
#include "gfunc.h"
#include "manipul.h"
#include "pcore.h"
#include "ui.h"

using namespace std;

Crystal *my_crystal; 

string Hkl::str() const 
{
    string s = (0 <= h && h <= 9 && 0 <= k && k <= 9 && 0 <= l && l <= 9 ?
                "" : " ");
    return "(" + S(h) + s + S(k) + s + S(l) + ")"; 
}

bool HklF::operator< (const HklF &a) const 
{ 
    if (h != a.h) 
        return h < a.h;
    else if (k != a.k)
        return k < a.k;
    else
        return l < a.l;
}

Phase::Phase (Sum *s, char t, const std::vector<Pag>& latt_a) 
    : PagContainer(s, latt_a), typ(t), 
      fwhm_refinable(0), shape_refinable(0)
{
    latt_0 = values_of_pags();
}

string Phase::ph_str(int n) const
{
    assert (n >= 0 && n < size(pags)); 
    const vector<fp>& aa = sum->pars()->values();
    const vector<V_g*>& gg = sum->gfuncs_vec();
    string s = pags[n].str(); 
    if (!pags[n].is_p()) 
        s += "=" + S(pags[n].value(aa, gg)); 
    if (pags[n].value(aa, gg) != latt_0[n])
            s += "(ini:" + S(latt_0[n]) + ")";
    return s;
}

void Phase::set_containers_descriptions(int n)
{
    c_name = "%" + S(n) + "/lattice/";
    fwhm_refinable.c_name = "%" + S(n) + "/fwhm/";
    shape_refinable.c_name = "%" + S(n) + "/shape/";
    for (set<HklF>::iterator j = planes.begin(); j != planes.end(); j++) {
        *const_cast<string*>(&(j->c_name)) = "%" + S(n) + j->str();
        //not harmful cast - not changing sort order
    }
}

fp Wavelengths::lambda(int n) const
{
    assert (n >= 0 && n < get_count()); 
    const vector<fp>& aa = sum->pars()->values();
    const vector<V_g*>& gg = sum->gfuncs_vec();
    return pags[n * 2].value (aa, gg);
}

fp Wavelengths::intensity(int n) const
{
    assert (n >= 0 && n < get_count()); 
    const vector<fp>& aa = sum->pars()->values();
    const vector<V_g*>& gg = sum->gfuncs_vec();
    return pags[n * 2 + 1].value (aa, gg);
}

string Wavelengths::lambda_str(int n) const
{
    assert (n >= 0 && n < get_count() && initial_values.size() == pags.size()); 
    string s = l_pag(n).str(); 
    if (!l_pag(n).is_p()) 
        s += "=" + S(lambda(n)); 
    if (lambda(n) != initial_values[n * 2])
            s += "(ini:" + S(initial_values[n * 2]) + ")";
    return s;
}

string Wavelengths::intensity_str(int n) const
{
    assert (n >= 0 && n < get_count() && initial_values.size() == pags.size()); 
    string s = in_pag(n).str(); 
    if (!in_pag(n).is_p()) 
        s += "=" + S(intensity(n)); 
    if (intensity(n) != initial_values[n * 2 + 1])
            s += "(ini:" + S(initial_values[n * 2 + 1]) + ")";
    return s;
}

Pag Wavelengths::l_pag(int n) const
{
    assert (n >= 0 && n < get_count()); 
    return pags[n * 2];
}

Pag Wavelengths::in_pag(int n) const
{
    assert (n >= 0 && n < get_count()); 
    return pags[n * 2 + 1];
}

int Wavelengths::add (vector<Pag> p) 
{
    if (PagContainer(0, p).is_ok(sum) == false)
        return -1;
    if (p.size() > 2 || p.empty()) {
        warn ("Only one wavelength and (optionally) intensity expected."
                " Canceled");
        return -1;
    }
    if (p.size() == 1) {
        p.push_back (Pag (1.));
        verbose ("Relative intensity assumed _1.0");
    }
    pags.push_back (p[0]); //lambda
    pags.push_back (p[1]);//intensity
    int n = get_count() - 1;
    initial_values.push_back (lambda(n));
    initial_values.push_back (intensity(n));
    verbose ("New wavelength (and intensity) added.");
    return n;
}

Crystal::Crystal (Sum* s) 
    : xrays(s), sum(s), 
      peak_type('P'), 
      constrained_shape ('p'),
      constrained_fwhm ('p')
{
    map_peak_type ['P'] = "pearson";
    map_peak_type ['S'] = "pseudo-voigt";
    map_peak_type ['V'] = "voigt";
    map_peak_type ['G'] = "gaussian";
    map_peak_type ['L'] = "lorentzian";
    map_peak_type ['T'] = "Mod-TCHpV";
    epar.insert (pair<string, Enum_string>("peak-type", 
                                   Enum_string (map_peak_type, &peak_type)));
    map_c_fwhm ['n'] = "none";
    map_c_fwhm ['p'] = "equal-in-plane";
    map_c_fwhm ['e'] = "equal-in-phase";
    map_c_fwhm ['i'] = "Lowe-Ma";
    epar.insert (pair<string, Enum_string>("fwhm-constrain",
                                Enum_string (map_c_fwhm, &constrained_fwhm)));
    map_c_shape ['n'] = "none";
    map_c_shape ['p'] = "equal-in-plane";
    map_c_shape ['c'] = "equal-in-plane-constrained";
    map_c_shape ['e'] = "equal-in-phase";
    map_c_shape ['t'] = "function-of-theta";
    epar.insert (pair<string, Enum_string>("shape-constrain",
                                Enum_string (map_c_shape, &constrained_shape)));
}

int Crystal::add_phase (char type, vector<Pag> pg)
{
    if (PagContainer(0, pg).is_ok(sum) == false)
        return -1;
    string type_name;
    int param_number = -1;
    switch (type) {
        case 'c': 
            type_name = "Cubic";
            param_number = 1;
            break;
        case 't':
            type_name = "Tetragonal";
            param_number = 2;
            break;
        case 'o':
            type_name = "Orthorhombic";
            param_number = 3;
            break;
        case 'h':
            type_name = "Hexagonal";
            param_number = 2;
            break;
        default:
            warn ("Unknown phase symbol: %" + S(type));
            return -1;
    }
    if (size(pg) != param_number) {
        warn (type_name + " phase requires " + S(param_number)+" parameter(s)");
        return -1;
    }
    phases.push_back (Phase(sum, type, pg));
    int n = phases.size() - 1;
    phases[n].set_containers_descriptions(n);
    info (type_name + " phase %" + S(n) + " added.");
    return n;
}

string Crystal::phase_type_info() const
{
    return
        "%c: cubic phase - requires 1 parameter (a)\n"
        "%t: tetragonal phase - requires 2 parameters (a, c)\n"
        "%o: orthorhombic phase - requires 3 parameters (a, b, c)\n"
        "%h: hexagonal phase - requires 2 parameters (a, c)";
}

int Crystal::rm_phase (int phase_nr) 
{
    if (phase_nr == -1) {  // rm_phase(-1) means: remove all phases
        phases.clear();
        info ("All phases deleted.");
        return 0;
    }
    if (!is_index (phase_nr, phases)){
        warn("Wrong phase number: %" + S(phase_nr));
        return -1;
    }
    phases.erase (phases.begin() + phase_nr);
    info ("Phase %" + S(phase_nr) + " deleted.");
    if (size_t(phase_nr) < phases.size() - 1 + 1)  {
        verbose ("Some phases have changed numbers");
        for (int i = phase_nr; i < size(phases); i++) {
            phases[i].set_containers_descriptions(i);
        }
    }
    return 0;
}

int Crystal::add_plane (int phase_nr, Hkl hkl) 
{
    if (hkl.invalid()) return -1;
    if (xrays.get_count() == 0) {
        warn ("Wavelength(s) not defined (yet)");
        return -1;
    }
    if (!is_index (phase_nr, phases)) {
        warn ("No such phase: %" + S(phase_nr));
        return -1;
    }
    if (phases[phase_nr].planes.count(hkl) != 0) {
        warn ("Plane already exists.");
        return -1;
    }
    
    //its complicated in case when there are many wavelengths
    fp center = peak_center (xrays.lambda(0), phases[phase_nr], hkl);
    sum->use_param_a_for_value();
    center -= sum->zero_shift(center);
    string sp;
    fp h, area, fwhm;
    bool r = my_manipul->estimate_peak_parameters (center, -1/*default*/, 
                                                  0, &h, &area, &fwhm);
    if (!r){
        warn ("Peak estimation plane " + hkl.str() + " failed.");
        return -1;
    }
    fp h_at_center_before = sum->value (center);
    vector<int> vint; //for storing peaks numbers
    Pag a_corr; //for final height correction
    vector<Pag> heights = heights_of_peaks (h, &a_corr);//one for every peak
    for (int i = 0; i < xrays.get_count(); i++){
        vector<Pag> vg;
        vg.push_back (heights[i]);
        Pag c_g = add_center_g (phases[phase_nr], hkl, i);
        vg.push_back (c_g);
        Pag fwhm_pag = fwhm_of_peak (fwhm, phases[phase_nr], i, c_g);
        vg.push_back (fwhm_pag);
        Pag shape_p = shape_of_peak (phases[phase_nr], i, c_g);
        if (!shape_p.empty())
            vg.push_back (shape_p);

        int n = 0;
        if (peak_type == 'T')
            n = sum->add_fzg (fType, 'S', vg);
        else
            n = sum->add_fzg (fType, peak_type, vg); 
        vint.push_back(n);
        sp += " ^" + S(n);
    }
        //correcting height
    sum->use_param_a_for_value();
    fp diff = sum->value (center) - h_at_center_before; 
    if (diff != 0.) 
        sum->pars()->change_a (a_corr.a(), h / diff, '*', false);
    HklF temp_hkl (hkl, sum, vint);
    temp_hkl.c_name = "%" + S(phase_nr) + hkl.str();
    phases[phase_nr].planes.insert (temp_hkl); 
    info ("Plane %" + S(phase_nr) + " " + hkl.str() + " added as:" + sp); 
    return 0;
}

vector<Pag> Crystal::heights_of_peaks (fp &h, Pag* a_for_h_correction) const
{
    fp xh = h / sum_of_wavelength_ratio();
    Pag ha = sum->pars()->add_a (xh, Domain());
    vector<Pag> hh;
    if (xrays.get_count() == 1)
        hh.push_back (ha);
    else {
        for (int i = 0; i < xrays.get_count(); i++) {
            if (xrays.intensity(i) == 1.)
                hh.push_back (ha);
            else {
                vector<Pag> vh;
                vh.push_back (ha);
                vh.push_back (xrays.in_pag(i));
                Pag g = sum->add_g ('p', vh); 
                hh.push_back (g);
            }
        }
    }
    if (a_for_h_correction)
        *a_for_h_correction = ha;
    return hh;
}

Pag Crystal::fwhm_of_peak_TCHpV (fp fwhm, Phase& p, Pag c_g) 
{
    assert(peak_type = 'T');
    fp hwhm = fwhm / 2;
    if (p.fwhm_refinable.pags_size() == 0) { //initialization
            Pag u = sum->pars()->add_a (0., Domain());
            Pag v = sum->pars()->add_a (0., Domain());
            Pag w = sum->pars()->add_a (hwhm * hwhm, Domain());
            Pag z = sum->pars()->add_a (0., Domain());
            Pag x = sum->pars()->add_a (0., Domain());
            Pag y = sum->pars()->add_a (0., Domain());
            vector<Pag> rf = vector4 (u, v, w, z);
            rf.push_back(x);
            rf.push_back(y);
            p.fwhm_refinable = PagContainer (sum, rf);
    }
    if (p.fwhm_refinable.pags_size() != 6) { 
        warn ("`peak-type' should not be changed for one phase.");
        return Pag();
    }
    //ignoring `constrained_fwhm'
    
    //G_G = sqrt(U tan^2(theta) + V tan(theta) + W + Z / cos^2(theta))
    
    //    Gtangent [M_PI/180./2., ctr] -> tan
    Pag g_tan = sum->add_g ('t', vector2(c_g, Pag(M_PI/180./2.)));
    //    Gcosinus [M_PI/180./2., ctr] -> cos
    Pag g_cos = sum->add_g ('c', vector2(c_g, Pag(M_PI/180./2.)));
    //    Gpolynomial [tan W V U 0] -> polyn 
    const vector<Pag> &p_fr = p.fwhm_refinable.copy_of_pags();
    vector<Pag> pp(5); 
    pp[0] = g_tan, pp[1] = p_fr[2], pp[2] = p_fr[1], pp[3] = p_fr[0],
    pp[4] = Pag(0.);
    Pag g_polyn = sum->add_g ('n', pp);
    //    Ginverse [1., cos] ->inv_cos
    Pag g_inv_cos = sum->add_g ('d', vector2 (Pag(1.), g_cos));
    //    Gproduct [inv_cos, inv_cos] -> inv_cos2
    Pag g_inv_cos2 = sum->add_g ('p', vector2 (g_inv_cos, g_inv_cos));
    //    Gsum_of_prod [Z, inv_cos2, 1., polyn] -> G_G2
    Pag g_z = p.fwhm_refinable.get_pag(3);
    Pag g_gg2 = sum->add_g ('k', vector4(g_z, g_inv_cos2, g_polyn,Pag(1.)));
    //    Gsqrt [G_G2] -> G_G
    Pag g_gg = sum->add_g ('q', vector1(g_gg2));
    
    //G_L = X tan(theta) + Y / cos(theta)
    
    //    Gsum_of_prod [X, tan, Y, inv_cos] -> G_L
    Pag g_x = p.fwhm_refinable.get_pag(4);
    Pag g_y = p.fwhm_refinable.get_pag(5);
    Pag g_gl = sum->add_g ('k', vector4 (g_x, g_tan, g_y, g_inv_cos));

    //G = (G_G^5 + A G_G^4 G_L + B G_G^3 G_L^2 + C G_G^2 G_L^3 
    //                                          + D G_G G_L^4 + G_L^5)^0.2
    // A, B, C, D = 2.69269, 2.42843, 4.47163, 0.07842 

    Pag g_g = sum->add_g ('F', vector2(g_gg, g_gl));

    //G_L, G -> shape_refinable
    //p.shape_refinable.recursive_rm();
    p.shape_refinable = PagContainer (sum, vector2(g_gl, g_g));

    return g_g;
}

Pag Crystal::fwhm_of_peak_LoweMa (fp fwhm, Phase& p, int xray_nr, Pag c_g) 
{
    // sqrt (U tan^2(2theta) + V tan(2theta) + W + CT cot^2(2theta))
    fp hwhm = fwhm / 2;
    if (p.fwhm_refinable.pags_size() == 4) {
        Pag g_tan = sum->add_g ('t', vector2(c_g, Pag(M_PI/180./2.)));
        vector<Pag> pp = p.fwhm_refinable.copy_of_pags();
        pp.insert (pp.end() - 1, Pag(0.));
        pp.insert (pp.begin(), g_tan);
        Pag g1 = sum->add_g ('r', pp);
        return sum->add_g ('q', vector1(g1));
    }
    else if (p.fwhm_refinable.pags_size() == 1) {
        warn ("Can't mix ICCD-recommendation and equal-in-phase in "
                "constrained-fwhm. Set to equal-in-phase.");
        constrained_fwhm = 'e';
        return fwhm_of_peak (fwhm, p, xray_nr, c_g);
    }
    else if (p.fwhm_refinable.pags_size() == 0) {
        Pag u = sum->pars()->add_a (0., Domain());
        Pag v = sum->pars()->add_a (0., Domain());
        Pag w = sum->pars()->add_a (hwhm * hwhm, Domain());
        Pag ct = sum->pars()->add_a (0., Domain());
        vector<Pag> rf = vector4 (u, v, w, ct);
        p.fwhm_refinable = PagContainer (sum, rf);
        return fwhm_of_peak (fwhm, p, xray_nr, c_g);
    }
    else {
        assert(0);
        return Pag();
    }
}

Pag Crystal::fwhm_of_peak (fp fwhm, Phase& p, int xray_nr, Pag c_g) 
{
    static Pag n0;
    if (peak_type == 'T') {
        return fwhm_of_peak_TCHpV(fwhm, p, c_g);
    }
    fp hwhm = fwhm / 2;
    switch (constrained_fwhm) {
        case 'n':
            return sum->pars()->add_a (hwhm, Domain());
        case 'p':
            if (xray_nr == 0)
                n0 = sum->pars()->add_a (hwhm, Domain());
            return n0;
        case 'e':
            if (p.fwhm_refinable.pags_size() == 1)
                return p.fwhm_refinable.get_pag(0);
            else if (p.fwhm_refinable.pags_size() == 4) {
                warn ("Can't mix equal-in-phase and ICCD-recommendation in "
                        "constrained-fwhm. Set to ICCD-recommendation.");
                constrained_fwhm = 'i';
                return fwhm_of_peak (fwhm, p, xray_nr, c_g);
            }
            else if (p.fwhm_refinable.pags_size() == 0) {
                Pag a = sum->pars()->add_a (hwhm, Domain());
                p.fwhm_refinable = PagContainer (sum, vector1(a));
                return p.fwhm_refinable.get_pag(0);
            }
            else 
                assert(0);
        case 'i':
            return fwhm_of_peak_LoweMa(fwhm, p, xray_nr, c_g);
        default:
            assert (0);
    }
    return Pag();
}

Pag Crystal::shape_of_peak_TCHpV (Phase& p) 
{
    assert(peak_type = 'T');
    //ignoring `constrained_shape' 
    if (p.shape_refinable.pags_size() != 2) 
        return Pag();
    else {
        Pag q = sum->add_g('d', p.shape_refinable.copy_of_pags());
        vector<Pag> rr(5);
        rr[0] = q;
        rr[1] = Pag(0.);
        rr[2] = Pag(1.36603);
        rr[3] = Pag(0.47719);
        rr[4] = Pag(0.1116);
        p.shape_refinable.clear(); //shape_refinable was used 
                                   //only temporarily
        return sum->add_g ('n', rr);
    }
}

Pag Crystal::shape_of_peak_func_theta (Phase& p, int xray_nr, Pag c_g)
{
    assert(constrained_shape == 't');
    int refin_size = p.shape_refinable.pags_size();
    if (peak_type == 'P') {
        if (refin_size == 0) { //initialization
            Pag na = sum->pars()->add_a (2., Domain());
            Pag nb = sum->pars()->add_a (0., Domain());
            Pag nc = sum->pars()->add_a (0., Domain());
            vector<Pag> rf = vector3 (na, nb, nc);
            p.shape_refinable = PagContainer (sum, rf);
            return shape_of_peak (p, xray_nr, c_g);
        }
        else if (refin_size == 3) {
            //na + nb/(2theta) + nc/(2theta)^2
            vector<Pag> pp = p.shape_refinable.copy_of_pags();
            pp.insert (pp.begin(), c_g);
            pp.insert (pp.begin() + 1, Pag(0.));
            pp.insert (pp.begin() + 1, Pag(0.));
            return sum->add_g ('r', pp);
        }
        else {
            warn("Can't mix some constrained-shape options or some "
                    "peak-types in the same phase. Set to equal-in-plane.");
            constrained_shape = 'p';
            return shape_of_peak (p, xray_nr, c_g);
        }
    }
    else if (peak_type == 'S' || peak_type == 'V') {
        // na + nb * (2theta)
        if (refin_size == 0) { //initialization
            Pag na = sum->pars()->add_a (0.5, Domain());
            Pag nb = sum->pars()->add_a (0., Domain());
            vector<Pag> rf = vector2 (na, nb);
            p.shape_refinable = PagContainer (sum, rf);
            return shape_of_peak (p, xray_nr, c_g);
        }
        else if (refin_size == 2) {
            vector<Pag> pp = vector4 (Pag(1.), p.shape_refinable.get_pag(0),
                                      p.shape_refinable.get_pag(1), c_g);
            return sum->add_g ('k', pp);
        }
        else {
            warn("Can't mix some constrained-shape options or some "
                    "peak-types in the same phase. "
                    "Set to equal-in-plane.");
            constrained_shape = 'p';
            return shape_of_peak (p, xray_nr, c_g);
        }
    }
    else { 
        assert(0); 
        return Pag(); 
    }
}

Pag Crystal::shape_of_peak_not_func_theta (Phase& p, int xray_nr)
{
    static Pag s_p;
    fp value = 0;
    fp domain_from = 0, domain_to = 0;
    bool lower_bound = true, upper_bound = false;
    switch (peak_type) {
        case 'P':
            value = 2.;
            domain_from = 0.5, domain_to = 5;
            break;
        case 'S':
            value = 0.5;
            domain_from = 0., domain_to = 1.;
            upper_bound = true;
            break;
        case 'V':
            value = 1.0;
            domain_from = 0., domain_to = 10.;
            break;
        default: 
            assert(0);
    }
    Domain domain((domain_from + domain_to) / 2, (domain_to - domain_from) / 2);
    int refin_size = p.shape_refinable.pags_size();
    switch (constrained_shape) {
        case 'n':
            return sum->pars()->add_a (value, domain);
        case 'p':
            if (xray_nr == 0)
                s_p = sum->pars()->add_a (value, domain);
            return s_p;
        case 'c': 
            if (xray_nr == 0) {  
                s_p = bounded_a (value, lower_bound, domain_from, 
                                 upper_bound, domain_to);
            }
            return s_p;
        case 'e':
            if (refin_size == 0) { //initialization
                Pag a = sum->pars()->add_a (value, domain);
                p.shape_refinable = PagContainer (sum, vector1(a));
                return p.shape_refinable.get_pag(0);
            }
            else if (refin_size == 1)
                return p.shape_refinable.get_pag(0);
            else {
                warn("Can't mix some constrained-shape options. "
                        "Set to equal-in-plane.");
                constrained_shape = 'p';
                return shape_of_peak_not_func_theta (p, xray_nr);
            }
        default:
            assert(0);
            return Pag();
    }
}

Pag Crystal::bounded_a (fp value, bool bounded_from, fp from, 
                                  bool bounded_to, fp to)
{
    if (bounded_from && bounded_to) {
        if (value < from || value > to)
            value = (from + to) / 2.;
        fp sin_param = asin((2 * value - from - to) / (to - from));
        Pag tmp = sum->pars()->add_a (sin_param, Domain (-M_PI/2., M_PI/2.));
        return sum->add_g ('b', vector3(tmp, Pag(from), Pag(to)));  
    }
    else if (bounded_from && !bounded_to) {
        if (value < from)
            value = from;
        Pag tmp = sum->pars()->add_a (sqrt(value - from), 
                                     Domain (0., sqrt(to - from)));
        return sum->add_g ('m', vector2(tmp, Pag(from)));
    }
    else if (!bounded_from && bounded_to) {
        if (value > to)
            value = to;
        Pag tmp = sum->pars()->add_a (sqrt(to - value), 
                                     Domain (0., sqrt(to - from)));  
        return sum->add_g ('M', vector2(tmp, Pag(to)));
    }
    else // !bounded_from && !bounded_to
        return sum->pars()->add_a (value, Domain((from+to)/2, (from-to/2)));  
}
            
Pag Crystal::shape_of_peak (Phase& p, int xray_nr, Pag c_g)
{
    static Pag s_p;
    switch (peak_type) {
        case 'P':
        case 'S':
        case 'V':
            if (constrained_shape == 't')
                return shape_of_peak_func_theta (p, xray_nr, c_g);
            else
                return shape_of_peak_not_func_theta (p, xray_nr);
        case 'G':
        case 'L':
            return Pag();
        case 'T':
            return shape_of_peak_TCHpV(p);

        default:
            warn ("No such peak type: ^" + S(peak_type));
            return Pag();
    }
}

int Crystal::add_plane_as_f (int phase_nr, Hkl hkl, vector<int> ff) 
{
    if (hkl.invalid()) return -1;
    if (!is_index (phase_nr, phases)) {
        warn ("No such phase: %" + S(phase_nr));
        return -1;
    }
    if (phases[phase_nr].planes.count(hkl) != 0) {
        warn ("Plane already exists.");
        return -1;
    }
    HklF temp_hkl (hkl, sum, ff);
    temp_hkl.c_name = "%" + S(phase_nr) + hkl.str();
    if (temp_hkl.is_ok(sum)) {
        phases[phase_nr].planes.insert (temp_hkl); 
        verbose ("Plane %" + S(phase_nr) + " " + hkl.str() + " added.");
    }
    return 0;
}

fp Crystal::sum_of_wavelength_ratio () const
{
    if (xrays.get_count() == 1)
        return 1.;
    fp ratio_sum = 0;
    for (int i = 0; i < xrays.get_count(); i++)
        ratio_sum += xrays.intensity(i);
    return ratio_sum;
}

fp Crystal::peak_center (fp lambda, const Phase& p, Hkl hkl, fp *d) const
{
    vector<Pag> params;
    char g_type = inverse_of_d (p, hkl, &params);
    V_g *g = V_g::factory (0, g_type, params);
    assert (g);
    fp inv_d = g->g_value (sum->pars()->values(), sum->gfuncs_vec());
    delete g;
    if (d)
        *d = 1. / inv_d;
    return 2 // 2theta
            * 180 / M_PI //rad -> deg
            * asin (lambda / 2 * inv_d); //lambda == 2 d sin(theta)
}

Pag Crystal::add_center_g (const Phase& p, Hkl hkl, int xray_nr) const
{
    static Pag inv_d;
    vector<Pag> params;
    char g_type = inverse_of_d (p, hkl, &params);
    if (xray_nr == 0)
        inv_d = sum->add_g (g_type, params);
    Pag lambda = xrays.l_pag(xray_nr);
    return sum->add_g ('X', vector2 (inv_d, lambda));
}

char Crystal::inverse_of_d (const Phase& p, Hkl hkl, vector<Pag>* params) const
{
    int h = hkl.h, k = hkl.k, l = hkl.l;
    char g_type = 0;
    assert (params->empty());
    switch (p.typ) {
        case 'c': //cubic 
            // 1 / d == sqrt (h * h + k * k + l * l) / a
            assert (p.pags_size() == 1);
            params->push_back (Pag (sqrt (static_cast<fp>(h*h + k*k + l*l))));
            params->push_back (p.get_pag(0));
            g_type = 'd';
            break;
        case 't': //tetragonal 
            // 1 / d == sqrt ((h * h + k * k ) / (a * a) + l * l / (c * c))
            assert (p.pags_size() == 2);
            params->push_back (Pag(static_cast<fp>(h * h + k * k)));
            params->push_back (p.get_pag(0));
            params->push_back (Pag(static_cast<fp>(l * l)));
            params->push_back (p.get_pag(1));
            params->push_back (Pag(0.));
            params->push_back (Pag(1.));
            g_type = 'x';
            break;
        case 'o': //orthorhombic 
            // 1 / d == sqrt (h * h / a / a + k * k / b / b + l * l / c / c)
            assert (p.pags_size() == 3);
            params->push_back (Pag(static_cast<fp>(h * h)));
            params->push_back (p.get_pag(0));
            params->push_back (Pag(static_cast<fp>(k * k)));
            params->push_back (p.get_pag(1));
            params->push_back (Pag(static_cast<fp>(l * l)));
            params->push_back (p.get_pag(2));
            g_type = 'x';
            break;
        case 'h': //hexagonal
            // 1 / d == sqrt (4./3. * (h*h + k*k + h*k) / a / a + l*l / c / c)
            assert (p.pags_size() == 2);
            params->push_back (Pag(4./3. * (h * h + k * k + h * k)));
            params->push_back (p.get_pag(0));
            params->push_back (Pag(static_cast<fp>(l * l)));
            params->push_back (p.get_pag(1));
            params->push_back (Pag(0.));
            params->push_back (Pag(1.));
            g_type = 'x';
            break;
        default :
            assert(0);
    }
    return g_type;
}

string Crystal::print_estimate (int phase_nr, Hkl hkl, fp w, int wave_nr) const
{
    if (hkl.invalid()) return "";
    if (wave_nr < 0 || wave_nr >= xrays.get_count()) {
        warn ("Wavelength not defined (yet)");
        return "";
    }
    if (!is_index (phase_nr, phases)) {
        warn ("No such phase: %" + S(phase_nr));
        return "";
    }
    fp center = peak_center (xrays.lambda(wave_nr), phases[phase_nr], hkl);
    return my_manipul->print_simple_estimate (center, w);
}

void Crystal::rm_plane (int phase_nr, Hkl hkl)
{
    if (hkl.invalid()) return;
    if (!is_index (phase_nr, phases)) {
        warn ("No such phase: %" + S(phase_nr));
        return;
    }
    int count = phases[phase_nr].planes.count(hkl);
    if (count > 0) {
        info ("Plane %" + S(phase_nr) + " " + hkl.str() + " removed.");
        phases[phase_nr].planes.erase(hkl);
    }
    else 
        warn ("No such plane.");
}

vector<int> Crystal::get_funcs_in_phase (int phase_nr) const
{
    vector<int> f;
    if (!is_index (phase_nr, phases)) {
        warn ("No such phase: %" + S(phase_nr));
        return int_v0;
    }
    const set<HklF> &pl = phases[phase_nr].planes;
    for (set<HklF>::const_iterator i = pl.begin(); i != pl.end(); ++i)
        f.insert (f.end(), i->get_ff().begin(), i->get_ff().end());
    return f;
}

const vector<int> &Crystal::get_funcs_in_plane (int phase_nr, Hkl hkl) const
{
    if (!is_index (phase_nr, phases)) 
        return int_v0;
    if (phases[phase_nr].planes.count (hkl))
        return phases[phase_nr].planes.find(hkl)->get_ff();
    else {
        warn ("Plane " + hkl.str() + " not found.");
        return int_v0;
    }
}

string Crystal::phase_info(int nr) const
{
    ostringstream os;
    if (nr == -1) { // info about all phases
        if (phases.empty())
            return "No defined phases"; 
        string s;
        for (unsigned int i = 0; i < phases.size(); i++)
            s += phase_info(i) + (i != phases.size() - 1 ? "\n" : "");
        return s;
    }
    else if (!is_index (nr, phases)) {
        warn ("No phase %" + S(nr));
        return "";
    }
    os << "%" << nr << ": ";
    const Phase &p = phases[nr];
    switch (p.typ) {
        case 'c': //cubic:
            os << "Cubic system with a: " << p.ph_str(0);
            break;
        case 't': //tetragonal:
            os << "Tetragonal system with a: " << p.ph_str(0) 
                << ";  c: " << p.ph_str(1);
            break;
        case 'o': //orthorhombic:
            os << "Orthorhombic system with a: " << p.ph_str(0) 
                << ";  b: " << p.ph_str(1) 
                << ";  c: " << p.ph_str(2);
            break;
        case 'h': //hexagonal:
            os << "Hexagonal system with a: " << p.ph_str(0) 
                << ";  c: " << p.ph_str(1);
            break;
        default:
            assert(0);
            break;
    }
    return os.str();
}

string Crystal::list_planes_in_phase (int phase_nr) const
{
    if (phase_nr == -1) { // about all phases
        string s;
        for (unsigned int i = 0; i < phases.size(); i++)
            s += list_planes_in_phase(i) + "\n";
        return s;
    }
    if (!is_index (phase_nr, phases)) {
        warn ("No such phase: %" + S(phase_nr));
        return "";
    }
    const Phase& p = phases[phase_nr];
    string s = "Planes defined in phase %" + S(phase_nr) + ":";
    if (p.planes.empty())
        s += " none";
    else {
        for (set<HklF>::const_iterator i = p.planes.begin(); 
                                                    i != p.planes.end(); i++)
            s += " " + i->str();
    }
    return s;
}

string Crystal::plane_info (int phase_nr, Hkl hkl) const
{ 
    if (hkl.invalid()) return "";
    if (!is_index (phase_nr, phases)) {
        warn ("No such phase: %" + S(phase_nr));
        return "";
    }
    if (phases[phase_nr].planes.count(hkl) == 0) { 
        warn ("No such plane.");
        return "";
    }
    const vector<int> &ff = get_funcs_in_plane (phase_nr, hkl);
    string s = "Plane %" + S(phase_nr) + " " + hkl.str() 
        + " is represented by " + S(ff.size()) + " peak(s):";
    for (vector<int>::const_iterator i = ff.begin(); i != ff.end(); i++)
        s += "\n" + sum->info_fzg (fType, *i);
    return s;
}

string Crystal::wavelength_info() const
{
    string s;
    if (xrays.get_count() == 0)
        s = "Wavelength(s) not defined (yet)";
    else if (xrays.get_count() == 1)
        s = "Wavelength: " + xrays.lambda_str(0);
    else if (xrays.get_count() == 2)
        s = "Wavelengths: " + xrays.lambda_str(0) + ", " 
            + xrays.lambda_str(1) + ", ratio: " 
            + xrays.intensity_str(0) + " : " + xrays.intensity_str(1);
    else {
        s = "Wavelengths (in () intensity ratio): ";
        for (int i = 0; i < xrays.get_count(); i++) {
            s += xrays.lambda_str(i) + " (" 
                + xrays.intensity_str(i) +"),  ";
        }
    }
    return s;
}

void Crystal::export_as_script (std::ostream& os) const
{
    //should be exported after Sum::export_as_script
    os << "# exporting crystal -- begin" << endl;
    os << set_script('c');
    if (xrays.get_count() != 0) {
        os << "c.wavelength "; 
        for (int i = 0; i < xrays.get_count(); i++)
            os << (i == 0 ? "" : ", ") 
                << xrays.l_pag(i).str() << " " << xrays.in_pag(i).str();
        os << endl;
    }
    for (vector<Phase>::const_iterator i = phases.begin(); i!=phases.end();i++){
        os << "c.add %" << i->typ;
        for (unsigned int j = 0; j < i->latt_0.size(); j++)
            os << " " << i->pag_str(j);
        os << endl;
        for (set<HklF>::const_iterator j = i->planes.begin(); 
                                                j != i->planes.end(); j++) {
            os << "c.add %" << i - phases.begin() << " " << j->str();
            const vector<int> &ff = j->get_ff();
            for (vector<int>::const_iterator k = ff.begin(); k != ff.end(); k++)
                os << " ^" << *k;
            os << endl;
        }
    }
    os << "# exporting crystal -- end" << endl;
}


