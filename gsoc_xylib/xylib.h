// XYlib library is a xy data reading library, aiming to read variaty of xy 
// data formats.
// Licence: GNU General Public License version 2
// $Id: xylib.h $

#ifndef XYLIB__API__H__
#define XYLIB__API__H__


#ifndef __cplusplus
#error "This library does not have C API."
#endif


#ifdef FP_IS_LDOUBLE
typedef long double fp;  
#else
typedef double fp;  
#endif

#include <string>
#include <vector>
#include <cstdio>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace xylib
{


class XY_FileInfo
{
public:
    std::string ext_name;
    std::string type_desc;
    std::string version;
};


class XY_Point
{
public:
    fp x, y, sig;
    XY_Point(fp x_, fp y_, fp sig_ = 0) : x(x_), y(y_), sig(sig_) {};
};



class XY_Error :
    public std::runtime_error
{
public:
    XY_Error(const std::string& msg) : std::runtime_error(msg) {};
};


class XY_Data
{
public:
    bool has_sigma;

    // methods
    //////////////////////////////////////////////////////////////////////////
    XY_Data() : has_sigma(false) {};
    
    int get_point_count() {return p.size();}
    fp get_x(const int i) {return p[i].x;}
    fp get_y(const int i) {return p[i].y;}
    fp get_sigma(const int i) {return p[i].sig;}
    void set_has_sigma(bool has_sigma_) {has_sigma = has_sigma_;}

    void push_point(const XY_Point pt) {p.push_back(pt);}
    void clear() {p.clear();}

    void export_xy_file(const std::string& fname)
    {
        using namespace std;
        std::ofstream of(fname.c_str());
        if (!of) 
        {
            throw XY_Error("can't create file " + fname + " to output");
        }
        int n = p.size();
        of << "total count:" << n << std::endl << std::endl;
        of << "x\ty\tsigma" << std::endl;
        for (int i=0; i<n; ++i )
        {
            of << setfill(' ') << setprecision(5) << setw(7) << 
                p[i].x << "\t" << setprecision(8) << setw(10) << p[i].y << "\t";
            if (has_sigma)
            {
                of << p[i].sig;
            }
            of << std::endl;
        }
    }

private:
    std::vector<XY_Point> p;
};


class XYlib
{
public:
    // attributes
    //////////////////////////////////////////////////////////////////////////


    // methods
    //////////////////////////////////////////////////////////////////////////
    XYlib(void);
    ~XYlib(void);

    static void load_file(std::string const& fname, XY_Data& data, std::string const& type = "");

private:
    // plain text file with data columns
    static void load_xy_file(std::ifstream& f, XY_Data& data, std::vector<int> const& columns);

    // SIEMENS/BRUKER uxd format
    static void load_uxd_file(std::ifstream& f, XY_Data& data, std::vector<int> const& columns);

    // DIFFRAC-AT Raw Data file format: v2 and v3
    static void load_diffracat_v2_raw_file(std::ifstream& f, XY_Data& data, 
        unsigned range = 0);
    

    static int read_line_and_get_all_numbers(std::istream &is, 
        std::vector<fp>& result_numbers);
    static std::string guess_ftype(std::string const fname, std::istream& is, XY_FileInfo* pfi = NULL);
    // set XY_FileInfo structrue
    void set_file_info(const std::string& ftype, XY_FileInfo* pfi);
};

}

#endif //#ifndef XYLIB__API__H__
