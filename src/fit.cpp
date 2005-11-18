// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "fit.h"
#include <algorithm>
#include <sstream>
#include <time.h>
#include <math.h>
#include "logic.h"
#include "sum.h"
#include "data.h"
#include "ui.h"
#include "LMfit.h"
#include "GAfit.h"
#include "NMfit.h"

using namespace std;

Fit *my_fit;
FitMethodsContainer *fitMethodsContainer;

Fit::Fit (char symb, string m)  
    : symbol(symb), method(m), 
      default_max_iterations(50), output_one_of(1), random_seed(-1),
      max_evaluations(0), evaluations(0), iter_nr (0), na(0)
{
    irpar ["output-one-of"] = IntRange (&output_one_of, 1, 999);
    irpar["pseudo-random-seed"] = IntRange (&random_seed, -1, 999999999);
    ipar["default-max-iterations"] = &default_max_iterations;
    ipar["max-wssr-evaluations"] = &max_evaluations;
    Distrib_enum ['u'] = "uniform";
    Distrib_enum ['g'] = "gauss";
    Distrib_enum ['l'] = "lorentz";
    Distrib_enum ['b'] = "bound";
}

string Fit::getInfo(int mode)
{
    AL->use_parameters();
    vector<fp> const &pp = AL->get_parameters();
    const int n = pp.size(); 
    //n_m = number of points - degrees of freedom (parameters)
    int n_m = -n;
    for (int i = 0; i < 1; i++)  //TODO
        n_m += AL->get_active_ds()->get_data()->get_n(); 
    string s = "Current WSSR = " + S(compute_wssr(pp)) 
                + " (expected: " + S(n_m) + "); SSR = " 
                + S(compute_wssr(pp, false));
    if (mode == 1 || mode == 2) {
        vector<fp> alpha(n*n), beta(n);
        compute_derivatives(pp, alpha, beta);
        reverse_matrix (alpha, n);
        for (vector<fp>::iterator i = alpha.begin(); i != alpha.end(); i++)
            (*i) *= 2;//FIXME: is it right? (S.Brandt, Analiza danych (10.17.4))
        if (mode == 1) {
            s += "\nSymetric errors: ";
            for (int i = 0; i < n; i++) {
                fp val = pp[i];
                s += AL->find_variable_handling_param(i)->xname 
                    + "=" + S(val) + "+-" + S(sqrt(alpha[i * n + i]));
            }
        }
        else if (mode == 2)
            s += "\n" + print_matrix(alpha /*reversed*/, n, n, 
                                     "(co)variance matrix");
    }
    return s;
}

fp Fit::compute_wssr(vector<fp> const &A, bool weigthed)
{
    //TODO fitting multiple plots
    evaluations++;
    fp wssr = 0;
    for (int i = 0; i < 1; ++i) {
        DataWithSum const *ds = AL->get_active_ds();
        AL->use_external_parameters(A);
        wssr += compute_wssr_for_data(ds->get_data(), ds->get_sum(), weigthed);
    }
    return wssr;
}

fp Fit::compute_wssr_for_data(Data const *data, Sum const *sum, bool weigthed)
{
    int n = data->get_n();
    vector<fp> xx(n);
    for (int j = 0; j < n; j++) 
        xx[j] = data->get_x(j);
    vector<fp> yy(n, 0.);
    sum->calculate_sum_value(xx, yy);
    fp wssr = 0;
    for (int j = 0; j < n; j++) {
        fp dy = data->get_y(j) - yy[j];
        if (weigthed)
            dy /= data->get_sigma(j);
        wssr += dy * dy;
    }  
    return wssr;
}

//results in alpha and beta 
void Fit::compute_derivatives(vector<fp> const &A, 
                                vector<fp>& alpha, vector<fp>& beta)
{
    assert (size(A) == na && size(alpha) == na * na && size(beta) == na);
    fill(alpha.begin(), alpha.end(), 0.0);
    fill(beta.begin(), beta.end(), 0.0);

    AL->use_external_parameters(A);
    //TODO fitting multiple plots
    for (int i = 0; i < 1; i++) { 
        DataWithSum const *ds = AL->get_active_ds();
        compute_derivatives_for(ds->get_data(), ds->get_sum(),
                                alpha, beta);
    }
    // filling second half of alpha[] 
    for (int j = 1; j < na; j++)
        for (int k = 0; k < j; k++)
            alpha[na * k + j] = alpha[na * j + k]; 
}

//results in alpha and beta 
//it computes only half of alpha matrix
void Fit::compute_derivatives_for(Data const* data, Sum const* sum,
                                    vector<fp>& alpha, vector<fp>& beta)
{
    int n = data->get_n();
    vector<fp> xx(n);
    for (int j = 0; j < n; j++) 
        xx[j] = data->get_x(j);
    vector<fp> yy(n, 0.);
    const int dyn = na+1;
    vector<fp> dy_da(n*dyn, 0.);
    sum->calculate_sum_value_deriv(xx, yy, dy_da);
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

string Fit::print_matrix (const vector<fp>& vec, int m, int n, char *name)
    //m rows, n columns
{ 
    assert (size(vec) == m * n);
    if (m < 1 || n < 1)
        warn ("In `print_matrix': It is not a matrix.");
    ostringstream h;
    h << name << "={ ";
    if (m == 1) { // vector 
        for (int i = 0; i < n; i++)
            h << vec[i] << (i < n - 1 ? ", " : " }") ;
    }
    else { //matrix 
        std::string blanks (strlen (name) + 1, ' ');
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
    string comment = method + (better ? "" : " (worse)");
    AL->put_new_parameters(aa, method, better);
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

void Fit::fit(bool ini, int max_iter)
{
    if (AL->get_parameters().empty()) 
        throw ExecuteError("there are no fittable parameters.");
    if (ini) {
        user_interrupt = false;
        iter_nr = 0;
        a_orig = AL->get_parameters();
        na = a_orig.size(); 
        int rs = random_seed >= 0 ? random_seed : time(0);
        srand (rs);
        verbose ("Seed for a sequence of pseudo-random numbers: " + S(rs));
        init();
    }
    //was init() callled ?
    else if (na != size(AL->get_parameters())) { 
        warn (method + " method should be initialized first. Canceled");
        return;
    }
    user_interrupt = false;
    evaluations = 0;
    max_iterations = max_iter >= 0 ? max_iter : default_max_iterations;
    autoiter();
}

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

FitMethodsContainer::FitMethodsContainer()
{
    Fit *f = new LMfit;
    methods.push_back(f); 
    f = new NMfit;
    methods.push_back(f); 
    f = new GAfit;
    methods.push_back(f); 
    my_fit = methods[0]; 
}

FitMethodsContainer::~FitMethodsContainer()
{
    for (vector<Fit*>::iterator i = methods.begin(); i != methods.end(); i++)
        delete *i;
}

string FitMethodsContainer::list_available_methods()
{
    string s = "Available methods: ";
    for (vector<Fit*>::iterator i = methods.begin(); i != methods.end(); i++)
        s += S(i == methods.begin() ? " [" : ",  [") + (*i)->symbol + "] " 
            + (*i)->method;
    return s;
}

void FitMethodsContainer::change_method (char c)
{
    if (my_fit->symbol == c) {
        info ("Fitting method already was: " + my_fit->method);
        return;
    }
    for (vector<Fit*>::iterator i = methods.begin(); i != methods.end(); i++)
        if ((*i)->symbol == c) {
            my_fit = *i;
            info ("Fitting method changed to: " + my_fit->method);
            return;
        }
    // if we are here, symbol c was not found in vector methods
    warn ("Unknown symbol for fitting method: " + S(c));
    info (list_available_methods());
}

string FitMethodsContainer::print_current_method()
{
    return "Current fitting method: " + my_fit->method + "\n" + 
                                                list_available_methods();
}

int FitMethodsContainer::current_method_number()
{
    int n = find(methods.begin(), methods.end(), my_fit) - methods.begin();
    assert (n < size(methods));
    return n;
}

void FitMethodsContainer::export_methods_settings_as_script(std::ostream& os)
{
    os << "### Settings of all method\n";
    for (vector<Fit*>::iterator i = methods.begin(); i != methods.end(); i++)
        os << "f.method " << (*i)->symbol << " ### " << (*i)->method
            << endl << (*i)->set_script('f');
}

static const fp TINY = 1e-12;

fp rand_gauss()
{
    static bool is_saved = false;
    static fp saved;
    if (!is_saved) {
        fp rsq = 0, x1, x2;
        while (rsq < TINY || rsq >= 1) {
            x1 = rand_1_1();
            x2 = rand_1_1();
            rsq = x1 * x1 + x2 * x2;
        }
        fp f = sqrt (-2. * log(rsq) / rsq);
        saved = x1 * f;
        is_saved = true;
        return x2 * f;
    }
    else {
        is_saved = false;
        return saved;
    }
}

fp rand_cauchy()
{
    fp rsq = 0, x1, x2;
    while (rsq < TINY || rsq >= 1) {
        x1 = rand_1_1(); 
        x2 = rand_1_1();
        rsq = x1 * x1 + x2 * x2;
    }
    if (fabs(x1) < TINY) //bad luck
        return rand_cauchy(); //try again
    return (x2 / x1);
}


int Fit::Jordan(vector<fp>& A, vector<fp>& b, int n) 
{
    
    /* This function solves a set of linear algebraic equations using
     * Jordan elimination with partial pivoting.
     *
     * A * x = b
     * 
     * A is n x n matrix, fp A[n*n]
     * b is vector b[n],   
     * Function returns vector x[] in b[], and 1-matrix in A[].
     * 
     */
    assert (size(A) == n*n && size(b) == n);
//#define DISABLE_PIVOTING   /*don't do it*/

#ifndef DISABLE_PIVOTING
    int maxnr;
    fp amax;
#endif
    for (int i = 0; i < n; i++) {
#ifndef DISABLE_PIVOTING 
        amax = 0; maxnr = -1;                           // looking for a pivot
        for (int j = i; j < n; j++)                     // element
            if (fabs (A[n*j+i]) > amax){
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
                    verbose ("Column " + S(i) + " is zeroed."
                            " Jordan method with partial pivoting used.");
                    return -i - 1;
                }
            verbose ("In L-M method: @" + S(i) + " not changed.");
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
            return -i - 1;
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
    return 0;
}

int Fit::reverse_matrix (vector<fp>&A, int n) 
    //returns A^(-1) in A
{
    assert (size(A) == n*n);    //it's slow, but there is no need 
    vector<fp> A_result(n*n);   // to optimize it
    for (int i = 0; i < n; i++) {
        vector<fp> A_copy = A;      
        vector<fp> v(n, 0);
        v[i] = 1;
        int r = Jordan(A_copy, v, n);
        if (r != 0)
            return r;
        for (int j = 0; j < n; j++) 
            A_result[j * n + i] = v[j];
    }
    A = A_result;
    return 0;
}


