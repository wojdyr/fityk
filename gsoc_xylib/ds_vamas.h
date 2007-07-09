// Header of class BruckerV23RawDataSet for reading meta-data and xy-data from 
// Siemens/Bruker Diffrac-AT Raw File v2/v3 format
// Licence: GNU General Public License version 2
// $Id: __MY_FILE_ID__ $

#ifndef VAMAS_DATASET
#define VAMAS_DATASET

class VamasDataSet : public DataSet
{
public:
    VamasDataSet(const std::string &filename)
        : DataSet(filename, FT_VAMAS) {}

    // implement the interfaces specified by DataSet
    bool is_filetype() const;
    void load_data();

protected:
    void vamas_read_blk(std::ifstream &f, 
                           FixedStepRange *p_rg,
                           const std::vector<bool> &include,
                           bool skip_flg = false, 
                           int block_future_upgrade_entries = 0,
                           int exp_future_upgrade_entries = 0,
                           int exp_mode = 0, 
                           int scan_mode = 0, 
                           int exp_variables = 0);

    int find_exp_mode_value(const std::string &str);
    int find_technique_value(const std::string &str);
    int find_scan_mode_value(const std::string &str);
    
}; 

#endif // #ifndef BRUCKER_RAW_V23_H

