// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__VAR__H__
#define FITYK__VAR__H__

#include <boost/spirit/core.hpp>
#include <boost/spirit/tree/ast.hpp>
using namespace boost::spirit;

#include "common.h"
#include "calc.h"


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
    bool is_auto_delete() const { return name.size() > 0 && name[0] == '_'; }
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


/// domain of variable, used _only_ for randomization of the variable
class Domain 
{ 
    bool ok, ctr_set;
    fp ctr, sigma; 

public:
    Domain() : ok(false), ctr_set(false) {}
    bool is_set() const { return ok; }
    bool is_ctr_set() const { return ctr_set; }
    fp get_ctr() const { assert(ok && ctr_set); return ctr; }
    fp get_sigma() const { assert(ok); return sigma; }
    void set(fp c, fp s) { ok=true; ctr_set=true; ctr=c; sigma=s; }
    void set_sigma(fp s) { ok=true; sigma=s; }
    std::string str() const 
    {
        if (ok)
            return "[" + (ctr_set ? S(ctr) : S()) + " +- " + S(sigma) + "]";
        else
            return std::string();
    }
};


/// the variable is either simple-variable and nr is the index in vector
/// of parameters, or it is "compound variable" and has nr==-1.
/// third special case: nr==-2 - it is mirror-variable (such variable
///        is not recalculated but copied)
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
    Domain domain;

    struct ParMult { int p; fp mult; };
    Variable(const std::string &name_, int nr_);
    Variable(std::string const &name_, std::vector<std::string> const &vars_,
             std::vector<OpTree*> const &op_trees_);
    void recalculate(std::vector<Variable*> const &variables, 
                     std::vector<fp> const &parameters);
  
    int get_nr() const { return nr; };
    void erased_parameter(int k);
    fp get_value() const { return value; };
    std::string get_info(std::vector<fp> const &parameters, 
                         bool extended=false) const;
    std::string get_formula(std::vector<fp> const &parameters) const;
    bool is_visible() const { return true; } //for future use
    void set_var_idx(std::vector<Variable*> const& variables);
    std::vector<ParMult> const& get_recursive_derivatives() const 
                                            { return recursive_derivatives; }
    bool is_simple() const { return nr != -1; }
    bool is_constant() const { return nr == -1 && af.is_constant(); }
    std::vector<OpTree*> const& get_op_trees() const 
                                                { return af.get_op_trees(); }
    void set_original(Variable const* orig) { assert(nr==-2); original=orig; }
    Variable const* freeze_original(fp val);

private:
    int nr; /// see description of this class in .h 
    fp value; 
    std::vector<fp> derivatives; 
    std::vector<ParMult> recursive_derivatives;
    AnyFormula af; //TODO use auto_ptr<AnyFormula>
    Variable const* original;
};


/// grammar for parsing "$variable_name_here"
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

/// grammar for parsing "%function_name_here"
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

/// grammar for parsing mathematic expressions (eg. variable right hand side)
struct FuncGrammar : public grammar<FuncGrammar>
{
    static const int real_constID = 1;
    static const int variableID = 2;
    static const int exptokenID = 3;
    static const int factorID = 4;
    static const int signargID = 5;
    static const int termID = 6;
    static const int expressionID = 7;

    template <typename ScannerT>
    struct definition
    {
        definition(FuncGrammar const& /*self*/);

        rule<ScannerT, parser_context<>, parser_tag<expressionID> > expression;
        rule<ScannerT, parser_context<>, parser_tag<termID> >       term;
        rule<ScannerT, parser_context<>, parser_tag<factorID> >     factor;
        rule<ScannerT, parser_context<>, parser_tag<signargID> >    signarg;
        rule<ScannerT, parser_context<>, parser_tag<exptokenID> >   exptoken;
        rule<ScannerT, parser_context<>, parser_tag<variableID> >   variable;
        rule<ScannerT, parser_context<>, parser_tag<real_constID> > real_const;

        rule<ScannerT, parser_context<>, parser_tag<expressionID> > const&
        start() const { return expression; }
    };
};

extern FuncGrammar FuncG;

#endif 
