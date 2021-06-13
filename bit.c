#include <lauxlib.h>

static int bit_band(lua_State *L) {
    unsigned int a, b;
    a = lua_tointeger(L, 1);
    b = lua_tointeger(L, 2);
    lua_pushinteger(L, a & b);
    return 1;
}

static int bit_bor(lua_State *L) {
    unsigned int a, b;
    a = lua_tointeger(L, 1);
    b = lua_tointeger(L, 2);
    lua_pushinteger(L, a | b);
    return 1;
}

static int bit_bnot(lua_State *L) {
    unsigned int a = lua_tointeger(L, 1);
    lua_pushinteger(L, ~a & 0xFFFFFFFF);
    return 1;
}

static int bit_bxor(lua_State *L) {
    unsigned int a, b;
    a = lua_tointeger(L, 1);
    b = lua_tointeger(L, 2);
    lua_pushinteger(L, a ^ b);
    return 1;
}

static int bit_blshift(lua_State *L) {
    unsigned int a, b;
    a = lua_tointeger(L, 1);
    b = lua_tointeger(L, 2);
    lua_pushinteger(L, a << b);
    return 1;
}

static int bit_brshift(lua_State *L) {
    unsigned int a, b;
    a = lua_tointeger(L, 1);
    b = lua_tointeger(L, 2);
    lua_pushinteger(L, a >> b | ((((a & 0x80000000) << b) - 1) << (32 - b)));
    return 1;
}

static int bit_blogic_rshift(lua_State *L) {
    unsigned int a, b;
    a = lua_tointeger(L, 1);
    b = lua_tointeger(L, 2);
    lua_pushinteger(L, a >> b);
    return 1;
}

const luaL_Reg bit_lib[] = {
    {"band", bit_band},
    {"bor", bit_bor},
    {"bnot", bit_bnot},
    {"bxor", bit_bxor},
    {"blshift", bit_blshift},
    {"brshift", bit_brshift},
    {"blogic_rshift", bit_blogic_rshift},
    {NULL, NULL}
};