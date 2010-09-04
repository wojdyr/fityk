// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $
#ifndef FITYK__UDF__H__
#define FITYK__UDF__H__

#include "func.h"

namespace UdfContainer
{
    enum UdfType { kCompound, kSplit, kCustom };

    struct UDF
    {
        std::string name;
        std::string formula; //full definition
        UdfType type;
        bool builtin;
        std::vector<OpTree*> op_trees;

        UDF(std::string const& formula_, bool is_builtin_=false);
    };

    extern std::vector<UDF> udfs;

    void initialize_udfs();
    /// checks partially the definition and puts formula into udfs
    void define(std::string const &formula);
    /// removes the definition from udfs
    void undefine(std::string const &type);
    inline UDF const* get_udf(size_t n) {return n < udfs.size() ? &udfs[n] : 0;}
    UDF const* get_udf(std::string const &type);
    inline bool is_defined(std::string const &type) { return get_udf(type); }
    inline std::vector<UDF> const& get_udfs() { return udfs; }

    void check_cpd_rhs_function(std::string const &fun,
                                   std::vector<std::string> const& lhs_vars);
    void check_fudf_rhs(std::string const& rhs,
                        std::vector<std::string> const& lhs_vars);
    std::vector<std::string> get_cpd_rhs_components(std::string const &formula,
                                                    bool full);
    void check_rhs(std::string const& rhs,
                   std::vector<std::string> const& lhs_vars);
}


/// Function which definition is based on other function(s)
class CompoundFunction: public Function
{
    friend class Function;
public:

    void more_precomputations();
    void calculate_value_in_range(std::vector<fp> const &xx,
                                  std::vector<fp> &yy,
                                  int first, int last) const;
    void calculate_value_deriv_in_range(std::vector<fp> const &xx,
                                   std::vector<fp> &yy, std::vector<fp> &dy_da,
                                   bool in_dx,
                                   int first, int last) const;
    std::string get_current_formula(std::string const& x = "x") const;
    bool has_center() const;
    fp center() const { return vmgr_.get_function(0)->center(); }
    bool has_height() const;
    fp height() const;
    bool has_fwhm() const;
    fp fwhm() const;
    bool has_area() const;
    fp area() const;
    bool get_nonzero_range(fp level, fp& left, fp& right) const;
    void precomputations_for_alternative_vv();
    void set_var_idx(std::vector<Variable*> const& variables);

protected:
    VariableManager vmgr_;

    CompoundFunction(Ftk const* F,
                     std::string const &name,
                     std::string const &type,
                     std::vector<std::string> const &vars);

    void init_components(std::vector<std::string>& rf);

private:
    virtual void init();
    CompoundFunction (const CompoundFunction&); //disable
};

//////////////////////////////////////////////////////////////////////////

/// User Defined Function, formula taken from user input
class CustomFunction: public Function
{
    friend class Function;
public:
    void more_precomputations();
    void calculate_value_in_range(std::vector<fp> const &xx,
                                  std::vector<fp> &yy,
                                  int first, int last) const;
    void calculate_value_deriv_in_range(std::vector<fp> const &xx,
                                        std::vector<fp> &yy,
                                        std::vector<fp> &dy_da,
                                        bool in_dx,
                                        int first, int last) const;
    void set_var_idx(std::vector<Variable*> const& variables);
    std::string get_bytecode() const { return afo_.get_vmcode_info(); }
private:
    CustomFunction(Ftk const* F,
                   std::string const &name,
                   std::string const &type,
                   std::vector<std::string> const &vars,
                   std::vector<OpTree*> const& op_trees);
    CustomFunction(const CustomFunction&); //disable

    fp value_;
    std::vector<fp> derivatives_;
    AnyFormulaO afo_;
};

//////////////////////////////////////////////////////////////////////////

/// split function, defined using "if x < ... then ... else ..."
class SplitFunction: public CompoundFunction
{
    friend class Function;
public:
    void calculate_value_in_range(std::vector<fp> const &xx,
                                  std::vector<fp> &yy,
                                  int first, int last) const;
    void calculate_value_deriv_in_range(std::vector<fp> const &xx,
                                        std::vector<fp> &yy,
                                        std::vector<fp> &dy_da,
                                        bool in_dx,
                                        int first, int last) const;
    std::string get_current_formula(std::string const& x = "x") const;
    virtual bool has_height() const;
    virtual fp height() const { return vmgr_.get_function(0)->height(); }
    virtual bool has_fwhm() const { return false; }
    virtual fp fwhm() const { return 0; }
    virtual bool has_area() const { return false; }
    virtual fp area() const { return 0; }
    virtual bool has_center() const;
    virtual fp center() const { return vmgr_.get_function(0)->center(); }
    bool get_nonzero_range(fp level, fp& left, fp& right) const;

private:
    SplitFunction(Ftk const* F,
                  std::string const &name,
                  std::string const &type,
                  std::vector<std::string> const &vars);
    virtual void init();

    SplitFunction(const SplitFunction&); //disable
};

#endif

