#ifdef __cplusplus
extern "C" {
#endif
#include <lua.h>
extern int handle_close(lua_State *L);
extern int handle_readAll(lua_State *L);
extern int handle_readLine(lua_State *L);
extern int handle_readChar(lua_State *L);
extern int handle_readByte(lua_State *L);
extern int handle_writeString(lua_State *L);
extern int handle_writeLine(lua_State *L);
extern int handle_writeByte(lua_State *L);
extern int handle_flush(lua_State *L);
extern int handle_istream_close(lua_State *L);
extern int handle_istream_readAll(lua_State *L);
extern int handle_istream_readLine(lua_State *L);
extern int handle_istream_readChar(lua_State *L);
extern int handle_istream_readByte(lua_State *L);
extern int handle_istream_readAllByte(lua_State *L);
extern int handle_istream_seek(lua_State *L);

typedef struct stream {
    const char * base;
    const char * eof;
    const char * cur;
    bool open = true;
} stream_t;
#ifdef __cplusplus
}
#endif