/*
 * Copyright (c) 2022 Rumbledethumps
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "tusb.h"

// Example of naive keycode mapping.

uint8_t kb_scan[8];

static const uint8_t CBM_TO_HID[] = {
    HID_KEY_1, HID_KEY_GRAVE, HID_KEY_CONTROL_LEFT, HID_KEY_ESCAPE,              // 0-3
    HID_KEY_SPACE, HID_KEY_ALT_LEFT, HID_KEY_Q, HID_KEY_2,                       // 4-7
    HID_KEY_3, HID_KEY_W, HID_KEY_A, HID_KEY_SHIFT_LEFT,                         // 8-11
    HID_KEY_Z, HID_KEY_S, HID_KEY_E, HID_KEY_4,                                  // 12-15
    HID_KEY_5, HID_KEY_R, HID_KEY_D, HID_KEY_X,                                  // 16-19
    HID_KEY_C, HID_KEY_F, HID_KEY_T, HID_KEY_6,                                  // 20-23
    HID_KEY_7, HID_KEY_Y, HID_KEY_G, HID_KEY_V,                                  // 24-27
    HID_KEY_B, HID_KEY_H, HID_KEY_U, HID_KEY_8,                                  // 28-31
    HID_KEY_9, HID_KEY_I, HID_KEY_J, HID_KEY_N,                                  // 32-35
    HID_KEY_M, HID_KEY_K, HID_KEY_O, HID_KEY_0,                                  // 36-39
    HID_KEY_MINUS, HID_KEY_P, HID_KEY_L, HID_KEY_COMMA,                          // 40-43
    HID_KEY_PERIOD, HID_KEY_SEMICOLON, HID_KEY_BRACKET_LEFT, HID_KEY_EQUAL,      // 44-47
    HID_KEY_BACKSLASH, HID_KEY_BRACKET_RIGHT, HID_KEY_APOSTROPHE, HID_KEY_SLASH, // 48-51
    HID_KEY_SHIFT_RIGHT, HID_KEY_END, HID_KEY_PAGE_DOWN, HID_KEY_HOME,           // 52-55
    HID_KEY_DELETE, HID_KEY_ENTER, HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_DOWN,      // 56-59
    HID_KEY_F1, HID_KEY_F3, HID_KEY_F5, HID_KEY_F7,                              // 60-63
    HID_KEY_F11                                                                  // 64 (RESTORE)
};

void kb_init()
{
    // Using GP16-17 for stdio
    stdio_uart_init_full(uart0, 115200, 16, 17);

    // RESTORE key not part of matrix
    // pin 1 to ground, pin 3 to GP18
    gpio_set_dir(18, GPIO_IN);
    gpio_pull_up(18);
    gpio_init(18);

    // "row data" pins 20-13 on GP0-7
    for (uint i = 0; i < 8; i++)
    {
        gpio_set_dir(i, GPIO_IN);
        gpio_pull_up(i);
        gpio_init(i);
    }

    // "column" pins 12-5 on GP8-15
    for (uint i = 8; i < 16; i++)
    {
        gpio_set_dir(i, GPIO_IN);
        gpio_put(i, false);
        gpio_disable_pulls(i);
        gpio_init(i);
    }

    // activate first column
    gpio_set_dir(8, GPIO_OUT);
}

void kb_task()
{
    for (uint col = 0; col < 8; col++)
    {
        // It takes some time between column select and data ready.
        // Next column is selected before we process the current row.
        busy_wait_us_32(10);
        uint8_t row_data = gpio_get_all();
        gpio_set_dir(8 + col, GPIO_IN);
        if (col == 7)
            gpio_set_dir(8, GPIO_OUT);
        else
            gpio_set_dir(9 + col, GPIO_OUT);

        // Save for later
        kb_scan[col] = row_data;
    }
}

hid_keyboard_modifier_bm_t kb_report(uint8_t keycode[6])
{
    uint8_t modifier = 0;
    size_t code_count = 0;

    if (!gpio_get(18)) // RESTORE key
        keycode[code_count++] = CBM_TO_HID[64];

    for (uint col = 0; col < 8; col++)
    {
        uint8_t row_data = kb_scan[col];
        for (uint row = 0; row < 8; row++)
        {
            if (!(row_data & (1u << row)))
            {
                if (code_count < 6)
                {
                    // 3 keycodes is ok, >=4 may be a decode error
                    uint8_t code = CBM_TO_HID[row * 8 + col];
                    keycode[code_count] = code;
                }
                else
                {
                    // phantom condition
                    keycode[0] = keycode[1] = keycode[2] =
                        keycode[3] = keycode[4] = keycode[5] = 1;
                }
                code_count++;
            }
        }
    }

    return modifier;
}
