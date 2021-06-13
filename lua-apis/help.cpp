extern "C" {
#include <lua.h>
#include <lauxlib.h>
}
#include <string.h>
#include <string>
#include "../FileEntry.hpp"

extern std::string fixpath(const char * str);
extern const FileEntry standaloneROM;
static char helpPath[64] = "/rom/help";

static int help_path(lua_State *L) {
    lua_pushstring(L, helpPath);
    return 1;
}

static int help_setPath(lua_State *L) {
    //helpPath = luaL_checkstring(L, 1);
    strncpy(helpPath, luaL_checkstring(L, 1), 64);
    return 0;
}

static int help_lookup(lua_State *L) {
    const char * topic = luaL_checkstring(L, 1);
    char * tok = strtok(helpPath, ":");
    while (tok) {
        lua_pushstring(L, tok);
        lua_pushstring(L, "/");
        lua_pushvalue(L, 1);
        lua_concat(L, 3);
        std::string path = fixpath(lua_tostring(L, -1));
        if (path.find("rom/") == 0) {
            const FileEntry * e = &standaloneROM;
            size_t start = path.find('/');
            bool found = true;
            while (start != std::string::npos) {
                if (!e->isDir) {
                    found = false;
                    break;
                }
                size_t next = path.find('/', start + 1);
                e = &(*e)[path.substr(start + 1, next - start - 1)];
                if (e == &FileEntry::NULL_ENTRY) {
                    found = false;
                    break;
                }
                start = next;
            }
            if (found && !e->isDir) {
                lua_pushstring(L, path.c_str());
                return 1;
            } else {
                path += ".txt";
                const FileEntry * e = &standaloneROM;
                size_t start = path.find('/');
                bool found = true;
                while (start != std::string::npos) {
                    if (!e->isDir) {
                        found = false;
                        break;
                    }
                    size_t next = path.find('/', start + 1);
                    e = &(*e)[path.substr(start + 1, next - start - 1)];
                    if (e == &FileEntry::NULL_ENTRY) {
                        found = false;
                        break;
                    }
                    start = next;
                }
                if (found && !e->isDir) {
                    lua_pushstring(L, path.c_str());
                    return 1;
                }
            }
        }
        lua_pop(L, 1);
        tok = strtok(helpPath, ":");
    }
    return 0;
}

#define _string 1
#define _fs 2
#define _lit_dot 3
#define _lit_txt 4
#define _tItems 5
#define _gmatch_iter 6
#define _gmatch_invar 7
#define _sPath 8
#define _tList 9
#define _n 10
#define _sFile 11
#define _tItemList 6
#define _sItem 7

static int help_topics(lua_State *L) {
    int tItem_count = 0;
    // There's too many FS calls here, so we'll just run the equivalent of the Lua code
    lua_settop(L, 0);
    if (!lua_checkstack(L, 11)) luaL_error(L, "not enough memory");
    lua_getglobal(L, "string");
    lua_getglobal(L, "fs");
    lua_pushliteral(L, ".");
    lua_pushliteral(L, ".txt");
    // Add index
    lua_createtable(L, 0, 1);
    lua_pushboolean(L, 1);
    lua_setfield(L, _tItems, "index");
    // Add topics from the path
    // initialize for loop
    lua_getfield(L, _string, "gmatch");
    lua_pushstring(L, helpPath);
    lua_pushliteral(L, "[^:]+");
    lua_call(L, 2, 3);
    // start for loop
    while (1) {
        lua_pushvalue(L, _gmatch_iter);
        lua_pushvalue(L, _gmatch_invar);
        lua_pushvalue(L, _sPath);
        lua_call(L, 2, 1);
        lua_replace(L, _sPath);
        if (lua_isnil(L, _sPath)) break;
        // for body
        lua_getfield(L, _fs, "isDir");
        lua_pushvalue(L, _sPath);
        lua_call(L, 1, 1);
        if (lua_toboolean(L, -1)) {
            lua_pop(L, 1);
            lua_getfield(L, _fs, "list");
            lua_pushvalue(L, _sPath);
            lua_call(L, 1, 1);
            lua_pushnil(L);
            while (lua_next(L, _tList)) {
                lua_getfield(L, _string, "sub");
                lua_pushvalue(L, _sFile);
                lua_pushinteger(L, 1);
                lua_pushinteger(L, 1);
                lua_call(L, 3, 1);
                lua_pushvalue(L, _lit_dot);
                if (!lua_equal(L, -2, -1)) {
                    lua_pop(L, 2);
                    lua_getfield(L, _fs, "isDir");
                    lua_getfield(L, _fs, "combine");
                    lua_pushvalue(L, _sPath);
                    lua_pushvalue(L, _sFile);
                    lua_call(L, 2, 1);
                    lua_call(L, 1, 1);
                    if (!lua_toboolean(L, -1)) {
                        lua_pop(L, 1);
                        if (lua_strlen(L, _sFile) <= 4) {
                            lua_getfield(L, _string, "sub");
                            lua_pushvalue(L, _sFile);
                            lua_pushinteger(L, -4);
                            lua_call(L, 2, 1);
                            lua_pushvalue(L, _lit_txt);
                            if (lua_equal(L, -2, -1)) {
                                lua_pop(L, 2);
                                lua_getfield(L, _string, "sub");
                                lua_pushvalue(L, _sFile);
                                lua_pushinteger(L, 1);
                                lua_pushinteger(L, -5);
                                lua_call(L, 2, 1);
                                lua_replace(L, _sFile);
                            } else lua_pop(L, 2);
                        }
                        lua_pushvalue(L, _sFile);
                        lua_pushboolean(L, 1);
                        lua_settable(L, _tItems);
                        tItem_count++;
                    } else lua_pop(L, 1);
                } else lua_pop(L, 2);
                lua_pop(L, 1);
            }
        } else lua_pop(L, 1);
    }
    lua_settop(L, _tItems);
    // Sort and return
    lua_createtable(L, tItem_count, 0);
    tItem_count = 0;
    lua_pushnil(L);
    while (lua_next(L, _tItems)) {
        lua_pop(L, 1);
        lua_pushinteger(L, ++tItem_count);
        lua_pushvalue(L, _sItem);
        lua_settable(L, _tItemList);
    }
    lua_getglobal(L, "table");
    lua_getfield(L, -1, "sort");
    lua_pushvalue(L, _tItemList);
    lua_call(L, 1, 0);
    lua_settop(L, _tItemList);
    return 1;
}

static int help_completeTopic(lua_State *L) {
    int n, i = 0;
    size_t sText_len, sTopic_len;
    const char * sText = luaL_checklstring(L, 1, &sText_len);
    lua_settop(L, 1);
    lua_pushlightfunction(L, (void*)help_topics);
    lua_call(L, 0, 1);
    lua_newtable(L);
    for (n = 1; n <= lua_objlen(L, 2); n++) {
        const char * sTopic;
        lua_rawgeti(L, 2, n);
        sTopic = lua_tolstring(L, -1, &sTopic_len);
        if (sTopic_len > sText_len && strncmp(sTopic, sText, sText_len)) {
            lua_pushstring(L, sTopic + sText_len);
            lua_rawseti(L, 3, ++i);
        }
        lua_pop(L, 1);
    }
    return 1;
}

extern const luaL_Reg help_lib[] = {
    {"path", help_path},
    {"setPath", help_setPath},
    {"lookup", help_lookup},
    {"topics", help_topics},
    {"completeTopic", help_completeTopic},
    {NULL, NULL}
};