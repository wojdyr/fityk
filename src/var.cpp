// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#include "var.h"
#include "common.h"
#include "calc.h"
#include "ast.h"

#include <stdlib.h>
#include <boost/spirit/version.hpp>
#include <algorithm>
#include <memory>

using namespace std;
using namespace boost::spirit::classic;

/// checks if *this depends (directly or indirectly) on variable with index idx
bool VariableUser::is_dependent_on(int idx,
                                   vector<Variable*> const &variables) const
{
    v_foreach (int, i, var_idx)
        if (*i == idx || variables[*i]->is_dependent_on(idx, variables))
            return true;
    return false;
}

void VariableUser::set_var_idx(vector<Variable*> const &variables)
{
    const int n = varnames.size();
    var_idx.resize(n);
    for (int v = 0; v < n; ++v) {
        bool found = false;
        for (int i = 0; i < size(variables); ++i) {
            if (varnames[v] == variables[i]->name) {
                var_idx[v] = i;
                found = true;
                break;
            }
        }
        if (!found)
            throw ExecuteError("Undefined variable: $" + varnames[v]);
    }
}

int VariableUser::get_max_var_idx()
{
    if (var_idx.empty())
        return -1;
    else
       return *max_element(var_idx.begin(), var_idx.end());
}

std::string VariableUser::get_debug_idx_info() const
{
    string r = xname + ": ";
    assert(varnames.size() == var_idx.size());
    for (size_t i = 0; i != varnames.size(); ++i)
        r += varnames[i] + "/" + S(var_idx[i]) + " ";
    return r;
}

////////////////////////////////////////////////////////////////////////////


// ctor for simple variables and mirror variables
Variable::Variable(string const &name, int nr)
    : VariableUser(name, "$"),
      nr_(nr), af_(value_, derivatives_), original_(NULL)
{
    assert(!name.empty());
    if (nr_ != -2) {
        ParMult pm;
        pm.p = nr_;
        pm.mult = 1;
        recursive_derivatives_.push_back(pm);
    }
}

// ctor for compound variables
Variable::Variable(string const &name_, vector<string> const &vars_,
                   vector<OpTree*> const &op_trees_)
    : VariableUser(name_, "$", vars_),
      nr_(-1), derivatives_(vars_.size()),
      af_(op_trees_, value_, derivatives_), original_(NULL)
{
    assert(!name_.empty());
}

void Variable::set_var_idx(vector<Variable*> const& variables)
{
    VariableUser::set_var_idx(variables);
    if (nr_ == -1)
        af_.tree_to_bytecode(var_idx);
}

string Variable::get_formula(vector<fp> const &parameters) const
{
    assert(nr_ >= -1);
    vector<string> vn;
    v_foreach (string, i, varnames)
        vn.push_back("$" + *i);
    return nr_ == -1 ? get_op_trees().back()->str(&vn)
                    : "~" + eS(parameters[nr_]);
}

void Variable::recalculate(vector<Variable*> const &variables,
                           vector<fp> const &parameters)
{
    if (nr_ >= 0) {
      value_ = parameters[nr_];
      assert(derivatives_.empty());
    }
    else if (nr_ == -1) {
        af_.run_vm(variables);
        recursive_derivatives_.clear();
        for (int i = 0; i < size(derivatives_); ++i) {
            Variable *v = variables[var_idx[i]];
            v_foreach (ParMult, j, v->recursive_derivatives()) {
                recursive_derivatives_.push_back(*j);
                recursive_derivatives_.back().mult *= derivatives_[i];
            }
        }
    }
    else if (nr_ == -2) {
        if (original_) {
            value_ = original_->value_;
            recursive_derivatives_ = original_->recursive_derivatives_;
        }
    }
    else
        assert(0);
}

void Variable::erased_parameter(int k)
{
    if (nr_ != -1 && nr_ > k)
            --nr_;
    for (vector<ParMult>::iterator i = recursive_derivatives_.begin();
                                        i != recursive_derivatives_.end(); ++i)
        if (i->p > k)
            -- i->p;
}

////////////////////////////////////////////////////////////////////////////
// from Spirit changelog:
// 1.8.5
// * For performance reasons, leaf_node_d/token_node_d have been changed to   
//   implicit lexems that create leaf nodes in one shot. The old              
//   token_node_d is still available and called reduced_node_d, now.
//
// The new leaf_node_d doesn't work here, so we use the old one
#if SPIRIT_VERSION >= 0x1805
#define leaf_node_d reduced_node_d
#endif

// don't use inner_node_d[], it returns wrong tree_parse_info::length
template <typename ScannerT>
FuncGrammar::definition<ScannerT>::definition(FuncGrammar const& /*self*/)
{
    //  Start grammar definition
    real_const  =  leaf_node_d[   real_p
                               |  as_lower_d[str_p("pi")]
                               |  '{' >> lexeme_d[+~ch_p('}') >> '}']
                              ];

    //"x" only in functions
    //all expressions but the last are for variables and functions
    //the last is for function types
    variable    = leaf_node_d[lexeme_d['$' >> +(alnum_p | '_')]]
                | leaf_node_d[lexeme_d['~' >> real_p]
                              >> !('[' >> !real_p >> "+-" >> real_p >> ']')]
                | leaf_node_d["~{" >> lexeme_d[+~ch_p('}') >> '}']
                              >> !('[' >> !real_p >> "+-" >> real_p >> ']')]
                | leaf_node_d[(lexeme_d["%" >> +(alnum_p | '_')]
                              | !lexeme_d['@' >> uint_p >> '.']
                                >> (str_p("F[")|"Z[") >> int_p >> ch_p(']')
                              )
                              >> '.' >> lexeme_d[alpha_p >> *(alnum_p | '_')]]
                | leaf_node_d[lexeme_d[alpha_p >> *(alnum_p | '_')]]
                ;

    exptoken    =  real_const
                //|  inner_node_d[ch_p('(') >> expression >> ')']
                |  discard_node_d[ch_p('(')]
                   >> expression
                   >> discard_node_d[ch_p(')')]
                |  root_node_d[ as_lower_d[ str_p("sqrt") | "exp"
                                          | "erfc" | "erf" | "log10" | "ln"
                                          | "sinh" | "cosh" | "tanh"
                                          | "sin" | "cos" | "tan"
                                          | "atan" | "asin" | "acos"
                                          | "lgamma" | "abs"
                                          ] ]
                   >>  inner_node_d[ch_p('(') >> expression >> ')']
                | root_node_d[ as_lower_d["voigt"] ]
                  >>  discard_node_d[ch_p('(')]
                  >>  expression
                  >>  discard_node_d[ch_p(',')]
                  >>  expression
                  >>  discard_node_d[ch_p(')')]
                |  variable
                ;

    signarg     =  exptoken >>
                   *(  (root_node_d[ch_p('^')] >> exptoken)
                    );

    factor      =  root_node_d[ch_p('-')] >> signarg
                |  discard_node_d[!ch_p('+')] >> signarg
                ;

    term        =  factor >>
                   *(  (root_node_d[ch_p('*')] >> factor)
                     | (root_node_d[ch_p('/')] >> factor)
                    );

    expression  =  term >>
                   *(  (root_node_d[ch_p('+')] >> term)
                     | (root_node_d[ch_p('-')] >> term)
                    );
}

// explicit template instantiation -- to accelerate compilation
#if SPIRIT_VERSION >= 0x1805
template FuncGrammar::definition<scanner<char const*, scanner_policies<skip_parser_iteration_policy<space_parser, iteration_policy>, ast_match_policy<char const*, node_val_data_factory<nil_t> >, action_policy> > >::definition(FuncGrammar const&);
//template FuncGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(FuncGrammar const&);
#else
template FuncGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(FuncGrammar const&);
//template FuncGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<no_actions_action_policy<action_policy> > > > >::definition(FuncGrammar const&);
#endif

/// small and slow utility function
/// uses calculate_deriv() to simplify formulae
string simplify_formula(string const &formula)
{
    tree_parse_info<> info = ast_parse(formula.c_str(), FuncG>>end_p, space_p);
    assert(info.full);
    const_tm_iter_t const &root = info.trees.begin();
    vector<string> vars(1, "x");
    vector<OpTree*> results = calculate_deriv(root, vars);
    string simplified = results.back()->str(&vars);
    purge_all_elements(results);
    return simplified;
}


FuncGrammar FuncG;

