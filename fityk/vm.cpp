// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
/// virtual machine - calculates expressions using by executing bytecode

#define BUILDING_LIBFITYK
#include "vm.h"

#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/special_functions/digamma.hpp>

#include "common.h"
#include "voigt.h"
#include "numfuncs.h"
#include "logic.h"

#include "func.h" // %f(...)
#include "model.h" // F(...)

using namespace std;

namespace {
using namespace fityk;

vector<int>::const_iterator
skip_code(vector<int>::const_iterator i, int start_op, int finish_op)
{
    //TODO: it doesn't work correctly now, because both op's and indices
    // are positive and mixed
    int counter = 1;
    while (counter) {
        ++i;
        if (*i == finish_op)
            counter--;
        else if (*i == start_op)
            counter++;
        else if (VMData::has_idx(*i))
            ++i;
    }
    return i;
}

template<typename T>
realt get_var_with_idx(realt idx, vector<Point> const& points, T Point::*t)
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
        realt fra = idx - flo;
        return (1-fra) * realt(points[flo].*t)
               + fra * realt(points[flo+1].*t);
    }
}

/// returns floating-point "index" of x in sorted vec of points
realt find_idx_in_sorted(vector<Point> const& pp, realt x)
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

} // anonymous namespace

namespace fityk {

/// debuging utility
#define OP_(x) \
    case OP_##x: return #x;

string op2str(int op)
{
    switch (static_cast<Op>(op)) {
        OP_(NUMBER) OP_(SYMBOL)
        OP_(X) OP_(PUT_DERIV)
        OP_(NEG)   OP_(EXP)  OP_(ERFC)  OP_(ERF)
        OP_(SIN)   OP_(COS)  OP_(TAN)  OP_(SINH) OP_(COSH)  OP_(TANH)
        OP_(ABS)  OP_(ROUND)
        OP_(ATAN) OP_(ASIN) OP_(ACOS)
        OP_(LOG10) OP_(LN)  OP_(SQRT)  OP_(POW)
        OP_(GAMMA) OP_(LGAMMA) OP_(DIGAMMA)
        OP_(VOIGT) OP_(DVOIGT_DX) OP_(DVOIGT_DY)
        OP_(XINDEX)
        OP_(ADD)   OP_(SUB)   OP_(MUL)   OP_(DIV)  OP_(MOD)
        OP_(MIN2)   OP_(MAX2) OP_(RANDNORM) OP_(RANDU)
        OP_(PX) OP_(PY) OP_(PS) OP_(PA)
        OP_(Px) OP_(Py) OP_(Ps) OP_(Pa)
        OP_(Pn) OP_(PM)
        OP_(OR) OP_(AFTER_OR) OP_(AND) OP_(AFTER_AND) OP_(NOT)
        OP_(TERNARY) OP_(TERNARY_MID) OP_(AFTER_TERNARY)
        OP_(GT) OP_(GE) OP_(LT) OP_(LE) OP_(EQ) OP_(NEQ)
        OP_(ASSIGN_X) OP_(ASSIGN_Y) OP_(ASSIGN_S) OP_(ASSIGN_A)
        OP_(FUNC) OP_(SUM_F) OP_(SUM_Z)
        OP_(NUMAREA) OP_(FINDX) OP_(FIND_EXTR)
        OP_(TILDE)
        OP_(DATASET) OP_(DT_SUM_SAME_X) OP_(DT_AVG_SAME_X) OP_(DT_SHIRLEY_BG)
        OP_(OPEN_ROUND)  OP_(OPEN_SQUARE)
    }
    return S(op); // unreachable (if all OPs are listed above)
}
#undef OP_

string vm2str(vector<int> const& code, vector<realt> const& data)
{
    string s;
    for (vector<int>::const_iterator i = code.begin(); i < code.end(); ++i) {
        s += op2str(*i);
        if (*i == OP_NUMBER) {
            ++i;
            assert (*i >= 0 && *i < size(data));
            s += "[" + S(*i) + "](" + S(data[*i]) + ")";
        } else if (VMData::has_idx(*i)) {
            ++i;
            s += "[" + S(*i) + "]";
        }
        s += " ";
    }
    return s;
}


void VMData::append_number(realt d)
{
    append_code(OP_NUMBER);
    int number_pos = numbers_.size();
    append_code(number_pos);
    numbers_.push_back(d);
}

void VMData::replace_symbols(const vector<realt>& vv)
{
    for (vector<int>::iterator op = code_.begin(); op < code_.end(); ++op) {
        if (*op == OP_SYMBOL) {
            *op = OP_NUMBER;
            ++op;
            realt value = vv[*op];
            vector<realt>::iterator x =
                    find(numbers_.begin(), numbers_.end(), value);
            if (x != numbers_.end())
                *op = x - numbers_.begin();
            else {
                *op = numbers_.size();
                numbers_.push_back(value);
            }
        } else if (has_idx(*op))
            ++op;
    }
}

/// switches between non-negative and negative indices (a -> -1-a),
/// the point of having negative indices is to avoid conflicts with opcodes.
/// The same transformation is used in OpTree. 
void VMData::flip_indices()
{
    for (vector<int>::iterator i = code_.begin(); i < code_.end(); ++i)
        if (has_idx(*i)) {
            ++i;
            *i = -1 - *i;
        }
}

bool VMData::has_op(int op) const
{
    for (vector<int>::const_iterator i = code_.begin(); i < code_.end(); ++i) {
        if (*i == op)
            return true;
        if (has_idx(*i))
            ++i;
    }
    return false;
}

inline realt as_bool(realt d) { return fabs(d) < 0.5 ? 0 : 1; }

#define STACK_OFFSET_CHANGE(ch) stackPtr+=(ch)

inline
void run_const_op(const Full* F, const std::vector<realt>& numbers,
                  vector<int>::const_iterator& i,
                  realt*& stackPtr,
                  const int n,
                  const vector<Point>& old_points,
                  const vector<Point>& new_points)
{
    switch (*i) {
        //unary-operators
        case OP_NEG:
            *stackPtr = - *stackPtr;
            break;
        case OP_SQRT:
            *stackPtr = sqrt(*stackPtr);
            break;
        case OP_GAMMA:
            *stackPtr = boost::math::tgamma(*stackPtr);
            break;
        case OP_LGAMMA:
            *stackPtr = boost::math::lgamma(*stackPtr);
            break;
        case OP_DIGAMMA:
            *stackPtr = boost::math::digamma(*stackPtr);
            break;
        case OP_EXP:
            *stackPtr = exp(*stackPtr);
            break;
        case OP_ERFC:
            *stackPtr = erfc(*stackPtr);
            break;
        case OP_ERF:
            *stackPtr = erf(*stackPtr);
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
        case OP_SINH:
            *stackPtr = sinh(*stackPtr);
            break;
        case OP_COSH:
            *stackPtr = cosh(*stackPtr);
            break;
        case OP_TANH:
            *stackPtr = tanh(*stackPtr);
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
        case OP_ABS:
            *stackPtr = fabs(*stackPtr);
            break;
        case OP_ROUND:
            *stackPtr = floor(*stackPtr + 0.5);
            break;

        case OP_XINDEX:
            *stackPtr = find_idx_in_sorted(old_points, *stackPtr);
            break;

#ifndef STANDALONE_DATATRANS
        case OP_FUNC:
            i++;
            *stackPtr = F->mgr.get_function(*i)->calculate_value(*stackPtr);
            break;
        case OP_SUM_F:
            i++;
            *stackPtr = F->dk.get_model(*i)->value(*stackPtr);
            break;
        case OP_SUM_Z:
            i++;
            *stackPtr = F->dk.get_model(*i)->zero_shift(*stackPtr);
            break;
        case OP_NUMAREA:
            i += 2;
            STACK_OFFSET_CHANGE(-2);
            if (*(i-1) == OP_FUNC) {
                *stackPtr = F->mgr.get_function(*i)->numarea(*stackPtr,
                                    *(stackPtr+1), iround(*(stackPtr+2)));
            } else if (*(i-1) == OP_SUM_F) {
                *stackPtr = F->dk.get_model(*i)->numarea(*stackPtr,
                                    *(stackPtr+1), iround(*(stackPtr+2)));
            } else // OP_SUM_Z
                throw ExecuteError("Z.numarea() is not implemented. "
                                   "Does anyone need it?");
            break;

        case OP_FINDX:
            i += 2;
            STACK_OFFSET_CHANGE(-2);
            if (*(i-1) == OP_FUNC) {
                *stackPtr = F->mgr.get_function(*i)->find_x_with_value(
                                  *stackPtr, *(stackPtr+1), *(stackPtr+2));
            } else if (*(i-1) == OP_SUM_F) {
                throw ExecuteError("F.findx() is not implemented. "
                                   "Does anyone need it?");
            } else // OP_SUM_Z
                throw ExecuteError("Z.findx() is not implemented. "
                                   "Does anyone need it?");
            break;

        case OP_FIND_EXTR:
            i += 2;
            STACK_OFFSET_CHANGE(-1);
            if (*(i-1) == OP_FUNC) {
                *stackPtr = F->mgr.get_function(*i)->find_extremum(*stackPtr,
                                                            *(stackPtr+1));
            } else if (*(i-1) == OP_SUM_F) {
                throw ExecuteError("F.extremum() is not implemented. "
                                   "Does anyone need it?");
            } else // OP_SUM_Z
                throw ExecuteError("Z.extremum() is not implemented. "
                                   "Does anyone need it?");
            break;
#endif //not STANDALONE_DATATRANS

        //binary-operators
        case OP_MIN2:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = min(*stackPtr, *(stackPtr+1));
            break;
        case OP_MAX2:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = max(*stackPtr, *(stackPtr+1));
            break;
        case OP_RANDU:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = rand_uniform(*stackPtr, *(stackPtr+1));
            break;
        case OP_RANDNORM:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr += rand_gauss() * *(stackPtr+1);
            break;
        case OP_ADD:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr += *(stackPtr+1);
            break;
        case OP_SUB:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr -= *(stackPtr+1);
            break;
        case OP_MUL:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr *= *(stackPtr+1);
            break;
        case OP_DIV:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr /= *(stackPtr+1);
            break;
        case OP_MOD:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr -= floor(*stackPtr / *(stackPtr+1)) * *(stackPtr+1);
            break;
        case OP_POW:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = pow(*stackPtr, *(stackPtr+1));
            break;
        case OP_VOIGT:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = humlik(*stackPtr, *(stackPtr+1)) / sqrt(M_PI);
            break;
        case OP_DVOIGT_DX:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = humdev_dkdx(*stackPtr, *(stackPtr+1)) / sqrt(M_PI);
            break;
        case OP_DVOIGT_DY:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = humdev_dkdy(*stackPtr, *(stackPtr+1)) / sqrt(M_PI);
            break;

        // comparisions
        case OP_LT:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = is_lt(*stackPtr, *(stackPtr+1));
            break;
        case OP_GT:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = is_gt(*stackPtr, *(stackPtr+1));
            break;
        case OP_LE:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = is_le(*stackPtr, *(stackPtr+1));
            break;
        case OP_GE:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = is_ge(*stackPtr, *(stackPtr+1));
            break;
        case OP_EQ:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = is_eq(*stackPtr, *(stackPtr+1));
            break;
        case OP_NEQ:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = is_neq(*stackPtr, *(stackPtr+1));
            break;

        // putting-number-to-stack-operators
        case OP_NUMBER:
            STACK_OFFSET_CHANGE(+1);
            i++;
            *stackPtr = numbers[*i];
            break;

        case OP_Pn:
            STACK_OFFSET_CHANGE(+1);
            *stackPtr = static_cast<realt>(n);
            break;
        case OP_PM:
            STACK_OFFSET_CHANGE(+1);
            *stackPtr = static_cast<realt>(old_points.size());
            break;

        case OP_Px:
            *stackPtr = get_var_with_idx(*stackPtr, old_points, &Point::x);
            break;
        case OP_Py:
            *stackPtr = get_var_with_idx(*stackPtr, old_points, &Point::y);
            break;
        case OP_Ps:
            *stackPtr = get_var_with_idx(*stackPtr, old_points,
                                         &Point::sigma);
            break;
        case OP_Pa:
            *stackPtr = as_bool(get_var_with_idx(*stackPtr, old_points,
                                                 &Point::is_active));
            break;
        case OP_PX:
            *stackPtr = get_var_with_idx(*stackPtr, new_points, &Point::x);
            break;
        case OP_PY:
            *stackPtr = get_var_with_idx(*stackPtr, new_points, &Point::y);
            break;
        case OP_PS:
            *stackPtr = get_var_with_idx(*stackPtr, new_points,
                                         &Point::sigma);
            break;
        case OP_PA:
            *stackPtr = as_bool(get_var_with_idx(*stackPtr, new_points,
                                                 &Point::is_active));
            break;

        // logical; can skip part of VM code !
        case OP_NOT:
            *stackPtr = is_eq(*stackPtr, 0.);
            break;
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
        case OP_TILDE:
            break;

        case OP_DATASET:
            throw ExecuteError("@n can not be used in this context");
            //break; // unreachable

        default:
            //cerr << "Unknown operator in VM code: " << *i << endl;
            assert(0);
    }
}

inline
void run_mutab_op(const Full* F, const std::vector<realt>& numbers,
                  vector<int>::const_iterator& i,
                  realt*& stackPtr,
                  const int n,
                  const vector<Point>& old_points,
                  vector<Point>& new_points)
{
    //cerr << "op " << dt_op(*i) << endl;
    switch (*i) {
        //assignment-operators
        case OP_ASSIGN_X:
            new_points[n].x = *stackPtr;
            STACK_OFFSET_CHANGE(-1);
            break;
        case OP_ASSIGN_Y:
            new_points[n].y = *stackPtr;
            STACK_OFFSET_CHANGE(-1);
            break;
        case OP_ASSIGN_S:
            new_points[n].sigma = *stackPtr;
            STACK_OFFSET_CHANGE(-1);
            break;
        case OP_ASSIGN_A:
            new_points[n].is_active = is_neq(*stackPtr, 0.);
            STACK_OFFSET_CHANGE(-1);
            break;
        default:
            run_const_op(F, numbers, i, stackPtr, n, old_points, new_points);
    }
}

inline
void run_func_op(const vector<realt>& numbers, vector<int>::const_iterator &i,
                 realt*& stackPtr)
{
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
        case OP_ERFC:
            *stackPtr = erfc(*stackPtr);
            break;
        case OP_ERF:
            *stackPtr = erf(*stackPtr);
            break;
        case OP_LOG10:
            *stackPtr = log10(*stackPtr);
            break;
        case OP_LN:
            *stackPtr = log(*stackPtr);
            break;
        case OP_SINH:
            *stackPtr = sinh(*stackPtr);
            break;
        case OP_COSH:
            *stackPtr = cosh(*stackPtr);
            break;
        case OP_TANH:
            *stackPtr = tanh(*stackPtr);
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
        case OP_LGAMMA:
            *stackPtr = boost::math::lgamma(*stackPtr);
            break;
        case OP_DIGAMMA:
            *stackPtr = boost::math::digamma(*stackPtr);
            break;
        case OP_ABS:
            *stackPtr = fabs(*stackPtr);
            break;

        //binary operators
        case OP_ADD:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr += *(stackPtr+1);
            break;
        case OP_SUB:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr -= *(stackPtr+1);
            break;
        case OP_MUL:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr *= *(stackPtr+1);
            break;
        case OP_DIV:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr /= *(stackPtr+1);
            break;
        case OP_POW:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = pow(*stackPtr, *(stackPtr+1));
            break;
        case OP_VOIGT:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = humlik(*stackPtr, *(stackPtr+1)) / sqrt(M_PI);
            break;
        case OP_DVOIGT_DX:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = humdev_dkdx(*stackPtr, *(stackPtr+1)) / sqrt(M_PI);
            break;
        case OP_DVOIGT_DY:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = humdev_dkdy(*stackPtr, *(stackPtr+1)) / sqrt(M_PI);
            break;

        // putting-number-to-stack-operators
        // stack overflow not checked
        case OP_NUMBER:
            STACK_OFFSET_CHANGE(+1);
            i++; // OP_NUMBER opcode is always followed by index
            *stackPtr = numbers[*i];
            break;

        case OP_TILDE:
            // used for default values in Runner::make_func_from_template()
            break;

        default:
            throw ExecuteError("op " + op2str(*i) +
                               " is not allowed for variables and functions");
    }
}


void ExprCalculator::transform_data(vector<Point>& points)
{
    if (points.empty())
        return;

    realt stack[16];
    realt* stackPtr = stack - 1; // will be ++'ed first
    vector<Point> new_points = points;

    // do the time-consuming overflow checking only for the first point
    v_foreach (int, i, vm_.code()) {
        run_mutab_op(F_, vm_.numbers(), i, stackPtr, 0, points, new_points);
        if (stackPtr - stack >= 16)
            throw ExecuteError("stack overflow");
    }
    assert(stackPtr == stack - 1); // ASSIGN_ op must be at the end

    // the same for the rest of points, but without checks
    for (int n = 1; n != size(points); ++n)
        v_foreach (int, i, vm_.code())
            run_mutab_op(F_, vm_.numbers(), i, stackPtr, n, points, new_points);

    points.swap(new_points);
}

realt ExprCalculator::calculate(int n, const vector<Point>& points) const
{
    realt stack[16];
    realt* stackPtr = stack - 1; // will be ++'ed first
    v_foreach (int, i, vm_.code()) {
        run_const_op(F_, vm_.numbers(), i, stackPtr, n, points, points);
        if (stackPtr - stack >= 16)
            throw ExecuteError("stack overflow");
    }
    //cerr << "stackPtr: " << stackPtr - stack << endl;
    assert(stackPtr == stack); // no ASSIGN_ at the end
    return stack[0];
}

realt ExprCalculator::calculate_custom(const vector<realt>& custom_val) const
{
    realt stack[16];
    realt* stackPtr = stack - 1; // will be ++'ed first
    const vector<Point> dummy;
    v_foreach (int, i, vm_.code()) {
        if (*i == OP_SYMBOL) {
            STACK_OFFSET_CHANGE(+1);
            ++i;
            if (is_index(*i, custom_val))
                *stackPtr = custom_val[*i];
            else
                throw ExecuteError("[internal] variable mismatch");
        } else
            run_const_op(F_, vm_.numbers(), i, stackPtr, 0, dummy, dummy);
        if (stackPtr - stack >= 16)
            throw ExecuteError("stack overflow");
    }
    assert(stackPtr == stack); // no ASSIGN_ at the end
    return stack[0];
}

/// executes VM code, sets derivatives and returns value
realt run_code_for_variable(const VMData& vm,
                            const vector<Variable*> &variables,
                            vector<realt> &derivatives)
{
    realt stack[16];
    realt* stackPtr = stack - 1; // will be ++'ed first
    v_foreach (int, i, vm.code()) {
        if (*i == OP_SYMBOL) {
            STACK_OFFSET_CHANGE(+1);
            ++i; // skip the next one
            *stackPtr = variables[*i]->value();
        } else if (*i == OP_PUT_DERIV) {
            ++i;
            // the OP_PUT_DERIV opcode is followed by a number n,
            // the derivative is calculated with respect to n'th variable
            assert(*i < (int) derivatives.size());
            derivatives[*i] = *stackPtr;
            STACK_OFFSET_CHANGE(-1);
        } else
            run_func_op(vm.numbers(), i, stackPtr);
    }
    assert(stackPtr == stack);
    return stack[0];
}


realt run_code_for_custom_func(const VMData& vm, realt x,
                               vector<realt> &derivatives)
{
    realt stack[16];
    realt* stackPtr = stack - 1; // will be ++'ed first
    v_foreach (int, i, vm.code()) {
        if (*i == OP_X) {
            STACK_OFFSET_CHANGE(+1);
            *stackPtr = x;
        } else if (*i == OP_PUT_DERIV) {
            ++i;
            // the OP_PUT_DERIV opcode is followed by a number n,
            // the derivative is calculated with respect to n'th variable
            assert(*i < (int) derivatives.size());
            derivatives[*i] = *stackPtr;
            STACK_OFFSET_CHANGE(-1);
        } else
            run_func_op(vm.numbers(), i, stackPtr);
    }
    assert(stackPtr == stack);
    return stack[0];
}

realt run_code_for_custom_func_value(const VMData& vm, realt x,
                                     int code_offset)
{
    realt stack[16];
    realt* stackPtr = stack - 1; // will be ++'ed first
    for (vector<int>::const_iterator i = vm.code().begin() + code_offset;
                                                 i != vm.code().end(); ++i) {
        if (*i == OP_X) {
            STACK_OFFSET_CHANGE(+1);
            *stackPtr = x;
        } else
            run_func_op(vm.numbers(), i, stackPtr);
    }
    assert(stackPtr == stack);
    return stack[0];
}

} // namespace fityk
