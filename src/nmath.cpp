
/* a few files from Mathlib from R-project (also GPL'ed)
 * merged into one file and slightly modified.
 * The homepage of R-project: http://www.r-project.org/
 * Mathlib is a library of mathematical special functions.
 * Original filenames: lgamma.c stirlerr.c chebyshev.c 
 *                     lgammacor.c gamma.c i1mach.c d1mach.c polygamma.c
 * All my modification have MW in comments -- Marcin Wojdyr
 * Declarations of public functions from this file are in numfuncs.h
 */

// a minimal set of declarations and macros that allows to not include
// the whole Mathlib
#include <math.h>
#include <float.h>
#include <limits.h>
#include <errno.h>

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_ISNAN
# define ISNAN(x) isnan(x)
#else
# define ISNAN(x) ((x) != (x))
#endif
#define attribute_hidden
int Rf_i1mach(int);
#define i1mach Rf_i1mach
#define d1mach Rf_d1mach
int imin2(int x, int y) { return (x < y) ? x : y; }

double fmin2(double x, double y)
{
    if (ISNAN(x) || ISNAN(y))
        return x + y;
    return (x < y) ? x : y;
}

double fmax2(double x, double y)
{
    if (ISNAN(x) || ISNAN(y))
        return x + y;
    return (x < y) ? y : x;
}

//#ifndef _AIX
//const double R_Zero_Hack = 0.0; /* Silence the Sun compiler */
//#else
double R_Zero_Hack = 0.0;
//#endif
#define ML_POSINF (1./R_Zero_Hack)
#define ML_NEGINF (-1./R_Zero_Hack)
#define ML_NAN (0./R_Zero_Hack)
#define ML_ERROR(x, s)  
#define MATHLIB_WARNING(fmt,x) 
#define MATHLIB_WARNING2(fmt,x,x2)
#define ML_ERR_return_NAN { return ML_NAN; }
#define ML_UNDERFLOW    (DBL_MIN * DBL_MIN)
#ifndef M_LN_SQRT_2PI
#define M_LN_SQRT_2PI   0.918938533204672741780329736406  /* log(sqrt(2*pi)) */
#endif
#ifndef M_LN_SQRT_PId2
#define M_LN_SQRT_PId2  0.225791352644727432363097614947  /* log(sqrt(pi/2)) */
#endif
double gammafn(double x);
double lgammacor(double x);


/////////////////////////////////////////////////////////////////////////
//                    lgamma.c
/////////////////////////////////////////////////////////////////////////
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *  Copyright (C) 2000-2001 The R Development Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  SYNOPSIS
 *
 *    #include <Rmath.h>
 *    extern int R_signgam;
 *    double lgammafn(double x);
 *
 *  DESCRIPTION
 *
 *    This function computes log|gamma(x)|.  At the same time
 *    the variable "R_signgam" is set to the sign of the gamma
 *    function.
 *
 *  NOTES
 *
 *    This routine is a translation into C of a Fortran subroutine
 *    by W. Fullerton of Los Alamos Scientific Laboratory.
 *
 *    The accuracy of this routine compares (very) favourably
 *    with those of the Sun Microsystems portable mathematical
 *    library.
 */

#if 0 //MW
#include "nmath.h"
#endif //0, MW

int attribute_hidden R_signgam;

double lgammafn(double x)
{
    double ans, y, sinpiy;

#ifdef NOMORE_FOR_THREADS
    static double xmax = 0.;
    static double dxrel = 0.;

    if (xmax == 0) {/* initialize machine dependent constants _ONCE_ */
	xmax = d1mach(2)/log(d1mach(2));/* = 2.533 e305	 for IEEE double */
	dxrel = sqrt (d1mach(4));/* sqrt(Eps) ~ 1.49 e-8  for IEEE double */
    }
#else
/* For IEEE double precision DBL_EPSILON = 2^-52 = 2.220446049250313e-16 :
   xmax  = DBL_MAX / log(DBL_MAX) = 2^1024 / (1024 * log(2)) = 2^1014 / log(2)
   dxrel = sqrt(DBL_EPSILON) = 2^-26 = 5^26 * 1e-26 (is *exact* below !)
 */
#define xmax  2.5327372760800758e+305
#define dxrel 1.490116119384765696e-8
#endif

    R_signgam = 1;

#ifdef IEEE_754
    if(ISNAN(x)) return x;
#endif

    if (x < 0 && fmod(floor(-x), 2.) == 0)
	R_signgam = -1;

    if (x <= 0 && x == trunc(x)) { /* Negative integer argument */
	ML_ERROR(ME_RANGE, "lgamma");
	return ML_POSINF;/* +Inf, since lgamma(x) = log|gamma(x)| */
    }

    y = fabs(x);

    if (y <= 10)
	return log(fabs(gammafn(x)));
    /*
      ELSE  y = |x| > 10 ---------------------- */

    if (y > xmax) {
	ML_ERROR(ME_RANGE, "lgamma");
	return ML_POSINF;
    }

    if (x > 0) { /* i.e. y = x > 10 */
#ifdef IEEE_754
	if(x > 1e17)
	    return(x*(log(x) - 1.));
	else if(x > 4934720.)
	    return(M_LN_SQRT_2PI + (x - 0.5) * log(x) - x);
	else
#endif
	    return M_LN_SQRT_2PI + (x - 0.5) * log(x) - x + lgammacor(x);
    }
    /* else: x < -10; y = -x */
    sinpiy = fabs(sin(M_PI * y));

    if (sinpiy == 0) { /* Negative integer argument ===
			  Now UNNECESSARY: caught above */
	MATHLIB_WARNING(" ** should NEVER happen! *** [lgamma.c: Neg.int, y=%g]\n",y);
	ML_ERR_return_NAN;
    }

    ans = M_LN_SQRT_PId2 + (x - 0.5) * log(y) - x - log(sinpiy) - lgammacor(y);

    if(fabs((x - trunc(x - 0.5)) * ans / x) < dxrel) {

	/* The answer is less than half precision because
	 * the argument is too near a negative integer. */

	ML_ERROR(ME_PRECISION, "lgamma");
    }

    return ans;
}
/////////////////////////////////////////////////////////////////////////
//                    stirlerr.c
/////////////////////////////////////////////////////////////////////////
/*
 *  AUTHOR
 *    Catherine Loader, catherine@research.bell-labs.com.
 *    October 23, 2000.
 *
 *  Merge in to R:
 *	Copyright (C) 2000, The R Core Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
 *  DESCRIPTION
 *
 *    Computes the log of the error term in Stirling's formula.
 *      For n > 15, uses the series 1/12n - 1/360n^3 + ...
 *      For n <=15, integers or half-integers, uses stored values.
 *      For other n < 15, uses lgamma directly (don't use this to
 *        write lgamma!)
 *
 * Merge in to R:
 * Copyright (C) 2000, The R Core Development Team
 * R has lgammafn, and lgamma is not part of ISO C
 */

#if 0 //MW
#include "nmath.h"
#endif //0, MW

/* stirlerr(n) = log(n!) - log( sqrt(2*pi*n)*(n/e)^n )
 *             = log Gamma(n+1) - 1/2 * [log(2*pi) + log(n)] - n*[log(n) - 1]
 *             = log Gamma(n+1) - (n + 1/2) * log(n) + n - log(2*pi)/2
 *
 * see also lgammacor() in ./lgammacor.c  which computes almost the same!
 */

double attribute_hidden stirlerr(double n)
{

#define S0 0.083333333333333333333       /* 1/12 */
#define S1 0.00277777777777777777778     /* 1/360 */
#define S2 0.00079365079365079365079365  /* 1/1260 */
#define S3 0.000595238095238095238095238 /* 1/1680 */
#define S4 0.0008417508417508417508417508/* 1/1188 */

/*
  error for 0, 0.5, 1.0, 1.5, ..., 14.5, 15.0.
*/
    const static double sferr_halves[31] = {
	0.0, /* n=0 - wrong, place holder only */
	0.1534264097200273452913848,  /* 0.5 */
	0.0810614667953272582196702,  /* 1.0 */
	0.0548141210519176538961390,  /* 1.5 */
	0.0413406959554092940938221,  /* 2.0 */
	0.03316287351993628748511048, /* 2.5 */
	0.02767792568499833914878929, /* 3.0 */
	0.02374616365629749597132920, /* 3.5 */
	0.02079067210376509311152277, /* 4.0 */
	0.01848845053267318523077934, /* 4.5 */
	0.01664469118982119216319487, /* 5.0 */
	0.01513497322191737887351255, /* 5.5 */
	0.01387612882307074799874573, /* 6.0 */
	0.01281046524292022692424986, /* 6.5 */
	0.01189670994589177009505572, /* 7.0 */
	0.01110455975820691732662991, /* 7.5 */
	0.010411265261972096497478567, /* 8.0 */
	0.009799416126158803298389475, /* 8.5 */
	0.009255462182712732917728637, /* 9.0 */
	0.008768700134139385462952823, /* 9.5 */
	0.008330563433362871256469318, /* 10.0 */
	0.007934114564314020547248100, /* 10.5 */
	0.007573675487951840794972024, /* 11.0 */
	0.007244554301320383179543912, /* 11.5 */
	0.006942840107209529865664152, /* 12.0 */
	0.006665247032707682442354394, /* 12.5 */
	0.006408994188004207068439631, /* 13.0 */
	0.006171712263039457647532867, /* 13.5 */
	0.005951370112758847735624416, /* 14.0 */
	0.005746216513010115682023589, /* 14.5 */
	0.005554733551962801371038690  /* 15.0 */
    };
    double nn;

    if (n <= 15.0) {
	nn = n + n;
	if (nn == (int)nn) return(sferr_halves[(int)nn]);
	return(lgammafn(n + 1.) - (n + 0.5)*log(n) + n - M_LN_SQRT_2PI);
    }

    nn = n*n;
    if (n>500) return((S0-S1/nn)/n);
    if (n> 80) return((S0-(S1-S2/nn)/nn)/n);
    if (n> 35) return((S0-(S1-(S2-S3/nn)/nn)/nn)/n);
    /* 15 < n <= 35 : */
    return((S0-(S1-(S2-(S3-S4/nn)/nn)/nn)/nn)/n);
}
/////////////////////////////////////////////////////////////////////////
//                    chebyshev.c
/////////////////////////////////////////////////////////////////////////
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 *  SYNOPSIS
 *
 *    int chebyshev_init(double *dos, int nos, double eta)
 *    double chebyshev_eval(double x, double *a, int n)
 *
 *  DESCRIPTION
 *
 *    "chebyshev_init" determines the number of terms for the
 *    double precision orthogonal series "dos" needed to insure
 *    the error is no larger than "eta".  Ordinarily eta will be
 *    chosen to be one-tenth machine precision.
 *
 *    "chebyshev_eval" evaluates the n-term Chebyshev series
 *    "a" at "x".
 *
 *  NOTES
 *
 *    These routines are translations into C of Fortran routines
 *    by W. Fullerton of Los Alamos Scientific Laboratory.
 *
 *    Based on the Fortran routine dcsevl by W. Fullerton.
 *    Adapted from R. Broucke, Algorithm 446, CACM., 16, 254 (1973).
 */

#if 0 //MW
#include "nmath.h"
#endif //0, MW

/* NaNs propagated correctly */


int attribute_hidden chebyshev_init(double *dos, int nos, double eta)
{
    int i, ii;
    double err;

    if (nos < 1)
	return 0;

    err = 0.0;
    i = 0;			/* just to avoid compiler warnings */
    for (ii=1; ii<=nos; ii++) {
	i = nos - ii;
	err += fabs(dos[i]);
	if (err > eta) {
	    return i;
	}
    }
    return i;
}


double attribute_hidden chebyshev_eval(double x, const double *a, const int n)
{
    double b0, b1, b2, twox;
    int i;

    if (n < 1 || n > 1000) ML_ERR_return_NAN;

    if (x < -1.1 || x > 1.1) ML_ERR_return_NAN;

    twox = x * 2;
    b2 = b1 = 0;
    b0 = 0;
    for (i = 1; i <= n; i++) {
	b2 = b1;
	b1 = b0;
	b0 = twox * b1 - b2 + a[n - i];
    }
    return (b0 - b2) * 0.5;
}
/////////////////////////////////////////////////////////////////////////
#undef xmax //MW
#undef dxrel //MW
/////////////////////////////////////////////////////////////////////////
//                    lgammacor.c
/////////////////////////////////////////////////////////////////////////
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *  Copyright (C) 2000-2001 The R Development Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  SYNOPSIS
 *
 *    #include <Rmath.h>
 *    double lgammacor(double x);
 *
 *  DESCRIPTION
 *
 *    Compute the log gamma correction factor for x >= 10 so that
 *
 *    log(gamma(x)) = .5*log(2*pi) + (x-.5)*log(x) -x + lgammacor(x)
 *
 *    [ lgammacor(x) is called	Del(x)	in other contexts (e.g. dcdflib)]
 *
 *  NOTES
 *
 *    This routine is a translation into C of a Fortran subroutine
 *    written by W. Fullerton of Los Alamos Scientific Laboratory.
 *
 *  SEE ALSO
 *
 *    Loader(1999)'s stirlerr() {in ./stirlerr.c} is *very* similar in spirit,
 *    is faster and cleaner, but is only defined "fast" for half integers.
 */

#if 0 //MW
#include "nmath.h"
#endif //0, MW

double attribute_hidden lgammacor(double x)
{
    const static double algmcs[15] = {
	+.1666389480451863247205729650822e+0,
	-.1384948176067563840732986059135e-4,
	+.9810825646924729426157171547487e-8,
	-.1809129475572494194263306266719e-10,
	+.6221098041892605227126015543416e-13,
	-.3399615005417721944303330599666e-15,
	+.2683181998482698748957538846666e-17,
	-.2868042435334643284144622399999e-19,
	+.3962837061046434803679306666666e-21,
	-.6831888753985766870111999999999e-23,
	+.1429227355942498147573333333333e-24,
	-.3547598158101070547199999999999e-26,
	+.1025680058010470912000000000000e-27,
	-.3401102254316748799999999999999e-29,
	+.1276642195630062933333333333333e-30
    };

    double tmp;

#ifdef NOMORE_FOR_THREADS
    static int nalgm = 0;
    static double xbig = 0, xmax = 0;

    /* Initialize machine dependent constants, the first time gamma() is called.
	FIXME for threads ! */
    if (nalgm == 0) {
	/* For IEEE double precision : nalgm = 5 */
	nalgm = chebyshev_init(algmcs, 15, DBL_EPSILON/2);/*was d1mach(3)*/
	xbig = 1 / sqrt(DBL_EPSILON/2); /* ~ 94906265.6 for IEEE double */
	xmax = exp(fmin2(log(DBL_MAX / 12), -log(12 * DBL_MIN)));
	/*   = DBL_MAX / 48 ~= 3.745e306 for IEEE double */
    }
#else
/* For IEEE double precision DBL_EPSILON = 2^-52 = 2.220446049250313e-16 :
 *   xbig = 2 ^ 26.5
 *   xmax = DBL_MAX / 48 =  2^1020 / 3 */
# define nalgm 5
# define xbig  94906265.62425156
# define xmax  3.745194030963158e306
#endif

    if (x < 10)
	ML_ERR_return_NAN
    else if (x >= xmax) {
	ML_ERROR(ME_UNDERFLOW, "lgammacor");
	return ML_UNDERFLOW;
    }
    else if (x < xbig) {
	tmp = 10 / x;
	return chebyshev_eval(tmp * tmp * 2 - 1, algmcs, nalgm) / x;
    }
    else return 1 / (x * 12);
}
/////////////////////////////////////////////////////////////////////////
#undef xmax //MW
/////////////////////////////////////////////////////////////////////////
//                    gamma.c
/////////////////////////////////////////////////////////////////////////
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *  Copyright (C) 2000-2001 The R Development Core Team
 *  Copyright (C) 2002-2004 The R Foundation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  SYNOPSIS
 *
 *    #include <Rmath.h>
 *    double gammafn(double x);
 *
 *  DESCRIPTION
 *
 *    This function computes the value of the gamma function.
 *
 *  NOTES
 *
 *    This function is a translation into C of a Fortran subroutine
 *    by W. Fullerton of Los Alamos Scientific Laboratory.
 *
 *    The accuracy of this routine compares (very) favourably
 *    with those of the Sun Microsystems portable mathematical
 *    library.
 *
 *    MM specialized the case of  n!  for n < 50 - for even better precision
 */

#if 0 //MW
#include "nmath.h"
#endif //0, MW

double gammafn(double x)
{
    const static double gamcs[42] = {
	+.8571195590989331421920062399942e-2,
	+.4415381324841006757191315771652e-2,
	+.5685043681599363378632664588789e-1,
	-.4219835396418560501012500186624e-2,
	+.1326808181212460220584006796352e-2,
	-.1893024529798880432523947023886e-3,
	+.3606925327441245256578082217225e-4,
	-.6056761904460864218485548290365e-5,
	+.1055829546302283344731823509093e-5,
	-.1811967365542384048291855891166e-6,
	+.3117724964715322277790254593169e-7,
	-.5354219639019687140874081024347e-8,
	+.9193275519859588946887786825940e-9,
	-.1577941280288339761767423273953e-9,
	+.2707980622934954543266540433089e-10,
	-.4646818653825730144081661058933e-11,
	+.7973350192007419656460767175359e-12,
	-.1368078209830916025799499172309e-12,
	+.2347319486563800657233471771688e-13,
	-.4027432614949066932766570534699e-14,
	+.6910051747372100912138336975257e-15,
	-.1185584500221992907052387126192e-15,
	+.2034148542496373955201026051932e-16,
	-.3490054341717405849274012949108e-17,
	+.5987993856485305567135051066026e-18,
	-.1027378057872228074490069778431e-18,
	+.1762702816060529824942759660748e-19,
	-.3024320653735306260958772112042e-20,
	+.5188914660218397839717833550506e-21,
	-.8902770842456576692449251601066e-22,
	+.1527474068493342602274596891306e-22,
	-.2620731256187362900257328332799e-23,
	+.4496464047830538670331046570666e-24,
	-.7714712731336877911703901525333e-25,
	+.1323635453126044036486572714666e-25,
	-.2270999412942928816702313813333e-26,
	+.3896418998003991449320816639999e-27,
	-.6685198115125953327792127999999e-28,
	+.1146998663140024384347613866666e-28,
	-.1967938586345134677295103999999e-29,
	+.3376448816585338090334890666666e-30,
	-.5793070335782135784625493333333e-31
    };

    int i, n;
    double y;
    double sinpiy, value;

#ifdef NOMORE_FOR_THREADS
    static int ngam = 0;
    static double xmin = 0, xmax = 0., xsml = 0., dxrel = 0.;

    /* Initialize machine dependent constants, the first time gamma() is called.
	FIXME for threads ! */
    if (ngam == 0) {
	ngam = chebyshev_init(gamcs, 42, DBL_EPSILON/20);/*was .1*d1mach(3)*/
	gammalims(&xmin, &xmax);/*-> ./gammalims.c */
	xsml = exp(fmax2(log(DBL_MIN), -log(DBL_MAX)) + 0.01);
	/*   = exp(.01)*DBL_MIN = 2.247e-308 for IEEE */
	dxrel = sqrt(1/DBL_EPSILON);/*was (1/d1mach(4)) */
    }
#else
/* For IEEE double precision DBL_EPSILON = 2^-52 = 2.220446049250313e-16 :
 * (xmin, xmax) are non-trivial, see ./gammalims.c
 * xsml = exp(.01)*DBL_MIN
 * dxrel = sqrt(1/DBL_EPSILON) = 2 ^ 26
*/
# define ngam 22
# define xmin -170.5674972726612
# define xmax  171.61447887182298
# define xsml 2.2474362225598545e-308
# define dxrel 67108864.
#endif

    if(ISNAN(x)) return x;

    /* If the argument is exactly zero or a negative integer
     * then return NaN. */
    if (x == 0 || (x < 0 && x == (long)x)) {
	ML_ERROR(ME_DOMAIN, "gammafn");
	return ML_NAN;
    }

    y = fabs(x);

    if (y <= 10) {

	/* Compute gamma(x) for -10 <= x <= 10
	 * Reduce the interval and find gamma(1 + y) for 0 <= y < 1
	 * first of all. */

#if 0 //MW
	n = x;
#else //MW
	n = (int) x; //MW
#endif //0, MW
	if(x < 0) --n;
	y = x - n;/* n = floor(x)  ==>	y in [ 0, 1 ) */
	--n;
	value = chebyshev_eval(y * 2 - 1, gamcs, ngam) + .9375;
	if (n == 0)
	    return value;/* x = 1.dddd = 1+y */

	if (n < 0) {
	    /* compute gamma(x) for -10 <= x < 1 */

	    /* exact 0 or "-n" checked already above */

	    /* The answer is less than half precision */
	    /* because x too near a negative integer. */
	    if (x < -0.5 && fabs(x - (int)(x - 0.5) / x) < dxrel) {
		ML_ERROR(ME_PRECISION, "gammafn");
	    }

	    /* The argument is so close to 0 that the result would overflow. */
	    if (y < xsml) {
		ML_ERROR(ME_RANGE, "gammafn");
		if(x > 0) return ML_POSINF;
		else return ML_NEGINF;
	    }

	    n = -n;

	    for (i = 0; i < n; i++) {
		value /= (x + i);
	    }
	    return value;
	}
	else {
	    /* gamma(x) for 2 <= x <= 10 */

	    for (i = 1; i <= n; i++) {
		value *= (y + i);
	    }
	    return value;
	}
    }
    else {
	/* gamma(x) for	 y = |x| > 10. */

	if (x > xmax) {			/* Overflow */
	    ML_ERROR(ME_RANGE, "gammafn");
	    return ML_POSINF;
	}

	if (x < xmin) {			/* Underflow */
	    ML_ERROR(ME_UNDERFLOW, "gammafn");
	    return ML_UNDERFLOW;
	}

	if(y <= 50 && y == (int)y) { /* compute (n - 1)! */
	    value = 1.;
	    for (i = 2; i < y; i++) value *= i;
	}
	else { /* normal case */
	    value = exp((y - 0.5) * log(y) - y + M_LN_SQRT_2PI +
			((2*y == (int)2*y)? stirlerr(y) : lgammacor(y)));
	}
	if (x > 0)
	    return value;

	if (fabs((x - (int)(x - 0.5))/x) < dxrel){

	    /* The answer is less than half precision because */
	    /* the argument is too near a negative integer. */

	    ML_ERROR(ME_PRECISION, "gammafn");
	}

	sinpiy = sin(M_PI * y);
	if (sinpiy == 0) {		/* Negative integer arg - overflow */
	    ML_ERROR(ME_RANGE, "gammafn");
	    return ML_POSINF;
	}

	return -M_PI / (y * sinpiy * value);
    }
}
/////////////////////////////////////////////////////////////////////////
//                    i1mach.c
/////////////////////////////////////////////////////////////////////////
/*
 *  Mathlib - A Mathematical Function Library
 *  Copyright (C) 1998  Ross Ihaka
 *  Copyright (C) 2000 The R Development Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if 0 //MW
#include "nmath.h" 
#undef i1mach
#endif //0, MW

/* This is (naughtily) used by port.c in package stats */
int Rf_i1mach(int i)
{
    switch(i) {

    case  1: return 5;
    case  2: return 6;
    case  3: return 0;
    case  4: return 0;

    case  5: return CHAR_BIT * sizeof(int);
    case  6: return sizeof(int)/sizeof(char);

    case  7: return 2;
    case  8: return CHAR_BIT * sizeof(int) - 1;
    case  9: return INT_MAX;

    case 10: return FLT_RADIX;

    case 11: return FLT_MANT_DIG;
    case 12: return FLT_MIN_EXP;
    case 13: return FLT_MAX_EXP;

    case 14: return DBL_MANT_DIG;
    case 15: return DBL_MIN_EXP;
    case 16: return DBL_MAX_EXP;

    default: return 0;
    }
}

#if 0 //MW
int F77_NAME(i1mach)(int *i)
{
    return Rf_i1mach(*i);
}
#endif //0, MW
/////////////////////////////////////////////////////////////////////////
//                    d1mach.c
/////////////////////////////////////////////////////////////////////////
/*
 *  Mathlib - A Mathematical Function Library
 *  Copyright (C) 1998  Ross Ihaka
 *  Copyright (C) 2000 The R Development Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

/* NaNs propagated correctly */


/*-- FIXME:  Eliminate calls to these
 *   =====   o   from C code when
 *	     o   it is only used to initialize "static" variables (threading)
 *  and use the DBL_... constants instead
 */

#if 0 //MW
#include "nmath.h"
#undef d1mach
#endif //0, MW

/* This is (naughtily) used by port.c in package stats */
double Rf_d1mach(int i)
{
    switch(i) {
    case 1: return DBL_MIN;
    case 2: return DBL_MAX;

    case 3: /* = FLT_RADIX  ^ - DBL_MANT_DIG
	      for IEEE:  = 2^-53 = 1.110223e-16 = .5*DBL_EPSILON */
	return pow((double)i1mach(10), -(double)i1mach(14));

    case 4: /* = FLT_RADIX  ^ (1- DBL_MANT_DIG) =
	      for IEEE:  = 2^52 = 4503599627370496 = 1/DBL_EPSILON */
	return pow((double)i1mach(10), 1-(double)i1mach(14));

    case 5: return log10(2.0);/* = M_LOG10_2 in Rmath.h */


    default: return 0.0;
    }
}

#if 0 //MW
double F77_NAME(d1mach)(int *i)
{
    return Rf_d1mach(*i);
}
#endif //0, MW
/////////////////////////////////////////////////////////////////////////
#undef xmin //MW
/////////////////////////////////////////////////////////////////////////
//                    polygamma.c
/////////////////////////////////////////////////////////////////////////
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *  Copyright (C) 2000-2001 the R Development Core Team
 *  Copyright (C) 2004	    The R Foundation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  SYNOPSIS
 *
 *    #include <Rmath.h>
 *    void dpsifn(double x, int n, int kode, int m,
 *		  double *ans, int *nz, int *ierr)
 *    double digamma(double x);
 *    double trigamma(double x)
 *    double tetragamma(double x)
 *    double pentagamma(double x)
 *
 *  DESCRIPTION
 *
 *    Compute the derivatives of the psi function
 *    and polygamma functions.
 *
 *    The following definitions are used in dpsifn:
 *
 *    Definition 1
 *
 *	 psi(x) = d/dx (ln(gamma(x)),  the first derivative of
 *				       the log gamma function.
 *
 *    Definition 2
 *		     k	 k
 *	 psi(k,x) = d /dx (psi(x)),    the k-th derivative
 *				       of psi(x).
 *
 *
 *    "dpsifn" computes a sequence of scaled derivatives of
 *    the psi function; i.e. for fixed x and m it computes
 *    the m-member sequence
 *
 *		  (-1)^(k+1) / gamma(k+1) * psi(k,x)
 *		     for k = n,...,n+m-1
 *
 *    where psi(k,x) is as defined above.   For kode=1, dpsifn
 *    returns the scaled derivatives as described.  kode=2 is
 *    operative only when k=0 and in that case dpsifn returns
 *    -psi(x) + ln(x).	That is, the logarithmic behavior for
 *    large x is removed when kode=2 and k=0.  When sums or
 *    differences of psi functions are computed the logarithmic
 *    terms can be combined analytically and computed separately
 *    to help retain significant digits.
 *
 *    Note that dpsifn(x, 0, 1, 1, ans) results in ans = -psi(x).
 *
 *  INPUT
 *
 *	x     - argument, x > 0.
 *
 *	n     - first member of the sequence, 0 <= n <= 100
 *		n == 0 gives ans(1) = -psi(x)	    for kode=1
 *				      -psi(x)+ln(x) for kode=2
 *
 *	kode  - selection parameter
 *		kode == 1 returns scaled derivatives of the
 *		psi function.
 *		kode == 2 returns scaled derivatives of the
 *		psi function except when n=0. In this case,
 *		ans(1) = -psi(x) + ln(x) is returned.
 *
 *	m     - number of members of the sequence, m >= 1
 *
 *  OUTPUT
 *
 *	ans   - a vector of length at least m whose first m
 *		components contain the sequence of derivatives
 *		scaled according to kode.
 *
 *	nz    - underflow flag
 *		nz == 0, a normal return
 *		nz != 0, underflow, last nz components of ans are
 *			 set to zero, ans(m-k+1)=0.0, k=1,...,nz
 *
 *	ierr  - error flag
 *		ierr=0, a normal return, computation completed
 *		ierr=1, input error,	 no computation
 *		ierr=2, overflow,	 x too small or n+m-1 too
 *			large or both
 *		ierr=3, error,		 n too large. dimensioned
 *			array trmr(nmax) is not large enough for n
 *
 *    The nominal computational accuracy is the maximum of unit
 *    roundoff (d1mach(4)) and 1e-18 since critical constants
 *    are given to only 18 digits.
 *
 *    The basic method of evaluation is the asymptotic expansion
 *    for large x >= xmin followed by backward recursion on a two
 *    term recursion relation
 *
 *	     w(x+1) + x^(-n-1) = w(x).
 *
 *    this is supplemented by a series
 *
 *	     sum( (x+k)^(-n-1) , k=0,1,2,... )
 *
 *    which converges rapidly for large n. both xmin and the
 *    number of terms of the series are calculated from the unit
 *    roundoff of the machine environment.
 *
 *  AUTHOR
 *
 *    Amos, D. E.  	(Fortran)
 *    Ross Ihaka   	(C Translation)
 *    Martin Maechler   (x < 0, and psigamma())
 *
 *  REFERENCES
 *
 *    Handbook of Mathematical Functions,
 *    National Bureau of Standards Applied Mathematics Series 55,
 *    Edited by M. Abramowitz and I. A. Stegun, equations 6.3.5,
 *    6.3.18, 6.4.6, 6.4.9 and 6.4.10, pp.258-260, 1964.
 *
 *    D. E. Amos, (1983). "A Portable Fortran Subroutine for
 *    Derivatives of the Psi Function", Algorithm 610,
 *    TOMS 9(4), pp. 494-502.
 *
 *    Routines called: d1mach, i1mach.
 */

#if 0 //MW
#include "nmath.h"
#endif //0, MW

#define n_max (100)

void dpsifn(double x, int n, int kode, int m, double *ans, int *nz, int *ierr)
{
    const static double bvalues[] = {	/* Bernoulli Numbers */
	 1.00000000000000000e+00,
	-5.00000000000000000e-01,
	 1.66666666666666667e-01,
	-3.33333333333333333e-02,
	 2.38095238095238095e-02,
	-3.33333333333333333e-02,
	 7.57575757575757576e-02,
	-2.53113553113553114e-01,
	 1.16666666666666667e+00,
	-7.09215686274509804e+00,
	 5.49711779448621554e+01,
	-5.29124242424242424e+02,
	 6.19212318840579710e+03,
	-8.65802531135531136e+04,
	 1.42551716666666667e+06,
	-2.72982310678160920e+07,
	 6.01580873900642368e+08,
	-1.51163157670921569e+10,
	 4.29614643061166667e+11,
	-1.37116552050883328e+13,
	 4.88332318973593167e+14,
	-1.92965793419400681e+16
    };
    const static double *b = (double *)&bvalues -1; /* ==> b[1] = bvalues[0], etc */
    const int nmax = n_max;

    int i, j, k, mm, mx, nn, np, nx, fn;
    double arg, den, elim, eps, fln, fx, rln, rxsq,
	r1m4, r1m5, s, slope, t, ta, tk, tol, tols, tss, tst,
	tt, t1, t2, wdtol, xdmln, xdmy, xinc, xln = 0.0 /* -Wall */, 
	xm, xmin, xq, yint;
    double trm[23], trmr[n_max + 1];

    *ierr = 0;
    if (n < 0 || kode < 1 || kode > 2 || m < 1) {
	*ierr = 1;
	return;
    }
    if (x <= 0.) {
	/* use	Abramowitz & Stegun 6.4.7 "Reflection Formula"
	 *	psi(k, x) = (-1)^k psi(k, 1-x)	-  pi^{n+1} (d/dx)^n cot(x)
	 */
	if (x == (long)x) {
	    /* non-positive integer : +Inf or NaN depends on n */
	    for(j=0; j < m; j++) /* k = j + n : */
		ans[j] = ((j+n) % 2) ? ML_POSINF : ML_NAN;
	    return;
	}
	dpsifn(1. - x, n, /*kode = */ 1, m, ans, nz, ierr);
	/* ans[j] == (-1)^(k+1) / gamma(k+1) * psi(k, 1 - x)
	 *	     for j = 0:(m-1) ,	k = n + j
	 */

	/* Cheat for now: only work for	 m = 1, n in {0,1,2,3} : */
	if(m > 1 || n > 3) {/* doesn't happen for digamma() .. pentagamma() */
	    /* not yet implemented */
	    *ierr = 4; return;
	}
	x *= M_PI; /* pi * x */
	if (n == 0)
	    tt = cos(x)/sin(x);
	else if (n == 1)
	    tt = -1/pow(sin(x),2);
	else if (n == 2)
	    tt = 2*cos(x)/pow(sin(x),3);
	else if (n == 3)
	    tt = -2*(2*pow(cos(x),2) + 1)/pow(sin(x),4);
	else /* can not happen! */
	    tt = ML_NAN;
	/* end cheat */

	s = (n % 2) ? -1. : 1.;/* s = (-1)^n */
	/* t := pi^(n+1) * d_n(x) / gamma(n+1)	, where
	 *		   d_n(x) := (d/dx)^n cot(x)*/
	t1 = t2 = s = 1.;
	for(k=0, j=k-n; j < m; k++, j++, s = -s) {
	    /* k == n+j , s = (-1)^k */
	    t1 *= M_PI;/* t1 == pi^(k+1) */
	    if(k >= 2)
		t2 *= k;/* t2 == k! == gamma(k+1) */
	    if(j >= 0) /* by cheat above,  tt === d_k(x) */
		ans[j] = s*(ans[j] + t1/t2 * tt);
	}
	if (n == 0 && kode == 2)
	    ans[0] += xln;
	return;
    } /* x <= 0 */

    *nz = 0;
    mm = m;
    nx = imin2(-i1mach(15), i1mach(16));/* = 1021 */
    r1m5 = d1mach(5);
    r1m4 = d1mach(4) * 0.5;
    wdtol = fmax2(r1m4, 0.5e-18); /* 1.11e-16 */

    /* elim = approximate exponential over and underflow limit */

    elim = 2.302 * (nx * r1m5 - 3.0);/* = 700.6174... */
    xln = log(x);
    for(;;) {
	nn = n + mm - 1;
	fn = nn;
	t = (fn + 1) * xln;

	/* overflow and underflow test for small and large x */

	if (fabs(t) > elim) {
	    if (t <= 0.0) {
		*nz = 0;
		*ierr = 2;
		return;
	    }
	}
	else {
	    if (x < wdtol) {
		ans[0] = pow(x, -n-1.0);
		if (mm != 1) {
		    for(k = 1; k < mm ; k++)
			ans[k] = ans[k-1] / x;
		}
		if (n == 0 && kode == 2)
		    ans[0] += xln;
		return;
	    }

	    /* compute xmin and the number of terms of the series,  fln+1 */

	    rln = r1m5 * i1mach(14);
	    rln = fmin2(rln, 18.06);
	    fln = fmax2(rln, 3.0) - 3.0;
	    yint = 3.50 + 0.40 * fln;
	    slope = 0.21 + fln * (0.0006038 * fln + 0.008677);
	    xm = yint + slope * fn;
	    mx = (int)xm + 1;
	    xmin = mx;
	    if (n != 0) {
		xm = -2.302 * rln - fmin2(0.0, xln);
		arg = xm / n;
		arg = fmin2(0.0, arg);
		eps = exp(arg);
		xm = 1.0 - eps;
		if (fabs(arg) < 1.0e-3)
		    xm = -arg;
		fln = x * xm / eps;
		xm = xmin - x;
		if (xm > 7.0 && fln < 15.0)
		    break;
	    }
	    xdmy = x;
	    xdmln = xln;
	    xinc = 0.0;
	    if (x < xmin) {
		nx = (int)x;
		xinc = xmin - nx;
		xdmy = x + xinc;
		xdmln = log(xdmy);
	    }

	    /* generate w(n+mm-1, x) by the asymptotic expansion */

	    t = fn * xdmln;
	    t1 = xdmln + xdmln;
	    t2 = t + xdmln;
	    tk = fmax2(fabs(t), fmax2(fabs(t1), fabs(t2)));
	    if (tk <= elim)
		goto L10;
	}
	nz++;
	mm--;
	ans[mm] = 0.;
	if (mm == 0)
	    return;
    }
    nn = (int)fln + 1;
    np = n + 1;
    t1 = (n + 1) * xln;
    t = exp(-t1);
    s = t;
    den = x;
    for(i=1; i <= nn; i++) {
	den += 1.;
	trm[i] = pow(den, (double)-np);
	s += trm[i];
    }
    ans[0] = s;
    if (n == 0 && kode == 2)
	ans[0] = s + xln;

    if (mm != 1) { /* generate higher derivatives, j > n */

	tol = wdtol / 5.0;
	for(j = 1; j < mm; j++) {
	    t /= x;
	    s = t;
	    tols = t * tol;
	    den = x;
	    for(i=1; i <= nn; i++) {
		den += 1.;
		trm[i] /= den;
		s += trm[i];
		if (trm[i] < tols)
		    break;
	    }
	    ans[j] = s;
	}
    }
    return;

  L10:
    tss = exp(-t);
    tt = 0.5 / xdmy;
    t1 = tt;
    tst = wdtol * tt;
    if (nn != 0)
	t1 = tt + 1.0 / fn;
    rxsq = 1.0 / (xdmy * xdmy);
    ta = 0.5 * rxsq;
    t = (fn + 1) * ta;
    s = t * b[3];
    if (fabs(s) >= tst) {
	tk = 2.0;
	for(k = 4; k <= 22; k++) {
	    t = t * ((tk + fn + 1)/(tk + 1.0))*((tk + fn)/(tk + 2.0)) * rxsq;
	    trm[k] = t * b[k];
	    if (fabs(trm[k]) < tst)
		break;
	    s += trm[k];
	    tk += 2.;
	}
    }
    s = (s + t1) * tss;
    if (xinc != 0.0) {

	/* backward recur from xdmy to x */

	nx = (int)xinc;
	np = nn + 1;
	if (nx > nmax) {
	    *nz = 0;
	    *ierr = 3;
	    return;
	}
	else {
	    if (nn==0)
		goto L20;
	    xm = xinc - 1.0;
	    fx = x + xm;

	    /* this loop should not be changed. fx is accurate when x is small */
	    for(i = 1; i <= nx; i++) {
		trmr[i] = pow(fx, (double)-np);
		s += trmr[i];
		xm -= 1.;
		fx = x + xm;
	    }
	}
    }
    ans[mm-1] = s;
    if (fn == 0)
	goto L30;

    /* generate lower derivatives,  j < n+mm-1 */

    for(j = 2; j <= mm; j++) {
	fn--;
	tss *= xdmy;
	t1 = tt;
	if (fn!=0)
	    t1 = tt + 1.0 / fn;
	t = (fn + 1) * ta;
	s = t * b[3];
	if (fabs(s) >= tst) {
	    tk = 4 + fn;
	    for(k=4; k <= 22; k++) {
		trm[k] = trm[k] * (fn + 1) / tk;
		if (fabs(trm[k]) < tst)
		    break;
		s += trm[k];
		tk += 2.;
	    }
	}
	s = (s + t1) * tss;
	if (xinc != 0.0) {
	    if (fn == 0)
		goto L20;
	    xm = xinc - 1.0;
	    fx = x + xm;
	    for(i=1 ; i<=nx ; i++) {
		trmr[i] = trmr[i] * fx;
		s += trmr[i];
		xm -= 1.;
		fx = x + xm;
	    }
	}
	ans[mm - j] = s;
	if (fn == 0)
	    goto L30;
    }
    return;

  L20:
    for(i = 1; i <= nx; i++)
	s += 1. / (x + nx - i);

  L30:
    if (kode!=2)
	ans[0] = s - xdmln;
    else if (xdmy != x) {
	xq = xdmy / x;
	ans[0] = s - log(xq);
    }
    return;
} /* dpsifn() */

double psigamma(double x, double deriv)
{
    /* n-th derivative of psi(x);  e.g., psigamma(x,0) == digamma(x) */
    double ans;
    int nz, ierr, k, n;

    if(ISNAN(x))
	return x;
    deriv = floor(deriv + 0.5);
    n = (int)deriv;
    if(n > n_max) {
	MATHLIB_WARNING2(_("deriv = %d > %d (= n_max)\n"), n, n_max);
	return ML_NAN;
    }
    dpsifn(x, n, 1, 1, &ans, &nz, &ierr);
    if(ierr != 0) {
	errno = EDOM;
	return ML_NAN;
    }
    /* ans ==  A := (-1)^(n+1) / gamma(n+1) * psi(n, x) */
    ans = -ans; /* = (-1)^(0+1) * gamma(0+1) * A */
    for(k = 1; k <= n; k++)
	ans *= (-k);/* = (-1)^(k+1) * gamma(k+1) * A */
    return ans;/* = psi(n, x) */
}

double digamma(double x)
{
    double ans;
    int nz, ierr;
    if(ISNAN(x)) return x;

    dpsifn(x, 0, 1, 1, &ans, &nz, &ierr);
    if(ierr != 0) {
	errno = EDOM;
	return ML_NAN;
    }
    return -ans;
}

double trigamma(double x)
{
    double ans;
    int nz, ierr;
    if(ISNAN(x)) return x;
    dpsifn(x, 1, 1, 1, &ans, &nz, &ierr);
    if(ierr != 0) {
	errno = EDOM;
	return ML_NAN;
    }
    return ans;
}

double tetragamma(double x)
{
    double ans;
    int nz, ierr;
    if(ISNAN(x)) return x;
    dpsifn(x, 2, 1, 1, &ans, &nz, &ierr);
    if(ierr != 0) {
	errno = EDOM;
	return ML_NAN;
    }
    return -2.0 * ans;
}

double pentagamma(double x)
{
    double ans;
    int nz, ierr;
    if(ISNAN(x)) return x;
    dpsifn(x, 3, 1, 1, &ans, &nz, &ierr);
    if(ierr != 0) {
	errno = EDOM;
	return ML_NAN;
    }
    return 6.0 * ans;
}


