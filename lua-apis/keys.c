#include <lua.h>
#include <lauxlib.h>

// A number entry in the read only table
typedef struct
{
  const char *name;
  lua_Number value;
} luaR_value_entry;

const luaR_value_entry keys_values[] = {
    {"one", 2},
    {"two", 3},
    {"three", 4},
    {"four", 5},
    {"five", 6},
    {"six", 7},
    {"seven", 8},
    {"eight", 9},
    {"nine", 10},
    {"zero", 11},
    {"minus", 12},
    {"equals", 13},
    {"backspace", 14},
    {"tab", 15},
    {"q", 16},
    {"w", 17},
    {"e", 18},
    {"r", 19},
    {"t", 20},
    {"y", 21},
    {"u", 22},
    {"i", 23},
    {"o", 24},
    {"p", 25},
    {"leftBracket", 26},
    {"rightBracket", 27},
    {"enter", 28},
    {"leftCtrl", 29},
    {"a", 30},
    {"s", 31},
    {"d", 32},
    {"f", 33},
    {"g", 34},
    {"h", 35},
    {"j", 36},
    {"k", 37},
    {"l", 38},
    {"semicolon", 39},
    {"apostrophe", 40},
    {"grave", 41},
    {"leftShift", 42},
    {"backslash", 43},
    {"z", 44},
    {"x", 45},
    {"c", 46},
    {"v", 47},
    {"b", 48},
    {"n", 49},
    {"m", 50},
    {"comma", 51},
    {"period", 52},
    {"slash", 53},
    {"rightShift", 54},
    {"leftAlt", 56},
    {"space", 57},
    {"capsLock", 58},
    {"f1", 59},
    {"f2", 60},
    {"f3", 61},
    {"f4", 62},
    {"f5", 63},
    {"f6", 64},
    {"f7", 65},
    {"f8", 66},
    {"f9", 67},
    {"f10", 68},
    {"numLock", 69},
    {"scollLock", 70},
    {"numPad7", 71},
    {"numPad8", 72},
    {"numPad9", 73},
    {"numPadSubtract", 74},
    {"numPad4", 75},
    {"numPad5", 76},
    {"numPad6", 77},
    {"numPadAdd", 78},
    {"numPad1", 79},
    {"numPad2", 80},
    {"numPad3", 81},
    {"numPad0", 82},
    {"numPadDecimal", 83},
    {"f11", 87},
    {"f12", 88},
    {"f13", 100},
    {"f14", 101},
    {"f15", 102},
    {"numPadEquals", 141},
    {"numPadEnter", 156},
    {"rightCtrl", 157},
    {"rightAlt", 184},
    {"pause", 197},
    {"home", 199},
    {"up", 200},
    {"pageUp", 201},
    {"left", 203},
    {"right", 205},
    {"end", 207},
    {"down", 208},
    {"pageDown", 209},
    {"insert", 210},
    {"delete", 211},
    {NULL, 0}
};

static int keys_getName(lua_State *L) {
    int key = luaL_checkinteger(L, 1);
    for (const luaR_value_entry * e = keys_values; e->name != NULL; e++) {
        if (e->value == key) {
            lua_pushstring(L, e->name);
            return 1;
        }
    }
    return 0;
}

const luaL_Reg keys_funcs[] = {
    {"getName", keys_getName},
    {NULL, NULL}
};