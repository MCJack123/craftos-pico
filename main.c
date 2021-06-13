#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lauxlib.h>
#include <lualib.h>
#include "os.h"
#include "term.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/sync.h"
#include "pico/time.h"
#include "bsp/board.h"
#include "tusb.h"
#include "pico/printf.h"
#include "craftos2-lua/src/lstate.h"

extern void inputCore();
extern int luaopen_term(lua_State *L);
extern lua_State *paramQueue;
extern critical_section_t paramQueueLock;
extern const char * standaloneBIOS;
extern const size_t standaloneBIOSSize;
extern char __StackLimit;
size_t* total_alloc;
bool collecting = false;
ptrdiff_t heap_size;
void * lastAllocation;

/*void * allocf(void* ud, void* ptr, size_t osize, size_t nsize) {
    if (*total_alloc > 200000 && !collecting) {
        //printf("memory warning: %d ", total_alloc);
        collecting = true;
        lua_gc(*(lua_State**)ud, LUA_GCCOLLECT, 0);
        collecting = false;
        //printf("%d\r\n", total_alloc);
    }
    if (nsize == 0) {
        free(ptr);
        total_alloc -= osize;
        //printf("- freed %d bytes at %p\n", osize, ptr);
        return NULL;
    } else if (osize == 0) {
        //if (total_alloc > 200000) return NULL;
        ptr = malloc(nsize);
        if (ptr) {total_alloc += nsize; lastAllocation = ptr;}
        else
            printf("Error allocating! Current size is %d\n", total_alloc);
        //if (ptr) printf("+ allocated %d bytes at %p\n", nsize, ptr);
        return ptr;
    } else {
        //if (total_alloc > 225000 && nsize > osize) return NULL;
        //printf("- freed %d bytes at %p\n", osize, ptr);
        ptr = realloc(ptr, nsize);
        if (ptr) {
            total_alloc -= osize;
            total_alloc += nsize;
        }
        //if (ptr) printf("+ allocated %d bytes at %p\n", nsize, ptr);
        return ptr;
    }
}

static int panicf (lua_State *L) {
  (void)L;  /* to avoid warnings * /
  printf("PANIC: unprotected error in call to Lua API (%s)\n",
                   lua_tostring(L, -1));
  return 0;
}*/

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
    //L = lua_newstate(allocf, &L);
    L = luaL_newstate();
    total_alloc = &L->l_G->totalbytes;
    heap_size = &__StackLimit - (char*)L;
    //lua_atpanic(L, panicf);
    lua_gc(L, LUA_GCSETPAUSE, 0);
    lua_gc(L, LUA_GCSETSTEPMUL, 1250);
    //lua_gc(L, LUA_GCSETMEMLIMIT, (heap_size - 2048) / 1024);
    lua_gc(L, LUA_GCSETMEMLIMIT, 215);
    printf("before start:\t%d\n", lua_gc(L, LUA_GCCOUNT, 0));

    coro = lua_newthread(L);
    paramQueue = lua_newthread(L);

    luaL_openlibs(L);
    luaL_register_light(L, "os", os_lib);
    lua_pushcfunction(L, luaopen_term);
    lua_call(L, 0, 0);
    termInit();
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, "_settings");
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
            if (lua_gettop(coro) && lua_isstring(coro, -1)) narg = getNextEvent(coro, lua_tostring(coro, -1));
            else narg = getNextEvent(coro, "");
        } else if (status != 0) {
            printf("%s\n", lua_tostring(coro, -1));
            sleep_ms(5000);
            termClose();
            lua_close(L);
            exit(1);
        }
    }

    printf("Closing session.\n");
    termClose();
    lua_close(L);   /* Cya, Lua */

    if (running == 2) {
        sleep_ms(1000);
        goto start;
    }

    return 0;
}
