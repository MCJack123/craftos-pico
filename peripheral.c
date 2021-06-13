#include <lauxlib.h>
static int peripheral_getNames(lua_State *L) {lua_newtable(L); return 1;}
static int peripheral_isPresent(lua_State *L) {lua_pushboolean(L, 0); return 1;}
static int peripheral_nil(lua_State *L) {return 0;}
const luaL_Reg peripheral_lib[] = {
    {"getNames", peripheral_getNames},
    {"isPresent", peripheral_isPresent},
    {"getType", peripheral_nil},
    {"getMethods", peripheral_nil},
    {"getName", peripheral_nil},
    {"call", peripheral_nil},
    {"wrap", peripheral_nil},
    {"find", peripheral_nil},
    {NULL, NULL}
};