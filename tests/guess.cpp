
#include <stdio.h>
#include <boost/scoped_ptr.hpp>
#include "fityk/logic.h"
#include "fityk/guess.h"
#include "fityk/data.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace std;
using namespace fityk;

// data from http://www.itl.nist.gov/div898/strd/lls/data/LINKS/DATA/Norris.dat
namespace norris {

const double yx[][2] = {
 {    0.1 ,      0.2 },
 {  338.8 ,    337.4 },
 {  118.1 ,    118.2 },
 {  888.0 ,    884.6 },
 {    9.2 ,     10.1 },
 {  228.1 ,    226.5 },
 {  668.5 ,    666.3 },
 {  998.5 ,    996.3 },
 {  449.1 ,    448.6 },
 {  778.9 ,    777.0 },
 {  559.2 ,    558.2 },
 {    0.3 ,      0.4 },
 {    0.1 ,      0.6 },
 {  778.1 ,    775.5 },
 {  668.8 ,    666.9 },
 {  339.3 ,    338.0 },
 {  448.9 ,    447.5 },
 {   10.8 ,     11.6 },
 {  557.7 ,    556.0 },
 {  228.3 ,    228.1 },
 {  998.0 ,    995.8 },
 {  888.8 ,    887.6 },
 {  119.6 ,    120.2 },
 {    0.3 ,      0.3 },
 {    0.6 ,      0.3 },
 {  557.6 ,    556.8 },
 {  339.3 ,    339.1 },
 {  888.0 ,    887.2 },
 {  998.5 ,    999.0 },
 {  778.9 ,    779.0 },
 {   10.2 ,     11.1 },
 {  117.6 ,    118.3 },
 {  228.9 ,    229.2 },
 {  668.4 ,    669.1 },
 {  449.2 ,    448.9 },
 {    0.2 ,      0.5 } };

const double cert_b0 = -0.262323073774029;
const double cert_b1 = 1.00211681802045;

} // namespace norris

template <typename T>
bool is_vector_sorted(const vector<T>& v)
{
    for (typename vector<T>::const_iterator i = v.begin()+1; i < v.end(); ++i)
            if (*i < *(i-1))
                return false;
    return true;
}

TEST_CASE("linear-guess", "test Guess::estimate_linear_parameters()") {
    boost::scoped_ptr<Ftk> ftk(new Ftk);
    Data *data = ftk->get_data(0);
    //ftk->settings_mgr()->set_as_number("verbosity", -1);
    int n = sizeof(norris::yx) / sizeof(norris::yx[0]);
    for (int i = 0; i < n; ++i) {
        double x = norris::yx[i][1];
        double y = norris::yx[i][0];
        data->add_one_point(x, y, 1);
    }
    REQUIRE(is_vector_sorted(data->points()));
    Guess g(ftk->get_settings());
    g.set_data(ftk->get_dm(0), RealRange(), -1);
    boost::array<double,3> lin_est = g.estimate_linear_parameters();
    //printf("slope %.14f\nintercept %.14f\n", lin_est[0], lin_est[1]);
    REQUIRE(lin_est[0] == Approx(norris::cert_b1));
    REQUIRE(lin_est[1] == Approx(norris::cert_b0));
}
