// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "fit.h"
#include <algorithm>
#include <sstream>
#include <math.h>
#include "logic.h"
#include "sum.h"
#include "data.h"
#include "ui.h"
#include "numfuncs.h"
#include "settings.h"
#include "LMfit.h"
#include "GAfit.h"
#include "NMfit.h"

using namespace std;

Fit::Fit(string m)  
    : name(m), evaluations(0), iter_nr (0), na(0)
{
    Distrib_enum ['u'] = "uniform";
    Distrib_enum ['g'] = "gauss";
    Distrib_enum ['l'] = "lorentz";
    Distrib_enum ['b'] = "bound";
}

string Fit::getInfo(vector<DataWithSum*> const& dsds)
{
    vector<fp> const &pp = AL->get_parameters();
    update_parameters(dsds);
    //n_m = number of points - degrees of freedom (parameters)
    int n_m = 0;
    for (vector<DataWithSum*>::const_iterator i = dsds.begin(); 
                                                    i != dsds.end(); ++i) 
        n_m += (*i)->get_data()->get_n(); 
    n_m -= count(par_usage.begin(), par_usage.end(), true);
    return "Current WSSR = " + S(compute_wssr(pp, dsds)) 
                + " (expected: " + S(n_m) + "); SSR = " 
                + S(compute_wssr(pp, dsds, false))
		+ "; R-squared = " 
		+ S(compute_r_squared(pp, dsds)) ;
}

vector<fp> Fit::get_covariance_matrix(vector<DataWithSum*> const& dsds)
{
    vector<fp> const &pp = AL->get_parameters();
    update_parameters(dsds);

    vector<fp> alpha(na*na), beta(na);
    compute_derivatives(pp, dsds, alpha, beta);
    for (int i = 0; i < na; ++i) //to avoid singular matrix, we put fake values
        if (!par_usage[i]) {     // corresponding unused parameters
            alpha[i*na + i] = 1.;
        }
    //sometimes some parameters are unused, although formally are "used". 
    //E.g. SplitGaussian with center < min(active x) will have hwhm1 unused
    //Anyway, if i'th column/row in alpha are only zeros, we must
    //do something about it -- symmetric error is undefined 
    vector<int> undefined;
    for (int i = 0; i < na; ++i) {
        bool has_nonzero = false;
        for (int j = 0; j < na; j++)                     
            if (alpha[na*i+j] != 0.) {
                has_nonzero = true;
                break;
            }
        if (!has_nonzero) {
            undefined.push_back(i);
            alpha[i*na + i] = 1.;
        }
    }

    reverse_matrix(alpha, na);

    for (vector<int>::const_iterator i = undefined.begin(); 
            i != undefined.end(); ++i)
        alpha[(*i)*na + (*i)] = 0.; 

    for (vector<fp>::iterator i = alpha.begin(); i != alpha.end(); i++)
        (*i) *= 2; //FIXME: is it right? (S.Brandt, Analiza danych (10.17.4))
    return alpha;
}

vector<fp> Fit::get_symmetric_errors(vector<DataWithSum*> const& dsds)
{
    vector<fp> alpha = get_covariance_matrix(dsds);
    vector<fp> errors(na);
    for (int i = 0; i < na; ++i)
        errors[i] = sqrt(alpha[i*na + i]); 
    return errors;
}

string Fit::getErrorInfo(vector<DataWithSum*> const& dsds, bool matrix)
{
    vector<fp> alpha = get_covariance_matrix(dsds);
    vector<fp> const &pp = AL->get_parameters();
    string s;
    s = "Symmetric errors: ";
    for (int i = 0; i < na; i++) {
        if (par_usage[i]) {
            fp err = sqrt(alpha[i*na + i]);
            s += "\n" + AL->find_variable_handling_param(i)->xname 
                + " = " + S(pp[i]) 
                + " +- " + (err == 0. ? string("??") : S(err));
        }
    }
    if (matrix) {
        s += "\nCovariance matrix\n    ";
        for (int i = 0; i < na; ++i)
            if (par_usage[i])
                s += "\t" + AL->find_variable_handling_param(i)->xname;
        for (int i = 0; i < na; ++i) {
            if (par_usage[i]) {
                s += "\n" + AL->find_variable_handling_param(i)->xname;
                for (int j = 0; j < na; ++j) {
                    if (par_usage[j])
                        s += "\t" + S(alpha[na*i + j]);
                }
            }
        }
    }
    return s;
}

fp Fit::compute_wssr(vector<fp> const &A, vector<DataWithSum*> const& dsds,
                     bool weigthed)
{
    evaluations++;
    fp wssr = 0;
    AL->use_external_parameters(A);
    for (vector<DataWithSum*>::const_iterator i = dsds.begin(); 
                                                    i != dsds.end(); ++i) {
        wssr += compute_wssr_for_data(*i, weigthed);
    }
    return wssr;
}

fp Fit::compute_wssr_for_data(DataWithSum const* ds, bool weigthed)
{
    Data const* data = ds->get_data();
    int n = data->get_n();
    vector<fp> xx(n);
    for (int j = 0; j < n; j++) 
        xx[j] = data->get_x(j);
    vector<fp> yy(n, 0.);
    ds->get_sum()->calculate_sum_value(xx, yy);
    fp wssr = 0;
    for (int j = 0; j < n; j++) {
        fp dy = data->get_y(j) - yy[j];
        if (weigthed)
            dy /= data->get_sigma(j);
        wssr += dy * dy;
    }  
    return wssr;
}

fp Fit::compute_r_squared(vector<fp> const &A, vector<DataWithSum*> const& dsds)
{
    evaluations++;
    fp r_squared = 0;
    AL->use_external_parameters(A);
    for (vector<DataWithSum*>::const_iterator i = dsds.begin(); 
                                                    i != dsds.end(); ++i) {
        r_squared += compute_r_squared_for_data(*i);
    }
    return r_squared ;
}

fp Fit::compute_r_squared_for_data(DataWithSum const* ds)
{
    Data const* data = ds->get_data();
    int n = data->get_n();
    vector<fp> xx(n);
    for (int j = 0; j < n; j++) 
        xx[j] = data->get_x(j);
    vector<fp> yy(n, 0.);
    ds->get_sum()->calculate_sum_value(xx, yy);
    fp mean = 0;
    fp ssr_curve = 0 ; // Sum of squares of distances between fitted curve and data
    fp ssr_mean = 0 ;  // Sum of squares of distances between mean and data
    for (int j = 0; j < n; j++) {
        mean += data->get_y(j) ;
        fp dy = data->get_y(j) - yy[j];	
	ssr_curve += dy * dy ;
    }
    mean = mean / (fp) n ;	// Mean computed here.

    for (int j = 0 ; j < n ; j++) {
	fp dy = data->get_y(j) - mean ;
	ssr_mean += dy * dy ;
	}

    return ( 1 - (ssr_curve/ssr_mean) ); // R^2 as defined.
}

//results in alpha and beta 
void Fit::compute_derivatives(vector<fp> const &A, 
                              vector<DataWithSum*> const& dsds,
                              vector<fp>& alpha, vector<fp>& beta)
{
    assert (size(A) == na && size(alpha) == na * na && size(beta) == na);
    fill(alpha.begin(), alpha.end(), 0.0);
    fill(beta.begin(), beta.end(), 0.0);

    AL->use_external_parameters(A);
    for (vector<DataWithSum*>::const_iterator i = dsds.begin(); 
                                                    i != dsds.end(); ++i) {
        compute_derivatives_for(*i, alpha, beta);
    }
    // filling second half of alpha[] 
    for (int j = 1; j < na; j++)
        for (int k = 0; k < j; k++)
            alpha[na * k + j] = alpha[na * j + k]; 
}

//results in alpha and beta 
//it computes only half of alpha matrix
void Fit::compute_derivatives_for(DataWithSum const* ds, 
                                  vector<fp>& alpha, vector<fp>& beta)
{
    Data const* data = ds->get_data();
    int n = data->get_n();
    vector<fp> xx(n);
    for (int j = 0; j < n; j++) 
        xx[j] = data->get_x(j);
    vector<fp> yy(n, 0.);
    const int dyn = na+1;
    vector<fp> dy_da(n*dyn, 0.);
    ds->get_sum()->calculate_sum_value_deriv(xx, yy, dy_da);
    for (int i = 0; i < n; i++) {
        fp inv_sig = 1.0 / data->get_sigma(i);
        fp dy_sig = (data->get_y(i) - yy[i]) * inv_sig;
        vector<fp>::iterator t = dy_da.begin() + i*dyn;
        for (vector<fp>::iterator j = t; j != t+na; j++) 
            *j *= inv_sig;
        for (int j = 0; j < na; j++) {
            for (int k = 0; k <= j; k++)    //half of alpha[]
                alpha[na * j + k] += *(t+j) * *(t+k);
            beta[j] += dy_sig * *(t+j); 
        }
    }   
}

string Fit::print_matrix (const vector<fp>& vec, int m, int n, char *mname)
    //m rows, n columns
{ 
    assert (size(vec) == m * n);
    if (m < 1 || n < 1)
        warn ("In `print_matrix': It is not a matrix.");
    ostringstream h;
    h << mname << "={ ";
    if (m == 1) { // vector 
        for (int i = 0; i < n; i++)
            h << vec[i] << (i < n - 1 ? ", " : " }") ;
    }
    else { //matrix 
        std::string blanks (strlen (mname) + 1, ' ');
        for (int j = 0; j < m; j++){
            if (j > 0)
                h << blanks << "  ";
            for (int i = 0; i < n; i++) 
                h << vec[j * n + i] << ", ";
            h << endl;
        }
        h << blanks << "}";
    }
    return h.str();
}

bool Fit::post_fit (const std::vector<fp>& aa, fp chi2)
{
    bool better = (chi2 < wssr_before);
    string comment = name + (better ? "" : " (worse)");
    AL->put_new_parameters(aa, name, better);
    if (better) {
        info ("Better fit found (WSSR = " + S(chi2) + ", was " + S(wssr_before)
                + ", " + S((chi2 - wssr_before) / wssr_before * 100) + "%).");
        return true;
    }
    else {
        if (chi2 > wssr_before) {
            info ("Better fit NOT found (WSSR = " + S(chi2)
                    + ", was " + S(wssr_before) + ").\nParameters NOT changed");
        }
        iteration_plot(a_orig); //reverting to old plot
        return false;
    }
}

fp Fit::draw_a_from_distribution (int nr, char distribution, fp mult)
{
    assert (nr >= 0 && nr < na);
    fp dv = 0;
    switch (distribution) {
        case 'g':
            dv = rand_gauss();
            break;
        case 'l':
            dv = rand_cauchy();
            break;
        case 'b':
            dv = rand_bool() ? -1 : 1;
            break;
        default: // 'u' - uniform
            dv = rand_1_1();
            break;
    }
    return AL->variation_of_a(nr, dv * mult);
}

/// initialize and run fitting procedure for not more than max_iter iterations
void Fit::fit(int max_iter, vector<DataWithSum*> const& dsds)
{
    update_parameters(dsds);
    datsums = dsds;
    a_orig = AL->get_parameters();
    iter_nr = 0;
    evaluations = 0;
    user_interrupt = false;
    init(); //method specific init
    max_iterations = max_iter;
    autoiter();
}

/// run fitting procedure (without initialization) 
void Fit::continue_fit(int max_iter)
{
    for (vector<DataWithSum*>::const_iterator i = datsums.begin(); 
                                                      i != datsums.end(); ++i) 
        if (!AL->has_ds(*i))
            throw ExecuteError(name + " method should be initialized first.");
    update_parameters(datsums);
    //a_orig = AL->get_parameters();  //should it be also updated?
    user_interrupt = false;
    evaluations = 0;
    max_iterations = max_iter;
    autoiter();
}

void Fit::update_parameters(vector<DataWithSum*> const& dsds)
{
    if (AL->get_parameters().empty()) 
        throw ExecuteError("there are no fittable parameters.");
    if (dsds.empty())
        throw ExecuteError("No datasets to fit.");

    na = AL->get_parameters().size(); 

    par_usage = vector<bool>(na, false);
    for (int idx = 0; idx < na; ++idx) {
        int var_idx = AL->find_nr_var_handling_param(idx);
        for (vector<DataWithSum*>::const_iterator i = dsds.begin(); 
                                                        i != dsds.end(); ++i) {
            if ((*i)->get_sum()->is_dependent_on_var(var_idx)) {
                par_usage[idx] = true;
                break; //go to next idx
            }
            //verbose(AL->find_variable_handling_param(idx)->xname 
            //        + " is not in chi2.");
        }
    }
}

/// checks termination criteria common for all fitting methods
bool Fit::common_termination_criteria(int iter)
{
    bool stop = false;
    if (user_interrupt) {
        user_interrupt = false;
        info ("Fitting stopped manually.");
        stop = true;
    }
    if (max_iterations >= 0 && iter >= max_iterations) {
        info("Maximum iteration number reached.");
        stop = true;
    }
    int max_evaluations = getSettings()->get_i("max-wssr-evaluations");
    if (max_evaluations > 0 && evaluations >= max_evaluations) {
        info("Maximum evaluations number reached.");
        stop = true;
    }
    return stop;
}

void Fit::iteration_plot(vector<fp> const &A)
{
    AL->use_external_parameters(A);
    getUI()->drawPlot(3, true);
}


/// This function solves a set of linear algebraic equations using
/// Jordan elimination with partial pivoting.
///
/// A * x = b
/// 
/// A is n x n matrix, fp A[n*n]
/// b is vector b[n],   
/// Function returns vector x[] in b[], and 1-matrix in A[].
/// return value: true=OK, false=singular matrix
///   with special exception: 
//     if i'th row, i'th column and i'th element in b all contains zeros,
//     it's just ignored, 
bool Fit::Jordan(vector<fp>& A, vector<fp>& b, int n) 
{
    assert (size(A) == n*n && size(b) == n);
//#define DISABLE_PIVOTING   //don't do it
    for (int i = 0; i < n; i++) {
#ifndef DISABLE_PIVOTING 
        fp amax = 0;                    // looking for a pivot element
        int maxnr = -1;  
        for (int j = i; j < n; j++)                     
            if (fabs (A[n*j+i]) > amax) {
                maxnr = j;
                amax = fabs (A[n * j + i]);
            }
        if (maxnr == -1) {    // singular matrix
            // it's not part of Jordan's method. i-th column has only zeros. 
            // If it's the same about i-th row, and b[i]==0, let x[i]==0. 
            // If not, warn and return
            for (int j = i; j < n; j++)
                if (A[n * i + j] || b[i]) {
                    verbose (print_matrix(A, n, n, "A"));
                    info (print_matrix(b, 1, n, "b"));
                    warn ("Inside Jordan elimination: singular matrix.");
                    verbose ("Column " + S(i) + " is zeroed.");
                    return false;
                }
            continue; // x[i]=b[i], b[i]==0
        }
        if (maxnr != i) {                            // interchanging rows
            for (int j=i; j<n; j++)
                Swap (A[n*maxnr+j], A[n*i+j]);
            Swap (b[i], b[maxnr]);
        }
#else
        if (A[i*n+i] == 0) {
            warn ("Inside Jordan elimination method with "
                    "_disabled_ pivoting: 0 on diagonal row=column=" + S(i));
            return false;
        }
#endif
        register fp foo = 1.0 / A[i*n+i];
        for (int j = i; j < n; j++)
            A[i*n+j] *= foo;
        b[i] *= foo;
        for (int k = 0; k < n; k++)
            if (k != i) {
                foo = A[k * n + i];
                for (int j = i; j < n; j++)
                    A[k * n + j] -= A[i * n + j] * foo;
                b[k] -= b[i] * foo;
            }
    }
    return true;
}

/// A - matrix n x n; returns A^(-1) in A
void Fit::reverse_matrix (vector<fp>&A, int n) 
{
    //no need to optimize it
    assert (size(A) == n*n);    
    vector<fp> A_result(n*n);   
    for (int i = 0; i < n; i++) {
        vector<fp> A_copy = A;      
        vector<fp> v(n, 0);
        v[i] = 1;
        bool r = Jordan(A_copy, v, n);
        if (!r)
            throw ExecuteError("Trying to reverse singular matrix.");
        for (int j = 0; j < n; j++) 
            A_result[j * n + i] = v[j];
    }
    A = A_result;
}

//-------------------------------------------------------------------

FitMethodsContainer* FitMethodsContainer::instance = 0;

FitMethodsContainer* FitMethodsContainer::getInstance()
{
    if (instance == 0)  
        instance = new FitMethodsContainer; 
    return instance; 
}

FitMethodsContainer::FitMethodsContainer()
{
    methods.push_back(new LMfit); 
    methods.push_back(new NMfit); 
    methods.push_back(new GAfit); 
}

FitMethodsContainer::~FitMethodsContainer()
{
    purge_all_elements(methods);
}

int FitMethodsContainer::current_method_number() const
{
    return getSettings()->get_e("fitting-method");
}


