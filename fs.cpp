extern "C" {
#include "fs.h"
#include <lauxlib.h>
}
#include "fs_handle.h"
#include "FileEntry.hpp"
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern FileEntry standaloneROM;

// TODO: Add actual filesystem access once SD/USB is implemented

std::string fixpath(const char * str) {
    std::string retval;
    retval.reserve(strlen(str));
    while (*str) {
        while (*str == '/') str++;
        while (*str && *str != '/') retval += *str++;
        if (*str) retval += '/';
    }
    return retval;
}

static void err(lua_State *L, std::string path, const char * err) {
    luaL_error(L, "%s: %s", path.c_str(), err);
}

static bool stringcompare(const char * a, const char * b) {
    if (a == NULL) return false;
    else if (b == NULL) return true;
    return strcmp(a, b) < 0;
}

int fs_list(lua_State *L) {
    std::string path = fixpath(luaL_checkstring(L, 1));
    if (path.find("rom") == 0) {
        FileEntry * e = &standaloneROM;
        size_t start = path.find('/');
        while (start != std::string::npos) {
            size_t next = path.find('/', start + 1);
            e = &(*e)[path.substr(start + 1, next - start - 1)];
            if (e == &FileEntry::NULL_ENTRY) err(L, path, "No such file");
            if (!e->isDir) err(L, path, "Not a directory");
            start = next;
        }
        const char ** keys = new const char*[e->size()];
        for (int i = 0; i < e->size(); i++) keys[i] = e->dir[i].name;
        std::sort(keys, keys + e->size(), stringcompare);
        lua_createtable(L, e->size(), 0);
        for (int i = 0; i < e->size(); i++) {
            lua_pushstring(L, keys[i]);
            lua_rawseti(L, -2, i + 1);
        }
        delete[] keys;
    } else {
        lua_newtable(L);
        int found = 0;
        for (const char * s = path.c_str(); *s; s++) {if (*s != '/') {found = 1; break;}}
        if (!found) {
            lua_pushstring(L, "rom");
            lua_rawseti(L, -2, 1);
        }
    }
    return 1;
}

int fs_exists(lua_State *L) {
    std::string path = fixpath(luaL_checkstring(L, 1));
    if (path.find("rom/") == 0) {
        FileEntry * e = &standaloneROM;
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
        lua_pushboolean(L, found);
    } else lua_pushboolean(L, path == "" || path == "rom");
    return 1;
}

int fs_isDir(lua_State *L) {
    std::string path = fixpath(luaL_checkstring(L, 1));
    if (path.find("rom/") == 0) {
        FileEntry * e = &standaloneROM;
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
        lua_pushboolean(L, found && e->isDir);
    } else lua_pushboolean(L, path == "" || path == "rom");
    return 1;
}

int fs_isReadOnly(lua_State *L) {
    lua_pushboolean(L, true);
    return 1;
}

int fs_getName(lua_State *L) {
    std::string path = fixpath(luaL_checkstring(L, 1));
    if (path.find('/') == std::string::npos) lua_pushvalue(L, 1);
    else lua_pushlstring(L, path.substr(path.find_last_of('/') + 1).c_str(), path.substr(path.find_last_of('/') + 1).size());
    return 1;
}

int fs_getDrive(lua_State *L) {
    std::string path = fixpath(luaL_checkstring(L, 1));
    if (path.find("rom") == 0) lua_pushstring(L, "rom");
    else lua_pushstring(L, "hdd");
    return 1;
}

int fs_getSize(lua_State *L) {
    std::string path = fixpath(luaL_checkstring(L, 1));
    if (path.find("rom/") == 0) {
        FileEntry * e = &standaloneROM;
        size_t start = path.find('/');
        bool found = true;
        while (start != std::string::npos) {
            if (!e->isDir) err(L, path, "Not a directory");
            size_t next = path.find('/', start + 1);
            e = &(*e)[path.substr(start + 1, next - start - 1)];
            if (e == &FileEntry::NULL_ENTRY) err(L, path, "No such file");
            start = next;
        }
        if (e->isDir) err(L, path, "No such file");
        lua_pushinteger(L, e->size());
    } else err(L, path, "No such file");
    return 1;
}

int fs_getFreeSpace(lua_State *L) {
    lua_pushinteger(L, 0);
    return 1;
}

int fs_makeDir(lua_State *L) {
    err(L, luaL_tostring(L, 1), "Permission denied");
    return 0;
}

int fs_move(lua_State *L) {
    err(L, luaL_tostring(L, 1), "Permission denied");
    return 0;
}

int fs_copy(lua_State *L) {
    err(L, luaL_tostring(L, 1), "Permission denied");
    return 0;
}

int fs_delete(lua_State *L) {
    err(L, luaL_tostring(L, 1), "Permission denied");
    return 0;
}

int fs_combine(lua_State *L) {
    std::string base = fixpath(luaL_checkstring(L, 1));
    for (int i = 2; i <= lua_gettop(L); i++) base += "/" + fixpath(luaL_checkstring(L, i));
    lua_pushlstring(L, base.c_str(), base.size());
    return 1;
}

int fs_open(lua_State *L) {
    std::string path = fixpath(luaL_checkstring(L, 1));
    const char * mode = luaL_checkstring(L, 2);
    if (path.find("rom/") == 0) {
        FileEntry * e = &standaloneROM;
        size_t start = path.find('/');
        bool found = true;
        while (start != std::string::npos) {
            if (!e->isDir) err(L, path, "Not a directory");
            size_t next = path.find('/', start + 1);
            e = &(*e)[path.substr(start + 1, next - start - 1)];
            if (e == &FileEntry::NULL_ENTRY) err(L, path, "No such file");
            start = next;
        }
        if (e->isDir) err(L, path, "No such file");
        
        lua_newuserdata(L, sizeof(stream_t));
        stream_t * strm = (stream_t*)lua_touserdata(L, -1);
        strm->base = strm->cur = e->data;
        strm->eof = e->data + e->size();
        strm->open = true;

        int pos = lua_gettop(L);
        lua_newtable(L);
        lua_pushstring(L, "close");
        lua_pushvalue(L, pos);
        lua_pushcclosure(L, handle_istream_close, 1);
        lua_settable(L, -3);
        if (strcmp(mode, "r") == 0) {
            lua_pushstring(L, "readAll");
            lua_pushvalue(L, pos);
            lua_pushcclosure(L, handle_istream_readAll, 1);
            lua_settable(L, -3);

            lua_pushstring(L, "readLine");
            lua_pushvalue(L, pos);
            lua_pushcclosure(L, handle_istream_readLine, 1);
            lua_settable(L, -3);

            lua_pushstring(L, "read");
            lua_pushvalue(L, pos);
            lua_pushcclosure(L, handle_istream_readChar, 1);
            lua_settable(L, -3);
        } else if (strcmp(mode, "rb") == 0) {
            lua_pushstring(L, "read");
            lua_pushvalue(L, pos);
            lua_pushcclosure(L, handle_istream_readByte, 1);
            lua_settable(L, -3);

            lua_pushstring(L, "readAll");
            lua_pushvalue(L, pos);
            lua_pushcclosure(L, handle_istream_readAllByte, 1);
            lua_settable(L, -3);

            lua_pushstring(L, "seek");
            lua_pushvalue(L, pos);
            lua_pushcclosure(L, handle_istream_seek, 1);
            lua_settable(L, -3);
        } else if (strcmp(mode, "w") == 0 || strcmp(mode, "a") == 0 || strcmp(mode, "wb") == 0 || strcmp(mode, "ab") == 0) {
            err(L, path, "Permission denied");
        } else {
            err(L, mode, "Invalid mode");
        }
    } else err(L, path, "No such file");
    return 1;
}

int fs_find(lua_State *L) {
    lua_newtable(L);
    return 1;
}

int fs_getDir(lua_State *L) {
    std::string path = fixpath(luaL_checkstring(L, 1));
    if (path.find('/') == std::string::npos) lua_pushvalue(L, 1);
    else lua_pushlstring(L, path.substr(0, path.find_last_of('/')).c_str(), path.substr(0, path.find_last_of('/')).size());
    return 1;
}

const char * fs_keys[16] = {
    "list",
    "exists",
    "isDir",
    "isReadOnly",
    "getName",
    "getDrive",
    "getSize",
    "getFreeSpace",
    "makeDir",
    "move",
    "copy",
    "delete",
    "combine",
    "open",
    "find",
    "getDir"
};

lua_CFunction fs_values[16] = {
    fs_list,
    fs_exists,
    fs_isDir,
    fs_isReadOnly,
    fs_getName,
    fs_getDrive,
    fs_getSize,
    fs_getFreeSpace,
    fs_makeDir,
    fs_move,
    fs_copy,
    fs_delete,
    fs_combine,
    fs_open,
    fs_find,
    fs_getDir
};

library_t fs_lib = {"fs", 16, fs_keys, fs_values};