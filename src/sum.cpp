// This file is part of fityk program. Copyright (C) Marcin Wojdyr
#include "common.h"
RCSID ("$Id$")

#include "sum.h"
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include "ffunc.h"
#include "gfunc.h"
#include "data.h"

using namespace std;

Sum::Sum () 
    : s_was_changed(true), 
      nA(0), history (vector1(History_item(fp_v0, "new"))), hp(0),
      adomain(), afrozen(),
      cut_tails(false), cut_level(10.),
      numerical_deriv(false), numerical_h(1e-4), two_side(false),
      recursive_remove(true), def_rel_domain_width(0.1) 
{
    bpar ["cut-tails"] = &cut_tails;
    fpar ["cut-tails-level"] = &cut_level;
    bpar ["numerical-d"] = &numerical_deriv;
    fpar ["numerical-d-rel-change"] = &numerical_h;
    bpar ["numerical-d-both-sides"] = &two_side;
    bpar ["recursive-remove"] = &recursive_remove;
    fpar ["default-relative-domain-width"] = &def_rel_domain_width;
}

Sum::~Sum()
{
    for (set<FuncContainer*>::iterator i=f_usage.begin(); i!=f_usage.end(); i++)
        (*i)->bye_from_sum(this);
    for (set<PagContainer*>::iterator i=vp_usage.begin(); i!=vp_usage.end();i++)
        (*i)->bye_from_sum(this);
    for (vector<V_f*>::iterator i = fvec.begin(); i != fvec.end(); i++)
        delete *i;
    for (vector<V_g*>::iterator i = gvec.begin(); i != gvec.end(); i++)
        delete *i;
    for (vector<V_z*>::iterator i = zvec.begin(); i != zvec.end(); i++)
        delete *i;
}

/*
Sum& Sum::operator= (Sum& r)
{
    this->DotSet::operator=(r);
    history = r.history;
    adomain = r.adomain;
    afrozen = r.afrozen;
    av_numder = r.av_numder;
    nA = r.nA;
    hp = r.hp; 

    for (set<FuncContainer*>::iterator i=f_usage.begin(); i!=f_usage.end(); i++)
        (*i)->bye_from_sum(this);
    for (set<PagContainer*>::iterator i=vp_usage.begin(); i!=vp_usage.end();i++)
        (*i)->bye_from_sum(this);
    for (vector<V_f*>::iterator i = fvec.begin(); i != fvec.end(); i++)
        delete *i;
    for (vector<V_g*>::iterator i = gvec.begin(); i != gvec.end(); i++)
        delete *i;
    for (vector<V_z*>::iterator i = zvec.begin(); i != zvec.end(); i++)
        delete *i;

    f_usage = r.f_usage;
    vp_usage = r.vp_usage;
    fvec = r.fvec;
    gvec = r.gvec;
    zvec = r.zvec;

    Sum& mr = const_cast<Sum&>(r);
    mr.f_usage.clear();
    mr.vp_usage.clear();
    mr.fvec.clear();
    mr.gvec.clear();
    mr.zvec.clear();

    return *this;
}
*/

void Sum::use_param_a_for_value (const vector<fp>& A)
{
    if (nA == 0)
        return;
    if (A.empty()) {
        use_param_a_for_value(current_a());
        return;
    }
    for (vector<V_f*>::iterator i = fvec.begin(); i != fvec.end(); i++)
        (*i)->pre_compute (A, gvec);
    for (vector<V_z*>::iterator i = zvec.begin(); i != zvec.end(); i++)
        (*i)->pre_compute (A, gvec);
    av_numder = A;
    if (cut_tails) //neglecting zero-shift here  
        for (vector<V_f*>::iterator i = fvec.begin(); i != fvec.end(); i++)
            (*i)->get_range(cut_level);
}
    
fp Sum::value (fp x) const
    // pre: use_param_a_for_value() called (or not in special cases)
{
    x += zero_shift(x);
    fp y = 0;
    for(vector<V_f*>::const_iterator i = fvec.begin(); i != fvec.end(); i++)
        if (!cut_tails || (*i)->range_l <= x && x <= (*i)->range_r)
            y += (*i)->compute (x, 0);
    return y;
}

fp Sum::f_value (fp x, int fn) const
    // pre: use_param_a_for_value() called (or not in special cases)
{
    assert (0 <= fn && fn < size(fvec));
    //assuming zero_shift is small
    if (!cut_tails || fvec[fn]->range_l <= x && x <= fvec[fn]->range_r) {
        x += zero_shift(x);
        return fvec[fn]->compute (x, 0);
    }
    else
        return 0;
}

fp Sum::funcs_value (const vector<int>& fn, fp x) const
    // pre: use_param_a_for_value() called
{
    //no need for optimization
    fp y = 0;
    for (vector<int>::const_iterator i = fn.begin(); i != fn.end(); i++) 
        y += f_value (x, *i);
    return y;
}

fp Sum::value_and_put_deriv (fp x, vector<fp>& dy_da) 
    // pre: use_param_a_for_value() called
    // this func does _not_ take into consideration Domain::frozen
{
    assert(size(dy_da) == nA);
    fill (dy_da.begin(), dy_da.end(), 0);
    if (numerical_deriv == false) {
        x += zero_shift(x);
        fp y = 0;
        fp dy_dx = 0;
        for (vector<V_f*>::const_iterator i = fvec.begin(); i != fvec.end();i++)
            if (!cut_tails || (*i)->range_l <= x && x <= (*i)->range_r) {
                y += (*i)->compute (x, &dy_dx);
                (*i)->put_deriv (dy_da);
            }
        for (vector<V_z*>::iterator i = zvec.begin(); i != zvec.end(); i++) {
            (*i)->compute_deriv (x, dy_dx);
            (*i)->put_deriv (dy_da);
        }
        return y;
    }
    else  // numerical derivation
        return value_and_add_numeric_deriv (x, two_side, dy_da);
}

fp Sum::value_and_add_numeric_deriv (fp x, bool both_sides, vector<fp>& dy_da)
{
    // pre: use_param_a_for_value() called
    // this func does _not_ take into consideration Domain::frozen
    // pre: dy_da is zeroed 
    
    assert (nA == size(av_numder));
    const fp small_number = 1e-10; //it only prevents h==0
    fp y = value(x);
    for (int k = 0; k < nA; k++) {
        fp acopy = av_numder[k];
        fp h = max (fabs(acopy), small_number) * numerical_h;
        av_numder[k] -= h;

        //use_param_a_for_value (av_numder);
        // a bit of optimization -- this is insteed of use_param_a_for_value():
        for (vector<V_f*>::iterator i = fvec.begin(); i != fvec.end(); i++)
            (*i)->pre_compute_value_only (av_numder, gvec);
        for (vector<V_z*>::iterator i = zvec.begin(); i != zvec.end(); i++)
            (*i)->pre_compute_value_only (av_numder, gvec);
        //till here 
        
        fp y_aless = value(x);

        if (both_sides) {
            av_numder[k] = acopy + h;
            //use_param_a_for_value (av_numder);
            // the same optimization again
            for (vector<V_f*>::iterator i = fvec.begin(); i != fvec.end(); i++)
                (*i)->pre_compute_value_only (av_numder, gvec);
            for (vector<V_z*>::iterator i = zvec.begin(); i != zvec.end(); i++)
                (*i)->pre_compute_value_only (av_numder, gvec);
            //till here 
            fp y_amore = value(x);
            dy_da[k] = (y_amore - y_aless) / (2 * h);
        }
        else
            dy_da[k] = (y - y_aless) / h;
        av_numder[k] = acopy;
    }
    //use_param_a_for_value (av_numder);
    // the same optimization again
    for (vector<V_f*>::iterator i = fvec.begin(); i != fvec.end(); i++)
        (*i)->pre_compute_value_only (av_numder, gvec);
    for (vector<V_z*>::iterator i = zvec.begin(); i != zvec.end(); i++)
        (*i)->pre_compute_value_only (av_numder, gvec);
    return y;
}

string Sum::general_info() const
{
    ostringstream os; 
    os << "Number of  " << V_fzg::full_type(fType) << ": " << fvec.size()
       <<"\nNumber of  "<< V_fzg::full_type(zType) << ": " << zvec.size()
       <<"\nNumber of  "<< V_fzg::full_type(gType) << ": " << gvec.size()
       <<"\nNumber of A-parameters @: " << nA; 
    return os.str();
}

void Sum::export_as_script (ostream& os) const
{
    os << "### sum exported as script -- begin" << endl;
    os << set_script('s');
    for (int i = 0; i < nA; i++) {  // @
        os << "s.add ~"<< get_a(i) << "  " << adomain[i].str();
        os << "  # @" << i 
            <<  "  (" << refs_to_ag(Pag(0., i)) << " link(s))";
        string refs = descr_refs_to_ag (Pag(0., i));
        if (!refs.empty())
            os << " (used by: " << refs << ")"; 
        os << endl;
    }
    export_fzg_as_script (os, gType);
    export_fzg_as_script (os, fType);
    export_fzg_as_script (os, zType);
    if (count_frozen())
        os << "s.freeze " << frozen_info() << endl;
    os << "### end of exported sum" << endl;
}

void Sum::export_fzg_as_script (ostream& os, One_of_fzg fzg) const
{
    for (int i = 0; i < fzg_size(fzg); i++) {
        const V_fzg* p = get_fzg(fzg, i);
        os << "s.add " << p->short_type() << p->type_info()->type;   
        for (int j = 0; j < p->g_size; j++)
            os << " " <<  p->pag_str(j);
        os << " # " << V_fzg::short_type(fzg) << i 
            << "  " << p->type_info()->name; 
        string refs = descr_refs_to_fzg (fzg, i);
        if (!refs.empty())
            os << " (used by: " << refs << ")"; 
        os << endl;
    }
}


void Sum::export_as_dat (ostream& os, vector<int> &peaks) 
{
    int n = my_data->get_n();
    if (n < 2) {
        warn ("At least two data points should be active when"
                " exporting sum as points");
        return;
    }
    use_param_a_for_value();
    int k = smooth_limit / n + 1;
    for (int i = 0; i < n - 1; i++) 
        for (int j = 0; j < k; j++) {
            fp x = ((k - j) * my_data->get_x(i) + j * my_data->get_x(i+1)) / k;
            fp y = peaks.empty() ? value(x) : funcs_value (peaks, x);
            os << x << "\t" << y << endl; 
        }
    fp x = my_data->get_x (n - 1);
    fp y = peaks.empty() ? value(x) : funcs_value (peaks, x);
    os << x << "\t" << y << endl; 
}

void Sum::export_as_peaks (std::ostream& os, vector<int> &peaks) const
{
    os << "# data file: " << my_data->get_filename() << endl;  
    os << "# Peak Type     Center  Height  Area    FWHM    "
                                        "a0     a1     a2     a3 (...)\n"; 
    if (peaks.empty()) 
        peaks = range_vector(0, fzg_size(fType));
    for (vector<int>::const_iterator i = peaks.begin(); i != peaks.end(); i++){
        V_f* p = fvec[*i];
        os << p->c_name;  //Peak 
        if (!p->c_description.empty()) 
            os << "(" << p->c_description << ")";
        os << "  " << p->type_info()->name; //Type
        os << "  " << p->center() << " " << p->height() << " " << p->area() 
            << " " << p->fwhm() << "  ";
        for (int i = 0; i < p->type_info()->psize; i++) // a0  a1 ...
            os << " " << p->get_pag(i).value(current_a(), gvec);
        os << endl;
    }
}

void Sum::export_as_xfit (std::ostream& os, vector<int> &peaks) const
{
    const char* header = "     File        Peak     Th2        "
                            "Code      Constr      Area        Err     \r\n";
    const char *line_template = "                PV                @       "
                                    "   1.000                  0.0000     \r\n";

    os << header;
    string filename = my_data->get_filename();
    size_t last_slash = filename.find_last_of("/\\");
    if (last_slash != string::npos) filename = filename.substr(last_slash+1);
    if (filename.size() > 16)
        filename = filename.substr(filename.size() - 16, 16); 
    if (peaks.empty()) 
        peaks = range_vector(0, fzg_size(fType));
    for (vector<int>::const_iterator i = peaks.begin(); i != peaks.end(); i++){
        if (*i < 0 || *i >= fzg_size(fType)) continue;
        V_f* p = fvec[*i];
        if (!p->is_peak()) continue;
        string line = line_template;
        if (i == peaks.begin()) line.replace (0, filename.size(), filename);
        string ctr = S(p->center());
        line.replace (23, ctr.size(), ctr);
        string area = S(p->area());
        line.replace (56, area.size(), area);
        os << line;
    }
}

void Sum::export_to_file (string filename, bool append, char filetype, 
                          vector<int> &peaks) 
{
    ofstream os(filename.c_str(), ios::out | (append ? ios::app : ios::trunc));
    if (!os) {
        warn ("Can't open file: " + filename);
        return;
    }
    int dot = 0;//for filetype detection
    switch (filetype) {
        case 'f': 
            {
            if (peaks.empty())  
                os << sum_full_formula();
            else
                os << sum_of_peaks_formula (peaks);
            os << endl;
            break;
            }
        case 'd': 
            export_as_dat (os, peaks);
            break;
        case 'p': 
            export_as_peaks (os, peaks);
            break;
        case 'x': 
            export_as_xfit (os, peaks);
            break;
        case 's':
            if (!peaks.empty())
                warn("Can't export as script only chosen peaks. "
                        "Exporting whole sum.");
            export_as_script (os);
            break;
        case 0:
            //guessing filetype
            dot = filename.rfind('.');
            if (dot > 0 && dot < static_cast<int>(filename.length()) - 1) {
                string exten(filename.begin() + dot, filename.end());
                if (exten == ".fit")
                    return export_to_file (filename, append, 's', peaks);
                if (exten == ".dat")
                    return export_to_file (filename, append, 'd', peaks);
                if (exten == ".peaks")
                    return export_to_file (filename, append, 'p', peaks);
                if (exten == ".txt" && filename.size() >= 8 &&
                     filename.substr(filename.size()-8).compare("xfit.txt")==0)
                    return export_to_file (filename, append, 'x', peaks);
            }
            mesg ("exporting as formula");
            return export_to_file (filename, append, 'f', peaks);
        default:
            warn ("Unknown filetype letter: " + S(filetype));
    }
}

string Sum::print_sum_value (fp x, bool with_deriv)
{
    if (nA == 0)
        return "empty";
    return print_fzg_value (fType, -2, x, with_deriv);
}

string Sum::print_fzg_value (One_of_fzg fzg, int n, fp x, bool with_deriv)
{
    //if (fzg == fType && n == -2) : special case -> value of whole sum
    bool whole_sum = (fzg == fType && n == -2);
    if (n == -1) { // print value for n = 0, 1, ...
        string s;
        for (int i = 0; i < fzg_size(fzg); i++)
            s += (i != 0 ? "\n" : "") + print_fzg_value (fzg, i, x);
        return s;
    }
    if (!whole_sum && (n < 0 || n >= fzg_size(fzg)))
        return V_fzg::full_type(fzg) + S(n) + " not found";
    if (numerical_h <= 0) {
        warn ("First set num-differentiation-rel-change greater than 0.");
        return "";
    }

    const fp small_number = 1e-10; //it only prevents from h==0
    vector<fp> df_s(nA), df_n1(nA), df_n2(nA); 
    fp y = 0, df_dx = 0;

    if (fzg != gType) use_param_a_for_value ();
    else av_numder = current_a();
    //computing y and df_s -- value and (semi-)symbolic derivatives
    if (whole_sum) {
        y = value_and_put_deriv (x, df_s);
    }
    else if (fzg == fType) {
        fp z = zero_shift(x);
        y = fvec[n]->compute (x + z, &df_dx);
        fvec[n]->put_deriv (df_s);
        //also parameters in zero-shifts are influenced
        for (vector<V_z*>::iterator i = zvec.begin(); i != zvec.end(); i++){
            (*i)->compute_deriv (x + z, df_dx);
            (*i)->put_deriv (df_s);
        }
    }
    else if (fzg == zType) {
        y = zvec[n]->z_value(x);
        zvec[n]->compute_deriv (x, 1.);
        zvec[n]->put_deriv (df_s);
    }
    else if (fzg == gType) {
        y = gvec[n]->g_value(current_a(), gvec);
        vector<int_fp> v = gvec[n]->get_derivatives (current_a(), gvec);
        for (vector<int_fp>::const_iterator i = v.begin(); i != v.end(); i++) 
            df_s[i->nr] = i->der;
    }
    else assert(0);

    if (with_deriv) {
        //computing numeric derivatives (two versions -- one and two sides) 
        for (int k = 0; k < nA; k++) {
            fp acopy = av_numder[k];
            fp h = max (fabs(acopy), small_number) * numerical_h;

            av_numder[k] -= h;
            if (fzg != gType) use_param_a_for_value (av_numder);
            fp y_aless;
            if (whole_sum) y_aless = value(x);
            else if (fzg == fType) y_aless = f_value(x, n);
            else if (fzg == zType) y_aless = zvec[n]->z_value(x);
            else /*(fzg == gType)*/ y_aless = gvec[n]->g_value(av_numder, gvec);

            av_numder[k] = acopy + h;
            if (fzg != gType) use_param_a_for_value (av_numder);
            fp y_amore;
            if (whole_sum) y_amore = value(x);
            else if (fzg == fType) y_amore = f_value(x, n);
            else if (fzg == zType) y_amore = zvec[n]->z_value(x);
            else /*(fzg == gType)*/ y_amore = gvec[n]->g_value(av_numder, gvec);

            df_n2[k] = (y_amore - y_aless) / (2 * h);
            df_n1[k] = (y - y_aless) / h;
            av_numder[k] = acopy;
        }
    }
    if (fzg != gType) use_param_a_for_value (av_numder);


    string s = whole_sum ? S("sum") : V_fzg::short_type(fzg) + S(n);
    if (fzg != gType) s += " (" + S(x) + ")";
    s += " = " + S(y);
    if (!whole_sum && fzg == fType && cut_tails)
            s += "   domain [" + S(fvec[n]->range_l) + " , " 
                + S(fvec[n]->range_r) + "] ; zero-shift: " + S(zero_shift(x)); 
    if (with_deriv)
        s += format_der_output(df_s, df_n1, df_n2);
    return s;
}

string Sum::format_der_output(const vector<fp>& df_s, const vector<fp>& df_n1,
                              const vector<fp>& df_n2)
{
    string s;
    s += " ;   non-zero derivatives:\n";
    int count = 0;
    for (int i = 0; i < nA; i++) { 
        const fp epsilon = 1e-12;
        if (fabs(df_s[i]) > epsilon || fabs(df_n1[i]) > epsilon 
                || fabs(df_n2[i]) > epsilon) {
            s += "                   @" + S(i) + "  ->  " + S(df_s[i]) 
                + " ~ " + S(df_n2[i]) + " ~ " + S(df_n1[i]) + "\n";
            if (fabs(df_n2[i] - df_s[i]) > max (10 * fabs(df_n1[i] - df_n2[i]),
                                                fabs(df_s[i]) * numerical_h))
                s += "       ***************^ SUSPECTED ^***************\n";
            count++;
        }
    }
    if (count == 0)
        s += "none";
    return s;
}

int Sum::add_fzg (One_of_fzg fzg, char type, const vector<Pag> &p, 
                  const string &desc)
{
    if (fzg == zType) {//only one zero-shift of one type is allowed
        int no = find_zshift (type);
        if (no != -1) {
            warn (zvec[no]->full_type() + " already set. Canceled.");
            return -1;
        }
    }
    V_fzg *v = V_fzg::add (fzg, this, type, p);
    if (!v)
        return -1;
    if      (fzg == fType) fvec.push_back (static_cast<V_f*>(v));
    else if (fzg == zType) zvec.push_back (static_cast<V_z*>(v));
    else if (fzg == gType) gvec.push_back (static_cast<V_g*>(v));
    int n = fzg_size(fzg) - 1;
    if (fzg == fType || fzg == zType) 
        s_was_changed = true;
    verbose ("Added " + V_fzg::full_type(fzg) + S(n));
    v->c_name = V_fzg::short_type(fzg) + S(n);
    v->c_description = desc;
    return n;
}

Pag Sum::add_g (char type, const vector<Pag> &p, const string &desc)
{
    int n = add_fzg (gType, type, p, desc);
    if (n == -1) 
        return Pag();
    else
        return Pag ((V_g*)0, n); 
}

Pag Sum::add_a (fp value, Domain d) 
{
    assert (size(current_a()) == nA && size(adomain)==nA && size(afrozen)==nA);
    vector<fp> a_new = current_a();
    a_new.push_back (value);
    adomain.push_back (d);
    afrozen.push_back (false);
    int n = nA;
    nA++;
    write_avec (a_new, "add @" + S(n));
    assert (history.size() == 1);
    return Pag (0., n); 
}

string Sum::info_fzg_parameters (One_of_fzg fzg, int n, bool only_val) const
{
    // return something like:  
    // [@19=861.501 $53=43.1904 $62=0.193862 $64=1.09655] 
    assert (n >= 0 && n < fzg_size(fzg));
    const V_fzg *p = get_fzg (fzg, n);
    string s = "[";
    for (int i = 0; i < p->g_size; i++) {
        if (i != 0) s += " ";
        Pag pag = p->get_pag(i);
        fp value = pag.value(current_a(), gvec);
        if (only_val) 
            s += S(value);
        else {
            s += pag.str();
            if (pag.is_a() || pag.is_g())
                s += "=" + S(value);
        }
    }
    return s + "]";
}

string Sum::info_fzg (One_of_fzg fzg, int n) const
{
    if (n == -1){ // info about all 
        string s;
        for (int i = 0; i < fzg_size(fzg); i++)
            s += info_fzg (fzg, i) + (i != fzg_size(fzg) - 1 ? "\n" : "");
        return s;
    }
    if (n < 0 || n >= fzg_size(fzg)) {
        return V_fzg::full_type(fzg) + S(n) + " not found."; 
    }
    const V_fzg *p = get_fzg (fzg, n);
    string s = V_fzg::short_type(fzg) + S(n) + ": " + p->type_info()->name
        + " " + info_fzg_parameters(fzg, n, false);

    string refs = descr_refs_to_fzg (fzg, n);
    if (!refs.empty())
        s += " (used by: " + refs + ")"; 
    return s;
}

int Sum::fzg_size (One_of_fzg fzg) const
{
    switch (fzg) {
        case fType: return fvec.size();
        case zType: return zvec.size();
        case gType: return gvec.size();
        default: assert(0); return 0;
    }
}

int Sum::multi_rm_fzg (One_of_fzg fzg, vector<int> nn, bool silent) 
{
    int counter = 0;
    sort (nn.begin(), nn.end());
    unique (nn.begin(), nn.end());
    reverse (nn.begin(), nn.end());
    for (vector<int>::const_iterator i = nn.begin(); i != nn.end(); i++) {
        counter += rm_fzg (fzg, *i, true);
    }
    if (!silent)
        mesg (S(counter) + " " + V_fzg::full_type(fzg, false) + "s removed.");
    return counter;
}

int Sum::rm_fzg (One_of_fzg fzg, int n, bool silent) 
{
    if (n == -1)
        return multi_rm_fzg (fzg, range_vector (0, fzg_size(fzg)), silent);
    if (n < 0 || n >= fzg_size(fzg)) {
        warn (V_fzg::full_type(fzg) + S(n) + " not found. Canceled.");
        return 0;
    }
    //checking references to item, that users want to delete
    int links = 0;
    if (fzg == fType)
        links = refs_to_f (n);
    else if (fzg == gType) 
        links = refs_to_ag (Pag((V_g*)0, n));
    if (links > 0) {
        if (!silent)
            warn (V_fzg::full_type(fzg) + S(n) + " NOT removed. " 
                    + S(links) + " links. First remove: " 
                    + descr_refs_to_fzg (fzg, n));
        return 0;
    }

    //no references -- deleting item and changing numbers to fill the gap
    const V_fzg *ptr = get_fzg (fzg, n);
    if (fzg == fType) {
        fvec.erase (fvec.begin() + n);
        s_was_changed = true;
        for (set<FuncContainer*>::iterator i = f_usage.begin(); 
                i != f_usage.end(); i++)
            (*i)->synch_after_rm_f (n);
    }
    else if (fzg == zType) {
        zvec.erase (zvec.begin() + n);
        s_was_changed = true;
    }
    else if (fzg == gType) {
        gvec.erase (gvec.begin() + n);
        synch_after_rm_ag (Pag((V_g*)0, n));
    }
    for (int i = n; i < fzg_size(fzg); ++i) {
        V_fzg *t = get_fzg_m (fzg, i);
        t->c_name = V_fzg::short_type(fzg) + S(i);
    }

    if (!silent) {
        mesg (V_fzg::full_type(fzg) + S(n) + " removed.");
        if (n < fzg_size(fzg) - 1 + 1) 
            verbose ("Some " + V_fzg::full_type(fzg, false) 
                     + " have changed numbers");
    }
    delete ptr;
    return 1;
}

int Sum::rm_all()
{
    int b_f = fzg_size(fType), b_z = fzg_size(zType), b_g = fzg_size(gType),
        b_a = count_a();
    rm_fzg (fType, -1, true); rm_fzg (zType, -1, true); rm_fzg(gType, -1, true);
    rm_a(-1, true);
    int r_f = b_f - fzg_size(fType), r_z = b_z - fzg_size(zType),
        r_g = b_g - fzg_size(gType), r_a = b_a - count_a();
    mesg (S(r_f) + " " + V_fzg::full_type(fType, false) + "s, " 
          + S(r_z) + " " + V_fzg::full_type(zType, false) + "s, " 
          + S(r_g) + " " + V_fzg::full_type(gType, false) + "s and "
          + S(r_a) + " a-parameters removed.");
    return r_f + r_z + r_g + r_a;
}

void Sum::change_f (int f, char type) //experimental
{
    if (f < 0 || size_t(f) >= fvec.size()) {
        warn ("Wrong function number: ^" + S(f));
        return;
    }
    V_f* nf = V_f::copy_factory (this, type, fvec[f]);  
    if (!nf) {
        warn ("Wrong f-function type or another error. Canceled.");
        return;
    }
    V_f* old = fvec[f];
    fvec[f] = nf;
    delete old;
}

void Sum::change_in_f (int f, vector<int> arg, vector<Pag> np) //experimental
{
    if (f < 0 || size_t(f) >= fvec.size()) {
        warn ("Wrong function number: ^" + S(f));
        return;
    }
    if (PagContainer(0, np).is_ok(this) == false)
        return;
    if (arg.size() != np.size()) {
        warn ("Too many or too few new parameters");
        return;
    }
    vector<Pag> old;
    for (int i = 0; i < size(arg); i++) {
        if (arg[i] >= fvec[i]->g_size) {
            warn ("No argument " + S(arg[i]) + " in ^" + S(f));
            return;
        }
        old.push_back (fvec[f]->change_arg(arg[i], np[i]));
    }
    PagContainer (this, old); //to call recursive_rm() in destructor
}

/*
vector<Pag> Sum::pags_of_f(int n) const 
{ 
    assert (0 <= n && n < size(fvec));
    return fvec[n]->copy_of_pags(); 
}

vector<Pag> Sum::pags_of_g(int n) const 
{ 
    assert (0 <= n && n < size(gvec));
    return gvec[n]->copy_of_pags(); 
}
*/

void Sum::write_avec (const vector<fp>& a, string comment, bool no_move) 
{ 
    assert (size(a) == nA && size(adomain) == nA && size(afrozen) == nA);
     //3 special cases:
    if (a == current_a()) //1. no change
        return;
    s_was_changed = true;
    if (a.size() != current_a().size()) { //2. added/removed a (change in size)
        history.clear();
        history.push_back (History_item (a, comment));
        hp = 0;
        return;
    }
    if (comment.empty() && !history[hp].saved) { //3. change in place
        history[hp].a = a; 
        return;
    }
    if (!no_move && hp < size(history) - 1) 
        for (vector<History_item>::iterator i = history.end() - 1; 
                                                i > history.begin() + hp; i--)
            if (!i->saved)
                history.erase (i);
    history.push_back (History_item (a, comment));
    if (!no_move) {
        hp = history.size() - 1;
        mesg ("A-parameters changed (by: " + comment + ")");
    }
}

string Sum::print_history () const
{
    string s = "History contains " + S(history.size()) + " entries. "
        "Current is #" + S(hp + 1);
    for (int i = 0; i < size(history); i++) {
        s += (hp == i ? "\n->" : "\n  ");
        s += (history[i].saved ? " *" : " #") + S(i + 1) 
            + "  changed by: " + history[i].comment;
        //if (...)
        //   s += "   WSSR = " + S(compute_wssr (history[i].a));
        // it would be necessary to include v_fit.h to do this 
        // convenient comparision of WSSRs is done in wxfityk version 
    }
    return s;
}

string Sum::history_diff (vector<int> hist_items) const
{
    //hist_items empty -> info about all items
    if (hist_items.empty())
        for (int i = 0; i < size(history); i++)
            hist_items.push_back(i+1);
    //check user input
    if (*max_element(hist_items.begin(), hist_items.end()) > size(history)
            || *min_element (hist_items.begin(), hist_items.end()) <= 0)
        return "Wrong numbers of history entries in query";
    //draw "table"
    string s = "   ";
    for (vector<int>::iterator j = hist_items.begin(); j!=hist_items.end(); j++)
        s += "   #" + S(*j) + "    "; 
    for (int i = 0; i < nA; i++) {
        s += "\n@" + S(i) + ": ";
        for (vector<int>::iterator j = hist_items.begin(); 
                                                j != hist_items.end(); j++)
            s += S(history[(*j) - 1].a[i]) + " | ";
    }
    return s;
}

void Sum::move_in_history (int k, bool relative)
{
    if (relative)
        hp += k;
    else
        hp = k - 1;
    if (hp < 0) {
        warn ("Beginning of history reached.");
        hp = 0;
    }
    else if (hp >= size(history)) {
        warn ("End of history reached.");
        hp = history.size() - 1;
    }
    s_was_changed = true;
    verbose ("Current history position is #" + S(hp + 1)
            + " of " + S(history.size()));
}

void Sum::toggle_history_item_saved(int k)
{
    if (k - 1 < 0) k = hp + 1;
    else if (k  - 1 >= size(history)) {
        warn ("There is no item #" + S(k) + " in history");
        return;  
    }  
    history[k - 1].saved = ! history[k - 1].saved;  
}

bool Sum::rm_a_core (int nr)
{
    assert (nr >= 0 && nr < nA);
    if (refs_to_ag (Pag(0., nr)) != 0) 
        return false;
    vector<fp> a_new = current_a();
    a_new.erase (a_new.begin() + nr);
    adomain.erase (adomain.begin() + nr);
    afrozen.erase (afrozen.begin() + nr);
    nA--;
    synch_after_rm_ag (Pag(0., nr));
    write_avec (a_new, "rm @" + S(nr));
    assert (history.size() == 1);
    return true;
}

int Sum::rm_a (int n, bool silent)
{
    if (n == -1) {
        int counter = 0;
        for (int i = nA - 1; i >= 0; i--)
            if (rm_a_core(i))
                counter ++;
        if (!silent) mesg (S(counter) + " A-parameters removed.");
        return counter;
    }

    if (n < 0 || n >= nA) {
        warn ("No such A: @" + S(n));
        return 0;
    }
    else if (rm_a_core (n)) {
        if (!silent) mesg ("A-parameter @" + S(n) + " removed.");
        return 1;
    }
    else {
        warn ("A-parameter @" + S(n) + " NOT removed. " 
                + S(refs_to_ag (Pag(0., n))) + " links. First remove: " 
                + descr_refs_to_ag (Pag(0., n)));
        return 0;
    }
}

string Sum::info_a (int nr) const
{
    if (nr == -1) { // info about all A's
        string s;
        for (int i = 0; i < nA; i++)
            s += info_a(i) + (i != nA - 1 ? "\n" : "");
        return s;
    }
    else if (nr < 0 || nr >= nA ) 
        return "No such A: @"  + S(nr);
    else {
        ostringstream os;
        os << "@" << nr << ": " << get_a(nr); 
        os << (afrozen[nr] ? " F " : "  ") << adomain[nr].str();
        if (refs_to_ag (Pag(0., nr)))
            os << " (used by: " << descr_refs_to_ag (Pag(0., nr)) + ")"; 
        return os.str();
    }
}

fp Sum::change_a (int nr, fp value, char c /* ='=' */, bool add_to_history) 
{
    if (nr == -1) {
        for (int i = 0; i < nA; i++)
            change_a (i, value, c, i == nA - 1 ? add_to_history : false);
        mesg (S(nA) + " A-parameters changed.");
        return 0.;
    }
    if (nr < 0 || nr >= nA) {
        warn ("No such A: @" + S(nr));
        return 0.;
    }
    fp before = get_a (nr); 
    vector<fp> a = current_a();
    switch (c) {
        case '=': a[nr] = value; break;
        case '+': a[nr] += value; break;
        case '-': a[nr] -= value; break;
        case '*': a[nr] *= value; break;
        case '/':
            if (value)
                a[nr] /= value;
            else 
                warn ("Trying to divide by zero in sum::change_a");
            break;
        case '%': if (value != 100.) a[nr] *= (value/100); break;
        default: assert(0); break;
    }
    if (before != a[nr])
        s_was_changed = true;
    if (add_to_history)
        write_avec (a, "manual");
    else
        write_avec (a, "");
    return before;
}

void Sum::change_domain (int nr, Domain d) 
{ 
    if (nr < 0 || nr >= size(adomain)) {
        warn ("No such A: @" + S(nr));
        return;
    }
    adomain[nr] = d; 
}

fp Sum::variation_of_a (int n, fp variat) const
{
    assert (0 <= n && n < nA);
    const Domain& dom = get_domain (n);
    fp ctr = dom.is_ctr_set() ? dom.Ctr() : get_a(n);
    fp sgm = dom.is_set() ? dom.Sigma() : def_rel_domain_width * ctr;
    return ctr + sgm * variat;
}

string Sum::sum_full_formula(const vector<fp>& localA) const
{
    vector<int> pks (fvec.size());
    for (unsigned int i = 0; i < pks.size(); i++)
        pks[i] = i;
    return sum_of_peaks_formula (pks, localA);
}

string Sum::sum_of_peaks_formula (vector<int>& peaks, const vector<fp>& localA)
                                                                          const
{
    const vector<fp>& workingA = !localA.empty() ? localA : current_a();
    string x = "x";
    //TODO () around x+a
    for (vector<V_z*>::const_iterator i = zvec.begin(); i != zvec.end(); i++)
        x += "+" + (*i)->formula(workingA, gvec);
    V_f::xstr = x;
    string yf = "   ";
    if (!peaks.empty()) 
        for (unsigned int i = 0; i < peaks.size(); i++) {
            yf += fvec[peaks[i]]->formula (workingA, gvec);
            if (i != peaks.size() - 1)
                yf += "\n + ";
        }
    else
        yf = "0";
    return yf;
}

int Sum::refs_to_f (int n) const
{
    int refs = 0;
    for (set<FuncContainer*>::const_iterator i = f_usage.begin(); 
                                                    i != f_usage.end(); i++)
        refs += (*i)->count_refs (n);
    return refs;
}

int Sum::refs_to_ag (Pag p) const
{
    int refs = 0;
    for (set<PagContainer*>::const_iterator i = vp_usage.begin(); 
                                                    i != vp_usage.end(); i++)
        refs += (*i)->count_refs (p);
    return refs;
}

string Sum::descr_refs_to_fzg (One_of_fzg fzg, int n) const
{
    if      (fzg == fType) return descr_refs_to_f(n); 
    else if (fzg == gType) return descr_refs_to_ag(Pag((V_g*)0, n));
    else return "";
}

string Sum::descr_refs_to_f (int n) const
{
    string s;
    for (set<FuncContainer*>::const_iterator i = f_usage.begin(); 
                                                    i != f_usage.end(); i++)
        if ((*i)->count_refs (n))
            s += (s.empty() ? "" : ", ") + (*i)->c_name;
    return s;
}

string Sum::descr_refs_to_ag (Pag p) const
{
    string s;
    for (set<PagContainer*>::const_iterator i = vp_usage.begin(); 
                                                    i != vp_usage.end(); i++)
        if ((*i)->count_refs (p))
            s += (s.empty() ? "" : ", ") + (*i)->c_name;
    return s;
}

void Sum::synch_after_rm_f (int n)
{
    for (set<FuncContainer*>::iterator i=f_usage.begin(); i!=f_usage.end(); i++)
        (*i)->synch_after_rm_f (n);
    for (int i = n; i < fzg_size(fType); ++i) {
        V_fzg *t = fvec[i];
        t->c_name = V_fzg::short_type(fType) + S(i);
    }
}

void Sum::synch_after_rm_ag (Pag p)
{
    for (set<PagContainer*>::iterator i=vp_usage.begin(); i!=vp_usage.end();i++)
        (*i)->synch_after_rm (p);
    if (p.is_g())
        for (int i = p.g(); i < fzg_size(gType); ++i) {
            V_fzg *t = gvec[i];
            t->c_name = V_fzg::short_type(gType) + S(i);
        }
}

fp Sum::zero_shift (fp x) const
{
    // pre: use_param_a_for_value() called
    fp z = 0;
    for (vector<V_z*>::const_iterator i = zvec.begin(); i != zvec.end(); i++)
        z += (*i)->z_value(x);
    return z;
}

int Sum::find_zshift (char type) const
{
    for (vector<V_z*>::const_iterator i = zvec.begin(); i != zvec.end(); i++)
        if (type == (*i)->type) 
            return i - zvec.begin();
    //if not found:
    return -1;
}

int Sum::freeze(int nr, bool frozen)
{
    if (nr == -1) {
        fill (afrozen.begin(), afrozen.end(), frozen);
        return 0;
    }
    if (nr >= nA || nr < 0) {
        warn("There is no @" + S(nr) + ".");
        return -1;
    }
    if (frozen == afrozen[nr]) {
        warn ("@" + S(nr) + " is already " + S(frozen ? "" : "not ") 
               + "frozen.");
        return -1;
    }
    else {
        afrozen[nr] = frozen;
        verbose ("@" + S(nr) + "set " + S(frozen ? "" : "not ") + "frozen");
        return 0;
    }
}

int Sum::count_frozen() const
{
    return count (afrozen.begin(), afrozen.end(), true);
}

string Sum::frozen_info () const
{
    string s;
    for (int i = 0; i < size(afrozen); i++)
        if (afrozen[i])
            s += (s.empty() ? "@" : ", @") + S(i);
    return s;
}

void Sum::register_fc (FuncContainer* cnt, bool flag)
{
    if (flag == true) {
        assert (f_usage.count(cnt) == 0);
        f_usage.insert (cnt);
    }
    else {//flag == false
        assert (f_usage.count(cnt) == 1);
        f_usage.erase (cnt);
    }
}

void Sum::register_pc (PagContainer* vp, bool flag)
{
    if (flag == true) {
        assert (vp_usage.count(vp) == 0);
        vp_usage.insert (vp);
    }
    else {//flag == false
        assert (vp_usage.count(vp) == 1);
        vp_usage.erase (vp);
    }
}

const V_fzg *Sum::get_fzg (One_of_fzg fzg, int n) const
{
    assert (n >= 0 && n < fzg_size(fzg));
    switch (fzg) {
        case fType: return fvec[n];
        case zType: return zvec[n];
        case gType: return gvec[n];
        default: return 0;
    }
}


//==================
Sum *my_sum;

