// Implementation of class VamasDataSet for reading meta-data and xy-data from 
// Licence: GNU General Public License version 2
// $Id: VamasDataSet.h $

#include "xylib.h"
#include "common.h"

using namespace std;
using namespace boost;
using namespace xylib;
using namespace xylib::util;

namespace xylib {

bool VamasDataSet::is_filetype() const
{
    // the first line must be "VAMAS Surface ..."
    ifstream &f = *p_ifs;

    static string magic = "VAMAS Surface Chemical Analysis Standard Data Transfer Format 1988 May 4";
    string line;

    return (getline(f, line) && str_trim(line) == magic);
}


void VamasDataSet::load_data() 
{
    init();
    ifstream &f = *p_ifs;

    unsigned total_regions, blk_cnt;
    string line;
    int n, n2;  // n2 is "entries_in_inclusion_list"

    // skip "institution id" etc, 4 items
    skip_lines(f, 4);

    // comment lines, n is nr of lines
    n = read_line_int(f);
    skip_lines(f, n);
    
    // experiment mode
    getline(f, line);
    line = str_trim(line);
    int exp_mode = find_exp_mode_value(line);

    // scan_mode
    getline(f, line); 
    line = str_trim(line);
    int scan_mode = find_scan_mode_value(line);

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
    for (unsigned i = 0; i < blk_cnt; ++i) {
        FixedStepRange *p_rg = new FixedStepRange;
        vamas_read_blk(f, 
            p_rg, include, false, 
            block_future_upgrade_entries,
            exp_future_upgrade_entries,
            exp_mode, 
            scan_mode, 
            exp_variables);
        ranges.push_back(p_rg);
    }
}




// read one blk, used by load_vamas_file()
void VamasDataSet::vamas_read_blk(std::ifstream &f, 
                           FixedStepRange *p_rg,
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
                line = str_trim(line);
                tech = find_technique_value(line);
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

    p_rg->set_x_start(x_start);
    p_rg->set_x_step(x_step);
    
    fp y;
    for (int c = 0; c < pt_cnt; ++c)
    {
        y = read_line_fp(f);
        p_rg->add_y(y);
    }
}


int VamasDataSet::find_exp_mode_value(const std::string &str)
{
    const static string exp_mode[] = {
        "MAP","MAPDP","MAPSV","MAPSVDP","NORM",
        "SDP","SDPSV","SEM","NOEXP",
    };

    return get_array_idx(exp_mode, 9, str);
}


int VamasDataSet::find_technique_value(const std::string &str)
{
    const static string tech_mode[] = {
        "AES diff","AES dir","EDX","ELS","FABMS",
        "FABMS energy spec","ISS","SIMS","SIMS energy spec","SNMS",
        "SNMS energy spec","UPS","XPS","XRF"
    };

    return get_array_idx(tech_mode, 14, str);
}


int VamasDataSet::find_scan_mode_value(const std::string &str)
{
    const static string scan_mode[] = {
        "REGULAR","IRREGULAR","MAPPING",
    };

    return get_array_idx(scan_mode, 3, str);
}

} // end of namespace xylib

