// This file is part of fityk program. Copyright Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_LUABRIDGE_H_
#define FITYK_LUABRIDGE_H_

#include "common.h"

struct lua_State;

namespace fityk {

class Ftk;
class BasicContext;

class LuaBridge
{
public:
    LuaBridge(Ftk *F);
    ~LuaBridge();
    void close_lua();
    void exec_lua_string(const std::string& str);
    void exec_lua_script(const std::string& str);
    void exec_lua_output(const std::string& str);
    bool is_lua_line_incomplete(const char* str);
    //lua_State* state() { return L_ };

private:
    lua_State *L_;
    BasicContext* ctx_;

    void handle_lua_error();
    DISALLOW_COPY_AND_ASSIGN(LuaBridge);
};

} // namespace fityk

#endif
