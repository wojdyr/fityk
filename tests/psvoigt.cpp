
#include <stdio.h>
#include <boost/scoped_ptr.hpp>
#include "fityk/fityk.h"
// derivatives need private API
#include "fityk/logic.h"
#include "fityk/model.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace std;

// these definitions of Pseudo-Voigt should be identical to the built-in one.
const char* PV2 = "define PV2(height, center, hwhm, shape) = "
 "Gaussian((1-shape)*height, center, hwhm) + "
 "Lorentzian(shape*height, center, hwhm)";
const char* PV3 = "define PV3(height, center, hwhm, shape) = "
 "(1-shape)*height*exp(-ln(2)*((x-center)/hwhm)^2) + "
 "shape*height/(1+((x-center)/hwhm)^2)";
const char* PV4 = "define PV4(height, center, hwhm, shape) = "
 "SplitPseudoVoigt(height, center, hwhm, hwhm, shape, shape)";

TEST_CASE("pseudo-voigt", "") {
    boost::scoped_ptr<fityk::Fityk> fik(new fityk::Fityk);
    fik->set_option_as_number("verbosity", -1);
    fik->execute(PV2);
    fik->execute(PV3);
    fik->execute(PV4);
    const char* types[] = { "PseudoVoigt", "PV2", "PV3", "PV4" };
    for (int i = 0; i < 4; ++i) {
        string t = types[i];
        INFO( "Testing " << t );
        fik->execute("%pv = " + t + "(~1.2, ~2.3, ~3.4, ~0.3)");
        double height = fik->calculate_expr("%pv.height");
        REQUIRE(height == 1.2);
        double center = fik->calculate_expr("%pv.Center");
        REQUIRE(center == 2.3);
        if (t != "PV3") {
            if (t != "PV2") {
                double fwhm = fik->calculate_expr("%pv.FWHM");
                REQUIRE(fwhm == 2*3.4);
            }
            double area = fik->calculate_expr("%pv.Area");
            REQUIRE(area == Approx(9.925545022985));
            double H = fik->calculate_expr("%pv.Height");
            REQUIRE(H == height);
        }
        const fityk::Func *f = fik->get_function("pv");
        double at_ctr = f->value_at(center);
        REQUIRE(at_ctr == height);
        double at_2 = f->value_at(2);
        REQUIRE(at_2 == Approx(1.1926980547071));
        double at_3 = f->value_at(3);
        REQUIRE(at_3 == Approx(1.1610401512578));

        fik->execute("F=%pv");
        vector<realt> deriv =
            fik->priv()->dk.get_model(0)->get_symbolic_derivatives(3.0);
        double expected_deriv[5] = { 0.967533459381518,
                                     0.108597246412477,
                                     0.022358256614333,
                                    -0.014052616793924,
                                    -0.108597246412477 };
        for (int j = 0; j != 5; ++j)
            REQUIRE(deriv[j] == Approx(expected_deriv[j]));
    }
}
