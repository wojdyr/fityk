// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef CrystaL__H__
#define CrystaL__H__
#include <vector>
#include <map>
#include <set>
#include "common.h"
#include "pag.h"
#include "fzgbase.h"
#include "dotset.h"
#include "sum.h"

class Hkl : public pre_Hkl
{
public:
    Hkl (int h_, int k_, int l_) { h = h_; k = k_; l = l_; } 
    Hkl (const pre_Hkl &phkl) : pre_Hkl(phkl) {} 
    std::string str() const; 
    bool invalid() { return h == 0 && k == 0 && l == 0; }
};

class HklF : public Hkl, public FuncContainer
{
public:
    HklF(Hkl hkl, Sum *s, const std::vector<int> &f) 
        : Hkl(hkl), FuncContainer(s,f){}
    HklF(Hkl hkl) : Hkl(hkl), FuncContainer(0, int_v0) {};
    bool operator== (const HklF &a) const { return h==a.h && k==a.k && l==a.l; }
    bool operator!= (const HklF &a) const { return !(operator==(a)); }
    bool operator< (const HklF &a) const;
};

class Phase : public PagContainer
{
public:
    char typ;
    std::vector<fp> latt_0;
    std::set<HklF> planes;
    PagContainer fwhm_refinable;
    PagContainer shape_refinable;

    Phase (Sum *s, char t, const std::vector<Pag>& latt_a);
    ~Phase() {}
    std::string ph_str(int n) const;
    void set_containers_descriptions(int n);
};

class Wavelengths : public PagContainer
{
public:
    std::vector<fp> initial_values;

    Wavelengths (Sum* sum) : PagContainer(sum) { c_name = "wavelength"; }
    fp lambda(int n) const; 
    fp intensity(int n) const;
    std::string lambda_str(int n) const; 
    std::string intensity_str(int n) const; 
    Pag l_pag(int n) const;
    Pag in_pag(int n) const;
    int get_count() const { return pags.size() / 2; }
    int add (std::vector<Pag> p);
    void clear() { pags.clear(); initial_values.clear(); }
};

class Crystal : public DotSet
{
public:
    Wavelengths xrays;

    Crystal(Sum *s); 
    ~Crystal() {} 
    int add_phase (char type, std::vector<Pag> pg);
    std::string phase_type_info() const;
    int rm_phase (int phase_nr);
    int add_plane (int phase_nr, Hkl hkl);
    int add_plane_as_f (int phase_nr, Hkl hkl, std::vector<int> ff);
    void rm_plane (int phase_nr, Hkl hkl); 
    int get_nr_of_phases() const {return phases.size();}
    std::string phase_info(int nr) const; 
    std::string list_planes_in_phase (int phase_nr) const;
    std::vector<int> get_funcs_in_phase (int phase_nr) const;
    const std::vector<int> &get_funcs_in_plane (int phase_nr, Hkl hkl) const;
    std::string plane_info (int phase_nr, Hkl hkl) const;
    std::string wavelength_info() const; 
    std::string print_estimate (int phase_nr, Hkl hkl, fp w = -1., 
                                int wave_nr = 0) const;
    void export_as_script (std::ostream& os) const;
    Pag bounded_a (fp value, bool lower_bound, fp from, 
                             bool upper_bound, fp to);

private:
    Sum *sum;
    std::vector<Phase> phases;
    char peak_type;
    char constrained_shape, constrained_fwhm;
    std::map<char, std::string> map_peak_type, map_c_fwhm, map_c_shape;

    fp sum_of_wavelength_ratio () const;
    Pag add_center_g (const Phase& p, Hkl hkl, int xray_nr) const;
    fp peak_center (fp lambda, const Phase& p, Hkl hkl, fp *d = 0) const;
    char inverse_of_d (const Phase& p, Hkl hkl, std::vector<Pag>* params) const;
    Pag fwhm_of_peak (fp fwhm, Phase& p, int xray_nr, Pag c_g);
    Pag shape_of_peak (Phase& p, int xray_nr, Pag c_g);
    Pag fwhm_of_peak_TCHpV(fp fwhm, Phase& p, Pag c_g);
    Pag fwhm_of_peak_LoweMa(fp fwhm, Phase& p, int xray_nr, Pag c_g);
    Pag shape_of_peak_TCHpV (Phase& p); 
    Pag shape_of_peak_func_theta (Phase& p, int xray_nr, Pag c_g);
    Pag shape_of_peak_not_func_theta (Phase& p, int xray_nr);
    std::vector<Pag> heights_of_peaks(fp &h, Pag* a_for_h_correction) const;
};

extern Crystal *my_crystal;

#endif
