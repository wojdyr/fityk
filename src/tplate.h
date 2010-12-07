// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $
#ifndef FITYK_TPLATE_H_
#define FITYK_TPLATE_H_

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

#include "common.h" // DISALLOW_COPY_AND_ASSIGN

class Ftk;
class Function;
class Parser;
struct OpTree;

/// template -- function type, like Gaussian(height, center, hwhm) = ...,
/// which can be used to create %functions by binding $variables to template's
/// parameters.
struct Tplate
{
    typedef boost::shared_ptr<const Tplate> Ptr;
    typedef Function* (*create_type)(const Ftk*, const std::string&,
                                     Ptr, const std::vector<std::string>&);

    //enum HowDefined { kCoded, kSum, kSplit, kCustom };

    struct Component
    {
        Ptr p;
        std::vector<std::string> values;
    };

    std::string name;
    std::vector<std::string> fargs;
    std::vector<std::string> defvals;
    std::string rhs; // used in info only, not in calculations
    bool linear_d; // uses Guess::linear_traits
    bool peak_d;   // uses Guess::peak_traits
    create_type create;
    std::vector<Component> components; // kSum, kSplit
    std::vector<OpTree*> op_trees;     // kCustom

    Tplate() {}
    std::string as_formula() const;


private:
    DISALLOW_COPY_AND_ASSIGN(Tplate);
};

// takes keyword args and returns positional args for given function.
// Used when we get Gaussian(center=1,whatever=2,height=3,hwhm=4)
// instead of Gaussian(3,1,4)
std::vector<std::string> reorder_args(Tplate::Ptr,
                                      const std::vector<std::string> &keys,
                                      const std::vector<std::string> &values);


/// template manager
class TplateMgr
{
public:
    TplateMgr() {}

    // initialization
    void add_builtin_types(Parser* p);

    /// stores the formula
    void define(Tplate::Ptr tp);

    /// removes the definition
    void undefine(const std::string& name);

    const Tplate* get_tp(const std::string& name) const;
    Tplate::Ptr get_shared_tp(const std::string& name) const;

    const std::vector<Tplate::Ptr>& tpvec() const { return tpvec_; }

private:
    std::vector<Tplate::Ptr> tpvec_;

    void add(const char* name, const char* cs_fargs, const char* cs_dv,
             const char* rhs, bool linear_d, bool peak_d,
             Tplate::create_type create, Parser* parser=NULL);

    DISALLOW_COPY_AND_ASSIGN(TplateMgr);
};

#endif // FITYK_TPLATE_H_
