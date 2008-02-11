// Powder Diffraction CIF (pdCIF)
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$


#include <map>
#include <boost/spirit/core.hpp>

#include "pdcif.h"
#include "util.h"

using namespace std;
using namespace boost::spirit;
using namespace xylib::util;

namespace xylib {

const FormatInfo PdCifDataSet::fmt_info(
    "pdcif",
    "The Crystallographic Information File for Powder Diffraction",
    vector<string>(1, "cif"),
    false,                      // not binary
    true,                       // multi-blocks
    &PdCifDataSet::ctor,
    &PdCifDataSet::check
);

bool PdCifDataSet::check(istream &f) 
{
    string line;
    // the 1st line (that is not a comment) must start with "data_"
    if (!get_valid_line(f, line, '#') || !str_startwith(line, "data_")) 
        return false;

    // in pdCIF, there must be at least a tag whose name starts with "_pd_"
    while (get_valid_line(f, line, '#')) 
        if (str_startwith(line, "_pd_")) 
            return true;

    return false;
}

//void my_assert(bool condition, const string &msg)
//{
//    if (!condition) {
//        throw XY_Error(msg);
//    }
//}


// Based on: http://www.iucr.org/iucr-top/cif/spec/version1.1/cifsyntax.html
// Appendix A: A formal grammar for CIF
struct CifGrammar : public grammar<CifGrammar>
{

    template <typename ScannerT>
    struct definition
    {
        definition(CifGrammar const& /*self*/)
        {
            // CIF 'syntactic units' that have equivalent Spirit parsers:
            //            CIF            |        Spirit
            //      ---------------------+---------------------------
            //     <Digit>               |        digit_p
            //     <UnsignedInteger>     |        uint_p
            //     <Integer>             |        int_p
            //     <Float>               |        real_p
            //     <Comments>            |        +comment_p("#")
            //     DATA_                 |        as_lower_d["data_"]
            //
            //     (LOOP_, GLOBAL_, SAVE_, STOP_ are similar to DATA_)

            CIF 
                = !Comments >> !WhiteSpace
                  >> !( DataBlock >> *(WhiteSpace >> DataBlock) >> !WhiteSpace
                      )
                ;

            DataBlock 
                = DataBlockHeading 
                  >> *( WhiteSpace >> (DataItems | SaveFrame) )
                ;

            DataBlockHeading 
                = as_lower_d["data_"] 
                  >> +NonBlankChar
                ;

            SaveFrame 
                = SaveFrameHeading 
                  >> +( WhiteSpace >> DataItems )
                  >> WhiteSpace >> as_lower_d["save_"]
                ;

            SaveFrameHeading 
                = as_lower_d["save_"] >> +NonBlankChar
                ;

            DataItems 
                = (Tag >> WhiteSpace >> Value
                  )
                | (LoopHeader >> LoopBody
                  )
                ;

            LoopHeader 
                = as_lower_d["loop_"] 
                  >> +( WhiteSpace >> Tag )
                ;

            LoopBody
                = Value >> *( WhiteSpace >> Value )
                ;

        }

        rule<ScannerT> CIF, DataBlock, DataBlockHeading, SaveFrame, 
                       SaveFrameHeading, DataItems, LoopHeader, LoopBody,
                       Tag, Value,
                       Numeric, Number, 
                       CharString, TextField,
                       WhiteSpace, TokenizedComments, Comments,
                       NonBlankChar;

        rule<ScannerT> const& start() const { return CIF; }
    };

};

void PdCifDataSet::load_data(std::istream &f) 
{
    // interesting data - data names without _pd_ prefix
    static const char* valued_keys[] = {
        // x-axis
        //_pd_meas_2theta_range_
        //_pd_proc_2theta_range_
        "pd_meas_2theta_scan",
        "pd_meas_time_of_flight",
        "pd_proc_2theta_corrected",
        "pd_proc_d_spacing",
        "pd_proc_energy_incident",
        "pd_proc_energy_detection",
        "pd_proc_recip_len_Q",
        "pd_proc_wavelength",

        // y-axis
        "pd_meas_counts_total",
        "pd_meas_counts_background",
        "pd_meas_counts_container",
        "pd_meas_intensity_total",
        "pd_meas_intensity_background",
        "pd_meas_intensity_container",
        "pd_proc_intensity_net",
        "pd_proc_intensity_total",
        "pd_proc_intensity_bkg_calc",
        "pd_proc_intensity_bkg_fix",
        "pd_calc_intensity_net",
        "pd_calc_intensity_total",

        // weight, sigma, etc.
        "pd_meas_step_count_time",
        "pd_meas_counts_monitor",
        "pd_meas_intensity_monitor",
        "pd_proc_intensity_norm",
        "pd_proc_intensity_incident",
        "pd_proc_ls_weight",
        NULL
    };

    // read file into vector<char>
    f.unsetf(ios::skipws); 
    vector<char> vec;
    std::copy(istream_iterator<char>(f), istream_iterator<char>(),
              std::back_inserter(vec));
    CifGrammar p;
    parse_info<vector<char>::const_iterator> info =
        parse(vec.begin(), vec.end(), p);
    format_assert(info.full,
                  "Parse error at character " + S(info.stop - vec.begin()));
}


} // namespace xylib

