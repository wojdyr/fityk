// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

/// big grammars in Spirit take a lot of time and memory to compile
/// so they must be splitted into separate compilation units
/// that's the only reason why this file is not a part of datatrans.cpp

#include "datatrans4.h"
#include "datatrans2.h"
using namespace datatrans;


template <typename ScannerT>
DataExprFunGrammar::definition<ScannerT>::definition(
                                            DataExprFunGrammar const& /*self*/)
{
#ifndef STANDALONE_DATATRANS
    func_or_f_or_z
        = FunctionLhsG 
          | (lexeme_d['@' >> uint_p >> '.']
            | eps_p 
            ) >> (ch_p('F')|'Z')
        ;
#endif //not STANDALONE_DATATRANS

    dfunc
        =   (as_lower_d["sqrt("] >> DataExpressionG >> ')') [push_op(OP_SQRT)] 
        |   (as_lower_d["gamma("] >> DataExpressionG >> ')')[push_op(OP_GAMMA)] 
        |   (as_lower_d["lgamma("] >> DataExpressionG >>')')[push_op(OP_LGAMMA)]
        |   (as_lower_d["erf("] >> DataExpressionG >> ')') [push_op(OP_ERF)] 
        |   (as_lower_d["exp("] >> DataExpressionG >> ')') [push_op(OP_EXP)] 
        |   (as_lower_d["log10("] >> DataExpressionG >> ')')[push_op(OP_LOG10)]
        |   (as_lower_d["ln("] >> DataExpressionG >> ')') [push_op(OP_LN)] 
        |   (as_lower_d["sin("] >> DataExpressionG >> ')') [push_op(OP_SIN)] 
        |   (as_lower_d["cos("] >> DataExpressionG >> ')') [push_op(OP_COS)] 
        |   (as_lower_d["tan("] >> DataExpressionG >> ')') [push_op(OP_TAN)] 
        |   (as_lower_d["atan("] >> DataExpressionG >> ')') [push_op(OP_ATAN)] 
        |   (as_lower_d["asin("] >> DataExpressionG >> ')') [push_op(OP_ASIN)] 
        |   (as_lower_d["acos("] >> DataExpressionG >> ')') [push_op(OP_ACOS)] 
        |   (as_lower_d["abs("] >> DataExpressionG >> ')') [push_op(OP_ABS)] 
        |   (as_lower_d["round("] >> DataExpressionG >> ')')[push_op(OP_ROUND)]

        |   (as_lower_d["min2"] >> '(' >> DataExpressionG >> ',' 
                                  >> DataExpressionG >> ')') [push_op(OP_MIN2)] 
        |   (as_lower_d["max2"] >> '(' >> DataExpressionG >> ',' 
                                  >> DataExpressionG >> ')') [push_op(OP_MAX2)] 
        |   (as_lower_d["randnormal"] >> '(' >> DataExpressionG >> ',' 
                              >> DataExpressionG >> ')') [push_op(OP_RANDNORM)] 
        |   (as_lower_d["randuniform"] >> '(' >> DataExpressionG >> ',' 
                              >> DataExpressionG >> ')') [push_op(OP_RANDU)] 
#ifndef STANDALONE_DATATRANS
        |   (func_or_f_or_z >> '(' >> DataExpressionG >> ')') [push_the_func()]
        |   as_lower_d["numarea"] >> '(' >> (func_or_f_or_z >> ',' 
                >> DataExpressionG >> ',' >> DataExpressionG >> ',' 
                >> DataExpressionG >> ')')[push_op(OP_NUMAREA)][push_the_func()]
        |   as_lower_d["findx"] >> '(' >> (func_or_f_or_z >> ',' 
                >> DataExpressionG >> ',' >> DataExpressionG >> ',' 
                >> DataExpressionG >> ')')[push_op(OP_FINDX)][push_the_func()]
        |   as_lower_d["extremum"] >> '(' >> (func_or_f_or_z >> ',' 
                >> DataExpressionG >> ',' >> DataExpressionG >> ')')
                                      [push_op(OP_FIND_EXTR)][push_the_func()]
#endif //not STANDALONE_DATATRANS
        ;
}

// explicit template instantiations 
template DataExprFunGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(DataExprFunGrammar const&);

template DataExprFunGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<no_actions_action_policy<action_policy> > > > >::definition(DataExprFunGrammar const&);

template DataExprFunGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, action_policy> > >::definition(DataExprFunGrammar const&);

DataExprFunGrammar DataExprFunG;


