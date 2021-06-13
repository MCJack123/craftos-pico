#include <lauxlib.h>
static int rs_getSides(lua_State *L) {lua_newtable(L); return 1;}
static int rs_false(lua_State *L) {lua_pushboolean(L, 0); return 1;}
static int rs_0(lua_State *L) {lua_pushinteger(L, 0); return 1;}
static int rs_none(lua_State *L) {return 0;}
const luaL_Reg rs_lib[] = {
    {"getSides", rs_getSides},
    {"getInput", rs_false},
    {"setOutput", rs_none},
    {"getOutput", rs_false},
    {"getAnalogInput", rs_0},
    {"setAnalogOutput", rs_none},
    {"getAnalogOutput", rs_0},
    {"getBundledInput", rs_0},
    {"getBundledOutput", rs_0},
    {"setBundledOutput", rs_none},
    {"testBundledInput", rs_false},
    {NULL, NULL}
};