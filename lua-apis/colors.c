#include <lua.h>
#include <lauxlib.h>

// A number entry in the read only table
typedef struct
{
  const char *name;
  lua_Number value;
} luaR_value_entry;

static int colors_combine(lua_State *L) {
    uint16_t n = 0;
    for (int i = 1; i <= lua_gettop(L); i++) n |= luaL_checkinteger(L, i);
    lua_pushinteger(L, n);
    return 1;
}

static int colors_subtract(lua_State *L) {
    uint16_t n = luaL_checkinteger(L, 1);
    for (int i = 2; i <= lua_gettop(L); i++) n &= ~luaL_checkinteger(L, i);
    lua_pushinteger(L, n);
    return 1;
}

static int colors_test(lua_State *L) {
    uint16_t a = luaL_checkinteger(L, 1);
    uint16_t b = luaL_checkinteger(L, 2);
    lua_pushboolean(L, (a & b) == b);
    return 1;
}

static int colors_rgb8(lua_State *L) {
    lua_Number r = luaL_checknumber(L, 1);
    if (lua_isnoneornil(L, 2) && lua_isnoneornil(L, 3)) {
        uint32_t rgb = (uint32_t)r;
        lua_pushnumber(L, ((rgb >> 16) & 0xFF) / 255.0);
        lua_pushnumber(L, ((rgb >> 8) & 0xFF) / 255.0);
        lua_pushnumber(L, (rgb & 0xFF) / 255.0);
        return 3;
    } else {
        lua_Number g = luaL_checknumber(L, 2);
        lua_Number b = luaL_checknumber(L, 3);
        lua_pushinteger(L, (uint32_t)(r * 255) << 16 | (uint32_t)(g * 255) << 8 | (uint32_t)(b * 255));
        return 1;
    }
}

const luaL_Reg colors_funcs[] = {
    {"combine", colors_combine},
    {"subtract", colors_subtract},
    {"test", colors_test},
    {"rgb8", colors_rgb8},
    {NULL, NULL}
};

const luaR_value_entry colors_values[] = {
    {"white", 1},
    {"orange", 2},
    {"magenta", 4},
    {"lightBlue", 8},
    {"yellow", 16},
    {"lime", 32},
    {"pink", 64},
    {"gray", 128},
    {"lightGray", 256},
    {"cyan", 512},
    {"purple", 1024},
    {"blue", 2048},
    {"brown", 4096},
    {"green", 8192},
    {"red", 16384},
    {"black", 32768},
    {NULL, 0}
};

const luaR_value_entry colours_values[] = {
    {"white", 1},
    {"orange", 2},
    {"magenta", 4},
    {"lightBlue", 8},
    {"yellow", 16},
    {"lime", 32},
    {"pink", 64},
    {"grey", 128},
    {"lightGrey", 256},
    {"cyan", 512},
    {"purple", 1024},
    {"blue", 2048},
    {"brown", 4096},
    {"green", 8192},
    {"red", 16384},
    {"black", 32768},
    {NULL, 0}
};