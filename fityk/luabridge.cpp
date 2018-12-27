
#define BUILDING_LIBFITYK
#include "luabridge.h"
#include "logic.h"
#include "ui.h"

#ifndef DISABLE_LUA

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "swig/luarun.h"   // the SWIG external runtime
extern int luaopen_fityk(lua_State *L); // the SWIG wrappered library
}

using namespace std;

// based on luaB_print from lbaselib.c
static int fityk_lua_print(lua_State* L) {
    using fityk::UserInterface;
    string str;
    int n = lua_gettop(L); // number of arguments
    lua_getglobal(L, "tostring");
    for (int i=1; i<=n; i++) {
        lua_pushvalue(L, -1);  // `tostring' function
        lua_pushvalue(L, i);   // i'th arg
        lua_call(L, 1, 1);     // calls tostring(arg_i)
        const char *s = lua_tostring(L, -1);  // get result
        if (s == NULL)
            return luaL_error(L, "cannot convert argument to string");
        if (i > 1)
            str += "\t";
        str += s;
        lua_pop(L, 1);  // pop result
    }
    UserInterface *ui = (UserInterface*) lua_touserdata(L, lua_upvalueindex(1));
    ui->output_message(UserInterface::kNormal, str);
    return 0;
}

// it may rely on implementation details of Lua
// SWIG-wrapped vector is indexed from 0. Return (n, vec[n]) starting from n=0.
static int lua_vector_iterator(lua_State* L)
{
    assert(lua_isuserdata(L,1)); // in SWIG everything is wrapped as userdata
    int idx = lua_isnil(L, -1) ? 0 : (int) lua_tonumber(L, -1) + 1;

    // no lua_len() in 5.1, let's call size() directly
    lua_getfield(L, 1, "size");
    lua_pushvalue(L, 1); // arg: vector as userdata
    lua_call(L, 1, 1);   // call vector<>::size(this)
    int size = (int) lua_tonumber(L, -1);

    if (idx >= size) {
        lua_settop(L, 0);
        return 0;
    }

    lua_settop(L, 1);
    lua_pushnumber(L, idx); // index, to be returned
    lua_pushvalue(L, -1);   // the same index, to access value
    lua_gettable(L, 1);     // value, to be returned
    lua_remove(L, 1);
    return 2;
}

namespace fityk {

LuaBridge::LuaBridge(Full *F)
    : ctx_(F)
{
    L_ = luaL_newstate();
    luaL_openlibs(L_);
    luaopen_fityk(L_);

    // SWIG-generated luaopen_fityk() leaves two tables on the stack,
    // clear the stack
    lua_settop(L_, 0);

    // make vectors also iterators over elements
    const char* vectors[] = { "FuncVector", "VarVector", "PointVector",
                              "RealVector", "IntVector" };
    for (int i = 0; i < 5; ++i) {
        SWIG_Lua_get_class_metatable(L_, vectors[i]);
        SWIG_Lua_add_function(L_, "__call", lua_vector_iterator);
        lua_pop(L_, 1);
    }

    // define F
    swig_type_info *type_info = SWIG_TypeQuery(L_, "fityk::Fityk *");
    assert(type_info != NULL);
    int owned = 1;
    Fityk *f = new Fityk(F);
    SWIG_NewPointerObj(L_, f, type_info, owned);
    lua_setglobal(L_, "F");

    // redefine print
    UserInterface *ui = ctx_->ui();
    lua_pushlightuserdata(L_, ui);
    lua_pushcclosure(L_, fityk_lua_print, 1);
    lua_setglobal(L_, "print");

    // Python-like string formatting with % operator, based on
    // http://lua-users.org/wiki/StringInterpolation
    const char *lua_init_str =
     "getmetatable('').__mod = function(a, b)\n"
     "  if not b then return a\n"
     "  elseif type(b) == 'table' then return string.format(a, "
#if LUA_VERSION_NUM >= 502
         "table."
#endif
         "unpack(b))\n"
     "  else return string.format(a, b)\n"
     "  end\n"
     "end\n";

    int status = luaL_dostring(L_, lua_init_str);
    if (status != 0) // just in case, should not happen
        handle_lua_error();
}


LuaBridge::~LuaBridge()
{
    if (L_ != NULL)
        lua_close(L_);
}


void LuaBridge::exec_lua_script(const string& str)
{
    // pass filename in arg[0], like in the Lua stand-alone interpreter
    lua_createtable(L_, 1, 0);
    lua_pushstring(L_, str.c_str());
    lua_rawseti(L_, -2, 0);
    lua_setglobal(L_, "arg");

    int status = luaL_dofile(L_, str.c_str());
    if (status != 0)
        handle_lua_error();
}

// for use in GUI EditorDlg
bool LuaBridge::is_lua_line_incomplete(const char* str)
{
    int status = luaL_loadstring(L_, str);
    if (status == LUA_ERRSYNTAX) {
        size_t lmsg;
        const char *msg = lua_tolstring(L_, -1, &lmsg);
#if LUA_VERSION_NUM <= 501
        if (lmsg >= 7 && strcmp(msg + lmsg - 7, "'<eof>'") == 0)
#else
        if (lmsg >= 5 && strcmp(msg + lmsg - 5, "<eof>") == 0)
#endif
        {
            lua_pop(L_, 1);
            return true;
        }
    }
    lua_pop(L_, 1);
    return false;
}

void LuaBridge::exec_lua_string(const string& str)
{
    int status = luaL_dostring(L_, str.c_str());
    if (status == 0 && lua_gettop(L_) > 0) { // print returned values
        luaL_checkstack(L_, LUA_MINSTACK, "too many results to print");
        lua_getglobal(L_, "print");
        lua_insert(L_, 1);
        status = lua_pcall(L_, lua_gettop(L_)-1, 0, 0);
    }
    if (status != 0) // LUA_OK(=0) was added in Lua 5.2
        handle_lua_error();
}

void LuaBridge::exec_lua_output(const string& str)
{
    int status = luaL_dostring(L_, ("return "+str).c_str());
    if (status == 0) {
        int n = lua_gettop(L_); // number of args
        lua_getglobal(L_, "tostring");
        for (int i = 1; i <= n; ++i) {
            lua_pushvalue(L_, -1); // tostring()
            lua_pushvalue(L_, i);  // value
            lua_call(L_, 1, 1);
            const char *s = lua_tolstring(L_, -1, NULL);
            if (s == NULL)
                // luaL_error() long-jumps or throws and doesn't return
                luaL_error(L_, "cannot covert value to string");

            UserInterface::Status r = ctx_->ui()->execute_line(s);
            if (r != UserInterface::kStatusOk &&
                    ctx_->get_settings()->on_error[0] != 'n'/*nothing*/)
                break;

            lua_pop(L_, 1); // pop tostring result
        }
        lua_settop(L_, 0);
    }
    if (status != 0) // LUA_OK(=0) was added in Lua 5.2
        handle_lua_error();
}

void LuaBridge::handle_lua_error()
{
    const char *msg = lua_tostring(L_, -1);
    ctx_->ui()->warn("Lua Error:\n" + S(msg ? msg : "(non-string error)"));
    lua_pop(L_, 1);
}

} // namespace fityk

#else // DISABLE_LUA

namespace fityk {
LuaBridge::LuaBridge(Full*) {}
LuaBridge::~LuaBridge() {}
void LuaBridge::close_lua() {}
void LuaBridge::exec_lua_string(const std::string&) {}
void LuaBridge::exec_lua_script(const std::string&) {}
void LuaBridge::exec_lua_output(const std::string&) {}
bool LuaBridge::is_lua_line_incomplete(const char*) { return true; }
}

#endif // DISABLE_LUA
