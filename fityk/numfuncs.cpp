// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "numfuncs.h"
#include <algorithm>
#include <string.h>
#include <assert.h>
#include "common.h"

using namespace std;

namespace fityk {

/// Returns position pos in sorted vector of points: points *pos and *(pos+1)
/// can be used for interpolation of a value at x.
/// Optimized for sequential calls with slowly increasing x's.
template<typename T>
typename vector<T>::iterator
get_interpolation_segment(vector<T> &bb,  double x)
{
    static size_t hint = 0;
    assert (size(bb) > 1);
    // when outside of the range, use the first or the last segment
    if (x <= bb[1].x) {
        hint = 0;
        return bb.begin();
    }
    if (x >= bb.back().x)
        return bb.end() - 2;
    if (hint >= bb.size())
        hint = 0;
    // check if hinted position is good
    typename vector<T>::iterator pos = bb.begin() + hint;
    if (pos->x <= x) {
        //pos->x <= x and x < bb.back().x and bb is sorted  => pos < bb.end()-1
        if (x <= (pos+1)->x) {
            return pos;
        // nope, try the next position
        } if (pos+2 == bb.end() || x <= (pos+2)->x) {
            ++hint;
            return pos+1;
        }
    }
    // nope, use general search
    pos = lower_bound(bb.begin(), bb.end(), T(x, 0)) - 1;
    //printf("DEBUG: get_interpolation_segment %zu -> %zu / %zu (%g)\n",
    //        hint, size_t(pos-bb.begin()), bb.size(), x);
    hint = pos - bb.begin();
    return pos;
}

// explicit instantiation for use in bfunc.cpp in FuncPolyline
template vector<PointD>::iterator
get_interpolation_segment<PointD>(vector<PointD> &bb,  double x);

void prepare_spline_interpolation (vector<PointQ> &bb)
{
    //first wroten for bb interpolation, then generalized
    const int n = bb.size();
    if (n == 0)
        return;
        //find d2y/dx2 and put it in .q
    bb[0].q = 0; //natural spline
    vector<double> u(n);
    for (int k = 1; k <= n-2; k++) {
        PointQ *b = &bb[k];
        double sig = (b->x - (b-1)->x) / ((b+1)->x - (b-1)->x);
        double t = sig * (b-1)->q + 2.;
        b->q = (sig - 1.) / t;
        u[k] = ((b+1)->y - b->y) / ((b+1)->x - b->x) - (b->y - (b-1)->y)
                            / (b->x - (b - 1)->x);
        u[k] = (6. * u[k] / ((b+1)->x - (b-1)->x) - sig * u[k-1]) / t;
    }
    bb.back().q = 0;
    for (int k = n-2; k >= 0; k--) {
        PointQ *b = &bb[k];
        b->q = b->q * (b+1)->q + u[k];
    }
}

double get_spline_interpolation(vector<PointQ> &bb, double x)
{
    if (bb.empty())
        return 0.;
    if (bb.size() == 1)
        return bb[0].y;
    vector<PointQ>::iterator pos = get_interpolation_segment(bb, x);
    // based on Numerical Recipes www.nr.com
    double h = (pos+1)->x - pos->x;
    double a = ((pos+1)->x - x) / h;
    double b = (x - pos->x) / h;
    double t = a * pos->y + b * (pos+1)->y + ((a * a * a - a) * pos->q
               + (b * b * b - b) * (pos+1)->q) * (h * h) / 6.;
    return t;
}

template <typename T>
double get_linear_interpolation_(vector<T> &bb, double x)
{
    if (bb.empty())
        return 0.;
    if (bb.size() == 1)
        return bb[0].y;
    typename vector<T>::iterator pos = get_interpolation_segment(bb, x);
    double a = ((pos + 1)->y - pos->y) / ((pos + 1)->x - pos->x);
    return pos->y + a * (x  - pos->x);
}

double get_linear_interpolation(vector<PointQ> &bb, double x)
{
    return get_linear_interpolation_(bb, x);
}

double get_linear_interpolation(vector<PointD> &bb, double x)
{
    return get_linear_interpolation_(bb, x);
}


// random number utilities
static const double TINY = 1e-12; //only for rand_gauss() and rand_cauchy()

/// normal distribution, mean=0, variance=1
double rand_gauss()
{
    static bool is_saved = false;
    static double saved;
    if (!is_saved) {
        double rsq, x1, x2;
        while(1) {
            x1 = rand_1_1();
            x2 = rand_1_1();
            rsq = x1 * x1 + x2 * x2;
            if (rsq >= TINY && rsq < 1)
                break;
        }
        double f = sqrt(-2. * log(rsq) / rsq);
        saved = x1 * f;
        is_saved = true;
        return x2 * f;
    } else {
        is_saved = false;
        return saved;
    }
}

double rand_cauchy()
{
    while (1) {
        double x1 = rand_1_1();
        double x2 = rand_1_1();
        double rsq = x1 * x1 + x2 * x2;
        if (rsq >= TINY && rsq < 1 && fabs(x1) >= TINY)
            return (x2 / x1);
    }
}


// matrix is in vec, m rows, n columns
string format_matrix(const vector<realt>& vec, int m, int n,
                     const char *mname)
{
    assert (size(vec) == m * n);
    soft_assert(!vec.empty());
    string h = S(mname) + "={ ";
    if (m == 1) { // vector
        for (int i = 0; i < n; i++)
            h += S(vec[i]) + (i < n - 1 ? ", " : " }") ;
    } else { //matrix
        string blanks(strlen(mname) + 1, ' ');
        for (int j = 0; j < m; j++){
            if (j > 0)
                h += blanks + "  ";
            for (int i = 0; i < n; i++)
                h += S(vec[j * n + i]) + ", ";
            h += "\n";
        }
        h += blanks + "}";
    }
    return h;
}

/// This function solves a set of linear algebraic equations using
/// Gauss-Jordan elimination with partial pivoting.
///
/// A * x = b
///
/// A is n x n matrix, realt A[n*n]
/// b is vector b[n],
/// Function returns vector x[] in b[], and 1-matrix in A[].
/// return value: true=OK, false=singular matrix
///   with special exception:
///     if i'th row, i'th column and i'th element in b all contains zeros,
///     it's just ignored,
void jordan_solve(vector<realt>& A, vector<realt>& b, int n)
{
    assert (size(A) == n*n && size(b) == n);
    for (int i = 0; i < n; i++) {
        // looking for a pivot element
        int maxnr = -1;
        realt amax = 0;
        for (int j = i; j < n; j++)
            if (fabs (A[n*j+i]) > amax) {
                maxnr = j;
                amax = fabs (A[n * j + i]);
            }
        // handle singular matrix
        if (maxnr == -1) {
            // i-th column has only zeros.
            // If it's the same about i-th row, and b[i]==0, let x[i]==0.
            for (int j = i; j < n; j++)
                if (A[n * i + j] || b[i]) {
                    // printf("%s\n", format_matrix(A, n, n, "A"));
                    printf("%s\n", format_matrix(b, 1, n, "b").c_str());
                    throw ExecuteError("Trying to reverse singular matrix."
                                       " Column " + S(i) + " is zeroed.");
                }
            continue; // x[i]=b[i], b[i]==0
        }
        // interchanging rows
        if (maxnr != i) {
            for (int j = i; j < n; j++)
                swap (A[n*maxnr+j], A[n*i+j]);
            swap (b[i], b[maxnr]);
        }
        // divide by a_ii -- to get a_ii=1
        realt c = 1.0 / A[i*n+i];
        for (int j = i; j < n; j++)
            A[i*n+j] *= c;
        b[i] *= c;
        // subtract -- to zero all remaining elements of this row
        for (int k = 0; k < n; k++)
            if (k != i) {
                realt d = A[k * n + i];
                for (int j = i; j < n; j++)
                    A[k * n + j] -= A[i * n + j] * d;
                b[k] -= b[i] * d;
            }
    }
}

/// Invert matrix using Gauss-Jordan elimination with partial pivoting,
/// based on http://www.isical.ac.in/~arnabc/matalgop1.pdf
/// A - matrix n x n; returns A^(-1) in A
void invert_matrix(vector<realt>&A, int n)
{
    assert(size(A) == n*n);

    vector<int> row_swaps(n);
    for (int i = 0; i != n; ++i) {
        // looking for a pivot element
        int maxnr = -1;
        realt amax = 0;
        for (int j = i; j != n; ++j)
            if (fabs(A[j*n+i]) > amax) {
                maxnr = j;
                amax = fabs(A[j*n+i]);
            }
        row_swaps[i] = maxnr;
        // handle singular matrix
        if (maxnr == -1) {
            // As a special exception we allow singular matrix if both
            // i-th column and i-th row are completely zeroed.
            // Such row and column are ignored and unchanged.
            // (Pivoting doesn't move such column+row.)
            // Actually this exception is not be necessary.
            for (int j = i; j != n; ++j)
                if (A[i*n+j] != 0.) {
                    // changed wording from jordan_solve() - to tell them apart
                    throw ExecuteError("Singular matrix cannot be reversed."
                                       " (zeroed column " + S(i) + ").");
                }
            continue;
        }
        // interchanging rows
        if (maxnr != i) {
            for (int j = 0; j != n; j++)
                swap(A[maxnr*n+j], A[i*n+j]);
        }
        realt inv_piv = 1. / A[i*n+i];
        for (int k = 0; k != n; ++k)
            if (k != i) {
                realt d = A[k*n+i] * inv_piv;
                for (int j = 0; j != n; ++j) {
                    if (j == i)
                        A[k*n+j] = -d;
                    else
                        A[k*n+j] -= A[i*n+j] * d;
                }
            }
        for (int j = 0; j != n; ++j)
            A[i*n+j] *= inv_piv;
        A[i*n+i] = inv_piv;
    }
    // apply the swaps in reverse order - row swap becomes a column swap
    for (int i = n-1; i >= 0; --i) {
        int p = row_swaps[i];
        if (p != -1 && p != i)
            for (int j = 0; j != n; ++j)
                swap(A[j*n+p], A[j*n+i]);
    }
}


void SimplePolylineConvex::push_point(PointD const& p)
{
    if (vertices_.size() < 2
            || is_left(*(vertices_.end() - 2), *(vertices_.end() - 1), p))
        vertices_.push_back(p);
    else {
        // the middle point (the last one currently in vertices_) is not convex
        // remove it and check again the last three points
        vertices_.pop_back();
        push_point(p);
    }
}

} // namespace fityk
