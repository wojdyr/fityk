// Author: Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$
//
// 

#ifndef FITYK_WX_CERIA_H_
#define FITYK_WX_CERIA_H_

#include <vector>
#include <string>
#include "atomtables.h"
#include "sgtables.h"

enum RadiationType
{
    kXRay,
    kNeutron
};

struct TransVec
{
    int x, y, z; // divide by 12. before use
    TransVec() {}
    TransVec(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}
};

// symmetry operations for the space group
struct SgOps
{
    std::vector<SeitzMatrix> seitz; // Seitz matrices
    std::vector<TransVec> tr; // translation vectors
    bool inv; // has center of inversion
    TransVec inv_t; // translation for centre of inversion
};


std::string fullHM(const SpaceGroupSetting *sgs);

const SpaceGroupSetting* parse_any_sg_symbol(const char *symbol);

struct AtomInCell
{
    int Z;
    double x, y, z;
};

struct CelFile
{
    double a, b, c, alpha, beta, gamma;
    const SpaceGroupSetting* sgs;
    std::vector<AtomInCell> atoms;
};

CelFile read_cel_file(FILE* f);
void write_cel_file(CelFile const& cel, FILE* f);
void write_default_cel_files(const char* path_prefix);


class UnitCell
{
public:
    const double a, b, c, alpha, beta, gamma;
    const double V;
    double M[3][3];
    double M_1[3][3]; // M^-1

    UnitCell(double a_, double b_, double c_,
             double alpha_, double beta_, double gamma_)
        : a(a_), b(b_), c(c_), alpha(alpha_), beta(beta_), gamma(gamma_),
          V(calculate_V())
            { set_M(); }

    double calculate_V() const;

    // pre: V should be set
    void set_M();

    UnitCell get_reciprocal() const;

    // returns |v|, where v = M_1 * [h k l]; 
    double calculate_distance(double h, double k, double l) const;

    // calculate interplanar distance
    double calculate_d(int h, int k, int l) const;
};

struct Miller
{
    int h, k, l;
};

struct Plane : public Miller
{
    int multiplicity;
    double F2; // |F_hkl|^2 (_not_ multiplied by multiplicity)

    Plane() {}
    Plane(Miller const& hkl) : Miller(hkl), multiplicity(1), F2(0.){}
};

struct PlanesWithSameD
{
    std::vector<Plane> planes;
    double d;
    double lpf; // Lorentz-polarization factor
    double intensity; // total intensity = lpf * sum(multiplicity * F2)
    bool enabled; // include this peak in model

    bool operator<(const PlanesWithSameD& p) const { return d > p.d; }
    void add(Miller const& hkl, const SgOps& sg_ops);
    double stol() const { return 1. / (2 * d); } // returns sin(theta)/lambda
};

struct Pos // position in unit cell, 0 <= x,y,z < 1
{
    double x, y, z;
};

struct Atom
{
    char symbol[8];
    // Contains positions of all symmetrically equivalent atoms in unit cell.
    // pos[0] contains the "original" atom.
    std::vector<Pos> pos;
    const t_it92_coeff *xray_sf;
    const t_nn92_record *neutron_sf;

    Atom() : pos(1) { pos[0].x = pos[0].y = pos[0].z = 0.; }
};


// crystal system, we keep it compatible with SgLite
enum CrystalSystem
{
    UndefinedSystem    = 0,
    TriclinicSystem    = 2,
    MonoclinicSystem   = 3,
    OrthorhombicSystem = 4,
    TetragonalSystem   = 5,
    TrigonalSystem     = 6,
    HexagonalSystem    = 7,
    CubicSystem        = 8
};

CrystalSystem get_crystal_system(int space_group);
const char* get_crystal_system_name(CrystalSystem xs);

class Crystal
{
public:
    const SpaceGroupSetting* sgs; // space group
    SgOps sg_ops; // symmetry operations
    UnitCell* uc;
    int n_atoms;
    std::vector<Atom> atoms;
    std::vector<PlanesWithSameD> bp; // boundles of hkl planes

    Crystal();
    ~Crystal();
    void set_space_group(const SpaceGroupSetting* name);
    void generate_reflections(double min_d);
    void set_unit_cell(double a, double b, double c,
                       double alpha, double beta, double gamma)
        { delete uc; uc = new UnitCell(a, b, c, alpha, beta, gamma); }
    void update_intensities(RadiationType r, double lambda);
    CrystalSystem xs() const
        { return sgs ? get_crystal_system(sgs->sgnumber) : UndefinedSystem; }
private:
    Crystal(const Crystal&); // disallow copy
    void operator=(const Crystal&); // disallow assign
};

int parse_atoms(const char* s, Crystal& cr);

extern const char *CrystalSystemNames[];
extern const SpaceGroupSetting space_group_settings[];

void add_symmetric_images(Atom& a, const SgOps& sg_ops);

// Returns the order of the space group,
// i.e. the maximum number of symmetry equivalent positions.
int get_sg_order(const SgOps& sg_ops);

struct Anode
{
    const char *name;
    double alpha1, alpha2;
};

extern const Anode anodes[];

#endif // FITYK_WX_CERIA_H_
