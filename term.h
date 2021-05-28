#include "lib.h"
#include "pico/sync.h"
#define TERM_WIDTH 80
#define TERM_HEIGHT 24

extern mutex_t screenLock;
extern unsigned char screen[TERM_HEIGHT*TERM_WIDTH];
extern unsigned char colors[TERM_HEIGHT*TERM_WIDTH];
extern unsigned int palette[16];
extern int cursorX, cursorY, cursorBlink;
extern int changed;
extern library_t term_lib;
extern void termInit();
extern void termClose();
extern void redrawTerm();