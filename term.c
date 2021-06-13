#include "term.h"
#include <lauxlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

mutex_t screenLock;
int cursorX = 0, cursorY = 0, cursorBlink = 0;
int changed = 0;
unsigned char current_colors = 0xF0;
unsigned char screen[TERM_HEIGHT*TERM_WIDTH];
unsigned char colors[TERM_HEIGHT*TERM_WIDTH];
unsigned int palette[16] = {
    0xF0F0F0,
    0xF2B233,
    0xE57FD8,
    0x99B2F2,
    0xDEDE6C,
    0x7FCC19,
    0xF2B2CC,
    0x4C4C4C,
    0x999999,
    0x4C99B2,
    0xB266E5,
    0x3366CC,
    0x7F664C,
    0x57A64E,
    0xCC4C4C,
    0x111111
};

void termInit() {
    mutex_init(&screenLock);
    mutex_enter_blocking(&screenLock);
    memset(screen, ' ', TERM_HEIGHT * TERM_WIDTH);
    memset(colors, current_colors, TERM_HEIGHT * TERM_WIDTH);
    changed = 1;
    mutex_exit(&screenLock);
}

void termClose() {
    
}

int term_write(lua_State *L) {
    size_t len = 0;
    const char * str = luaL_checklstring(L, 1, &len);
    if (cursorY < 0 || cursorY >= TERM_HEIGHT) return 0;
    mutex_enter_blocking(&screenLock);
    for (; len > 0 && cursorX < TERM_WIDTH; len--, str++) {
        if (cursorX >= 0) {
            screen[cursorY*TERM_WIDTH+cursorX] = *str;
            colors[cursorY*TERM_WIDTH+cursorX++] = current_colors;
        }
    }
    changed = 1;
    mutex_exit(&screenLock);
    return 0;
}

int term_scroll(lua_State *L) {
    int lines = luaL_checkinteger(L, 1);
    mutex_enter_blocking(&screenLock);
    if (lines > 0 ? (unsigned)lines >= TERM_HEIGHT : (unsigned)-lines >= TERM_HEIGHT) {
        // scrolling more than the height is equivalent to clearing the screen
        memset(screen, ' ', TERM_HEIGHT * TERM_WIDTH);
        memset(colors, current_colors, TERM_HEIGHT * TERM_WIDTH);
        changed = 1;
    } else if (lines > 0) {
        memmove(screen, screen + lines * TERM_WIDTH, (TERM_HEIGHT - lines) * TERM_WIDTH);
        memset(screen + (TERM_HEIGHT - lines) * TERM_WIDTH, ' ', lines * TERM_WIDTH);
        memmove(colors, colors + lines * TERM_WIDTH, (TERM_HEIGHT - lines) * TERM_WIDTH);
        memset(colors + (TERM_HEIGHT - lines) * TERM_WIDTH, current_colors, lines * TERM_WIDTH);
        changed = 1;
    } else if (lines < 0) {
        memmove(screen - lines * TERM_WIDTH, screen, (TERM_HEIGHT + lines) * TERM_WIDTH);
        memset(screen, ' ', -lines * TERM_WIDTH);
        memmove(colors - lines * TERM_WIDTH, colors, (TERM_HEIGHT + lines) * TERM_WIDTH);
        memset(colors, current_colors, -lines * TERM_WIDTH);
        changed = 1;
    }
    mutex_exit(&screenLock);
    return 0;
}

int term_setCursorPos(lua_State *L) {
    mutex_enter_blocking(&screenLock);
    cursorX = luaL_checkinteger(L, 1) - 1;
    cursorY = luaL_checkinteger(L, 2) - 1;
    changed = 1;
    mutex_exit(&screenLock);
    return 0;
}

int term_setCursorBlink(lua_State *L) {
    mutex_enter_blocking(&screenLock);
    cursorBlink = lua_toboolean(L, 1);
    changed = 1;
    mutex_exit(&screenLock);
    return 0;
}

int term_getCursorPos(lua_State *L) {
    lua_pushinteger(L, cursorX + 1);
    lua_pushinteger(L, cursorY + 1);
    return 2;
}

int term_getSize(lua_State *L) {
    lua_pushinteger(L, TERM_WIDTH);
    lua_pushinteger(L, TERM_HEIGHT);
    return 2;
}

int term_clear(lua_State *L) {
    mutex_enter_blocking(&screenLock);
    memset(screen, ' ', TERM_HEIGHT * TERM_WIDTH);
    memset(colors, current_colors, TERM_HEIGHT * TERM_WIDTH);
    changed = 1;
    mutex_exit(&screenLock);
    return 0;
}

int term_clearLine(lua_State *L) {
    if (cursorY < 0 || cursorY >= TERM_HEIGHT) return 0;
    mutex_enter_blocking(&screenLock);
    memset(screen + (cursorY * TERM_WIDTH), ' ', TERM_HEIGHT * TERM_WIDTH);
    memset(colors + (cursorY * TERM_WIDTH), current_colors, TERM_HEIGHT * TERM_WIDTH);
    changed = 1;
    mutex_exit(&screenLock);
    return 0;
}

int log2i(int num) {
    int retval;
    for (retval = 0; num; retval++) num = num >> 1;
    return retval - 1;
}

int term_setTextColor(lua_State *L) {
    current_colors = (current_colors & 0xF0) | (log2i(luaL_checkinteger(L, 1)) & 0x0F);
    return 0;
}

int term_setBackgroundColor(lua_State *L) {
    current_colors = (current_colors & 0x0F) | (log2i(luaL_checkinteger(L, 1)) << 4);
    return 0;
}

int term_isColor(lua_State *L) {
    lua_pushboolean(L, true);
    return 1;
}

int term_getTextColor(lua_State *L) {
    lua_pushinteger(L, 1 >> (current_colors & 0x0F));
    return 1;
}

int term_getBackgroundColor(lua_State *L) {
    lua_pushinteger(L, 1 >> (current_colors >> 4));
    return 1;
}

unsigned char htoi(char c, unsigned char d) {
    if (c >= '0' && c <= '9') return c - '0';
    else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return d;
}

int term_blit(lua_State *L) {
    size_t len, fl, bl;
    const char * str = luaL_checklstring(L, 1, &len);
    const char * fg = luaL_checklstring(L, 2, &fl);
    const char * bg = luaL_checklstring(L, 3, &bl);
    if (len != fl || fl != bl) luaL_error(L, "Arguments must be the same length");
    if (cursorY < 0 || cursorY >= TERM_HEIGHT) return 0;
    mutex_enter_blocking(&screenLock);
    for (; len > 0 && cursorX < TERM_WIDTH; len--, str++, fg++, bg++) {
        if (cursorX >= 0) {
            screen[cursorY*TERM_WIDTH+cursorX] = *str;
            colors[cursorY*TERM_WIDTH+cursorX++] = htoi(*bg, 15) << 4 | htoi(*fg, 0);
        }
    }
    changed = 1;
    mutex_exit(&screenLock);
    return 0;
}

int term_getPaletteColor(lua_State *L) {
    int color = log2i(lua_tointeger(L, 1));
    if (color > 15) luaL_error(L, "Invalid color");
    lua_pushnumber(L, (palette[color] >> 16) / 255.0);
    lua_pushnumber(L, ((palette[color] >> 8) & 0xFF) / 255.0);
    lua_pushnumber(L, ((palette[color] >> 16) & 0xFF) / 255.0);
    return 3;
}

int term_setPaletteColor(lua_State *L) {
    int color = log2i(lua_tointeger(L, 1));
    if (color > 15) luaL_error(L, "Invalid color");
    mutex_enter_blocking(&screenLock);
    if (lua_isnoneornil(L, 3)) palette[color] = luaL_checkinteger(L, 2);
    else palette[color] = (unsigned int)(luaL_checknumber(L, 2) * 255) << 16 | (unsigned int)(luaL_checknumber(L, 3) * 255) << 16 | (unsigned int)(luaL_checknumber(L, 4) * 255);
    changed = 1;
    mutex_exit(&screenLock);
    return 0;
}

const luaL_Reg term_lib[] = {
    {"write", term_write},
    {"scroll", term_scroll},
    {"setCursorPos", term_setCursorPos},
    {"setCursorBlink", term_setCursorBlink},
    {"getCursorPos", term_getCursorPos},
    {"getSize", term_getSize},
    {"clear", term_clear},
    {"clearLine", term_clearLine},
    {"setTextColour", term_setTextColor},
    {"setTextColor", term_setTextColor},
    {"setBackgroundColour", term_setBackgroundColor},
    {"setBackgroundColor", term_setBackgroundColor},
    {"isColour", term_isColor},
    {"isColor", term_isColor},
    {"getTextColour", term_getTextColor},
    {"getTextColor", term_getTextColor},
    {"getBackgroundColour", term_getBackgroundColor},
    {"getBackgroundColor", term_getBackgroundColor},
    {"blit", term_blit},
    {"getPaletteColor", term_getPaletteColor},
    {"getPaletteColour", term_getPaletteColor},
    {"setPaletteColor", term_setPaletteColor},
    {"setPaletteColour", term_setPaletteColor},
    {NULL, NULL}
};
