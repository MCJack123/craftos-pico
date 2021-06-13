#include <lua.h>
#include <lauxlib.h>

// A number entry in the read only table
typedef struct
{
  const char *name;
  lua_Number value;
} luaR_value_entry;

// This does not implement full Rednet - it only provides a stub,
// since modems are not implemented (yet).

static int rednet_open(lua_State *L) {
    return luaL_error(L, "No such modem: %s", luaL_checkstring(L, 1));
}

static int rednet_isOpen(lua_State *L) {
    lua_pushboolean(L, 0);
    return 1;
}

static int rednet_receive(lua_State *L) {
    lua_Number timeout;
    if (lua_icontext(L) == 1) return 0;
    if (lua_isnumber(L, 1)) timeout = lua_tonumber(L, 1);
    else if (lua_isnumber(L, 2)) timeout = lua_tonumber(L, 2);
    else return lua_iyield(L, 0, 2); // yield forever
    lua_getglobal(L, "sleep");
    lua_pushnumber(L, timeout);
    lua_icall(L, 1, 0, 1);
    return 0;
}

static int rednet_nil(lua_State *L) {
    return 0;
}

static int rednet_run(lua_State *L) {
    return lua_iyield(L, 0, 1);
}

const luaL_Reg rednet_funcs[] = {
    {"open", rednet_open},
    {"close", rednet_nil},
    {"isOpen", rednet_isOpen},
    {"send", rednet_nil},
    {"broadcast", rednet_nil},
    {"receive", rednet_receive},
    {"host", rednet_nil},
    {"unhost", rednet_nil},
    {"lookup", rednet_nil},
    {"run", rednet_run},
    {NULL, NULL}
};

const luaR_value_entry rednet_values[] = {
    {"CHANNEL_BROADCAST", 65535},
    {"CHANNEL_REPEAT", 65533},
    {NULL, 0}
};