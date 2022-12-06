/*
 * Copyright (c) 2022 Rumbledethumps
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "tusb.h"

// Simple example to show API for these tests.
// Button from GPIO2 to ground acts as right arrow key.

void kb_init()
{
    gpio_set_dir(2, GPIO_IN);
    gpio_pull_up(2);
    gpio_init(2);
}

void kb_task()
{
}

uint8_t kb_report(uint8_t keycode[6])
{
    uint8_t modifier = 0;
    if (!gpio_get(2))
        keycode[0] = HID_KEY_ARROW_RIGHT;
    return modifier;
}
