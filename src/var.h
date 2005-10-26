// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef VAR__H__
#define VAR__H__

#include <boost/spirit/core.hpp>
#include <boost/spirit/tree/ast.hpp>
using namespace boost::spirit;

#include "common.h"


struct OpTree;

//struct Parameter
//{
//    Parameter(fp val_) : val(val_) {}
//    fp val;
//};
typedef fp Parameter;

class Variable
{
    static int unnamed_counter;
public:
    Variable(const std::string &name_, int nr_, 
             bool auto_delete_=false, bool hidden_=false);
    Variable(const std::string &name_, const std::vector<std::string> &vmvar_,
             const std::vector<OpTree*> &op_trees_, 
             bool auto_delete_=false, bool hidden_=false);
    void set_vmvar_idx(const std::vector<Variable*> &variables);
    int get_max_vmvar_idx();
    void recalculate(const std::vector<Variable*> &variables, 
                     const std::vector<fp>& parameters);
    bool is_dependent_on(int idx, const std::vector<Variable*> &variables);
    bool is_directly_dependent_on(int idx);
  
    std::string get_name() const { return name; };
    int get_nr() const { return nr; };
    fp get_value() const { return value; };
    std::string get_info(bool extended=false) const;
    std::string get_formula() const;
    /// (re-)create bytecode, required after set_vmvar_idx()
    void tree_to_bytecode(); 
private:
    std::string name;
    int nr; /// -1 unless it's simple "variable"
    bool auto_delete;
    bool hidden;
  
    // these are recalculated every time parameters or variables are changed
    fp value;
    std::vector<fp> derivatives;
  
    /// vmvar_idx is set after initialization and on variable removal or change
    std::vector<int> vmvar_idx;
  
    // these are set on initialization and never changed
    std::vector<OpTree*> op_trees; 
    std::vector<int> vmcode; //OP_PUT_DERIV, OP_PUT_VAL, OP_NUMBER, OP_VAR
    std::vector<fp> vmdata;
    std::vector<std::string> vmvar;

    void run_vm(const std::vector<Variable*> &variables);
};

/// sorted, a doesn't depend on b if idx(a)>idx(b)
extern std::vector<Variable*> variables; 

/// if name is empty, variable name is generated automatically
/// name of created variable is returned
std::string assign_variable(const std::string &name, const std::string &rhs);

void sort_variables();

bool del_variable(const std::string &name);

//returns -1 if not found or idx in variables if found
int find_variable_nr(const std::string &name);
const Variable* find_variable(const std::string &name);

// search for "simple" variable which handles parameter par
//returns -1 if not found or idx in variables if found
int find_parameter_variable(int par);

// calculate value and derivatives of all variables
void recalculate_variables();


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

struct VariableRhsGrammar : public grammar<VariableRhsGrammar>
{
    static const int real_constID = 1;
    static const int variableID = 2;
    static const int exptokenID = 3;
    static const int factorID = 4;
    static const int termID = 5;
    static const int expressionID = 6;
    static const int datatrans_constID = 7;

    template <typename ScannerT>
    struct definition
    {
        definition(VariableRhsGrammar const& /*self*/);

        rule<ScannerT, parser_context<>, parser_tag<expressionID> > expression;
        rule<ScannerT, parser_context<>, parser_tag<termID> >       term;
        rule<ScannerT, parser_context<>, parser_tag<factorID> >     factor;
        rule<ScannerT, parser_context<>, parser_tag<exptokenID> >   exptoken;
        rule<ScannerT, parser_context<>, parser_tag<variableID> >   variable;
        rule<ScannerT, parser_context<>, parser_tag<real_constID> > real_const;
        rule<ScannerT, parser_context<>, parser_tag<datatrans_constID> > 
                                                               datatrans_const;

        rule<ScannerT, parser_context<>, parser_tag<expressionID> > const&
        start() const { return expression; }
    };
};

extern std::vector<Parameter> parameters;
extern VariableRhsGrammar VariableRhsG;

#endif 
