// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $
#ifndef FITYK_UDF_H_
#define FITYK_UDF_H_

#include "func.h"
#include "mgr.h"
#include "common.h"

/// Function which definition is based on other function(s)
class CompoundFunction: public Function
{
public:
    CompoundFunction(const Settings* settings,
                     const std::string &name,
                     Tplate::Ptr tp,
                     const std::vector<std::string> &vars);
    virtual void init();

    void more_precomputations();
    void calculate_value_in_range(const std::vector<fp> &xx,
                                  std::vector<fp> &yy,
                                  int first, int last) const;
    void calculate_value_deriv_in_range(const std::vector<fp> &xx,
                                   std::vector<fp> &yy, std::vector<fp> &dy_da,
                                   bool in_dx,
                                   int first, int last) const;
    std::string get_current_formula(const std::string& x = "x") const;
    bool get_center(fp* a) const;
    bool get_height(fp* a) const;
    bool get_fwhm(fp* a) const;
    bool get_area(fp* a) const;
    bool get_nonzero_range(fp level, fp& left, fp& right) const;
    void set_var_idx(const std::vector<Variable*>& variables);

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
    std::string get_bytecode() const;


private:
    mutable fp value_;
    mutable std::vector<fp> derivatives_;

    std::vector<OpTree*> op_trees_;
    VMData vm_;
    VMData substituted_vm_; // made by substituting symbols with numbers in vm_
    int value_offset_;

    DISALLOW_COPY_AND_ASSIGN(CustomFunction);
};

//////////////////////////////////////////////////////////////////////////

/// split function, defined using "x < expr ? Func(...) : FuncType(...)
class SplitFunction: public Function
{
public:
    SplitFunction(const Settings* settings,
                  const std::string &name,
                  Tplate::Ptr tp,
                  const std::vector<std::string> &vars);
    virtual void init();


    void more_precomputations();
    void calculate_value_in_range(const std::vector<fp> &xx,
                                  std::vector<fp> &yy,
                                  int first, int last) const;
    void calculate_value_deriv_in_range(std::vector<fp> const &xx,
                                        std::vector<fp> &yy,
                                        std::vector<fp> &dy_da,
                                        bool in_dx,
                                        int first, int last) const;
    std::string get_current_formula(const std::string& x = "x") const;
    virtual bool get_center(fp* a) const;
    virtual bool get_height(fp* a) const;
    virtual bool get_fwhm(fp*) const { return false; }
    virtual bool get_area(fp*) const { return false; }
    bool get_nonzero_range(fp level, fp& left, fp& right) const;
    void set_var_idx(const std::vector<Variable*>& variables);

private:
    std::vector<Variable*> intern_variables_;
    Function *left_, *right_;

    DISALLOW_COPY_AND_ASSIGN(SplitFunction);
};

#endif

