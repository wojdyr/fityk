// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__DATATRANS2__H__
#define FITYK__DATATRANS2__H__

/// big grammars in Spirit take a lot of time and memory to compile
/// so they must be splitted into separate compilation units
/// that's the only reason why this file is not a part of datatrans.cpp
/// code here was originally part of datatrans.cpp (yes, .cpp)
///
/// this file is included only by datatrans*.cpp

#include "datatrans.h"
#include "common.h"
#include "data.h"
#include "var.h"
#include "func.h"
#include "numfuncs.h"
#include "logic.h"
#include "fityk.h"
#include <boost/spirit/core.hpp>

using namespace std;
using namespace boost::spirit;

namespace datatrans {

/// used in data expressions, a base class for eg. SplineFunction
class ParameterizedFunction
{
public:
    ParameterizedFunction(std::vector<fp> const& params_, 
                          std::map<int, std::vector<int> > const& pcodes_) 
        : params(params_), pcodes(pcodes_) {}
    virtual ~ParameterizedFunction() {}
    virtual fp calculate(fp x) = 0;
    void prepare_parameters(std::vector<Point> const& points);
    virtual void do_prepare() = 0;
protected:
    std::vector<fp> params;
    std::map<int, std::vector<int> > pcodes;
};


extern vector<int> code;        //  VM code 
extern vector<fp> numbers;  //  VM data 
extern vector<ParameterizedFunction*> parameterized; // also used by VM 
extern const int stack_size;  //should be enough, 
                              //there are no checks for stack overflow  

inline bool x_lt(const Point &p1, const Point &p2) { return p1.x < p2.x; }
inline bool x_gt(const Point &p1, const Point &p2) { return p1.x > p2.x; }
inline bool y_lt(const Point &p1, const Point &p2) { return p1.y < p2.y; }
inline bool y_gt(const Point &p1, const Point &p2) { return p1.y > p2.y; }
inline bool sigma_lt(const Point &p1, const Point &p2) 
                                              { return p1.sigma < p2.sigma; }
inline bool sigma_gt(const Point &p1, const Point &p2) 
                                              { return p1.sigma > p2.sigma; }
inline bool active_lt(const Point &p1, const Point &p2) 
                                        { return p1.is_active < p2.is_active; }
inline bool active_gt(const Point &p1, const Point &p2) 
                                        { return p1.is_active > p2.is_active; }

/// operators used in VM code
enum DataTransformVMOperator
{
    OP_NEG=-200, OP_EXP, OP_ERFC, OP_ERF, OP_SIN, OP_COS,  OP_TAN,  
    OP_SINH, OP_COSH, OP_TANH, OP_ABS,  OP_ROUND, 
    OP_ATAN, OP_ASIN, OP_ACOS, OP_LOG10, OP_LN,  OP_SQRT,  OP_POW,   
    OP_GAMMA, OP_LGAMMA, OP_VOIGT,
    OP_ADD,   OP_SUB,   OP_MUL,   OP_DIV,  OP_MOD,
    OP_MIN2,   OP_MAX2, OP_RANDNORM, OP_RANDU,    
    OP_VAR_X, OP_VAR_FIRST_OP=OP_VAR_X, OP_VAR_Y, OP_VAR_S, OP_VAR_A, 
    OP_VAR_x, OP_VAR_y, OP_VAR_s, OP_VAR_a, 
    OP_VAR_n, OP_VAR_M, OP_VAR_LAST_OP=OP_VAR_M,
    OP_NUMBER,  
    OP_OR, OP_AFTER_OR, OP_AND, OP_AFTER_AND, OP_NOT,
    OP_TERNARY, OP_TERNARY_MID, OP_AFTER_TERNARY, OP_DELETE_COND,
    OP_GT, OP_GE, OP_LT, OP_LE, OP_EQ, OP_NEQ, OP_NCMP_HACK, 
    OP_RANGE, OP_INDEX, OP_x_IDX,
    OP_ASSIGN_X, OP_ASSIGN_Y, OP_ASSIGN_S, OP_ASSIGN_A,
    OP_DO_ONCE, OP_RESIZE, OP_ORDER, OP_DELETE, OP_BEGIN, OP_END, 
    OP_END_AGGREGATE, OP_AGCONDITION, 
    OP_AGSUM, OP_AGMIN, OP_AGMAX, OP_AGAREA, OP_AGAVG, OP_AGSTDDEV,
    OP_PARAMETERIZED, OP_PLIST_BEGIN, OP_PLIST_SEP, OP_PLIST_END,
    OP_FUNC, OP_SUM_F, OP_SUM_Z, OP_NUMAREA, OP_FINDX, OP_FIND_EXTR
};

/// parametrized functions
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

#ifndef STANDALONE_DATATRANS
struct push_var: public push_double
{
    void operator()(char const* a, char const* b) const 
     { push_double::operator()(AL->find_variable(string(a+1,b))->get_value()); }
};

struct push_func_param: public push_double
{
    void operator()(char const* a, char const* b) const;
};

struct push_func
{
    void operator()(char const* a, char const* b) const;
};

#endif //not STANDALONE_DATATRANS

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


} //namespace

#endif 
