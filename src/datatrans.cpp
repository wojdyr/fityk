// this is stand-alone program for a while
//
// the idea of VM is based on one of boost::spirit samples - vmachine_calc

//TODO how to cooperate with background/calibration??

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
// with '&', all are executed of n=0, then all for n=1, and so on).
// Before transformation the new point coordinates are a copy of the old ones.
//
// There is a special function sum(), which can be used as a real value. 
// and returns a sum over all points of value of expression 
// eg. sum(n > 0 ? (x[n] - x[n-1]) * (y[n-1] + y[n])/2 : 0) -> area.
// The value of sum() is computed before transformation.
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
//        sort data points; sorting is stable. Order of points affects only
//        export of data and some transform command
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
// All computations are performed using real numbers, but using round for
// comparisions should not be neccessary. Two numbers that differ less
// than epsilon=1e-9 ie. abs(a-b)<epsilon, are considered equal. 
// Indices are also computed in real number domain, and then rounded to nearest 
// integer.
//     
//
// Syntax examples:
//    s = sqrt(max(1,y))
//    is the same as:  s = sqrt(max(1,y[n]))
//                or:  S = sqrt(max(1,y[n]))
//                or:  S = sqrt(max(1,Y[n]))
//                or:  S = sqrt(max(1,Y))
//    integration:   Y[1...] = Y[n-1] + y[n]  
//    swaping x and y axes: y=x & x=y & s=sqrt(Y) 
//                      or: y=x & x=y & s=sqrt(x)
//    smoothing: Y[1...-1] = (y[n-1] + y[n+1])/2  
//    reducing: order = x,
//              x[...-1] = (x[n]+x[n+1])/2, 
//              y[...-1] = y[n]+y[n+1],
//              delete(n%2==1)    
//           delete(not a)  
//           normalize area: 
//           y = y / sum(n > 0 ? (x[n] - x[n-1]) * (y[n-1] + y[n])/2 : 0) 
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
//
//
//#define BOOST_SPIRIT_DEBUG


//from common.h
#include <math.h>
#include <vector>
inline int iround(double d) { return static_cast<int>(floor(d+0.5)); }
template <typename T>
inline int size(const std::vector<T>& v) { return static_cast<int>(v.size()); }

#include <boost/spirit/core.hpp>
#include <string>
#include <assert.h>
#include <iostream> 

using namespace std;
using namespace boost::spirit;

const double epsilon = 1e-9;

struct Point 
{ 
    Point() : x(0), y(0), sigma(0), is_active(false) {}
    Point(double x_, double y_, double s_) : x(x_), y(y_), sigma(s_), 
                                             is_active(true) {} 
    double x, y, sigma; 
    bool is_active;
};

ostream& operator<<(ostream& out, const Point& p)
    { out << '(' << p.x << ", " << p.y << ", " << p.sigma << ')'; return out; }


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

//------------------------  Virtual Machine  --------------------------------

vector<int> code;        //  VM code 
vector<double> numbers;  //  VM data 

// operators used in VM code
enum DataTransformVMOperators
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
    OP_GT, OP_GE, OP_LT, OP_LE, OP_EQ, OP_NEQ, OP_RANGE, OP_INDEX, 
    OP_ASSIGN_X, OP_ASSIGN_Y, OP_ASSIGN_S, OP_ASSIGN_A,
    OP_DO_ONCE, OP_RESIZE, OP_ORDER, OP_DELETE, OP_BEGIN, OP_END, 
    OP_SUM, OP_IGNORE
};

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
    cout << "SKIPing" << endl;
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
    cout << "executing code; n=" << n << " M=" << M << endl;
    assert(M == size(new_points));
    bool once = (n == M);
    bool return_value=false; 
    vector<double>::iterator stackPtr = stack.begin() - 1;//will be ++'ed first
    for (vector<int>::const_iterator i=code.begin(); i != code.end(); i++) {
        cout << "NOW op " << *i << endl;
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
                new_points[n].is_active = abs(*stackPtr) < epsilon;
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
                if (right < 0)
                    right += M;
                int left = iround(*stackPtr);
                stackPtr--;
                if (left <= 0)
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
                cout << "in OP_ORDER with " << ord << endl;
                if (ord == 1) {
                    cout << "sort x_lt" << endl;
                    stable_sort(new_points.begin(), new_points.end(), x_lt);
                }
                else if(ord == -1) {
                    cout << "sort x_gt" << endl;
                    stable_sort(new_points.begin(), new_points.end(), x_gt);
                }
                else if(ord == 2) {
                    cout << "sort y_lt" << endl;
                    stable_sort(new_points.begin(), new_points.end(), y_lt);
                }
                else if(ord == -2) {
                    cout << "sort y_gt" << endl;
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
                cout << "Unknown operator in VM code: " << int(*i) << endl;
        }
    }
    assert(stackPtr == stack.begin() - 1);
    return return_value;
}

//change  BEGIN X X X SUM END  with  NUMBER END END END END
void replace_sums(int M, vector<double>& stack, vector<Point> const& old_points)
{
    cout << "code before replace:";
    for (vector<int>::const_iterator i=code.begin(); i != code.end(); i++) 
        cout << " " << *i;
    cout << endl;
    bool nested = false;
    bool in_sum = false;
    vector<int>::iterator sum_end;
    for (vector<int>::iterator i = code.end()-1; i != code.begin(); i--) {
        if (*i == OP_SUM && *(i-1) != OP_NUMBER) {
            if (in_sum)
                nested = true;
            sum_end = i;
            in_sum = true;
        }
        else if (in_sum && *i == OP_BEGIN && *(i-1) != OP_NUMBER) {
            double sum = 0.;
            vector<Point> fake_new_points(M);
            vector<int> sum_code(i, sum_end+1);
            for (int n = 0; n != M; n++) {
                execute_code(n, M, stack, old_points, fake_new_points,sum_code);
                cout  << "n=" << n << " on stack: " << stack.front();
                sum += stack.front();
                cout<< " sum:" << sum << endl;
            }
            *i = OP_NUMBER;
            *(i+1) = size(numbers);
            numbers.push_back(sum);
            for (vector<int>::iterator j=i+2; j < sum_end+1; j++) 
                *j = OP_IGNORE;
            in_sum = false;
        }
    }
    //in rare case of nested sum() we need:
    if (nested)
        replace_sums(M, stack, old_points);
    cout << "code after replace:";
    for (vector<int>::const_iterator i=code.begin(); i != code.end(); i++) 
        cout << " " << *i;
    cout << endl;
}


//-- functors used in the grammar for putting VM code and data into vectors --

struct push_double
{
    void operator()(const double& n) const
    {
        cout << "PUSH_DOUBLE "<< n << endl;
        code.push_back(OP_NUMBER);
        code.push_back(size(numbers));
        numbers.push_back(n);
    }

   //void operator()(const int& n) const { operator()(static_cast<double>(n)); }
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
    push_op(int op_) : op(op_) {}

    void push() const { cout<<"PUSHOP "<<op<<endl;  code.push_back(op); }
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
    definition(DataTransformGrammar const& self)
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
              //sum will be refactored, see: replace_sums()
          |   (as_lower_d["sum"] [push_op(OP_BEGIN)]
                  >> '(' >> rprec1 >> ')') [push_op(OP_SUM)]
          |   (as_lower_d["sqrt"] >> '(' >> rprec1 >> ')') [push_op(OP_SQRT)] 
          |   (as_lower_d["exp"] >> '(' >> rprec1 >> ')') [push_op(OP_EXP)] 
          |   (as_lower_d["log10"] >> '(' >> rprec1 >> ')') [push_op(OP_LOG10)]
          |   (as_lower_d["ln"] >> '(' >> rprec1 >> ')') [push_op(OP_LN)] 
          |   (as_lower_d["sin"] >> '(' >> rprec1 >> ')') [push_op(OP_SIN)] 
          |   (as_lower_d["cos"] >> '(' >> rprec1 >> ')') [push_op(OP_COS)] 
          |   (as_lower_d["tan"] >> '(' >> rprec1 >> ')') [push_op(OP_TAN)] 
          |   (as_lower_d["atan"] >> '(' >> rprec1 >> ')') [push_op(OP_ATAN)] 
          |   (as_lower_d["asin"] >> '(' >> rprec1 >> ')') [push_op(OP_ASIN)] 
          |   (as_lower_d["acos"] >> '(' >> rprec1 >> ')') [push_op(OP_ACOS)] 
          |   (as_lower_d["abs"] >> '(' >> rprec1 >> ')') [push_op(OP_ABS)] 
          |   (as_lower_d["round"] >> '(' >> rprec1 >> ')') [push_op(OP_ROUND)]
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
          = rprec2
             >> !( ("<=" >> rprec2) [push_op(OP_LE)]
                 | (">=" >> rprec2) [push_op(OP_GE)]
                 | ((str_p("==")|"=") >> rprec2)  [push_op(OP_EQ)]
                 | ((str_p("!=")|"<>") >> rprec2)  [push_op(OP_NEQ)]
                 | ('<' >> rprec2)  [push_op(OP_LT)]
                 | ('>' >> rprec2)  [push_op(OP_GT)]
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
          =  (eps_p[push_op(OP_BEGIN)] >> assignment >> eps_p[push_op(OP_END)])
             % ch_p('&') 
          ;
    }

    rule<ScannerT> rprec1, rprec2, rprec3, rprec4, rprec5, rprec6,  
                   rbool_or, rbool_and, rbool_not, rbool,
                   real_constant, real_variable,       
                   index, assignment, statement, range, order;

    rule<ScannerT> const& start() const { return statement; }
  };
};


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


bool do_transform(string const& str, 
                  vector<Point> const& old_points, 
                  vector<Point> &new_points)
{
    // First compile string...
    code.clear();
    numbers.clear();
    DataTransformGrammar calc; 
    parse_info<> result = parse(str.c_str(), calc, space_p);
    // and then execute compiled code.
    if (result.full) {
        new_points = old_points; //initial values of new_points
        execute_vm_code(old_points, new_points);
        return true;
    }
    else
        return false;
}

//-------------------------  Main program  ---------------------------------
int main()
{
    cout << "DataTransformVMachine test started...\n";

    string str;
    while (getline(cin, str))
    {
        vector<Point> points, transformed_points;

        points.push_back(Point(0.1, 4, 1));
        points.push_back(Point(0.2, 2, 1));
        points.push_back(Point(0.3, -2, 1));
        points.push_back(Point(0.4, 3, 1));
        points.push_back(Point(0.5, 3, 1));

        bool r = do_transform(str, points, transformed_points);
        int len = (int) points.size();
        if (r)
            for (int n = 0; n != (int) transformed_points.size(); n++) 
                cout << "point " << n << ": " << (n < len ? points[n] : Point())
                    << " -> " << transformed_points[n] <<  endl;
        else
            cout << "Syntax error" << endl;
    }
    return 0;
}

