// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "var.h"
#include "common.h"
//#include "datatrans.h"
#include "calc.h"
#include "ui.h"
#include <boost/spirit/core.hpp>
#include <algorithm>
#include <memory>

using namespace std;
using namespace boost::spirit;

const int stack_size = 8192;  //should be enough, 
                              //there are no checks for stack overflow  
vector<double> stack(stack_size);

Variable::Variable(const std::string &name_, int nr_, 
                   bool auto_delete_, bool hidden_)
    : nr(nr_), auto_delete(auto_delete_), hidden(hidden_)
{
    name = name_.empty() ? "var" + S(++unnamed_counter) : name_; 
}

Variable::Variable(const std::string &name_, const vector<string> &vmvar_,
                   const vector<OpTree*> &op_trees_, 
                   bool auto_delete_, bool hidden_)
    : nr(-1), auto_delete(auto_delete_), hidden(hidden_),
      derivatives(vmvar_.size()),
      op_trees(op_trees_), vmvar(vmvar_)
{
    name = name_.empty() ? "var" + S(++unnamed_counter) : name_; 
}


string Variable::get_formula() const
{
    return nr == -1 ? op_trees.back()->str(&vmvar) 
                    : "~" + S(parameters[nr]);
}

string Variable::get_info(bool extended) const 
{ 
    string s = "$" + name + ": " + get_formula() + " == " + S(value); 
    if (extended && nr == -1) {
        for (unsigned int i = 0; i < vmvar.size(); ++i)
            s += "\nd($" + name + ")/d($" + vmvar[i] + "): " 
                      + op_trees[i]->str(&vmvar) + " == " + S(derivatives[i]);
    }
    return s;
} 

void Variable::set_vmvar_idx(const vector<Variable*> &variables)
{
    const int n = vmvar.size();
    vmvar_idx.resize(n);
    for (int v = 0; v < n; ++v) {
        bool found = false;
        for (int i = 0; i < size(variables); ++i) {
            if (vmvar[v] == variables[i]->get_name()) {
                vmvar_idx[v] = i;
                found = true;
                break;
            }
        }
        if (!found)
            throw ExecuteError("Undefined variable: $" + vmvar[v]);
    }
}


int Variable::get_max_vmvar_idx() 
{
    if (vmvar_idx.empty()) 
        return -1; 
    else
       return *max_element(vmvar_idx.begin(), vmvar_idx.end());
}

void Variable::run_vm(const vector<Variable*> &variables)
{
    vector<double>::iterator stackPtr = stack.begin() - 1;//will be ++'ed first
    for (vector<int>::const_iterator i = vmcode.begin(); i!=vmcode.end(); i++) {
        switch (*i) {
            //unary operators
            case OP_NEG:
                *stackPtr = - *stackPtr;
                break;
            case OP_SQRT:
                *stackPtr = sqrt(*stackPtr);
                break;
            case OP_EXP:
                *stackPtr = exp(*stackPtr);
                break;
            case OP_LOG10:
                *stackPtr = log10(*stackPtr); 
                break;
            case OP_LN:
                *stackPtr = log(*stackPtr); 
                break;
            case OP_SIN:
                *stackPtr = sin(*stackPtr);
                break;
            case OP_COS:
                *stackPtr = cos(*stackPtr);
                break;
            case OP_TAN:
                *stackPtr = tan(*stackPtr); 
                break;
            case OP_ATAN:
                *stackPtr = atan(*stackPtr); 
                break;
            case OP_ASIN:
                *stackPtr = asin(*stackPtr); 
                break;
            case OP_ACOS:
                *stackPtr = acos(*stackPtr); 
                break;

            //binary operators
            case OP_ADD:
                stackPtr--;
                *stackPtr += *(stackPtr+1);
                break;
            case OP_SUB:
                stackPtr--;
                *stackPtr -= *(stackPtr+1);
                break;
            case OP_MUL:
                stackPtr--;
                *stackPtr *= *(stackPtr+1);
                break;
            case OP_DIV:
                stackPtr--;
                *stackPtr /= *(stackPtr+1);
                break;
            case OP_POW:
                stackPtr--;
                *stackPtr = pow(*stackPtr, *(stackPtr+1));
                break;

            // putting-number-to-stack-operators
            // stack overflow not checked
            case OP_CONSTANT:
                stackPtr++;
                i++;
                *stackPtr = vmdata[*i];
                break;
            case OP_VARIABLE:
                stackPtr++;
                i++;
                *stackPtr = variables[*i]->get_value();
                break;
            //assignment-operators
            case OP_PUT_VAL:
                value = *stackPtr;
                stackPtr--; 
                break;
            case OP_PUT_DERIV:
                i++;
                derivatives[*i] = *stackPtr;
                stackPtr--; 
                break;

            default:
                assert(0); //("Unknown operator in VM code: " + S(*i))
        }
    }
    assert(stackPtr == stack.begin() - 1);
}

void Variable::recalculate(const vector<Variable*> &variables, 
                           const vector<fp>& parameters)
{
  if (nr == -1) {
      run_vm(variables);
  }
  else {
      value = parameters[nr];
      if (!derivatives.empty())
          derivatives.clear();
  }
}

bool Variable::is_dependent_on(int idx, const vector<Variable*> &variables)
{
    for (vector<int>::const_iterator i = vmvar_idx.begin(); 
            i != vmvar_idx.end(); ++i)
        if (*i == idx || variables[*i]->is_dependent_on(idx, variables))
            return true;
    return false;
}

bool Variable::is_directly_dependent_on(int idx) {
    return count(vmvar_idx.begin(), vmvar_idx.end(), idx);
}

void Variable::tree_to_bytecode()
{
    if (nr == -1) {
        assert(vmvar_idx.size() + 1 == op_trees.size()); 
        int n = vmvar_idx.size();
        vmcode.clear();
        vmdata.clear();
        add_calc_bytecode(op_trees.back(), vmvar_idx, vmcode, vmdata);
        vmcode.push_back(OP_PUT_VAL);
        for (int i = 0; i < n; ++i) {
            add_calc_bytecode(op_trees[i], vmvar_idx, vmcode, vmdata);
            vmcode.push_back(OP_PUT_DERIV);
            vmcode.push_back(i);
        }
    }
}


void sort_variables(vector<Variable*> &variables)
{
    for (vector<Variable*>::iterator i = variables.begin(); 
            i != variables.end(); ++i)
        (*i)->set_vmvar_idx(variables);
    int pos = 0;
    while (pos < size(variables)) {
        int M = variables[pos]->get_max_vmvar_idx();
        if (M > pos) {
            swap(variables[pos], variables[M]);
            for (vector<Variable*>::iterator i = variables.begin(); 
                    i != variables.end(); ++i)
                (*i)->set_vmvar_idx(variables);
        }
        else
            ++pos;
    }
}


Variable *create_simple_variable(const string &name, const string &rhs)
{
    assert(rhs.size() > 1 && rhs[0] == '~');
    fp val = get_constant_value(string(rhs, 1));
    parameters.push_back(val);
    int nr = parameters.size() - 1;
    Variable *var = new Variable(name, nr);
    return var;
}

Variable *create_variable(const string &name, const string &rhs)
{
    tree_parse_info<> info = ast_parse(rhs.c_str(), VariableRhsG, space_p);
    assert(info.full);
    const_tm_iter_t const &root = info.trees.begin();
    if (root->value.id() == VariableRhsGrammar::variableID
            && *root->value.begin() == '~') {
        string root_str = string(root->value.begin(), root->value.end());
        return create_simple_variable(name, root_str);
    }
    vector<string> vmvar = find_tokens(VariableRhsGrammar::variableID, info);
    vector<OpTree*> op_trees = calculate_deriv(root, vmvar);
    // ~14.3 -> $var4
    for (vector<string>::iterator i = vmvar.begin(); i != vmvar.end(); ++i) {
        assert(i->size() >= 1);
        if ((*i)[0] == '~') {
            *i = assign_variable("", *i);
        }
        else if ((*i)[0] == '$')
            *i = string(i->begin()+1, i->end());
    }
    return new Variable(name, vmvar, op_trees);
}

void remove_unreffered()
{
    //TODO
}

string assign_variable(const string &name, const string &rhs)
{
    auto_ptr<Variable> var(create_variable(name, rhs));
    string var_name = var->get_name();
    var->set_vmvar_idx(variables);
    var->tree_to_bytecode();
    int old_pos = find_variable_nr(var->get_name());
    if (old_pos == -1) {
        var->recalculate(variables, parameters);
        variables.push_back(var.release());
    }
    else {
        if (var->is_dependent_on(old_pos, variables)) { //check for loops
            throw ExecuteError("detected loop in variable dependencies");
        }
        delete variables[old_pos];
        variables[old_pos] = var.release();
        if (variables[old_pos]->get_max_vmvar_idx() > old_pos) {
            sort_variables(variables);
            for (vector<Variable*>::iterator i = variables.begin(); 
                    i != variables.end(); ++i)
                (*i)->tree_to_bytecode();
        }
        recalculate_variables();
        remove_unreffered();
    }
    return var_name;
}

void delete_variables(const vector<string> &names)
{
    const int n = names.size();
    vector<int> nrs (n);
    for (int i = 0; i < n; ++i) {
        nrs[i] = find_variable_nr(names[i]);
        if (nrs[i] == -1)
            throw ExecuteError("undefined variable: " + names[i]);
        //check if it is reffered by other variable
        for (vector<Variable*>::iterator j = variables.begin(); 
                j != variables.end(); ++j) {
            if ((*j)->is_directly_dependent_on(nrs[i])
                    && find(names.begin(), names.end(), (*j)->get_name()) 
                        == names.end()) {
                throw ExecuteError("can't delete $" + names[i] + " because $"
                                   + (*j)->get_name() + " depends on it.");
            }
        //TODO check if it is reffered by functions
        }
    }
    sort(nrs.begin(), nrs.end());
    for (int i = n-1; i >= 0; --i) {
        int k = nrs[i];
        delete variables[k];
        variables.erase(variables.begin() + k);
    }
    remove_unreffered();
}


int find_variable_nr(const string &name) {
    for (int i = 0; i < size(variables); ++i)
        if (variables[i]->get_name() == name)
            return i;
    return -1;
}

const Variable* find_variable(const string &name) {
    int n = find_variable_nr(name);
    return n == -1 ? 0 : variables[n];
}


int get_variable_value(const string &name) {
    for (int i = 0; i < size(variables); ++i)
        if (variables[i]->get_name() == name)
            return i;
    return -1;
}


int find_parameter_variable(int par)
{
    for (int i = 0; i < size(variables); ++i)
        if (variables[i]->get_nr() == par)
            return i;
    return -1;
}

void recalculate_variables()
{
    for (vector<Variable*>::iterator i = variables.begin(); 
                i != variables.end(); ++i)
        (*i)->recalculate(variables, parameters);
}


template <typename ScannerT>
VariableRhsGrammar::definition<ScannerT>::definition(
                                          VariableRhsGrammar const& /*self*/)
{
    //  Start grammar definition
    real_const  =  leaf_node_d[   real_p 
                               |  as_lower_d[str_p("pi")] 
                               | '{' >> +~ch_p('}') >> '}' 
                              ];

    variable    =  leaf_node_d[lexeme_d['$' >> +(alnum_p | '_')]]
    // variable    =  root_node_d[VariableLhsG] //FIXME: why it doesn't work???
                |  leaf_node_d[lexeme_d['~' >> real_p]]
                //|  leaf_node_d["~{" >> no_actions_d[DataTransformG] >> '}']
                |  leaf_node_d["~{" >> +~ch_p('}') >> '}']
                ;

    exptoken    =  real_const
                |  inner_node_d[ch_p('(') >> expression >> ')']
                |  root_node_d[ as_lower_d[ str_p("sqrt") 
                                          | "exp" | "log10" | "ln" 
                                          | "sin" | "cos" | "tan" 
                                          | "atan" | "asin" | "acos"
                                          ] ]
                   >>  inner_node_d[ch_p('(') >> expression >> ')']
                |  (root_node_d[ch_p('-')] >> exptoken)
                |  variable;

    factor      =  exptoken >>
                   *(  (root_node_d[ch_p('^')] >> exptoken)
                    );

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
template VariableRhsGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(VariableRhsGrammar const&);



VariableRhsGrammar VariableRhsG;
VariableLhsGrammar VariableLhsG;
int Variable::unnamed_counter = 0;
vector<Parameter> parameters;
vector<Variable*> variables; 


