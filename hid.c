#include "tusb_config.h"
#include "tusb.h"
#include "bsp/board.h"
#include "pico/stdlib.h"
#include "class/hid/hid.h"
#include "pico/sync.h"
#include "term.h"
#include <lua.h>

extern lua_State *paramQueue;
extern critical_section_t paramQueueLock;

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+
#if CFG_TUH_HID_KEYBOARD

CFG_TUSB_MEM_SECTION static hid_keyboard_report_t usb_keyboard_report;
uint8_t const keycode2ascii[128][2] = {HID_KEYCODE_TO_ASCII};

// look up new key in previous keys
static inline bool find_key_in_report(hid_keyboard_report_t const *p_report, uint8_t keycode) {
    for (uint8_t i = 0; i < 6; i++) {
        if (p_report->keycode[i] == keycode) return true;
    }

    return false;
}

static inline void process_kbd_report(hid_keyboard_report_t const *p_new_report) {
    static hid_keyboard_report_t prev_report = {0, 0, {0}}; // previous report to check key released

    //------------- example code ignore control (non-printable) key affects -------------//
    for (uint8_t i = 0; i < 6; i++) {
        if (p_new_report->keycode[i]) {
            if (find_key_in_report(&prev_report, p_new_report->keycode[i])) {
                // exist in previous report means the current key is holding
            } else {
                // not existed in previous report means the current key is pressed
                bool const is_shift =
                        p_new_report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
                uint8_t ch = keycode2ascii[p_new_report->keycode[i]][is_shift ? 1 : 0];
                lua_State *param;
                char tmp[2] = {ch, 0};
                critical_section_enter_blocking(&paramQueueLock);
                param = lua_newthread(paramQueue);
                lua_pushstring(param, "key");
                lua_pushinteger(param, p_new_report->keycode[i]);
                lua_pushboolean(param, 0);
                if (ch) {
                    param = lua_newthread(paramQueue);
                    lua_pushstring(param, "char");
                    lua_pushstring(param, tmp);
                }
                critical_section_exit(&paramQueueLock);
                __sev();
            }
        }
        // TODO example skips key released
    }

    prev_report = *p_new_report;
}

void tuh_hid_keyboard_mounted_cb(uint8_t dev_addr) {
    // application set-up
    printf("A Keyboard device (address %d) is mounted\r\n", dev_addr);

    tuh_hid_keyboard_get_report(dev_addr, &usb_keyboard_report);
}

void tuh_hid_keyboard_unmounted_cb(uint8_t dev_addr) {
    // application tear-down
    printf("A Keyboard device (address %d) is unmounted\r\n", dev_addr);
}

// invoked ISR context
void tuh_hid_keyboard_isr(uint8_t dev_addr, xfer_result_t event) {
    (void) dev_addr;
    (void) event;
}

#endif

#if CFG_TUH_HID_MOUSE

CFG_TUSB_MEM_SECTION static hid_mouse_report_t usb_mouse_report;

void cursor_movement(int8_t x, int8_t y, int8_t wheel) {
    //------------- X -------------//
    if (x < 0) {
        //printf(ANSI_CURSOR_BACKWARD(%d), (-x)); // move left
    } else if (x > 0) {
        //printf(ANSI_CURSOR_FORWARD(%d), x); // move right
    } else {}

    //------------- Y -------------//
    if (y < 0) {
        //printf(ANSI_CURSOR_UP(%d), (-y)); // move up
    } else if (y > 0) {
        //printf(ANSI_CURSOR_DOWN(%d), y); // move down
    } else {}

    //------------- wheel -------------//
    if (wheel < 0) {
        //printf(ANSI_SCROLL_UP(%d), (-wheel)); // scroll up
    } else if (wheel > 0) {
        //printf(ANSI_SCROLL_DOWN(%d), wheel); // scroll down
    } else {}
}

static inline void process_mouse_report(hid_mouse_report_t const *p_report) {
    static hid_mouse_report_t prev_report = {0};

    //------------- button state  -------------//
    uint8_t button_changed_mask = p_report->buttons ^prev_report.buttons;
    if (button_changed_mask & p_report->buttons) {
        /*printf(" %c%c%c ",
               p_report->buttons & MOUSE_BUTTON_LEFT ? 'L' : '-',
               p_report->buttons & MOUSE_BUTTON_MIDDLE ? 'M' : '-',
               p_report->buttons & MOUSE_BUTTON_RIGHT ? 'R' : '-');*/
    }

    //------------- cursor movement -------------//
    cursor_movement(p_report->x, p_report->y, p_report->wheel);
}


void tuh_hid_mouse_mounted_cb(uint8_t dev_addr) {
    // application set-up
    printf("A Mouse device (address %d) is mounted\r\n", dev_addr);
}

void tuh_hid_mouse_unmounted_cb(uint8_t dev_addr) {
    // application tear-down
    printf("A Mouse device (address %d) is unmounted\r\n", dev_addr);
}

// invoked ISR context
void tuh_hid_mouse_isr(uint8_t dev_addr, xfer_result_t event) {
    (void) dev_addr;
    (void) event;
}

#endif


void hid_task(void) {
    uint8_t const addr = 1;

#if CFG_TUH_HID_KEYBOARD
    if (tuh_hid_keyboard_is_mounted(addr)) {
        if (!tuh_hid_keyboard_is_busy(addr)) {
            process_kbd_report(&usb_keyboard_report);
            tuh_hid_keyboard_get_report(addr, &usb_keyboard_report);
        }
    }
#endif

#if CFG_TUH_HID_MOUSE
    if (tuh_hid_mouse_is_mounted(addr)) {
        if (!tuh_hid_mouse_is_busy(addr)) {
            process_mouse_report(&usb_mouse_report);
            tuh_hid_mouse_get_report(addr, &usb_mouse_report);
        }
    }
#endif
}

void inputCore() {
    tusb_init();
    gpio_put(25, 0);
    while (!changed);
    unsigned char report = 0;
    while (1) {
        tuh_task();
#if CFG_TUH_HID_KEYBOARD || CFG_TUH_HID_MOUSE
        hid_task();
#endif
        if (changed) {
            mutex_enter_blocking(&screenLock);
            redrawTerm();
            mutex_exit(&screenLock);
            changed = 0;
        }
        sleep_ms(50);
        report = !report;
        gpio_put(25, report);
    }
}