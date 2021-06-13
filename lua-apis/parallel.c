#include <lua.h>
#include <lauxlib.h>
#include <string.h>

static int runUntilLimit(lua_State *L, int count, int limit) {
    int i, j, args, status, lastdead = 0, living = count;
    lua_State *thread;
    if (count == 0) return 0;
    for (i = 1; i <= count; i++) {
        if (lua_isthread(L, i)) {
            thread = lua_tothread(L, i);
            args = lua_gettop(L) - count - 1;
            if (args > 0) {
                lua_rawgeti(L, count + 1, i);
                if (lua_isstring(L, count + 2) && !(lua_isnil(L, -1) || lua_equal(L, count + 2, -1) || strcmp(lua_tostring(L, count + 2), "terminate") == 0)) {
                    lua_pop(L, 1);
                    continue;
                }
                lua_pop(L, 1);
                if (!lua_checkstack(L, args)) luaL_error(L, "not enough memory");
                for (j = 1; j <= args; j++) lua_pushvalue(L, count + 1 + j);
                if (!lua_checkstack(thread, args)) luaL_error(L, "not enough memory");
                lua_xmove(L, thread, args);
            }
            status = lua_resume(thread, args);
            if (status == 0) {
                lua_pushnil(L);
                lua_replace(L, i);
                living--;
            } else if (status == LUA_YIELD) {
                lua_settop(thread, 1);
                lua_checkstack(L, 1);
                lua_xmove(thread, L, 1);
                lua_rawseti(L, count + 1, i);
            } else {
                lua_checkstack(L, 1);
                lua_xmove(thread, L, 1);
                lua_error(L);
            }
        } else living--;
        if (living <= limit) {
            if (limit > 0) {
                lua_pushinteger(L, i);
                return 1;
            } else return 0;
        }
    }
    lua_settop(L, count + 1);
    return lua_iyield(L, 0, count);
}

static int parallel_waitForAny(lua_State *L) {
    lua_State *tmp;
    int count = lua_gettop(L);
    if (lua_icontext(L)) return runUntilLimit(L, lua_icontext(L), lua_icontext(L) - 1);
    for (int i = 1; i <= count; i++) {
        luaL_checkanyfunction(L, i);
        tmp = lua_newthread(L);
        lua_pushvalue(L, i);
        lua_xmove(L, tmp, 1);
        lua_replace(L, i);
    }
    lua_createtable(L, count, 0);
    return runUntilLimit(L, count, count - 1);
}

static int parallel_waitForAll(lua_State *L) {
    lua_State *tmp;
    int count = lua_gettop(L);
    if (lua_icontext(L)) return runUntilLimit(L, lua_icontext(L), 0);
    for (int i = 1; i <= count; i++) {
        luaL_checkanyfunction(L, i);
        tmp = lua_newthread(L);
        lua_pushvalue(L, i);
        lua_xmove(L, tmp, 1);
        lua_replace(L, i);
    }
    return runUntilLimit(L, count, 0);
}

const luaL_Reg parallel_lib[] = {
    {"waitForAny", parallel_waitForAny},
    {"waitForAll", parallel_waitForAll},
    {NULL, NULL}
};