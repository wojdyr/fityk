// Header of class BruckerV23RawDataSet for reading meta-data and xy-data from 
// Siemens/Bruker Diffrac-AT Raw File v2/v3 format
// Licence: GNU General Public License version 2
// $Id: __MY_FILE_ID__ $

#ifndef RIGAKU_DATASET
#define RIGAKU_DATASET

class RigakuDataSet : public DataSet
{
public:
    RigakuDataSet(const std::string &filename)
        : DataSet(filename, FT_RIGAKU) {}

    // implement the interfaces specified by DataSet
    
    
    bool is_filetype() const;
    void load_data();

protected:
    void parse_range();
}; 

#endif // #ifndef RIGAKU_DATASET

