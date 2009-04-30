// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

#ifndef FITYK__MGR__H__
#define FITYK__MGR__H__

#include "common.h"

class Variable;
class Function;
class Ftk;
class Model;

/// keeps all functions and variables
class VariableManager
{
public:
    bool silent;

    VariableManager(Ftk const* F_) : silent(false), F(F_), 
                        var_autoname_counter(0), func_autoname_counter(0) {}
    ~VariableManager();
    void register_model(Model *m) { models.push_back(m); }
    void unregister_model(Model const *m);

    /// if name is empty, variable name is generated automatically
    /// name of created variable is returned
    std::string assign_variable(std::string const &name,std::string const &rhs);

    void sort_variables();

    std::string assign_variable_copy(std::string const& name, 
                                     Variable const* orig, 
                                     std::map<int,std::string> const& varmap);

    void delete_variables(std::vector<std::string> const &name);

    ///returns -1 if not found or idx in variables if found
    int find_variable_nr(std::string const &name);
    Variable const* find_variable(std::string const &name);
    int find_nr_var_handling_param(int p);
    Variable const* find_variable_handling_param(int p)
                { return variables[find_nr_var_handling_param(p)]; }

    /// search for "simple" variable which handles parameter par
    /// returns -1 if not found or idx in variables if found
    int find_parameter_variable(int par);

    /// remove unreffered variables and parameters
    void remove_unreferred();
    /// remove unreffered functions
    void auto_remove_functions();
    bool is_function_referred(int n) const;

    std::string get_variable_info(std::string const &s, bool extended_print);
    std::vector<fp> const& get_parameters() const { return parameters; }
    std::vector<Variable*> const& get_variables() const { return variables; }
    Variable const* get_variable(int n) const { return variables[n]; }
    Variable* get_variable(int n) { return variables[n]; }

    std::string assign_func(std::string const &name, 
                            std::string const &function, 
                            std::vector<std::string> const &vars,
                            bool parse_vars=true);
    std::string assign_func_copy(std::string const &name, 
                                 std::string const &orig);
    void substitute_func_param(std::string const &name, 
                               std::string const &param,
                               std::string const &var);
    void delete_funcs(std::vector<std::string> const &names);
    void delete_funcs_and_vars(std::vector<std::string> const &xnames);
    ///returns -1 if not found or idx in variables if found
    int find_function_nr(std::string const &name) const;
    Function const* find_function(std::string const &name) const;
    std::vector<Function*> const& get_functions() const { return functions; }
    Function const* get_function(int n) const { return functions[n]; }

    /// calculate value and derivatives of all variables; 
    /// do precomputations for all functions
    void use_parameters(); 
    void use_external_parameters(std::vector<fp> const &ext_param);
    void put_new_parameters(std::vector<fp> const &aa);
    fp variation_of_a(int n, fp variat) const;
    std::vector<std::string> get_variable_references(std::string const &name);

protected:
    Ftk const* F;
    std::vector<Model*> models;
    std::vector<fp> parameters;
    /// sorted, a doesn't depend on b if idx(a)>idx(b)
    std::vector<Variable*> variables; 
    std::vector<Function*> functions;
    int var_autoname_counter; ///for names for "anonymous" variables
    int func_autoname_counter; ///for names for "anonymous" functions

    std::string do_assign_func(Function* func);
    std::string get_or_make_variable(std::string const& func);
    Variable *create_variable(std::string const &name, std::string const &rhs);
    std::string put_into_variables(Variable* new_var);
    bool is_variable_referred(int i, 
                              std::vector<std::string> const &ignore_vars
                                                 =std::vector<std::string>(),
                              std::string *first_referrer=0);
    std::vector<std::string> make_varnames(std::string const &function,
                                          std::vector<std::string> const &vars);
    std::vector<std::string> get_vars_from_kw(std::string const &function,
                                         std::vector<std::string> const &vars);
    std::string make_var_copy_name(Variable const* v);
    std::string next_var_name(); ///generate name for "anonymous" variable
    std::string next_func_name();///generate name for "anonymous" function
    void do_reset();
};

#endif 
