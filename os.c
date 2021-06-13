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

struct key_event {
    unsigned char flags;
    unsigned char key;
};

struct mouse_event {
    unsigned char flags; // top 2 bits = type, bottom 6 bits = button/direction
    unsigned char x;
    unsigned char y;
};

struct event {
    struct event * next;
    unsigned char type; // 0 = key, 1 = mouse, 2 = timer, 3 = alarm, 255 = Lua
    union {
        struct key_event key;
        struct mouse_event mouse;
        alarm_id_t timer;
    };
};

struct event * eventQueue_front = NULL;
struct event * eventQueue_back = NULL;
lua_State * paramQueue;
critical_section_t paramQueueLock;

static const unsigned char keymap[] = {0, 0, 0, 0, 0, 0, 0, 0, 14, 15, 28, 0, 0, 28, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                       57, 2, 40, 4, 5, 6, 8, 40, 10, 11, 9, 13, 51, 12, 52, 53, 11, 2, 3, 4, 5, 6, 7, 8, 9, 10, 39, 39, 51, 13, 52, 53,
                                       3, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44, 26, 43, 27, 7, 12,
                                       41, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44, 26, 43, 27, 41, 14};

static const char * keyevents[] = {"key", "key_up", "char"};
static const char * mouseevents[] = {"mouse_click", "mouse_up", "mouse_drag", "mouse_scroll"};

int getNextEvent(lua_State *L, const char * filter) {
    int count;
    lua_State *param;
    struct event * ev;
    char tmp[2] = {0, 0};
    absolute_time_t timeout_time = make_timeout_time_us(500);
    while (!eventQueue_front) {
        do {
            critical_section_enter_blocking(&paramQueueLock);
            while (uart_is_readable(uart0)) {
                uart_read_blocking(uart0, tmp, 1);
                if (!tmp[0] || tmp[0] > 127 || !keymap[tmp[0]]) continue;
                ev = malloc(sizeof(struct event));
                ev->next = NULL;
                ev->type = 0;
                ev->key.key = keymap[tmp[0]];
                ev->key.flags = 0;
                if (eventQueue_back != NULL) eventQueue_back->next = ev;
                eventQueue_back = ev;
                if (eventQueue_front == NULL) eventQueue_front = ev;
                if (tmp[0] >= 32 && tmp[0] < 127) {
                    ev = malloc(sizeof(struct event));
                    ev->next = NULL;
                    ev->type = 0;
                    ev->key.key = tmp[0];
                    ev->key.flags = 2;
                    if (eventQueue_back != NULL) eventQueue_back->next = ev;
                    eventQueue_back = ev;
                    if (eventQueue_front == NULL) eventQueue_front = ev;
                }
            }
            if (eventQueue_front) break;
            critical_section_exit(&paramQueueLock);
        } while (best_effort_wfe_or_timeout(timeout_time));
    }
    ev = eventQueue_front;
    eventQueue_front = ev->next;
    if (eventQueue_front == NULL) eventQueue_back = NULL;
    switch (ev->type) {
    case 0:
        lua_pushstring(L, keyevents[ev->key.flags & 3]);
        if (ev->key.flags & 2) lua_pushlstring(L, &ev->key.key, 1);
        else lua_pushinteger(L, ev->key.key);
        if ((ev->key.flags & 3) == 0) lua_pushboolean(L, ev->key.flags & 4);
        count = (ev->key.flags & 3) == 0 ? 3 : 2;
        free(ev);
        critical_section_exit(&paramQueueLock);
        return count;
    case 1:
        lua_pushstring(L, mouseevents[ev->mouse.flags >> 6]);
        lua_pushinteger(L, ev->mouse.x);
        lua_pushinteger(L, ev->mouse.y);
        if ((ev->mouse.flags >> 6) == 3) lua_pushinteger(L, (ev->mouse.flags & 0x3F) ? 1 : -1);
        else lua_pushinteger(L, ev->mouse.flags & 0x3F);
        free(ev);
        critical_section_exit(&paramQueueLock);
        return 4;
    case 2: case 3:
        lua_pushstring(L, ev->type == 2 ? "timer" : "alarm");
        lua_pushinteger(L, ev->timer);
        free(ev);
        critical_section_exit(&paramQueueLock);
        return 2;
    case 255:
        free(ev);
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
    default: return 0;
    }
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
    struct event * ev;
    int count = lua_gettop(L);
    critical_section_enter_blocking(&paramQueueLock);
    ev = malloc(sizeof(struct event));
    ev->next = NULL;
    ev->type = 255;
    if (eventQueue_back != NULL) eventQueue_back->next = ev;
    eventQueue_back = ev;
    if (eventQueue_front == NULL) eventQueue_front = ev;
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
    struct event * ev;
    critical_section_enter_blocking(&paramQueueLock);
    ev = malloc(sizeof(struct event));
    ev->next = NULL;
    ev->type = ud ? 3 : 2;
    ev->timer = id;
    if (eventQueue_back != NULL) eventQueue_back->next = ev;
    eventQueue_back = ev;
    if (eventQueue_front == NULL) eventQueue_front = ev;
    critical_section_exit(&paramQueueLock);
    __sev();
    return 0;
}

int os_startTimer(lua_State *L) {
    lua_pushinteger(L, add_alarm_in_ms(luaL_checknumber(L, 1) * 1000, timer, NULL, 1));
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
    lua_pushinteger(L, add_alarm_in_ms(luaL_checknumber(L, 1) * 1000, timer, (void*)1, 1));
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

int os_print(lua_State *L) {
    for (int i = 1; i <= lua_gettop(L); i++) {
        switch (lua_type(L, i)) {
            case LUA_TNIL: printf("nil\t"); break;
            case LUA_TBOOLEAN: printf(lua_toboolean(L, i) ? "true\t" : "false\t"); break;
            case LUA_TNUMBER: printf("%.14g\t", lua_tonumber(L, i)); break;
            case LUA_TSTRING: printf("%s\t", lua_tostring(L, i)); break;
            default: printf("%s: %p\t", lua_typename(L, lua_type(L, i)), lua_topointer(L, i)); break;
        }
    }
    printf("\n");
    return 0;
}

const luaL_Reg os_lib[] = {
    {"getComputerID", os_getComputerID},
    {"getComputerLabel", os_getComputerLabel},
    {"setComputerLabel", os_setComputerLabel},
    {"queueEvent", os_queueEvent},
    {"clock", os_clock},
    {"startTimer", os_startTimer},
    {"cancelTimer", os_cancelTimer},
    {"time", os_time},
    {"epoch", os_epoch},
    {"day", os_day},
    {"setAlarm", os_setAlarm},
    {"cancelAlarm", os_cancelAlarm},
    {"shutdown", os_shutdown},
    {"reboot", os_reboot},
    {"print", os_print},
    {NULL, NULL}
};
