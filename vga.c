#include "term.h"
#include <stdio.h>

// This is NOT VGA - just for testing

static unsigned char fgmap[16] = {97, 96, 95, 94, 93, 92, 91, 90, 37, 36, 35, 34, 33, 32, 31, 30};
#define fgmap(n) fgmap[n & 0x0F]
#define bgmap(n) (fgmap[n >> 4] + 10)

void redrawTerm() {
    //return;
    unsigned char lastc = colors[0];
    printf("\x1b[?25l\x1b[2J\x1b[%d;%dm", fgmap(lastc), bgmap(lastc));
    for (int y = 0; y < TERM_HEIGHT; y++) {
        printf("\x1b[%d;1H", y + 1);
        for (int x = 0; x < TERM_WIDTH; x++) {
            if (colors[y*TERM_WIDTH+x] != lastc) {
                lastc = colors[y*TERM_WIDTH+x];
                printf("\x1b[%d;%dm", fgmap(lastc), bgmap(lastc));
            }
            printf("%c", screen[y*TERM_WIDTH+x]);
        }
    }
    printf("\x1b[%d;%dH", cursorY + 1, cursorX + 1);
    if (cursorBlink) printf("\x1b[?25h");
    fflush(stdout);
}
