// Header of class BruckerV23RawDataSet for reading meta-data and xy-data from 
// Siemens/Bruker Diffrac-AT Raw File v2/v3 format
// Licence: GNU General Public License version 2
// $Id: __MY_FILE_ID__ $

#ifndef UXD_DATASET_H
#define UXD_DATASET_H

class UxdDataSet : public DataSet
{
public:
    UxdDataSet(const std::string &filename)
        : DataSet(filename, FT_UXD) {}

    // implement the interfaces specified by DataSet
    bool is_filetype() const;
    void load_data();
    
protected:
    void parse_range(std::vector<std::string> &lines, FixedStepRange *p_rg);
}; 

#endif // #ifndef UXD_DATASET_H

