#include <lua.h>
#include <lauxlib.h>

static int settings_set(lua_State *L) {
    int type = lua_type(L, 2);
    luaL_checkstring(L, 1);
    if (type != LUA_TNUMBER && type != LUA_TSTRING && type != LUA_TBOOLEAN && type != LUA_TTABLE)
        luaL_typerror(L, 2, "value");
    lua_getfield(L, LUA_REGISTRYINDEX, "_settings");
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_settable(L, -3);
    return 0;
}

static int settings_get(lua_State *L) {
    // this is supposed to copy tables, but that will use a bit extra memory
    luaL_checkstring(L, 1);
    lua_settop(L, 2); // to add a default value = nil if not provided
    lua_getfield(L, LUA_REGISTRYINDEX, "_settings");
    lua_pushvalue(L, 1);
    lua_gettable(L, -2);
    if (lua_isnil(L, -1)) lua_pop(L, 2); // pop _settings + value
    return 1;
}

static int settings_unset(lua_State *L) {
    luaL_checkstring(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "_settings");
    lua_pushvalue(L, 1);
    lua_pushnil(L);
    lua_settable(L, -3);
    return 0;
}

static int settings_clear(lua_State *L) {
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, "_settings");
    return 0;
}

static int settings_getNames(lua_State *L) {
    int i;
    lua_settop(L, 0);
    lua_getfield(L, LUA_REGISTRYINDEX, "_settings");
    lua_newtable(L);
    lua_pushnil(L);
    for (i = 1; lua_next(L, 1); i++) {
        lua_pop(L, 1);
        lua_pushvalue(L, -1);
        lua_rawseti(L, 2, i);
    }
    lua_getglobal(L, "table");
    lua_getfield(L, -1, "sort");
    lua_pushvalue(L, 2);
    lua_call(L, 1, 0);
    lua_settop(L, 2);
    return 1;
}

static int settings_load(lua_State *L) {
    size_t sz;
    const char * data;
    luaL_checkstring(L, 1);
    lua_settop(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "_settings");
    lua_getglobal(L, "fs");
    lua_getfield(L, -1, "open");
    lua_pushvalue(L, 1);
    lua_pushliteral(L, "r");
    lua_call(L, 2, 1);
    if (lua_isnil(L, -1)) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_getfield(L, -1, "readAll");
    lua_call(L, 0, 1);
    lua_getfield(L, -2, "close");
    lua_call(L, 0, 0);
    lua_pushliteral(L, "return ");
    lua_pushvalue(L, -2);
    lua_concat(L, 2);
    lua_remove(L, -2);
    data = lua_tolstring(L, -1, &sz);
    if (luaL_loadbuffer(L, data, sz, "=unserialize")) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_newtable(L);
    lua_setfenv(L, -2);
    if (lua_pcall(L, 0, 1, 0) || lua_type(L, -1) != LUA_TTABLE) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        sz = lua_type(L, -1);
        if (lua_type(L, -2) == LUA_TSTRING && (sz == LUA_TSTRING || sz == LUA_TNUMBER || sz == LUA_TBOOLEAN || sz == LUA_TTABLE)) {
            lua_pushvalue(L, -2);
            lua_pushvalue(L, -2);
            lua_settable(L, 2);
        }
        lua_pop(L, 1);
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int settings_save(lua_State *L) {
    luaL_checkstring(L, 1);
    lua_getglobal(L, "fs");
    lua_getfield(L, -1, "open");
    lua_pushvalue(L, 1);
    lua_pushliteral(L, "w");
    lua_call(L, 2, 1);
    if (lua_isnil(L, -1)) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_getfield(L, -1, "write");
    lua_getglobal(L, "textutils");
    lua_getfield(L, -1, "serialize");
    lua_remove(L, -2);
    lua_getfield(L, LUA_REGISTRYINDEX, "_settings");
    lua_call(L, 1, 1);
    lua_call(L, 1, 0);
    lua_getfield(L, -1, "close");
    lua_call(L, 0, 0);
    lua_pushboolean(L, 1);
    return 1;
}

const luaL_Reg settings_lib[] = {
    {"set", settings_set},
    {"get", settings_get},
    {"unset", settings_unset},
    {"clear", settings_clear},
    {"getNames", settings_getNames},
    {"load", settings_load},
    {"save", settings_save},
    {NULL, NULL}
};