// Author: Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: powdifpat.cpp 553 2009-09-20 23:54:10Z wojdyr $

#include "ceria.h"

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <algorithm>
//#include <iostream> // debug
extern "C" {
#include <sglite/sglite.h> // ParseHallSymbol in set_space_group()
}

using namespace std;

Pos apply_seitz(Pos const& p0, SeitzMatrix const& s)
{
    Pos p;
    p.x = s.R[0]*p0.x + s.R[1]*p0.y + s.R[2]*p0.z + s.T[0]/12.;
    p.y = s.R[3]*p0.x + s.R[4]*p0.y + s.R[5]*p0.z + s.T[1]/12.;
    p.z = s.R[6]*p0.x + s.R[7]*p0.y + s.R[8]*p0.z + s.T[2]/12.;
    return p;
}


string fullHM(const SpaceGroupSetting *sgs)
{
    if (sgs == NULL)
        return "";
    else if (sgs->ext == 0)
        return sgs->HM;
    else
        return string(sgs->HM) + ":" + sgs->ext;
}

bool check_symmetric_hkl(const SgOps &sg_ops, const Miller &p1,
                                              const Miller &p2)
{
    for (size_t i = 0; i < sg_ops.seitz.size(); ++i)
    {
        const int* R = sg_ops.seitz[i].R;
        int h = R[0] * p1.h + R[3] * p1.k + R[6] * p1.l;
        int k = R[1] * p1.h + R[4] * p1.k + R[7] * p1.l;
        int l = R[2] * p1.h + R[5] * p1.k + R[8] * p1.l;
        if (h == p2.h && k == p2.k && l == p2.l)
            return  true;
        // we assume Friedel symmetry here
        if (h == -p2.h && k == -p2.k && l == -p2.l)
            return true;
    }
    return false;
}

bool is_position_empty(const vector<Pos>& pp, const Pos& p)
{
    const double eps = 0.01;
    for (vector<Pos>::const_iterator j = pp.begin(); j != pp.end(); ++j) {
        if ((fabs(p.x - j->x) < eps || fabs(p.x - j->x) > 1 - eps) &&
            (fabs(p.y - j->y) < eps || fabs(p.y - j->y) > 1 - eps) &&
            (fabs(p.z - j->z) < eps || fabs(p.z - j->z) > 1 - eps))
            return false;
    }
    return true;
}

double mod1(double x) { return x - floor(x); }

void add_symmetric_images(Atom& a, const SgOps& sg_ops)
{
    assert(a.pos.size() == 1);
    // iterate over translation, Seitz matrices and inversion
    for (size_t nt = 0; nt != sg_ops.tr.size(); ++nt) {
        const TransVec& t = sg_ops.tr[nt];
        for (size_t ns = 0; ns != sg_ops.seitz.size(); ++ns) {
            Pos ps = apply_seitz(a.pos[0], sg_ops.seitz[ns]);
            Pos p = { mod1(ps.x + t.x/12.),
                      mod1(ps.y + t.y/12.),
                      mod1(ps.z + t.z/12.) };
            if (is_position_empty(a.pos, p)) {
                a.pos.push_back(p);
            }
            if (sg_ops.inv) {
                Pos p2 = { mod1(-ps.x + t.x/12.),
                           mod1(-ps.y + t.y/12.),
                           mod1(-ps.z + t.z/12.) };
                if (is_position_empty(a.pos, p2)) {
                    a.pos.push_back(p2);
                }
            }
        }
    }
}

int parse_atoms(const char* s, Crystal& cr)
{
    int line_with_error = -1;

    int atom_count = 0;
    // to avoid not necessary copying, don't use atoms.clear()
    for (int line_nr = 1; ; ++line_nr) {
        // skip whitespace
        while(isspace(*s) && *s != '\n')
            ++s;
        if (*s == '\n')
            continue;
        if (*s == '\0')
            break;

        // usually the atom is not changed, so we first parse data
        // into a new struct Atom, and if it is different we copy it
        // and do calculations
        Atom a;

        // parse symbol
        const char* word_end = s;
        while (isalnum(*word_end))
            ++word_end;
        int symbol_len = word_end - s;
        if (symbol_len == 0 || symbol_len >= 8) {
            line_with_error = line_nr;
            break;
        }
        memcpy(a.symbol, s, symbol_len);
        a.symbol[symbol_len] = '\0';
        s = word_end;

        // parse x, y, z
        char *endptr;
        a.pos[0].x = strtod(s, &endptr);
        s = endptr;
        a.pos[0].y = strtod(s, &endptr);
        s = endptr;
        a.pos[0].z = strtod(s, &endptr);

        // check if the numbers were parsed
        if (endptr == s || // one or more numbers were not parsed
            (!isspace(*endptr) && *endptr != '\0')) // e.g. "Si 0 0 0foo"
        {
            line_with_error = line_nr;
            break;
        }

        s = endptr;

        // if there is more than 4 words in the line, ignore the extra words
        while (*s != '\n' && *s != '\0')
            ++s;
        if (*s == '\n')
            ++s;

        // check if the atom needs to be copied, and copy it if necessary
        if (atom_count == (int) cr.atoms.size()) {
            a.xray_sf = find_in_it92(a.symbol);
            a.neutron_sf = find_in_nn92(a.symbol);
            cr.atoms.push_back(a);
            add_symmetric_images(cr.atoms[atom_count], cr.sg_ops);
        }
        else {
            Atom &b = cr.atoms[atom_count];
            if (strcmp(a.symbol, b.symbol) != 0) {
                memcpy(b.symbol, a.symbol, 8);
                b.xray_sf = find_in_it92(b.symbol);
                b.neutron_sf = find_in_nn92(b.symbol);
            }
            if (a.pos[0].x != b.pos[0].x
                    || a.pos[0].y != b.pos[0].y
                    || a.pos[0].z != b.pos[0].z) {
                b.pos.resize(1);
                b.pos[0] = a.pos[0];
                add_symmetric_images(b, cr.sg_ops);
            }
        }
        ++atom_count;
    }
    // vector cr.atoms may be longer than necessary
    cr.atoms.resize(atom_count);
    return line_with_error;
}


void PlanesWithSameD::add(Miller const& hkl, const SgOps& sg_ops)
{
    for (vector<Plane>::iterator i = planes.begin(); i != planes.end(); ++i) {
        if (check_symmetric_hkl(sg_ops, *i, hkl)) {
            i->multiplicity++;
            return;
        }
    }
    // equivalent plane not found
    planes.push_back(Plane(hkl));
}

Crystal::Crystal()
    : uc(NULL)
{
    atoms.reserve(16);
}

Crystal::~Crystal()
{
    delete uc;
}

// in the same order as in enum CrystalSystem
const char *CrystalSystemNames[] = {
    "Undefined", // 0
    NULL,
    "Triclinic", // 2
    "Monoclinic",
    "Orthorhombic",
    "Tetragonal",
    "Trigonal",
    "Hexagonal",
    "Cubic" // 8
};

const char* get_crystal_system_name(CrystalSystem xs)
{
    return CrystalSystemNames[xs];
}

CrystalSystem get_crystal_system(int space_group)
{
    if (space_group <= 2)
        return TriclinicSystem;
    else if (space_group <= 15)
        return MonoclinicSystem;
    else if (space_group <= 74)
        return OrthorhombicSystem;
    else if (space_group <= 142)
        return TetragonalSystem;
    else if (space_group <= 167)
        return TrigonalSystem;
    else if (space_group <= 194)
        return HexagonalSystem;
    else
        return CubicSystem;
}

int get_sg_order(const SgOps& sg_ops)
{
    return sg_ops.tr.size() * sg_ops.seitz.size() * (sg_ops.inv ? 2 : 1);
}

char parse_sg_extension(const char *symbol, char *qualif)
{
    if (symbol == NULL || *symbol == '\0') {
        if (qualif != NULL)
            qualif[0] = '\0';
        return 0;
    }
    char ext = 0;
    while (isspace(*symbol) || *symbol == ':')
        ++symbol;
    if (isdigit(*symbol) || *symbol == 'R' || *symbol == 'H') {
        ext = *symbol;
        ++symbol;
        while (isspace(*symbol) || *symbol == ':')
            ++symbol;
    }
    if (qualif != NULL) {
        strncpy(qualif, symbol, 4);
        qualif[4] = '\0';
    }
    return ext;
}

const SpaceGroupSetting* parse_hm_or_hall(const char *symbol)
{
    // copy and 'normalize' symbol (up to ':') to table s
    char s[32];
    for (int i = 0; i < 32; ++i) {
        if (*symbol == '\0' || *symbol == ':') {
            s[i] = '\0';
            break;
        }
        else if (isspace(*symbol)) {
            s[i] = ' ';
            ++symbol;
            while (isspace(*symbol))
                ++symbol;
        }
        else {
            // In HM symbols, first character is upper case letter.
            // In Hall symbols, the second character is upper case.
            // The first and second chars are either upper or ' ' or '-'.
            // The rest of alpha chars is lower case. 
            s[i] = (i < 2 ? toupper(*symbol) : tolower(*symbol));
            ++symbol;
        }
    }
    // now *symbol is either '\0' or ':'
    for (const SpaceGroupSetting *p = space_group_settings;
                                                p->sgnumber != 0; ++p) {
        if (strcmp(p->HM, s) == 0) {
            if (*symbol == ':') {
                char ext = parse_sg_extension(symbol+1, NULL);
                while (p->ext != ext) {
                    ++p;
                    if (strcmp(p->HM, s) != 0) // full match not found
                        return NULL;
                }
            }
            return p;
        }
        else if (strcmp(p->Hall + (p->Hall[0] == ' ' ? 1 : 0), s) == 0) {
            return p;
        }
    }
    return NULL;
}

const SpaceGroupSetting* find_first_sg_with_number(int sgn)
{
    for (const SpaceGroupSetting *p = space_group_settings; p->sgnumber!=0; ++p)
        if (p->sgnumber == sgn)
            return p;
    return NULL;
}

const SpaceGroupSetting* find_space_group_setting(int sgn, const char *setting)
{
    char qualif[5];
    char ext = parse_sg_extension(setting, qualif);
    const SpaceGroupSetting *p = find_first_sg_with_number(sgn);
    if (p == NULL)
        return NULL;
    while (p->ext != ext || strcmp(p->qualif, qualif) != 0) {
        ++p;
        if (p->sgnumber != sgn) // not found
            return NULL;
    }
    return p;
}

const SpaceGroupSetting* parse_any_sg_symbol(const char *symbol)
{
    if (symbol == NULL)
        return NULL;
    while (isspace(*symbol))
        ++symbol;
    if (isdigit(*symbol)) {
        const char* colon = strchr(symbol, ':');
        int sgn = strtol(symbol, NULL, 10);
        return find_space_group_setting(sgn, colon);
    }
    else {
        return parse_hm_or_hall(symbol);
    }
}

void Crystal::set_space_group(const SpaceGroupSetting* sgs_)
{
    sgs = sgs_;
    sg_ops.seitz.clear();
    sg_ops.tr.clear();
    sg_ops.inv = false;
    if (sgs == NULL)
        return;
    T_SgOps sglite_ops;
    ClrSgError();
    ResetSgOps(&sglite_ops);
    ParseHallSymbol(sgs->Hall, &sglite_ops, PHSymOptPedantic);
    if (SgError)
        fprintf(stderr, "%s\n", SgError);
    //DumpSgOps(&sg_ops->s, stdout);
    for (int i = 0; i != sglite_ops.nSMx; ++i) {
        SeitzMatrix sm;
        for (int j = 0; j != 9; ++j)
            sm.R[j] = sglite_ops.SMx[i].s.R[j];
        for (int j = 0; j != 3; ++j)
            sm.T[j] = sglite_ops.SMx[i].s.T[j];
        sg_ops.seitz.push_back(sm);
    }
    // TODO:
    // tr can be set directly from sgs->Hall[1]:
    //    A ('0,0,0', '0,1/2,1/2')
    //    B ('0,0,0', '1/2,0,1/2')
    //    C ('0,0,0', '1/2,1/2,0')
    //    F ('0,0,0', '0,1/2,1/2', '1/2,0,1/2', '1/2,1/2,0')
    //    I ('0,0,0', '1/2,1/2,1/2')
    //    P ('0,0,0',)
    //    R ('0,0,0', '2/3,1/3,1/3', '1/3,2/3,2/3')
    for (int i = 0; i != sglite_ops.nLTr; ++i) {
        const int *v = sglite_ops.LTr[i].v;
        TransVec t = { v[0], v[1], v[2] };
        sg_ops.tr.push_back(t);
    }
    for (int i = 0; i != 3; ++i)
        sg_ops.inv_t[i] = sglite_ops.InvT[i];
    sg_ops.inv = (sglite_ops.fInv == 2);
}

// returns true if exists t in sg_ops.tr, such that: h*(t+T) != n
// used by is_sys_absent()
bool has_nonunit_tr(const SgOps& sg_ops, const int* T, int h, int k, int l)
{
    for (vector<TransVec>::const_iterator t = sg_ops.tr.begin();
                                                t != sg_ops.tr.end(); ++t)
        if (((T[0]+t->x) * h + (T[1]+t->y) * k + (T[2]+t->z) * l) % 12 != 0)
            return true;
    return false;
}

bool is_sys_absent(const SgOps& sg_ops, int h, int k, int l)
{
    for (size_t i = 0; i < sg_ops.seitz.size(); ++i) {
        const int* R = sg_ops.seitz[i].R;
        const int* T = sg_ops.seitz[i].T;
        int M[3] = { h * R[0] + k * R[3] + l * R[6],
                     h * R[1] + k * R[4] + l * R[7],
                     h * R[2] + k * R[5] + l * R[8] };
        if (h == M[0] && k == M[1] && l == M[2]) {
            if (has_nonunit_tr(sg_ops, T, h, k, l))
                return true;
        }
        else if (h == -M[0] && k == -M[1] && l == -M[2] && sg_ops.inv) {
            int ts[3] = { sg_ops.inv_t[0] - T[0],
                          sg_ops.inv_t[1] - T[1],
                          sg_ops.inv_t[2] - T[2] };
            if (has_nonunit_tr(sg_ops, ts, h, k, l))
                return true;
        }
    }
    return false;
}


// helper to generate sequence 0, 1, -1, 2, -2, 3, ...
static int inc_neg(int h) { return h > 0 ? -h : -h+1; }

void Crystal::generate_reflections(double min_d)
{
    bp.clear();
    UnitCell reciprocal = uc->get_reciprocal();

    // set upper limit for iteration of Miller indices
    // TODO: smarter algorithm, like in uctbx::unit_cell::max_miller_indices()
    int max_h = 20;
    int max_k = 20;
    int max_l = 20;
    if (fabs(uc->alpha - M_PI/2) < 1e-9 && fabs(uc->beta - M_PI/2) < 1e-9 &&
                                          fabs(uc->gamma - M_PI/2) < 1e-9) {
        max_h = (int) (uc->a / min_d);
        max_k = (int) (uc->b / min_d);
        max_l = (int) (uc->c / min_d);
    }

    for (int h = 0; h != max_h+1; h = inc_neg(h))
        for (int k = 0; k != max_k+1; k = inc_neg(k))
            for (int l = (h==0 && k==0 ? 1 : 0); l != max_l+1; l = inc_neg(l)) {
                double d = 1 / reciprocal.calculate_distance(h, k, l);
                //double d = uc->calculate_d(h, k, l); // the same
                if (d < min_d)
                    continue;

                // check for systematic absence
                if (is_sys_absent(sg_ops, h, k, l))
                    continue;

                Miller hkl = { h, k, l };
                bool found = false;
                for (vector<PlanesWithSameD>::iterator i = bp.begin();
                        i != bp.end(); ++i) {
                    if (fabs(d - i->d) < 1e-9) {
                        i->add(hkl, sg_ops);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    PlanesWithSameD new_p;
                    new_p.planes.push_back(Plane(hkl));
                    new_p.d = d;
                    new_p.lpf = 0.;
                    new_p.intensity = 0.;
                    new_p.enabled = true;
                    bp.push_back(new_p);
                }
            }
    sort(bp.begin(), bp.end());
}


// stol = sin(theta)/lambda
void calculate_intensity(Plane& p, const vector<Atom>& atoms, double stol)
{
    // calculating F_hkl, (Pecharsky & Zavalij, eq. (2.89) and (2.103))
    // assuming population g = 1
    // assuming temperature factor t = 1
    double F_real = 0.;
    double F_img = 0;
    for (vector<Atom>::const_iterator i = atoms.begin(); i != atoms.end(); ++i){
        double f = 1.;
        if (i->xray_sf)
            f = calculate_it92_factor(i->xray_sf, stol*stol);
        for (vector<Pos>::const_iterator j = i->pos.begin();
                                                j != i->pos.end(); ++j) {
            double hx = p.h * j->x + p.k * j->y + p.l * j->z;
            F_real += f * cos(2*M_PI * hx);
            F_img += f * sin(2*M_PI * hx);
        }
    }
    p.F2 = F_real*F_real + F_img*F_img;
}

void calculate_total_intensity(PlanesWithSameD &bp, const vector<Atom>& atoms,
                               double lambda)
{
    double stol = 1. / (2 * bp.d); // == sin(T) / lambda
    double T = asin(stol * lambda); // theta

    // for x-rays, we assume K=0.5 and
    // LP = (1 + cos(2T)^2) / (cos(T) sin(T)^2)
    //  (Pecharsky & Zavalij, eq. (2.70), p. 192)
    bp.lpf = (1 + cos(2*T)*cos(2*T)) / (cos(T)*sin(T)*sin(T));

    double t = 0;
    for (vector<Plane>::iterator i = bp.planes.begin();
                                                i != bp.planes.end(); ++i) {
        calculate_intensity(*i, atoms, stol);
        t += i->multiplicity * i->F2;
    }
    bp.intensity = bp.lpf * t;
}

void Crystal::calculate_intensities(double lambda)
{
    if (atoms.empty())
        return;
    for (vector<PlanesWithSameD>::iterator i = bp.begin(); i != bp.end(); ++i)
        calculate_total_intensity(*i, atoms, lambda);
}

double UnitCell::calculate_V() const
{
    // Giacovazzo p.62
    double cosA = cos(alpha), cosB = cos(beta), cosG = cos(gamma);
    double t = 1 - cosA*cosA - cosB*cosB - cosG*cosG + 2*cosA*cosB*cosG;
    return a*b*c * sqrt(t);
}

double UnitCell::calculate_d(int h, int k, int l) const
{
    double sinA=sin(alpha), sinB=sin(beta), sinG=sin(gamma),
           cosA=cos(alpha), cosB=cos(beta), cosG=cos(gamma);
    return
      sqrt((1 - cosA*cosA - cosB*cosB - cosG*cosG + 2*cosA*cosB*cosG)
              / (  (h/a*sinA) * (h/a*sinA)  //(h/a*sinA)^2
                 + (k/b*sinB) * (k/b*sinB)  //(k/b*sinB)^2
                 + (l/c*sinG) * (l/c*sinG)  //(l/c*sinG)^2
                 + 2*h*l/a/c*(cosA*cosG-cosB)
                 + 2*h*k/a/b*(cosA*cosB-cosG)
                 + 2*k*l/b/c*(cosB*cosG-cosA)
                )
          );
}

//     [       1/a               0        0   ]
// M = [  -cosG/(a sinG)    1/(b sinG)    0   ]
//     [     a*cosB*         b*cosA*      c*  ]
//
// where A is alpha, B is beta, G is gamma, * means reciprocal space.
// (Giacovazzo, p.68)
void UnitCell::set_M()
{
    double sinA=sin(alpha), sinB=sin(beta), sinG=sin(gamma),
           cosA=cos(alpha), cosB=cos(beta), cosG=cos(gamma);
    M[0][0] = 1/a;
    M[0][1] = 0.;
    M[0][2] = 0.;
    M[1][0] = -cosG/(a*sinG);
    M[1][1] = 1/(b*sinG);
    M[1][2] = 0.;
    M[2][0] = b * c * sinA / V // a*
              * (cosA*cosG-cosB) / (sinA*sinG); //cosB*
    M[2][1] = a * c * sinB / V // b*
              * (cosB*cosG-cosA) / (sinB*sinG); //cosA*
    M[2][2] = a * b * sinG / V; // c*

    M_1[0][0] = a;
    M_1[0][1] = 0;
    M_1[0][2] = 0;
    M_1[1][0] = b * cosG;
    M_1[1][1] = b * sinG;
    M_1[1][2] = 0;
    M_1[2][0] = c * cosB;
    M_1[2][1] = -c * sinB * (cosB*cosG-cosA) / (sinB*sinG);
    M_1[2][2] = 1 / M[2][2];
}

// returns UnitCell reciprocal to this, i.e. that has parameters a*, b*, ...
// (Giacovazzo, p. 64)
UnitCell UnitCell::get_reciprocal() const
{
    double ar = b * c * sin(alpha) / V;
    double br = a * c * sin(beta) / V;
    double cr = a * b * sin(gamma) / V;
    double cosAr = (cos(beta)*cos(gamma)-cos(alpha)) / (sin(beta)*sin(gamma));
    double cosBr = (cos(alpha)*cos(gamma)-cos(beta)) / (sin(alpha)*sin(gamma));
    double cosGr = (cos(alpha)*cos(beta)-cos(gamma)) / (sin(alpha)*sin(beta));
    return UnitCell(ar, br, cr, acos(cosAr), acos(cosBr), acos(cosGr));
}

// returns 1/|v|, where v = M * [h k l]; 
double UnitCell::calculate_distance(double h, double k, double l) const
{
    double v2 = 0.;
    for (int i = 0; i != 3; ++i) {
        double t = h * M_1[0][i] + k * M_1[1][i] + l * M_1[2][i];
        v2 += t*t;
    }
    return sqrt(v2);
}

const Anode anodes[] = {
    { "Cu", 1.54056, 1.54439 },
    { "Cr", 2.28970, 2.29361 },
    { "Fe", 1.93604, 1.93998 },
    { "Mo", 0.70930, 0.71359 },
    { "Ag", 0.55941, 0.56380 },
    { "Co", 1.78901, 1.79290 },
    { NULL, 0, 0 }
};

// space_group_settings was generated by cctbx using the following code:

//from cctbx import sgtbx
//
//for s in sgtbx.space_group_symbol_iterator():
//    ext_ = s.extension()
//    ext = ("'%s'" % ext_ if (ext_ and ext_ != '\0') else "0")
//    qualif = '"%s"' % s.qualifier()
//    hm = '"%s",' % s.hermann_mauguin()
//    hall = '"%s"' % s.hall().replace('"', r'\"')
//    print '{ %3d, %3s, %6s, %-13s %-17s },' % (
//            s.number(), ext, qualif, hm, hall)

const SpaceGroupSetting space_group_settings[] = {
{   1,   0,     "", "P 1",        " P 1"            },
{   2,   0,     "", "P -1",       "-P 1"            },
{   3,   0,    "b", "P 1 2 1",    " P 2y"           },
{   3,   0,    "c", "P 1 1 2",    " P 2"            },
{   3,   0,    "a", "P 2 1 1",    " P 2x"           },
{   4,   0,    "b", "P 1 21 1",   " P 2yb"          },
{   4,   0,    "c", "P 1 1 21",   " P 2c"           },
{   4,   0,    "a", "P 21 1 1",   " P 2xa"          },
{   5,   0,   "b1", "C 1 2 1",    " C 2y"           },
{   5,   0,   "b2", "A 1 2 1",    " A 2y"           },
{   5,   0,   "b3", "I 1 2 1",    " I 2y"           },
{   5,   0,   "c1", "A 1 1 2",    " A 2"            },
{   5,   0,   "c2", "B 1 1 2",    " B 2"            },
{   5,   0,   "c3", "I 1 1 2",    " I 2"            },
{   5,   0,   "a1", "B 2 1 1",    " B 2x"           },
{   5,   0,   "a2", "C 2 1 1",    " C 2x"           },
{   5,   0,   "a3", "I 2 1 1",    " I 2x"           },
{   6,   0,    "b", "P 1 m 1",    " P -2y"          },
{   6,   0,    "c", "P 1 1 m",    " P -2"           },
{   6,   0,    "a", "P m 1 1",    " P -2x"          },
{   7,   0,   "b1", "P 1 c 1",    " P -2yc"         },
{   7,   0,   "b2", "P 1 n 1",    " P -2yac"        },
{   7,   0,   "b3", "P 1 a 1",    " P -2ya"         },
{   7,   0,   "c1", "P 1 1 a",    " P -2a"          },
{   7,   0,   "c2", "P 1 1 n",    " P -2ab"         },
{   7,   0,   "c3", "P 1 1 b",    " P -2b"          },
{   7,   0,   "a1", "P b 1 1",    " P -2xb"         },
{   7,   0,   "a2", "P n 1 1",    " P -2xbc"        },
{   7,   0,   "a3", "P c 1 1",    " P -2xc"         },
{   8,   0,   "b1", "C 1 m 1",    " C -2y"          },
{   8,   0,   "b2", "A 1 m 1",    " A -2y"          },
{   8,   0,   "b3", "I 1 m 1",    " I -2y"          },
{   8,   0,   "c1", "A 1 1 m",    " A -2"           },
{   8,   0,   "c2", "B 1 1 m",    " B -2"           },
{   8,   0,   "c3", "I 1 1 m",    " I -2"           },
{   8,   0,   "a1", "B m 1 1",    " B -2x"          },
{   8,   0,   "a2", "C m 1 1",    " C -2x"          },
{   8,   0,   "a3", "I m 1 1",    " I -2x"          },
{   9,   0,   "b1", "C 1 c 1",    " C -2yc"         },
{   9,   0,   "b2", "A 1 n 1",    " A -2yab"        },
{   9,   0,   "b3", "I 1 a 1",    " I -2ya"         },
{   9,   0,  "-b1", "A 1 a 1",    " A -2ya"         },
{   9,   0,  "-b2", "C 1 n 1",    " C -2yac"        },
{   9,   0,  "-b3", "I 1 c 1",    " I -2yc"         },
{   9,   0,   "c1", "A 1 1 a",    " A -2a"          },
{   9,   0,   "c2", "B 1 1 n",    " B -2ab"         },
{   9,   0,   "c3", "I 1 1 b",    " I -2b"          },
{   9,   0,  "-c1", "B 1 1 b",    " B -2b"          },
{   9,   0,  "-c2", "A 1 1 n",    " A -2ab"         },
{   9,   0,  "-c3", "I 1 1 a",    " I -2a"          },
{   9,   0,   "a1", "B b 1 1",    " B -2xb"         },
{   9,   0,   "a2", "C n 1 1",    " C -2xac"        },
{   9,   0,   "a3", "I c 1 1",    " I -2xc"         },
{   9,   0,  "-a1", "C c 1 1",    " C -2xc"         },
{   9,   0,  "-a2", "B n 1 1",    " B -2xab"        },
{   9,   0,  "-a3", "I b 1 1",    " I -2xb"         },
{  10,   0,    "b", "P 1 2/m 1",  "-P 2y"           },
{  10,   0,    "c", "P 1 1 2/m",  "-P 2"            },
{  10,   0,    "a", "P 2/m 1 1",  "-P 2x"           },
{  11,   0,    "b", "P 1 21/m 1", "-P 2yb"          },
{  11,   0,    "c", "P 1 1 21/m", "-P 2c"           },
{  11,   0,    "a", "P 21/m 1 1", "-P 2xa"          },
{  12,   0,   "b1", "C 1 2/m 1",  "-C 2y"           },
{  12,   0,   "b2", "A 1 2/m 1",  "-A 2y"           },
{  12,   0,   "b3", "I 1 2/m 1",  "-I 2y"           },
{  12,   0,   "c1", "A 1 1 2/m",  "-A 2"            },
{  12,   0,   "c2", "B 1 1 2/m",  "-B 2"            },
{  12,   0,   "c3", "I 1 1 2/m",  "-I 2"            },
{  12,   0,   "a1", "B 2/m 1 1",  "-B 2x"           },
{  12,   0,   "a2", "C 2/m 1 1",  "-C 2x"           },
{  12,   0,   "a3", "I 2/m 1 1",  "-I 2x"           },
{  13,   0,   "b1", "P 1 2/c 1",  "-P 2yc"          },
{  13,   0,   "b2", "P 1 2/n 1",  "-P 2yac"         },
{  13,   0,   "b3", "P 1 2/a 1",  "-P 2ya"          },
{  13,   0,   "c1", "P 1 1 2/a",  "-P 2a"           },
{  13,   0,   "c2", "P 1 1 2/n",  "-P 2ab"          },
{  13,   0,   "c3", "P 1 1 2/b",  "-P 2b"           },
{  13,   0,   "a1", "P 2/b 1 1",  "-P 2xb"          },
{  13,   0,   "a2", "P 2/n 1 1",  "-P 2xbc"         },
{  13,   0,   "a3", "P 2/c 1 1",  "-P 2xc"          },
{  14,   0,   "b1", "P 1 21/c 1", "-P 2ybc"         },
{  14,   0,   "b2", "P 1 21/n 1", "-P 2yn"          },
{  14,   0,   "b3", "P 1 21/a 1", "-P 2yab"         },
{  14,   0,   "c1", "P 1 1 21/a", "-P 2ac"          },
{  14,   0,   "c2", "P 1 1 21/n", "-P 2n"           },
{  14,   0,   "c3", "P 1 1 21/b", "-P 2bc"          },
{  14,   0,   "a1", "P 21/b 1 1", "-P 2xab"         },
{  14,   0,   "a2", "P 21/n 1 1", "-P 2xn"          },
{  14,   0,   "a3", "P 21/c 1 1", "-P 2xac"         },
{  15,   0,   "b1", "C 1 2/c 1",  "-C 2yc"          },
{  15,   0,   "b2", "A 1 2/n 1",  "-A 2yab"         },
{  15,   0,   "b3", "I 1 2/a 1",  "-I 2ya"          },
{  15,   0,  "-b1", "A 1 2/a 1",  "-A 2ya"          },
{  15,   0,  "-b2", "C 1 2/n 1",  "-C 2yac"         },
{  15,   0,  "-b3", "I 1 2/c 1",  "-I 2yc"          },
{  15,   0,   "c1", "A 1 1 2/a",  "-A 2a"           },
{  15,   0,   "c2", "B 1 1 2/n",  "-B 2ab"          },
{  15,   0,   "c3", "I 1 1 2/b",  "-I 2b"           },
{  15,   0,  "-c1", "B 1 1 2/b",  "-B 2b"           },
{  15,   0,  "-c2", "A 1 1 2/n",  "-A 2ab"          },
{  15,   0,  "-c3", "I 1 1 2/a",  "-I 2a"           },
{  15,   0,   "a1", "B 2/b 1 1",  "-B 2xb"          },
{  15,   0,   "a2", "C 2/n 1 1",  "-C 2xac"         },
{  15,   0,   "a3", "I 2/c 1 1",  "-I 2xc"          },
{  15,   0,  "-a1", "C 2/c 1 1",  "-C 2xc"          },
{  15,   0,  "-a2", "B 2/n 1 1",  "-B 2xab"         },
{  15,   0,  "-a3", "I 2/b 1 1",  "-I 2xb"          },
{  16,   0,     "", "P 2 2 2",    " P 2 2"          },
{  17,   0,     "", "P 2 2 21",   " P 2c 2"         },
{  17,   0,  "cab", "P 21 2 2",   " P 2a 2a"        },
{  17,   0,  "bca", "P 2 21 2",   " P 2 2b"         },
{  18,   0,     "", "P 21 21 2",  " P 2 2ab"        },
{  18,   0,  "cab", "P 2 21 21",  " P 2bc 2"        },
{  18,   0,  "bca", "P 21 2 21",  " P 2ac 2ac"      },
{  19,   0,     "", "P 21 21 21", " P 2ac 2ab"      },
{  20,   0,     "", "C 2 2 21",   " C 2c 2"         },
{  20,   0,  "cab", "A 21 2 2",   " A 2a 2a"        },
{  20,   0,  "bca", "B 2 21 2",   " B 2 2b"         },
{  21,   0,     "", "C 2 2 2",    " C 2 2"          },
{  21,   0,  "cab", "A 2 2 2",    " A 2 2"          },
{  21,   0,  "bca", "B 2 2 2",    " B 2 2"          },
{  22,   0,     "", "F 2 2 2",    " F 2 2"          },
{  23,   0,     "", "I 2 2 2",    " I 2 2"          },
{  24,   0,     "", "I 21 21 21", " I 2b 2c"        },
{  25,   0,     "", "P m m 2",    " P 2 -2"         },
{  25,   0,  "cab", "P 2 m m",    " P -2 2"         },
{  25,   0,  "bca", "P m 2 m",    " P -2 -2"        },
{  26,   0,     "", "P m c 21",   " P 2c -2"        },
{  26,   0, "ba-c", "P c m 21",   " P 2c -2c"       },
{  26,   0,  "cab", "P 21 m a",   " P -2a 2a"       },
{  26,   0, "-cba", "P 21 a m",   " P -2 2a"        },
{  26,   0,  "bca", "P b 21 m",   " P -2 -2b"       },
{  26,   0, "a-cb", "P m 21 b",   " P -2b -2"       },
{  27,   0,     "", "P c c 2",    " P 2 -2c"        },
{  27,   0,  "cab", "P 2 a a",    " P -2a 2"        },
{  27,   0,  "bca", "P b 2 b",    " P -2b -2b"      },
{  28,   0,     "", "P m a 2",    " P 2 -2a"        },
{  28,   0, "ba-c", "P b m 2",    " P 2 -2b"        },
{  28,   0,  "cab", "P 2 m b",    " P -2b 2"        },
{  28,   0, "-cba", "P 2 c m",    " P -2c 2"        },
{  28,   0,  "bca", "P c 2 m",    " P -2c -2c"      },
{  28,   0, "a-cb", "P m 2 a",    " P -2a -2a"      },
{  29,   0,     "", "P c a 21",   " P 2c -2ac"      },
{  29,   0, "ba-c", "P b c 21",   " P 2c -2b"       },
{  29,   0,  "cab", "P 21 a b",   " P -2b 2a"       },
{  29,   0, "-cba", "P 21 c a",   " P -2ac 2a"      },
{  29,   0,  "bca", "P c 21 b",   " P -2bc -2c"     },
{  29,   0, "a-cb", "P b 21 a",   " P -2a -2ab"     },
{  30,   0,     "", "P n c 2",    " P 2 -2bc"       },
{  30,   0, "ba-c", "P c n 2",    " P 2 -2ac"       },
{  30,   0,  "cab", "P 2 n a",    " P -2ac 2"       },
{  30,   0, "-cba", "P 2 a n",    " P -2ab 2"       },
{  30,   0,  "bca", "P b 2 n",    " P -2ab -2ab"    },
{  30,   0, "a-cb", "P n 2 b",    " P -2bc -2bc"    },
{  31,   0,     "", "P m n 21",   " P 2ac -2"       },
{  31,   0, "ba-c", "P n m 21",   " P 2bc -2bc"     },
{  31,   0,  "cab", "P 21 m n",   " P -2ab 2ab"     },
{  31,   0, "-cba", "P 21 n m",   " P -2 2ac"       },
{  31,   0,  "bca", "P n 21 m",   " P -2 -2bc"      },
{  31,   0, "a-cb", "P m 21 n",   " P -2ab -2"      },
{  32,   0,     "", "P b a 2",    " P 2 -2ab"       },
{  32,   0,  "cab", "P 2 c b",    " P -2bc 2"       },
{  32,   0,  "bca", "P c 2 a",    " P -2ac -2ac"    },
{  33,   0,     "", "P n a 21",   " P 2c -2n"       },
{  33,   0, "ba-c", "P b n 21",   " P 2c -2ab"      },
{  33,   0,  "cab", "P 21 n b",   " P -2bc 2a"      },
{  33,   0, "-cba", "P 21 c n",   " P -2n 2a"       },
{  33,   0,  "bca", "P c 21 n",   " P -2n -2ac"     },
{  33,   0, "a-cb", "P n 21 a",   " P -2ac -2n"     },
{  34,   0,     "", "P n n 2",    " P 2 -2n"        },
{  34,   0,  "cab", "P 2 n n",    " P -2n 2"        },
{  34,   0,  "bca", "P n 2 n",    " P -2n -2n"      },
{  35,   0,     "", "C m m 2",    " C 2 -2"         },
{  35,   0,  "cab", "A 2 m m",    " A -2 2"         },
{  35,   0,  "bca", "B m 2 m",    " B -2 -2"        },
{  36,   0,     "", "C m c 21",   " C 2c -2"        },
{  36,   0, "ba-c", "C c m 21",   " C 2c -2c"       },
{  36,   0,  "cab", "A 21 m a",   " A -2a 2a"       },
{  36,   0, "-cba", "A 21 a m",   " A -2 2a"        },
{  36,   0,  "bca", "B b 21 m",   " B -2 -2b"       },
{  36,   0, "a-cb", "B m 21 b",   " B -2b -2"       },
{  37,   0,     "", "C c c 2",    " C 2 -2c"        },
{  37,   0,  "cab", "A 2 a a",    " A -2a 2"        },
{  37,   0,  "bca", "B b 2 b",    " B -2b -2b"      },
{  38,   0,     "", "A m m 2",    " A 2 -2"         },
{  38,   0, "ba-c", "B m m 2",    " B 2 -2"         },
{  38,   0,  "cab", "B 2 m m",    " B -2 2"         },
{  38,   0, "-cba", "C 2 m m",    " C -2 2"         },
{  38,   0,  "bca", "C m 2 m",    " C -2 -2"        },
{  38,   0, "a-cb", "A m 2 m",    " A -2 -2"        },
{  39,   0,     "", "A b m 2",    " A 2 -2b"        },
{  39,   0, "ba-c", "B m a 2",    " B 2 -2a"        },
{  39,   0,  "cab", "B 2 c m",    " B -2a 2"        },
{  39,   0, "-cba", "C 2 m b",    " C -2a 2"        },
{  39,   0,  "bca", "C m 2 a",    " C -2a -2a"      },
{  39,   0, "a-cb", "A c 2 m",    " A -2b -2b"      },
{  40,   0,     "", "A m a 2",    " A 2 -2a"        },
{  40,   0, "ba-c", "B b m 2",    " B 2 -2b"        },
{  40,   0,  "cab", "B 2 m b",    " B -2b 2"        },
{  40,   0, "-cba", "C 2 c m",    " C -2c 2"        },
{  40,   0,  "bca", "C c 2 m",    " C -2c -2c"      },
{  40,   0, "a-cb", "A m 2 a",    " A -2a -2a"      },
{  41,   0,     "", "A b a 2",    " A 2 -2ab"       },
{  41,   0, "ba-c", "B b a 2",    " B 2 -2ab"       },
{  41,   0,  "cab", "B 2 c b",    " B -2ab 2"       },
{  41,   0, "-cba", "C 2 c b",    " C -2ac 2"       },
{  41,   0,  "bca", "C c 2 a",    " C -2ac -2ac"    },
{  41,   0, "a-cb", "A c 2 a",    " A -2ab -2ab"    },
{  42,   0,     "", "F m m 2",    " F 2 -2"         },
{  42,   0,  "cab", "F 2 m m",    " F -2 2"         },
{  42,   0,  "bca", "F m 2 m",    " F -2 -2"        },
{  43,   0,     "", "F d d 2",    " F 2 -2d"        },
{  43,   0,  "cab", "F 2 d d",    " F -2d 2"        },
{  43,   0,  "bca", "F d 2 d",    " F -2d -2d"      },
{  44,   0,     "", "I m m 2",    " I 2 -2"         },
{  44,   0,  "cab", "I 2 m m",    " I -2 2"         },
{  44,   0,  "bca", "I m 2 m",    " I -2 -2"        },
{  45,   0,     "", "I b a 2",    " I 2 -2c"        },
{  45,   0,  "cab", "I 2 c b",    " I -2a 2"        },
{  45,   0,  "bca", "I c 2 a",    " I -2b -2b"      },
{  46,   0,     "", "I m a 2",    " I 2 -2a"        },
{  46,   0, "ba-c", "I b m 2",    " I 2 -2b"        },
{  46,   0,  "cab", "I 2 m b",    " I -2b 2"        },
{  46,   0, "-cba", "I 2 c m",    " I -2c 2"        },
{  46,   0,  "bca", "I c 2 m",    " I -2c -2c"      },
{  46,   0, "a-cb", "I m 2 a",    " I -2a -2a"      },
{  47,   0,     "", "P m m m",    "-P 2 2"          },
{  48, '1',     "", "P n n n",    " P 2 2 -1n"      },
{  48, '2',     "", "P n n n",    "-P 2ab 2bc"      },
{  49,   0,     "", "P c c m",    "-P 2 2c"         },
{  49,   0,  "cab", "P m a a",    "-P 2a 2"         },
{  49,   0,  "bca", "P b m b",    "-P 2b 2b"        },
{  50, '1',     "", "P b a n",    " P 2 2 -1ab"     },
{  50, '2',     "", "P b a n",    "-P 2ab 2b"       },
{  50, '1',  "cab", "P n c b",    " P 2 2 -1bc"     },
{  50, '2',  "cab", "P n c b",    "-P 2b 2bc"       },
{  50, '1',  "bca", "P c n a",    " P 2 2 -1ac"     },
{  50, '2',  "bca", "P c n a",    "-P 2a 2c"        },
{  51,   0,     "", "P m m a",    "-P 2a 2a"        },
{  51,   0, "ba-c", "P m m b",    "-P 2b 2"         },
{  51,   0,  "cab", "P b m m",    "-P 2 2b"         },
{  51,   0, "-cba", "P c m m",    "-P 2c 2c"        },
{  51,   0,  "bca", "P m c m",    "-P 2c 2"         },
{  51,   0, "a-cb", "P m a m",    "-P 2 2a"         },
{  52,   0,     "", "P n n a",    "-P 2a 2bc"       },
{  52,   0, "ba-c", "P n n b",    "-P 2b 2n"        },
{  52,   0,  "cab", "P b n n",    "-P 2n 2b"        },
{  52,   0, "-cba", "P c n n",    "-P 2ab 2c"       },
{  52,   0,  "bca", "P n c n",    "-P 2ab 2n"       },
{  52,   0, "a-cb", "P n a n",    "-P 2n 2bc"       },
{  53,   0,     "", "P m n a",    "-P 2ac 2"        },
{  53,   0, "ba-c", "P n m b",    "-P 2bc 2bc"      },
{  53,   0,  "cab", "P b m n",    "-P 2ab 2ab"      },
{  53,   0, "-cba", "P c n m",    "-P 2 2ac"        },
{  53,   0,  "bca", "P n c m",    "-P 2 2bc"        },
{  53,   0, "a-cb", "P m a n",    "-P 2ab 2"        },
{  54,   0,     "", "P c c a",    "-P 2a 2ac"       },
{  54,   0, "ba-c", "P c c b",    "-P 2b 2c"        },
{  54,   0,  "cab", "P b a a",    "-P 2a 2b"        },
{  54,   0, "-cba", "P c a a",    "-P 2ac 2c"       },
{  54,   0,  "bca", "P b c b",    "-P 2bc 2b"       },
{  54,   0, "a-cb", "P b a b",    "-P 2b 2ab"       },
{  55,   0,     "", "P b a m",    "-P 2 2ab"        },
{  55,   0,  "cab", "P m c b",    "-P 2bc 2"        },
{  55,   0,  "bca", "P c m a",    "-P 2ac 2ac"      },
{  56,   0,     "", "P c c n",    "-P 2ab 2ac"      },
{  56,   0,  "cab", "P n a a",    "-P 2ac 2bc"      },
{  56,   0,  "bca", "P b n b",    "-P 2bc 2ab"      },
{  57,   0,     "", "P b c m",    "-P 2c 2b"        },
{  57,   0, "ba-c", "P c a m",    "-P 2c 2ac"       },
{  57,   0,  "cab", "P m c a",    "-P 2ac 2a"       },
{  57,   0, "-cba", "P m a b",    "-P 2b 2a"        },
{  57,   0,  "bca", "P b m a",    "-P 2a 2ab"       },
{  57,   0, "a-cb", "P c m b",    "-P 2bc 2c"       },
{  58,   0,     "", "P n n m",    "-P 2 2n"         },
{  58,   0,  "cab", "P m n n",    "-P 2n 2"         },
{  58,   0,  "bca", "P n m n",    "-P 2n 2n"        },
{  59, '1',     "", "P m m n",    " P 2 2ab -1ab"   },
{  59, '2',     "", "P m m n",    "-P 2ab 2a"       },
{  59, '1',  "cab", "P n m m",    " P 2bc 2 -1bc"   },
{  59, '2',  "cab", "P n m m",    "-P 2c 2bc"       },
{  59, '1',  "bca", "P m n m",    " P 2ac 2ac -1ac" },
{  59, '2',  "bca", "P m n m",    "-P 2c 2a"        },
{  60,   0,     "", "P b c n",    "-P 2n 2ab"       },
{  60,   0, "ba-c", "P c a n",    "-P 2n 2c"        },
{  60,   0,  "cab", "P n c a",    "-P 2a 2n"        },
{  60,   0, "-cba", "P n a b",    "-P 2bc 2n"       },
{  60,   0,  "bca", "P b n a",    "-P 2ac 2b"       },
{  60,   0, "a-cb", "P c n b",    "-P 2b 2ac"       },
{  61,   0,     "", "P b c a",    "-P 2ac 2ab"      },
{  61,   0, "ba-c", "P c a b",    "-P 2bc 2ac"      },
{  62,   0,     "", "P n m a",    "-P 2ac 2n"       },
{  62,   0, "ba-c", "P m n b",    "-P 2bc 2a"       },
{  62,   0,  "cab", "P b n m",    "-P 2c 2ab"       },
{  62,   0, "-cba", "P c m n",    "-P 2n 2ac"       },
{  62,   0,  "bca", "P m c n",    "-P 2n 2a"        },
{  62,   0, "a-cb", "P n a m",    "-P 2c 2n"        },
{  63,   0,     "", "C m c m",    "-C 2c 2"         },
{  63,   0, "ba-c", "C c m m",    "-C 2c 2c"        },
{  63,   0,  "cab", "A m m a",    "-A 2a 2a"        },
{  63,   0, "-cba", "A m a m",    "-A 2 2a"         },
{  63,   0,  "bca", "B b m m",    "-B 2 2b"         },
{  63,   0, "a-cb", "B m m b",    "-B 2b 2"         },
{  64,   0,     "", "C m c a",    "-C 2ac 2"        },
{  64,   0, "ba-c", "C c m b",    "-C 2ac 2ac"      },
{  64,   0,  "cab", "A b m a",    "-A 2ab 2ab"      },
{  64,   0, "-cba", "A c a m",    "-A 2 2ab"        },
{  64,   0,  "bca", "B b c m",    "-B 2 2ab"        },
{  64,   0, "a-cb", "B m a b",    "-B 2ab 2"        },
{  65,   0,     "", "C m m m",    "-C 2 2"          },
{  65,   0,  "cab", "A m m m",    "-A 2 2"          },
{  65,   0,  "bca", "B m m m",    "-B 2 2"          },
{  66,   0,     "", "C c c m",    "-C 2 2c"         },
{  66,   0,  "cab", "A m a a",    "-A 2a 2"         },
{  66,   0,  "bca", "B b m b",    "-B 2b 2b"        },
{  67,   0,     "", "C m m a",    "-C 2a 2"         },
{  67,   0, "ba-c", "C m m b",    "-C 2a 2a"        },
{  67,   0,  "cab", "A b m m",    "-A 2b 2b"        },
{  67,   0, "-cba", "A c m m",    "-A 2 2b"         },
{  67,   0,  "bca", "B m c m",    "-B 2 2a"         },
{  67,   0, "a-cb", "B m a m",    "-B 2a 2"         },
{  68, '1',     "", "C c c a",    " C 2 2 -1ac"     },
{  68, '2',     "", "C c c a",    "-C 2a 2ac"       },
{  68, '1', "ba-c", "C c c b",    " C 2 2 -1ac"     },
{  68, '2', "ba-c", "C c c b",    "-C 2a 2c"        },
{  68, '1',  "cab", "A b a a",    " A 2 2 -1ab"     },
{  68, '2',  "cab", "A b a a",    "-A 2a 2b"        },
{  68, '1', "-cba", "A c a a",    " A 2 2 -1ab"     },
{  68, '2', "-cba", "A c a a",    "-A 2ab 2b"       },
{  68, '1',  "bca", "B b c b",    " B 2 2 -1ab"     },
{  68, '2',  "bca", "B b c b",    "-B 2ab 2b"       },
{  68, '1', "a-cb", "B b a b",    " B 2 2 -1ab"     },
{  68, '2', "a-cb", "B b a b",    "-B 2b 2ab"       },
{  69,   0,     "", "F m m m",    "-F 2 2"          },
{  70, '1',     "", "F d d d",    " F 2 2 -1d"      },
{  70, '2',     "", "F d d d",    "-F 2uv 2vw"      },
{  71,   0,     "", "I m m m",    "-I 2 2"          },
{  72,   0,     "", "I b a m",    "-I 2 2c"         },
{  72,   0,  "cab", "I m c b",    "-I 2a 2"         },
{  72,   0,  "bca", "I c m a",    "-I 2b 2b"        },
{  73,   0,     "", "I b c a",    "-I 2b 2c"        },
{  73,   0, "ba-c", "I c a b",    "-I 2a 2b"        },
{  74,   0,     "", "I m m a",    "-I 2b 2"         },
{  74,   0, "ba-c", "I m m b",    "-I 2a 2a"        },
{  74,   0,  "cab", "I b m m",    "-I 2c 2c"        },
{  74,   0, "-cba", "I c m m",    "-I 2 2b"         },
{  74,   0,  "bca", "I m c m",    "-I 2 2a"         },
{  74,   0, "a-cb", "I m a m",    "-I 2c 2"         },
{  75,   0,     "", "P 4",        " P 4"            },
{  76,   0,     "", "P 41",       " P 4w"           },
{  77,   0,     "", "P 42",       " P 4c"           },
{  78,   0,     "", "P 43",       " P 4cw"          },
{  79,   0,     "", "I 4",        " I 4"            },
{  80,   0,     "", "I 41",       " I 4bw"          },
{  81,   0,     "", "P -4",       " P -4"           },
{  82,   0,     "", "I -4",       " I -4"           },
{  83,   0,     "", "P 4/m",      "-P 4"            },
{  84,   0,     "", "P 42/m",     "-P 4c"           },
{  85, '1',     "", "P 4/n",      " P 4ab -1ab"     },
{  85, '2',     "", "P 4/n",      "-P 4a"           },
{  86, '1',     "", "P 42/n",     " P 4n -1n"       },
{  86, '2',     "", "P 42/n",     "-P 4bc"          },
{  87,   0,     "", "I 4/m",      "-I 4"            },
{  88, '1',     "", "I 41/a",     " I 4bw -1bw"     },
{  88, '2',     "", "I 41/a",     "-I 4ad"          },
{  89,   0,     "", "P 4 2 2",    " P 4 2"          },
{  90,   0,     "", "P 4 21 2",   " P 4ab 2ab"      },
{  91,   0,     "", "P 41 2 2",   " P 4w 2c"        },
{  92,   0,     "", "P 41 21 2",  " P 4abw 2nw"     },
{  93,   0,     "", "P 42 2 2",   " P 4c 2"         },
{  94,   0,     "", "P 42 21 2",  " P 4n 2n"        },
{  95,   0,     "", "P 43 2 2",   " P 4cw 2c"       },
{  96,   0,     "", "P 43 21 2",  " P 4nw 2abw"     },
{  97,   0,     "", "I 4 2 2",    " I 4 2"          },
{  98,   0,     "", "I 41 2 2",   " I 4bw 2bw"      },
{  99,   0,     "", "P 4 m m",    " P 4 -2"         },
{ 100,   0,     "", "P 4 b m",    " P 4 -2ab"       },
{ 101,   0,     "", "P 42 c m",   " P 4c -2c"       },
{ 102,   0,     "", "P 42 n m",   " P 4n -2n"       },
{ 103,   0,     "", "P 4 c c",    " P 4 -2c"        },
{ 104,   0,     "", "P 4 n c",    " P 4 -2n"        },
{ 105,   0,     "", "P 42 m c",   " P 4c -2"        },
{ 106,   0,     "", "P 42 b c",   " P 4c -2ab"      },
{ 107,   0,     "", "I 4 m m",    " I 4 -2"         },
{ 108,   0,     "", "I 4 c m",    " I 4 -2c"        },
{ 109,   0,     "", "I 41 m d",   " I 4bw -2"       },
{ 110,   0,     "", "I 41 c d",   " I 4bw -2c"      },
{ 111,   0,     "", "P -4 2 m",   " P -4 2"         },
{ 112,   0,     "", "P -4 2 c",   " P -4 2c"        },
{ 113,   0,     "", "P -4 21 m",  " P -4 2ab"       },
{ 114,   0,     "", "P -4 21 c",  " P -4 2n"        },
{ 115,   0,     "", "P -4 m 2",   " P -4 -2"        },
{ 116,   0,     "", "P -4 c 2",   " P -4 -2c"       },
{ 117,   0,     "", "P -4 b 2",   " P -4 -2ab"      },
{ 118,   0,     "", "P -4 n 2",   " P -4 -2n"       },
{ 119,   0,     "", "I -4 m 2",   " I -4 -2"        },
{ 120,   0,     "", "I -4 c 2",   " I -4 -2c"       },
{ 121,   0,     "", "I -4 2 m",   " I -4 2"         },
{ 122,   0,     "", "I -4 2 d",   " I -4 2bw"       },
{ 123,   0,     "", "P 4/m m m",  "-P 4 2"          },
{ 124,   0,     "", "P 4/m c c",  "-P 4 2c"         },
{ 125, '1',     "", "P 4/n b m",  " P 4 2 -1ab"     },
{ 125, '2',     "", "P 4/n b m",  "-P 4a 2b"        },
{ 126, '1',     "", "P 4/n n c",  " P 4 2 -1n"      },
{ 126, '2',     "", "P 4/n n c",  "-P 4a 2bc"       },
{ 127,   0,     "", "P 4/m b m",  "-P 4 2ab"        },
{ 128,   0,     "", "P 4/m n c",  "-P 4 2n"         },
{ 129, '1',     "", "P 4/n m m",  " P 4ab 2ab -1ab" },
{ 129, '2',     "", "P 4/n m m",  "-P 4a 2a"        },
{ 130, '1',     "", "P 4/n c c",  " P 4ab 2n -1ab"  },
{ 130, '2',     "", "P 4/n c c",  "-P 4a 2ac"       },
{ 131,   0,     "", "P 42/m m c", "-P 4c 2"         },
{ 132,   0,     "", "P 42/m c m", "-P 4c 2c"        },
{ 133, '1',     "", "P 42/n b c", " P 4n 2c -1n"    },
{ 133, '2',     "", "P 42/n b c", "-P 4ac 2b"       },
{ 134, '1',     "", "P 42/n n m", " P 4n 2 -1n"     },
{ 134, '2',     "", "P 42/n n m", "-P 4ac 2bc"      },
{ 135,   0,     "", "P 42/m b c", "-P 4c 2ab"       },
{ 136,   0,     "", "P 42/m n m", "-P 4n 2n"        },
{ 137, '1',     "", "P 42/n m c", " P 4n 2n -1n"    },
{ 137, '2',     "", "P 42/n m c", "-P 4ac 2a"       },
{ 138, '1',     "", "P 42/n c m", " P 4n 2ab -1n"   },
{ 138, '2',     "", "P 42/n c m", "-P 4ac 2ac"      },
{ 139,   0,     "", "I 4/m m m",  "-I 4 2"          },
{ 140,   0,     "", "I 4/m c m",  "-I 4 2c"         },
{ 141, '1',     "", "I 41/a m d", " I 4bw 2bw -1bw" },
{ 141, '2',     "", "I 41/a m d", "-I 4bd 2"        },
{ 142, '1',     "", "I 41/a c d", " I 4bw 2aw -1bw" },
{ 142, '2',     "", "I 41/a c d", "-I 4bd 2c"       },
{ 143,   0,     "", "P 3",        " P 3"            },
{ 144,   0,     "", "P 31",       " P 31"           },
{ 145,   0,     "", "P 32",       " P 32"           },
{ 146, 'H',     "", "R 3",        " R 3"            },
{ 146, 'R',     "", "R 3",        " P 3*"           },
{ 147,   0,     "", "P -3",       "-P 3"            },
{ 148, 'H',     "", "R -3",       "-R 3"            },
{ 148, 'R',     "", "R -3",       "-P 3*"           },
{ 149,   0,     "", "P 3 1 2",    " P 3 2"          },
{ 150,   0,     "", "P 3 2 1",    " P 3 2\""        },
{ 151,   0,     "", "P 31 1 2",   " P 31 2 (0 0 4)" },
{ 152,   0,     "", "P 31 2 1",   " P 31 2\""       },
{ 153,   0,     "", "P 32 1 2",   " P 32 2 (0 0 2)" },
{ 154,   0,     "", "P 32 2 1",   " P 32 2\""       },
{ 155, 'H',     "", "R 3 2",      " R 3 2\""        },
{ 155, 'R',     "", "R 3 2",      " P 3* 2"         },
{ 156,   0,     "", "P 3 m 1",    " P 3 -2\""       },
{ 157,   0,     "", "P 3 1 m",    " P 3 -2"         },
{ 158,   0,     "", "P 3 c 1",    " P 3 -2\"c"      },
{ 159,   0,     "", "P 3 1 c",    " P 3 -2c"        },
{ 160, 'H',     "", "R 3 m",      " R 3 -2\""       },
{ 160, 'R',     "", "R 3 m",      " P 3* -2"        },
{ 161, 'H',     "", "R 3 c",      " R 3 -2\"c"      },
{ 161, 'R',     "", "R 3 c",      " P 3* -2n"       },
{ 162,   0,     "", "P -3 1 m",   "-P 3 2"          },
{ 163,   0,     "", "P -3 1 c",   "-P 3 2c"         },
{ 164,   0,     "", "P -3 m 1",   "-P 3 2\""        },
{ 165,   0,     "", "P -3 c 1",   "-P 3 2\"c"       },
{ 166, 'H',     "", "R -3 m",     "-R 3 2\""        },
{ 166, 'R',     "", "R -3 m",     "-P 3* 2"         },
{ 167, 'H',     "", "R -3 c",     "-R 3 2\"c"       },
{ 167, 'R',     "", "R -3 c",     "-P 3* 2n"        },
{ 168,   0,     "", "P 6",        " P 6"            },
{ 169,   0,     "", "P 61",       " P 61"           },
{ 170,   0,     "", "P 65",       " P 65"           },
{ 171,   0,     "", "P 62",       " P 62"           },
{ 172,   0,     "", "P 64",       " P 64"           },
{ 173,   0,     "", "P 63",       " P 6c"           },
{ 174,   0,     "", "P -6",       " P -6"           },
{ 175,   0,     "", "P 6/m",      "-P 6"            },
{ 176,   0,     "", "P 63/m",     "-P 6c"           },
{ 177,   0,     "", "P 6 2 2",    " P 6 2"          },
{ 178,   0,     "", "P 61 2 2",   " P 61 2 (0 0 5)" },
{ 179,   0,     "", "P 65 2 2",   " P 65 2 (0 0 1)" },
{ 180,   0,     "", "P 62 2 2",   " P 62 2 (0 0 4)" },
{ 181,   0,     "", "P 64 2 2",   " P 64 2 (0 0 2)" },
{ 182,   0,     "", "P 63 2 2",   " P 6c 2c"        },
{ 183,   0,     "", "P 6 m m",    " P 6 -2"         },
{ 184,   0,     "", "P 6 c c",    " P 6 -2c"        },
{ 185,   0,     "", "P 63 c m",   " P 6c -2"        },
{ 186,   0,     "", "P 63 m c",   " P 6c -2c"       },
{ 187,   0,     "", "P -6 m 2",   " P -6 2"         },
{ 188,   0,     "", "P -6 c 2",   " P -6c 2"        },
{ 189,   0,     "", "P -6 2 m",   " P -6 -2"        },
{ 190,   0,     "", "P -6 2 c",   " P -6c -2c"      },
{ 191,   0,     "", "P 6/m m m",  "-P 6 2"          },
{ 192,   0,     "", "P 6/m c c",  "-P 6 2c"         },
{ 193,   0,     "", "P 63/m c m", "-P 6c 2"         },
{ 194,   0,     "", "P 63/m m c", "-P 6c 2c"        },
{ 195,   0,     "", "P 2 3",      " P 2 2 3"        },
{ 196,   0,     "", "F 2 3",      " F 2 2 3"        },
{ 197,   0,     "", "I 2 3",      " I 2 2 3"        },
{ 198,   0,     "", "P 21 3",     " P 2ac 2ab 3"    },
{ 199,   0,     "", "I 21 3",     " I 2b 2c 3"      },
{ 200,   0,     "", "P m -3",     "-P 2 2 3"        },
{ 201, '1',     "", "P n -3",     " P 2 2 3 -1n"    },
{ 201, '2',     "", "P n -3",     "-P 2ab 2bc 3"    },
{ 202,   0,     "", "F m -3",     "-F 2 2 3"        },
{ 203, '1',     "", "F d -3",     " F 2 2 3 -1d"    },
{ 203, '2',     "", "F d -3",     "-F 2uv 2vw 3"    },
{ 204,   0,     "", "I m -3",     "-I 2 2 3"        },
{ 205,   0,     "", "P a -3",     "-P 2ac 2ab 3"    },
{ 206,   0,     "", "I a -3",     "-I 2b 2c 3"      },
{ 207,   0,     "", "P 4 3 2",    " P 4 2 3"        },
{ 208,   0,     "", "P 42 3 2",   " P 4n 2 3"       },
{ 209,   0,     "", "F 4 3 2",    " F 4 2 3"        },
{ 210,   0,     "", "F 41 3 2",   " F 4d 2 3"       },
{ 211,   0,     "", "I 4 3 2",    " I 4 2 3"        },
{ 212,   0,     "", "P 43 3 2",   " P 4acd 2ab 3"   },
{ 213,   0,     "", "P 41 3 2",   " P 4bd 2ab 3"    },
{ 214,   0,     "", "I 41 3 2",   " I 4bd 2c 3"     },
{ 215,   0,     "", "P -4 3 m",   " P -4 2 3"       },
{ 216,   0,     "", "F -4 3 m",   " F -4 2 3"       },
{ 217,   0,     "", "I -4 3 m",   " I -4 2 3"       },
{ 218,   0,     "", "P -4 3 n",   " P -4n 2 3"      },
{ 219,   0,     "", "F -4 3 c",   " F -4a 2 3"      },
{ 220,   0,     "", "I -4 3 d",   " I -4bd 2c 3"    },
{ 221,   0,     "", "P m -3 m",   "-P 4 2 3"        },
{ 222, '1',     "", "P n -3 n",   " P 4 2 3 -1n"    },
{ 222, '2',     "", "P n -3 n",   "-P 4a 2bc 3"     },
{ 223,   0,     "", "P m -3 n",   "-P 4n 2 3"       },
{ 224, '1',     "", "P n -3 m",   " P 4n 2 3 -1n"   },
{ 224, '2',     "", "P n -3 m",   "-P 4bc 2bc 3"    },
{ 225,   0,     "", "F m -3 m",   "-F 4 2 3"        },
{ 226,   0,     "", "F m -3 c",   "-F 4a 2 3"       },
{ 227, '1',     "", "F d -3 m",   " F 4d 2 3 -1d"   },
{ 227, '2',     "", "F d -3 m",   "-F 4vw 2vw 3"    },
{ 228, '1',     "", "F d -3 c",   " F 4d 2 3 -1ad"  },
{ 228, '2',     "", "F d -3 c",   "-F 4ud 2vw 3"    },
{ 229,   0,     "", "I m -3 m",   "-I 4 2 3"        },
{ 230,   0,     "", "I a -3 d",   "-I 4bd 2c 3"     },
{   0,   0,     "", "",           ""                }
};

const char* SchoenfliesSymbols[] = {
    "C1^1", "Ci^1", "C2^1", "C2^2", "C2^3", "Cs^1", "Cs^2", "Cs^3", "Cs^4",
    "C2h^1", "C2h^2", "C2h^3", "C2h^4", "C2h^5", "C2h^6", "D2^1", "D2^2",
    "D2^3", "D2^4", "D2^5", "D2^6", "D2^7", "D2^8", "D2^9", "C2v^1", "C2v^2",
    "C2v^3", "C2v^4", "C2v^5", "C2v^6", "C2v^7", "C2v^8", "C2v^9", "C2v^10",
    "C2v^11", "C2v^12", "C2v^13", "C2v^14", "C2v^15", "C2v^16", "C2v^17",
    "C2v^18", "C2v^19", "C2v^20", "C2v^21", "C2v^22", "D2h^1", "D2h^2",
    "D2h^3", "D2h^4", "D2h^5", "D2h^6", "D2h^7", "D2h^8", "D2h^9", "D2h^10",
    "D2h^11", "D2h^12", "D2h^13", "D2h^14", "D2h^15", "D2h^16", "D2h^17",
    "D2h^18", "D2h^19", "D2h^20", "D2h^21", "D2h^22", "D2h^23", "D2h^24",
    "D2h^25", "D2h^26", "D2h^27", "D2h^28", "C4^1", "C4^2", "C4^3", "C4^4",
    "C4^5", "C4^6", "S4^1", "S4^2", "C4h^1", "C4h^2", "C4h^3", "C4h^4",
    "C4h^5", "C4h^6", "D4^1", "D4^2", "D4^3", "D4^4", "D4^5", "D4^6", "D4^7",
    "D4^8", "D4^9", "D4^10", "C4v^1", "C4v^2", "C4v^3", "C4v^4", "C4v^5",
    "C4v^6", "C4v^7", "C4v^8", "C4v^9", "C4v^10", "C4v^11", "C4v^12", "D2d^1",
    "D2d^2", "D2d^3", "D2d^4", "D2d^5", "D2d^6", "D2d^7", "D2d^8", "D2d^9",
    "D2d^10", "D2d^11", "D2d^12", "D4h^1", "D4h^2", "D4h^3", "D4h^4", "D4h^5",
    "D4h^6", "D4h^7", "D4h^8", "D4h^9", "D4h^10", "D4h^11", "D4h^12", "D4h^13",
    "D4h^14", "D4h^15", "D4h^16", "D4h^17", "D4h^18", "D4h^19", "D4h^20",
    "C3^1", "C3^2", "C3^3", "C3^4", "C3i^1", "C3i^2", "D3^1", "D3^2", "D3^3",
    "D3^4", "D3^5", "D3^6", "D3^7", "C3v^1", "C3v^2", "C3v^3", "C3v^4",
    "C3v^5", "C3v^6", "D3d^1", "D3d^2", "D3d^3", "D3d^4", "D3d^5", "D3d^6",
    "C6^1", "C6^2", "C6^3", "C6^4", "C6^5", "C6^6", "C3h^1", "C6h^1", "C6h^2",
    "D6^1", "D6^2", "D6^3", "D6^4", "D6^5", "D6^6", "C6v^1", "C6v^2", "C6v^3",
    "C6v^4", "D3h^1", "D3h^2", "D3h^3", "D3h^4", "D6h^1", "D6h^2", "D6h^3",
    "D6h^4", "T^1", "T^2", "T^3", "T^4", "T^5", "Th^1", "Th^2", "Th^3", "Th^4",
    "Th^5", "Th^6", "Th^7", "O^1", "O^2", "O^3", "O^4", "O^5", "O^6", "O^7",
    "O^8", "Td^1", "Td^2", "Td^3", "Td^4", "Td^5", "Td^6", "Oh^1", "Oh^2",
    "Oh^3", "Oh^4", "Oh^5", "Oh^6", "Oh^7", "Oh^8", "Oh^9", "Oh^10"
};

// PowderCell uses settings (the second number after RGNR) that are listed at:
// http://www.ccp14.ac.uk/ccp/web-mirrors/powdcell/a_v/v_1/powder/details/setting.htm
// The following table was generated from PCWSPGR.DAT file. In this file
// lines starting with "spgr" contain description of settings.
// Examples:
//
// spgr   3  1  3  0  2  4  1   P 1 2 1
// spgr  48  1  5  0  8 12  3   P n n n
// spgr  48  2  5  1  4 12  2   P n n n
// spgr 155  1 11  0  6  5  2   R 3 2
// spgr 155  2 10  0  6  5  2   R 3 2
//
// We used the following records from these lines:
// - the number of space-group type used in IT generally (2nd record)
// - the corresponding Laue group (actually the code explained below, 4th)
// - the existence of a inversion centre in origin (0...no, 1...yes) (5th)
// - the full Hermann-Mauguin symbol (the last one), 
//
// The code of Laue group is useful to distinguish :R and :H settings:
// 8 trigonal, rhombohedral coordinate system (-3)
// 9 trigonal, hexagonal coordinate system (-3)
// 10 trigonal, rhombohedral coordinate system (-3m)
// 11 trigonal, hexagonal coordinate system (-3m)

// Here we have a dictionary to match them with the settings in the table above.
// E.g.  sg=15, pc_setting==9, std_setting==3 means that
// "RGNR 15 9" <-> 4th setting for SG 15 in space_group_settings[]
// If there is only one position in space_group_settings[] for given SG number,
// of pc_setting=n corresponds to n-th position in space_group_settings[],
// the item is omitted.
struct PowderCellSgSetting
{
    unsigned char sg; // space group number
    unsigned char pc_setting;
    // 0 - 1st setting for this SG in the table above, 1 - 2nd, ...
    unsigned char std_setting;
};

const PowderCellSgSetting powdercell_settings[] = {
{3,2,0}, {3,3,1}, {3,4,1}, {3,5,2}, {3,6,2},
{4,2,0}, {4,3,1}, {4,4,1}, {4,5,2}, {4,6,2},
{5,3,3}, {5,4,4}, {5,5,6}, {5,6,7}, {5,7,1}, {5,8,0}, {5,9,4}, {5,10,3},
{5,11,7}, {5,12,6}, {5,13,2}, {5,14,2}, {5,15,5}, {5,16,5}, {5,17,8}, {5,18,8},
{6,2,0}, {6,3,1}, {6,4,1}, {6,5,2}, {6,6,2},
{7,2,2}, {7,3,3}, {7,4,5}, {7,5,6}, {7,6,8}, {7,7,1}, {7,8,1}, {7,9,4},
{7,10,4}, {7,11,7}, {7,12,7}, {7,13,2}, {7,14,0}, {7,15,5}, {7,16,3}, {7,17,8},
{7,18,6},
{8,3,3}, {8,4,4}, {8,5,6}, {8,6,7}, {8,7,1}, {8,8,0}, {8,9,4}, {8,10,3},
{8,11,7}, {8,12,6}, {8,13,2}, {8,14,2}, {8,15,5}, {8,16,5}, {8,17,8}, {8,18,8},
{9,2,3}, {9,3,6}, {9,4,9}, {9,5,12}, {9,6,15}, {9,7,1}, {9,8,4}, {9,9,7},
{9,10,10}, {9,11,13}, {9,12,16}, {9,13,2}, {9,14,5}, {9,15,8}, {9,16,11},
{9,17,14},
{10,2,0}, {10,3,1}, {10,4,1}, {10,5,2}, {10,6,2},
{11,2,0}, {11,3,1}, {11,4,1}, {11,5,2}, {11,6,2},
{12,3,3}, {12,4,4}, {12,5,6}, {12,6,7}, {12,7,1}, {12,8,0}, {12,9,4},
{12,10,3}, {12,11,7}, {12,12,6}, {12,13,2}, {12,14,2}, {12,15,5}, {12,16,5},
{12,17,8}, {12,18,8},
{13,2,2}, {13,3,3}, {13,4,5}, {13,5,6}, {13,6,8}, {13,7,1}, {13,8,1}, {13,9,4},
{13,10,4}, {13,11,7}, {13,12,7}, {13,13,2}, {13,14,0}, {13,15,5}, {13,16,3},
{13,17,8}, {13,18,6},
{14,2,2}, {14,3,3}, {14,4,5}, {14,5,6}, {14,6,8}, {14,7,1}, {14,8,1}, {14,9,4},
{14,10,4}, {14,11,7}, {14,12,7}, {14,13,2}, {14,14,0}, {14,15,5}, {14,16,3},
{14,17,8}, {14,18,6},
{15,2,3}, {15,3,6}, {15,4,9}, {15,5,12}, {15,6,15}, {15,7,1}, {15,8,4},
{15,9,7}, {15,10,10}, {15,11,13}, {15,12,16}, {15,13,2}, {15,14,5}, {15,15,8},
{15,16,11}, {15,17,14},
{17,2,0}, {17,3,1}, {17,4,1}, {17,5,2}, {17,6,2},
{18,2,0}, {18,3,1}, {18,4,1}, {18,5,2}, {18,6,2},
{20,2,0}, {20,3,1}, {20,4,1}, {20,5,2}, {20,6,2},
{21,2,0}, {21,3,1}, {21,4,1}, {21,5,2}, {21,6,2},
{25,2,0}, {25,3,1}, {25,4,1}, {25,5,2}, {25,6,2},
{27,2,0}, {27,3,1}, {27,4,1}, {27,5,2}, {27,6,2},
{32,2,0}, {32,3,1}, {32,4,1}, {32,5,2}, {32,6,2},
{34,2,0}, {34,3,1}, {34,4,1}, {34,5,2}, {34,6,2},
{35,2,0}, {35,3,1}, {35,4,1}, {35,5,2}, {35,6,2},
{37,2,0}, {37,3,1}, {37,4,1}, {37,5,2}, {37,6,2},
{42,2,0}, {42,3,1}, {42,4,1}, {42,5,2}, {42,6,2},
{43,2,0}, {43,3,1}, {43,4,1}, {43,5,2}, {43,6,2},
{44,2,0}, {44,3,1}, {44,4,1}, {44,5,2}, {44,6,2},
{45,2,0}, {45,3,1}, {45,4,1}, {45,5,2}, {45,6,2},
{48,3,0}, {48,4,1}, {48,5,0}, {48,6,1}, {48,7,0}, {48,8,1}, {48,9,0},
{48,10,1}, {48,11,0}, {48,12,1},
{49,2,0}, {49,3,1}, {49,4,1}, {49,5,2}, {49,6,2},
{50,3,0}, {50,4,1}, {50,5,2}, {50,6,3}, {50,7,2}, {50,8,3}, {50,9,4},
{50,10,5}, {50,11,4}, {50,12,5},
{55,2,0}, {55,3,1}, {55,4,1}, {55,5,2}, {55,6,2},
{56,2,0}, {56,3,1}, {56,4,1}, {56,5,2}, {56,6,2},
{58,2,0}, {58,3,1}, {58,4,1}, {58,5,2}, {58,6,2},
{59,3,0}, {59,4,1}, {59,5,2}, {59,6,3}, {59,7,2}, {59,8,3}, {59,9,4},
{59,10,5}, {59,11,4}, {59,12,5},
{61,3,0}, {61,4,1}, {61,5,0}, {61,6,1},
{65,2,0}, {65,3,1}, {65,4,1}, {65,5,2}, {65,6,2},
{66,2,0}, {66,3,1}, {66,4,1}, {66,5,2}, {66,6,2},
{70,3,0}, {70,4,1}, {70,5,0}, {70,6,1}, {70,7,0}, {70,8,1}, {70,9,0},
{70,10,1}, {70,11,0}, {70,12,1},
{72,2,0}, {72,3,1}, {72,4,1}, {72,5,2}, {72,6,2},
{73,3,0}, {73,4,1}, {73,5,0}, {73,6,1},
{0,0,0}
};


const SpaceGroupSetting* get_sg_from_powdercell_rgnr(int sgn, int setting)
{
    const SpaceGroupSetting *sgs0 = find_first_sg_with_number(sgn);
    if (setting == 0 || (sgs0+1)->sgnumber != sgn)
        return sgs0;
    for (const PowderCellSgSetting *i = powdercell_settings; i->sg != 0; ++i)
        if (i->sg == sgn && i->pc_setting)
            return sgs0 + i->std_setting;
    // if we are here, the setting is not in PowderCell table,
    // i.e. std_setting == pc_setting - 1
    return sgs0 + setting - 1;
}


extern const char* default_cel_files[] = {

"cell  4.358 4.358 4.358  90 90 90\n"
"Si   14  0 0 0\n"
"C     6  0.25 0.25 0.25\n"
"rgnr 216\n"
"filename bSiC",

"cell  3.082 3.082 15.123  90 90 120\n"
"SI1  14  0       0       0\n"
"SI2  14  0.3333  0.6667  0.1667\n"
"SI3  14  0.3333  0.6667  0.8333\n"
"C1    6  0.0     0.0     0.125\n"
"C2    6  0.3333  0.6667  0.2917\n"
"C3    6  0.3333  0.6667  0.9583\n"
"rgnr 186\n"
"filename aSiC",

"cell  5.64009 5.64009 5.64009  90 90 90\n"
"Na   11   0   0   0\n"
"Cl   17   0.5 0   0\n"
"rgnr 225\n"
"filename NaCl",

"cell  3.5595 3.5595 3.5595  90 90 90\n"
"C     6  0 0 0\n"
"rgnr 227\n"
"filename diamond",

"cell  5.4309 5.4309 5.4309  90 90 90\n"
"Si   14  0 0 0\n"
"rgnr 227\n"
"filename Si",

"cell  5.41 5.41 5.41  90 90 90\n"
"Ce   58  0   0   0\n"
"O     8  0.25 0.25 0.25\n"
"rgnr 225\n"
"filename CeO2",

NULL
};


CelFile read_cel_file(const char* path)
{
    CelFile cel;
    cel.sgs = NULL;
    FILE *f = fopen(path, "r");
    if (!f)
        return cel;
    char s[20];
    int r = fscanf(f, "%4s %lf %lf %lf %lf %lf %lf",
               s, &cel.a, &cel.b, &cel.c, &cel.alpha, &cel.beta, &cel.gamma);
    if (r != 7) {
        fclose(f);
        return cel;
    }
    if (strcmp(s, "cell") != 0 && strcmp(s, "CELL") != 0
            && strcmp(s, "Cell") != 0) {
        fclose(f);
        return cel;
    }
    while (1) {
        fscanf(f, "%12s", s);
        if (strcmp(s, "RGNR") == 0 || strcmp(s, "rgnr") == 0
                || strcmp(s, "Rgnr") == 0)
            break;
        AtomInCell atom;
        r = fscanf(f, "%d %lf %lf %lf", &atom.Z, &atom.x, &atom.y, &atom.z);
        if (r != 4) {
            fclose(f);
            return cel;
        }
        cel.atoms.push_back(atom);
        // skip the rest of the line
        for (char c = fgetc(f); c != '\n' && c != EOF; c = fgetc(f))
            ;
    }
    int sgn;
    r = fscanf(f, "%d", &sgn);
    if (r != 1) {
        fclose(f);
        return cel;
    }
    if (sgn < 1 || sgn > 230) {
        fclose(f);
        return cel;
    }
    cel.sgs = find_first_sg_with_number(sgn);
    for (char c = fgetc(f); c != '\n' && c != EOF; c = fgetc(f)) {
        if (c == ':') {
            fscanf(f, "%8s", s);
            cel.sgs = find_space_group_setting(sgn, s);
            break;
        }
        else if (isdigit(c)) {
            ungetc(c, f);
            int pc_setting;
            fscanf(f, "%d", &pc_setting);
            cel.sgs = get_sg_from_powdercell_rgnr(sgn, pc_setting);
            break;
        }
        else if (!isspace(c))
            break;
    }
    fclose(f);
    return cel;
}

void write_cel_file(CelFile const& cel, const char* path)
{
    for (int i = 1; i != 104; ++i)
        assert(find_Z_in_pse(i)->Z == i);
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "cell  %g %g %g  %g %g %g\n", cel.a, cel.b, cel.c,
                                             cel.alpha, cel.beta, cel.gamma);
    for (vector<AtomInCell>::const_iterator i = cel.atoms.begin();
                                                   i != cel.atoms.end(); ++i) {
        const t_pse* pse = find_Z_in_pse(i->Z);
        fprintf(f, "%-2s   %2d  %g %g %g\n",
                pse->symbol, i->Z, i->x, i->y, i->z);
    }
    int sgn = cel.sgs->sgnumber;
    fprintf(f, "rgnr %d", sgn);
    if (sgn != 1 && (cel.sgs-1)->sgnumber != sgn) {
        fprintf(f, " :");
        if (cel.sgs->ext != 0)
            fprintf(f, "%c", cel.sgs->ext);
        fprintf(f, "%s", cel.sgs->qualif);
    }
    fprintf(f, "\n");
    fclose(f);
}

void write_default_cel_files(const char* path_prefix)
{
    const char* special_str = "\nfilename ";
    for (const char** s = default_cel_files; *s != NULL; ++s) {
        const char* fn = strstr(*s, special_str) + strlen(special_str);
        string filename = string(path_prefix) + fn;
        FILE *f = fopen(filename.c_str(), "w");
        if (!f)
            continue;
        fprintf(f, "%s\n", *s);
        fclose(f);
    }
}


