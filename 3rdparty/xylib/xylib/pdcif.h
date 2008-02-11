// Powder Diffraction CIF (pdCIF)
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

// Powder Diffraction CIF (Crystallographic Information File)
// specification: http://www.iucr.org/iucr-top/cif/index.html
//
// This file doesn't implement full CIF grammar. The parser may fail to read 
// some valid files and read invalid ones.
//
// There may be more than one data-blocks in one block (defined by the format 
// specification), and the point counts of these data-blocks are different. 
// To handle this, xylib uses a new xylib::Block with duplicated meta-info to 
// represent them. 
//
// implementation is based on the file format specification and the tcl/tk
// program ciftools

#ifndef PDCIF_DATASET_H
#define PDCIF_DATASET_H
#include "xylib.h"

namespace xylib {

    class PdCifDataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(PdCifDataSet)
        
    protected:
            /*
        void get_all_values(const std::string &line, std::istream &f, 
                            std::vector<std::string> &values, bool in_loop);
        bool add_key_val(Block *p_blk, const std::string &key, 
                         const std::string &val);
        void add_block(Block* p_blk);

        VecColumn* get_col_ptr(Block *p_blk, const std::string name, 
            std::map<std::string, VecColumn*>& mapper);
            */
    }; 

} // namespace

#endif // #ifndef PDCIF_DATASET_H
