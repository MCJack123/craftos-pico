#include "os.h"
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "pico/sync.h"
#include "pico/time.h"
#include "pico/stdio_uart.h"

int running = 1;
const char * label;
bool label_defined = false;
lua_State * paramQueue;
critical_section_t paramQueueLock;

static const unsigned char keymap[] = {0, 0, 0, 0, 0, 0, 0, 0, 14, 15, 28, 0, 0, 28, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                       57, 2, 40, 4, 5, 6, 8, 40, 10, 11, 9, 13, 51, 12, 52, 53, 11, 2, 3, 4, 5, 6, 7, 8, 9, 10, 39, 39, 51, 13, 52, 53,
                                       3, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44, 26, 43, 27, 7, 12,
                                       41, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44, 26, 43, 27, 41, 14};

int getNextEvent(lua_State *L, const char * filter) {
    int count;
    lua_State *param;
    char tmp[2] = {0, 0};
    absolute_time_t timeout_time = make_timeout_time_us(500);
    int finished = 1;
    while (finished) {
        do {
            critical_section_enter_blocking(&paramQueueLock);
            while (uart_is_readable(uart0)) {
                uart_read_blocking(uart0, tmp, 1);
                if (!tmp[0] || tmp[0] > 127 || !keymap[tmp[0]]) continue;
                param = lua_newthread(paramQueue);
                lua_pushstring(param, "key");
                lua_pushinteger(param, keymap[tmp[0]]);
                lua_pushboolean(param, 0);
                if (tmp[0] >= 32 && tmp[0] < 127) {
                    param = lua_newthread(paramQueue);
                    lua_pushstring(param, "char");
                    lua_pushstring(param, tmp);
                }
            }
            if (lua_gettop(paramQueue)) {finished = 0; break;}
            critical_section_exit(&paramQueueLock);
        } while (best_effort_wfe_or_timeout(timeout_time));
    }
    param = lua_tothread(paramQueue, 1);
    if (param == NULL) {critical_section_exit(&paramQueueLock); return 0;}
    count = lua_gettop(param);
    if (!lua_checkstack(L, count + 1)) {
        printf("Could not allocate enough space in the stack for %d elements, skipping event\n", count);
        critical_section_exit(&paramQueueLock);
        return 0;
    }
    lua_xmove(param, L, count);
    lua_remove(paramQueue, 1);
    lua_gc(L, LUA_GCCOLLECT, 0);
    critical_section_exit(&paramQueueLock);
    return count;
}

int os_getComputerID(lua_State *L) {lua_pushinteger(L, 0); return 1;}
int os_getComputerLabel(lua_State *L) {
    if (!label_defined) return 0;
    lua_pushstring(L, label);
    return 1;
}

int os_setComputerLabel(lua_State *L) {
    label = lua_tostring(L, 1);
    label_defined = true;
    return 0;
}

int os_queueEvent(lua_State *L) {
    lua_State *param;
    int count = lua_gettop(L);
    critical_section_enter_blocking(&paramQueueLock);
    param = lua_newthread(paramQueue);
    lua_xmove(L, param, count);
    critical_section_exit(&paramQueueLock);
    return 0;
}

int os_clock(lua_State *L) {
    #ifdef UPTIME
    struct sysinfo info;
    sysinfo(&info);
    lua_pushinteger(L, info.uptime);
    #else
    lua_pushinteger(L, clock() / CLOCKS_PER_SEC);
    #endif
    return 1;
}

static int64_t timer(alarm_id_t id, void* ud) {
    lua_State *param;
    critical_section_enter_blocking(&paramQueueLock);
    param = lua_newthread(paramQueue);
    lua_pushstring(param, (const char *)ud);
    lua_pushinteger(param, id);
    critical_section_exit(&paramQueueLock);
    __sev();
    return 0;
}

int os_startTimer(lua_State *L) {
    lua_pushinteger(L, add_alarm_in_ms(luaL_checknumber(L, 1) * 1000, timer, "timer", 1));
    return 1;
}

int os_cancelTimer(lua_State *L) {
    cancel_alarm(luaL_checkinteger(L, 1));
    return 0;
}

int os_time(lua_State *L) {
    struct tm rightNow;
    time_t t;
    int hour, minute, second;
    const char * type = "ingame";
    if (lua_gettop(L) > 0) type = lua_tostring(L, 1);
    t = time(NULL);
    if (strcmp(type, "utc") == 0) rightNow = *gmtime(&t);
    else rightNow = *localtime(&t);
    hour = rightNow.tm_hour;
    minute = rightNow.tm_min;
    second = rightNow.tm_sec;
    lua_pushnumber(L, (double)hour + ((double)minute/60.0) + ((double)second/3600.0));
    return 1;
}

int os_epoch(lua_State *L) {
    time_t t;
    struct tm rightNow;
    int hour, minute, second;
    double m_time, m_day;
    const char * type = "ingame";
    if (lua_gettop(L) > 0) type = lua_tostring(L, 1);
    if (strcmp(type, "utc") == 0) {
        lua_pushinteger(L, (long long)time(NULL) * 1000LL);
    } else if (strcmp(type, "local") == 0) {
        t = time(NULL);
        lua_pushinteger(L, (long long)mktime(localtime(&t)) * 1000LL);
    } else {
        t = time(NULL);
        rightNow = *localtime(&t);
        hour = rightNow.tm_hour;
        minute = rightNow.tm_min;
        second = rightNow.tm_sec;
        m_time = (double)hour + ((double)minute/60.0) + ((double)second/3600.0);
        m_day = rightNow.tm_yday;
        lua_pushinteger(L, m_day * 86400000 + (int) (m_time * 3600000.0f));
    }
    return 1;
}

int os_day(lua_State *L) {
    time_t t;
    struct tm rightNow;
    const char * type = "ingame";
    if (lua_gettop(L) > 0) type = lua_tostring(L, 1);
    t = time(NULL);
    if (strcmp(type, "ingame") == 0) {
        rightNow = *localtime(&t);
        lua_pushinteger(L, rightNow.tm_yday);
        return 1;
    } else if (strcmp(type, "local")) t = mktime(localtime(&t));
    lua_pushinteger(L, t/(60*60*24));
    return 1;
}

int os_setAlarm(lua_State *L) {
    // TODO: make this do what it's supposed to
    lua_pushinteger(L, add_alarm_in_ms(luaL_checknumber(L, 1) * 1000, timer, "alarm", 1));
    return 1;
}

int os_cancelAlarm(lua_State *L) {
    cancel_alarm(luaL_checkinteger(L, 1));
    return 0;
}

int os_shutdown(lua_State *L) {
    running = 0;
    //reboot(LINUX_REBOOT_CMD_POWER_OFF);
    return 0;
}

int os_reboot(lua_State *L) {
    running = 2;
    //reboot(LINUX_REBOOT_CMD_RESTART);
    return 0;
}

const char * os_keys[] = {
    "getComputerID",
    "getComputerLabel",
    "setComputerLabel",
    "queueEvent",
    "clock",
    "startTimer",
    "cancelTimer",
    "time",
    "epoch",
    "day",
    "setAlarm",
    "cancelAlarm",
    "shutdown",
    "reboot",
};

lua_CFunction os_values[] = {
    os_getComputerID,
    os_getComputerLabel,
    os_setComputerLabel,
    os_queueEvent,
    os_clock,
    os_startTimer,
    os_cancelTimer,
    os_time,
    os_epoch,
    os_day,
    os_setAlarm,
    os_cancelAlarm,
    os_shutdown,
    os_reboot,
};

library_t os_lib = {"os", 14, os_keys, os_values};
