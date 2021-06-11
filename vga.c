#include "term.h"
#include "pico/stdio_uart.h"
#include <stdio.h>

// This is NOT VGA - just for testing

static unsigned char fgmap[16] = {97, 96, 95, 94, 93, 92, 91, 90, 37, 36, 35, 34, 33, 32, 31, 30};
#define fgmap(n) fgmap[n & 0x0F]
#define bgmap(n) (fgmap[n >> 4] + 10)

void redrawTerm() {
    //return;
    char buf[4096];
    char * cur = buf;
    unsigned char lastc = colors[0];
    cur += sprintf(cur, "\x1b[?25l\x1b[2J\x1b[%d;%dm", fgmap(lastc), bgmap(lastc));
    for (int y = 0; y < TERM_HEIGHT; y++) {
        cur += sprintf(cur, "\x1b[%d;1H", y + 1);
        for (int x = 0; x < TERM_WIDTH; x++) {
            if (colors[y*TERM_WIDTH+x] != lastc) {
                lastc = colors[y*TERM_WIDTH+x];
                cur += sprintf(cur, "\x1b[%d;%dm", fgmap(lastc), bgmap(lastc));
            }
            cur += sprintf(cur, "%c", screen[y*TERM_WIDTH+x]);
            if (cur > buf + 4000) {
                uart_write_blocking(uart0, buf, cur - buf);
                cur = buf;
            }
        }
    }
    cur += sprintf(cur, "\x1b[%d;%dH", cursorY + 1, cursorX + 1);
    if (cursorBlink) cur += sprintf(cur, "\x1b[?25h");
    uart_write_blocking(uart0, buf, cur - buf);
    fflush(stdout);
}
