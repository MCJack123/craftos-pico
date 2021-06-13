#include <lua.h>
#include <lauxlib.h>
#include <math.h>

typedef lua_Number luaR_result;

// A number entry in the read only table
typedef struct
{
  const char *name;
  lua_Number value;
} luaR_value_entry;

// A mapping between table name and its entries
typedef struct
{
  const char *name;
  const luaL_reg *pfuncs;
  const luaR_value_entry *pvalues;
} luaR_table;

extern const luaR_table lua_rotable[];

static void newvector(lua_State *L, lua_Number x, lua_Number y, lua_Number z);

static int vector_add(lua_State *L) {
    lua_settop(L, 2);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 2, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 2, "y");
    lua_getfield(L, 1, "z");
    lua_getfield(L, 2, "z");
    newvector(L,
        lua_tonumber(L, 3) + lua_tonumber(L, 4),
        lua_tonumber(L, 5) + lua_tonumber(L, 6),
        lua_tonumber(L, 7) + lua_tonumber(L, 8)
    );
    return 1;
}

static int vector_sub(lua_State *L) {
    lua_settop(L, 2);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 2, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 2, "y");
    lua_getfield(L, 1, "z");
    lua_getfield(L, 2, "z");
    newvector(L,
        lua_tonumber(L, 3) - lua_tonumber(L, 4),
        lua_tonumber(L, 5) - lua_tonumber(L, 6),
        lua_tonumber(L, 7) - lua_tonumber(L, 8)
    );
    return 1;
}

static int vector_mul(lua_State *L) {
    lua_settop(L, 2);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 1, "z");
    newvector(L,
        lua_tonumber(L, 3) * lua_tonumber(L, 2),
        lua_tonumber(L, 4) * lua_tonumber(L, 2),
        lua_tonumber(L, 5) * lua_tonumber(L, 2)
    );
    return 1;
}

static int vector_div(lua_State *L) {
    lua_settop(L, 2);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 1, "z");
    newvector(L,
        lua_tonumber(L, 3) / lua_tonumber(L, 2),
        lua_tonumber(L, 4) / lua_tonumber(L, 2),
        lua_tonumber(L, 5) / lua_tonumber(L, 2)
    );
    return 1;
}

static int vector_unm(lua_State *L) {
    lua_settop(L, 1);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 1, "z");
    newvector(L,
        -lua_tonumber(L, 3),
        -lua_tonumber(L, 4),
        -lua_tonumber(L, 5)
    );
    return 1;
}

static int vector_dot(lua_State *L) {
    lua_settop(L, 2);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 2, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 2, "y");
    lua_getfield(L, 1, "z");
    lua_getfield(L, 2, "z");
    lua_pushnumber(L,
        lua_tonumber(L, 3) * lua_tonumber(L, 4) +
        lua_tonumber(L, 5) * lua_tonumber(L, 6) +
        lua_tonumber(L, 7) * lua_tonumber(L, 8)
    );
    return 1;
}

static int vector_cross(lua_State *L) {
    lua_settop(L, 2);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 2, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 2, "y");
    lua_getfield(L, 1, "z");
    lua_getfield(L, 2, "z");
    newvector(L,
        lua_tonumber(L, 5) * lua_tonumber(L, 8) - lua_tonumber(L, 7) * lua_tonumber(L, 6),
        lua_tonumber(L, 7) * lua_tonumber(L, 4) - lua_tonumber(L, 3) * lua_tonumber(L, 8),
        lua_tonumber(L, 3) * lua_tonumber(L, 6) - lua_tonumber(L, 5) * lua_tonumber(L, 4)
    );
    return 1;
}

static int vector_length(lua_State *L) {
    lua_settop(L, 1);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 1, "z");
    lua_pushnumber(L, sqrt(
        lua_tonumber(L, 3)*lua_tonumber(L, 3) +
        lua_tonumber(L, 4)*lua_tonumber(L, 4) +
        lua_tonumber(L, 5)*lua_tonumber(L, 5)
    ));
    return 1;
}

static int vector_normalize(lua_State *L) {
    lua_Number len;
    lua_settop(L, 2);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 1, "z");
    len = 1.0 / sqrt(
        lua_tonumber(L, 3)*lua_tonumber(L, 3) +
        lua_tonumber(L, 4)*lua_tonumber(L, 4) +
        lua_tonumber(L, 5)*lua_tonumber(L, 5)
    );
    newvector(L,
        lua_tonumber(L, 3) * len,
        lua_tonumber(L, 4) * len,
        lua_tonumber(L, 5) * len
    );
    return 1;
}

static int vector_round(lua_State *L) {
    double tol;
    tol = luaL_optnumber(L, 2, 1.0);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 1, "z");
    newvector(L,
        floor((lua_tonumber(L, 3) + (tol * 0.5)) / tol) * tol,
        floor((lua_tonumber(L, 4) + (tol * 0.5)) / tol) * tol,
        floor((lua_tonumber(L, 5) + (tol * 0.5)) / tol) * tol
    );
    return 1;
}

static int vector_tostring(lua_State *L) {
    lua_getfield(L, 1, "x");
    lua_pushliteral(L, ",");
    lua_getfield(L, 1, "y");
    lua_pushliteral(L, ",");
    lua_getfield(L, 1, "z");
    lua_concat(L, 5);
    return 1;
}

static void newvector(lua_State *L, lua_Number x, lua_Number y, lua_Number z) {
    lua_createtable(L, 0, 3);
    lua_pushnumber(L, x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, y);
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, z);
    lua_setfield(L, -2, "z");
    lua_getfield(L, LUA_REGISTRYINDEX, "vector.mt");
    if (lua_isnil(L, -1)) {
        const luaR_table * t = lua_rotable;
        for (; t->name; t++);
        t += 1;
        lua_pop(L, 1);
        lua_createtable(L, 0, 7);
        lua_pushlightfunction(L, (void*)vector_add);
        lua_setfield(L, -2, "__add");
        lua_pushlightfunction(L, (void*)vector_sub);
        lua_setfield(L, -2, "__sub");
        lua_pushlightfunction(L, (void*)vector_mul);
        lua_setfield(L, -2, "__mul");
        lua_pushlightfunction(L, (void*)vector_div);
        lua_setfield(L, -2, "__div");
        lua_pushlightfunction(L, (void*)vector_unm);
        lua_setfield(L, -2, "__unm");
        lua_pushlightfunction(L, (void*)vector_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushrotable(L, (void*)(t - lua_rotable + 1));
        lua_setfield(L, -2, "__index");
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, "vector.mt");
    }
    lua_setmetatable(L, -2);
}

static int vector_new(lua_State *L) {
    lua_settop(L, 3);
    newvector(L, lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3));
    return 1;
}

const luaL_Reg vector_mt[] = {
    {"add", vector_add},
    {"sub", vector_sub},
    {"mul", vector_mul},
    {"div", vector_div},
    {"unm", vector_unm},
    {"dot", vector_dot},
    {"cross", vector_cross},
    {"length", vector_length},
    {"normalize", vector_normalize},
    {"round", vector_round},
    {"tostring", vector_tostring},
    {NULL, NULL}
};

const luaL_Reg vector_lib[] = {
    {"new", vector_new},
    {NULL, NULL}
};