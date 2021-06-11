#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lauxlib.h>
#include <lualib.h>
#include "bit.h"
#include "fs.h"
#include "os.h"
#include "peripheral.h"
#include "term.h"
#include "redstone.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/sync.h"
#include "pico/time.h"
#include "bsp/board.h"
#include "tusb.h"
#include "pico/printf.h"
extern const luaL_Reg parallel_lib[];

extern void inputCore();
extern lua_State *paramQueue;
extern critical_section_t paramQueueLock;
extern const char * standaloneBIOS;
extern size_t standaloneBIOSSize;
size_t total_alloc = 0;
bool collecting = false;

void * allocf(void* ud, void* ptr, size_t osize, size_t nsize) {
    /*if (total_alloc > 175000 && !collecting) {
        printf("memory warning: %d ", total_alloc);
        //collecting = true;
        //lua_gc(*(lua_State**)ud, total_alloc > 170000 ? LUA_GCCOLLECT : LUA_GCSTEP, 0);
        //collecting = false;
        printf("%d\r\n", total_alloc);
    }*/
    if (osize == 0) {
        if (total_alloc > 200000) return NULL;
        total_alloc += nsize;
        ptr = malloc(nsize);
        //if (ptr) printf("+ allocated %d bytes at %p\n", nsize, ptr);
        return ptr;
    } else if (nsize == 0) {
        total_alloc -= osize;
        free(ptr);
        //printf("- freed %d bytes at %p\n", osize, ptr);
        return NULL;
    } else {
        if (total_alloc > 225000 && nsize > osize) return NULL;
        total_alloc -= osize;
        total_alloc += nsize;
        //printf("- freed %d bytes at %p\n", osize, ptr);
        ptr = realloc(ptr, nsize);
        //if (ptr) printf("+ allocated %d bytes at %p\n", nsize, ptr);
        return ptr;
    }
}

static int panicf (lua_State *L) {
  (void)L;  /* to avoid warnings */
  printf("PANIC: unprotected error in call to Lua API (%s)\n",
                   lua_tostring(L, -1));
  return 0;
}

int main() {
    int status, result, i, narg;
    double sum;
    lua_State *L;
    lua_State *coro;

    stdout_uart_init();
    uart_set_baudrate(uart0, 115200);
    gpio_init(25);
    gpio_set_dir(25, 1);
    gpio_put(25, 1);

    board_init();
    alarm_pool_init_default();
    critical_section_init(&paramQueueLock);
    multicore_launch_core1(inputCore);
    printf("CraftOS is initializing...\n");

start:
    /*
     * All Lua contexts are held in this structure. We work with it almost
     * all the time.
     */
    L = lua_newstate(allocf, &L);
    lua_atpanic(L, panicf);
    lua_gc(L, LUA_GCSETPAUSE, 0);
    lua_gc(L, LUA_GCSETSTEPMUL, 125);
    lua_gc(L, LUA_GCSETMEMLIMIT, 200000);
    printf("before start:\t%d\n", lua_gc(L, LUA_GCCOUNT, 0));

    coro = lua_newthread(L);
    paramQueue = lua_newthread(L);

    luaL_openlibs(L);
    luaL_register_light(coro, "bit", bit_lib);
    luaL_register_light(coro, "fs", fs_lib);
    luaL_register_light(coro, "os", os_lib);
    luaL_register_light(coro, "peripheral", peripheral_lib);
    luaL_register_light(coro, "redstone", rs_lib);
    lua_getglobal(coro, "redstone");
    lua_setglobal(coro, "rs");
    luaL_register(coro, "term", term_lib);
    termInit();
    luaL_register_light(coro, "parallel", parallel_lib);
    printf("after init:\t%d\n", lua_gc(L, LUA_GCCOUNT, 0));

    lua_pushstring(L, "bios.use_multishell=false,shell.autocomplete=false");
    lua_setglobal(L, "_CC_DEFAULT_SETTINGS");
    lua_pushstring(L, "ComputerCraft 1.80 (craftos-native)");
    lua_setglobal(L, "_HOST");

    /* Set up pcall fix */
#ifndef CRAFTOS2_LUA
    status = luaL_loadstring(L, "_G.xpcall = function( _fn, _fnErrorHandler )\n\
    local typeT = type( _fn )\n\
    assert( typeT == \"function\", \"bad argument #1 to xpcall (function expected, got \"..typeT..\")\" )\n\
    local co = coroutine.create( _fn )\n\
    local tResults = { coroutine.resume( co ) }\n\
    while coroutine.status( co ) ~= \"dead\" do\n\
        tResults = { coroutine.resume( co, coroutine.yield() ) }\n\
    end\n\
    if tResults[1] == true then\n\
        return true, unpack( tResults, 2 )\n\
    else\n\
        return false, _fnErrorHandler( tResults[2] )\n\
    end\n\
end\n\
\n\
_G.pcall = function( _fn, ... )\n\
    local typeT = type( _fn )\n\
    assert( typeT == \"function\", \"bad argument #1 to pcall (function expected, got \"..typeT..\")\" )\n\
    local tArgs = { ... }\n\
    return xpcall(\n\
        function()\n\
            return _fn( unpack( tArgs ) )\n\
        end,\n\
        function( _error )\n\
            return _error\n\
        end\n\
    )\n\
end");
    if (status) {
        /* If something went wrong, error message is at the top of */
        /* the stack */
        fprintf(stdout, "Couldn't load pcall fix: %s\n", lua_tostring(L, -1));
        exit(1);
    }
    lua_call(L, 0, 0);
#endif

    /* Load the file containing the script we are going to run */
    status = luaL_loadbuffer(coro, standaloneBIOS, standaloneBIOSSize, "@bios.lua");
    if (status) {
        /* If something went wrong, error message is at the top of */
        /* the stack */
        sleep_ms(500);
        printf("Couldn't load file: %s\r\n", lua_tostring(L, -1));
        fflush(stdout);
        // uart_puts(uart0, "Couldn't load file: ");
        // uart_puts(uart0, lua_tostring(L, -1));
        // uart_puts(uart0, "\r\n");
        sleep_ms(5000);
        exit(1);
    }
    printf("after bios:\t%d\n", lua_gc(L, LUA_GCCOUNT, 0));

    /* Ask Lua to run our little script */
    status = LUA_YIELD;
    narg = 0;
    while (status == LUA_YIELD && running == 1) {
        status = lua_resume(coro, narg);
        //printf("after yield:\t%d\n", lua_gc(L, LUA_GCCOUNT, 0));
        lua_gc(L, LUA_GCCOLLECT, 0);
        //printf("after yield gc:\t%d\n", lua_gc(L, LUA_GCCOUNT, 0));
        if (running == 1 && status == LUA_YIELD) {
            if (lua_isstring(coro, -1)) narg = getNextEvent(coro, lua_tostring(coro, -1));
            else narg = getNextEvent(coro, "");
        } else if (status != 0) {
            printf("%s\n", lua_tostring(coro, -1));
            sleep_ms(5000);
            termClose();
            lua_close(L);
            exit(1);
        }
    }

    termClose();
    lua_close(L);   /* Cya, Lua */

    if (running == 2) {
        sleep_ms(1000);
        goto start;
    }

    return 0;
}
