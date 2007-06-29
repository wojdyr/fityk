// XYlib library is a xy data reading library, aiming to read variaty of xy 
// data formats.
// Licence: GNU General Public License version 2
// $Id: xylib.h $

#include "xylib.h"
#include "common.h"
#include <algorithm>
#include <boost/detail/endian.hpp>
#include <boost/cstdint.hpp>
#include <boost/regex.hpp>

using namespace std;

namespace xylib
{

XYlib::XYlib(void)
{
}

XYlib::~XYlib(void)
{
}

//////////////////////////////////////////////////////////////////////////
// some functions are borrowed from fityk v0.8.1 code with necessary modifications
void XYlib::load_file(std::string const& file, XY_Data& data, std::string const& type /* =  */)
{
    ifstream f (file.c_str(), ios::in | ios::binary);
    if (!f) {
        throw XY_Error("Can't open file: " + file);
    }

    string const& ft = type.empty() ? guess_ftype(file, f) // "detect" format
        : type;

    data.clear(); //removing previous file
    
    vector<int> col;
    col.push_back(1);
    col.push_back(2);

    if (ft == "text")                         // x y x y ... 
        load_xy_file(f, data, col);
    else if (ft == "uxd")
        load_uxd_file(f, data, col);
    else if (ft == "diffracat_v1_raw")
        load_diffracat_v1_raw_file(f, data);
    else if (ft == "diffracat_v2_raw")
        load_diffracat_v2_raw_file(f, data);
    else if (ft == "rigaku_dat")
        load_rigaku_dat_file(f, data);
    else if (ft == "vamas_iso14976")
        load_vamas_file(f, data);
    else {                                  // other ?
        throw XY_Error("Unknown filetype.");
    }
}

void XYlib::load_xy_file (ifstream& f, XY_Data& data, vector<int> const& columns)
{
    /* format  x y \n x y \n x y \n ...
    *           38.834110      361
    *           38.872800  ,   318
    *           38.911500      352.431
    * delimiters: white spaces and  , : ;
    */
    if (columns.size() == 1 || columns.size() > 3) //0, 2, or 3 columns allowed
        throw XY_Error("If columns are specified, two or three of them"
        " must be given (eg. 1,2,3)");
    vector<int> const& cols = columns;
    vector<fp> xy;
    data.has_sigma = (columns.size() == 3);
    int maxc = *max_element (cols.begin(), cols.end());
    int minc = *min_element (cols.begin(), cols.end());
    if (minc < 0) {
        throw XY_Error("Invalid (negative) column number: " + S(minc));
    }

    //TODO: optimize loop below
    //most of time (?) is spent in getline() in read_line_and_get_all_numbers()
    while (read_line_and_get_all_numbers(f, xy)) {
        if (xy.empty()) {
            continue;
        }

        if (size(xy) < maxc) {
            continue;
        }

        // TODO: need modify, (use col[] specified columns)
        // so far, it is assumed that col 1,2,3 are x, y, sigma
        fp x = xy[cols[0] - 1];
        fp y = xy[cols[1] - 1];
        if (cols.size() == 2)
            data.push_point(XY_Point(x, y));
        else if (cols.size() == 3){
            fp sig = xy[cols[2] - 1];
            if (sig > 0) 
                data.push_point(XY_Point(x, y, sig));
        }
    }
}


void XYlib::load_uxd_file(ifstream& f, XY_Data& data, vector<int> const& columns)
{
    /* format example
    ; C:\httpd\HtDocs\course\content\week-1\sample5.raw - (Diffrac AT V1) File converted by XCH V1.0
    _FILEVERSION=1
    _SAMPLE='D2'.
    ...
    ; (Data for Range number 1)
    _DRIVE='COUPLED'
    _STEPTIME=39.000000
    _STEPSIZE=0.020000
    _STEPMODE='C'
    _START=4.990000
    _2THETA=4.990000
    _THETA=0.000000
    _KHI=0.000000
    _PHI=0.000000
    _COUNTS
    6234      6185      5969      6129      6199      5988      6046      5922
    ...
    */
    data.clear();
/*
    string data_flg = "_COUNTS";
    int data_flg_l = data_flg.size();
*/

    fp x_start, x_step;

    string line;
    while (getline(f, line))
    {
        if (line.empty() || line[0] == ';' || line.find_first_not_of(" \t\r\n") == string::npos)
            continue;     // skip meaningless lines 
        if (line.substr(0, 7) == "_START=") 
        {
            istringstream(line.substr(7)) >> x_start;
            continue;
        }
        if (line.substr(0, 10) == "_STEPSIZE=")
        {
            istringstream(line.substr(10)) >> x_step;
            continue;
        }

        if (line.substr(0, 7) == "_COUNTS")
        {
            int i = 0;
            while (getline(f, line))
            {
                fp val;
                istringstream iss(line);
                while (iss >> val)
                {
                    data.push_point(XY_Point(x_start + i * x_step, val));
                    ++i;
                }
            }
        }
    }
}


// SIEMENS/BRUKER version 1 raw file
void XYlib::load_diffracat_v1_raw_file(std::ifstream& f, XY_Data& data, 
                                       unsigned range /*= 0*/)
{
    using namespace boost;

    // TODO: check whether this is Diffract-At v1 format (move the verify code to another funciton)
    char buf[256];

    f.clear();

    // get file header
    f.seekg(0);
    f.read(buf, 3);

    if (f.rdstate() & ios::failbit) 
    {
        throw XY_Error("error when reading file head");
    }

    if (0 != strncmp(buf, "RAW", 3))
    {
        throw XY_Error("file is not Diffrac-at v1 file");
    }

    // get the max range index
    uint32_t max_range_idx;
    f.seekg(152);
    f.read(reinterpret_cast<char*>(&max_range_idx), sizeof(max_range_idx));
    le_to_host(&max_range_idx, sizeof(max_range_idx));
    
    if (range > max_range_idx)
    {
        throw XY_Error(string("file does not have range") + S(range));
    }

    unsigned cur_range = 0;
    unsigned cur_range_offset = 0;
    uint32_t cur_range_steps;
    float start_x, step_size;

    // skip the previous ranges
    while (cur_range < range) 
    {
        f.seekg(cur_range_offset + 4);
        f.read(reinterpret_cast<char*>(&cur_range_steps), sizeof(cur_range_steps));
        le_to_host(&cur_range_steps, sizeof(cur_range_steps));

        cur_range_offset += (39 + cur_range_steps) * 4;
    }
    
    // get step count of specified range
    f.seekg(cur_range_offset + 4);
    f.read(reinterpret_cast<char*>(&cur_range_steps), sizeof(cur_range_steps));
    le_to_host(&cur_range_steps, sizeof(cur_range_steps));

    // get start_x (2-theta) and step_size
    f.seekg(cur_range_offset + 12);
    f.read(reinterpret_cast<char*>(&step_size), sizeof(step_size));
    le_to_host(&cur_range_offset, sizeof(cur_range_offset));
    f.seekg(cur_range_offset + 24);
    f.read(reinterpret_cast<char*>(&start_x), sizeof(start_x));
    le_to_host(&start_x, sizeof(start_x));

    // get data
    float x, y;
    for (unsigned i=0; i<cur_range_steps; ++i)
    {
        f.seekg(cur_range_offset + 156 + i * 4);
        f.read(reinterpret_cast<char*>(&y), sizeof(y));
        le_to_host(&y, sizeof(y));
        x = start_x + i * step_size;
        data.push_point(XY_Point(x, y));
    }
}


// SIEMENS/BRUKER version 2 and version 3 raw file
void XYlib::load_diffracat_v2_raw_file(std::ifstream& f, XY_Data& data, 
                                unsigned range /*= 0*/)
{
    // TODO: check whether this is Diffract-At v2/3 format (move the verify code to another funciton)
    char buf[256];

    f.clear();

    // get file header
    f.seekg(0);
    f.read(buf, 4);

    if (f.rdstate() & ios::failbit) 
    {
        throw XY_Error("error when reading filehead");
    }

    if (0 != strncmp(buf, "RAW2", 4))
    {
        throw XY_Error("file is not Diffrac-at v2/v3 file");
    }

    // get number of ranges 
    unsigned short ranges = 0;
    f.seekg(4);
    f.read(reinterpret_cast<char*>(&ranges), sizeof(ranges));
    if (range >= ranges)
    {
        sprintf(buf, "file does not have range %d", range);
        throw XY_Error(string(buf));
    }

    // get header of current range
    unsigned short cur_range_start = 256;
    unsigned short cur_header_len = 0;

    // skip the previous data ranges
    for (unsigned cur_range = 0; cur_range < range; ++cur_range)
    {
        f.seekg(cur_range_start);
        f.read(reinterpret_cast<char*>(&cur_header_len), sizeof(cur_header_len));
        cur_range_start += cur_header_len;
    }
    
    // read the specified range data
    //////////////////////////////////////////////////////////////////////////
    unsigned short step_count;
    float x_step, x_start;

    // range header length
    f.seekg(cur_range_start);
    f.read(reinterpret_cast<char*>(&cur_header_len), sizeof(cur_header_len));

    // step count
    f.seekg(cur_range_start + 2);
    f.read(reinterpret_cast<char*>(&step_count), sizeof(step_count));
    
    // step size
    f.seekg(cur_range_start + 12);
    f.read(reinterpret_cast<char*>(&x_step), sizeof(x_step));

    // according to data format specification, here these 24B is of FORTRAN type "6R4"
    // starting axes position for range
    // I find ff[0] is the x coordinate of starting point, and don't konw what others mean
    float ff[6];
    for (int i=0; i<6; ++i)
    {
        f.seekg(cur_range_start + 16 + i * 4);
        f.read(reinterpret_cast<char*>(&ff[i]), sizeof(ff[i]));
    }
    x_start = ff[0];

    // get the x-y data points
    float x, y;
    for (int i=0; i<step_count; ++i)
    {
        f.seekg(cur_range_start + cur_header_len + i * sizeof(y));
        f.read(reinterpret_cast<char*>(&y), sizeof(y));
        x = x_start + i * x_step;
        data.push_point(XY_Point(x, y));
    }
}


void XYlib::load_rigaku_dat_file(std::ifstream& f, XY_Data& data,
                          unsigned range /*= 0*/)
{
    // for details of this format, see docs/formats.

    using namespace boost;
    
    unsigned range_cnt = 0, cur_range = 0;
    fp x_start = 0, x_step = 0;
    string line;

    // verify with magic number: first line must start with "*TYPE"
    if (!(getline(f, line) && line.substr(0, 5) == "*TYPE"))
    {
        throw XY_Error("error when checking file format");
    }

    cmatch what;
    regex range_cnt_ptn("[*]GROUP_COUNT\\s*=\\s*(\\d*)\\s*");
    regex range_idx_ptn("[*]GROUP\\s*=\\s*(\\d*)\\s*");

    while (getline(f, line))
    {
        if (regex_match(line.c_str(), what, range_cnt_ptn))
        {
            range_cnt = atoi(what[1].first);
            if (range >= range_cnt)
            {
                throw XY_Error(S("file does not have range") + S(range));
            }
            continue;
        }

        // skip the previous ranges before specified
        while (cur_range < range)
        {
            while (getline(f, line))
            {
                if (line.substr(0, 4) == "*END")
                {
                    ++cur_range;
                    break;
                }
            }
        }
        
        if (cur_range == range && range_cnt != 0)
            break;
    }

    // this code block is separated from the above "while" for efficiency consideration
    regex x_start_ptn("[*]START\\s*=\\s*(\\d*[.]?\\d*)\\s*");
    regex x_step_ptn("[*]STEP\\s*=\\s*(\\d*[.]?\\d*)\\s*");
    while (getline(f, line))
    {
        if (regex_match(line.c_str(), what, x_start_ptn))
        {
            x_start = string_to_fp(what[1].first);
            continue;
        }

        if (regex_match(line.c_str(), what, x_step_ptn))
        {
            x_step = string_to_fp(what[1].first);
            break;
        }
    }

    // also for efficiency consideration
    vector<fp> y_one_line(4);
    int y_count_line;
    int y_idx = 0;
    while (getline(f, line))
    {
        if (line.substr(0, 6) == "*COUNT")
        {
            // Y data starts
            while (read_line_and_get_all_numbers(f, y_one_line))
            {
                y_count_line = y_one_line.size();
                for (int i=0; i<y_count_line; ++i)
                {
                    data.push_point(
                        XY_Point(x_start + y_idx * x_step, y_one_line[i]));
                    ++y_idx;
                }
            }
        }
    }
}


// load the vamas_iso14976 file
void XYlib::load_vamas_file(std::ifstream& f, XY_Data& data,
                                 unsigned range /*= 0*/)
{
    // for details of this format, see docs/formats.
    /** For description of the format see
    W.A. Dench, L. B. Hazell and M. P. Seah,
    VAMAS Surface Chemical Analysis Standard Data Transfer Format
    with Skeleton Decoding Programs.
    Surface and Interface Analysis, 13(1988)63-122
    or National Physics Laboratory Report DMA(A)164 July 1988 */
    
    int total_regions, blk_cnt;
    string line;
    int n, n2;  // n2 is "entries_in_inclusion_list"

    // verify the file format
    string magic = "VAMAS Surface Chemical Analysis Standard Data Transfer Format 1988 May 4";
    if (!(getline(f, line) && trim(line) == magic)) 
    {
        throw XY_Error("file is not in vamas_iso14976 format");
    }

    // skip "institution id" etc, 4 items
    skip_lines(f, 4);

    // comment lines, n is nr of lines
    n = read_line_int(f);
    skip_lines(f, n);
    
    // experiment mode
    getline(f, line);
    line = trim(line);
    int exp_mode = find_exp_mode_value(line.c_str());

    // scan_mode
    getline(f, line); 
    line = trim(line);
    int scan_mode = find_scan_mode_value(line.c_str());

    // Experiment mode is MAP, MAPD, NORM, or SDP
    if ((exp_mode < 3) ||
        (exp_mode == 5) ||
        (exp_mode == 6))	
    {
        total_regions = read_line_int(f);
    }

    // experiment mode is MAP or MAPD
    if (exp_mode < 3)
    {
        int i = read_line_int(f);	// number of analysis positions
        i = read_line_int(f);	    // number of discrete x coordinates available in full map
        i = read_line_int(f);	    // number of discrete y coordinates available in full map
    };

    // experimental variables
    int exp_variables = read_line_int(f);
    for (int i = 1; i <= exp_variables; ++i)
    {
        getline(f, line);   // experimental labels
        getline(f, line);   // experimental units
    }

    /* n2 indicates the number of lines below */
    n2 = read_line_int(f);	// number of entries in parameter inclusion or exclusion list
    bool d = (n2 <= 0);

    // a complete blk contains 40 parts. include[i] indicates if the i-th part is included 
    vector<bool> include(41, d);  

    d = !d;
    n2 = (n2 < 0) ? -n2 : n2;
    for (int i = 1; i <= n2; ++i)
    {
        int e = read_line_int(f);
        include[e] = d;
    }
    
    // number of manually entered items in blk
    n = read_line_int(f);
    skip_lines(f, n);

    // number of future upgrade experiment entries
    int exp_future_upgrade_entries = read_line_int(f);
    skip_lines(f, exp_future_upgrade_entries);

    // number of future upgrade block entries
    int block_future_upgrade_entries = read_line_int(f);
    skip_lines(f, block_future_upgrade_entries);

    // handle the blocks
    blk_cnt = read_line_int(f);
    if (range < 0 || range >= blk_cnt)
    {
        throw XY_Error(S("file does not have range") + S(range));
    }

    // skip the previews blocks
    for (unsigned i = 0; i < range; ++i)
    {
        vamas_read_blk(f, data, include, true);
    }
    
    vamas_read_blk(f, 
        data, include, false, 
        block_future_upgrade_entries,
        exp_future_upgrade_entries,
        exp_mode, 
        scan_mode, 
        exp_variables);
}

// helper functions
//////////////////////////////////////////////////////////////////////////

// read one blk, used by load_vamas_file()
void XYlib::vamas_read_blk(std::ifstream &f, 
                           XY_Data& data, 
                           const std::vector<bool> &include,
                           bool skip_flg /* = false */, 
                           int block_future_upgrade_entries /* = 0 */,
                           int exp_future_upgrade_entries /* = 0 */,
                           int exp_mode /* = 0 */, 
                           int scan_mode /* = 0 */, 
                           int exp_variables /* = 0 */)
{
    string line;
    int n;
    int corresponding_var, tech;
    fp x_start, x_step;
    int pt_cnt;          // number of points

    skip_lines(f, 2);   // block headers
    
    // all block header lines
    for (int i = 1; i <= 40; ++i)
    {
        if (include[i])
        {
            switch (i)
            {
            case 8:     // block comments
                n = read_line_int(f);
                skip_lines(f, n);
                break;

            case 9:     // get tech
                getline(f, line);
                line = trim(line);
                tech = find_technique_value(line.c_str());
                break;

            case 10:    // additional lines iff exp_mode is MAP or MAPDP
                if (exp_mode < 3)
                {
                    skip_lines(f, 2); 
                }
                break;

            case 11:    // values of exp_variables
                skip_lines(f, exp_variables);
                break;

            case 13:    
                // additional lines iff exp_mode is MAPDP, MAPSVDP, SDP, SDPSV
                // or tech = FABMS, FABMS eneergy spec, SIMS, sims energy spec, SNMS, SNMSenergy spec or ISS
                if ((exp_mode == 2) ||
                    (exp_mode == 4) ||
                    (exp_mode == 6) ||
                    (exp_mode == 7) ||
                    (tech > 4 && tech < 12))
                {
                    skip_lines(f, 3);
                }
                break;

            case 16:    // analysis source beam width X and Y
                skip_lines(f, 2);
                break;

            case 17:
                // additional lines iff exp_mode is MAP,MAPDP,MAPSV,MAPSVDP or SEM
                if (exp_mode < 5 || exp_mode == 8)
                {
                    skip_lines(f, 2);
                }
                break;

            case 18:    // MAPPING mode, get args here
                // NOTICE: in MAPPING mode, every (x, y) is mapped to the "ordinate values" in the block body
                // so, this is a 2 z=f(x,y)
                // What should we do in fityk??
                if (exp_mode == 3 ||
                    exp_mode == 4 ||
                    exp_mode == 8)	// MAPSV,MAPSVDP or SEM
                {
                    throw XY_Error("do not support MAPPING mode now");
                    //skip_lines(f, 6);
                }
                break;

            case 23:
                if (1 == tech)
                {
                    skip_lines(f, 1);
                }
                break;

            case 27:    // 2 lines: analysis width X & Y
            case 28:    // 2 lines: analyser axis take off "polar angle" & "azimuth"
            case 30:    // 2 lines: "transition or charge state label" & "charge of detected particle"
                skip_lines(f, 2);
                break;

            case 31:    // REGULAR mode, get x_start & x_step
                if (1 == scan_mode)     // 'REGULAR' mode
                {
                    skip_lines(f, 2);   // abscissa label & units
                    getline(f, line);
                    x_start = string_to_fp(line);
                    getline(f, line);
                    x_step = string_to_fp(line);
                }
                break;

            case 32:
                corresponding_var = read_line_int(f);
                skip_lines(f, 2 * corresponding_var);   // 2 lines per corresponding_var
                break;

            case 37:
                if ((tech < 5 || tech > 11)	// AES2,AES1,EDX,ELS,UPS,XPS or XRF
                    &&
                    (exp_mode == 2 ||
                    exp_mode == 4 ||
                    exp_mode == 6 ||
                    exp_mode == 7))	// MAPDP,MAPSVDP,SDP or SDPSV
                {   // block params: sputtering source energy,beam current, 
                    // width X, width Y, polar angle of incidence, azimuth, mode
                    skip_lines(f, 7);
                }
                break;

            case 38:
                skip_lines(f, 2);
                break;

            case 40:    // 3 lines per param: additional numerical parameter label, units, value
                n = read_line_int(f);
                skip_lines(f, 3 * n);
                break;

            default:    // simply read one line, which we do not care
                getline(f, line);
                break;
            } // switch
        } // if
    } // for

    skip_lines(f, block_future_upgrade_entries);
    pt_cnt = read_line_int(f);
    skip_lines(f, 2 * corresponding_var);   // min & max ordinate
    
    if (skip_flg)
    {
        // if skip_flg is set, point will not be added to data
        skip_lines(f, pt_cnt);
        return;
    }

    fp y;
    for (int c = 0; c < pt_cnt; ++c)
    {
        y = read_line_fp(f);
        data.push_point(XY_Point(x_start + c * x_step, y));
    }
}


// guess an unknown file format type. may be expanded by deciding the file 
// type according to the first lines of file
string XYlib::guess_ftype(std::string const fname, std::istream & is,
                          XY_FileInfo* pfi/* = NULL */)
{
    //return "text";
    string::size_type pos = fname.find_last_of('.');

    if (string::npos == pos)
    {
        return "unknown";
    }

    string ext = fname.substr(pos + 1);
    //transform(ext.begin(), ext.end(), ext.begin(), tolower);
    for (string::iterator it = ext.begin(); it != ext.end(); ++it)
    {
        *it = tolower(*it);
    }


    if ("txt" == ext)
    {
        if (pfi)
        {
            pfi->ext_name = ext;
            pfi->type_desc = "text";
            pfi->version = "";
        }
        return "text";
    }
    else if ("uxd" == ext)
    {
        if (pfi)
        {
            pfi->ext_name = ext;
            pfi->type_desc = "SIEMENS/BRUKER ASCII UXD";
            pfi->version = "";
        }
        return "uxd";
    }
    else
    {
        return "unknown";
    }
}


// TODO: this function can be rewritten with boost::regex if necessary
int XYlib::read_line_and_get_all_numbers(istream &is, vector<fp>& result_numbers)
{
    // returns number of numbers in line
    result_numbers.clear();
    string s;
    while (getline(is, s) && 
        (s.empty() || s[0] == '#' 
        || s.find_first_not_of(" \t\r\n") == string::npos))
        ;//ignore lines with '#' at first column or empty lines
    for (string::iterator i = s.begin(); i != s.end(); i++) {
        if (*i == ',' || *i == ';' || *i == ':') {
            *i = ' ';
        }
    }

    istringstream q(s);
    fp d;
    while (q >> d) {
        result_numbers.push_back(d);
    }
    return !is.eof();
}


}


// set XY_FileInfo structrue
void xylib::XYlib::set_file_info(const std::string& ftype, XY_FileInfo* pfi)
{
    if (NULL == pfi)
    {
        return;
    }

    if ("text" == ftype) {
        pfi->ext_name;
        // to be completed, or try to change the interface to do such things?
    }
}

int xylib::XYlib::read_line_int(std::ifstream& is)
{
    string str;
    getline(is, str);
    return string_to_int(str);
}


fp xylib::XYlib::read_line_fp(std::ifstream& is)
{
    string str;
    getline(is, str);
    return string_to_fp(str);
}

// convert a std::string to fp
fp xylib::XYlib::string_to_fp(const std::string &str)
{
    fp ret;
    istringstream is(str);
    is >> ret;
    return ret;
}


// convert a std::string to int
int xylib::XYlib::string_to_int(const std::string &str)
{
    int ret;
    istringstream is(str);
    is >> ret;
    return ret;
}

// trim the string 
std::string xylib::XYlib::trim(const string &str)
{
    basic_string <char>::size_type first, last;
    first = str.find_first_not_of(" \r\n\t");
    last = str.find_last_not_of(" \r\n\t");
    return str.substr(first, last - first + 1);
}

// skip "count" lines in f
void xylib::XYlib::skip_lines(ifstream &f, const int count)
{
    string line;
    for (int i = 0; i < count; ++i)
    {
        if (!getline(f, line))
        {
            throw XY_Error("unexpected end of file");
        }
    }
}


//////////////////////////////////////////////////////////////////////////
// following 3 functions are used by load_vamas_file()

// get the index number of "exp_mode"
int xylib::XYlib::find_exp_mode_value(const char *string)
{
    static char *name[] =
    {
        "MAP","MAPDP","MAPSV","MAPSVDP","NORM","SDP","SDPSV","SEM","NOEXP"
    };

    int k = sizeof(name) / sizeof(name[0]);

    int i = 0;
    while ((i < k) && strcmp(string, name[i++]))
    {
        continue;
    }

    if (i == k)
    {
        i = -1;
    }
    
    return i;
}

// get the index number of "technique_mode"
int xylib::XYlib::find_technique_value(const char *string)
{
    static char *name[] =
    {
        "AES diff","AES dir","EDX","ELS","FABMS",
        "FABMS energy spec","ISS","SIMS","SIMS energy spec","SNMS",
        "SNMS energy spec","UPS","XPS","XRF"
    };
    int k = sizeof(name) / sizeof(name[0]);
    int i = 0;
    while ((i < k) && strcmp(string, name[i++]))
    {
        continue;
    }

    if (i == k)
    {
        i = -1;// Not found
    }

    return i;
}

// get the index number of "scan_mode"
int xylib::XYlib::find_scan_mode_value(const char *string)
{
    static char *name[] = {"REGULAR","IRREGULAR","MAPPING"};
    int k = sizeof(name) / sizeof(name[0]);
    int i = 0;
    while ((i < k) && strcmp(string, name[i++]))
    {
        continue;
    }

    if (i == k)
    {
        i = -1;// Not found
    }

    return i;
}

// change the byte-order from "little endian" to host endian
// p: pointer to the data
// len: length of the data type, should be 1, 2, 4
void xylib::XYlib::le_to_host(void *p, unsigned len)
{
    /*
    for more info, see "endian" and "pdp-11" items in wikipedia
    www.wikipedia.org
    */
    if ((len != 1) && (len != 2) && (len != 4))
    {
        throw XY_Error("len should be 1, 2, 4");
    }

# if defined(BOOST_LITTLE_ENDIAN)
    // nothing need to do
    return;
# else   // BIG_ENDIAN or PDP_ENDIAN
    // store the old values
    uint8_t byte0, byte1, byte2, byte3;
    switch (len)
    {
    case 1:
        break;
    case 2:
        byte0 = (reinterpretcast<uint8_t*>(p))[0];
        byte1 = (reinterpretcast<uint8_t*>(p))[1];
    case 4:
        byte2 = (reinterpretcast<uint8_t*>(p))[2];
        byte3 = (reinterpretcast<uint8_t*>(p))[3];
        break;
    default:
        break;
    }
#  if defined(BOOST_BIG_ENDIAN)
    switch (len)
    {
    case 1:
        break;
    case 2:
        (reinterpretcast<uint8_t*>(p))[0] = byte1;
        (reinterpretcast<uint8_t*>(p))[1] = byte0;
        break;
    case 4:
        (reinterpretcast<uint8_t*>(p))[0] = byte3;
        (reinterpretcast<uint8_t*>(p))[1] = byte2;
        (reinterpretcast<uint8_t*>(p))[2] = byte1;
        (reinterpretcast<uint8_t*>(p))[3] = byte0;
    }
#  elif defined(BOOST_PDP_ENDIAN)  // PDP_ENDIAN
    // 16-bit word are stored in little_endian
    // 32-bit word are stored in middle_endian: the most significant "half" (16-bits) followed by 
    // the less significant half (as if big-endian) but with each half stored in little-endian format
    switch (len)
    {
    case 1:
    case 2:
        break;
    case 4:
        (reinterpretcast<uint8_t*>(p))[0] = byte2;
        (reinterpretcast<uint8_t*>(p))[1] = byte3;
        (reinterpretcast<uint8_t*>(p))[2] = byte0;
        (reinterpretcast<uint8_t*>(p))[3] = byte1;
    }
#  endif
# endif
}

