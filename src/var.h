// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__VAR__H__
#define FITYK__VAR__H__

#include <boost/spirit/core.hpp>
#include <boost/spirit/tree/ast.hpp>
using namespace boost::spirit;

#include "common.h"


struct OpTree;

class Variable;
class Function;
class Sum;

std::string simplify_formula(std::string const &formula);

class VariableUser
{
public:
    const std::string name;
    const std::string prefix;
    const std::string xname;

    VariableUser(std::string const &name_, std::string const &prefix_, 
              std::vector<std::string> const &vars = std::vector<std::string>())
        : name(name_), prefix(prefix_), xname(prefix_+name), varnames(vars) {}
    virtual ~VariableUser() {}
    bool is_dependent_on(int idx, std::vector<Variable*> const &variables)const;
    bool is_directly_dependent_on(int idx);
    virtual void set_var_idx(std::vector<Variable*> const& variables);
    int get_var_idx(int n) const 
             { assert(n >= 0 && n < size(var_idx)); return var_idx[n]; }
    int get_max_var_idx();
    int get_vars_count() const { return varnames.size(); }
    std::string get_var_name(int n) const
             { assert(n >= 0 && n < size(varnames)); return varnames[n]; }
    void substitute_param(int n, std::string const &new_p)
             { assert(n >= 0 && n < size(varnames)); varnames[n] = new_p; }
    std::string get_debug_idx_info() const { return xname + ": " 
                   + join_vector(concat_pairs(varnames, var_idx, "/"), " "); }
protected:
    std::vector<std::string> varnames; // variable names 
    /// var_idx is set after initialization (in derived class)
    /// and modified after variable removal or change
    std::vector<int> var_idx;
};

/// the variable is either simple-variable and nr is the index in vector
/// of parameters, or it is not simple, and has nr==-1.
/// third special case: nr==-2 - it is kind of mirror variable (such variable
///        is not recalculated but copied set with copy_recalculated())
/// In second case, the value and derivatives are calculated 
/// in following steps:
///  0. string is parsed by Spirit parser to Spirit AST representation,
///     and then expression is simplified and derivates are
///     calculated using calculate_deriv() function.
///     It results in struct-OpTree-based trees (for value and all derivatives)
///     That's before creating the Variable
///  1  set_var_idx() finds indices of variables in variables vector
///      (references to variables are kept using names of the variables
///       is has to be called when indices of referred variables change
///  1a. tree_to_bytecode() called from Variable::set_var_idx 
///       takes trees and var_idx and results in bytecode  
///  3. recalculate() calculates (using run_vm()) value and derivatives
///     for current parameter value
class Variable : public VariableUser
{
public:
    bool const auto_delete;
    bool const hidden;

    struct ParMult { int p; fp mult; };
    Variable(const std::string &name_, int nr_, 
             bool auto_delete_=false, bool hidden_=false);
    Variable(std::string const &name_, std::vector<std::string> const &vars_,
             std::vector<OpTree*> const &op_trees_, 
             bool auto_delete_=false, bool hidden_=false);
    void recalculate(std::vector<Variable*> const &variables, 
                     std::vector<fp> const &parameters);
  
    int get_nr() const { return nr; };
    void erased_parameter(int k);
    fp get_value() const { return value; };
    std::string get_info(std::vector<fp> const &parameters, 
                         bool extended=false) const;
    std::string get_formula(std::vector<fp> const &parameters) const;
    bool is_visible() const { return !hidden; }
    /// (re-)create bytecode, required after ::set_var_idx()
    void tree_to_bytecode(); 
    void set_var_idx(std::vector<Variable*> const& variables)
                 { VariableUser::set_var_idx(variables); tree_to_bytecode(); }
    std::vector<ParMult> const& get_recursive_derivatives() const 
                                            { return recursive_derivatives; }
    bool is_simple() const { return nr != -1; }
    std::vector<OpTree*> const& get_op_trees() const { return op_trees; }
    void copy_recalculated(Variable const& v) 
       {nr=-2; value=v.value; recursive_derivatives=v.recursive_derivatives;}

private:
    int nr; /// -1 unless it's simple "variable"
    // these are recalculated every time parameters or variables are changed
    fp value;
    std::vector<fp> derivatives;
    std::vector<ParMult> recursive_derivatives;
  
    // these are set on initialization and never changed
    std::vector<OpTree*> op_trees; 
    std::vector<int> vmcode; //OP_PUT_DERIV, OP_PUT_VAL, OP_NUMBER, OP_VAR
    std::vector<fp> vmdata;

    void run_vm(std::vector<Variable*> const &variables);
};


/// keeps all functions and variables
class VariableManager
{
public:
    bool silent;

    VariableManager() : silent(false), 
                        var_autoname_counter(0), func_autoname_counter(0) {}
    ~VariableManager();
    void register_sum(Sum *s) { sums.push_back(s); }
    void unregister_sum(Sum const *s);

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
    Variable const* find_variable_handling_param(int p);

    /// search for "simple" variable which handles parameter par
    /// returns -1 if not found or idx in variables if found
    int find_parameter_variable(int par);

    /// remove unreffered variables and parameters
    void remove_unreferred();

    std::string get_variable_info(std::string const &s, bool extended_print) {
        Variable const* v = find_variable(s);
        return v ? v->get_info(parameters, extended_print) 
                 : "Undefined variable: " + s;
    }
    std::vector<fp> const& get_parameters() const { return parameters; }
    std::vector<Variable*> const& get_variables() const { return variables; }
    Variable const* get_variable(int n) const { return variables[n]; }
    //hack used eg. in CompoundFunction, no checks
    void set_recalculated_variable(int n, Variable const& v) 
                 { variables[n]->copy_recalculated(v); }

    std::string assign_func(std::string const &name, 
                            std::string const &function, 
                            std::vector<std::string> const &vars,
                            bool parse_vars=true);
    std::string get_func_param(std::string const&name, std::string const&param);
    std::string assign_func_copy(std::string const &name, 
                                 std::string const &orig);
    void substitute_func_param(std::string const &name, 
                               std::string const &param,
                               std::string const &var);
    void delete_funcs(std::vector<std::string> const &names);
    void delete_funcs_and_vars(std::vector<std::string> const &xnames);
    ///returns -1 if not found or idx in variables if found
    int find_function_nr(std::string const &name);
    Function const* find_function(std::string const &name);
    std::vector<Function*> const& get_functions() const { return functions; }
    Function const* get_function(int n) const { return functions[n]; }

    /// calculate value and derivatives of all variables; 
    /// do precomputations for all functions
    void use_parameters(); 
    void use_external_parameters(std::vector<fp> const &ext_param);
    void put_new_parameters(std::vector<fp> const &aa, std::string const&method,
                            bool change=true);
    fp variation_of_a(int n, fp variat) const;
    std::vector<std::string> get_variable_references(std::string const &name);

protected:
    std::vector<Sum*> sums;
    std::vector<fp> parameters;
    /// sorted, a doesn't depend on b if idx(a)>idx(b)
    std::vector<Variable*> variables; 
    std::vector<Function*> functions;
    int var_autoname_counter; ///for names for "anonymous" variables
    int func_autoname_counter; ///for names for "anonymous" functions

    std::string do_assign_func(Function* func);
    std::string get_or_make_variable(std::string const& func);
    Variable *create_variable(std::string const &name, std::string const &rhs);
    std::string do_assign_variable(Variable* new_var);
    bool is_variable_referred(int i, 
                              std::vector<std::string> const &ignore_vars
                                                 =std::vector<std::string>(),
                              std::string *first_referrer=0);
    std::vector<std::string> make_varnames(std::string const &function,
                                          std::vector<std::string> const &vars);
    std::vector<std::string> get_vars_from_kw(std::string const &function,
                                         std::vector<std::string> const &vars);
    std::string get_variable_from_kw(std::string const& function,
                                     std::string const& tname, 
                                     std::string const& tvalue, 
                                     std::vector<std::string> const& vars);
    std::string get_var_from_expression(std::string const& expr,
                                        std::vector<std::string> const& vars);
    std::string make_var_copy_name(Variable const* v);
    std::string next_var_name() { return "_" + S(++var_autoname_counter); }
    std::string next_func_name() { return "_" + S(++func_autoname_counter); }
};


struct VariableLhsGrammar : public grammar<VariableLhsGrammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(VariableLhsGrammar const& /*self*/)
    {
        t = lexeme_d["$" >> +(alnum_p | '_')];
    }
    rule<ScannerT> t;
    rule<ScannerT> const& start() const { return t; }
  };
};

extern VariableLhsGrammar  VariableLhsG;

struct FunctionLhsGrammar : public grammar<FunctionLhsGrammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(FunctionLhsGrammar const& /*self*/)
    {
        t = lexeme_d["%" >> +(alnum_p | '_')];
    }
    rule<ScannerT> t;
    rule<ScannerT> const& start() const { return t; }
  };
};

extern FunctionLhsGrammar  FunctionLhsG;

struct FuncGrammar : public grammar<FuncGrammar>
{
    static const int real_constID = 1;
    static const int variableID = 2;
    static const int exptokenID = 3;
    static const int factorID = 4;
    static const int termID = 5;
    static const int expressionID = 6;

    template <typename ScannerT>
    struct definition
    {
        definition(FuncGrammar const& /*self*/);

        rule<ScannerT, parser_context<>, parser_tag<expressionID> > expression;
        rule<ScannerT, parser_context<>, parser_tag<termID> >       term;
        rule<ScannerT, parser_context<>, parser_tag<factorID> >     factor;
        rule<ScannerT, parser_context<>, parser_tag<exptokenID> >   exptoken;
        rule<ScannerT, parser_context<>, parser_tag<variableID> >   variable;
        rule<ScannerT, parser_context<>, parser_tag<real_constID> > real_const;

        rule<ScannerT, parser_context<>, parser_tag<expressionID> > const&
        start() const { return expression; }
    };
};

extern FuncGrammar FuncG;

#endif 
