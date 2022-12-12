
#include <stdio.h>
#include <memory>  // for unique_ptr
#include "fityk/fityk.h"
// derivatives need private API
#include "fityk/logic.h"
#include "fityk/model.h"
// get_nonzero_range() needs private API
#include "fityk/func.h"

#include "catch.hpp"

using namespace std;

// these definitions of Pseudo-Voigt should be identical to the built-in one.
static const char* PV2 = "define PV2(height, center, hwhm, shape) = "
 "Gaussian((1-shape)*height, center, hwhm) + "
 "Lorentzian(shape*height, center, hwhm)";
static const char* PV3 = "define PV3(height, center, hwhm, shape) = "
 "(1-shape)*height*exp(-ln(2)*((x-center)/hwhm)^2) + "
 "shape*height/(1+((x-center)/hwhm)^2)";
static const char* PV4 = "define PV4(height, center, hwhm, shape) = "
 "SplitPseudoVoigt(height, center, hwhm, hwhm, shape, shape)";

TEST_CASE("pseudo-voigt", "") {
    unique_ptr<fityk::Fityk> fik(new fityk::Fityk);
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
            double ib = fik->calculate_expr("%pv.IB");
            REQUIRE(ib == Approx(area/height));
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
            fik->priv()->dk.get_model(0)->get_symbolic_derivatives(3.0, NULL);
        double expected_deriv[5] = { 0.967533459381518,
                                     0.108597246412477,
                                     0.022358256614333,
                                    -0.014052616793924,
                                    -0.108597246412477 };
        for (int j = 0; j != 5; ++j)
            REQUIRE(deriv[j] == Approx(expected_deriv[j]));
    }
}


// test Function::get_nonzero_range() implementations

static double nonzero_left(double level, const string& func) {
    unique_ptr<fityk::Fityk> fik(new fityk::Fityk);
    fik->set_option_as_number("verbosity", -1);
    fik->execute("%f = " + func);
    const fityk::Func *f = fik->get_function("f");
    double left, right;
    const fityk::Function *fpriv = dynamic_cast<const fityk::Function*>(f);
    bool r = fpriv->get_nonzero_range(level, left, right);
    REQUIRE(r == true);
    double y_left = f->value_at(left);
    return y_left;
}

TEST_CASE("Gaussian::get_nonzero_range()", "") {
    REQUIRE(nonzero_left(0.01, "Gaussian(0.9, 11.8, 1.08)") == Approx(0.01));
    REQUIRE(nonzero_left(1e-5, "Gaussian(-20.2, -0.8, 6.7)") == Approx(-1e-5));
}

TEST_CASE("Lorentzian::get_nonzero_range()", "") {
    REQUIRE(nonzero_left(0.01, "Lorentzian(0.9, 11.8, 1.08)") == Approx(0.01));
    REQUIRE(nonzero_left(1e-5, "Lorentzian(-20, -0.8, 6.7)") == Approx(-1e-5));
}

TEST_CASE("Pearson7::get_nonzero_range()", "") {
    REQUIRE(nonzero_left(0.01, "Pearson7(0.9, 11.8, 1.08, 1.3)") ==
            Approx(0.01));
    REQUIRE(nonzero_left(1e-5, "Pearson7(-20.2, -0.8, 6.7, 1.3)") ==
            Approx(-1e-5));
}

TEST_CASE("PseudoVoigt::get_nonzero_range()", "") {
    // PseudoVoigt::get_nonzero_range() uses rough estimation
    double b = nonzero_left(0.01, "PseudoVoigt(0.9, 11.8, 1.08, 0.7)");
    REQUIRE(b > 0.004);
    REQUIRE(b < 0.01);
    REQUIRE(nonzero_left(1e-5, "PseudoVoigt(-20, -0.8, 6.7, 0.7)") ==
            Approx(-1e-5));
}

TEST_CASE("Voigt::get_nonzero_range()", "") {
    // Voigt::get_nonzero_range() also uses rough estimation
    double b1 = nonzero_left(0.04, "Voigt(0.9, 11.8, 1.08, 0.4)");
    REQUIRE(b1 > 0.026);
    REQUIRE(b1 < 0.04);
    double b2 = nonzero_left(0.8, "Voigt(0.9, 11.8, 1.08, 1.8)");
    REQUIRE(b2 > 0.7);
    REQUIRE(b2 < 0.8);
    double b3 = nonzero_left(0.2, "Voigt(0.9, 11.8, 1.08, 1.8)");
    REQUIRE(b3 > 0.14);
    REQUIRE(b3 < 0.2);
    REQUIRE(nonzero_left(1e-5, "Voigt(-20, -0.8, 6.7, 1.1)") == Approx(-1e-5));
}
