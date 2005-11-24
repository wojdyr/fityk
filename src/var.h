// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef VAR__H__
#define VAR__H__

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
    bool is_dependent_on(int idx, std::vector<Variable*> const &variables);
    bool is_directly_dependent_on(int idx);
    void set_var_idx(std::vector<Variable*> const &variables);
    int get_max_var_idx();
    void substitute_param(int n, std::string const &new_p)
             { assert(n >= 0 && n < size(varnames)); varnames[n] = new_p; }
protected:
    std::vector<std::string> varnames; // variable names 
    /// var_idx is set after initialization (in derived class)
    /// and modified after variable removal or change
    std::vector<int> var_idx;
};

class Variable : public VariableUser
{
    static int unnamed_counter;
public:
    struct ParMult { int p; fp mult; };
    Variable(const std::string &name_, int nr_, 
             bool auto_delete_=false, bool hidden_=false);
    Variable(std::string const &name_, std::vector<std::string> const &vars_,
             std::vector<OpTree*> const &op_trees_, 
             bool auto_delete_=false, bool hidden_=false);
    static std::string next_auto_name() { return "var" + S(++unnamed_counter); }
    void recalculate(std::vector<Variable*> const &variables, 
                     std::vector<fp> const &parameters);
  
    int get_nr() const { return nr; };
    void decrease_nr() { assert(nr != -1); --nr; };
    fp get_value() const { return value; };
    std::string get_info(std::vector<fp> const &parameters, 
                         bool extended=false) const;
    std::string get_formula(std::vector<fp> const &parameters) const;
    bool is_visible() const { return !hidden; }
    bool is_auto_delete() const { return auto_delete; }
    /// (re-)create bytecode, required after set_var_idx()
    void tree_to_bytecode(); 
    std::vector<ParMult> const& get_recursive_derivatives() const 
                                            { return recursive_derivatives; }
    bool is_simple() const { return nr != -1; }
private:
    int nr; /// -1 unless it's simple "variable"
    bool auto_delete;
    bool hidden;
  
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


class VariableManager
{
public:
    void register_sum(Sum *s) { sums.push_back(s); }
    void unregister_sum(Sum const *s);

    /// if name is empty, variable name is generated automatically
    /// name of created variable is returned
    std::string assign_variable(std::string const &name,std::string const &rhs);

    void sort_variables();

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

    std::string assign_func(std::string const &name, 
                            std::string const &function, 
                            std::vector<std::string> const &vars);
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

    Variable *create_simple_variable(std::string const &name, 
                                     std::string const &rhs);
    Variable *create_variable(std::string const &name, std::string const &rhs);
    int get_variable_value(std::string const &name);
    bool is_variable_referred(int i, 
                              std::vector<std::string> const &ignore_vars
                                                 =std::vector<std::string>(),
                              std::string *first_referrer=0);
    std::vector<std::string> make_varnames(std::string const &function,
                                          std::vector<std::string> const &vars);
    std::vector<std::string> get_vars_from_kw(std::string const &function,
                                         std::vector<std::string> const &vars);
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
