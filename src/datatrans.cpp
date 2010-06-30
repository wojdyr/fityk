// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

// the idea of VM is based on one of boost::spirit samples - vmachine_calc

// this file can be compiled to stand-alone test program:
// $ g++ -I../3rdparty -DSTANDALONE_DATATRANS datatrans*.cpp numfuncs.cpp -o dt
// $ ./dt

// how it works:
//   * parse string and prepare VM code (list of operators)
//     and data (list of numbers). There is one VM code and
//     data array for both do-once operations
//     for for-many-points operations (perhaps it should be separated).
//
//   * execute do-once operations (eg. x[3]=4)
//     and computes value of aggregate functions
//
//   * execute all assignments for every point (from first to last),
//     unless the point is outside of specified range.
//


//#define BOOST_SPIRIT_DEBUG


//#include "datatrans.h"
#include "datatrans2.h"
#include "model.h"
#include "voigt.h"

#include <boost/math/special_functions/gamma.hpp>

//using namespace std;
//using namespace boost::spirit::classic;
using namespace datatrans;

//#include "ui.h"
//#define DT_DEBUG(x) msg(x);

#ifdef STANDALONE_DATATRANS

#include <iostream>
bool dt_verbose = false;
#define DT_DEBUG(x) if (dt_verbose) std::cout << (x) << std::endl;
//-------------------------  Main program  ---------------------------------
#include <string.h>

int main(int argc, char **argv)
{
    cout << "DataTransformVMachine test started...\n";
    if (argc > 1 && !strcmp(argv[1], "-v")) {
        cout << "[verbose mode]" << endl;
        dt_verbose=true;
    }
    cout << "==> ";

    string str;
    while (getline(cin, str))
    {
        vector<Point> points;

        points.push_back(Point(0.1, 4, 1));
        points.push_back(Point(0.2, 2, 1));
        points.push_back(Point(0.3, -2, 1));
        points.push_back(Point(0.4, 3, 1));
        points.push_back(Point(0.5, 3, 1));

        try {
            vector<Point> transformed_points = transform_data(str, points);
            int len = (int) points.size();
            for (int n = 0; n != (int) transformed_points.size(); n++)
                cout << "point " << n << ": "
                    << (n < len ? points[n] : Point()).str()
                    << " -> " << transformed_points[n].str() << endl;
        } catch (ExecuteError& e) {
            cout << e.what() << endl;
        }
        cout << "==> ";
    }
    return 0;
}

#else

#  ifndef DT_DEBUG
#    define DT_DEBUG(x) ;
#  endif

#endif //STANDALONE_DATATRANS


//----------------------------  grammar  ----------------------------------
template <typename ScannerT>
DataTransformGrammar::definition<ScannerT>::definition(
                                          DataTransformGrammar const& /*self*/)
{
    range
        =  ch_p('[') [push_op(OP_DO_ONCE)]
           >> int_p [push_double()] [push_op(OP_INDEX)]
           >> ']'
        ;

    assignment //not only assignments
        =  (as_lower_d["x"] >> !range >> '=' >> DataExpressionG)
                                                     [push_op(OP_ASSIGN_X)]
        |  (as_lower_d["y"] >> !range >> '=' >> DataExpressionG)
                                                     [push_op(OP_ASSIGN_Y)]
        |  (as_lower_d["s"] >> !range >> '=' >> DataExpressionG)
                                                     [push_op(OP_ASSIGN_S)]
        |  (as_lower_d["a"] >> !range >> '=' >> DataExpressionG)
                                                     [push_op(OP_ASSIGN_A)]
        |  ((ch_p('M') >> '=') [push_op(OP_DO_ONCE)]
           >> DataExpressionG) [push_op(OP_RESIZE)]
        ;

    statement
        = (eps_p[push_op(OP_BEGIN)] >> assignment >> eps_p[push_op(OP_END)])
                                                             % ch_p(',')
        ;
}

// explicit template instantiations
template DataTransformGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(DataTransformGrammar const&);

template DataTransformGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<no_actions_action_policy<action_policy> > > > >::definition(DataTransformGrammar const&);

DataTransformGrammar DataTransformG;


namespace datatrans {

/// debuging utility
#define OP_(x) \
    if (op == OP_##x) return #x;
string dt_op(int op)
{
    OP_(NEG)   OP_(EXP)
    OP_(SIN)   OP_(COS)  OP_(TAN)  OP_(SINH) OP_(COSH)  OP_(TANH)
    OP_(ABS)  OP_(ROUND)
    OP_(ATAN) OP_(ASIN) OP_(ACOS)
    OP_(LOG10) OP_(LN)  OP_(SQRT)  OP_(POW)
    OP_(GAMMA) OP_(LGAMMA) OP_(VOIGT) OP_(XINDEX)
    OP_(ADD)   OP_(SUB)   OP_(MUL)   OP_(DIV)  OP_(MOD)
    OP_(MIN2)   OP_(MAX2) OP_(RANDNORM) OP_(RANDU)
    OP_(VAR_X) OP_(VAR_Y) OP_(VAR_S) OP_(VAR_A)
    OP_(VAR_x) OP_(VAR_y) OP_(VAR_s) OP_(VAR_a)
    OP_(VAR_n) OP_(VAR_M) OP_(NUMBER)
    OP_(OR) OP_(AFTER_OR) OP_(AND) OP_(AFTER_AND) OP_(NOT)
    OP_(TERNARY) OP_(TERNARY_MID) OP_(AFTER_TERNARY)
    OP_(GT) OP_(GE) OP_(LT) OP_(LE) OP_(EQ) OP_(NEQ)
    OP_(INDEX)
    OP_(ASSIGN_X) OP_(ASSIGN_Y) OP_(ASSIGN_S) OP_(ASSIGN_A)
    OP_(DO_ONCE) OP_(RESIZE) OP_(BEGIN) OP_(END)
    OP_(END_AGGREGATE) OP_(AGCONDITION)
    OP_(AGSUM) OP_(AGMIN) OP_(AGMAX) OP_(AGAREA) OP_(AGAVG) OP_(AGSTDDEV)
    OP_(FUNC) OP_(SUM_F) OP_(SUM_Z) OP_(NUMAREA) OP_(FINDX) OP_(FIND_EXTR)
    return S(op);
};

/// debuging utility
string dt_ops(vector<int> const& code)
{
    string r;
    for (std::vector<int>::const_iterator i=code.begin(); i != code.end(); ++i)
        if (*i < 0)
            r += dt_op(*i) + " ";
        else
            r += "[" + S(*i) + "] ";
    return r;
}

string get_code_as_text(vector<int> const& code, vector<fp> const& numbers)
{
    string txt;
    for (vector<int>::const_iterator i = code.begin(); i != code.end(); ++i) {
        txt += " " + dt_op(*i);
        if (*i == OP_NUMBER && i+1 != code.end()) {
            ++i;
            txt += "(" + S(numbers[*i]) + ")";
        }
    }
    return txt;
}
//------------------------  Virtual Machine  --------------------------------

vector<int> code;        //  VM code
vector<fp> numbers;  //  VM data (numeric values)
const int stack_size = 128;  //should be enough,
                              //there are no checks for stack overflow

void clear_parse_vecs()
{
    code.clear();
    numbers.clear();
}

vector<int>::const_iterator
skip_code(vector<int>::const_iterator i, int start_op, int finish_op)
{
    int counter = 1;
    while (counter) {
        ++i;
        if (*i == finish_op) counter--;
        else if (*i == start_op)  counter++;
    }
    return i;
}

void skip_to_end(vector<int>::const_iterator &i)
{
    DT_DEBUG("SKIPing")
    while (*i != OP_END)
        ++i;
}

template<typename T>
fp get_var_with_idx(fp idx, vector<Point> const& points, T Point::*t)
{
    if (points.empty())
        return 0.;
    else if (idx <= 0)
        return points[0].*t;
    else if (idx >= points.size() - 1)
        return points.back().*t;
    else if (is_eq(idx, iround(idx)))
        return points[iround(idx)].*t;
    else {
        int flo = int(floor(idx));
        fp fra = idx - flo;
        return (1-fra) * fp(points[flo].*t) + fra * fp(points[flo+1].*t);
    }
}

/// returns floating-point "index" of x in sorted vec of points
fp find_idx_in_sorted(vector<Point> const& pp, fp x)
{
    if (pp.empty())
        return 0;
    else if (x <= pp.front().x)
        return 0;
    else if (x >= pp.back().x)
        return pp.size() - 1;
    vector<Point>::const_iterator i = lower_bound(pp.begin(), pp.end(),
                                                  Point(x, 0));
    assert (i > pp.begin() && i < pp.end());
    if (is_eq(x, i->x))
            return i - pp.begin();
    else
        return i - pp.begin() - (i->x - x) / (i->x - (i-1)->x);
}


// Return value:
//  if once==true (one-time pass).
//   true means: it is neccessary to call this function for every point.
// n: index of point; //     special value: n==M: one-time operations
//     n(==M) can be changed in OP_INDEX
// M: number of all points (==new_points.size())
bool execute_code(int n, int &M, vector<fp>& stack,
                  vector<Point> const& old_points, vector<Point>& new_points,
                  vector<int> const& code)
{
    DT_DEBUG("executing code; n=" + S(n) + " M=" + S(M))
    assert(M == size(new_points));
    bool once = (n == M);
    bool return_value = false;
    vector<fp>::iterator stackPtr = stack.begin() - 1;//will be ++'ed first

#define STACK_OP
#define STACK_OFFSET_CHANGE(ch) stackPtr+=(ch);

    for (vector<int>::const_iterator i=code.begin(); i != code.end(); i++) {
//---8<--- START OF DT COMMON BLOCK --------------------------------------
        DT_DEBUG("op " + dt_op(*i))
        switch (*i) {
            //unary-operators
            case OP_NEG:
                STACK_OP *stackPtr = - *stackPtr;
                break;
            case OP_SQRT:
                STACK_OP *stackPtr = sqrt(*stackPtr);
                break;
            case OP_GAMMA:
                STACK_OP *stackPtr = boost::math::tgamma(*stackPtr);
                break;
            case OP_LGAMMA:
                STACK_OP *stackPtr = boost::math::lgamma(*stackPtr);
                break;
            case OP_EXP:
                STACK_OP *stackPtr = exp(*stackPtr);
                break;
            case OP_ERFC:
                STACK_OP *stackPtr = erfc(*stackPtr);
                break;
            case OP_ERF:
                STACK_OP *stackPtr = erf(*stackPtr);
                break;
            case OP_LOG10:
                STACK_OP *stackPtr = log10(*stackPtr);
                break;
            case OP_LN:
                STACK_OP *stackPtr = log(*stackPtr);
                break;
            case OP_SIN:
                STACK_OP *stackPtr = sin(*stackPtr);
                break;
            case OP_COS:
                STACK_OP *stackPtr = cos(*stackPtr);
                break;
            case OP_TAN:
                STACK_OP *stackPtr = tan(*stackPtr);
                break;
            case OP_SINH:
                STACK_OP *stackPtr = sinh(*stackPtr);
                break;
            case OP_COSH:
                STACK_OP *stackPtr = cosh(*stackPtr);
                break;
            case OP_TANH:
                STACK_OP *stackPtr = tanh(*stackPtr);
                break;
            case OP_ATAN:
                STACK_OP *stackPtr = atan(*stackPtr);
                break;
            case OP_ASIN:
                STACK_OP *stackPtr = asin(*stackPtr);
                break;
            case OP_ACOS:
                STACK_OP *stackPtr = acos(*stackPtr);
                break;
            case OP_ABS:
                STACK_OP *stackPtr = fabs(*stackPtr);
                break;
            case OP_ROUND:
                STACK_OP *stackPtr = floor(*stackPtr + 0.5);
                break;

            case OP_XINDEX:
                STACK_OP *stackPtr = find_idx_in_sorted(old_points, *stackPtr);
                break;

#ifndef STANDALONE_DATATRANS
            case OP_FUNC:
                i++;
                STACK_OP
                *stackPtr = AL->get_function(*i)->calculate_value(*stackPtr);
                break;
            case OP_SUM_F:
                i++;
                STACK_OP *stackPtr = AL->get_model(*i)->value(*stackPtr);
                break;
            case OP_SUM_Z:
                i++;
                STACK_OP *stackPtr = AL->get_model(*i)->zero_shift(*stackPtr);
                break;
            case OP_NUMAREA:
                i += 2;
                STACK_OFFSET_CHANGE(-2)
                if (*(i-1) == OP_FUNC) {
                    STACK_OP
                    *stackPtr = AL->get_function(*i)->numarea(*stackPtr,
                                        *(stackPtr+1), iround(*(stackPtr+2)));
                }
                else if (*(i-1) == OP_SUM_F) {
                    STACK_OP
                    *stackPtr = AL->get_model(*i)->numarea(*stackPtr,
                                        *(stackPtr+1), iround(*(stackPtr+2)));
                }
                else // OP_SUM_Z
                    throw ExecuteError("numarea(Z,...) is not implemented."
                                       "Does anyone need it?");
                break;

            case OP_FINDX:
                i += 2;
                STACK_OFFSET_CHANGE(-2)
                if (*(i-1) == OP_FUNC) {
                    STACK_OP
                    *stackPtr = AL->get_function(*i)->find_x_with_value(
                                      *stackPtr, *(stackPtr+1), *(stackPtr+2));
                }
                else if (*(i-1) == OP_SUM_F) {
                    throw ExecuteError("findx(F,...) is not implemented. "
                                       "Does anyone need it?");
                }
                else // OP_SUM_Z
                    throw ExecuteError("findx(Z,...) is not implemented. "
                                       "Does anyone need it?");
                break;

            case OP_FIND_EXTR:
                i += 2;
                STACK_OFFSET_CHANGE(-1)
                if (*(i-1) == OP_FUNC) {
                    STACK_OP
                    *stackPtr = AL->get_function(*i)->find_extremum(*stackPtr,
                                                                *(stackPtr+1));
                }
                else if (*(i-1) == OP_SUM_F) {
                    throw ExecuteError("extremum(F,...) is not implemented. "
                                       "Does anyone need it?");
                }
                else // OP_SUM_Z
                    throw ExecuteError("extremum(Z,...) is not implemented. "
                                       "Does anyone need it?");
                break;
#endif //not STANDALONE_DATATRANS

            //binary-operators
            case OP_MIN2:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr = min(*stackPtr, *(stackPtr+1));
                break;
            case OP_MAX2:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr = max(*stackPtr, *(stackPtr+1));
                break;
            case OP_RANDU:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr = rand_uniform(*stackPtr, *(stackPtr+1));
                break;
            case OP_RANDNORM:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr += rand_gauss() * *(stackPtr+1);
                break;
            case OP_ADD:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr += *(stackPtr+1);
                break;
            case OP_SUB:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr -= *(stackPtr+1);
                break;
            case OP_MUL:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr *= *(stackPtr+1);
                break;
            case OP_DIV:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr /= *(stackPtr+1);
                break;
            case OP_MOD:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP
                *stackPtr -= floor(*stackPtr / *(stackPtr+1)) * *(stackPtr+1);
                break;
            case OP_POW:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr = pow(*stackPtr, *(stackPtr+1));
                break;
            case OP_VOIGT:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr = humlik(*stackPtr, *(stackPtr+1))
                                                               / sqrt(M_PI);
                break;

            // comparisions
            case OP_LT:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr = is_lt(*stackPtr, *(stackPtr+1));
                break;
            case OP_GT:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr = is_gt(*stackPtr, *(stackPtr+1));
                break;
            case OP_LE:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr = is_le(*stackPtr, *(stackPtr+1));
                break;
            case OP_GE:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr = is_ge(*stackPtr, *(stackPtr+1));
                break;
            case OP_EQ:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr = is_eq(*stackPtr, *(stackPtr+1));
                break;
            case OP_NEQ:
                STACK_OFFSET_CHANGE(-1)
                STACK_OP *stackPtr = is_neq(*stackPtr, *(stackPtr+1));
                break;

            // putting-number-to-stack-operators
            // stack overflow not checked
            case OP_NUMBER:
                STACK_OFFSET_CHANGE(+1)
                i++;
                STACK_OP *stackPtr = numbers[*i];
                break;
            case OP_VAR_n:
                STACK_OFFSET_CHANGE(+1)
                STACK_OP *stackPtr = static_cast<fp>(n);
                break;
            case OP_VAR_M:
                STACK_OFFSET_CHANGE(+1)
                STACK_OP *stackPtr = static_cast<fp>(new_points.size());
                break;
            case OP_VAR_x:
                STACK_OP
                *stackPtr = get_var_with_idx(*stackPtr, old_points, &Point::x);
                break;
            case OP_VAR_y:
                STACK_OP
                *stackPtr = get_var_with_idx(*stackPtr, old_points, &Point::y);
                break;
            case OP_VAR_s:
                STACK_OP
                *stackPtr = get_var_with_idx(*stackPtr, old_points,
                                             &Point::sigma);
                break;
            case OP_VAR_a:
                STACK_OP
                *stackPtr = bool(iround(get_var_with_idx(*stackPtr, old_points,
                                                         &Point::is_active)));
                break;
            case OP_VAR_X:
                STACK_OP
                *stackPtr = get_var_with_idx(*stackPtr, new_points, &Point::x);
                break;
            case OP_VAR_Y:
                STACK_OP
                *stackPtr = get_var_with_idx(*stackPtr, new_points, &Point::y);
                break;
            case OP_VAR_S:
                STACK_OP
                *stackPtr = get_var_with_idx(*stackPtr, new_points,
                                             &Point::sigma);
                break;
            case OP_VAR_A:
                STACK_OP
                *stackPtr = bool(iround(get_var_with_idx(*stackPtr, new_points,
                                                         &Point::is_active)));
                break;

            //assignment-operators
            case OP_ASSIGN_X:
                STACK_OP new_points[n].x = *stackPtr;
                STACK_OFFSET_CHANGE(-1)
                break;
            case OP_ASSIGN_Y:
                STACK_OP new_points[n].y = *stackPtr;
                STACK_OFFSET_CHANGE(-1)
                break;
            case OP_ASSIGN_S:
                STACK_OP new_points[n].sigma = *stackPtr;
                STACK_OFFSET_CHANGE(-1)
                break;
            case OP_ASSIGN_A:
                STACK_OP new_points[n].is_active = is_neq(*stackPtr, 0.);
                STACK_OFFSET_CHANGE(-1)
                break;

            // logical; can skip part of VM code !
            case OP_NOT:
                STACK_OP *stackPtr = is_eq(*stackPtr, 0.);
                break;
//---8<--- END OF DT COMMON BLOCK --------------------------------------
            case OP_AND:
                if (is_neq(*stackPtr, 0))    //return second
                    stackPtr--;
                else              // return first
                    i = skip_code(i, OP_AND, OP_AFTER_AND);
                break;

            case OP_OR:
                if (is_neq(*stackPtr, 0))    //return first
                    i = skip_code(i, OP_OR, OP_AFTER_OR);
                else              // return second
                    stackPtr--;
                break;

            case OP_TERNARY:
                if (! *stackPtr)
                    i = skip_code(i, OP_TERNARY, OP_TERNARY_MID);
                stackPtr--;
                break;
            case OP_TERNARY_MID:
                //if we are here, condition was true. Skip.
                i = skip_code(i, OP_TERNARY_MID, OP_AFTER_TERNARY);
                break;

            case OP_AFTER_AND: //do nothing
            case OP_AFTER_OR:
            case OP_AFTER_TERNARY:
                break;

            //transformation condition
            case OP_INDEX:
                assert(once);  //x[n]=
                n = iround(*stackPtr);//changing n(!!) for use in OP_ASSIGN_
                stackPtr--;
                if (n < 0)
                    n += M;
                if (n < 0 || n >= M)
                    skip_to_end(i);
                break;
            case OP_BEGIN:
              {
                bool next_op_once = (*(i+1) == OP_DO_ONCE);
                if (next_op_once != once) {
                    skip_to_end(i);
                    if (once)
                        return_value=true;
                }
                break;
              }
            case OP_END: //nothing -- it is used only for skipping assignment
                break;
            // once - operators
            case OP_DO_ONCE: //do nothing, it is only a flag
                assert(once);
                break;
            case OP_RESIZE:
                assert(once);
                M = iround(*stackPtr);
                stackPtr--;
                new_points.resize(M);
                break;
            default:
                DT_DEBUG("Unknown operator in VM code: " + S(*i))
                assert(0);
        }
    }
    //assert(stackPtr == stack.begin() - 1 //DataTransformGrammar
    //        || (stackPtr == stack.begin() && once)); //DataExpressionGrammar
    return return_value;
}


/// change  AGSUM X ... X END_AGGREGATE  to  NUMBER INDEX
void replace_aggregates(int M, vector<Point> const& old_points,
                        vector<int>& code, vector<int>::iterator cb)
{
    vector<fp> stack(stack_size);
    for (vector<int>::iterator i = cb; i != code.end(); ++i) {
        if (*i == OP_AGMIN || *i == OP_AGMAX || *i == OP_AGSUM
                || *i == OP_AGAREA || *i == OP_AGAVG || *i == OP_AGSTDDEV) {
            int op = *i;
            vector<int>::iterator const start = i;
            DT_DEBUG("code before replace: " + dt_ops(code));
            replace_aggregates(M, old_points, code, start+1);
            fp result = 0.;
            fp mean = 0.; //needed for OP_AGSTDDEV
            int counter = 0;
            vector<Point> dummy_points(M);
            ++i;
            while (*i != OP_AGCONDITION && *i != OP_END_AGGREGATE)
                ++i;
            vector<int> acode(start+1, i); //code for aggragate function
            vector<int> ccode; //code for condition, empty if no condition
            if (*i == OP_AGCONDITION) {
                vector<int>::iterator const start_cond = i;
                while (*i != OP_END_AGGREGATE)
                    ++i;
                ccode = vector<int>(start_cond+1, i);
            }
            for (int n = 0; n != M; n++) {
                if (!ccode.empty()) {
                    execute_code(n,M,stack, old_points, dummy_points, ccode);
                    if (is_eq(stack.front(), 0))
                        continue;
                }
                ++counter;
                execute_code(n, M, stack, old_points, dummy_points, acode);
                if (op == OP_AGSUM)
                    result += stack.front();
                else if (op == OP_AGMIN) {
                    if (counter == 1)
                        result = stack.front();
                    else if (result > stack.front())
                        result = stack.front();
                }
                else if (op == OP_AGMAX) {
                    if (counter == 1)
                        result = stack.front();
                    else if (result < stack.front())
                        result = stack.front();
                }
                else if (op == OP_AGAREA) {
                    fp dx = (old_points[min(n+1, M-1)].x
                             - old_points[max(n-1, 0)].x) / 2.;
                    result += stack.front() * dx;
                }
                else if (op == OP_AGAVG) {
                    result += (stack.front() - result) / counter;
                }
                else if (op == OP_AGSTDDEV) {
                    // see: http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
                    fp x = stack.front();
                    fp delta = x - mean;
                    mean += delta / counter;
                    result += delta * (x - mean);
                }
                else
                    assert(0);
                DT_DEBUG("n=" + S(n) + " stack.front() = " + S(stack.front()));
            }
            if (op == OP_AGSTDDEV)
                result = sqrt(result / (counter - 1));
            *start = OP_NUMBER;
            *(start+1) = size(numbers);
            numbers.push_back(result);
            code.erase(start+2, i+1);
            i = start+1;
            DT_DEBUG("code after replace: " + dt_ops(code));
        }
    }
}

//-------------------------------------------------------------------------


void execute_vm_code(const vector<Point> &old_points, vector<Point> &new_points)
{
    vector<fp> stack(stack_size);
    int M = (int) new_points.size();
    replace_aggregates(M, old_points, code, code.begin());
    // first execute one-time operations: e.g. x[15]=3
    // n==M => one-time op.
    bool t = execute_code(M, M, stack, old_points, new_points, code);
    if (!t)
        return;
    for (int n = 0; n != M; n++) {
        execute_code(n, M, stack, old_points, new_points, code);
    }
}

} //namespace


string get_trans_repr(string const& s)
{
    bool r = compile_data_transformation(s);
    if (!r)
        r = compile_data_expression(s);
    if (!r)
        return "ERROR";
    return get_code_as_text(code, numbers);
}

bool compile_data_transformation(string const& str)
{
    clear_parse_vecs();
    parse_info<> result = parse(str.c_str(), DataTransformG >> end_p, space_p);
    return (bool) result.full;
}

bool compile_data_expression(string const& str)
{
    clear_parse_vecs();
    parse_info<> result = parse(str.c_str(), DataExpressionG >> end_p, space_p);
    return (bool) result.full;
}

vector<Point> transform_data(string const& str, vector<Point> const& old_points)
{
    bool r = compile_data_transformation(str);
    if (!r)
        throw ExecuteError("Syntax error in data transformation formula.");

    // and then execute compiled code.
    vector<Point> new_points = old_points;
    execute_vm_code(old_points, new_points);
    return new_points;
}

bool is_data_dependent_code(vector<int> const& code)
{
    for (vector<int>::const_iterator i = code.begin(); i != code.end(); ++i)
        if ((*i >= OP_VAR_FIRST_OP && *i <= OP_VAR_LAST_OP)
                || *i == OP_END_AGGREGATE)
            return true;
    return false;
}

bool is_data_dependent_expression(string const& s)
{
    if (!compile_data_expression(s)) //it fills `code'
        return false;
    return is_data_dependent_code(code);
}

fp get_transform_expression_value(string const &s, Data const* data)
{
    bool r = compile_data_expression(s);
    if (!r)
        throw ExecuteError("Syntax error in expression: " + s);

    if (!data && is_data_dependent_code(code))
        throw ExecuteError("Dataset is not specified and the expression "
                           "depends on it: " + s);
    vector<Point> const no_points;
    return get_transform_expr_value(code, data ? data->points() : no_points);
}

fp get_transform_expr_value(vector<int>& code_, vector<Point> const& points)
{
    static vector<fp> stack(stack_size);
    int M = (int) points.size();
    vector<Point> new_points = points;
    replace_aggregates(M, points, code_, code_.begin());
    // n==M => one-time op.
    bool t = execute_code(M, M, stack, points, new_points, code_);
    if (t)
        throw ExecuteError("Expression depends on undefined `n' index.");
    return stack.front();
}

bool get_dt_code(string const& s, vector<int>& code_, vector<fp>& numbers_)
{
    bool r = compile_data_expression(s);
    if (!r)
        return false;

    for (vector<int>::iterator i = code.begin(); i != code.end(); ++i)
        if (*i == OP_AGMIN || *i == OP_AGMAX || *i == OP_AGSUM ||
                *i == OP_AGAREA || *i == OP_AGAVG || *i == OP_AGSTDDEV)
            return false;
    code_ = code;
    numbers_ = numbers;
    return true;
}

fp get_value_for_point(vector<int> const& code_, vector<fp> const& numbers_,
                       fp x, fp y)
{
    static vector<fp> stack(stack_size);
    static vector<Point> points(1);
    static vector<Point> new_points(1);
    numbers = numbers_;
    points[0] = new_points[0] = Point(x, y);
    int M = 1;
    execute_code(0, M, stack, points, new_points, code_);
    return stack.front();
}

vector<fp> get_all_point_expressions(string const &s, Data const* data,
                                     bool only_active)
{
    vector<fp> values;
    vector<Point> const& points = data->points();

    bool r = compile_data_expression(s);
    if (!r)
        throw ExecuteError("Syntax error in expression: " + s);

    int M = (int) points.size();
    vector<Point> new_points = points;
    vector<fp> stack(stack_size);
    replace_aggregates(M, points, code, code.begin());
    for (int i = 0; i < M; ++i) {
        if (only_active && !points[i].is_active)
            continue;
        execute_code(i, M, stack, points, new_points, code);
        values.push_back(stack.front());
    }
    return values;
}


