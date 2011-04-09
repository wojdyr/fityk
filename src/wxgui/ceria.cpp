// Author: Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include "ceria.h"

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <algorithm>

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
        if (*s == '\n') {
            ++s;
            continue;
        }
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
    sg_ops.tr.push_back(TransVec(0, 0, 0)); // always keep trivial tr
    // add unit seitz matrix
    SeitzMatrix sm = { { 1, 0, 0,
                         0, 1, 0,
                         0, 0, 1, },
                       { 0, 0, 0 } };
    sg_ops.seitz.push_back(sm);
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
    sg_ops.tr.resize(1); // leave only the trivial tr (0, 0, 0)
    sg_ops.inv = false;
    sg_ops.inv_t.x = sg_ops.inv_t.y = sg_ops.inv_t.z = 0;
    if (sgs == NULL)
        return;

    int n_seitz = get_seitz_mx_count(sgs);
    // the first seitz matrix is unit matrix
    sg_ops.seitz.resize(n_seitz + 1);
    for (int i = 0; i != n_seitz; ++i)
        get_seitz_mx(sgs, i, &sg_ops.seitz[i+1]);

    switch (sgs->Hall[1])
    {
        case 'A': sg_ops.tr.push_back(TransVec(0,6,6)); break;
        case 'B': sg_ops.tr.push_back(TransVec(6,0,6)); break;
        case 'C': sg_ops.tr.push_back(TransVec(6,6,0)); break;
        case 'I': sg_ops.tr.push_back(TransVec(6,6,6)); break;
        case 'P': break;
        case 'R': sg_ops.tr.push_back(TransVec(8,4,4));
                  sg_ops.tr.push_back(TransVec(4,8,8));
                  break;
        case 'F': sg_ops.tr.push_back(TransVec(0,6,6));
                  sg_ops.tr.push_back(TransVec(6,0,6));
                  sg_ops.tr.push_back(TransVec(6,6,0));
                  break;
        default: assert(0);
    }
    if (sgs->Hall[0] == '-') {
        sg_ops.inv = true;
    }
    else {
        const char* t = strstr(sgs->Hall, " -1");
        if (t != NULL) {
            sg_ops.inv = true;
            t += 3;
            if      (strcmp(t, "ab") == 0) sg_ops.inv_t = TransVec(6, 6, 0);
            else if (strcmp(t, "ac") == 0) sg_ops.inv_t = TransVec(6, 0, 6);
            else if (strcmp(t, "bc") == 0) sg_ops.inv_t = TransVec(0, 6, 6);
            else if (strcmp(t, "ad") == 0) sg_ops.inv_t = TransVec(9, 3, 3);
            else if (strcmp(t, "bw") == 0) sg_ops.inv_t = TransVec(0, 6, 3);
            else if (strcmp(t, "d" ) == 0) sg_ops.inv_t = TransVec(3, 3, 3);
            else if (strcmp(t, "n" ) == 0) sg_ops.inv_t = TransVec(6, 6, 6);
            else assert(0);
        }
    }
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
            int ts[3] = { sg_ops.inv_t.x - T[0],
                          sg_ops.inv_t.y - T[1],
                          sg_ops.inv_t.z - T[2] };
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
    // Don't generate too many reflections (it could happen
    // when user chooses Q instead of 2T, or puts wrong wavelength)
    if (max_h * max_k * max_l > 8000)
        max_h = max_k = max_l = 20;

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
    old_min_d = min_d;
}


// stol = sin(theta)/lambda
void set_F2(Plane& p, const vector<Atom>& atoms,
                 RadiationType radiation, double stol)
{
    // calculating F_hkl, (Pecharsky & Zavalij, eq. (2.89) and (2.103))
    // assuming population g = 1
    // assuming temperature factor t = 1
    double F_real = 0.;
    double F_img = 0;
    for (vector<Atom>::const_iterator i = atoms.begin(); i != atoms.end(); ++i){
        double f = 1.;
        if (radiation == kXRay && i->xray_sf)
            f = calculate_it92_factor(i->xray_sf, stol*stol);
        else if (radiation == kNeutron && i->neutron_sf)
            f = i->neutron_sf->bond_coh_scatt_length;
        for (vector<Pos>::const_iterator j = i->pos.begin();
                                                j != i->pos.end(); ++j) {
            double hx = p.h * j->x + p.k * j->y + p.l * j->z;
            F_real += f * cos(2*M_PI * hx);
            F_img += f * sin(2*M_PI * hx);
        }
    }
    p.F2 = F_real*F_real + F_img*F_img;
    //printf("hkl=(%d %d %d) F=(%g, %g) F2=%g\n",
    //       p.h, p.k, p.l, F_real, F_img, p.F2);
}

void set_lpf(PlanesWithSameD &bp, RadiationType radiation, double lambda)
{
    if (lambda == 0)
        bp.lpf = 1.;
    else {
        double T = asin(bp.stol() * lambda); // theta
        if (radiation == kXRay) {
            // for x-rays, we assume K=0.5 and
            // LP = (1 + cos(2T)^2) / (cos(T) sin(T)^2)
            //  (Pecharsky & Zavalij, eq. (2.70), p. 192)
            bp.lpf = (1 + cos(2*T)*cos(2*T)) / (cos(T)*sin(T)*sin(T));
        }
        else if (radiation == kNeutron) {
            // Kisi & Howard, Applications of Neutron Powder Diffraction (2.38)
            // no polarization only the Lorentz factor:
            // L = 1 / (4 sin^2(T) cos(T))
            bp.lpf = 1 / (4 * sin(T)*sin(T)*cos(T));
            // for TOF diffractometers with a fixed diffraction angle:
            // L = d^4 sin(theta)
        }
    }
}

void Crystal::update_intensities(RadiationType r, double lambda)
{
    if (atoms.empty())
        return;
    for (vector<PlanesWithSameD>::iterator i = bp.begin(); i != bp.end(); ++i) {
        set_lpf(*i, r, lambda);
        double t = 0;
        for (vector<Plane>::iterator j = i->planes.begin();
                                     j != i->planes.end(); ++j) {
            set_F2(*j, atoms, r, i->stol());
            t += j->multiplicity * j->F2;
        }
        i->intensity = i->lpf * t;
    }
}

double UnitCell::calculate_V() const
{
    // Giacovazzo p.62
    double cosA = cos(alpha), cosB = cos(beta), cosG = cos(gamma);
    double t = 1 - cosA*cosA - cosB*cosB - cosG*cosG + 2*cosA*cosB*cosG;
    return a*b*c * sqrt(t);
}

/*
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
*/

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


const char* default_cel_files[][2] = {

{"bSiC",
"cell  4.358 4.358 4.358  90 90 90\n"
"Si   14  0 0 0\n"
"C     6  0.25 0.25 0.25\n"
"rgnr 216"
},

{"aSiC",
"cell  3.082 3.082 15.123  90 90 120\n"
"SI1  14  0       0       0\n"
"SI2  14  0.3333  0.6667  0.1667\n"
"SI3  14  0.3333  0.6667  0.8333\n"
"C1    6  0.0     0.0     0.125\n"
"C2    6  0.3333  0.6667  0.2917\n"
"C3    6  0.3333  0.6667  0.9583\n"
"rgnr 186",
},

{"NaCl",
"cell  5.64009 5.64009 5.64009  90 90 90\n"
"Na   11   0   0   0\n"
"Cl   17   0.5 0   0\n"
"rgnr 225"
},

{"diamond",
"cell  3.5595 3.5595 3.5595  90 90 90\n"
"C     6  0 0 0\n"
"rgnr 227"
},

{"Si",
"cell  5.4309 5.4309 5.4309  90 90 90\n"
"Si   14  0 0 0\n"
"rgnr 227"
},

{"CeO2",
"cell  5.41 5.41 5.41  90 90 90\n"
"Ce   58  0   0   0\n"
"O     8  0.25 0.25 0.25\n"
"rgnr 225"
},

{"Zn",
"cell  2.665 2.665 4.946  90 90 120\n"
"Zn   30  0.33333 0.66667 0.25\n"
"rgnr 194"
},

{NULL, NULL}
};


CelFile read_cel_file(FILE *f)
{
    CelFile cel;
    cel.sgs = NULL;
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
    return cel;
}

void write_cel_file(CelFile const& cel, FILE *f)
{
    //for (int i = 1; i != 104; ++i)
    //    assert(find_Z_in_pse(i)->Z == i);
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
    if (sgn != 1 && (cel.sgs-1)->sgnumber == sgn) {
        fprintf(f, " :");
        if (cel.sgs->ext != 0)
            fprintf(f, "%c", cel.sgs->ext);
        fprintf(f, "%s", cel.sgs->qualif);
    }
    fprintf(f, "\n");
}

void write_default_cel_files(const char* path_prefix)
{
    for (const char*(*s)[2] = default_cel_files; (*s)[0] != NULL; ++s) {
        string filename = string(path_prefix) + (*s)[0] + ".cel";
        FILE *f = fopen(filename.c_str(), "w");
        if (!f)
            continue;
        fprintf(f, "%s\n", (*s)[1]);
        fclose(f);
    }
}


