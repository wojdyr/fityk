// This file is part of fityk program. Copyright (C) Marcin Wojdyr
#include "common.h"
RCSID ("$Id$")

#include "fzgbase.h"
#include <algorithm>
#include "ffunc.h"
#include "gfunc.h"
#include "sum.h"

using namespace std;


PagContainer::PagContainer (Sum *s) : PFContainer(s) 
    { if (sum) sum->register_pc(this, true); }

PagContainer::PagContainer (Sum *s, const std::vector<Pag> &v) 
        : PFContainer(s), pags(v) 
    { if (sum) sum->register_pc(this, true); }

PagContainer::PagContainer (const PagContainer& p) 
        : PFContainer(p), pags(p.pags) 
    { if (sum) sum->register_pc(this, true); }

PagContainer::~PagContainer () 
{ 
    if (sum) {
        recursive_rm();
        sum->register_pc(this, false); 
    }
}

PagContainer& PagContainer::operator= (const PagContainer& r)
{ 
    if (sum) {
        recursive_rm();
        sum->register_pc(this, false); 
    }
    sum = r.sum; 
    pags = r.pags; 
    if (sum) 
        sum->register_pc(this, true); 
    return *this; 
}

void PagContainer::recursive_rm () 
{
    if (sum && sum->is_remove_recursive()) {
        while (!pags.empty()) {
            vector<Pag>::iterator i = pags.end() - 1;
            if (i->is_p())
                pags.erase(i);
            else if (i->is_a()) { 
                int n = i->a();
                pags.erase(i);
                sum->rm_a_core (n);
            }
            else if (i->is_g()) {
                int n = i->g();
                pags.erase(i);
                sum->rm_fzg (gType, n, true);
            }
        }
    }
    else
        pags.clear();
}

bool PagContainer::is_ok (const Sum* with_sum) const
{
    if (!with_sum) 
        return sum ? is_ok(sum) : false;
    bool ok = true;
    for (vector<Pag>::const_iterator i = pags.begin(); i != pags.end(); i++) {
        if (i->is_a() && (i->a() < 0 || i->a() >= with_sum->count_a())) {
            warn ("@" + S(i->a()) + " not found.");
            ok = false;
        }
        else if (i->is_g() 
                 && (i->g() < 0 || i->g() >= with_sum->fzg_size(gType))) {
            warn ("$" + S(i->g()) + " not found.");
            ok = false;
        }
    }
    return ok;
}

std::vector<fp> PagContainer::values_of_pags() const
{
    const vector<fp>& aa = sum->current_a();
    const vector<V_g*>& gg = sum->gfuncs_vec();
    vector<fp> v;
    for (vector<Pag>::const_iterator i = pags.begin(); i != pags.end(); i++)
        v.push_back (i->value (aa, gg));
    return v;
}

int PagContainer::count_refs (Pag p) const
{
    return count (pags.begin(), pags.end(), p);
}

void PagContainer::synch_after_rm (Pag p)
{
    for (vector<Pag>::iterator i = pags.begin(); i != pags.end(); i++)
        i->synch(p);
}

//==================

FuncContainer::FuncContainer(Sum *s, const std::vector<int> &f) 
        : PFContainer(s), ff(f) 
    { if (sum) sum->register_fc (this, true); }

FuncContainer::FuncContainer(const FuncContainer& v) 
        : PFContainer(v), ff(v.ff) 
    { if (sum) sum->register_fc (this, true); }

FuncContainer::~FuncContainer() 
{ 
    if (sum) {
        sum->register_fc (this, false);
        if (sum->is_remove_recursive()) 
            sum->multi_rm_fzg (fType, ff, true);
    }
}

FuncContainer& FuncContainer::operator= (const FuncContainer& r) 
{ 
    if (sum) 
        sum->register_fc (this, false);
    sum = r.sum; 
    ff = r.ff;
    if (sum)
        sum->register_fc (this, true);
    return *this;
}

bool FuncContainer::is_ok (const Sum *with_sum) const 
{ 
    if (!with_sum) 
        return sum ? is_ok(sum) : false;
    bool ok = true;
    for (std::vector<int>::const_iterator i = ff.begin(); i != ff.end(); i++) 
        if (*i < 0 || *i >= sum->fzg_size(fType)) {
            warn (V_fzg::short_type(fType) + S(*i) + " not found.");
            ok = false;
        }
    return ok;
}

void FuncContainer::synch_after_rm_f (int fn)
{
    for (vector<int>::iterator i = ff.begin(); i != ff.end(); i++)
            if (*i > fn)
                --(*i);
}

int FuncContainer::count_refs (int fn) const
{
    return count (ff.begin(), ff.end(), fn);
}

//===================

void V_fzg::prepare_av (const vector<fp>& A, const vector<V_g*>& G)
{
    assert (av.size() >= pags.size());
    for (unsigned int i = 0; i < pags.size(); i++)
        av[i] = pags[i].value (A, G);
}

string V_fzg::ps (int n, const vector<fp>& A, const vector<V_g*>& G) 
{ 
    assert (n >= 0 && n < g_size);
    return A.empty() ? pags[n].str() : pags[n].str(A, G); 
}

vector<const z_names_type*> V_fzg::all_types (One_of_fzg t)
{
    vector<const z_names_type*> v;
    switch (t) {
        case fType:
            for (int i = 0; V_f::f_names[i].type != 0; i++) 
                v.push_back(&V_f::f_names[i]);
            break;
        case gType:
            for (int i = 0; V_g::g_names[i].type != 0; i++) 
                v.push_back(&V_g::g_names[i]);
            break;
        case zType:
            for (int i = 0; V_z::z_names[i].type != 0; i++) 
                v.push_back(&V_z::z_names[i]);
            break;
    }
    return v;
}

V_fzg* V_fzg::add (One_of_fzg fzg, Sum *sum, char type, const vector<Pag> &p)
{
    if (PagContainer(0, p).is_ok(sum) == false)
        return 0;
    V_fzg *h = 0;
    switch (fzg) {
        case fType:
            h = V_f::factory (sum, type, p);
            break;
        case gType:
            h = V_g::factory (sum, type, p);
            break;
        case zType:
            h = V_z::factory (sum, type, p);
            break;
    }
    if (h == 0)
        return 0;
    if (h->init_ok() == false) {
        warn (h->full_type() + " (" + h->type_info()->name + ") "
                "requires " + S(h->g_size) + " parameters.");
        delete h;
        return 0;
    }
    return h;
}

string V_fzg::print_type_info (One_of_fzg fzg, char t) 
{
    const z_names_type *zt = V_fzg::type_info(fzg, t);
    if (zt)
        return (fzg == fType ? 
                zt->name + "\n" + static_cast<const f_names_type*>(zt)->formula 
                : zt->name);
    else {
        vector<const z_names_type*> va = all_types(fzg);
        string s = "Available " + full_type(fzg, false) + "s: "; 
        for (vector<const z_names_type*>::const_iterator i = va.begin(); 
                i != va.end(); i++)
            s += (i == va.begin() ? "": ", ") + short_type(fzg) + S((*i)->type);
        s += ".\nTry `s.info " + V_fzg::short_type(fzg) + "X' for each";
        return s;
    }
}

const z_names_type* V_fzg::type_info (One_of_fzg fzg, char t)
{
    if (fzg == fType) {
        for (int i = 0; V_f::f_names[i].type != 0; i++) 
            if (V_f::f_names[i].type == t) 
                return &V_f::f_names[i];
    }
    else if (fzg == zType) {
        for (int i = 0; V_z::z_names[i].type != 0; i++) 
            if (V_z::z_names[i].type == t) 
                return &V_z::z_names[i];
    }
    else if (fzg == gType) {
        for (int i = 0; V_g::g_names[i].type != 0; i++) 
            if (V_g::g_names[i].type == t) 
                return &V_g::g_names[i];
    }
    //if we are here, type t not found
    return 0;
}

string V_fzg::full_type (One_of_fzg fzg, bool with_symbol) 
{
    string s;
    switch (fzg) {
        case fType: s += "f-function"; break;
        case zType: s += "zero-shift"; break;
        case gType: s += "g-parameter"; break;
    }
    if (with_symbol)
        s += " " + short_type(fzg); 
    return s;
}

string V_fzg::short_type (One_of_fzg fzg) 
{
    switch (fzg) {
        case fType: return "^";
        case zType: return "<";
        case gType: return "$";
        default: return "";
    }
}

One_of_fzg V_fzg::type_of_symbol (char symbol) 
{
    switch (symbol) {
        case '^' : return fType;
        case '<' : return zType;
        case '$' : return gType;
        default: assert(0); return /*to suppress compiler warning*/fType;
    }
}


