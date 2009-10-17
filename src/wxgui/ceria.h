// Author: Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $
//
// 

#ifndef FITYK_WX_CERIA_H_
#define FITYK_WX_CERIA_H_

#include <vector>
#include "atomtables.h"

struct SgOps;

struct SpaceGroupSetting
{
    int sgnumber; // space group number (1-230)
    char ext; // '1', '2', 'H', 'R' or '\0'
    char qualif[5]; // e.g. "-cba" or "b1"

    char HM[11]; // H-M symbol; nul-terminated string
    char Hall[16]; // Hall symbol; nul-terminated string
};

    // number that goes after SG number in PowderCell .cel file (1-18 or 0)
    //char powdercell_setting;
extern const SpaceGroupSetting space_group_settings[];
extern const char* SchoenfliesSymbols[];

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

CelFile read_cel_file(const char* path);
void write_cel_file(CelFile const& cel, const char* path);
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
    Plane(Miller const& hkl) : Miller(hkl), multiplicity(1.), F2(0.){}
};

struct PlanesWithSameD
{
    std::vector<Plane> planes;
    double d;
    double lpf; // Lorentz-polarization factor
    double intensity; // total intensity = lpf * sum(multiplicity * F2)
    bool enabled; // include this peak in model

    bool operator<(const PlanesWithSameD& p) const { return d > p.d; }
    void add(Miller const& hkl, const SgOps* sg_ops);
};

struct Pos // position in unit cell, 0 <= x,y,z < 1
{
    double x, y, z;
};

struct Atom
{
    char symbol[8];
    // Contains positions of all symmetrically equivalent atoms in unit cell.
    // positions[0] contains the "original" atom.
    std::vector<Pos> positions;
    const t_it92_coeff *xray_sf;
    const t_nn92_record *neutron_sf;

    Atom() : positions(1) {}
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

class Crystal
{
public:
    int sg_number; // space group number
    const char* sg_hm; // Hermann-Mauguin symbol
    SgOps *sg_ops; // symmetry operations
    CrystalSystem sg_xs; // crystal system
    UnitCell* uc;
    int n_atoms;
    std::vector<Atom> atoms;
    std::vector<PlanesWithSameD> bp; // boundles of hkl planes

    Crystal();
    ~Crystal();
    void set_space_group(const char* name);
    void generate_reflections(double min_d);
    void set_unit_cell(double a, double b, double c,
                       double alpha, double beta, double gamma)
        { delete uc; uc = new UnitCell(a, b, c, alpha, beta, gamma); }
    void calculate_intensities(double lambda);
private:
    Crystal(const Crystal&); // disallow copy
    void operator=(const Crystal&); // disallow assign
};

extern const char *CrystalSystemNames[];
extern const SpaceGroupSetting space_group_settings[];

CrystalSystem get_crystal_system(int space_group);
const char* get_crystal_system_name(int xs);
void add_symmetric_images(Atom& a, const SgOps* sg_ops);

const SpaceGroupSetting* find_first_sg_with_number(int sgn);

// Returns the order of the space group,
// i.e. the maximum number of symmetry equivalent positions.
int get_sg_order(const SgOps* sg_ops);

struct Anode
{
    const char *name;
    double alpha1, alpha2;
};

extern const Anode anodes[];

extern const char* default_cel_files[];

#endif // FITYK_WX_CERIA_H_
