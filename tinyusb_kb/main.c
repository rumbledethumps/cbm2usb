/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "pico/stdlib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tusb.h"
#include "usb_descriptors.h"
#include "get_serial.h"

void hid_task(void);

// declares for src/kb*.c
extern hid_keyboard_modifier_bm_t kb_report(uint8_t keycode[6]);
extern void kb_init(void);
extern void kb_task(void);

/*------------- MAIN -------------*/
int main(void)
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    usb_serial_init();
    tud_init(BOARD_TUD_RHPORT);
    kb_init();

    while (1)
    {
        tud_task();
        kb_task();
        hid_task();
    }

    return 0;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

// Every 8ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void)
{
    // Poll every 10ms
    const uint32_t interval_ms = 8;
    static absolute_time_t start_us = {0};

    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(now, start_us) > 0)
        return;
    start_us = delayed_by_us(now, interval_ms * 1000);

    uint8_t report_id = 0;
    uint8_t keycode[6] = {0};

    if (tud_suspended())
    {
        // Wake up host if we are in suspend mode
        // and REMOTE_WAKEUP feature is enabled by host
        uint8_t modifier = kb_report(keycode);
        if (modifier || keycode[0])
            tud_remote_wakeup();
    }
    else if (tud_hid_n_ready(ITF_NUM_KEYBOARD))
    {
        uint8_t modifier = kb_report(keycode);
        tud_hid_n_keyboard_report(ITF_NUM_KEYBOARD, report_id, modifier, keycode);
    }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    // TODO not Implemented
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)report_id;

    // keyboard interface
    if (instance == ITF_NUM_KEYBOARD)
    {
        // Set keyboard LED e.g Capslock, Numlock etc...
        if (report_type == HID_REPORT_TYPE_OUTPUT)
        {
            // bufsize should be (at least) 1
            if (bufsize < 1)
                return;

            uint8_t const kbd_leds = buffer[0];

            if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
            {
                gpio_put(PICO_DEFAULT_LED_PIN, true);
            }
            else
            {
                gpio_put(PICO_DEFAULT_LED_PIN, false);
            }
        }
    }
}
