/*  debyer -- program for calculation of diffration patterns
 *  Copyright (C) 2009 Marcin Wojdyr
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
 *  $Id$
 *
 *  Functions for access to the following data:
 *   - periodic system of elements
 *   - x-ray scattering factors (based on IT92 data)
 *   - neutron scattering lengths and cross sections (NN92)
 *
 *  The data in tables is taken from sources cited below.
 */

#ifndef DEBYER_ATOMTABLES_H_
#define DEBYER_ATOMTABLES_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* element of periodic system of elements */
typedef struct
{
    int Z; // atomic number
    const char* symbol;
    const char* name;
    float mass;
}
t_pse;

/* Finds element with the symbol that matches `label' in periodic table.
 * Match is case insensitive. Trailing non-alpha characters in label are
 * ignored.
 * Returns NULL if not found.
 */
const t_pse *find_in_pse(const char *label);

/* Finds element with atomic number Z. Returns NULL if not found.
 */
const t_pse *find_Z_in_pse(int Z);


/* Coefficients for approximation to the scattering factor called IT92.
 * AFAIR the data was taken from ObjCryst. It can be also found
 * in cctbx and in old atominfo program, and originaly comes from the paper
 * publication cited below.
 */
/*
    International Tables for Crystallography
      Volume C
      Mathematical, Physical and Chemical Tables
      Edited by A.J.C. Wilson
      Kluwer Academic Publishers
      Dordrecht/Boston/London
      1992

    Table 6.1.1.4 (pp. 500-502)
      Coefficients for analytical approximation to the scattering factors
      of Tables 6.1.1.1 and 6.1.1.3

    [ Table 6.1.1.4 is a reprint of Table 2.2B, pp. 99-101,
      International Tables for X-ray Crystallography, Volume IV,
      The Kynoch Press: Birmingham, England, 1974.
      There is just one difference, see "Tl3+".
    ]
 */
typedef struct
{
    const char* symbol;
    float a[4], b[4], c;
}
t_it92_coeff;

/* Finds record in IT92 table with the symbol that matches `label'. */
const t_it92_coeff *find_in_it92(const char *label);

/* Calculate scattering factor using coefficients p.
 * stol2 = (sin(theta)/lambda)^2
 */
double calculate_it92_factor(const t_it92_coeff *p, double stol2);


/*  Neutron bound scattering lengths & cross-section

Data from: http://www.ncnr.nist.gov/resources/n-lengths/list.html

All of this data was taken from the Special Feature section of neutron
scattering lengths and cross sections of the elements and their isotopes in
Neutron News, Vol. 3, No. 3, 1992, pp. 29-37.
*/

typedef struct
{
    const char *symbol;
    float bond_coh_scatt_length;
    float bond_coh_scatt_length_imag;
    float abs_cross_sect; /* for 2200 m/s neutrons*/
}
t_nn92_record;

const t_nn92_record *find_in_nn92(const char *label);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DEBYER_ATOMTABLES_H_ */
