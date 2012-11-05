// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__MGR__H__
#define FITYK__MGR__H__

#include <map>
#include "common.h"
#include "tplate.h" // Tplate::Ptr

namespace fityk {

class Variable;
class Function;
class Ftk;
class Model;
struct FunctionSum;

/// keeps all functions and variables
class FITYK_API VariableManager
{
public:
    static bool is_auto(const std::string& name)
        { return !name.empty() && name[0] == '_'; }

    VariableManager(const Ftk* F);
    ~VariableManager();
    void register_model(Model *m) { models_.push_back(m); }
    void unregister_model(const Model *m);

    //int assign_variable(const std::string &name, const std::string &rhs);
    int make_variable(const std::string &name, VMData* vd);
    int add_variable(Variable* new_var);

    void sort_variables();

    std::string assign_variable_copy(const Variable* orig,
                                     const std::map<int,std::string>& varmap);

    void delete_variables(const std::vector<std::string> &name);

    ///returns -1 if not found or idx in variables if found
    int find_variable_nr(const std::string &name) const;
    const Variable* find_variable(const std::string &name) const;
    int find_nr_var_handling_param(int p) const;
    const Variable* find_variable_handling_param(int p) const
                { return variables_[find_nr_var_handling_param(p)]; }

    /// search for "simple" variable which handles parameter par
    /// returns -1 if not found or idx in variables if found
    //int find_parameter_variable(int par) const;

    /// remove unreffered variables and parameters
    void remove_unreferred();
    /// remove unreffered functions
    void auto_remove_functions();
    bool is_function_referred(int n) const;

    const std::vector<realt>& parameters() const { return parameters_; }
    const std::vector<Variable*>& variables() const { return variables_; }
    const Variable* get_variable(int n) const { return variables_[n]; }
    //Variable* get_variable(int n) { return variables_[n]; }
    void set_domain(int n, const RealRange& domain);

    /// returns index of the new function in functions_
    int assign_func(const std::string &name, Tplate::Ptr tp,
                    std::vector<VMData*> &args);
    /// returns index of the new function in functions_
    int assign_func_copy(const std::string &name, const std::string &orig);
    void substitute_func_param(const std::string &name,
                               const std::string &param,
                               VMData* vd);
    void delete_funcs(const std::vector<std::string> &names);
    ///returns -1 if not found or idx in functions_ if found
    int find_function_nr(const std::string &name) const;
    const Function* find_function(const std::string &name) const;
    const std::vector<Function*>& functions() const { return functions_; }
    const Function* get_function(int n) const { return functions_[n]; }

    /// calculate value and derivatives of all variables;
    /// do precomputations for all functions
    void use_parameters();
    void use_external_parameters(const std::vector<realt> &ext_param);
    void put_new_parameters(const std::vector<realt> &aa);
    realt variation_of_a(int n, realt variat) const;
    std::vector<std::string>
        get_variable_references(const std::string &name) const;
    void update_indices_in_models();
    void do_reset();

    std::string next_var_name(); ///generate name for "anonymous" variable
    std::string next_func_name(); ///generate name for "anonymous" function


private:
    const Ftk* F_;
    std::vector<Model*> models_;
    std::vector<realt> parameters_;
    /// sorted, a doesn't depend on b if idx(a)>idx(b)
    std::vector<Variable*> variables_;
    std::vector<Function*> functions_;
    int var_autoname_counter_; ///for names for "anonymous" variables
    int func_autoname_counter_; ///for names for "anonymous" functions

    int add_func(Function* func);
    //Variable *create_variable(const std::string &name, const std::string &rhs);
    //std::string get_or_make_variable(const std::string& func);
    bool is_variable_referred(int i, std::string *first_referrer = NULL);
    void reindex_all();
    std::string name_var_copy(const Variable* v);
    void update_indices(FunctionSum& sum);

};

// used in mgr.cpp and udf.cpp
Variable* make_compound_variable(const std::string &name, VMData* vd,
                                 const std::vector<Variable*>& all_variables);

} // namespace fityk
#endif
