// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef DATATRANS__H__
#define DATATRANS__H__


#include <boost/spirit/core.hpp>

#include "common.h"
using namespace boost::spirit;

struct Point;

bool transform_data(std::string const& str, 
                    std::vector<Point> const& old_points, 
                    std::vector<Point> &new_points);


// operators used in VM code
enum DataTransformVMOperator
{
    OP_NEG=1,   OP_EXP,   OP_SIN,   OP_COS,  OP_ATAN,  OP_ABS,  OP_ROUND, 
    OP_TAN/*!*/, OP_ASIN, OP_ACOS,
    OP_LOG10, OP_LN,  OP_SQRT,  OP_POW,   //these functions can set errno    
    OP_ADD,   OP_SUB,   OP_MUL,   OP_DIV/*!*/,  OP_MOD,
    OP_MIN,   OP_MAX,     
    OP_VAR_X, OP_VAR_Y, OP_VAR_S, OP_VAR_A, 
    OP_VAR_x, OP_VAR_y, OP_VAR_s, OP_VAR_a, 
    OP_VAR_n, OP_VAR_M, OP_NUMBER,  
    OP_OR, OP_AFTER_OR, OP_AND, OP_AFTER_AND, OP_NOT,
    OP_TERNARY, OP_TERNARY_MID, OP_AFTER_TERNARY, OP_DELETE_COND,
    OP_GT, OP_GE, OP_LT, OP_LE, OP_EQ, OP_NEQ, OP_NCMP_HACK, 
    OP_RANGE, OP_INDEX,
    OP_ASSIGN_X, OP_ASSIGN_Y, OP_ASSIGN_S, OP_ASSIGN_A,
    OP_DO_ONCE, OP_RESIZE, OP_ORDER, OP_DELETE, OP_BEGIN, OP_END, 
    OP_SUM, OP_IGNORE, 
    OP_PARAMETERIZED, OP_PLIST_BEGIN, OP_PLIST_END
};

// parametrized functions
enum {
    PF_INTERPOLATE, PF_SPLINE
};

//-- functors used in the grammar for putting VM code and data into vectors --

struct push_double
{
    void operator()(const double& n) const;
};


struct push_the_double: public push_double
{
    push_the_double(double d_) : d(d_) {}
    void operator()(char const*, char const*) const 
                                               { push_double::operator()(d); }
    void operator()(const char) const { push_double::operator()(d); }
    double d;
};

struct push_op
{
    push_op(int op_, int op2_=0) : op(op_), op2(op2_) {}
    void push() const;
    void operator()(char const*, char const*) const { push(); }
    void operator()(char) const { push(); }

    int op, op2;
};


struct parameterized_op
{
    parameterized_op(int op_) : op(op_) {}

    void push() const;
    void operator()(char const*, char const*) const { push(); }
    void operator()(char) const { push(); }

    int op;
};

//----------------------------  grammar  ----------------------------------
struct DataTransformGrammar : public grammar<DataTransformGrammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(DataTransformGrammar const& /*self*/)
    {
      BOOST_SPIRIT_DEBUG_RULE(rprec1);
      BOOST_SPIRIT_DEBUG_RULE(rprec2);
      BOOST_SPIRIT_DEBUG_RULE(rprec3);
      BOOST_SPIRIT_DEBUG_RULE(rprec4);
      BOOST_SPIRIT_DEBUG_RULE(rprec5);
      BOOST_SPIRIT_DEBUG_RULE(rprec6);
      BOOST_SPIRIT_DEBUG_RULE(real_constant);
      BOOST_SPIRIT_DEBUG_RULE(real_variable);
      BOOST_SPIRIT_DEBUG_RULE(index);
      BOOST_SPIRIT_DEBUG_RULE(rbool);
      BOOST_SPIRIT_DEBUG_RULE(rbool_or);
      BOOST_SPIRIT_DEBUG_RULE(rbool_and);
      BOOST_SPIRIT_DEBUG_RULE(range);
      BOOST_SPIRIT_DEBUG_RULE(assignment);
      BOOST_SPIRIT_DEBUG_RULE(statement);

        index 
            =  ch_p('[') >> rprec1 >> ch_p(']')
            |  (!(ch_p('[') >> ']')) [push_op(OP_VAR_n)]
            ;

        //--------

        real_constant
            =  real_p [push_double()] 
            |  as_lower_d["pi"] [push_the_double(M_PI)]
            |  as_lower_d["true"] [push_the_double(1.)]
            |  as_lower_d["false"] [push_the_double(0.)]
            ;

        parameterized_args
            =  ch_p('[') [push_op(OP_PLIST_BEGIN)]
                >> +real_constant 
                    >>  ch_p(']') [push_op(OP_PLIST_END)]
                        >> '(' >> rprec1 >> ')'
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

        rprec6 
            =   real_constant
            |   '(' >> rprec1 >> ')'
                // sum will be refactored, see: replace_sums()
            |   (as_lower_d["sum"] [push_op(OP_BEGIN)]
                    >> '(' >> rprec1 >> ')') [push_op(OP_SUM)]
            |   (as_lower_d["interpolate"] >> parameterized_args)
                                              [parameterized_op(PF_INTERPOLATE)]
            |   (as_lower_d["spline"] >> parameterized_args)
                                              [parameterized_op(PF_SPLINE)]
            |   (as_lower_d["sqrt"] >> '(' >> rprec1 >> ')') [push_op(OP_SQRT)] 
            |   (as_lower_d["exp"] >> '(' >> rprec1 >> ')') [push_op(OP_EXP)] 
            |   (as_lower_d["log10"] >> '(' >> rprec1 >> ')')[push_op(OP_LOG10)]
            |   (as_lower_d["ln"] >> '(' >> rprec1 >> ')') [push_op(OP_LN)] 
            |   (as_lower_d["sin"] >> '(' >> rprec1 >> ')') [push_op(OP_SIN)] 
            |   (as_lower_d["cos"] >> '(' >> rprec1 >> ')') [push_op(OP_COS)] 
            |   (as_lower_d["tan"] >> '(' >> rprec1 >> ')') [push_op(OP_TAN)] 
            |   (as_lower_d["atan"] >> '(' >> rprec1 >> ')') [push_op(OP_ATAN)] 
            |   (as_lower_d["asin"] >> '(' >> rprec1 >> ')') [push_op(OP_ASIN)] 
            |   (as_lower_d["acos"] >> '(' >> rprec1 >> ')') [push_op(OP_ACOS)] 
            |   (as_lower_d["abs"] >> '(' >> rprec1 >> ')') [push_op(OP_ABS)] 
            |   (as_lower_d["round"] >> '(' >> rprec1 >> ')')[push_op(OP_ROUND)]
            |   (as_lower_d["min"] >> '(' >> rprec1 >> ',' >> rprec1 >> ')')
                                                           [push_op(OP_MIN)] 
            |   (as_lower_d["max"] >> '(' >> rprec1 >> ',' >> rprec1 >> ')')
                                                           [push_op(OP_MAX)] 
            |   real_variable   //"s" is checked after "sin" and "sqrt"   
            ;

        rprec5 
            =   rprec6
            |   ('-' >> rprec6) [push_op(OP_NEG)]
            |   ('+' >> rprec6)
            ;

        rprec4 
            =   rprec5 
                >> *(
                      ('^' >> rprec5) [push_op(OP_POW)]
                    )
            ;
            
        rprec3 
            =   rprec4
                >> *(   ('*' >> rprec4)[push_op(OP_MUL)]
                    |   ('/' >> rprec4)[push_op(OP_DIV)]
                    |   ('%' >> rprec4)[push_op(OP_MOD)]
                    )
            ;

        rprec2 
            =   rprec3
                >> *(   ('+' >> rprec3) [push_op(OP_ADD)]
                    |   ('-' >> rprec3) [push_op(OP_SUB)]
                    )
            ;

        rbool
        // a hack for handling 10 < x < 20 
        // how it works:
        //  3 < 5.2
        //    op:    OP_NUMBER(3)    OP_NUMBER(5.2)   OP_LT
        //    stack:    ... 3         ... 3 5.2      ... 1 5.2
        //    stack end:    ^                 ^          ^
        //  3 < 5.2 < 4 (continued previous example)
        //    op:     OP_AND     OP_NCMP_HACK  OP_NUMBER(4) OP_LT  OP_AFTER_AND
        //    stack: ... 1 5.2   ... 5.2       ... 5.2 4   ... 0 4     (flag)
        //    s. end:  ^               ^               ^       ^  
            = rprec2
               >> !(( ("<=" >> rprec2) [push_op(OP_LE)]
                    | (">=" >> rprec2) [push_op(OP_GE)]
                    | ((str_p("==")|"=") >> rprec2)  [push_op(OP_EQ)]
                    | ((str_p("!=")|"<>") >> rprec2)  [push_op(OP_NEQ)]
                    | ('<' >> rprec2)  [push_op(OP_LT)]
                    | ('>' >> rprec2)  [push_op(OP_GT)]
                    )
                    >> *( (str_p("<=")[push_op(OP_AND, OP_NCMP_HACK)] 
                            >> rprec2) [push_op(OP_LE, OP_AFTER_AND)]
                        | (str_p(">=")[push_op(OP_AND, OP_NCMP_HACK)] 
                            >> rprec2) [push_op(OP_GE, OP_AFTER_AND)]
                        | ((str_p("==")|"=")[push_op(OP_AND, OP_NCMP_HACK)] 
                            >> rprec2) [push_op(OP_EQ, OP_AFTER_AND)]
                        | ((str_p("!=")|"<>")[push_op(OP_AND, OP_NCMP_HACK)] 
                            >> rprec2)  [push_op(OP_NEQ, OP_AFTER_AND)]
                        | (ch_p('<')[push_op(OP_AND, OP_NCMP_HACK)] 
                            >> rprec2) [push_op(OP_LT, OP_AFTER_AND)]
                        | (ch_p('>')[push_op(OP_AND, OP_NCMP_HACK)] 
                            >> rprec2) [push_op(OP_GT, OP_AFTER_AND)]
                        )
                   )
            ;

        rbool_not
            =  (as_lower_d["not"] >> rbool) [push_op(OP_NOT)]
            |  rbool
            ;

        rbool_and
            =  rbool_not
               >> *(
                    as_lower_d["and"] [push_op(OP_AND)] 
                    >> rbool_not
                   ) [push_op(OP_AFTER_AND)]
            ;

        rbool_or  
            =  rbool_and 
               >> *(
                    as_lower_d["or"] [push_op(OP_OR)] 
                    >> rbool_and
                   ) [push_op(OP_AFTER_OR)]
            ;  
   

        rprec1
            =  rbool_or 
               >> !(ch_p('?') [push_op(OP_TERNARY)] 
                    >> rbool_or 
                    >> ch_p(':') [push_op(OP_TERNARY_MID)] 
                    >> rbool_or
                   ) [push_op(OP_AFTER_TERNARY)]
            ;

        //--------

        range
            =  '[' >> 
                  ( eps_p(int_p >> ']')[push_op(OP_DO_ONCE)] 
                    >> int_p [push_double()] [push_op(OP_INDEX)]
                  | (
                      ch_p('n') [push_the_double(0.)] [push_the_double(0.)]
                    | (
                        ( int_p [push_double()]
                        | eps_p [push_the_double(0.)]
                        )
                         >> (str_p("...") | "..")
                         >> ( int_p [push_double()]
                            | eps_p [push_the_double(0)]
                            )
                       )
                    ) [push_op(OP_RANGE)] 
                  )  
                >> ']'
            ;

        order 
            =  ('-' >> as_lower_d["x"])        [push_the_double(-1.)]
            |  (!ch_p('+') >> as_lower_d["x"]) [push_the_double(+1.)]
            |  ('-' >> as_lower_d["y"])        [push_the_double(-2.)]
            |  (!ch_p('+') >> as_lower_d["y"]) [push_the_double(+2.)]
            |  ('-' >> as_lower_d["s"])        [push_the_double(-3.)]
            |  (!ch_p('+') >> as_lower_d["s"]) [push_the_double(+3.)]
            ;


        assignment //not only assignments
            =  (as_lower_d["x"] >> !range >>'='>> rprec1) [push_op(OP_ASSIGN_X)]
            |  (as_lower_d["y"] >> !range >>'='>> rprec1) [push_op(OP_ASSIGN_Y)]
            |  (as_lower_d["s"] >> !range >>'='>> rprec1) [push_op(OP_ASSIGN_S)]
            |  (as_lower_d["a"] >> !range >>'='>> rprec1) [push_op(OP_ASSIGN_A)]
            |  ((ch_p('M') >> '=') [push_op(OP_DO_ONCE)]  
               >> rprec1) [push_op(OP_RESIZE)]  
            |  ((as_lower_d["order"] >> '=') [push_op(OP_DO_ONCE)]  
               >> order) [push_op(OP_ORDER)] 
            |  (as_lower_d["delete"] >> eps_p('[') [push_op(OP_DO_ONCE)]  
                >> range) [push_op(OP_DELETE)]
            |  (as_lower_d["delete"] >> '(' >> rprec1 >> ')') 
                                                       [push_op(OP_DELETE_COND)]
            ;

        statement 
            = (eps_p[push_op(OP_BEGIN)] >> assignment >> eps_p[push_op(OP_END)])
               % ch_p('&') 
            ;
    }

    rule<ScannerT> rprec1, rprec2, rprec3, rprec4, rprec5, rprec6,  
                   rbool_or, rbool_and, rbool_not, rbool,
                   real_constant, real_variable, parameterized_args,
                   index, assignment, statement, range, order;

    rule<ScannerT> const& start() const { return statement; }
  };
} DataTransformG;




#endif //DATATRANS__H__
