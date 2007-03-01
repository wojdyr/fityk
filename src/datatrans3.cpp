// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

/// big grammars in Spirit take a lot of time and memory to compile
/// so they must be splitted into separate compilation units
/// that's the only reason why this file is not a part of datatrans.cpp

#include "datatrans3.h"
#include "datatrans4.h"
#include "datatrans2.h"
using namespace datatrans;


template <typename ScannerT>
DataE2Grammar::definition<ScannerT>::definition(DataE2Grammar const& /*self*/)
{
    index 
        =  ch_p('[') >> ( (ch_p('x') >> '=' >> DataExpressionG >> ch_p(']'))
                                                           [push_op(OP_x_IDX)]
                        | (DataExpressionG >> ch_p(']'))
                        )
        |  (!(ch_p('[') >> ']')) [push_op(OP_VAR_n)]
        ;

    //--------

    real_constant
        =  real_p [push_double()] 
        |  as_lower_d["pi"] [push_the_double(M_PI)]
        |  as_lower_d["true"] [push_the_double(1.)]
        |  as_lower_d["false"] [push_the_double(0.)]
#ifndef STANDALONE_DATATRANS
        |  VariableLhsG [push_var()]
        |  ((FunctionLhsG 
            | !lexeme_d['@' >> uint_p >> '.']
              >> ((str_p("F[")|"Z[") >> int_p >> ch_p(']')) 
            )
            >> lexeme_d['.' >> +(alnum_p|'_')])[push_func_param()]


#endif //not STANDALONE_DATATRANS
        ;

    parameterized_args
        =  ch_p('[') [push_op(OP_PLIST_BEGIN)]
            >> DataExpressionG % ch_p(',')[push_op(OP_PLIST_SEP)]
                >>  ch_p(']') [push_op(OP_PLIST_END)]
                    >> '(' >> DataExpressionG >> ')'
        ;

    real_variable 
        =  ("x" >> index) [push_op(OP_VAR_x)]
        |  ("y" >> index) [push_op(OP_VAR_y)]
        |  ("s" >> index) [push_op(OP_VAR_s)]
        |  ("a" >> index) [push_op(OP_VAR_a)]
        |  ("X" >> index) [push_op(OP_VAR_X)]
        |  ("Y" >> index) [push_op(OP_VAR_Y)]
        |  ("S" >> index) [push_op(OP_VAR_S)]
        |  ("A" >> index) [push_op(OP_VAR_A)]
        |  str_p("n") [push_op(OP_VAR_n)] 
        |  str_p("M") [push_op(OP_VAR_M)]
        ;

    aggregate_arg
        = '(' >> DataExpressionG 
          >> !(str_p("if") [push_op(OP_AGCONDITION)]
               >> DataExpressionG)
          >> ch_p(')') [push_op(OP_END_AGGREGATE)]
        ;

    rprec6 
        =   real_constant
        |   '(' >> DataExpressionG >> ')'
        |   DataExprFunG
            // `aggregate functions' will be refactored in replace_aggregates()
        |   as_lower_d["sum"] [push_op(OP_AGSUM)]
            >> aggregate_arg
        |   as_lower_d["min"] [push_op(OP_AGMIN)] //"min" is after "min2"
            >> aggregate_arg
        |   as_lower_d["max"] [push_op(OP_AGMAX)]
            >> aggregate_arg
        |   as_lower_d["avg"] [push_op(OP_AGAVG)]
            >> aggregate_arg
        |   as_lower_d["stddev"] [push_op(OP_AGSTDDEV)]
            >> aggregate_arg
        |   as_lower_d["darea"] [push_op(OP_AGAREA)]
            >> aggregate_arg
        |   (as_lower_d["interpolate"] >> parameterized_args)
                                          [parameterized_op(PF_INTERPOLATE)]
        |   (as_lower_d["spline"] >> parameterized_args)
                                          [parameterized_op(PF_SPLINE)]
        |   real_variable   //"s" is checked after "sin" and "sqrt"   
        ;
}

// explicit template instantiations 
template DataE2Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(DataE2Grammar const&);

template DataE2Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<no_actions_action_policy<action_policy> > > > >::definition(DataE2Grammar const&);

template DataE2Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, action_policy> > >::definition(DataE2Grammar const&);

DataE2Grammar DataE2G;


