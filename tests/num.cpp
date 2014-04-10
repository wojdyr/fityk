
#include <boost/scoped_ptr.hpp>
#include "fityk/numfuncs.h"
#include "catch.hpp"

using std::vector;
using fityk::invert_matrix;

TEST_CASE("invert-matrix-1x1", "") {
    vector<realt> mat(1, 4.);
    invert_matrix(mat, 1);
    REQUIRE(mat[0] == 0.25);
}

TEST_CASE("invert-matrix-4x4", "") {
    const double a[16] = {
         1.,  2.,  3.,  4.,
        -1.,  0.,  1.,  2.,
         2.,  3.,  1.,  4.,
         1.,  2.,  1.,  0. };
    const double a_inv[16] = {
         0.5, -1.0,  0.0, -0.5,
        -0.5,  0.6,  0.2,  0.7,
         0.5, -0.2, -0.4,  0.1,
         0.0,  0.1,  0.2, -0.3 };
    vector<realt> mat(a, a+16);
    invert_matrix(mat, 4);
    for (size_t i = 0; i != mat.size(); ++i)
        REQUIRE(mat[i] == Approx(a_inv[i]));
    invert_matrix(mat, 4);
    for (size_t i = 0; i != mat.size(); ++i)
        REQUIRE(mat[i] == Approx(a[i]));
}

/*
TEST_CASE("invert-matrix-with-zeros", "") {
    const double a[9] = { 4., 0., 7.,
                          0., 0., 0.,
                          2., 0., 6. };
    const double a_inv[9] = { 0.6,  0.0, -0.7,
                              0.0,  0.0,  0.0,
                             -0.2,  0.0,  0.4 };
    vector<realt> mat(a, a+9);
    invert_matrix(mat, 3);
    for (size_t i = 0; i != mat.size(); ++i)
        REQUIRE(mat[i] == Approx(a_inv[i]));
}
*/
