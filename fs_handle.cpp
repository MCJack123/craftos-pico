#include "fs_handle.h"
extern "C" {
#include <lauxlib.h>
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>

extern "C" {

/*int handle_close(lua_State *L) {
    fclose((FILE*)lua_touserdata(L, lua_upvalueindex(1)));
    return 0;
}

char checkChar(char c) {
    if (c == 127) return '?';
    if ((c >= 32 && c < 127) || c == '\n' || c == '\t' || c == '\r') return c;
    else return '?';
}

int handle_readAll(lua_State *L) {
    long pos, size;
    char * retval;
    int i;
    FILE * fp = (FILE*)lua_touserdata(L, lua_upvalueindex(1));
    if (feof(fp)) return 0;
    pos = ftell(fp);
    fseek(fp, 0, SEEK_END);
    size = ftell(fp) - pos;
    retval = (char*)malloc(size + 1);
    memset(retval, 0, size + 1);
    fseek(fp, pos, SEEK_SET);
    for (i = 0; !feof(fp) && i < size; i++)
        retval[i] = checkChar(fgetc(fp));
    lua_pushstring(L, retval);
    return 1;
}

int handle_readLine(lua_State *L) {
    int i;
    char * retval;
    long size;
    FILE * fp = (FILE*)lua_touserdata(L, lua_upvalueindex(1));
    if (feof(fp)) return 0;
    size = 0;
    while (fgetc(fp) != '\n' && !feof(fp)) size++;
    fseek(fp, 0 - size - 1, SEEK_CUR);
    retval = (char*)malloc(size + 1);
    for (i = 0; i < size; i++) retval[i] = checkChar(fgetc(fp));
    fgetc(fp);
    lua_pushstring(L, retval);
    return 1;
}

int handle_readChar(lua_State *L) {
    char retval[2];
    FILE * fp = (FILE*)lua_touserdata(L, lua_upvalueindex(1));
    if (feof(fp)) return 0;
    retval[0] = checkChar(fgetc(fp));
    lua_pushstring(L, retval);
    return 1;
}

int handle_readByte(lua_State *L) {
    char retval;
    FILE * fp = (FILE*)lua_touserdata(L, lua_upvalueindex(1));
    if (feof(fp)) return 0;
    retval = fgetc(fp);
    lua_pushinteger(L, retval);
    return 1;
}

int handle_writeString(lua_State *L) {
    FILE * fp;
    const char * str = lua_tostring(L, 1);
    fp = (FILE*)lua_touserdata(L, lua_upvalueindex(1));
    fwrite(str, strlen(str), 1, fp);
    return 0;
}

int handle_writeLine(lua_State *L) {
    FILE * fp;
    const char * str = lua_tostring(L, 1);
    fp = (FILE*)lua_touserdata(L, lua_upvalueindex(1));
    fwrite(str, strlen(str), 1, fp);
    fputc('\n', fp);
    return 0;
}

int handle_writeByte(lua_State *L) {
    FILE * fp;
    const char b = lua_tointeger(L, 1) & 0xFF;
    fp = (FILE*)lua_touserdata(L, lua_upvalueindex(1));
    fputc(b, fp);
    return 0;
}

int handle_flush(lua_State *L) {
    fflush((FILE*)lua_touserdata(L, lua_upvalueindex(1)));
    return 0;
}*/

std::string makeASCIISafe(const char * retval, size_t len) {
    /*std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wstr;
    wstr = converter.from_bytes(retval, retval + len);
    std::string out;
    for (wchar_t c : wstr) {if (c < 256) out += (char)c; else out += '?';}
    return out;*/
    std::string out;
    for (int i = 0; i < len; i++) {
        if (retval[i] > 127) out += '?';
        else out += retval[i];
    }
    return out;
}

int handle_istream_close(lua_State *L) {
    lua_gc(L, LUA_GCCOLLECT, 0);
    if (!((stream_t*)lua_touserdata(L, lua_upvalueindex(1)))->open) {
        //if (!lua_istable(L, 1)) luaL_error(L, "attempt to use a closed file");
        return 0;
    }
    ((stream_t*)lua_touserdata(L, lua_upvalueindex(1)))->open = false;
    return 0;
}

int handle_istream_readAll(lua_State *L) {
    return handle_istream_readAllByte(L);
    stream_t * fp = (stream_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (!fp->open) return luaL_error(L, "attempt to use a closed file");
    if (fp->cur >= fp->eof) return 0;
    const long size = fp->eof - fp->cur;
    char * retval = new char[size + 1];
    memset(retval, 0, size + 1);
    int i;
    for (i = 0; fp->cur < fp->eof && i < size; i++) {
        int c = *fp->cur++;
        if (fp->cur >= fp->eof) c = '\n';
        if (c == '\n' && (i > 0 && retval[i-1] == '\r')) retval[--i] = '\n';
        else retval[i] = (char)c;
    }
    /*const std::string out = makeASCIISafe(retval, i - (i == size ? 0 : 1));
    lua_pushlstring(L, out.c_str(), out.length());*/
    lua_pushlstring(L, retval, i - (i == size ? 0 : 1));
    delete[] retval;
    return 1;
}

int handle_istream_readLine(lua_State *L) {
    stream_t * fp = (stream_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (!fp->open) return luaL_error(L, "attempt to use a closed file");
    if (fp->cur >= fp->eof) return 0;
    std::string retval;
    while (*fp->cur != '\n' && fp->cur < fp->eof) retval += *fp->cur++;
    if (retval.empty() && fp->cur >= fp->eof) return 0;
    size_t len = retval.length() - (retval[retval.length()-1] == '\n' && !lua_toboolean(L, 1));
    if (len > 0 && retval[len-1] == '\r') {if (lua_toboolean(L, 1)) {retval[len] = '\0'; retval[--len] = '\n';} else retval[--len] = '\0';}
    const std::string out = lua_toboolean(L, lua_upvalueindex(2)) ? std::string(retval, 0, len) : makeASCIISafe(retval.c_str(), len);
    lua_pushlstring(L, out.c_str(), out.length());
    return 1;
}

int handle_istream_readChar(lua_State *L) {
    stream_t * fp = (stream_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (!fp->open) return luaL_error(L, "attempt to use a closed file");
    if ((fp->cur >= fp->eof)) return 0;
    std::string retval;
    for (int i = 0; i < luaL_optinteger(L, 1, 1) && !(fp->cur >= fp->eof); i++) {
        uint32_t codepoint;
        const int c = *fp->cur++;
        if (c == EOF) break;
        else if (c > 0x7F) {
            if (c & 64) {
                const int c2 = *fp->cur++;
                if (c2 == EOF) {retval += '?'; break;}
                else if (c2 < 0x80 || c2 & 64) codepoint = 1U<<31;
                else if (c & 32) {
                    const int c3 = *fp->cur++;
                    if (c3 == EOF) {retval += '?'; break;}
                    else if (c3 < 0x80 || c3 & 64) codepoint = 1U<<31;
                    else if (c & 16) {
                        if (c & 8) codepoint = 1U<<31;
                        else {
                            const int c4 = *fp->cur++;
                            if (c4 == EOF) {retval += '?'; break;}
                            else if (c4 < 0x80 || c4 & 64) codepoint = 1U<<31;
                            else codepoint = ((c & 0x7) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
                        }
                    } else codepoint = ((c & 0xF) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
                } else codepoint = ((c & 0x1F) << 6) | (c2 & 0x3F);
            } else codepoint = 1U<<31;
        } else codepoint = (unsigned char)c;
        if (codepoint > 255) retval += '?';
        else {
            if (codepoint == '\r') {
                const int nextc = *fp->cur++;
                if (nextc == '\n') codepoint = nextc;
                else fp->cur--;
            }
            retval += (char)codepoint;
        }
    }
    lua_pushlstring(L, retval.c_str(), retval.length());
    return 1;
}

int handle_istream_readByte(lua_State *L) {
    stream_t * fp = (stream_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (!fp->open) return luaL_error(L, "attempt to use a closed file");
    if ((fp->cur >= fp->eof)) return 0;
    if (lua_isnumber(L, 1)) {
        const size_t s = lua_tointeger(L, 1);
        if (s == 0) {
            if (fp->cur >= fp->eof) return 0;
            lua_pushstring(L, "");
            return 1;
        }
        const size_t actual = fp->cur + s >= fp->eof ? fp->eof - fp->cur : s;
        char* retval = new char[actual];
        memcpy(retval, fp->cur, actual);
        fp->cur += actual;
        if (actual == 0) {delete[] retval; return 0;}
        lua_pushlstring(L, retval, actual);
        delete[] retval;
    } else {
        const int retval = *fp->cur++;
        if (fp->cur >= fp->eof) return 0;
        lua_pushinteger(L, (unsigned char)retval);
    }
    return 1;
}

int handle_istream_readAllByte(lua_State *L) {
    stream_t * fp = (stream_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (!fp->open) return luaL_error(L, "attempt to use a closed file");
    if ((fp->cur >= fp->eof)) return 0;
    lua_pushlstring(L, fp->cur, fp->eof - fp->cur);
    return 1;
}

int handle_istream_seek(lua_State *L) {
    stream_t * fp = (stream_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (!fp->open) return luaL_error(L, "attempt to use a closed file");
    const char * whence = luaL_optstring(L, 1, "cur");
    const size_t offset = luaL_optinteger(L, 2, 0);
    if (strcmp(whence, "set") == 0) fp->cur = fp->base + offset;
    else if (strcmp(whence, "cur") == 0) fp->cur += offset;
    else if (strcmp(whence, "end") == 0) fp->cur = fp->eof - offset;
    else return luaL_error(L, "bad argument #1 to 'seek' (invalid option '%s')", whence);
    lua_pushinteger(L, fp->cur - fp->base);
    return 1;
}

}
