// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_UDF_H_
#define FITYK_UDF_H_

#include "func.h"
#include "mgr.h"
#include "common.h"

namespace fityk {

/// Function which definition is based on other function(s)
class CompoundFunction: public Function
{
public:
    CompoundFunction(const Settings* settings,
                     const std::string &name,
                     Tplate::Ptr tp,
                     const std::vector<std::string> &vars);
    ~CompoundFunction();
    virtual void init();

    void more_precomputations();
    void calculate_value_in_range(const std::vector<realt> &xx,
                                  std::vector<realt> &yy,
                                  int first, int last) const;
    void calculate_value_deriv_in_range(const std::vector<realt> &xx,
                                        std::vector<realt> &yy,
                                        std::vector<realt> &dy_da,
                                        bool in_dx,
                                        int first, int last) const;
    std::string get_current_formula(const std::string& x,
                                    const char *num_fmt) const;
    bool is_symmetric() const;
    bool get_center(realt* a) const;
    bool get_height(realt* a) const;
    bool get_fwhm(realt* a) const;
    bool get_area(realt* a) const;
    bool get_nonzero_range(double level, realt& left, realt& right) const;
    void update_var_indices(const std::vector<Variable*>& variables);

protected:
    std::vector<Variable*> intern_variables_;
    std::vector<Function*> intern_functions_;

private:
    DISALLOW_COPY_AND_ASSIGN(CompoundFunction);
};

//////////////////////////////////////////////////////////////////////////

/// User Defined Function, formula taken from user input
class CustomFunction: public Function
{
public:
    CustomFunction(const Settings* settings,
                   const std::string &name,
                   const Tplate::Ptr tp,
                   const std::vector<std::string> &vars);
    ~CustomFunction();

    void more_precomputations();
    void calculate_value_in_range(std::vector<realt> const &xx,
                                  std::vector<realt> &yy,
                                  int first, int last) const;
    void calculate_value_deriv_in_range(std::vector<realt> const &xx,
                                        std::vector<realt> &yy,
                                        std::vector<realt> &dy_da,
                                        bool in_dx,
                                        int first, int last) const;
    std::string get_current_formula(const std::string& x,
                                    const char *num_fmt) const;
    void update_var_indices(std::vector<Variable*> const& variables);
    std::string get_bytecode() const;


private:
    // used locally in calculate_value_deriv_in_range(),
    // declared as a member only as optimization, to avoid allocations
    mutable std::vector<realt> derivatives_;

    VMData vm_;
    VMData substituted_vm_; // made by substituting symbols with numbers in vm_
    int value_offset_;

    DISALLOW_COPY_AND_ASSIGN(CustomFunction);
};

//////////////////////////////////////////////////////////////////////////

/// split function, defined using "x < expr ? Func1(...) : Func2(...)
class SplitFunction: public Function
{
public:
    SplitFunction(const Settings* settings,
                  const std::string &name,
                  Tplate::Ptr tp,
                  const std::vector<std::string> &vars);
    ~SplitFunction();
    void init();


    void more_precomputations();
    void calculate_value_in_range(const std::vector<realt> &xx,
                                  std::vector<realt> &yy,
                                  int first, int last) const;
    void calculate_value_deriv_in_range(std::vector<realt> const &xx,
                                        std::vector<realt> &yy,
                                        std::vector<realt> &dy_da,
                                        bool in_dx,
                                        int first, int last) const;
    std::string get_current_formula(const std::string& x,
                                    const char *num_fmt) const;
    bool get_center(realt* a) const;
    bool get_height(realt* a) const;
    bool get_fwhm(realt* a) const;
    bool get_area(realt* a) const;
    bool get_nonzero_range(double level, realt& left, realt& right) const;
    void update_var_indices(const std::vector<Variable*>& variables);

private:
    std::vector<Variable*> intern_variables_;
    Function *left_, *right_;

    DISALLOW_COPY_AND_ASSIGN(SplitFunction);
};

} // namespace fityk
#endif

