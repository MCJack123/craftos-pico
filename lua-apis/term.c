#include <lua.h>
#include <lauxlib.h>
#include <assert.h>

static int redirect_missing(lua_State *L) {
    return luaL_error(L, "Redirect object is missing method %s.", lua_tostring(L, lua_upvalueindex(1)));
}

static int term_redirect(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getglobal(L, "term");
    if (lua_equal(L, 1, -1)) luaL_error(L, "term is not a recommended redirect target, try term.current() instead");
    lua_settop(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "term.native");
    lua_pushnil(L);
    while (lua_next(L, 2)) {
        if (lua_type(L, -2) == LUA_TSTRING && (lua_type(L, -1) == LUA_TFUNCTION || lua_type(L, -1) == LUA_TLIGHTFUNCTION)) {
            lua_pushvalue(L, -2);
            lua_gettable(L, 1);
            if (lua_type(L, -1) != LUA_TFUNCTION && lua_type(L, -1) != LUA_TLIGHTFUNCTION) {
                lua_pushvalue(L, -3);
                lua_pushvalue(L, -1);
                lua_pushcclosure(L, redirect_missing, 1);
                lua_settable(L, 1);
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_getfield(L, LUA_REGISTRYINDEX, "term.current");
    lua_getfield(L, -1, "__index");
    lua_pushvalue(L, 1);
    lua_setfield(L, -3, "__index");
    assert(lua_type(L, -1) == LUA_TTABLE || lua_type(L, -1) == LUA_TROTABLE);
    return 1;
}

static int term_current(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "term.current");
    lua_getfield(L, -1, "__index");
    return 1;
}

static int term_native(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "term.native");
    return 1;
}

extern int luaopen_term(lua_State *L) {
    lua_settop(L, 0);
    lua_pushrotable(L, (void*)(size_t)4); // 1 (native)
    lua_pushvalue(L, 1);
    lua_setfield(L, LUA_REGISTRYINDEX, "term.native");

    lua_newtable(L); // 2 (term)
    lua_createtable(L, 0, 1); // 3 (mt)
    lua_pushvalue(L, 1);
    lua_setfield(L, 3, "__index");
    lua_pushvalue(L, 3);
    lua_setfield(L, LUA_REGISTRYINDEX, "term.current");
    lua_setmetatable(L, 2);
    lua_pushvalue(L, 2);
    lua_setglobal(L, "term");

    lua_pushlightfunction(L, term_redirect);
    lua_setfield(L, 2, "redirect");
    lua_pushlightfunction(L, term_current);
    lua_setfield(L, 2, "current");
    lua_pushlightfunction(L, term_native);
    lua_setfield(L, 2, "native");
    return 1;
}