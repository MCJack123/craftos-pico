#include <lauxlib.h>
extern luaL_Reg os_lib[];
extern int getNextEvent(lua_State *L, const char * filter);
extern int running;