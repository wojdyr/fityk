// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

// the idea of VM is based on one of boost::spirit samples - vmachine_calc

// this file can be compiled to stand-alone test program:
// $ g++ -I../3rdparty -DSTANDALONE_DATATRANS datatrans.cpp numfuncs.cpp -o dt
// $ ./dt 

// right hand variables: 
//       arrays (index in square brackets can be ommitted and n is assumed):
//              x[] - x coordinate of point before transformation (old x)  
//              X[] - x coordinate after transformation (new x),
//              y[], Y[] - y coordinate of point (before/after transformation)
//              s[], S[]  -  std. dev. of y, 
//              a[], A[] - active point, either 0 and 1
//       scalars:
//              M - size of arrays, 
//              n - current index (index of currently transformed point)
// upper-case variables are assignable 
// assignment syntax: 
//        or: assignable_var = expression     --> executed for all points
//        or: assignable_var[k] = expression  --> executed for k-th point
//        or: assignable_var[k...m] = expression  
//        or: assignable_var[k...] = expression  
//        or: assignable_var[...k] = expression  
//   where k and m are integers (negative are counted from the end:
//                               -1 is last, -2 one before last, and so on)
//                               FIXME: should x[-1] also mean x[M-1]?
//     k...m means that expression is executed for k, k+1, ... m-1. Not for m.
//
// Assignments are executed for n = 0, ..., M-1 (if assignments are joined 
// with '&', all are executed for n=0, then all for n=1, and so on).
// Before transformation the new point coordinates are a copy of the old ones.
//
// There is a special function sum(), which can be used as a real value. 
// and returns a sum over all points of value of expression 
// eg. sum(n > 0 ? (x[n] - x[n-1]) * (y[n-1] + y[n])/2 : 0) -> area.
// The value of sum() is computed before transformation.
//
//
// There are statements, that are executed only once: 
//    M=..., 
//    order=...
// and assignments T[k]=..., if these statements are joined with other 
// assignments using '&', they are executed first, before iteration over all
// indices.
//
//        M=length (eg. M=M+5 or M=100) changes number of points: 
//        either appends new data points with x=y=sigma=0, is_active=false,
//        or deletes last points.
//        order=[-]coordinate (eg. order=x, order=-x) 
//        sort data points; sorting is stable. After finishing transform 
//        command, points are sorted using x.
//        delete[k] (or delete[k...m])
//        delete(condition)
//           deletion and change of M is postponed to the end of computation
// 

// operators: binary +, -, *, /, % , ^  
// ternary operator:       condition ? expression1 : expression2 
// 
//functions: sqrt exp log10 ln sin cos tan atan asin acos abs min max round
// boolean: AND, OR, >, >=, <, <=, = (or ==), != (or <>), TRUE, FALSE, NOT
//
//parametrized functions: spline, interpolate
// The general syntax is: pfunc[param1 param2 ...](expression), 
//  eg. spline[22.1 37.9 48.1 17.2 93.0 20.7](x)
// spline - cubic spline interpolation, parameters are x1, y1, x2, y2, ...
// interpolate - polyline interpolation, parameters are x1, y1, x2, y2, ...
//
// All computations are performed using real numbers, but using round for
// comparisions should not be neccessary. Two numbers that differ less
// than epsilon=1e-9 ie. abs(a-b)<epsilon, are considered equal. 
// Indices are also computed in real number domain, and then rounded to nearest 
// integer.
//     
//
// Syntax examples:
//    set standard deviation as sqrt(max(1,y))
//                         s = sqrt(max(1,y))
//         or (the same):  s = sqrt(max(1,y[n]))
//                    or:  S = sqrt(max(1,y[n]))
//                    or:  S = sqrt(max(1,Y[n]))
//                    or:  S = sqrt(max(1,Y))
//
//    integration:   Y[1...] = Y[n-1] + y[n]  
//
//    swaping x and y axes: y=x & x=y & s=sqrt(Y) 
//                      or: y=x & x=y & s=sqrt(x)
//
//    smoothing: Y[1...-1] = (y[n-1] + y[n+1])/2  
//
//    reducing: order = x, # not neccessary, points are always in this order
//              x[...-1] = (x[n]+x[n+1])/2, 
//              y[...-1] = y[n]+y[n+1],
//              delete(n%2==1)    
//
//    delete inactive points:    delete(not a)  
//    normalize area: 
//           y = y / sum(n > 0 ? (x[n] - x[n-1]) * (y[n-1] + y[n])/2 : 0) 
//
//    substract spline baseline given by points (22.17, 37.92), (48.06, 17.23),
//                                              (93.03, 20.68).
//    y = y - spline[22.17 37.92  48.06 17.23  93.03 20.68](x) 
//

//-----------------------------------------------------------------
// how it works:
//   * parse string and prepare VM code (list of operators) 
//     and data (list of numbers). Actually there are two VM codes and
//     data arrays - one for do-once operations, and second 
//     for for-many-points operations. 
//  
//   * execute do-once operations (eg. order=x, x[3]=4) 
//     and computes value of sum()
//
//   * execute all assignments for every point (from first to last),
//     unless the point is outside of specified range.
//


//#define BOOST_SPIRIT_DEBUG


#include "datatrans.h"
#include "common.h"
#include "data.h"
#include "numfuncs.h"
#include <boost/spirit/core.hpp>

using namespace std;
using namespace boost::spirit;

const double epsilon = 1e-9;



#ifdef STANDALONE_DATATRANS

#include <iostream>  
bool dt_verbose = false;
#define DT_DEBUG(x) if (dt_verbose) std::cout << (x) << std::endl;
#define DT_DEBUG_N(x) if (dt_verbose) std::cout << (x); 
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
        vector<Point> points, transformed_points;

        points.push_back(Point(0.1, 4, 1));
        points.push_back(Point(0.2, 2, 1));
        points.push_back(Point(0.3, -2, 1));
        points.push_back(Point(0.4, 3, 1));
        points.push_back(Point(0.5, 3, 1));

        bool r = transform_data(str, points, transformed_points);
        int len = (int) points.size();
        if (r)
            for (int n = 0; n != (int) transformed_points.size(); n++) 
                cout << "point " << n << ": " 
                    << (n < len ? points[n] : Point()).str()
                    << " -> " << transformed_points[n].str() << endl;
        else
            cout << "Syntax error." << endl;
        cout << "==> ";
    }
    return 0;
}

#else 

#    define DT_DEBUG(x) ;
#    define DT_DEBUG_N(x) ;

#endif //STANDALONE_DATATRANS


bool x_lt(const Point &p1, const Point &p2) { return p1.x < p2.x; }
bool x_gt(const Point &p1, const Point &p2) { return p1.x > p2.x; }
bool y_lt(const Point &p1, const Point &p2) { return p1.y < p2.y; }
bool y_gt(const Point &p1, const Point &p2) { return p1.y > p2.y; }
bool sigma_lt(const Point &p1, const Point &p2) { return p1.sigma < p2.sigma; }
bool sigma_gt(const Point &p1, const Point &p2) { return p1.sigma > p2.sigma; }
bool active_lt(const Point &p1, const Point &p2) 
                                        { return p1.is_active < p2.is_active; }
bool active_gt(const Point &p1, const Point &p2) 
                                        { return p1.is_active > p2.is_active; }


//----------------------  Parameterized Functions -------------------------
class ParameterizedFunction
{
public:
    ParameterizedFunction() {}
    virtual ~ParameterizedFunction() {}
    virtual fp calculate(fp x) = 0;
};


class InterpolateFunction : public ParameterizedFunction
{
public:
    InterpolateFunction(vector<fp> &params) {
        for (int i=0; i < size(params) - 1; i+=2)
            bb.push_back(B_point(params[i], params[i+1]));
    }

    fp calculate(fp x) { return get_linear_interpolation(bb, x); }

private:
    std::vector<B_point> bb;
};


class SplineFunction : public ParameterizedFunction
{
public:
    SplineFunction(vector<fp> &params) {
        for (int i=0; i < size(params) - 1; i+=2)
            bb.push_back(B_point(params[i], params[i+1]));
        prepare_spline_interpolation(bb);
    }

    fp calculate(fp x) { return get_spline_interpolation(bb, x); }

private:
    std::vector<B_point> bb;
};


//------------------------  Virtual Machine  --------------------------------

static vector<int> code;        //  VM code 
static vector<double> numbers;  //  VM data 
static vector<ParameterizedFunction*> parameterized; // also used by VM 

// code vector contains not only operators, but also indices that
// points locations in numbers or parameterized vectors
bool is_operator(vector<int>::iterator i, DataTransformVMOperator op)
{
    assert(code.begin() <= i && i < code.end());
    return (*i == op && (i != code.begin() 
                        || *(i-1) != OP_NUMBER && *(i-1) != OP_PARAMETERIZED));
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


// Return value: 
//  if once==true (one-time pass).
//   true means: it is neccessary to call this function for every point.
//  if iterative pass:
//   true means: delete this point.
// n: index of point
// M: number of all points (==new_points.size())
bool execute_code(int n, int &M, vector<double>& stack,
                  vector<Point> const& old_points, vector<Point>& new_points,
                  vector<int> const& code)  
{
    DT_DEBUG("executing code; n=" + S(n) + " M=" + S(M))
    assert(M == size(new_points));
    bool once = (n == M);
    bool return_value=false; 
    vector<double>::iterator stackPtr = stack.begin() - 1;//will be ++'ed first
    for (vector<int>::const_iterator i=code.begin(); i != code.end(); i++) {
        DT_DEBUG("NOW op " + S(*i))
        switch (*i) {
            //unary-operators
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
            case OP_ABS:
                *stackPtr = fabs(*stackPtr);
                break;
            case OP_ROUND:
                *stackPtr = floor(*stackPtr + 0.5);
                break;

            case OP_PARAMETERIZED:
                i++;
                *stackPtr = parameterized[*i]->calculate(*stackPtr);
                break;

            //binary-operators
            case OP_MIN:
                stackPtr--;
                *stackPtr = min(*stackPtr, *(stackPtr+1));
                break;
            case OP_MAX:
                stackPtr--;
                *stackPtr = max(*stackPtr, *(stackPtr+1));
                break;
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
            case OP_MOD:
                stackPtr--;
                *stackPtr -= floor(*stackPtr / *(stackPtr+1)) * *(stackPtr+1);
                break;
            case OP_POW:
                stackPtr--;
                *stackPtr = pow(*stackPtr, *(stackPtr+1));
                break;

            // putting-number-to-stack-operators
            // stack overflow not checked
            case OP_NUMBER:
                stackPtr++;
                i++;
                *stackPtr = numbers[*i];
                break;
            case OP_VAR_n:
                stackPtr++;
                *stackPtr = static_cast<double>(n);
                break;
            case OP_VAR_M:
                stackPtr++;
                *stackPtr = static_cast<double>(new_points.size());
                break;
            case OP_VAR_x:
                {
                int t = iround(*stackPtr);
                *stackPtr = t >= 0 && t < M ? old_points[t].x : 0.;
                break;
                }
            case OP_VAR_y:
                {
                int t = iround(*stackPtr);
                *stackPtr = t >= 0 && t < M ? old_points[t].y : 0.;
                break;
                }
            case OP_VAR_s:
                {
                int t = iround(*stackPtr);
                *stackPtr = t >= 0 && t < M ? old_points[t].sigma : 0.;
                break;
                }
            case OP_VAR_a:
                {
                int t = iround(*stackPtr);
                *stackPtr = t >= 0 && t < M && old_points[t].is_active ? 1 : 0.;
                break;
                }
            case OP_VAR_X:
                {
                int t = iround(*stackPtr);
                *stackPtr = t >= 0 && t < M ? new_points[t].x : 0.;
                break;
                }
            case OP_VAR_Y:
                {
                int t = iround(*stackPtr);
                *stackPtr = t >= 0 && t < M ? new_points[t].y : 0.;
                break;
                }
            case OP_VAR_S:
                {
                int t = iround(*stackPtr);
                *stackPtr = t >= 0 && t < M ? new_points[t].sigma : 0.;
                break;
                }
            case OP_VAR_A:
                {
                int t = iround(*stackPtr);
                *stackPtr = t >= 0 && t < M && new_points[t].is_active ? 1 : 0.;
                break;
                }

            //assignment-operators
            case OP_ASSIGN_X:
                new_points[n].x = *stackPtr;
                stackPtr--; 
                break;
            case OP_ASSIGN_Y:
                new_points[n].y = *stackPtr;
                stackPtr--; 
                break;
            case OP_ASSIGN_S:
                new_points[n].sigma = *stackPtr;
                stackPtr--; 
                break;
            case OP_ASSIGN_A:
                new_points[n].is_active = abs(*stackPtr) > epsilon;
                stackPtr--; 
                break;

            // logical; can skip part of VM code !
            case OP_AND:
                if (*stackPtr)    //return second
                    stackPtr--; 
                else              // return first
                    i = skip_code(i, OP_AND, OP_AFTER_AND);
                break;

            case OP_OR:
                if (*stackPtr)    //return first
                    i = skip_code(i, OP_OR, OP_AFTER_OR);
                else              // return second
                    stackPtr--; 
                break;

            case OP_NOT:
                *stackPtr = (fabs(*stackPtr) < epsilon);
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

            case OP_DELETE_COND:
                if (*stackPtr)
                    return_value = true;
                stackPtr--;
                break;

            // comparisions
            case OP_LT:
                stackPtr--;
                *stackPtr = (*stackPtr < *(stackPtr+1) - epsilon);
                break;
            case OP_GT:
                stackPtr--;
                *stackPtr = (*stackPtr > *(stackPtr+1) + epsilon);
                break;
            case OP_LE:
                stackPtr--;
                *stackPtr = (*stackPtr <= *(stackPtr+1) + epsilon);
                break;
            case OP_GE:
                stackPtr--;
                *stackPtr = (*stackPtr >= *(stackPtr+1) - epsilon);
                break;
            case OP_EQ:
                stackPtr--;
                *stackPtr = (fabs(*stackPtr - *(stackPtr+1)) < epsilon);
                break;
            case OP_NEQ:
                stackPtr--;
                *stackPtr = (fabs(*stackPtr - *(stackPtr+1)) > epsilon);
                break;

                // next comparision hack, see rbool rule for more...
            case OP_NCMP_HACK:
                stackPtr++;                // put number, that is accidentally 
                *stackPtr = *(stackPtr+1); // in unused part of the stack
                break;
            
            //transformation condition 
            case OP_INDEX:
                assert(once);  //x[n]= or delete[n]
                n = iround(*stackPtr);//changing n(!!) for use in OP_ASSIGN_
                stackPtr--;
                if (n < 0)
                    n += M;
                if (n < 0 || n >= M)
                    skip_to_end(i);
                if (*(i+1) == OP_DELETE) {
                    new_points.erase(new_points.begin() + n);
                    M--;
                    skip_to_end(i);
                }
                break;
            case OP_RANGE: 
              {
                //x[i...j]= or delete[i...j]
                int right = iround(*stackPtr); //Last In First Out
                stackPtr--;                    
                if (right <= 0)
                    right += M;
                int left = iround(*stackPtr);
                stackPtr--;
                if (left < 0)
                    left += M;
                if (*(i+1) == OP_DELETE) {
                    if (0 < left && left < right && right <= M) {
                        new_points.erase(new_points.begin()+left, 
                                         new_points.begin()+right);
                        M = size(new_points);
                    }
                    skip_to_end(i);
                }
                else { 
                    //if n not in [i...j] then skip to prevent OP_ASSIGN_.
                    bool n_between = (left <= n && n < right);
                    if (!n_between) 
                        skip_to_end(i);
                }
                break;
              }
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
            case OP_ORDER:
              {
                assert(once);
                int ord = iround(*stackPtr);
                stackPtr--;
                DT_DEBUG("in OP_ORDER with " + S(ord))
                if (ord == 1) {
                    DT_DEBUG("sort x_lt")
                    stable_sort(new_points.begin(), new_points.end(), x_lt);
                }
                else if(ord == -1) {
                    DT_DEBUG("sort x_gt")
                    stable_sort(new_points.begin(), new_points.end(), x_gt);
                }
                else if(ord == 2) {
                    DT_DEBUG("sort y_lt")
                    stable_sort(new_points.begin(), new_points.end(), y_lt);
                }
                else if(ord == -2) {
                    DT_DEBUG("sort y_gt")
                    stable_sort(new_points.begin(), new_points.end(), y_gt);
                }
                break;
              }
            case OP_DELETE:
                assert(0); //OP_DELETE is processed in OP_INDEX OR OP_RANGE
                break;
            case OP_SUM:
                assert(!once);
                assert(stackPtr == stack.begin());
                return false;
            case OP_IGNORE:
                break;
            default:
                DT_DEBUG("Unknown operator in VM code: " + S(*i))
        }
    }
    assert(stackPtr == stack.begin() - 1);
    return return_value;
}

//change  BEGIN X ... X SUM END  with  NUMBER INDEX IGNORE ... IGNORE END
void replace_sums(int M, vector<double>& stack, vector<Point> const& old_points)
{
    DT_DEBUG_N("code before replace:");
    for (vector<int>::const_iterator i=code.begin(); i != code.end(); i++) 
        DT_DEBUG_N(" " + S(*i));
    DT_DEBUG("")
    bool nested = false;
    bool in_sum = false;
    vector<int>::iterator sum_end;
    for (vector<int>::iterator i = code.end()-1; i != code.begin(); i--) {
        if (is_operator(i, OP_SUM)) {
            if (in_sum)
                nested = true;
            sum_end = i;
            in_sum = true;
        }
        else if (in_sum && is_operator(i, OP_BEGIN)) {
            double sum = 0.;
            vector<Point> fake_new_points(M);
            vector<int> sum_code(i, sum_end+1);
            for (int n = 0; n != M; n++) {
                execute_code(n, M, stack, old_points, fake_new_points,sum_code);
                DT_DEBUG_N("n=" + S(n) + " on stack: " + S(stack.front()));
                sum += stack.front();
                DT_DEBUG(" sum:" + S(sum))
            }
            *i = OP_NUMBER;
            *(i+1) = size(numbers);
            numbers.push_back(sum);
            for (vector<int>::iterator j=i+2; j < sum_end+1; j++) 
                *j = OP_IGNORE; //FIXME: why not code.erase() ?
            in_sum = false;
        }
    }
    //in rare case of nested sum() we need:
    if (nested)
        replace_sums(M, stack, old_points);
    DT_DEBUG_N("code after replace:");
    for (vector<int>::const_iterator i=code.begin(); i != code.end(); i++) 
        DT_DEBUG_N(" " + S(*i));
    DT_DEBUG("")
}

void push_double::operator()(const double& n) const
{
    code.push_back(OP_NUMBER);
    code.push_back(size(numbers));
    numbers.push_back(n);
}

void push_op::push() const 
{ 
    code.push_back(op); 
    if (op2)
        code.push_back(op2); 
}

void parameterized_op::push() const
{ 
    DT_DEBUG("PARAMETERIZED " + S(op))  
    typedef vector<int>::iterator viit;
    viit first = find(code.begin(), code.end(), OP_PLIST_BEGIN);
    viit last = find(code.begin(), code.end(), OP_PLIST_END) + 1;
    vector<fp> params;
    for (viit i=first; i != last; ++i) 
        if (*i == OP_NUMBER) 
            params.push_back(numbers[*++i]);
    code.erase(first, last);
    code.push_back(OP_PARAMETERIZED); 
    code.push_back(size(parameterized));
    ParameterizedFunction *func = 0;
    switch (op) {
        case PF_INTERPOLATE:
            func = new InterpolateFunction(params);
            break;
        case PF_SPLINE:
            func = new SplineFunction(params);
            break;
        default:
            assert(0);
    }
    parameterized.push_back(func);
}


//-------------------------------------------------------------------------


void execute_vm_code(const vector<Point> &old_points, vector<Point> &new_points)
{
    const int stack_size = 8192;  //should be enough, 
                                  //there are no checks for stack overflow  
    vector<double> stack(stack_size);
    int M = (int) new_points.size();
    replace_sums(M, stack, old_points);
    // first execute one-time operations: sorting, x[15]=3, etc. 
    // n==M => one-time op.
    bool t = execute_code(M, M, stack, old_points, new_points, code);
    if (!t) 
        return;
    vector<int> to_be_deleted;
    for (int n = 0; n != M; n++) {
        bool r = execute_code(n, M, stack, old_points, new_points, code);
        if (r)
            to_be_deleted.push_back(n);
    }
    if (!to_be_deleted.empty())
        for (vector<int>::const_iterator i = to_be_deleted.end() - 1; 
                                             i >= to_be_deleted.begin(); --i)
            new_points.erase(new_points.begin() + *i);
}


bool transform_data(string const& str, 
                  vector<Point> const& old_points, 
                  vector<Point> &new_points)
{
    assert(code.empty());
    // First compile string...
    parse_info<> result = parse(str.c_str(), DataTransformG, space_p);
    // and then execute compiled code.
    if (result.full) {
        new_points = old_points; //initial values of new_points
        execute_vm_code(old_points, new_points);
    }
    // cleaning
    code.clear();
    numbers.clear();
    for (vector<ParameterizedFunction*>::iterator i=parameterized.begin();
            i != parameterized.end(); ++i)
        delete *i;
    parameterized.clear();

    return (bool) result.full;
}

