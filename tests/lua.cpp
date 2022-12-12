
#include <memory>  // for unique_ptr
#include <string>
#include "fityk/fityk.h"
#include "fityk/logic.h"
#include "fityk/luabridge.h"
#include "catch.hpp"

TEST_CASE("formatted F:execute", "") {
    std::unique_ptr<fityk::Fityk> fik(new fityk::Fityk);
    fityk::Full* priv = fik->priv();
    std::string str = "a=123.456; F:execute('$v = %g' % a)";
    priv->lua_bridge()->exec_lua_string(str);
    double v = fik->get_variable("v")->value();
    REQUIRE(v == 123.456);
}

