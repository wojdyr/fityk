// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef CALC__H__
#define CALC__H__

#include <boost/spirit/core.hpp>
#include <boost/spirit/tree/ast.hpp>
using namespace boost::spirit;

// used for functions and variables
// there is a different set of opcodes for data transformation
enum 
{
    OP_CONSTANT=0,
    OP_VARIABLE, OP_PUT_VAL, OP_PUT_DERIV,
    OP_ONE_ARG,
    OP_NEG,   OP_EXP,   OP_SIN,   OP_COS,  OP_ATAN,  
    OP_TAN, OP_ASIN, OP_ACOS, OP_LOG10, OP_LN,  OP_SQRT,  
    OP_TWO_ARG,
    OP_POW, OP_MUL, OP_DIV, OP_ADD, OP_SUB,
    OP_END
};


///  function grammar
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
        definition(FuncGrammar const& /*self*/)
        {
            //  Start grammar definition
            real_const  =  leaf_node_d[ real_p |  as_lower_d[str_p("pi")] ];

            variable    =  leaf_node_d[lexeme_d[+alpha_p]];

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

struct OpTree
{
    int op;   // op < 0: variable (n=-op-1)
              // op == 0: constant
              // op > 0: operator
    OpTree *c1, 
           *c2;
    fp val;

    explicit OpTree(fp v) : op(0), c1(0), c2(0), val(v) {}
    explicit OpTree(int n, const std::string &/*s*/) 
                            : op(-n-1), c1(0), c2(0), val(0.) {}
    explicit OpTree(int n, OpTree *arg1);
    explicit OpTree(int n, OpTree *arg1, OpTree *arg2);

    ~OpTree() { delete c1; delete c2; }
    std::string str(const std::vector<std::string> *vars=0); 
    std::string str_b(bool b=true, const std::vector<std::string> *vars) 
                            { return b ? "(" + str(vars) + ")" : str(vars); } 
    std::string ascii_tree(int width=64, int start=0, 
                           const std::vector<std::string> *vars=0);
    OpTree *copy();
    //void swap_args() { assert(c1 && c2); OpTree *t=c1; c1=c2; c2=t; }
    OpTree* remove_c1() { OpTree *t=c1; c1=0; return t; }
    OpTree* remove_c2() { OpTree *t=c2; c2=0; return t; }
    void change_op(int op_) { op=op_; }
    bool operator==(const OpTree &t) { 
        return op == t.op && val == t.val 
               && (c1 == t.c1 || (c1 && t.c1 && *c1 == *t.c1)) 
               && (c2 == t.c2 || (c2 && t.c2 && *c2 == *t.c2));
    }
};

std::vector<std::string> 
find_tokens(int tokenID, const tree_parse_info<> &info);

typedef tree_match<char const*>::const_tree_iterator const_tm_iter_t;
std::vector<OpTree*> calculate_deriv(const_tm_iter_t const &i,
                                     std::vector<std::string> const &vars);
void add_calc_bytecode(const OpTree* tree, const std::vector<int> &vmvar_idx,
                       std::vector<int> &vmcode, std::vector<fp> &vmdata);

#endif

