// one of Canberra MCA output formats
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

// based on a chapter from an unknown instruction, pages B1-B5: 
// APPENDIX B: FILE STRUCTURES
// "Spectral data acquired on the MCA system are directly 
// accumulated in memory. These data are transferred to a disk
// file via the \Move command in the MCA program."
//
// This format is (was?) used in one of synchrotron stations (in Hamburg?).
// Data is produced by Canberra multi-channel analyser (MCA).
// .mca is not a canonical extension.


#ifndef CANBERRA_MCA_DATASET_H_
#define CANBERRA_MCA_DATASET_H_
#include "xylib.h"

namespace xylib {

    class CanberraMcaDataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(CanberraMcaDataSet)
    public:
        static double pdp11_f (char* p);
    };

}
#endif // CANBERRA_MCA_DATASET_H_

